#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

int main(int argc, char* argv[]) {
    AppArgs args = {NULL, -1, 16};

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0) && i + 1 < argc) {
            args.file_path = argv[++i];
        } else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--size") == 0) && i + 1 < argc) {
            args.tri_size = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--group") == 0) && i + 1 < argc) {
            args.local_size = atoi(argv[++i]);
        } else {
            fprintf(stderr, "Error: Unknown or incomplete parameter: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!args.file_path || args.tri_size <= 0) {
        fprintf(stderr, "Error: Missing required arguments (-f and -s).\n");
        print_usage(argv[0]);
        return 1;
    }

    FILE* file = fopen(args.file_path, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open matrix file '%s'\n", args.file_path);
        return 1;
    }

    int rows, cols;
    if (fscanf(file, "%d %d", &rows, &cols) != 2) {
        fprintf(stderr, "Error: Invalid matrix file header format.\n");
        fclose(file);
        return 1;
    }

    int* matrix = (int*)malloc((size_t)rows * cols * sizeof(int));
    for (int i = 0; i < rows * cols; i++) {
        if (fscanf(file, "%d", &matrix[i]) != 1) {
            fprintf(stderr, "Error: Matrix file ended prematurely.\n");
            free(matrix);
            fclose(file);
            return 1;
        }
    }
    fclose(file);

    unsigned long long seq_sum, par_sum;
    unsigned long long seq_count, par_count;
    int seq_r, seq_c, par_r, par_c;

    int triangle_elements = args.tri_size * (args.tri_size + 1) / 2;
    int* seq_triangle = (int*)malloc(triangle_elements * sizeof(int));
    int* par_triangle = (int*)malloc(triangle_elements * sizeof(int));

    double t_seq_start = get_time_sec();
    run_sequential(matrix, rows, cols, args.tri_size, &seq_sum, &seq_r, &seq_c, seq_triangle, &seq_count);
    double t_seq_end = get_time_sec();
    double seq_time = t_seq_end - t_seq_start;

    double t_par_start = get_time_sec();
    run_parallel(matrix, rows, cols, args.tri_size, args.local_size, &par_sum, &par_r, &par_c, par_triangle, &par_count);
    double t_par_end = get_time_sec();
    double max_par_time = t_par_end - t_par_start;
    
    // Многокритериальная программная верификация
    int match = 1;
    if (seq_sum != par_sum || seq_count != par_count || seq_r != par_r || seq_c != par_c) {
        match = 0;
    } else {
        for (int i = 0; i < triangle_elements; i++) {
            if (seq_triangle[i] != par_triangle[i]) {
                match = 0;
                break;
            }
        }
    }

    // Запись результатов в файл submatrix_output.txt
    FILE* out_file = fopen("submatrix_output.txt", "w");
    if (out_file) {
        fprintf(out_file, "=== MULTI-CRITERIA ANALYSIS REPORT ===\n");
        fprintf(out_file, "Source file: %s (%dx%d)\n", args.file_path, rows, cols);
        fprintf(out_file, "Global Minimum Sum: %llu\n", par_sum);
        fprintf(out_file, "Total submatrices matching minimum sum (CPU): %llu\n", seq_count);
        fprintf(out_file, "Total submatrices matching minimum sum (GPU): %llu\n\n", par_count);
        
        fprintf(out_file, "=== LAST SEQUENTIAL TRIANGLE MATCH ({%d:%d}) ===\n", seq_r, seq_c);
        int idx = 0;
        for (int i = 0; i < args.tri_size; i++) {
            for (int j = 0; j <= i; j++) {
                fprintf(out_file, "%d ", seq_triangle[idx++]);
            }
            fprintf(out_file, "\n");
        }

        fprintf(out_file, "\n=== LAST PARALLEL TRIANGLE MATCH ({%d:%d}) ===\n", par_r, par_c);
        idx = 0;
        for (int i = 0; i < args.tri_size; i++) {
            for (int j = 0; j <= i; j++) {
                fprintf(out_file, "%d ", par_triangle[idx++]);
            }
            fprintf(out_file, "\n");
        }
        fclose(out_file);
    }

    double speedup = seq_time / max_par_time;
    char dims_buf[32], group_buf[48], seq_buf[48], par_buf[48];
    sprintf(dims_buf, "%dx%d", rows, cols);
    sprintf(group_buf, "%dx%d (%d)", args.local_size, args.local_size, args.local_size * args.local_size);
    sprintf(seq_buf, "%llu at {%d:%d}", seq_count, seq_r, seq_c);
    sprintf(par_buf, "%llu at {%d:%d}", par_count, par_r, par_c);

    printf("\n");
    printf("+------------------------------------------+-----------------------+\n");
    printf("|              OpenCL Submatrix Search Benchmarks                  |\n");
    printf("+------------------------------------------+-----------------------+\n");
    printf("| %-40s | %-21s |\n", "Metric / Configuration Parameter", "Value");
    printf("+------------------------------------------+-----------------------+\n");
    printf("| Matrix Dimensions                        | %-21s |\n", dims_buf);
    printf("| Target Triangle Size                     | %-21d |\n", args.tri_size);
    printf("| OpenCL Local Group Configuration         | %-21s |\n", group_buf);
    printf("| CPU Global Minima (Count & Last Coord)   | %-21s |\n", seq_buf);
    printf("| GPU Global Minima (Count & Last Coord)   | %-21s |\n", par_buf);
    printf("| Absolute Minimum Elements Sum            | %-21llu |\n", par_sum);
    printf("+------------------------------------------+-----------------------+\n");
    printf("| Sequential Execution Time                | %-17.6f sec |\n", seq_time);
    printf("| Parallel Execution Time (Max Process)    | %-17.6f sec |\n", max_par_time);
    printf("+------------------------------------------+-----------------------+\n");
    printf("| Achieved Speedup Factor                  | %-20.2fx |\n", speedup);
    printf("+------------------------------------------+-----------------------+\n");
    printf("| Software Verification Status             | %-21s |\n", 
           match ? "SUCCESS (Identical)" : "ERROR (Mismatch!)");
    printf("+------------------------------------------+-----------------------+\n");
    printf("\n* Note: Detailed distribution metrics dumped into 'submatrix_output.txt'\n\n");

    free(matrix); free(seq_triangle); free(par_triangle);
    return 0;
}
