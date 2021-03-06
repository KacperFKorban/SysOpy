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

#define QUEUE_MAX 10
#define SERVER "/serverQueue1"

int serverQueue = -1;
int clients_no = 0;

struct client {
  int clientQueue;
  int friends[CLIENT_MAX];
  int curr_friends_number;
  pid_t pid;
};

struct msg {
  long mType;
  pid_t sender;
  char msg[MSG_MAX];
};

struct client clients[CLIENT_MAX];

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

void send(long type, char msg[MSG_MAX], int clientID) {
  if(clientID >= CLIENT_MAX || clientID < 0 || clients[clientID].clientQueue < 0) {
    return;
  }

  mq_send(clients[clientID].clientQueue, msg, MSG_MAX, 11-type);
}

void quit(int signo) {
  int i;
  for(i = 0; i < CLIENT_MAX; i++) {
    if(clients[i].clientQueue != -1) {
      char response[MSG_MAX];
      sprintf(response, "%d;%s", STOP, "stop");
      kill(clients[i].pid, SIGRTMIN);
    }
  }
  mq_close(serverQueue);

  mq_unlink(SERVER);

  printf("Queue deleted\n");
}

void init(int clientPID, char msg[MSG_MAX]) {
  int id;
  for(id = 0; id < CLIENT_MAX; id++) {
    if(clients[id].clientQueue == -1)
      break;
  }

  if(id >= CLIENT_MAX)
    return;

  clients[id].clientQueue = mq_open(msg, O_WRONLY);

  clients[id].pid = clientPID;
  clients[id].curr_friends_number = 0;

  printf("New client initialized with id: %d \n", id);

  char toClient[MSG_MAX];
  sprintf(toClient, "%d", id);
  send(INIT, toClient, id);
  clients_no++;
}

void echo(int clientID, char msg[MSG_MAX]) {
  printf("Echo for client with id: %d is %s\n", clientID, msg);
  char response[MSG_MAX];
  char date[64];
  FILE *f = popen("date", "r");
  fread(date, sizeof(char), 31, f);
  pclose(f);
  sprintf(response, "%s date: %s", msg, date);
  send(ECHO, response, clientID);
}

void stop(int clientID) {
  if(clientID >= 0 && clientID < CLIENT_MAX) {
    clients[clientID].clientQueue = -1;
    clients[clientID].curr_friends_number = 0;
    int i;
    for(i = 0; i < CLIENT_MAX; i++)
      clients[clientID].friends[i] = -1;
    clients_no--;
    printf("Number of working clients: %d \n", clients_no);
    if(clients_no == 0) {
      kill(getpid(), SIGINT);
    }
  }
}

void list(int clientID) {
  printf("List to client with id: %d \n", clientID);
  char response[MSG_MAX], buf[MSG_MAX];
  strcpy(response, "");
  int i;
  for(i = 0; i < CLIENT_MAX; i++) {
    if(clients[i].clientQueue >= 0) {
      sprintf(buf, "ID: %d  queueID: %d \n", i, clients[i].clientQueue);
      strcat(response, buf);
    }
  }
  send(LIST, response, clientID);
}

void makeFriendsList(int clientID, char friends[MSG_MAX]) {
  char *friend = strtok(friends, ":");

  while(friend && clients[clientID].curr_friends_number < CLIENT_MAX) {
    char *s;
    int f = strtol(friend, &s, 10);
    if(f < 0 || f >= CLIENT_MAX || f < 0 || clientID == f) {
      return;
    }
    int found = 0;
    int i;
    for(i = 0; i < clients[clientID].curr_friends_number; i++)
      if(f == clients[clientID].friends[i]) {
        found = 1;
      }
    if(!found) {
      clients[clientID].friends[clients[clientID].curr_friends_number++] = f;
      printf("Friend with id %d has just been added \n", f);
    } else {
      printf("Friend with id %d has been already added \n", f);
    }
    friend = strtok(NULL, ":");
  }
}

void friends(int clientID, char msg[MSG_MAX]) {
  if(clientID >= CLIENT_MAX || clientID < 0 || clients[clientID].clientQueue < 0) {
    return;
  }

  printf("Preparing new list of client's friends \n");

  char friends[MSG_MAX];
  int state = sscanf(msg, "%s", friends);

  if(state != 1)
    return;

  clients[clientID].curr_friends_number = 0;
  int i;
  for (i = 0; i < CLIENT_MAX; i++) {
    clients[clientID].friends[i] = -1;
  }
  
  makeFriendsList(clientID, friends);
}

void add(int clientID, char msg[MSG_MAX]) {
  if(clientID >= CLIENT_MAX || clientID < 0 || clients[clientID].clientQueue < 0) {
    return;
  }

  char friends[MSG_MAX];
  int state = sscanf(msg, "%s", friends);

  if(state != 1)
    return;

  makeFriendsList(clientID, friends);
}

