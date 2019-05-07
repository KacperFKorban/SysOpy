#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/param.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>

#define STOP 1
#define FRIENDS 2
#define INIT 3
#define LIST 4
#define ADD 5
#define DEL 6
#define ECHO 7
#define TOONE 8
#define TOFRIENDS 9
#define TOALL 10

#define CLIENT_MAX 100
#define MSG_MAX 100
#define CMD_MAX 100

int serverQueue = -1;
int clientQueue = -1;
int clientID = -1;

struct msg {
  long mType;
  pid_t sender;
  char msg[MSG_MAX];
};

key_t sKey() {
  char *home = getenv("HOME");
  key_t s_key = ftok(home, 'a');
  return s_key;
}

key_t cKey() {
  char *home = getenv("HOME");
  key_t c_key = ftok(home, getpid());
  return c_key;
}

void receive(int signo) {
  struct msg msg;
  msgrcv(clientQueue, &msg, sizeof(struct msg), 0, 0);

  if(msg.mType == TOALL || msg.mType == TOONE || msg.mType == TOFRIENDS) {
    printf("The message from a client has come: %s \n", msg.msg);
  } else if(msg.mType == STOP) {
    msgctl(clientQueue, IPC_RMID, NULL);
    exit(0);
  }
}

void send(long type, char content[MSG_MAX]) {
  struct msg msg;
  msg.mType = type;
  strcpy(msg.msg, content);
  msg.sender = clientID;
  msgsnd(serverQueue, &msg, sizeof(struct msg), IPC_NOWAIT);
}

void stop() {
  send(STOP, "");
  msgctl(clientQueue, IPC_RMID, NULL);
  exit(0);
}

void quit(int signo) {
  msgctl(clientQueue, IPC_RMID, NULL);
  printf("Queue deleted");
  stop();
  sleep(1);
  exit(0);
}

void init() {
  struct msg msg;
  char content[MSG_MAX];
  msg.mType = INIT;
  sprintf(content, "%i", clientQueue);
  strcpy(msg.msg, content);
  msg.sender = getpid();
  msgsnd(serverQueue, &msg, sizeof(struct msg), IPC_NOWAIT);

  msgrcv(clientQueue, &msg, sizeof(struct msg), 0, 0);

  sscanf(msg.msg, "%d", &clientID);
  printf("Client got ID: %d \n", clientID);
}

void echo(char content[MSG_MAX]) {
  char command[CMD_MAX];
  char toSend[MSG_MAX];

  int state = sscanf(content, "%s %s", command, toSend);
  if(state == EOF || state < 2)
    return;
  send(ECHO, toSend);
  struct msg msg;
  msgrcv(clientQueue, &msg, sizeof(struct msg), 0, 0);

  printf("ECHO response: %s \n", msg.msg);
}

void list() {
  send(LIST, "");
  struct msg msg;
  msgrcv(clientQueue, &msg, sizeof(struct msg), 0, 0);

  printf("List of working clients: \n %s \n", msg.msg);
}

void friends(char arguments[MSG_MAX]) {
  char command[CMD_MAX];
  char text[MSG_MAX];
  int args_no = sscanf(arguments, "%s %s", command, text);
  if(args_no == EOF || args_no == 0)
    return;
  if(args_no == 1){
    send(FRIENDS, "n");
  }else{
    send(FRIENDS, text);
  }
}

void add(char arguments[MSG_MAX]) {
  char command[CMD_MAX];
  char list[MSG_MAX];

  int args_no = sscanf(arguments, "%s %s", command, list);
  if(args_no == EOF || args_no == 0 || args_no == 1)
    return;

  send(ADD, list);
}

void del(char arguments[MSG_MAX]) {
  char command[CMD_MAX];
  char list[MSG_MAX];
  int numberOfArguments = sscanf(arguments, "%s %s", command, list);
  if(numberOfArguments == EOF || numberOfArguments == 0 || numberOfArguments == 1)
    return;

  send(DEL, list);
}

void toone(char arguments[MSG_MAX]) {
  char command[CMD_MAX];
  char content[MSG_MAX];
  int addressee;
  int args_no = sscanf(arguments, "%s %d %s", command, &addressee, content);
  if(args_no == EOF || args_no < 3)
    return;
  sprintf(command, "%d %s", addressee, content);
  send(TOONE, command);
}

void tofriends(char arguments[MSG_MAX]) {
  char command[CMD_MAX], text[MSG_MAX];
  int args_no = sscanf(arguments, "%s %s", command, text);
  if(args_no == EOF || args_no < 2)
    return;
  send(TOFRIENDS, text);
}

void toall(char arguments[MSG_MAX]) {
  char command[CMD_MAX], text[MSG_MAX];
  int args_no = sscanf(arguments, "%s %s", command, text);
  if(args_no == EOF || args_no < 2)
    return;
  send(TOALL, text);
}

void readCmd(char *args);

int execCmd(FILE* file) {
  char args[CMD_MAX];
  char command[MSG_MAX];

  if(fgets(args, CMD_MAX * sizeof(char), file) == NULL)
    return EOF;

  int sscanf_state = sscanf(args, "%s", command);

  if(sscanf_state == EOF || sscanf_state == 0)
    return 0;
  if(!strcmp(command, "ECHO")) {
    echo(args);
  } else if(!strcmp(command, "LIST")) {
    list();
  } else if(!strcmp(command, "STOP")) {
    stop();
  } else if(!strcmp(command, "READ")) {
    readCmd(args);
  } else if(!strcmp(command, "FRIENDS")) {
    friends(args);
  } else if(!strcmp(command, "ADD")) {
    add(args);
  } else if(!strcmp(command, "DEL")) {
    del(args);
  } else if(!strcmp(command, "2ONE")) {
    toone(args);
  } else if(!strcmp(command, "2FRIENDS")) {
    tofriends(args);
  } else if(!strcmp(command, "2ALL")) {
    toall(args);
  }
  return 0;
}

void readCmd(char *args) {
  char command[CMD_MAX], fileName[CMD_MAX];
  int numberOfArguments = sscanf(args, "%s %s", command, fileName);
  if(numberOfArguments == EOF || numberOfArguments < 2) {
    return;
  }
  FILE *f = fopen(fileName, "r");
  if(f == NULL)
    return;
  while(execCmd(f) != EOF);
  fclose(f);
}

int main() {
  struct sigaction act;
  act.sa_handler = receive;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(SIGRTMIN, &act, NULL);

  signal(SIGINT, quit);

  if((serverQueue = msgget(sKey(), 0)) == -1)
    return -1;
  if((clientQueue = msgget(cKey(), IPC_CREAT | IPC_EXCL | 0666)) == -1)
    return -1;
  init();
  printf("Server queue ID:\t%d \n", serverQueue);
  while(1){
    execCmd(fdopen(STDIN_FILENO, "r"));
  }
  stop();

  return 0;
}