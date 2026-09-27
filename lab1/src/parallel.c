#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <omp.h>
#include "determinant.h"

// Параллельный аналог математического движка на базе OpenMP API.
// Параллелизации подвергаются самые тяжелые независимые расчетные блоки.
DeterminantResult compute_determinant_parallel(const double *A, int n, int threads_count) {
    DeterminantResult result = {1.0, 0.0, 0.0};
    double *LU = (double *)malloc(n * n * sizeof(double));
    if (!LU) {
        printf("Error: Memory allocation failed inside parallel solver.\n");
        result.raw_value = 0.0;
        return result;
    }
    for (int i = 0; i < n * n; i++) LU[i] = A[i];

    int pivot_swaps = 0;

    for (int i = 0; i < n; i++) {
        // Поиск максимального ведущего элемента (Pivoting) выполняется последовательно.
        // Операция является шагом редукции/поиска максимума и на мелких объемах 
        // в параллельном режиме генерировала бы больше накладных расходов, чем пользы.
        double max_element_val = 0.0;
        int pivot_row_idx = i;
        
        for (int k = i; k < n; k++) {
            double current_abs = fabs(LU[k * n + i]);
            if (current_abs > max_element_val) {
                max_element_val = current_abs;
                pivot_row_idx = k;
            }
        }

        if (max_element_val < 1e-12) {
            free(LU);
            result.sign = 1.0;
            result.log_value = -INFINITY;
            result.raw_value = 0.0;
            return result;
        }

        if (pivot_row_idx != i) {
            for (int k = 0; k < n; k++) {
                double temp_swap = LU[i * n + k];
                LU[i * n + k] = LU[pivot_row_idx * n + k];
                LU[pivot_row_idx * n + k] = temp_swap;
            }
            pivot_swaps++;
        }

        // ---------------------------------------------------------------------
        // ВНЕДРЕНИЕ OpenMP ТЕХНОЛОГИИ (Главная параллельная область программы)
        // ---------------------------------------------------------------------
        // Разбор параметров директивы:
        // 1. num_threads(threads_count) — явно ограничивает пул потоков переданным числом.
        // 2. schedule(guided) — динамический планировщик. Выдает крупные блоки строк в начале
        //    и уменьшает их размер к концу LUP, когда подматрица сужается. Балансирует нагрузку ядер.
        // 3. default(none) — жесткое требование компилятора явно определить статус каждой переменной (защита от ошибок).
        // 4. shared(LU, n, i) — общие ресурсы в памяти, к которым имеют доступ все потоки параллельно.
        // 5. Переменные j (счетчик цикла) и k неявно становятся private — у каждого потока своя копия.
        // 6. Оптимизация if(n - i > 150) — критически важный сторожевой флаг. Если до конца расчета
        //    осталось менее 150 строк, OpenMP отключается и этот шаг выполняется последовательно.
        //    Это убирает накладные расходы (Thread Overhead) на мелких финальных итерациях.
        // ---------------------------------------------------------------------
        #pragma omp parallel for num_threads(threads_count) schedule(guided) default(none) shared(LU, n, i) if(n - i > 150)
        for (int j = i + 1; j < n; j++) {
            // Вычисление множителя происходит локально внутри каждого потока (потокобезопасно)
            double multiplier = LU[j * n + i] / LU[i * n + i];
            LU[j * n + i] = multiplier;
            for (int k = i + 1; k < n; k++) {
                LU[j * n + k] -= multiplier * LU[i * n + k];
            }
        }
    }

    // Сбор результатов по главной диагонали оставляем последовательным, 
    // так как это легкий цикл длины N, выполняющийся за наносекунды.
    double sign_modifier = (pivot_swaps % 2 == 0) ? 1.0 : -1.0;
    double accumulator_log = 0.0;

    for (int i = 0; i < n; i++) {
        double diagonal_element = LU[i * n + i];
        if (diagonal_element < 0.0) {
            sign_modifier = -sign_modifier;
        }
        accumulator_log += log(fabs(diagonal_element));
    }

    result.sign = sign_modifier;
    result.log_value = accumulator_log;
    result.raw_value = sign_modifier * exp(accumulator_log);

    free(LU);
    return result;
}
