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
#include <sys/time.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include "line.h"

#define key_0 "/trucker_s"
#define key_1 "/loader_s"
#define key_sh_data "/loader_m"
#define key_sh_line "/trucker_m"
#define key_sh_placed "/placed_m"

int quit = 0;

void handler(int signo) {
    quit = 1;
}

int quit2 = 0;

void handler2(int signo) {
    quit2 = 1;
}

int loader(int N, int ID, int MAX) {
    signal(SIGINT, handler2);


    sem_t *sem0 = sem_open(key_0, O_RDWR, 0666, 0);
    sem_t *sem1 = sem_open(key_1, O_RDWR, 0666, 0);

    srand(time(0));

    int sh_linedata = shm_open(key_sh_data, O_RDWR, 0666);
    data * linedata = (data *) mmap (NULL, sizeof(data), PROT_READ | PROT_WRITE, MAP_SHARED , sh_linedata, 0);

    int K = linedata->K;
    
    int sh_line = shm_open(key_sh_line, O_RDWR, 0666);
    assembly_line * line = (assembly_line *) mmap (NULL, K * sizeof(assembly_line), PROT_READ | PROT_WRITE, MAP_SHARED , sh_line, 0);    

    int sh_placed = shm_open(key_sh_placed, O_RDWR | O_CREAT, 0666);
    int * placed = (int *) mmap (NULL, MAX * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED , sh_placed, 0);    

    int package = (rand()%N + 1);
    while(!quit2) {

        sem_wait(sem1);

        

        if(linedata->curr_m + package <= linedata->M) {
            linedata->curr_m += package;
            printf("Loader ID: %d put a package with weight %d\n", getpid(), package);
            
            for(int i = 0; i < linedata->K; i++) {
                if(line[i].line == -1) {
                    line[i].line = package;
                    line[i].pid = getpid();
                    struct timeval current_time;
                    gettimeofday(&current_time,NULL);
                    line[i].tv = current_time;
                    

                    if(i == linedata->K - 1 || linedata->curr_m == linedata->M) {
                        sem_post(sem0);
                    } else {
                        sem_post(sem1);
                    }

                    break;
                }
            }
            package = (rand()%N + 1);
        } else {  
            placed[ID] = 1;
            int res = 0;
            for(int i = 0; i < linedata->K; i++) res += placed[i];
            if(res == MAX) {
                sem_post(sem0);
            } else {
                sem_post(sem1);
            }
        }

    }

    munmap(line, K * sizeof(assembly_line));
    munmap(linedata, sizeof(data));
    sem_close(sem0);
    sem_close(sem1);
    exit(0);
}

void init(int M, int N) {
    
    int sh_placed = shm_open(key_sh_placed, O_RDWR | O_CREAT, 0666);
    ftruncate(sh_placed, M * sizeof(int));
    int * placed = (int *) mmap (NULL, M * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED , sh_placed, 0);    

    pid_t loaders[M];
    for(int i = 0; i < M; i++) {
        loaders[i] = fork();
        if(loaders[i] == 0) {
            loader(N, i, M);
        }
    }
    while(!quit) {}

    for(int i = 0; i < M; i++) {
        kill(loaders[i], SIGINT);
    }
    shm_unlink(key_sh_placed);
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