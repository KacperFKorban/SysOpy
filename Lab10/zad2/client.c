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
enum connect_type c_type;
int client_socket;

int request_counter = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t request1_mutex = PTHREAD_MUTEX_INITIALIZER;

int signal_ = 0;

void send_msg(uint8_t message_type, char *value) {
    message_t msg;
    msg.message_type = message_type;
    snprintf(msg.name, 64, "%s", name);
    msg.connect_type = c_type;
    if(value){
        snprintf(msg.value, strlen(value), "%s", value);
        printf("RES: %s\n", msg.value);
    }
    pthread_mutex_lock(&mutex);
    if(write(client_socket, &msg, sizeof(message_t)) != sizeof(message_t)) return;
    pthread_mutex_unlock(&mutex);
}

void* handle_request(void* arg) {

    struct request_t* got_arg = arg;
    struct request_t req;
    strcpy(req.text, got_arg->text);
    req.ID = got_arg->ID;

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
    sprintf(buffer_res, "ID: %d|sum: %d|words count: \n %s",req.ID, words_count, buffer);
    pthread_mutex_lock(&request1_mutex);
    send_msg(RESULT, buffer_res);
    pthread_mutex_unlock(&request1_mutex);
    free(buffer);
    free(buffer_res);
    return NULL;

}

void register_on_server() {
    send_msg(REGISTER, NULL);

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
        if(read(client_socket, &message_type, sizeof(uint8_t)) != sizeof(uint8_t)) return;
        switch (message_type) {
            case REQUEST:
                printf("GOT REQUEST \n");
                request_t req;
                if(read(client_socket, &req, sizeof(request_t)) <= 0) return;
                pthread_create(&thread, NULL, handle_request, &req);
                pthread_detach(thread);
                break;
            case PING:
                pthread_mutex_lock(&request1_mutex);
                send_msg(PONG, NULL);
                pthread_mutex_unlock(&request1_mutex);
                break;
            default:
                printf("Unknown message type \n");
                break;
        }
    }
}

void handle_signals(int signo) {
    signal_++;
    exit(1);
}

void init(char *connection_type, char *server_ip_path, char *port) {

    struct sigaction act;
    act.sa_handler = handle_signals;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);

    if(strcmp("WEB", connection_type) == 0) {

        c_type = WEB;
        uint16_t port_num = (uint16_t) atoi(port);
        if(port_num < 1024 || port_num > 65535) return;

        struct sockaddr_in web_address;
        memset(&web_address, 0, sizeof(struct sockaddr_in));

        web_address.sin_family = AF_INET;
        web_address.sin_addr.s_addr = htonl(
                INADDR_ANY);
        web_address.sin_port = htons(port_num);

        if((client_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return;
        int yes = 1;
         if(setsockopt(client_socket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if(connect(client_socket, (const struct sockaddr *) &web_address, sizeof(web_address)) == -1) return;

    } else if(strcmp("LOCAL", connection_type) == 0) {
        c_type = LOCAL;

        if((client_socket = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) return;

        struct sockaddr_un local_address_bind;
        local_address_bind.sun_family = AF_UNIX;
        snprintf(local_address_bind.sun_path, MAX_PATH, "%s", name);

        if(bind(client_socket, (const struct sockaddr *) &local_address_bind, sizeof(local_address_bind))) return;

        struct sockaddr_un local_address;
        local_address.sun_family = AF_UNIX;
        snprintf(local_address.sun_path, MAX_PATH, "%s", server_ip_path);

        if(connect(client_socket, (const struct sockaddr *) &local_address, sizeof(local_address)) == -1) return;

    } else {
        return;
    }
}

void quit() {
    if(signal_ > 0){
        send_msg(UNREGISTER, NULL);
        unlink(name);
    }

    if(close(client_socket) == -1)
        fprintf(stderr, "\n Could not close Socket\n");
    unlink(name);
}

int main(int argc, char *argv[]) {

    name = argv[1];
    char *connection_type = argv[2];
    char *server_ip = argv[3];
    char *port = NULL;
    if(argc == 5) {
        port = argv[4];
    }

    atexit(quit);
    init(connection_type, server_ip, port);
    register_on_server();
    handle_message();
    return 0;
}