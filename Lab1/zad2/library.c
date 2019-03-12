#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "library.h"

wrapped_block* create_table(int size) {
    if(size < 0) return NULL;
    wrapped_block *wb = (wrapped_block*) calloc(1, sizeof(wrapped_block));
    wb->table = (char**) calloc(size, sizeof(char*));
    wb->size = size;
    wb->index = 0;
    return wb;
}

void set_file(wrapped_block* wb, char* file) {
    wb->file = file;
}

void set_directory(wrapped_block* wb, char* directory) {
    wb->dir = directory;
}

void set_tmp_file_name(wrapped_block* wb, char* tmp) {
    wb->tmp = tmp;
}

void find(wrapped_block* wb) {
    if(wb == NULL
        || wb->file == NULL
        || wb->dir == NULL
        || wb->tmp == NULL) return;
    char* call = (char*) calloc(35
        + strlen(wb->dir)
        + strlen(wb->file), sizeof(char));
    strcat(call, "find ");
    strcat(call, wb->dir);
    strcat(call, " ");
    strcat(call, wb->file);
    strcat(call, " > ");
    strcat(call, wb->tmp);
    strcat(call, " 2> /dev/null");
    system(call);
    free(call);
}

int copy_to_mem(wrapped_block* wb) {
    if(wb == NULL
        || wb->index >= wb->size
        || wb->index < 0
        || wb->tmp == NULL) return -1;
    FILE *fp;
    int sz;
    fp = fopen(wb->tmp, "r");
    if(fp == NULL) {
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    wb->table[wb->index] = (char*) calloc(sz + 1, sizeof(char));
    fread(wb->table[wb->index], sizeof(char), sz, fp);
    fclose(fp);
    wb->index++;
    return wb->index - 1;
}

void remove_block(wrapped_block* wb, int index) {
    free(wb->table[index]);
    wb->table[index] = NULL;
}

void free_mem(wrapped_block* wb) {
    if(wb == NULL) return;
    int i;
    if(wb->table != NULL) {
        for(i = 0; i < wb->size; i++) {
            if(wb->table[i] != NULL)
                free(wb->table[i]);
        }
    }
    free(wb->table);
    free(wb);
}