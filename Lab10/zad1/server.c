#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "common.h"

int web_socket;
int local_socket;
int epoll;
char *local_path;
int id;

pthread_t ping;
pthread_t command;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
Client clients[CLIENT_MAX];
int clients_amount = 0;

void handle_signal(int signo) {
    exit(1);
}

size_t get_file_size(const char *file_name) {
    int fd;
    if((fd = open(file_name, O_RDONLY)) == -1) {
        return -1;
    }
    struct stat stats;
    fstat(fd, &stats);
    size_t size = (size_t) stats.st_size;
    close(fd);
    return size;
}

size_t read_whole_file(const char *file_name, char *buffer) {
    size_t size = get_file_size(file_name);
    if(size == -1) {
        return -1;
    }
    FILE *file;
    if((file = fopen(file_name, "r")) == NULL) {
        fprintf(stderr, "Unable to open file \n");
        return -1;
    }
    size_t read_size;
    if((read_size = fread(buffer, sizeof(char), size, file)) != size) {
        fprintf(stderr, "Unable to read file\n");
        return -1;
    }
    fclose(file);
    return read_size;
}

int in(void *const a, void *const pbase, size_t total_elems, size_t size, __compar_fn_t cmp) {
    char *base_ptr = (char *) pbase;
    if(total_elems > 0) {
        for(int i = 0; i < total_elems; ++i) {
            if((*cmp)(a, (void *) (base_ptr + i * size)) == 0) return i;
        }
    }
    return -1;
}

int cmp_name(char *name, Client *client) {
    return strcmp(name, client->name);
}

void delete_socket(int socket) {
    epoll_ctl(epoll, EPOLL_CTL_DEL, socket, NULL);
    shutdown(socket, SHUT_RDWR);
    close(socket);
}

void delete_client(int i) {
    delete_socket(clients[i].fd);

    free(clients[i].name);

    clients_amount--;
    for(int j = i; j < clients_amount; ++j)
        clients[j] = clients[j + 1];

}

void unregister_client(char *client_name) {
    pthread_mutex_lock(&mutex);
    int i = in(client_name, clients, (size_t) clients_amount, sizeof(Client), (__compar_fn_t) cmp_name);
    if(i >= 0) {
        delete_client(i);
        printf("Client %s unregistered\n", client_name);
    }
    pthread_mutex_unlock(&mutex);
}

void *ping_routine(void *arg) {
    uint8_t message_type = PING;
    while(1) {
        pthread_mutex_lock(&mutex);
        for(int i = 0; i < clients_amount; ++i) {
            if(clients[i].active_counter != 0) {
                printf("num: %d \n",clients[i].active_counter );
                printf("Client \"%s\" doesn't respond. Removing from registered clients\n", clients[i].name);
                delete_client(i--);
            } else {
                if(write(clients[i].fd, &message_type, 1) != 1)
                    return NULL;

                clients[i].active_counter++;
            }
        }
        pthread_mutex_unlock(&mutex);
        sleep(10);
    }
}

void send_msg(int type, int len, request_t *req, int i) {
    if(write(clients[i].fd, &type, 1) != 1) return;
    if(write(clients[i].fd, &len, 2) != 2) return;
    if(write(clients[i].fd, req, len) != len) return;
}

void *handle_terminal(void *arg) {

    while(1) {
        char buffer[256];
        memset(buffer,0, 256);
        printf("Enter command: \n");
        fgets( buffer,256,stdin);
        char file_buffer[10240];
        memset( file_buffer, '\0', sizeof(char)*10240);
        int scan_res = sscanf(buffer, "%s", file_buffer);
        printf("state: %d \n", scan_res);
        request_t req;
        if(scan_res == 1) {
            id++;
            printf("REQUEST ID: %d \n", id);
            printf("%s \n", file_buffer);
            memset(req.text, 0, sizeof(req.text));
            int status = read_whole_file(file_buffer, req.text);
            req.ID = id;
            if(strlen(file_buffer) <= 0){
                printf("EMPTY FILE \n");
                continue;
            }
            if(status < 0) {
                printf("WRONG FILE \n");
                continue;
            }
            int i = 0;
            int sent = 0;
            for(i = 0; i < clients_amount; i++) {
                if(clients[i].reserved == 0) {
                    printf("Request sent to %s \n", clients[i].name);
                    clients[i].reserved = 1;
                    send_msg(REQUEST, sizeof(req), &req, i);
                    sent = 1;
                    break;
                }
            }
            if(!sent) {
                i = 0;
                if(clients[i].reserved > -1){
                    clients[i].reserved  ++;
                    send_msg(REQUEST, sizeof(req), &req, i);
                }
            }
        }
    }
}

void handle_connection(int socket) {
    int client = accept(socket, NULL, NULL);
    if(client == -1) return;

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;
    event.data.fd = client;

    if(epoll_ctl(epoll, EPOLL_CTL_ADD, client, &event) == -1) return;

}

