#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <libgen.h> 
#include <unistd.h>
#include <string.h>
#include <stdint.h>

const char format[] = "_%Y-%m-%d %H:%M:%S";

typedef struct {
    char *table;
    int size;
    time_t mtime;
} wrapped_block;

void free_wb(wrapped_block *wb) {
    if(wb != NULL) {
        if(wb->table != NULL) {
            free(wb->table);
        }
        free(wb);
        wb = NULL;
    }
}

int set_limit(int max_time, int max_memory) {
    struct rlimit rlim;
    rlim.rlim_max = (rlim_t) max_time;
    rlim.rlim_cur = (rlim_t) max_time;
    if(setrlimit(RLIMIT_CPU, &rlim) != 0) return -1;

    rlim.rlim_max = (rlim_t) max_memory;
    rlim.rlim_cur = (rlim_t) max_memory;
    if(setrlimit(RLIMIT_AS, &rlim) != 0) return -1;

    return 0;
}

int copy_from_wb(wrapped_block *wb, char *file_name) {
    FILE *fp;
    char dest[128];
    strcpy(dest, "./archiwum/");
    strcat(dest, file_name);
    if((fp = fopen(dest, "w+")) == NULL) return -1;
    if(fwrite(wb->table, sizeof(char), wb->size, fp) != wb->size * sizeof(char)) return -1;
    fclose(fp);
    return 0;
}

int copy_to_wb(wrapped_block *wb, char *file_name) {
    FILE *fp;
    if(wb == NULL) wb = calloc(1, sizeof(wrapped_block));
    if((fp = fopen(file_name, "r")) == NULL) return -1;
    fseek(fp, 0, SEEK_END);
    wb->size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if(wb->table != NULL) free(wb->table);
    wb->table = calloc(wb->size, sizeof(char));
    fread(wb->table, sizeof(char), wb->size, fp);
    fclose(fp);
    return 0;
}

void cp(char *source, char *file_name) {
    char dest[128];
    strcpy(dest, "./archiwum/");
    strcat(dest, file_name);
    execlp("cp", "cp", source, dest, NULL);
    exit(0);
}

int monitor_with_mem(FILE *fp, char * file_name, int frequency, int wait_seconds) {
    wrapped_block *wb = calloc(1, sizeof(wrapped_block));
    if(copy_to_wb(wb, file_name) != 0) exit(-1);
    struct stat st;
    if(stat(file_name, &st) != 0) exit(-1);
    wb->mtime = st.st_mtime;
    char new_file_name[128];
    char mtime_str[128];
    int time_iter = 0;
    int counter = 0;
    while((time_iter + 1) * frequency < wait_seconds) {
        sleep(frequency);
        stat(file_name, &st);
        if(st.st_mtime != wb->mtime) {
            strcpy(new_file_name, basename(file_name));
            strftime(mtime_str, 100, format, localtime(&(wb->mtime)));
            strcat(new_file_name, mtime_str);
            if(copy_from_wb(wb, new_file_name) != 0) exit(-1);
            if(copy_to_wb(wb, file_name) != 0) exit(-1);
            counter++;
            wb->mtime = st.st_mtime;
        }
        time_iter++;
    }
    free_wb(wb);
    exit(counter);
}

int monitor_with_cp(FILE *fp, char * file_name, int frequency, int wait_seconds) {
    struct stat st;
    if(stat(file_name, &st) != 0) exit(-1);
    time_t mtime = st.st_mtime;
    char new_file_name[128];
    char mtime_str[128];
    int time_iter = 0;
    int counter = 0;

    strcpy(new_file_name, basename(file_name));
    strftime(mtime_str, 100, format, localtime(&mtime));
    strcat(new_file_name, mtime_str);

    pid_t pid = vfork();
    if(pid == 0) cp(file_name, new_file_name);

    while((time_iter + 1) * frequency < wait_seconds) {
        sleep(frequency);
        stat(file_name, &st);
        if(st.st_mtime != mtime) {
            strcpy(new_file_name, basename(file_name));
            strftime(mtime_str, 100, format, localtime(&mtime));
            strcat(new_file_name, mtime_str);
            pid = fork();
            if(pid == 0) cp(file_name, new_file_name);
            counter++;
            mtime = st.st_mtime;
        }
        time_iter++;
    }
    exit(counter);
}

int main(int argc, char *argv[]) {
    if(argc != 6) return -1;

    int wait_seconds;
    if(sscanf(argv[2], "%d", &wait_seconds) != 1) return -1;

    int max_time;
    if(sscanf(argv[4], "%d", &max_time) != 1) return -1;

    int max_memory;
    if(sscanf(argv[5], "%d", &max_memory) != 1) return -1;
    max_memory *= 1048576;

    char *file_name = calloc (128, sizeof(char));
    int frequency;
    int is_mem;
    if(strcmp(argv[3], "mem") == 0) is_mem = 1;
    else if(strcmp(argv[3], "cp") == 0) is_mem = 0;
    else return -1;

    FILE *fp;
    if((fp = fopen(argv[1], "r")) == NULL) return -1;

    pid_t pids[128];
    int pids_iter = 0;

    struct rusage r_usage;

    getrusage(RUSAGE_CHILDREN, &r_usage);


    while(fscanf(fp, "%s %d", file_name, &frequency) == 2) {
        pids[pids_iter] = fork();
        if(is_mem && pids[pids_iter] == 0) {
            if(set_limit(max_time, max_memory) != 0) {
                free(file_name);
                fclose(fp);
                return -1;
            }
            monitor_with_mem(fp, file_name, frequency, wait_seconds);
        } else if(pids[pids_iter] == 0) {
            if(set_limit(max_time, max_memory) != 0) {
                free(file_name);
                fclose(fp);
                return -1;
            }
            monitor_with_cp(fp, file_name, frequency, wait_seconds);
        }
        pids_iter++;
    }
    sleep(wait_seconds);

    struct timeval ru_utime;
    struct timeval ru_stime;
    struct rusage new_r_usage;
    int i;
    for(i = 0; i < pids_iter; i++) {
        int stat_loc;
        waitpid(pids[i], &stat_loc, 0);
        getrusage(RUSAGE_CHILDREN, &new_r_usage);
        timersub(&new_r_usage.ru_stime, &r_usage.ru_stime, &ru_stime);
        timersub(&new_r_usage.ru_utime, &r_usage.ru_utime, &ru_utime);
        printf("Process no. %d created %d copies\n", pids[i], (stat_loc/256));
        printf("System time usage: %ld.%06ld\n", ru_stime.tv_sec, ru_stime.tv_usec);
        printf("User time usage: %ld.%06ld\n", ru_utime.tv_sec, ru_utime.tv_usec);
        r_usage = new_r_usage;
    }

    free(file_name);
    fclose(fp);

    return 0;
}