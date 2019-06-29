#ifndef SOCKETS_COMMON_H
#define SOCKETS_COMMON_H

#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#define MAX_TEXT_SIZE 10240
#define MAX_PATH 108
#define CLIENT_MAX 12
#define TYPE_SIZE 1
#define LEN_SIZE 2
typedef enum message_type {
    REGISTER = 0,
    UNREGISTER = 1,
    SUCCESS = 2,
    FAILSIZE = 3,
    WRONGNAME = 4,
    REQUEST = 5,
    RESULT = 6,
    PING = 7,
    PONG = 8,
} message_type;

typedef struct Client {
    int fd;
    char *name;
    int active_counter;
    int reserved;
} Client;

typedef struct request_t{
    char text[10240];
    int ID;
} request_t;


void raise_error(char* message){
    fprintf(stderr, "%s :: %s \n", message, strerror(errno));
    exit(1);
}

#endif
