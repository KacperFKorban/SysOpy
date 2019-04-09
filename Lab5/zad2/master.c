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

	if(argc != 2) return -1;

	if(mkfifo(argv[1], 0666) < 0) return -1;

	int fp;
	if((fp = open(argv[1], O_RDONLY)) < 0) return -1;

	char *line = calloc(128, sizeof(char));
	while(1) {
		if(read(fp, line, 128))
			printf("%s", line);
	}

	free(line);

	close(fp);

	return 0;
}