#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <zconf.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>
#include <time.h>

int counter = 0;
int pid;

void counting_handler(int signum, siginfo_t *info, void *ptr) {
	counter++;
	pid = info->si_pid;
}

void set_counting_signal(int mode) {
	struct sigaction a;
	sigemptyset(&a.sa_mask);
	a.sa_flags = SA_SIGINFO;
	a.sa_sigaction = &counting_handler;
	switch(mode) {
		case 0:
			sigaction(SIGUSR1, &a, NULL);
			break;
		case 1:
			sigaction(SIGUSR1, &a, NULL);
			break;
		case 2:
			sigaddset(&a.sa_mask,SIGRTMIN+1);
			sigaction(SIGRTMIN, &a, NULL);
			break;	
	}
}

void end_handler_kill(int signum, siginfo_t *info, void *ptr) {
    int i;
    for(i = 0; i < counter; i++){
        kill(pid, SIGUSR1);
    }
    kill(pid, SIGUSR2);
    
    printf("Catcher: Odebrano %d sygnalow\n", counter);
    exit(0);
}

void end_handler_sigqueue(int signum, siginfo_t *info, void *ptr) {
	int i;
    union sigval value;
    for(i = 0; i < counter; i++){
        value.sival_int = i;
        sigqueue(pid, SIGUSR1, value);
    }
    value.sival_int = counter;
    sigqueue(pid, SIGUSR2, value);

    printf("Catcher: Odebrano %d sygnalow\n", counter);
	exit(0);
}

void end_handler_sigrt(int signum, siginfo_t *info, void *ptr) {
	int i;
    for(i = 0; i < counter; i++){
        kill(pid, SIGRTMIN);
    }
    kill(pid, SIGRTMIN+1);

	printf("Catcher: Odebrano %d sygnalow\n", counter);
	exit(0);
}

void set_end_signal(int mode) {
	struct sigaction a;
	sigemptyset(&a.sa_mask);
	a.sa_flags = SA_SIGINFO;
	switch(mode) {
		case 0:
			a.sa_sigaction = &end_handler_kill;
			sigaction(SIGUSR2, &a, NULL);
			break;
		case 1:
			a.sa_sigaction = &end_handler_sigqueue;
			sigaction(SIGUSR2, &a, NULL);
			break;
		case 2:
			a.sa_sigaction = &end_handler_sigrt;
			sigaction(SIGRTMIN+1, &a, NULL);
			break;
	}
}

int main(int argc, char *argv[]) {
	if(argc != 2) return -1;

	int mode;
	if(strcmp(argv[1], "KILL") == 0) mode = 0;
	else if(strcmp(argv[1], "SIGQUEUE") == 0) mode = 1;
	else if(strcmp(argv[1], "SIGRT") == 0) mode = 2;
	else return -1;

	sigset_t set;
	sigfillset(&set);
	sigdelset(&set, SIGUSR1);
	sigdelset(&set, SIGUSR2);
	sigdelset(&set, SIGRTMIN);
	sigdelset(&set, SIGRTMIN+1);

	set_end_signal(mode);
	set_counting_signal(mode);

	printf("Catcher: Moje pid to %d \n", getpid());

	while(1) {

	}

	return 0;
}