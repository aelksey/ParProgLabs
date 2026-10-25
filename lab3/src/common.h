#ifndef COMMON_H
#define COMMON_H

#include <CL/cl.h>

typedef struct {
    char* file_path;
    int tri_size;
    int local_size;
} AppArgs;

double get_time_sec(void);
void print_usage(const char* prog_name);
char* load_kernel_source(const char* filename);

void run_sequential(const int* matrix, int rows, int cols, int tri_size, 
                    unsigned long long* out_sum, int* out_r, int* out_c, 
                    int* out_triangle, unsigned long long* out_count);

void run_parallel(const int* matrix, int rows, int cols, int tri_size, int local_size_param,
                  unsigned long long* out_sum, int* out_r, int* out_c, 
                  int* out_triangle, unsigned long long* out_count);

#endif
