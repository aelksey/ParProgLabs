#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>
#include "determinant.h" // Подключаем наш общий интерфейс

void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS] <matrix_file_path> [threads_count]\n", program_name);
    printf("\nParallel and sequential matrix determinant computer.\n");
    printf("\nOptions:\n");
    printf("  -h, --help    Show this help message and exit\n");
    printf("\nArguments:\n");
    printf("  matrix_file_path  Path to the text file containing matrix dimensions and data\n");
    printf("  threads_count     Optional: Number of OpenMP threads to use (default: max cores)\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    char *matrix_source_file = argv[1];

    int threads_num = (argc >= 3) ? atoi(argv[2]) : omp_get_max_threads();
    if (threads_num <= 0) {
        threads_num = omp_get_max_threads();
    }

    FILE *file_handle = fopen(matrix_source_file, "r");
    if (!file_handle) {
        printf("Error: Target matrix file '%s' could not be opened.\n", matrix_source_file);
        printf("Run '%s --help' for usage info.\n", argv[0]);
        return 1;
    }

    int matrix_rows = 0;
    int matrix_cols = 0;

    if (fscanf(file_handle, "%d %d", &matrix_rows, &matrix_cols) != 2) {
        printf("Error: Failed to parse matrix dimensions.\n");
        fclose(file_handle);
        return 1;
    }

    if (matrix_rows != matrix_cols) {
        printf("Error: Matrix geometry mismatch (%dx%d). Square matrix expected.\n", matrix_rows, matrix_cols);
        fclose(file_handle);
        return 1;
    }

    int total_cells = matrix_rows * matrix_cols;
    double *matrix_buffer = (double *)malloc(total_cells * sizeof(double));
    
    if (!matrix_buffer) {
        printf("Error: High-capacity memory allocation failed for %d elements.\n", total_cells);
        fclose(file_handle);
        return 1;
    }

    for (int i = 0; i < total_cells; i++) {
        if (fscanf(file_handle, "%lf", &matrix_buffer[i]) != 1) {
            break;
        }
    }
    fclose(file_handle);

    printf("+------------------------------------------+-----------------------+\n");
    printf("| %-64s |\n", "Matrix Info");
    printf("+------------------------------------------+-----------------------+\n");
    printf("| Matrix Dimensions                        | %d x %-13d |\n", matrix_rows, matrix_cols);
    printf("| Target Threads Allocated                 | %-21d |\n", threads_num);

    // Запуск последовательного расчёта
    double t_seq_start = omp_get_wtime();
    DeterminantResult seq_report = compute_determinant_sequential(matrix_buffer, matrix_rows);
    double t_seq_end = omp_get_wtime();
    double time_seq = t_seq_end - t_seq_start;

    printf("+------------------------------------------+-----------------------+\n");
    printf("|                   Sequential Execution Results:                  |\n");
    printf("+------------------------------------------+-----------------------+\n");
    printf("| %-40s | %-21s |\n", "Metric", "Value");
    printf("+------------------------------------------+-----------------------+\n");
    printf("| Determinant Multiplier (Sign)            | %-21.1f |\n", seq_report.sign);
    printf("| Raw Value                                | %-21.6e |\n", seq_report.raw_value);
    printf("| Logarithm (ln(D))                        | %-21.6f |\n", seq_report.log_value);
    printf("| Execution Time:                          | %-17.6f sec |\n", time_seq);

    // Выполняем параллельный расчёт
    double t_par_start = omp_get_wtime(); 
    DeterminantResult par_report = compute_determinant_parallel(matrix_buffer, matrix_rows, threads_num);
    double t_par_end = omp_get_wtime();
    double time_par = t_par_end - t_par_start;

    printf("+------------------------------------------+-----------------------+\n");
    printf("|                   Parallel Execution Results:                    |\n");
    printf("+------------------------------------------+-----------------------+\n");
    printf("| %-40s | %-21s |\n", "Metric", "Value");
    printf("+------------------------------------------+-----------------------+\n");
    printf("| Determinant Multiplier (Sign)            | %-21.1f |\n", par_report.sign);
    printf("| Raw Value                                | %-21.6e |\n", par_report.raw_value);
    printf("| Logarithm (ln(D))                        | %-21.6f |\n", par_report.log_value);
    printf("| Execution Time:                          | %-17.6f sec |\n", time_par);

    int seq_par_same = 0;
    if (fabs(seq_report.log_value - par_report.log_value) < 1e-5 && seq_report.sign == par_report.sign) {
        seq_par_same = 1;
    }

    printf("+------------------------------------------+-----------------+\n");
    printf("|                   Performance Report:                      |\n");
    printf("+------------------------------------------------------------+\n");
    printf("| Calculated Speedup Factor (seq / par):   | %-15.2f |\n", time_seq / time_par);
    printf("| Results match? (1 - yes ; 0 - no):       | %-15d |\n", seq_par_same);
    printf("+------------------------------------------------------------+\n");

    free(matrix_buffer);
    return 0;
}