void register_client(char *client_name, int socket) {
    uint8_t message_type;
    pthread_mutex_lock(&mutex);
    if(clients_amount == CLIENT_MAX) {
        message_type = FAILSIZE;
        if(write(socket, &message_type, 1) != 1)
            return;
        delete_socket(socket);
    } else {
        int exists = in(client_name, clients, (size_t) clients_amount, sizeof(Client), (__compar_fn_t) cmp_name);
        if(exists != -1) {
            message_type = WRONGNAME;
            if(write(socket, &message_type, 1) != 1)
                return;
            delete_socket(socket);
        } else {
            clients[clients_amount].fd = socket;
            clients[clients_amount].name = malloc(strlen(client_name) + 1);
            clients[clients_amount].active_counter = 0;
            clients[clients_amount].reserved = 0;
            strcpy(clients[clients_amount++].name, client_name);
            message_type = SUCCESS;
            if(write(socket, &message_type, 1) != 1)
                return;
        }
    }
    pthread_mutex_unlock(&mutex);
}

void handle_msg(int socket) {
    uint8_t message_type;
    uint16_t message_size;

    if(read(socket, &message_type, TYPE_SIZE) != TYPE_SIZE) raise_error(" Could not read message type\n");
    if(read(socket, &message_size, LEN_SIZE) != LEN_SIZE) raise_error(" Could not read message size\n");
    char *client_name = malloc(message_size);

    switch (message_type) {
        case REGISTER: {
            if(read(socket, client_name, message_size) != message_size) return;
            register_client(client_name, socket);
            break;
        }
        case UNREGISTER: {
            if(read(socket, client_name, message_size) != message_size) return;
            unregister_client(client_name);
            break;
        }
        case RESULT: {
            printf("SERVER GOT RESULT \n");

            if(read(socket, client_name, message_size) < 0)
                return;
            int size;
            if(read(socket, &size, sizeof(int)) < 0)
                return;
            char* result = malloc(size);
            memset(result, 0, size);
            if(read(socket, result, size) < 0)
                return;

            printf("Computations from: %s : %s \n",client_name, result);

            for(int i = 0; i < CLIENT_MAX; i++){
                if(clients[i].reserved > 0 && strcmp(client_name, clients[i].name) == 0){
                    clients[i].reserved --;
                    clients[i].active_counter = 0;
                    printf("Client %s is free now \n", client_name);
                }
            }
            free(result);
            break;
        }
        case PONG: {
            if(read(socket, client_name, message_size) != message_size) return;
            pthread_mutex_lock(&mutex);
            int i = in(client_name, clients, (size_t) clients_amount, sizeof(Client), (__compar_fn_t) cmp_name);
            if(i >= 0) clients[i].active_counter = clients[i].active_counter == 0 ? 0 : clients[i].active_counter-1;
            pthread_mutex_unlock(&mutex);
            break;
        }
        default:
            printf("Unknown message\n");
            break;
    }
    free(client_name);
}

void init(char *port, char *path) {

    struct sigaction act;
    act.sa_handler = handle_signal;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);

    for(int i = 0; i < CLIENT_MAX; i++) {
        clients[i].reserved = -1;
    }

    uint16_t port_num = (uint16_t) atoi(port);
    local_path = path;

    struct sockaddr_in web_address;
    memset(&web_address, 0, sizeof(struct sockaddr_in));
    web_address.sin_family = AF_INET;
    web_address.sin_addr.s_addr = htonl(INADDR_ANY);
    web_address.sin_port = htons(port_num);

    if((web_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) return;

    int yes = 1;
    if(setsockopt(web_socket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
        exit(1);
    }

    if(bind(web_socket, (const struct sockaddr *) &web_address, sizeof(web_address))) return;

    if(listen(web_socket, 64) == -1) return;

    struct sockaddr_un local_address;
    local_address.sun_family = AF_UNIX;

    sprintf(local_address.sun_path, "%s", local_path);

    if((local_socket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) return;
    if(bind(local_socket, (const struct sockaddr *) &local_address, sizeof(local_address))) return;
    if(listen(local_socket, 64) == -1) return;

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLPRI;
    if((epoll = epoll_create1(0)) == -1) return;
    event.data.fd = -web_socket;
    if(epoll_ctl(epoll, EPOLL_CTL_ADD, web_socket, &event) == -1) return;
    event.data.fd = -local_socket;
    if(epoll_ctl(epoll, EPOLL_CTL_ADD, local_socket, &event) == -1) return;


    if(pthread_create(&ping, NULL, ping_routine, NULL) != 0) return;
    if(pthread_create(&command, NULL, handle_terminal, NULL) != 0) return;
}

void quit() {
    pthread_cancel(ping);
    pthread_cancel(command);
    close(web_socket);
    close(local_socket);
    unlink(local_path);
    close(epoll);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    if(argc != 3)
        return -1;
    if(atexit(quit) == -1)
        return -1;

    init(argv[1], argv[2]);

    struct epoll_event event;
    while(1) {
        if(epoll_wait(epoll, &event, 1, -1) == -1)
            return -1;
        if(event.data.fd < 0)
            handle_connection(-event.data.fd);
        else
            handle_msg(event.data.fd);
    }
}