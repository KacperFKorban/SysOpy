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

char* formatdate(char* str, time_t val) {
        strftime(str, 36, "%d.%m.%Y %H:%M:%S", localtime(&val));
        return str;
}

void traverse(char* path, char* sign, time_t mtime) {
    DIR* root_of_search = opendir(path);

    if(root_of_search == NULL) return;

    struct dirent* de;
    struct stat* st = (struct stat*) calloc(1, sizeof(struct stat));

    char str[36];

    char c = sign[0];
    while((de = readdir(root_of_search)) != NULL) {
        int f = 0;
        char* new_path = strcat(path, de->d_name);
        stat(new_path, st);
        switch(c) {
            case '<':
                if(st->st_mtime < mtime) {
                    f = 1;
                }
            case '=':
                if(st->st_mtime == mtime) {
                    f = 1;
                }
            case '>':
                if(st->st_mtime > mtime) {
                    f = 1;
                }
        }
        
        if(st->st_mode & S_IFDIR) {
            traverse(new_path, sign, mtime);
        }
        if(f == 1) {
            char* file_type;
            if(st->st_mode & S_IFREG) {
                file_type = "file";
            } else if(st->st_mode & S_IFDIR) {
                file_type = "dir";
            } else if(st->st_mode & S_IFCHR) {
                file_type = "char dev";
            } else if(st->st_mode & S_IFBLK) {
                file_type = "block dev";
            } else if(st->st_mode & S_IFIFO) {
                file_type = "fifo";
            } else if(st->st_mode & S_IFLNK) {
                file_type = "slink";
            } else if(st->st_mode & /* TODO */) {
                file_type = "sock";
            }
            printf("%s %s %d %s %s\n", new_path, file_type, st->st_size,
                formatdate(str, st->st_atime), formatdate(str, st->st_mtime));
        }
    }

free(st);
    closedir(root_of_search);
}

int main(int argc, char* argv[]) {

    if(argc != 4) return -1;
    char* path = argv[1];
    char* sign = argv[2];
    char* date = argv[3];

    char* ptr;
    time_t mtime = strtol(data, ptr, 10);

    traverse(path, sign, mtime);

    return 0;
}