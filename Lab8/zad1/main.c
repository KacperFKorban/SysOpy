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
#include <math.h>
#include <pthread.h>

int **pic = NULL;
int picH;
int picW;
int maxVal;

double **filter = NULL;
int filterS;

int **res;

int *threadsArgs = NULL;
int threadsNumber;

int max(int a, int b) {
    return a > b ? a : b;
}

int min(int a, int b) {
    return a < b ? a : b;
}

void quit() {
    if(pic != NULL) {
        for(int i = 0; i < picH; i++) {
            free(pic[i]);
        }
        free(pic);
    }
    if(filter != NULL) {
        for(int i = 0; i < filterS; i++) {
            free(filter[i]);
        }
        free(filter);
    }
    if(res != NULL) {
        for(int i = 0; i < picH; i++) {
            free(res[i]);
        }
        free(res);
    }
    if(threadsArgs != NULL) {
        free(threadsArgs);
    }
}

int calcNewValue(int x, int y) {
    double sum = 0;
    for(int i = 0; i < filterS; i++) {
        for(int j = 0; j < filterS; j++) {
            int fst = max(0, x - ceil(filterS / 2.0) + i);
            int snd = max(0, y - ceil(filterS / 2.0) + j);
            sum += fst < picH && snd < picW ? pic[fst][snd] * filter[i][j] : 0;
        }
    }
    return sum;
}

struct timeval *calcInterleaved(void* args) {
    int k = *((int*)args);

    struct timeval *start = calloc(1, sizeof(struct timeval));
    struct timeval *end = calloc(1, sizeof(struct timeval));
    struct timeval *diff = calloc(1, sizeof(struct timeval));

    gettimeofday(start, NULL);
    for(int i = k; i < picW; i += threadsNumber) {
        for(int j = 0; j < picH; j++) {
            res[j][i] = abs(calcNewValue(j, i));
        }
    }

    gettimeofday(end, NULL);
    timersub(end, start, diff);
    free(start);
    free(end);
    return diff;
}

struct timeval *calcBlocked(void* args) {
    int k = *((int*)args);

    struct timeval *start = calloc(1, sizeof(struct timeval));
    struct timeval *end = calloc(1, sizeof(struct timeval));
    struct timeval *diff = calloc(1, sizeof(struct timeval));

    gettimeofday(start, NULL);
    int from = ceil(k * picW / ((double) threadsNumber));
    int to = ceil((k + 1) * picW / ((double) threadsNumber)) - 1;
    for(int i = from; i <= to; i ++) {
        for(int j = 0; j < picH; j++) {
            res[j][i] = abs(calcNewValue(j, i));
        }
    }

    gettimeofday(end, NULL);
    timersub(end, start, diff);
    free(start);
    free(end);
    return diff;
}

void loadImage(FILE *fp) {
    if(getc(fp) != 'P' || getc(fp) != '2') return;
    while(getc(fp) != '\n');
    while(getc(fp) == '#') {
        while(getc(fp) != '\n');
    }
    fseek(fp, -1, SEEK_CUR);

    fscanf(fp, "%d", &picW);
    fscanf(fp, "%d", &picH);
    fscanf(fp, "%d", &maxVal);

    pic = calloc(picH, sizeof(int*));
    res = calloc(picH, sizeof(int*));
    for(int i = 0; i < picH; i++) {
        pic[i] = calloc(picW, sizeof(int));
        res[i] = calloc(picW, sizeof(int));
    }

    for(int i = 0; i < picH; i++) {
        for(int j = 0; j < picW; j++) {
            fscanf(fp, "%d", &pic[i][j]);
        }
    }
}

void loadFilter(FILE *fp) {
    fscanf(fp, "%d", &filterS);

    filter = calloc(filterS, sizeof(double*));
    for(int i = 0; i < filterS; i++) {
        filter[i] = calloc(filterS, sizeof(double));
    }

    for(int i = 0; i < filterS; i++) {
        for(int j = 0; j < filterS; j++) {
            fscanf(fp, "%lf", &filter[i][j]);
        }
    }
}

void writeRes(FILE *fp) {
    fprintf(fp, "P2\n%d %d\n%d\n", picW, picH, 255);
    for(int i = 0; i < picH; i++) {
        for(int j = 0; j < picW; j++) {
            fprintf(fp, "%d ", res[i][j]);
        }
    }
}

int main(int argc, char *argv[]) {
    if(argc != 6) return -1;

    threadsNumber = atoi(argv[1]);
    int block = 0;
    threadsArgs = calloc(threadsNumber, sizeof(int));

    if(strcmp(argv[2], "block") == 0) {
        block = 1;
    } else if(strcmp(argv[2], "interleaved") == 0) {
        block = 0;
    } else {
        return -1;
    }

    atexit(quit);

    FILE *inFile = fopen(argv[3], "r");
    loadImage(inFile);
    fclose(inFile);
    FILE *filterFile = fopen(argv[4], "r");
    loadFilter(filterFile);
    fclose(filterFile);
    FILE *outFile = fopen(argv[5], "w"); 

    printf("Mode: %s\n", argv[2]);
    printf("Number of threads: %d\n", threadsNumber);
    printf("Filter size: %d\n", filterS);

    struct timeval *start = calloc(1, sizeof(struct timeval));
    struct timeval *end = calloc(1, sizeof(struct timeval));
    struct timeval *diff = calloc(1, sizeof(struct timeval));

    gettimeofday(start, NULL);
    pthread_t tids[threadsNumber];
    for(int i = 0; i < threadsNumber; i++) {
        threadsArgs[i] = i;
        if(block) {
            pthread_create(&tids[i], NULL, (void*)calcBlocked, &threadsArgs[i]);
        } else {
            pthread_create(&tids[i], NULL, (void*)calcInterleaved, &threadsArgs[i]);
        }
    }

    struct timeval *threadTime;
    for(int i = 0; i < threadsNumber; i++) {
        pthread_join(tids[i], (void*)&threadTime);
        printf("Thread tid: %ld worked for %ld.%06ld\n", tids[i], threadTime->tv_sec, threadTime->tv_usec);
    }

    gettimeofday(end, NULL);
    timersub(end, start, diff);
    free(start);
    free(end);

    printf("Total time: %ld.%06ld\n", diff->tv_sec, diff->tv_usec);

    writeRes(outFile);
    fclose(outFile);
}