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

char* formatdate(char* str, time_t val) {
        strftime(str, 36, "%d.%m.%Y %H:%M:%S", localtime(&val));
        return str;
}

char* file_type(int st_mode) {
    if(S_ISREG(st_mode)) {
        return "file";
    } else if(S_ISDIR(st_mode)) {
        return "dir";
    } else if(S_ISCHR(st_mode)) {
        return "char dev";
    } else if(S_ISBLK(st_mode)) {
        return "block dev";
    } else if(S_ISFIFO(st_mode)) {
        return "fifo";
    } else if(S_ISLNK(st_mode)) {
        return "slink";
    } else if(S_ISSOCK(st_mode)) {
        return "sock";
    }
    return "unknown";
}

void traverse(char* path, char* comp, time_t mtime) {
    DIR* root_of_search = opendir(path);
    char new_path[PATH_MAX + 1];

    if(root_of_search == NULL) return;

    struct dirent* de;
    struct stat st;

    char str[36];

    char c = comp[0];
    while((de = readdir(root_of_search)) != NULL) {
        int f = 0;
        if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        realpath(de->d_name, new_path);
        lstat(new_path, &st);
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

        if(st.st_mode & S_IFDIR) {
            traverse(new_path, comp, mtime);
        }
        if(f == 1) {
            printf("%s %s %ld %s %s\n", new_path, file_type(st.st_mode), st.st_size,
                formatdate(str, st.st_atime), formatdate(str, st.st_mtime));
        }
    }

    closedir(root_of_search);
}

int main(int argc, char* argv[]) {

    if(argc != 4) return -1;
    char* path = argv[1];
    char* comp = argv[2];
    char* date = argv[3];

    char* ptr;
    time_t mtime = strtol(date, &ptr, 10);

    // TODO sciezka wzgledna i wyswietlanie bezwzglednej

    traverse(path, comp, mtime);

    return 0;
}