#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "determinant.h"

DeterminantResult compute_determinant_sequential(const double *A, int n) {
    DeterminantResult result = {1.0, 0.0, 0.0};
    
    double *LU = (double *)malloc(n * n * sizeof(double));
    if (!LU) {
        printf("Error: Memory allocation failed inside sequential solver.\n");
        result.raw_value = 0.0;
        return result;
    }
    
    for (int i = 0; i < n * n; i++) {
        LU[i] = A[i];
    }

    int pivot_swaps = 0;

    for (int i = 0; i < n; i++) {
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

        for (int j = i + 1; j < n; j++) {
            double multiplier = LU[j * n + i] / LU[i * n + i];
            LU[j * n + i] = multiplier;
            for (int k = i + 1; k < n; k++) {
                LU[j * n + k] -= multiplier * LU[i * n + k];
            }
        }
    }

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
