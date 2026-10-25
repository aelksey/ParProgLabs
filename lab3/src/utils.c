#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include "common.h"

double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void print_usage(const char* prog_name) {
    printf("Usage: %s -f <matrix_file> -s <triangle_size> [OPTIONS]\n\n", prog_name);
    printf("Required arguments:\n");
    printf("  -f, --file <path>            Path to the text file containing the input matrix.\n");
    printf("  -s, --size <number>          Size of the compact triangular submatrix to find.\n\n");
    printf("Optional arguments:\n");
    printf("  -g, --group <number>         OpenCL local work group size (default: 16).\n");
    printf("  -h, --help                   Display this help information.\n\n");
}

char* load_kernel_source(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open kernel file %s\n", filename);
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* src = (char*)malloc(size + 1);
    size_t read_size = fread(src, 1, size, fp);
    src[read_size] = '\0';
    fclose(fp);
    return src;
}

void run_sequential(const int* matrix, int rows, int cols, int tri_size, 
                    unsigned long long* out_sum, int* out_r, int* out_c, 
                    int* out_triangle, unsigned long long* out_count) {
    unsigned long long min_sum = 18446744073709551615ULL;
    unsigned long long count = 0;
    int last_r = -1, last_c = -1;

    // Первый проход: ищем абсолютный минимум
    for (int r = 0; r <= rows - tri_size; r++) {
        for (int c = 0; c <= cols - tri_size; c++) {
            unsigned long long current_sum = 0;
            for (int i = 0; i < tri_size; i++) {
                int row_offset = (r + i) * cols;
                for (int j = 0; j <= i; j++) {
                    current_sum += (unsigned long long)matrix[row_offset + (c + j)];
                }
            }
            if (current_sum < min_sum) {
                min_sum = current_sum;
            }
        }
    }

    // Второй проход: считаем количество и запоминаем САМЫЙ ПОСЛЕДНИЙ треугольник
    for (int r = 0; r <= rows - tri_size; r++) {
        for (int c = 0; c <= cols - tri_size; c++) {
            unsigned long long current_sum = 0;
            for (int i = 0; i < tri_size; i++) {
                int row_offset = (r + i) * cols;
                for (int j = 0; j <= i; j++) {
                    current_sum += (unsigned long long)matrix[row_offset + (c + j)];
                }
            }
            if (current_sum == min_sum) {
                count++;
                last_r = r;
                last_c = c;
            }
        }
    }

    *out_sum = min_sum;
    *out_r = last_r;
    *out_c = last_c;
    *out_count = count;

    // Копируем элементы последней найденной матрицы
    int idx = 0;
    for (int i = 0; i < tri_size; i++) {
        int row_offset = (last_r + i) * cols;
        for (int j = 0; j <= i; j++) {
            out_triangle[idx++] = matrix[row_offset + (last_c + j)];
        }
    }
}
