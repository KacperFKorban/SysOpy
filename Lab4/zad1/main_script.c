#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <zconf.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

int pid = -1;

void obslugaTSTP(int signum) {
    if(pid != -1) {
        kill(pid, SIGKILL);
        printf("Oczekuję na CTRL+Z - kontynuacja albo CTR+C - zakończenie programu \n");
        pid = -1;
    } else {
        pid = fork();
    }
    if(pid != 0) signal(SIGTSTP, obslugaTSTP);
    return;
}

void obslugaINT(int signum) {
    if(pid != -1) {
        kill(pid, SIGKILL);
    }
    printf("Odebrano SIGINT \n");
    exit(0);
}

int main(int argc, char* argv[]) {

    struct sigaction action;
    action.sa_handler = obslugaINT;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);

    signal(SIGTSTP, obslugaTSTP);

    pid = fork();

    while(pid != 0) {
        sleep(1);
    }

    if(pid == 0) {
        execlp("./date_printer", "./date_printer", NULL);
    }

    return 0;
}