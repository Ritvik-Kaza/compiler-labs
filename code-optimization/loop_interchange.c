#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}

static void mm_naive(const double *A, const double *B, double *C, int n)
{
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            double s = 0.0;
            for (int k = 0; k < n; k++)
                s += A[(size_t)i * n + k] * B[(size_t)k * n + j];
            C[(size_t)i * n + j] = s;
        }
}

static void mm_interchange(const double *A, const double *B, double *C, int
n)
{
    memset(C, 0, (size_t)n * n * sizeof(double));
    for (int i = 0; i < n; i++)
        for (int k = 0; k < n; k++)
            for (int j = 0; j < n; j++)
                C[(size_t)i * n + j] += A[(size_t)i * n + k] * B[(size_t)k *
n + j];
}

int main(int argc, char **argv)
{
    int n = argc > 1 ? atoi(argv[1]) : 1024;
    if (n <= 0) {
        fprintf(stderr, "usage: %s [n>0]\n", argv[0]);
        return 1;
    }

    size_t len = (size_t)n * n;
    double *A = malloc(len * sizeof *A);
    double *B = malloc(len * sizeof *B);
    double *C1 = malloc(len * sizeof *C1);
    double *C2 = malloc(len * sizeof *C2);
    if (!A || !B || !C1 || !C2) {
        fprintf(stderr, "allocation failed\n");
        return 1;
    }

    srand(42);
    for (size_t i = 0; i < len; i++) {
        A[i] = (double)rand() / RAND_MAX;
        B[i] = (double)rand() / RAND_MAX;
    }

    double t0 = now();
    mm_naive(A, B, C1, n);
    double t_naive = now() - t0;

    double t_opt = 1e30;
    for (int r = 0; r < 3; r++) {
          t0 = now();
          mm_interchange(A, B, C2, n);
          double t = now() - t0;
          if (t < t_opt) t_opt = t;
    }

    double maxdiff = 0.0;
    for (size_t i = 0; i < len; i++) {
        double d = fabs(C1[i] - C2[i]);
        if (d > maxdiff) maxdiff = d;
    }

    printf("n = %d\n", n);
    printf("naive (ijk)       : %.3f s\n", t_naive);
    printf("interchange (ikj) : %.3f s\n", t_opt);
    printf("speedup           : %.2fx\n", t_naive / t_opt);
    printf("max |difference| : %.2e\n", maxdiff);

    free(A); free(B); free(C1); free(C2);
    return 0;
}

