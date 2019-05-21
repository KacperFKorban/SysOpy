#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <libgen.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include "line.h"

int quit = 0;

double timediff(struct timeval start , struct timeval end){
    double start_ms, end_ms;

    start_ms = (double)start.tv_sec*1000000 + (double)start.tv_usec;
    end_ms = (double)end.tv_sec*1000000 + (double)end.tv_usec;

    return (double)end_ms - (double)start_ms;
}

void handle(int sig){
    quit = 1;
}

int main(int argc, char* argv[]) {

    if(argc != 4)
        return 1;

    int X = atoi(argv[1]);
    int K = atoi(argv[2]);
    int M = atoi(argv[3]);

    signal(SIGINT, handle);

    key_t key = ftok("/tmp", 'a');
    int semaphores_set = semget(key, 2, IPC_CREAT | 0666);
    

    int sh_line = shmget(key+1, K * sizeof(assembly_line), IPC_CREAT | 0666);
    assembly_line *line = (assembly_line *) shmat (sh_line, NULL,  0);

    int sh_linedata = shmget(key+2, sizeof(data), IPC_CREAT | 0666);
    data * linedata = (data *) shmat (sh_linedata, NULL,  0);

    linedata->curr_m = 0;
    linedata->K = K;
    linedata->M = M;
    for(int i = 0; i < K; i++) {
        line[i].line = -1;
    }

    int load;
    struct sembuf sops;
    sops.sem_flg = SEM_UNDO;
    int i = 0;

    
    while(quit == 0) {
        printf("New truck just came!\n");
        semctl(semaphores_set, 1, SETVAL, 1);

        sops.sem_num = 0;
        sops.sem_op = -1;
        semop(semaphores_set, &sops, 1);

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

        semctl(semaphores_set, 0, SETVAL, 0);
        sleep(1);
    }


    semctl(semaphores_set, 0, IPC_RMID, 0);
    shmctl(sh_line, IPC_RMID, NULL);
    shmctl(sh_linedata, IPC_RMID, NULL);
    return 0;
}
