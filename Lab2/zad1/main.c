#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <zconf.h>
#include <unistd.h>
#include <string.h>

char* gen_str(int length) {
    char* res = (char*) calloc(length + 1, sizeof(char));
    int i;
    for(i = 0; i < length; i++) {
        res[i] = rand() % 26 + 97;
    }
    res[length] = '\n';
    return res;
}

void generate(char* file_name, int size, int length) {
    int file_no = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    srand(time(0));
    int j;
    for(j = 0; j < size; j++) {
        char* buff = gen_str(length);
        write(file_no, buff, (length+1) * sizeof(char));
    }
    close(file_no);
}

void sort_sys(char* file_name, int size, int length) {
    int i;
    int file_no;
    int line_size = ((length + 1) * sizeof(char));
    char* tmp = (char*) calloc((length+1), sizeof(char));
    char* tmp1 = (char*) calloc((length+1), sizeof(char));
    file_no = open(file_name, O_RDWR);
    for(i = 0; i < size; i++) {
        lseek(file_no, i * line_size, SEEK_SET);
        int j;
        int min_line_no = i;
        char min_char = 'z';
        for(j = i; j < size; j++) {
            lseek(file_no, j * line_size, SEEK_SET);
            read(file_no, tmp, sizeof(char) * (length+1));
            if(tmp[0] <= min_char) {
                min_char = tmp[0];
                min_line_no = j;
            }
        }
        if(min_line_no != i) {
            lseek(file_no, min_line_no * line_size, SEEK_SET);
            read(file_no, tmp, sizeof(char) * (length+1));
            lseek(file_no, i * line_size, SEEK_SET);
            read(file_no, tmp1, sizeof(char) * (length+1));
            lseek(file_no, min_line_no * line_size, SEEK_SET);
            write(file_no, tmp1, (length+1) * sizeof(char));
            lseek(file_no, i * line_size, SEEK_SET);
            write(file_no, tmp, (length+1) * sizeof(char));
        }
    }
    free(tmp);
    free(tmp1);
    close(file_no);
}

void sort_lib(char* file_name, int size, int length) {
    int i;
    FILE* file;
    int line_size = ((length + 1) * sizeof(char));
    char* tmp = (char*) calloc((length+1), sizeof(char));
    char* tmp1 = (char*) calloc((length+1), sizeof(char));
    file = fopen(file_name, "r+");
    for(i = 0; i < size; i++) {
        fseek(file, i * line_size, 0);
        int j;
        int min_line_no = i;
        char min_char = 'z';
        for(j = i; j < size; j++) {
            fseek(file, j * line_size, 0);
            fread(tmp, sizeof(char), (length+1), file);
            if(tmp[0] <= min_char) {
                min_char = tmp[0];
                min_line_no = j;
            }
        }
        if(min_line_no != i) {
            fseek(file, min_line_no * line_size, 0);
            fread(tmp, sizeof(char), (length+1), file);
            fseek(file, i * line_size, 0);
            fread(tmp1, sizeof(char), (length+1), file);
            fseek(file, min_line_no * line_size, 0);
            fwrite(tmp1, sizeof(char), (length+1), file);
            fseek(file, i * line_size, 0);
            fwrite(tmp, sizeof(char), (length+1), file);
        }
    }
    free(tmp);
    free(tmp1);
    fclose(file);
}

void copy_sys(char* source, char* destination, int size, int length) {
    char* tmp = (char*) calloc(length + 1, sizeof(char));
    int source_no;
    int destination_no;
    source_no = open(source, O_RDONLY);
    destination_no = open(destination, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    int i;
    for(i = 0; i < size; i++) {
        read(source_no, tmp, (length + 1) * sizeof(char));
        write(destination_no, tmp, (length + 1) * sizeof(char));
    }
    close(source_no);
    close(destination_no);
    free(tmp);
}

void copy_lib(char* source, char* destination, int size, int length) {
    char* tmp = (char*) calloc(length + 1, sizeof(char));
    FILE* source_p;
    FILE* destination_p;
    source_p = fopen(source, "r");
    destination_p = fopen(destination, "w+");
    int i;
    for(i = 0; i < size; i++) {
        fread(tmp, length + 1, sizeof(char), source_p);
        fwrite(tmp, length + 1, sizeof(char), destination_p);
    }
    fclose(source_p);
    fclose(destination_p);
    free(tmp);
}

int main(int argc, char* argv[]) {

    int i = 1;
    while(i < argc) {
        if(strcmp(argv[i], "generate") == 0 && i+3 < argc) {
            i++;
            char* file_name = argv[i];
            i++;
            char* sz = argv[i];
            int size = atoi(sz);
            i++;
            sz = argv[i];
            int length = atoi(sz);
            generate(file_name, size, length);
            printf("Generated %d lines, %d chars each\n", size, length);
        } else if(strcmp(argv[i], "sort") == 0 && i+4 < argc) {
            i++;
            char* file_name = argv[i];
            i++;
            char* sz = argv[i];
            int size = atoi(sz);
            i++;
            sz = argv[i];
            int length = atoi(sz);
            i++;
            char* option = argv[i];
            printf("####################\n\t%s\n####################\n", option);
            if(strcmp(option, "sys") == 0) {
                sort_sys(file_name, size, length);
            } else if(strcmp(option, "lib") == 0) {
                sort_lib(file_name, size, length);
            }
            printf("Sorted file with %d lines, %d chars each\n", size, length);
        } else if(strcmp(argv[i], "copy") == 0 && i+5 < argc) {
            i++;
            char* source = argv[i];
            i++;
            char* destination = argv[i];
            i++;
            char* sz = argv[i];
            int size = atoi(sz);
            i++;
            sz = argv[i];
            int length = atoi(sz);
            i++;
            char* option = argv[i];
            printf("####################\n\t%s\n####################\n", option);
            if(strcmp(option, "sys") == 0) {
                copy_sys(source, destination, size, length);
            } else if(strcmp(option, "lib") == 0) {
                copy_lib(source, destination, size, length);
            }
            printf("Copied file with %d lines, %d chars each\n", size, length);
        }
        i++;
    }

    /*
        Analizujac czasy wykonania programu i poszczegolnych procesow,
        szczegolnie sortowania (ktore mialo najwieksza zlozonosz obliczeniawa
        i czasowa), mozna zauwazyc ze dla malych wielkosci rekordow szybszy jest
        algorytm uzywajacy biblioteczne funkcje do dzialania na plikach, natomiast
        im wieksze rozmiary rekordow, tym proporcjonalnie do drugiego rozwiazania,
        szybszy staje sie algorytm korzystajacy z funkcji systemowych.
    */

    return 0;
}
