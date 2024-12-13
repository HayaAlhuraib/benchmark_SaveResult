/* Set features */
#define _GNU_SOURCE

/* Standard C includes */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sched.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Include all implementations declarations */
#include "impl/ref.h"
#include "impl/naive.h"
#include "impl/opt.h"
#include "impl/vec.h"
#include "impl/para.h"

/* Include common headers */
#include "common/types.h"
#include "common/macros.h"

/* Include application-specific headers */
#include "include/types.h"

/* Default values */
const int SIZE_DATA = 4 * 1024 * 1024;
int num_runs = 10000;
int nstdevs = 3;

/* Helper function to print a matrix */
void print_matrix(const char* name, float* matrix, size_t rows, size_t cols) {
    printf("%s:\n", name);
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            printf("%.2f ", matrix[i * cols + j]);
        }
        printf("\n");
    }
    printf("\n");
}

/* Helper function to create the Result directory */
void create_result_directory() {
    struct stat st = {0};
    if (stat("Result", &st) == -1) {
        if (mkdir("Result", 0700) == 0) {
            printf("Result directory created successfully.\n");
        } else {
            perror("Error creating Result directory");
        }
    }
}

/* Helper function to export a matrix to a CSV file in the Result directory */
void export_matrix_to_csv(const char* filename, float* matrix, size_t rows, size_t cols) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "Result/%s", filename);

    FILE* file = fopen(filepath, "w");
    if (!file) {
        fprintf(stderr, "Error opening file %s for writing.\n", filepath);
        return;
    }

    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            fprintf(file, "%.6f", matrix[i * cols + j]);
            if (j < cols - 1) {
                fprintf(file, ",");
            }
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

int main(int argc, char** argv) {
    /* Set the buffer for printf to NULL */
    setbuf(stdout, NULL);

    /* Arguments */
    int nthreads = 1;
    int cpu = 0;
    int data_size = SIZE_DATA;
    void* (*impl)(void* args) = NULL;
    const char* impl_str = NULL;
    bool run_both = false;
    bool help = false;

    /* Parse command-line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--impl") == 0) {
            assert(++i < argc);
            if (strcmp(argv[i], "naive") == 0) {
                impl = impl_scalar_naive;
                impl_str = "naive";
            } else if (strcmp(argv[i], "opt") == 0) {
                impl = impl_scalar_opt;
                impl_str = "opt";
            } else if (strcmp(argv[i], "vec") == 0) {
                impl = impl_vector;
                impl_str = "vectorized";
            } else if (strcmp(argv[i], "para") == 0) {
                impl = impl_parallel;
                impl_str = "parallelized";
            } else if (strcmp(argv[i], "both") == 0) {
                run_both = true;
            } else {
                fprintf(stderr, "Unknown implementation: %s\n", argv[i]);
                exit(1);
            }
            continue;
        }

        if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--nthreads") == 0) {
            assert(++i < argc);
            nthreads = atoi(argv[i]);
            continue;
        }

        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cpu") == 0) {
            assert(++i < argc);
            cpu = atoi(argv[i]);
            continue;
        }

        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--size") == 0) {
            assert(++i < argc);
            data_size = atoi(argv[i]);
            continue;
        }

        if (strcmp(argv[i], "--nruns") == 0) {
            assert(++i < argc);
            num_runs = atoi(argv[i]);
            continue;
        }

        if (strcmp(argv[i], "--nstdevs") == 0) {
            assert(++i < argc);
            nstdevs = atoi(argv[i]);
            continue;
        }

        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            help = true;
            break;
        }
    }

    if (help || impl == NULL) {
        printf("Usage: %s -i {naive|opt|vec|para|both} [Options]\n", argv[0]);
        printf("\nOptions:\n");
        printf("  -h | --help       Print this message\n");
        printf("  -n | --nthreads   Set number of threads available (default = %d)\n", nthreads);
        printf("  -c | --cpu        Set the main CPU for the program (default = %d)\n", cpu);
        printf("  -s | --size       Size of input and output data (default = %d)\n", data_size);
        printf("     --nruns        Number of runs to the implementation (default = %d)\n", num_runs);
        printf("     --nstdevs      Number of standard deviations to exclude outliers (default = %d)\n", nstdevs);
        exit(help ? 0 : 1);
    }

    /* Prompt the user for matrix dimensions */
    size_t rows_A, cols_A, rows_B, cols_B;

    printf("Enter the number of rows for Matrix A: ");
    scanf("%zu", &rows_A);

    printf("Enter the number of columns for Matrix A: ");
    scanf("%zu", &cols_A);

    printf("Enter the number of rows for Matrix B: ");
    scanf("%zu", &rows_B);

    /* Ensure cols_A == rows_B */
    while (cols_A != rows_B) {
        printf("Number of columns for Matrix A must equal the number of rows for Matrix B.\n");
        printf("Enter the number of rows for Matrix B: ");
        scanf("%zu", &rows_B);
    }

    printf("Enter the number of columns for Matrix B: ");
    scanf("%zu", &cols_B);

    /* Print parsed values for verification */
    printf("\nConfiguration:\n");
    printf("  Implementation: %s\n", impl_str);
    printf("  Number of Threads: %d\n", nthreads);
    printf("  CPU: %d\n", cpu);
    printf("  Data Size: %d\n", data_size);
    printf("  Matrix A: %zux%zu\n", rows_A, cols_A);
    printf("  Matrix B: %zux%zu\n", rows_B, cols_B);

    return 0;
}
