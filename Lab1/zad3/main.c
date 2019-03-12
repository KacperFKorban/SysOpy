#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <zconf.h>
#include <unistd.h>
#include <string.h>
#include "library.h"

double calculate_time(clock_t start, clock_t end) {
    return (double) (end - start) / sysconf(_SC_CLK_TCK);
}


int main(int argc, char* argv[]) {
    struct tms *gstart = malloc(sizeof(struct tms));
    struct tms *gend = malloc(sizeof(struct tms));
    struct tms *start = malloc(sizeof(struct tms));
    struct tms *end = malloc(sizeof(struct tms));
    clock_t greal_start;
    clock_t greal_end;
    clock_t real_start;
    clock_t real_end;

    greal_start = times(gstart);

    int i = 1;
    wrapped_block* wb = NULL;
    while(i < argc) {
        real_start = times(start);
        if(strcmp(argv[i], "create_table") == 0 && i+1 < argc) {
            i++;
            char* sz = argv[i];
            int size = atoi(sz);
            wb = create_table(size);
            printf("Allocated memory for %d blocks\n", size);
        } else if(strcmp(argv[i], "search_directory") == 0 && i+3 < argc) {
            i++;
            set_directory(wb, argv[i]);
            i++;
            set_file(wb, argv[i]);
            i++;
            set_tmp_file_name(wb, argv[i]);
            find(wb);
            int idx = copy_to_mem(wb);
            if(idx < 0) return idx;
            printf("Executed find %s %s > %s 2> /dev/null\n", wb->dir, wb->file, wb->tmp);
            printf("And copied to memory at index %d\n", idx);
        } else if(strcmp(argv[i], "remove_block") == 0 && i+1 < argc) {
            i++;
            char* idx = argv[i];
            int index = atoi(idx);
            remove_block(wb, index);
            printf("Removed block number %d\n", index);
        }
        i++;

        real_end = times(end);
        
        printf("real\t\tuser\t\tsystem\t\n");
        printf("%lf\t", calculate_time(real_start, real_end));
        printf("%lf\t", calculate_time(start->tms_cutime, end->tms_cutime));
        printf("%lf\n", calculate_time(start->tms_cstime, end->tms_cstime));
    }

    free_mem(wb);

    greal_end = times(gend);

    printf("\nGlobal:\n");
    printf("real\t\tuser\t\tsystem\t\n");
    printf("%lf\t", calculate_time(greal_start, greal_end));
    printf("%lf\t", calculate_time(gstart->tms_cutime, gend->tms_cutime));
    printf("%lf\n", calculate_time(gstart->tms_cstime, gend->tms_cstime));
    
    free(gstart);
    free(gend);
    free(start);
    free(end);

    return 0;
}