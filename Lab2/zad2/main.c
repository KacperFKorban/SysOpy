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

char* formatdate(char* str, time_t val) {
        strftime(str, 36, "%d.%m.%Y %H:%M:%S", localtime(&val));
        return str;
}

char* file_type(int st_mode) {
    if(st_mode & S_IFREG) {
        return "file";
    } else if(st_mode & S_IFDIR) {
        return "dir";
    } else if(st_mode & S_IFCHR) {
        return "char dev";
    } else if(st_mode & S_IFBLK) {
        return "block dev";
    } else if(st_mode & S_IFIFO) {
        return "fifo";
    } else if(st_mode & S_IFLNK) {
        return "slink";
    } else if(st_mode && S_IFSOCK) {
        return "sock";
    }
    return "unknown";
}

void traverse(char* dirpath, char* comp, time_t mtime) {
    DIR* dir = opendir(dirpath);

    if(dir == NULL) return;

    struct dirent* de;
    char new_path[PATH_MAX];
    struct stat st;
    char str[36];

    char c = comp[0];
    while((de = readdir(dir)) != NULL) {
        int f = 0;
        if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }
        strcpy(new_path, dirpath);
        strcat(new_path, "/");
        strcat(new_path, de->d_name);
        if(lstat(new_path, &st) != 0) return;
        switch(c) {
            case '<':
                if(st.st_mtime < mtime) {
                    f = 1;
                }
                break;
            case '=':
                if(st.st_mtime == mtime) {
                    f = 1;
                }
                break;
            case '>':
                if(st.st_mtime > mtime) {
                    f = 1;
                }
                break;
        }

        if(S_ISDIR(st.st_mode)) {
            traverse(new_path, comp, mtime);
        }
        if(f == 1) {
            printf("%s %s %ld %s %s\n", new_path, file_type(st.st_mode), st.st_size,
                formatdate(str, st.st_atime), formatdate(str, st.st_mtime));
        }
    }

    closedir(dir);
}

int main(int argc, char* argv[]) {

    if(argc != 4) return -1;
    char* path = argv[1];
    char* comp = argv[2];
    char* date = argv[3];

    const char format[] = "%Y-%m-%d %H:%M:%S";
    struct tm *timestamp = calloc(1, sizeof(struct tm));
    strptime(date, format, timestamp);
    time_t mtime = mktime(timestamp);

    char dirpath[PATH_MAX];
    realpath(path, dirpath);

    // TODO parsowanie dat
    // TODO nftw

    traverse(dirpath, comp, mtime);

    free(timestamp);

    return 0;
}