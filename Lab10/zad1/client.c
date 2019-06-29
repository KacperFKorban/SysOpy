#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>
#include <string.h>
#include "common.h"
#include <netinet/in.h>
#include <sys/un.h>
#include <endian.h>
#include <arpa/inet.h>
#include <pthread.h>

char *name;
int client_socket;
int ID = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void send_msg(uint8_t message_type) {
    uint16_t message_size = (uint16_t) (strlen(name) + 1);
    if(write(client_socket, &message_type, TYPE_SIZE) != TYPE_SIZE)return;
    if(write(client_socket, &message_size, LEN_SIZE) != LEN_SIZE) return;
    if(write(client_socket, name, message_size) != message_size) return;
}

void* handle_request(void * arg) {

    request_t* req_tmp = (request_t *)arg;
    request_t req;
    memset(req.text,0, sizeof(req.text));
    strcpy(req.text,req_tmp->text);
    int id = req_tmp ->ID;

    char *buffer = malloc(100 + 2 * strlen(req.text));
    char *buffer_res = malloc(100 + 2 * strlen(req.text));
    sprintf(buffer, "echo '%s' | awk '{for(x=1;$x;++x)print $x}' | sort | uniq -c", (char *) req.text);
    FILE *result = popen(buffer, "r");
    int n = fread(buffer, 1, 99 + 2 * strlen(req.text), result);
    buffer[n] = '\0';

    int words_count = 1;
    char *res = strtok(req.text, " ");
    while(strtok(NULL, " ") && res) {
        words_count++;
    }

    sprintf(buffer_res, "ID: %d sum: %d || %s",id, words_count, buffer);
    printf("RES: %s\n", buffer);
    pthread_mutex_lock(&mutex);
    send_msg(RESULT);
    int len = strlen(buffer_res);
    if(write(client_socket,&len, sizeof(int)) != sizeof(int)) return NULL;
    if(write(client_socket, buffer_res, len) != len) return NULL;
    printf("RESULT SENT \n");
    free(buffer);
    free(buffer_res);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void connect_to_server() {
    send_msg(REGISTER);

    uint8_t message_type;
    if(read(client_socket, &message_type, 1) != 1) return;

    switch (message_type) {
        case WRONGNAME:
            return;
        case FAILSIZE:
            return;
        case SUCCESS:
            printf("LOGGED\n");
            break;
        default:
            return;
    }
}

void handle_message() {
    uint8_t message_type;
    pthread_t thread;

    while(1) {
        if(read(client_socket, &message_type, TYPE_SIZE) != TYPE_SIZE) return;
        switch (message_type) {
            case REQUEST:
                printf(" ");
                uint16_t req_len;
                if(read(client_socket, &req_len, 2) <= 0) return;

                request_t req;
                memset( req.text, '\0', sizeof(req.text));
                if(read(client_socket, req.text, req_len) < 0) return;

                pthread_create(&thread, NULL, handle_request, &req);
                pthread_detach(thread);
                break;
            case PING:
                pthread_mutex_lock(&mutex);
                send_msg(PONG);
                pthread_mutex_unlock(&mutex);
                break;
            default:
                break;
        }
    }
}

void handle_signals(int signo) {
    send_msg(UNREGISTER);
    exit(1);
}

void init(char *connection_type, char *server_ip_path, char *port) {

    struct sigaction act;
    act.sa_handler = handle_signals;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);

    if(strcmp("WEB", connection_type) == 0) {
        uint16_t port_num = (uint16_t) atoi(port);
        if(port_num < 1024 || port_num > 65535) return;
        struct sockaddr_in web_address;
        memset(&web_address, 0, sizeof(struct sockaddr_in));
        web_address.sin_family = AF_INET;
        web_address.sin_addr.s_addr = htonl(INADDR_ANY);
        web_address.sin_port = htons(port_num);
        if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) return;
        if(connect(client_socket, (const struct sockaddr *) &web_address, sizeof(web_address)) == -1) return;
        printf("WEB CONNECTED\n");

    } else if(strcmp("LOCAL", connection_type) == 0) {
        char *unix_path = server_ip_path;
        struct sockaddr_un local_address;
        local_address.sun_family = AF_UNIX;
        snprintf(local_address.sun_path, MAX_PATH, "%s", unix_path);
        if((client_socket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) return;

        if(connect(client_socket, (const struct sockaddr *) &local_address, sizeof(local_address)) == -1) return;

    } else {
        return;
    }
}

void quit() {
    send_msg(UNREGISTER);
    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);
}

int main(int argc, char *argv[]) {

    name = argv[1];
    char *connection_type = argv[2];
    char *server_ip = argv[3];
    char *port = NULL;
    if(argc == 5) {
        port = argv[4];
    }

    init(connection_type, server_ip, port);
    connect_to_server();
    handle_message();
    quit();
    return 0;
}