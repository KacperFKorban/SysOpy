#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <zconf.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>

int main(int argc, char *argv[]) {

	srand(time(0));

	if(argc != 3) return -1;

	int fp;

	if((fp = open(argv[1], O_WRONLY)) < 0) return -1;

	printf("%d\n", getpid());

	int n = atoi(argv[2]);

	while(n--) {
		FILE *dp;
		if((dp = popen("date", "r")) == NULL) return -1;

		char *line = calloc(128, sizeof(char));
		sprintf(line, "%d ", getpid());
		char *tmp = calloc(128, sizeof(char));
		fread(tmp, sizeof(char), 64, dp);
		strcat(line, tmp);
		write(fp, line, 128*sizeof(char));

		pclose(dp);

		free(tmp);
		free(line);

		int st = rand() % 4 + 2;

		if(n != 0) 
			sleep(st);
	}

	close(fp);

	return 0;
}