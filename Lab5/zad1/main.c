#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <zconf.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>

void dup2_and_close(int *fd, int new0, int new1) {
	if(new0 >= 0) dup2(fd[0], new0);
	if(new1 >= 0) dup2(fd[1], new1);
	close(fd[0]);
	close(fd[1]);
}

int main(int argc, char *argv[]) {
	if(argc != 2) return -1;

	FILE *fp;
	if((fp = fopen(argv[1], "r")) == NULL) return -1;

	int fd[2][2];
	char line[512];
	char* commands[32][32];
	pid_t pids[32];
	while(fgets(line, sizeof(line), fp)) {
		char* cmd = strtok(line, " \n");
		int i, j;
		for(j = 0, i = 0; cmd != NULL; j++, cmd = strtok(NULL, " \n")) {
			if(strcmp(cmd, "|") == 0) {
				commands[i][j] = NULL;
				j = -1;
				i++;
			} else {
				commands[i][j] = cmd;
			}
		}

		int n = i+1;
		for(i = 0; i < n; i++) {
			if(pipe(fd[1]) < 0) return -1;
			
			pids[i] = fork();
			if(pids[i] == 0) {
				if(i > 0) {
					dup2_and_close(fd[0], STDIN_FILENO, -1);
				}
				if(i < n-1) {
					dup2_and_close(fd[1], -1, STDOUT_FILENO);
				}
				execvp(commands[i][0], commands[i]);
				exit(0);
			} else {
				if(i > 0) {
					dup2_and_close(fd[0], -1, -1);
				}
				if(i < n-1) {
					fd[0][0] = fd[1][0];
					fd[0][1] = fd[1][1];
				}
			}
		}

		for(i = 0; i < n-1; i++) {
			int stat_loc;
        	waitpid(pids[i], &stat_loc, 0);
		}
    }

    fclose(fp);

	return 0;
}