typedef struct assembly_line {
    int line;
    pid_t pid;
    struct timeval tv;
} assembly_line;

typedef struct data {
    int K;
    int M;
    int curr_m;
} data;