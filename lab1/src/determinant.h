#ifndef DETERMINANT_H
#define DETERMINANT_H

// Cтруктура для возврата результатов вычислений
typedef struct {
    double sign;      // Множитель определителя (+1.0 или -1.0)
    double log_value; // Натуральный логарифм абсолютного значения определителя
    double raw_value; // Сырое значение определителя
} DeterminantResult;

// Прототипы функций
DeterminantResult compute_determinant_sequential(const double *A, int n);
DeterminantResult compute_determinant_parallel(const double *A, int n, int threads_count);

#endif // DETERMINANT_H
