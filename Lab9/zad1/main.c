#define _OPEN_THREADS
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
#include <errno.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>

int passengers_number;
int carts_number;
int cart_capacity;
int runs_number;
int runs_left;
int curr_id = 0;
int *carts_curr_capacity;
int curr_id_finish = 0;
pthread_t *passengers_tids;
pthread_t *carts_tids;
int still_loading = 0; 
int still_unloading = 0;
int *carts;
int *passengers;

pthread_mutex_t load_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t load_cart_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t load_passenger_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t load_passenger_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t unload_passenger_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t unload_passenger_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t unload_cart_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t unload_cart_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t full_cart_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full_cart_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t empty_cart_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty_cart_cond = PTHREAD_COND_INITIALIZER;

void *cart_fun(void *args) {
    int id = *((int*)args);
    while(1) {
        pthread_mutex_lock(&load_mutex);
        if(runs_left == 0) {
            break;
        } 
        while(id != curr_id) {
            pthread_cond_wait(&load_cart_cond, &load_mutex);
        }
        printf("Cart ID: %d is ready to be loaded\n", id);
        still_loading = 1;
        pthread_cond_broadcast(&load_passenger_cond);
        while(carts_curr_capacity[id] < cart_capacity){
            pthread_cond_wait(&full_cart_cond, &full_cart_mutex);
        }
        printf("Cart ID: %d departs on %d/%d run\n", id, runs_number-runs_left + 1, runs_number);
        if(curr_id == carts_number-1) runs_left--;
        curr_id = (curr_id+1)%carts_number;
        pthread_mutex_unlock(&load_mutex);
        pthread_cond_broadcast(&load_cart_cond);
        usleep(500000);
        pthread_mutex_lock(&unload_cart_mutex);
        while(id != curr_id_finish) {
            pthread_cond_wait(&unload_cart_cond, &unload_cart_mutex);
        }
        printf("Cart ID: %d finished a run and in unloading passengers\n", id);
        still_unloading = 1;
        pthread_cond_broadcast(&unload_passenger_cond);
        while(carts_curr_capacity[id] > 0){
            pthread_cond_wait(&empty_cart_cond, &empty_cart_mutex);
        }
        curr_id_finish = (curr_id_finish+1)%carts_number;
        printf("Cart ID: %d is empty and waiting\n", id);
        pthread_mutex_unlock(&unload_cart_mutex);
        pthread_cond_broadcast(&unload_cart_cond);
    }
    printf("Cart ID: %d is finished\n", id);
    pthread_mutex_unlock(&load_mutex);
    pthread_exit(NULL);
}

void *passenger_fun(void *args) {
    int id = *((int*)args);
    int cart = -1;
    while(1) {
        pthread_mutex_lock(&load_passenger_mutex);
        while(!still_loading) {
            pthread_cond_wait(&load_passenger_cond, &load_passenger_mutex);
        }
        cart = curr_id;
        carts_curr_capacity[cart]++;
        printf("Passenger ID: %d enters Cart ID: %d as %d/%d\n", id, cart, carts_curr_capacity[cart], cart_capacity);
        if(carts_curr_capacity[cart] < cart_capacity) {
            pthread_mutex_unlock(&load_passenger_mutex);
            pthread_cond_broadcast(&load_passenger_cond);
        } else {
            still_loading = 0;
            printf("Passenger ID: %d pressed the button\n", id);
            pthread_mutex_unlock(&load_passenger_mutex);
            pthread_cond_broadcast(&full_cart_cond);
        }

        pthread_mutex_lock(&unload_passenger_mutex);
        while(!still_unloading || cart != curr_id_finish) {
            pthread_cond_wait(&unload_passenger_cond, &unload_passenger_mutex);
        }
        carts_curr_capacity[cart]--;
        printf("Passenger ID: %d leaves Cart ID: %d as %d/%d\n", id, cart, cart_capacity-carts_curr_capacity[cart], cart_capacity);
        if(carts_curr_capacity[cart] > 0) {
            cart = -1;
            pthread_mutex_unlock(&unload_passenger_mutex);
            pthread_cond_broadcast(&unload_passenger_cond);
        } else {
            cart = -1;
            still_unloading = 0;
            pthread_mutex_unlock(&unload_passenger_mutex);
            pthread_cond_broadcast(&empty_cart_cond);
        }
    }
}

int main(int argc, char * argv[]) {
    if(argc != 5) return -1;
    passengers_number = atoi(argv[1]);
    carts_number = atoi(argv[2]);
    cart_capacity = atoi(argv[3]);
    runs_number = atoi(argv[4]);

    carts_tids = calloc(carts_number, sizeof(pthread_t));
    passengers_tids = calloc(passengers_number, sizeof(pthread_t));
    runs_left = runs_number;
    carts = calloc(carts_number, sizeof(int));
    passengers = calloc(passengers_number, sizeof(int));
    carts_curr_capacity = calloc(carts_number, sizeof(int));

    for(int i = 0; i < carts_number; i++){
        carts[i] = i;
        carts_curr_capacity[i] = 0;
        pthread_create(&carts_tids[i], NULL, cart_fun, (void*)&carts[i]);
    }
    
    for(int i = 0; i < passengers_number; i++){
        passengers[i] = i;
        pthread_create(&passengers_tids[i], NULL, passenger_fun, (void*)&passengers[i]);
    }

    for(int i = 0; i < carts_number; i++){
        pthread_join(carts_tids[i], NULL);
    }

    for(int i = 0; i < passengers_number; i++){
        pthread_cancel(passengers_tids[i]);
        printf("Passenger ID: %d has finished\n", i);
    }

    free(carts_tids);
    free(passengers_tids);
    free(carts);
    free(passengers);
    free(carts_curr_capacity);

    return 0;
}