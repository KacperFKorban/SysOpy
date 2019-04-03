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

int main(int argc, char *argv[]) {
	if(argc != 2) return -1;

	FILE *fp;
	if(fp = fopen(argv[1], "r") == NULL) return -1;

	

	while(fscanf(fp_main, "%s", prog1) == 2) {

    }

	return 0;
}