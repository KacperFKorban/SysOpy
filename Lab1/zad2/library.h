#ifndef SYSOPY_LIBRARY_H
#define SYSOPY_LIBRARY_H

typedef struct {
    char** table;
    int size;
    int index;
    char* dir;
    char* file;
    char* tmp;
} wrapped_block;

wrapped_block* create_table(int size);

void set_file(wrapped_block* wb, char* file);

void set_directory(wrapped_block* wb, char* directory);

void set_tmp_file_name(wrapped_block* wb, char* tmp);

void find(wrapped_block* wb);

int copy_to_mem(wrapped_block* wb);

void remove_block(wrapped_block* wb, int index);

void free_mem(wrapped_block* wb);

#endif