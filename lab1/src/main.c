#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>
#include "determinant.h" // Подключаем прототипы наших функций

// Функция вывода Unix-подобной справки по использованию параметров командной строки
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
    // 1. Проверка на полное отсутствие аргументов запуска
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // 2. Обработка флагов вызова встроенного справочного руководства
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    // Фиксируем путь к текстовому файлу с матрицей
    char *matrix_source_file = argv[1];

    // 3. Динамическая настройка количества потоков OpenMP (Выполнение ТЗ)
    // Если передан 3-й аргумент, парсим его в число, иначе запрашиваем максимум у системы
    int threads_num = (argc >= 3) ? atoi(argv[2]) : omp_get_max_threads();
    if (threads_num <= 0) {
        threads_num = omp_get_max_threads(); // Защита от некорректного или нулевого ввода
    }

    // Открытие файла данных в режиме чтения
    FILE *file_handle = fopen(matrix_source_file, "r");
    if (!file_handle) {
        printf("Error: Target matrix file '%s' could not be opened.\n", matrix_source_file);
        printf("Run '%s --help' for usage info.\n", argv[0]);
        return 1;
    }

    int matrix_rows = 0;
    int matrix_cols = 0;

    // Считываем первую строку файла — геометрию матрицы (строки и столбцы)
    if (fscanf(file_handle, "%d %d", &matrix_rows, &matrix_cols) != 2) {
        printf("Error: Failed to parse matrix dimensions.\n");
        fclose(file_handle);
        return 1;
    }

    // Математическая валидация: определитель существует только для квадратных матриц
    if (matrix_rows != matrix_cols) {
        printf("Error: Matrix geometry mismatch (%dx%d). Square matrix expected.\n", matrix_rows, matrix_cols);
        fclose(file_handle);
        return 1;
    }

    // Выделение памяти под матрицу линейным (одномерным) куском double-элементов.
    // Это исключает фрагментацию памяти и ускоряет работу с кэшем процессора.
    int total_cells = matrix_rows * matrix_cols;
    double *matrix_buffer = (double *)malloc(total_cells * sizeof(double));
    
    if (!matrix_buffer) {
        printf("Error: High-capacity memory allocation failed for %d elements.\n", total_cells);
        fclose(file_handle);
        return 1;
    }

    // Пошагово выкачиваем все вещественные элементы из файла в буфер оперативной памяти
    int elements_read = 0;
    for (int i = 0; i < total_cells; i++) {
        if (fscanf(file_handle, "%lf", &matrix_buffer[i]) == 1) {
            elements_read++;
        } else {
            break; // Прерываем чтение, если файл внезапно закончился раньше времени
        }
    }
    fclose(file_handle); // Корректно закрываем дескриптор файла

    // Блок красивого логирования метаданных загрузки
    printf("+------------------------------------------+-----------------------+\n");
    printf("| %-64s |\n", "Matrix Info");
    printf("+------------------------------------------+-----------------------+\n");
    printf("| Matrix Dimensions                        | %d x %-13d |\n", matrix_rows, matrix_cols);
    printf("| Target Threads Allocated                 | %-21d |\n", threads_num);

    // ==========================================
    // ЗАПУСК №1: ПОСЛЕДОВАТЕЛЬНЫЙ РАСЧЕТ И ТАЙМИНГ
    // ==========================================
    double t_seq_start = omp_get_wtime(); // Высокоточный замер времени ДО старта вычислений
    DeterminantResult seq_report = compute_determinant_sequential(matrix_buffer, matrix_rows);
    double t_seq_end = omp_get_wtime();   // Замер времени ПОСЛЕ окончания расчета
    double time_seq = t_seq_end - t_seq_start;

    // Вывод результатов последовательного блока (с использованием экспоненциального вида %e для Raw)
    printf("+------------------------------------------+-----------------------+\n");
    printf("|                   Sequential Execution Results:                  |\n");
    printf("+------------------------------------------+-----------------------+\n");
    printf("| %-40s | %-21s |\n", "Metric", "Value");
    printf("+------------------------------------------+-----------------------+\n");
    printf("| Determinant Multiplier (Sign)            | %-21.1f |\n", seq_report.sign);
    printf("| Raw Value                                | %-21.6e |\n", seq_report.raw_value);
    printf("| Logarithm (ln(D))                        | %-21.6f |\n", seq_report.log_value);
    printf("| Execution Time:                          | %-17.6f sec |\n", time_seq);

    // ==========================================
    // ЗАПУСК №2: ПАРАЛЛЕЛЬНЫЙ OpenMP РАСЧЕТ И ТАЙМИНГ
    // ==========================================
    double t_par_start = omp_get_wtime(); // Высокоточный замер времени параллельного режима
    DeterminantResult par_report = compute_determinant_parallel(matrix_buffer, matrix_rows, threads_num);
    double t_par_end = omp_get_wtime();
    double time_par = t_par_end - t_par_start;

    // Вывод результатов параллельного блока
    printf("+------------------------------------------+-----------------------+\n");
    printf("|                   Parallel Execution Results:                    |\n");
    printf("+------------------------------------------+-----------------------+\n");
    printf("| %-40s | %-21s |\n", "Metric", "Value");
    printf("+------------------------------------------+-----------------------+\n");
    printf("| Determinant Multiplier (Sign)            | %-21.1f |\n", par_report.sign);
    printf("| Raw Value                                | %-21.6e |\n", par_report.raw_value);
    printf("| Logarithm (ln(D))                        | %-21.6f |\n", par_report.log_value);
    printf("| Execution Time:                          | %-17.6f sec |\n", time_par);

    // ==========================================
    // СЕКЦИЯ ПРОГРАММНОГО СРАВНЕНИЯ И ЭФФЕКТИВНОСТИ
    // ==========================================
    int seq_par_same = 0;
    // Сравнение производим по логарифмическому значению, так как прямые значения double 
    // могут превратиться в бесконечности (inf), и операция inf == inf выдаст ошибку.
    if (fabs(seq_report.log_value - par_report.log_value) < 1e-5 && seq_report.sign == par_report.sign) {
        seq_par_same = 1; // Результаты идентичны с учетом погрешности округления
    }

    printf("+------------------------------------------+-----------------+\n");
    printf("|                   Performance Report:                      |\n");
    printf("+------------------------------------------------------------+\n");
    // Вычисляем коэффициент ускорения: время последовательного делим на параллельное
    printf("| Calculated Speedup Factor (seq / par):   | %-15.2fx |\n", time_seq / time_par);
    printf("| Results match? (1 - yes ; 0 - no):       | %-15d |\n", seq_par_same);
    printf("+------------------------------------------------------------+\n");

    free(matrix_buffer); // Тотальное освобождение ресурсов перед закрытием приложения
    return 0;
}
