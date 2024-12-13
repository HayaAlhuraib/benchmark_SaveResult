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

        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            help = true;
            break;
        }
    }

    if (help || (impl == NULL && !run_both)) {
        printf("Usage: %s -i {naive|opt|vec|para|both} [Options]\n", argv[0]);
        printf("\nOptions:\n");
        printf("  -h | --help       Print this message\n");
        printf("  -n | --nthreads   Set number of threads available (default = %d)\n", nthreads);
        printf("  -c | --cpu        Set the main CPU for the program (default = %d)\n", cpu);
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

    while (cols_A != rows_B) {
        printf("Number of columns for Matrix A must equal the number of rows for Matrix B.\n");
        printf("Enter the number of rows for Matrix B: ");
        scanf("%zu", &rows_B);
    }

    printf("Enter the number of columns for Matrix B: ");
    scanf("%zu", &cols_B);

    /* Create the Result directory */
    create_result_directory();

    /* Allocate matrices */
    size_t size_A = rows_A * cols_A;
    size_t size_B = rows_B * cols_B;
    size_t size_R = rows_A * cols_B;

    float* A = malloc(size_A * sizeof(float));
    float* B = malloc(size_B * sizeof(float));
    float* R = malloc(size_R * sizeof(float));

    if (!A || !B || !R) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }

    /* Initialize matrices with random values */
    srand((unsigned int)time(NULL));
    for (size_t i = 0; i < size_A; i++) {
        A[i] = (float)(rand() % 10);
    }
    for (size_t i = 0; i < size_B; i++) {
        B[i] = (float)(rand() % 10);
    }

    /* Print input matrices */
    print_matrix("Matrix A", A, rows_A, cols_A);
    print_matrix("Matrix B", B, rows_B, cols_B);

    /* Perform the selected implementation(s) */
    if (run_both) {
        printf("Running naive implementation...\n");
        args_t args_naive = { .input = A, .output = R, .size = rows_A };
        clock_t start_naive = clock();
        impl_scalar_naive(&args_naive);
        clock_t end_naive = clock();
        double naive_time = (double)(end_naive - start_naive) / CLOCKS_PER_SEC;
        printf("Naive Implementation Runtime: %.6f seconds\n", naive_time);
        print_matrix("Result Matrix R (Naive)", R, rows_A, cols_B);
    } else {
        args_t args = { .input = A, .output = R, .size = rows_A };
        clock_t start = clock();
        impl(&args);
        clock_t end = clock();
        double runtime = (double)(end - start) / CLOCKS_PER_SEC;
        printf("%s Implementation Runtime: %.6f seconds\n", impl_str, runtime);
        print_matrix("Result Matrix R", R, rows_A, cols_B);
    }

    /* Free allocated memory */
    free(A);
    free(B);
    free(R);

    return 0;
}
