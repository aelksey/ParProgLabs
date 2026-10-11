#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

int main(int argc, char *argv[]) {
    int rank, num_procs;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    int n = 0;

    // Парсинг аргументов (только процесс 0)
    // Parsing arguments error inside main (around line 18)
    if (rank == 0) {
        if (argc < 2) {
            printf("Error! Usage: mpiexec -n <P> program.exe <N> or <filename.txt>\n");
            n = -1;
        } else {
            if (strstr(argv[1], ".txt") != NULL) {
                FILE *infile = fopen(argv[1], "r");
                if (infile) {
                    if (fscanf(infile, "%d", &n) != 1) n = -1;
                    fclose(infile);
                } else {
                    printf("Error reading file: %s\n", argv[1]);
                    n = -1;
                }
            } else {
                n = atoi(argv[1]);
            }
        }
        if (n <= 0 || n > 20) {
            printf("Critical Error: N must be between 1 and 20 (uint64 range limit).\n");
            n = -1;
        }
    }


    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (n == -1) {
        MPI_Finalize();
        return 1;
    }

    uint64 total_permutations = get_factorial(n);

    // Равномерное деление пространства перестановок
    uint64 base_count = total_permutations / num_procs;
    uint64 remainder = total_permutations % num_procs;

    uint64 *send_counts = (uint64*)malloc(num_procs * sizeof(uint64));
    uint64 *displacements = (uint64*)malloc(num_procs * sizeof(uint64));

    uint64 current_disp = 0;
    for (int i = 0; i < num_procs; i++) {
        send_counts[i] = base_count + (i < remainder ? 1 : 0);
        displacements[i] = current_disp;
        current_disp += send_counts[i];
    }

    uint64 my_count = send_counts[rank];
    uint64 my_start = displacements[rank];

    MPI_Barrier(MPI_COMM_WORLD);

    // Вызов ПОСЛЕДОВАТЕЛЬНОЙ программы (только на процессе 0)
    double seq_time = 0.0;
    if (rank == 0) {
        double start_time = MPI_Wtime();
        run_sequential(n, total_permutations);
        seq_time = MPI_Wtime() - start_time;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // Вызов ПАРАЛЛЕЛЬНОЙ программы (вызывают все процессы в main)
    double par_start_time = MPI_Wtime();
    run_parallel(n, my_start, my_count, rank);
    double my_par_time = MPI_Wtime() - par_start_time;

    double max_par_time = 0.0;
    MPI_Reduce(&my_par_time, &max_par_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // Verification of results
    int match = verify_results(total_permutations, num_procs, send_counts);

    // Beautiful tabular output for the MPI Permutations project
    if (rank == 0) {
        double speedup = seq_time / max_par_time;

        printf("\n");
        printf("+------------------------------------------+-----------------------+\n");
        printf("|              MPI Permutation Generation Benchmarks               |\n");
        printf("+------------------------------------------+-----------------------+\n");
        printf("| %-40s | %-21s |\n", "Metric / Configuration Parameter", "Value");
        printf("+------------------------------------------+-----------------------+\n");
        printf("| Elements in Set (N)                      | %-21d |\n", n);
        printf("| Total Permutations to Generate (N!)      | %-21llu |\n", total_permutations);
        printf("| MPI Processes (Degree of Parallelism)    | %-21d |\n", num_procs);
        printf("+------------------------------------------+-----------------------+\n");
        printf("| Sequential Execution Time                | %-17.6f sec |\n", seq_time);
        printf("| Parallel Execution Time (Max Process)    | %-17.6f sec |\n", max_par_time);
        printf("+------------------------------------------+-----------------------+\n");
        printf("| Achieved Speedup Factor                  | %-20.2fx |\n", speedup);
        printf("+------------------------------------------+-----------------------+\n");
        printf("| Software Verification Status             | %-21s |\n", 
               match ? "SUCCESS (Identical)" : "ERROR (Mismatch!)");
        printf("+------------------------------------------+-----------------------+\n");
        printf("\n");
    }



    free(send_counts);
    free(displacements);
    MPI_Finalize();
    return 0;
}
