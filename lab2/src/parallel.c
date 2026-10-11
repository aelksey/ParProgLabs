#include "common.h"
#include <stdlib.h>

void run_parallel(int n, uint64 start_idx, uint64 count, int rank) {
    char filename[64];
    sprintf(filename, "par_output_rank_%d.txt", rank);
    
    FILE *out = fopen(filename, "w");
    if (!out) return;

    if (count == 0) {
        fclose(out);
        return;
    }

    int *perm = (int*)malloc(n * sizeof(int));
    get_permutation_by_index(n, start_idx, perm);

    for (uint64 i = 0; i < count; i++) {
        write_permutation(start_idx + i, perm, n, out);
        next_permutation(perm, n);
    }

    free(perm);
    fclose(out);
}
