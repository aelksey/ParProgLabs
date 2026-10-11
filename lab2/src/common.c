#include "common.h"
#include <stdlib.h>
#include <string.h>

uint64 get_factorial(int n) {
    uint64 f = 1;
    for (int i = 1; i <= n; ++i) f *= i;
    return f;
}

static void remove_at(int *arr, int *size, int index) {
    for (int i = index; i < *size - 1; i++) {
        arr[i] = arr[i + 1];
    }
    (*size)--;
}

void get_permutation_by_index(int n, uint64 index, int *out_perm) {
    int *numbers = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) numbers[i] = i + 1;
    
    int current_size = n;
    uint64 fact = get_factorial(n);

    for (int i = 0; i < n; i++) {
        fact /= (n - i);
        uint64 pos = index / fact;
        index %= fact;
        out_perm[i] = numbers[pos];
        remove_at(numbers, &current_size, (int)pos);
    }
    free(numbers);
}

int next_permutation(int *perm, int n) {
    int i = n - 2;
    while (i >= 0 && perm[i] >= perm[i + 1]) i--;
    if (i < 0) return 0;

    int j = n - 1;
    while (perm[j] <= perm[i]) j--;

    int temp = perm[i];
    perm[i] = perm[j];
    perm[j] = temp;

    int left = i + 1, right = n - 1;
    while (left < right) {
        temp = perm[left];
        perm[left] = perm[right];
        perm[right] = temp;
        left++;
        right--;
    }
    return 1;
}

void write_permutation(uint64 idx, const int *perm, int n, FILE *out) {
    fprintf(out, "%llu: ", idx);
    for (int i = 0; i < n; i++) {
        fprintf(out, "%d%s", perm[i], (i == n - 1 ? "" : " "));
    }
    fprintf(out, "\n");
}

int verify_results(uint64 total_permutations, int num_procs, const uint64 *counts) {
    FILE *seq_file = fopen("seq_output.txt", "r");
    if (!seq_file) return 0;

    char seq_line[256];
    char par_line[256];

    for (int r = 0; r < num_procs; r++) {
        if (counts[r] == 0) continue;

        char par_filename[64];
        sprintf(par_filename, "par_output_rank_%d.txt", r);
        FILE *par_file = fopen(par_filename, "r");
        if (!par_file) {
            fclose(seq_file);
            return 0;
        }

        while (fgets(par_line, sizeof(par_line), par_file)) {
            if (!fgets(seq_line, sizeof(seq_line), seq_file) || strcmp(seq_line, par_line) != 0) {
                fclose(par_file);
                fclose(seq_file);
                return 0; 
            }
        }
        fclose(par_file);
    }
    fclose(seq_file);
    return 1;
}
