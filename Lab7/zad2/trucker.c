#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libgen.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pthread.h>
#include "line.h"

#define key_0 "/trucker_s"
#define key_1 "/loader_s"
#define key_sh_data "/loader_m"
#define key_sh_line "/trucker_m"

int quit = 0;

void handle(int signo){
    quit = 1;
}

double timediff(struct timeval start , struct timeval end){
    double start_ms, end_ms;

    start_ms = (double)start.tv_sec*1000000 + (double)start.tv_usec;
    end_ms = (double)end.tv_sec*1000000 + (double)end.tv_usec;

    return (double)end_ms - (double)start_ms;
}

int main(int argc, char* argv[]) {

    if(argc != 4)
        return -1;
    int X = atoi(argv[1]);
    int K = atoi(argv[2]);
    int M = atoi(argv[3]);

    signal(SIGINT, handle);

    sem_t * sem0 = sem_open(key_0, O_RDWR | O_CREAT, 0666, 0);
    sem_t * sem1 = sem_open(key_1, O_RDWR | O_CREAT, 0666, 0);


    int sh_line = shm_open(key_sh_line, O_RDWR | O_CREAT, 0666);
    ftruncate(sh_line, K * sizeof(assembly_line));
    assembly_line * line = (assembly_line *) mmap (NULL, K * sizeof(assembly_line), PROT_READ | PROT_WRITE, MAP_SHARED , sh_line, 0);    

    int sh_data = shm_open(key_sh_data, O_RDWR | O_CREAT, 0666);
    ftruncate(sh_data, sizeof(data));
    data * linedata = (data *) mmap (NULL, sizeof(data), PROT_READ | PROT_WRITE, MAP_SHARED , sh_data, 0);    

    linedata->curr_m = 0;
    linedata->K = K;
    linedata->M = M;
    for(int i = 0; i < K; i++) {
        line[i].line = -1;
    }

    int load;
    int i = 0;
    
    while(!quit) {
        printf("New truck just came!\n");

        sem_post(sem1);
        sem_wait(sem0);

        load = 0;
        struct timeval current_time;
        gettimeofday(&current_time,NULL);
        for(i = 0; i < K && load + line[i].line <= X && line[i].line > 0; i++) {
            load += line[i].line;
            printf("Loading from loader with ID: %d Time diff %f Weight %d/%d\n", line[i].pid, timediff(line[i].tv, current_time), load, X);
        }

        linedata->curr_m -= load;

        for(int j = i; j < K; j++) {
            line[j-i].line = line[j].line;
            line[j-i].pid = line[j].pid;
            line[j-i].tv = line[j].tv;
        }

        for(int j = K-i; j < K; j++) {
            line[j].line = -1;
        }

        printf("Truck is leaving!\n");

        sleep(1);
    }

    sem_unlink(key_0);
    sem_unlink(key_1);
    sem_close(sem0);
    sem_close(sem1);

    shm_unlink(key_sh_line);
    shm_unlink(key_sh_data);
    munmap(line,  K * sizeof(assembly_line));
    munmap(linedata, sizeof(data));
    return 0;
}