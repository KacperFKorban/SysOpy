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
#include <mqueue.h>
#include <fcntl.h>

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

#define QUEUE_MAX 10
#define SERVER "/serverQueue1"

int serverQueue = -1;
int clientQueue = -1;
int clientID = -1;
char *queueName;

struct msg {
  long mType;
  pid_t sender;
  char msg[MSG_MAX];
};     

void receive(int signo) {
  char msg[MSG_MAX];
  mq_receive(clientQueue, msg, MSG_MAX, NULL);

  char *s;
  long type = strtol(strtok(msg, ";"), &s, 10);

  if(type == TOALL || type == TOONE || type == TOFRIENDS) {
    printf("A message from a client has come: %s \n", strtok(NULL, ";"));
  } else if(type == STOP) {
    mq_close(clientQueue);
    mq_unlink(queueName);
    exit(0);
  }
}

void send(long type, char content[MSG_MAX]) {
  mq_send(serverQueue, content, MSG_MAX, 11-type);
}

void stop() {
  char msg[MSG_MAX];
  sprintf(msg, "%d;%d;%s", STOP, clientID, "");
  send(STOP, msg);
  exit(0);
}

void quit(int signo) {
  mq_close(clientQueue);
  mq_unlink(queueName);
  printf("Queue deleted");
  stop();
  exit(0);
}

void init() {
  char content[MSG_MAX];

  sprintf(content, "%d;%d;%s", INIT, getpid(), queueName);  
  mq_send(serverQueue, content, MSG_MAX, 11-INIT);

  char response[MSG_MAX];
  
  mq_receive(clientQueue, response, MSG_MAX, NULL);

  sscanf(response, "%d", &clientID);
  printf("Client got ID: %d \n", clientID);
}

void echo(char content[MSG_MAX]) {
  char command[CMD_MAX];
  char toSend[MSG_MAX];

  int state = sscanf(content, "%s %s", command, toSend);
  if(state == EOF || state < 2)
    return;

  char msg[MSG_MAX];
  sprintf(msg, "%d;%d;%s", ECHO, clientID, toSend);

  send(ECHO, msg);

  char response[MSG_MAX];
  mq_receive(clientQueue, response, MSG_MAX, NULL);

  printf("ECHO response: %s \n", response);
}

void list() {
  char msg[MSG_MAX];
  char response[MSG_MAX];

  sprintf(msg, "%d;%d;%s", LIST, clientID, "");

  send(LIST, msg);

  mq_receive(clientQueue, response, MSG_MAX, NULL);

  printf("List of working clients: \n %s \n", response);
}

void friends(char arguments[MSG_MAX]) {
  char command[CMD_MAX];
  char text[MSG_MAX];
  int args_no = sscanf(arguments, "%s %s", command, text);
  if(args_no == EOF || args_no == 0)
    return;
  char msg[MSG_MAX];
  if(args_no == 1){
    sprintf(msg, "%d;%d;%s", FRIENDS, clientID, "n");
    send(FRIENDS, msg);
  }else{
    sprintf(msg, "%d;%d;%s", FRIENDS, clientID, text);
    send(FRIENDS, msg);
  }
}

void add(char arguments[MSG_MAX]) {
  char command[CMD_MAX];
  char list[MSG_MAX];

  int args_no = sscanf(arguments, "%s %s", command, list);
  if(args_no == EOF || args_no == 0 || args_no == 1)
    return;

  char msg[MSG_MAX];
  sprintf(msg, "%d;%d;%s", ADD, clientID, list);

  send(ADD, msg);
}

void del(char arguments[MSG_MAX]) {
  char command[CMD_MAX];
  char list[MSG_MAX];
  int numberOfArguments = sscanf(arguments, "%s %s", command, list);
  if(numberOfArguments == EOF || numberOfArguments == 0 || numberOfArguments == 1)
    return;

  char msg[MSG_MAX];
  sprintf(msg, "%d;%d;%s", DEL, clientID, list);

  send(DEL, msg);
}

void toone(char arguments[MSG_MAX]) {
  char command[CMD_MAX];
  char content[MSG_MAX];
  int addressee;
  int args_no = sscanf(arguments, "%s %d %s", command, &addressee, content);
  if(args_no == EOF || args_no < 3)
    return;
  sprintf(command, "%d %s", addressee, content);

  char msg[MSG_MAX];
  sprintf(msg, "%d;%d;%s", TOONE, clientID, command);
  send(TOONE, msg);
}

void tofriends(char arguments[MSG_MAX]) {
  char command[CMD_MAX], text[MSG_MAX];
  int args_no = sscanf(arguments, "%s %s", command, text);
  if(args_no == EOF || args_no < 2)
    return;
  
  char msg[MSG_MAX];
  sprintf(msg, "%d;%d;%s", TOFRIENDS, clientID, text);
  send(TOFRIENDS, msg);
}

void toall(char arguments[MSG_MAX]) {
  char command[CMD_MAX], text[MSG_MAX];
  int args_no = sscanf(arguments, "%s %s", command, text);
  if(args_no == EOF || args_no < 2)
    return;
  
  char msg[MSG_MAX];
  sprintf(msg, "%d;%d;%s", TOALL, clientID, text);
  send(TOALL, msg);
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

  queueName = malloc(32*sizeof(char));
  sprintf(queueName, "/cli%i%i", getpid(), rand()%1000);

  if((serverQueue = mq_open(SERVER, O_WRONLY)) == -1)
    return -1;

  struct mq_attr queue_attr;
  queue_attr.mq_maxmsg = QUEUE_MAX;
  queue_attr.mq_msgsize = MSG_MAX;

  if((clientQueue = mq_open(queueName, O_RDONLY | O_CREAT | O_EXCL, 0666, &queue_attr)) == -1)
    return -1;
  init();
  printf("Client queue name:\t%s \n", queueName);

  while(1){
    execCmd(fdopen(STDIN_FILENO, "r"));
  }

  mq_close(clientQueue);

  mq_unlink(queueName);

  stop();

  return 0;
}