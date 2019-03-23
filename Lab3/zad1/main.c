#define _XOPEN_SOURCE 500
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
#include <pwd.h>
#include <ftw.h>

static int nftw_fun(const char *fpath, const struct stat *st, 
    int tflag, struct FTW *ftwbuf) {

    if(S_ISDIR(st->st_mode)) {
        pid_t pid;
        pid = fork();
        if(pid == 0) {
            execl("/bin/ls", "ls", fpath, "-l", (char *)0);
        } else {
            printf("%s %d\n", fpath, pid);
            usleep(10000);
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {

    if(argc != 2) return -1;
    char* path = argv[1];

    char dirpath[PATH_MAX];
    realpath(path, dirpath);

    nftw(dirpath, nftw_fun, 10, FTW_PHYS);

    return 0;
}