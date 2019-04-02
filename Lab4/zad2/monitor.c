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
#include <signal.h>

const char format[] = "_%Y-%m-%d %H:%M:%S";

int running = 1;
int stopped = 0;
int killed = 0;
pid_t *pids;
int pids_iter;

void end_process() {
    int i;
    for(i = 0; i < pids_iter; i++) {
        kill(pids[i], SIGTSTP);
    }
    running = 0;
}

void obslugaINT(int signum) {
    end_process();
    return;
}

void obslugaUSR1(int signum) {
    stopped = 1;
    signal(SIGUSR1, obslugaUSR1);
    return;
}

void obslugaUSR2(int signum) {
    stopped = 0;
    signal(SIGUSR2, obslugaUSR2);
    return;
}

void obslugaTSTP(int signum) {
    killed = 1;
    return;
}

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

int monitor_with_mem(char * file_name, int frequency) {
    wrapped_block *wb = calloc(1, sizeof(wrapped_block));
    if(copy_to_wb(wb, file_name) != 0) exit(-1);
    struct stat st;
    if(stat(file_name, &st) != 0) exit(-1);
    wb->mtime = st.st_mtime;
    char new_file_name[128];
    char mtime_str[128];
    int time_iter = 0;
    int counter = 0;
    while(!killed) {
        sleep(frequency);
        if(!stopped) {
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
        }
        time_iter++;
    }
    free_wb(wb);
    exit(counter);
}

int main(int argc, char *argv[]) {
    if(argc != 2) return -1;

    char *file_name = calloc (128, sizeof(char));
    int frequency;

    FILE *fp;
    if((fp = fopen(argv[1], "r")) == NULL) return -1;

    pids = calloc(128, sizeof(pid_t));
    pids_iter = 0;

    while(fscanf(fp, "%s %d", file_name, &frequency) == 2) {
        pids[pids_iter] = fork();
        if(pids[pids_iter] == 0) {
            signal(SIGUSR1, obslugaUSR1);
            signal(SIGUSR2, obslugaUSR2);
            signal(SIGTSTP, obslugaTSTP);
            killed = 0;
            stopped = 0;
            monitor_with_mem(file_name, frequency);
        }
        printf("%d %s %d %d\n", pids_iter, file_name, frequency, pids[pids_iter]);
        pids_iter++;
    }

    signal(SIGINT, obslugaINT);

    char *input = calloc(128, sizeof(char));
    while(running) {
        scanf("%s", input); 
        if(strcmp(input, "LIST") == 0) {
            int i;
            for(i = 0; i < pids_iter; i++) {
                printf("%d ", pids[i]);
            }
            printf("\n");
        } else if(strcmp(input, "END") == 0) {
            end_process();
        } else {
            if(strcmp(input, "STOP") == 0) {
                scanf("%s", input);
                if(strcmp(input, "ALL") == 0) {
                    int i;
                    for(i = 0; i < pids_iter; i++) {
                        kill(pids[i], SIGUSR1);
                    }
                } else {
                    int i = atoi(input);
                    kill(pids[i], SIGUSR1);
                }
            } else if(strcmp(input, "START") == 0) {
                scanf("%s", input);
                if(strcmp(input, "ALL") == 0) {
                    int i;
                    for(i = 0; i < pids_iter; i++) {
                        kill(pids[i], SIGUSR2);
                    }
                } else {
                    int i = atoi(input);
                    kill(pids[i], SIGUSR2);
                }
            }
        }
    }

    int i;
    for(i = 0; i < pids_iter; i++) {
        int stat_loc;
        waitpid(pids[i], &stat_loc, 0);
        int numer_of_copies = (stat_loc/256) == 255 ? 0 : (stat_loc/256);
        printf("Process no. %d created %d copies\n", pids[i], numer_of_copies);
    }

    free(pids);
    free(input);
    free(file_name);
    fclose(fp);

    return 0;
}