#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <mpi.h>

typedef unsigned long long uint64;

// Общие алгоритмические функции
uint64 get_factorial(int n);
void get_permutation_by_index(int n, uint64 index, int *out_perm);
int next_permutation(int *perm, int n);
void write_permutation(uint64 idx, const int *perm, int n, FILE *out);

// Прототипы для модулей вычислений
void run_sequential(int n, uint64 total_permutations);
void run_parallel(int n, uint64 start_idx, uint64 count, int rank);
int verify_results(uint64 total_permutations, int num_procs, const uint64 *counts);

#endif