void delete(int clientID, char msg[MSG_MAX]) {
  if(clientID >= CLIENT_MAX || clientID < 0 || clients[clientID].clientQueue < 0) {
    return;
  }

  char list[MSG_MAX];
  int num = sscanf(msg, "%s", list);
  char *elem = NULL;
  int id = -1;
  int i = 0;

  if(num == EOF || num == 0) {
    return;
  } else if(num == 1) {
    elem = strtok(list, ":");
    while(elem != NULL && (clients[clientID].curr_friends_number) > 0) {
      id = (int) strtol(elem, NULL, 10);
      i = 0;
      for(; i < (clients[clientID].curr_friends_number); i++)
        if(id == clients[clientID].friends[i]) break;
      if(i >= (clients[clientID].curr_friends_number))id = -1;
      if(id < CLIENT_MAX && id >= 0 && id != clientID) {
        clients[clientID].friends[i] = clients[clientID].friends[(clients[clientID].curr_friends_number) - 1];
        (clients[clientID].curr_friends_number)--;
      }
      elem = strtok(NULL, ":");
    }
  }

  printf("Friends after deleting: \n");
  int j;
  for (j = 0; j < clients[clientID].curr_friends_number; j++) {
      printf("\t ID: %d\n", clients[clientID].friends[j]);
  }
}

void toall(int clientID, char msg[MSG_MAX]) {
  printf("Server will send messages to all clients \n");

  char response[MSG_MAX];
  char date[32];
  FILE *p = popen("date", "r");
  int check = fread(date, sizeof(char), 31, p);
  if(check == EOF)
    return;
  pclose(p);

  sprintf(response, "message: %s from: %d date: %s \n", msg, clientID, date);

  int i;
  for (i = 0; i < CLIENT_MAX; i++) {
    if(clients[i].clientQueue == -1 || i == clientID)
      continue;
    send(TOALL, response, i);
    kill(clients[i].pid, SIGRTMIN);
  }
}

void toone(int clientID, char msg[MSG_MAX]) {
  printf("Server will send messages to one client \n");
  char date[32];

  char content[MSG_MAX];
  char response[MSG_MAX];

  int addressee;
  FILE *f = popen("date", "r");
  int check = fread(date, sizeof(char), 31, f);
  if(check == EOF)
    return;
  pclose(f);
  int x = sscanf(msg, "%i %s", &addressee, content);
  if(x != 2)
    return;
  sprintf(response, "message %s from: %d date: %s \n", content, clientID, date);

  if(addressee >= CLIENT_MAX || addressee < 0 || clients[addressee].clientQueue < 0 || addressee == clientID) {
    return;
  }

  send(TOONE, response, addressee);
  kill(clients[addressee].pid, SIGRTMIN);
}

void tofriends(int clientID, char msg[MSG_MAX]) {
  printf("Server will send messages to friends of client \n");
  char response[MSG_MAX];
  char date[64];
  FILE *f = popen("date", "r");
  int check = fread(date, sizeof(char), 31, f);
  if(check == EOF)
    return;
  pclose(f);
  sprintf(response, "message: %s from: %d date: %s\n", msg, clientID, date);
  int i;
  for(i = 0; i < clients[clientID].curr_friends_number; i++) {
    int addressee = clients[clientID].friends[i];
    if(clients[clientID].friends[i] == -1)
      continue;
    if(addressee >= CLIENT_MAX || addressee < 0 || clients[addressee].clientQueue < 0) {
      return;
    }
    printf("%d\t", addressee);
    send(TOFRIENDS, response, addressee);
    kill(clients[addressee].pid, SIGRTMIN);
  }
}

void execCmd(char *msg) {
  char *s;
  long type = strtol(strtok(msg, ";"), &s, 10);
  long sender = strtol(strtok(NULL, ";"), &s, 10);

  if(type == STOP) {
    stop(sender);
  } else if(type == INIT) {
    init(sender, strtok(NULL, ";"));
  } else if(type == ECHO) {
    echo(sender, strtok(NULL, ";"));
  } else if(type == FRIENDS) {
    friends(sender, strtok(NULL, ";"));
  } else if(type == LIST) {
    list(sender);
  } else if(type == TOALL) {
    toall(sender, strtok(NULL, ";"));
  } else if(type == TOONE) {
    toone(sender, strtok(NULL, ";"));
  } else if(type == TOFRIENDS) {
    tofriends(sender, strtok(NULL, ";"));
  } else if(type == ADD) {
    add(sender, strtok(NULL, ";"));
  } else if(type == DEL) {
    delete(sender, strtok(NULL, ";"));
  }
}

int main() {
  int i;
  for(i = 0; i < CLIENT_MAX; i++) {
    clients[i].clientQueue = -1;
    clients[i].curr_friends_number = 0;
  }

  struct mq_attr queue_attr;
  queue_attr.mq_maxmsg = QUEUE_MAX;
  queue_attr.mq_msgsize = MSG_MAX;

  struct sigaction act;
  act.sa_handler = quit;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(SIGINT, &act, NULL);

  serverQueue = mq_open(SERVER, O_RDONLY | O_CREAT | O_EXCL, 0666, &queue_attr);
  char content[MSG_MAX];

  while(1) {
    if(mq_receive(serverQueue, content, MSG_MAX, NULL) == -1)
      return -1;
    execCmd(content);

    sleep(1);
  }

  mq_close(serverQueue);

  mq_unlink(SERVER);

  return 0;
}