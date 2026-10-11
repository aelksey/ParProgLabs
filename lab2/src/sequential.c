#include "common.h"
#include <stdlib.h>

void run_sequential(int n, uint64 total_permutations) {
    FILE *out = fopen("seq_output.txt", "w");
    if (!out) return;

    int *perm = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) perm[i] = i + 1;

    uint64 count = 0;
    do {
        write_permutation(count, perm, n, out);
        count++;
    } while (next_permutation(perm, n) && count < total_permutations);

    free(perm);
    fclose(out);
}
