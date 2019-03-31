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

const char format[] = "%Y-%m-%d %H:%M:%S";

int running = 1;

void obslugaTSTP(int signum) {
    if(running) printf("Oczekuję na CTRL+Z - kontynuacja albo CTR+C - zakończenie programu \n");
    running = 1 - running;
    signal(SIGTSTP, obslugaTSTP);
    return;
}

void obslugaINT(int signum) {
    printf("Odebrano SIGINT \n");
    exit(0);
}

char* formatdate(char* str, time_t val) {
        strftime(str, 36, format, localtime(&val));
        return str;
}

int main(int argc, char* argv[]) {

    struct sigaction action;
    action.sa_handler = obslugaINT;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);

    signal(SIGTSTP, obslugaTSTP);
    time_t time_val;
    char time_str[128];

    while(1) {
        if(running) {
            time_val = time(0);
            printf("%s\n", formatdate(time_str, time_val));
            sleep(1);
        }
    }

    return 0;
}