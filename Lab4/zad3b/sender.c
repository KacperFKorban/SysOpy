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
int n;
int can_send = 1;

void send_counted_signal(int mode, int pid) {
	union sigval value;
	switch(mode) {
		case 0:
			kill(pid, SIGUSR1);
			break;
		case 1:
			sigqueue(pid, SIGUSR1, value);
			break;
		case 2:
			kill(pid, SIGRTMIN);
			break;
	}
}

void send_end_signal(int mode, int pid) {
	union sigval value;
	switch(mode) {
		case 0:
			kill(pid, SIGUSR2);
			break;
		case 1:
			sigqueue(pid, SIGUSR2, value);
			break;
		case 2:
			kill(pid, SIGRTMIN+1);
			break;
	}
}

void counting_handler(int signum, siginfo_t *info, void *ptr) {
	counter++;
	if(info->si_code == SI_QUEUE){
        printf("Odebrano sygnal nr %d \n",info->si_value.sival_int);
	}
	can_send = 1;
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
			sigaction(SIGRTMIN, &a, NULL);
			break;	
	}
}

void end_handler(int signum) {
	printf("Sender: Wyslano %d sygnalow, a odebrano %d sygnalow\n", n, counter);
	exit(0);
}

void set_end_signal(int mode) {
	struct sigaction a;
	sigemptyset(&a.sa_mask);
	a.sa_flags = 0;
	a.sa_handler = end_handler;
	switch(mode) {
		case 0:
			sigaction(SIGUSR2, &a, NULL);
			break;
		case 1:
			sigaction(SIGUSR2, &a, NULL);
			break;
		case 2:
			sigaction(SIGRTMIN+1, &a, NULL);
			break;
	}
}

int main(int argc, char *argv[]) {
	if(argc != 4) return -1;

	pid_t pid;
	int mode;

	if(sscanf(argv[1], "%d", &pid) != 1) return -1;
	if(sscanf(argv[2], "%d", &n) != 1) return -1;
	if(strcmp(argv[3], "KILL") == 0) mode = 0;
	else if(strcmp(argv[3], "SIGQUEUE") == 0) mode = 1;
	else if(strcmp(argv[3], "SIGRT") == 0) mode = 2;
	else return -1;

	sigset_t set;
	sigfillset(&set);

	sigdelset(&set, SIGUSR1);
	sigdelset(&set, SIGUSR2);
	sigdelset(&set, SIGRTMIN);
	sigdelset(&set, SIGRTMIN+1);

	set_end_signal(mode);
	set_counting_signal(mode);

	int i;
	for(i = 0; i < n; i++) {
		send_counted_signal(mode, pid);
		can_send = 0;
		while(!can_send) {
			
		}
	}

	send_end_signal(mode, pid);
	while(1) {

	}

	return 0;
}