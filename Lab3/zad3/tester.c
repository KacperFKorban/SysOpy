#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> 
#include <zconf.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>


int rand_from_to(int min, int max) {
	return rand() % (max - min + 1) + min;
}

int main(int argc, char *argv[]) {
	if(argc != 5) return -1;
	FILE *fp;
	if((fp = fopen(argv[1], "a+")) == NULL) return -1;

	int pmin;
	int pmax;
	int bytes;
	sscanf(argv[2], "%d", &pmin);
    sscanf(argv[3], "%d", &pmax);
    sscanf(argv[4], "%d", &bytes);

    char new_line[128];
    char secs_str[16];
    char mtime_str[128];
    char pid[16];
    struct tm *t;

    srand(time(0));

    while(1) {
    	int secs = rand_from_to(pmin, pmax);
    	sleep(secs);
    	time_t mtime = time(0);
    	t = localtime(&mtime);
    	strftime(mtime_str, 128, "%c", t);
    	sprintf(secs_str, "%d", secs);
    	sprintf(pid, "%d", getpid());
    	strcpy(new_line, pid);
    	strcat(new_line, " ");
    	strcat(new_line, secs_str);
    	strcat(new_line, " ");
    	strcat(new_line, mtime_str);
    	fseek(fp, 0, SEEK_END);
    	fputs(new_line, fp);
    	int i = 0;
		char tmp[2];
		tmp[1] = '\0';
    	while(i < bytes) {
    		tmp[0] = rand_from_to('A', 'Z');
    		fputs(tmp, fp);
    		i++;
    	}
    	fputs("\n", fp);
    }

    free(t);
    fclose(fp);

    return 0;
}