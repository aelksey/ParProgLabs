#ifndef DETERMINANT_H
#define DETERMINANT_H

// Общая структура для возврата расширенных результатов вычислений.
// Использование логарифма позволяет обойти проблему переполнения данных (inf)
typedef struct {
    double sign;      // Итоговый математический знак определителя (+1.0 или -1.0)
    double log_value; // Натуральный логарифм абсолютного значения: ln(|det|)
    double raw_value; // Прямое значение определителя (может уйти в inf на больших порядках)
} DeterminantResult;

// Прототипы вычислительных модулей (экспортируемый интерфейс функций)
// Передача исходного массива A идет через указатель на константу (const double *), 
// чтобы гарантировать, что функции расчета не изменят оригинальные данные в памяти.
DeterminantResult compute_determinant_sequential(const double *A, int n);
DeterminantResult compute_determinant_parallel(const double *A, int n, int threads_count);

#endif // DETERMINANT_H
