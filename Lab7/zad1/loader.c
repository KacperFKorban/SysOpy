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

void handler(int sig) {
    quit = 1;
}

int quit2 = 0;

void handler2(int sig) {
    quit2 = 1;
}

void loader(int N, int ID, int MAX) {
    signal(SIGINT, handler2);

    key_t key = ftok("/tmp", 'a');
    int semaphores_set = semget(key, 2, 0666);

    srand(time(0));
    int sh_line = shmget(key+1, 0, 0666);
    assembly_line * line = (assembly_line *) shmat (sh_line, NULL, 0);
    
    int sh_linedata = shmget(key+2, 0, 0666);
    data * linedata = (data *) shmat (sh_linedata, NULL,  0);

    int sh_placed = shmget(key+3, 0, 0666);
    int * placed = (int *) shmat (sh_placed, NULL,  0);

    int sample_package = (rand()%N + 1);
    while(!quit2) {

        struct sembuf sops;
        sops.sem_flg = SEM_UNDO;
        sops.sem_num = 0;
        sops.sem_op = 0;

        semop(semaphores_set, &sops, 1);

        

        if(linedata->curr_m + sample_package <= linedata->M) {
            linedata->curr_m += sample_package;
            printf("Loader ID: %d put a package with weight %d\n", getpid(), sample_package);
            
            for(int i = 0; i < linedata->K; i++) {
                if(line[i].line == -1) {
                    line[i].line = sample_package;
                    line[i].pid = getpid();
                    struct timeval current_time;
                    gettimeofday(&current_time,NULL);
                    line[i].tv = current_time;
                    

                    if(i == linedata->K - 1 || linedata->curr_m == linedata->M) {
                        sops.sem_num = 0;
                        sops.sem_op = 1;
                        semop(semaphores_set, &sops, 1);
                    } else {
                        sops.sem_num = 1;
                        sops.sem_op = 1;
                        semop(semaphores_set, &sops, 1);
                    }

                    break;
                }
            }
            sample_package = (rand()%N + 1);
        } else {  
            placed[ID] = 1;
            int res = 0;
            for(int i = 0; i < linedata->K; i++) res += placed[i];
            if(res == MAX) {
                sops.sem_num = 0;
                sops.sem_op = 1;
                semop(semaphores_set, &sops, 1);
            } else {
                sops.sem_num = 1;
                sops.sem_op = 1;
                semop(semaphores_set, &sops, 1);
            }
        }

    }

    shmdt((void *) line);
    shmdt((void *) linedata);
    shmdt((void *) placed);
    exit(0);
}

void init(int M, int N) {
    
    key_t key = ftok("/tmp", 'a');
    int sh_placed = shmget(key+3, sizeof(int) * M, IPC_CREAT | 0666);
    int *placed = (int *) shmat (sh_placed, NULL,  0);

    pid_t inits[M];
    for(int i = 0; i < M; i++) {
        inits[i] = fork();
        if(inits[i] == 0) {
            loader(N, i, M);
        }
    }
    while(!quit) {

    }
    shmctl(sh_placed, IPC_RMID, NULL);
    for(int i = 0; i < M; i++) {
        kill(inits[i], SIGINT);
    }
}

int main(int argc, char* argv[]) {

    signal(SIGINT, handler);

    if(argc == 3) {
        int M = atoi(argv[1]);
        int N = atoi(argv[2]);
        init(M, N);
    }

    return 0;
}