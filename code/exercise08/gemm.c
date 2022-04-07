#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <errno.h>

#ifdef LIKWID_PERFMON
#include <likwid.h>
#else
#define LIKWID_MARKER_START(a) do { (void)a; } while (0)
#define LIKWID_MARKER_STOP(a) do { (void)a; } while (0)
#define LIKWID_MARKER_INIT do { } while (0)
#define LIKWID_MARKER_THREADINIT do { } while (0)
#define LIKWID_MARKER_CLOSE do { } while (0)
#define LIKWID_MARKER_REGISTER(a) do { (void)a; } while (0)
#endif  /* LIKWID_PERFMON */

typedef void (*gemm_fn_t)(int, int, int,
                          const double *, int,
                          const double *, int,
                          double *, int);

/* Compute C = C + A*B
 *
 * C has rank m x n (m rows, n columns)
 * A has rank m x k
 * B has rank k x n
 * ldX is the leading dimension of the respective matrix.
 *
 * All matrices are stored in column major format.
 * That is, for row index i, column index j, and leading dimension ldX,
 * the correct entry is at (ldX*i + j).
 */
static void basic_gemm(int m, int n, int k,
                       const double *a, int lda,
                       const double *b, int ldb,
                       double *c, int ldc)
{
  int i, j, p;
  LIKWID_MARKER_START("BASIC_GEMM");
  for (j = 0; j < n; j++) {
    for (p = 0; p < k; p++) {
      for (i = 0; i < m; i++) {
        c[j*ldc + i] = c[j*ldc + i] + a[p*lda + i] * b[j*ldb + p];
      }
    }
  }
  LIKWID_MARKER_STOP("BASIC_GEMM");
}

#ifndef TILESIZE
#define TILESIZE 64
#endif
static void tiled_gemm(int m, int n, int k,
                       const double * restrict a, int lda,
                       const double * restrict b, int ldb,
                       double * restrict c, int ldc)
{
  const int ilim = (m / TILESIZE) * TILESIZE;
  const int jlim = (n / TILESIZE) * TILESIZE;
  const int plim = (k / TILESIZE) * TILESIZE;
  int ii, jj, pp, i, j, p;

  if (ilim != m || jlim != n || plim != k) {
    fprintf(stderr, "Tile size %d must evenly divide matrix dimension %d\n", TILESIZE, m);
    exit(1);
  }
  LIKWID_MARKER_START("TILED_GEMM");
  for (jj = 0; jj < jlim; jj += TILESIZE) {
    for (pp = 0; pp < plim; pp += TILESIZE) {
      for (ii = 0; ii < ilim; ii += TILESIZE) {
        for (j = 0; j < TILESIZE; j++) {
          const int j_ = j + jj;
          for (p = 0; p < TILESIZE; p++) {
            const int p_ = p + pp;
            for (i = 0; i < TILESIZE; i++) {
              const int i_ = i + ii;
              c[j_*ldc + i_] += a[p_*lda + i_] * b[j_*ldb + p_];
            }
          }
        }
      }
    }
  }
  LIKWID_MARKER_STOP("TILED_GEMM");
}

static void tiled_packed_gemm(int m, int n, int k,
                              const double * restrict a, int lda,
                              const double * restrict b, int ldb,
                              double * restrict c, int ldc)
{
  const int ilim = (m / TILESIZE) * TILESIZE;
  const int jlim = (n / TILESIZE) * TILESIZE;
  const int plim = (k / TILESIZE) * TILESIZE;
  int ii, jj, pp, i, j, p;
  double bpack[TILESIZE*TILESIZE] __attribute__((aligned(64)));
  double apack[TILESIZE*TILESIZE] __attribute__((aligned(64)));
  double cpack[TILESIZE*TILESIZE] __attribute__((aligned(64)));
  if (ilim != m || jlim != n || plim != k) {
    fprintf(stderr, "Tile size %d must evenly divide matrix dimension %d\n", TILESIZE, m);
    exit(1);
  }
  LIKWID_MARKER_START("TILED_PACKED_GEMM");
  for (jj = 0; jj < jlim; jj += TILESIZE) {
    for (pp = 0; pp < plim; pp += TILESIZE) {
      for (j = 0; j < TILESIZE; j++) {
        const int j_ = j + jj;
        for (p = 0; p < TILESIZE; p++) {
          const int p_ = p + pp;
          bpack[j*TILESIZE + p] = b[j_*ldb + p_];
        }
      }
      for (ii = 0; ii < ilim; ii += TILESIZE) {
        for (p = 0; p < TILESIZE; p++) {
          const int p_ = p + pp;
          for (i = 0; i < TILESIZE; i++) {
            const int i_ = i + ii;
            apack[i*TILESIZE + p] = a[p_*lda + i_];
          }
        }
        for (j = 0; j < TILESIZE; j++) {
          const int j_ = j + jj;
          for (i = 0; i < TILESIZE; i++) {
            const int i_ = i + ii;
#ifdef SIMD_REDUCTION
            double c_ = 0;
#pragma omp simd reduction (+: c_)
            for (p = 0; p < TILESIZE; p++) {
              c_ += apack[i*TILESIZE + p] * bpack[j*TILESIZE + p];
            }
            c[j_*ldc + i_] += c_;
#else
            for (p = 0; p < TILESIZE; p++) {
              c[j_*ldc + i_] += apack[i*TILESIZE + p] * bpack[j*TILESIZE + p];
            }
#endif
          }
        }
      }
    }
  }
  LIKWID_MARKER_STOP("TILED_PACKED_GEMM");
}

static void alloc_matrix(int m, int n, double **a)
{
  int err;
  err = posix_memalign((void **)a, 64, m*n*sizeof(**a));
  if (err) {
    fprintf(stderr, "posix_memalign failed: ");
    switch (err) {
    case EINVAL:
      fprintf(stderr, "alignment is not a power of 2\n");
      break;
    case ENOMEM:
      fprintf(stderr, "memory allocation error\n");
      break;
    default:
      fprintf(stderr, "reason unknown\n");
    }
    exit(1);
  }
}

static void free_matrix(double **a)
{
  free(*a);
  *a = NULL;
}

static void zero_matrix(int m, int n, double *a, int lda)
{
  int i, j;
  for (j = 0; j < n; j++) {
    for (i = 0; i < m; i++) {
      a[j*lda + i] = 0.0;
    }
  }    
}

static void random_matrix(int m, int n, double *a, int lda)
{
  int i, j;
  for (j = 0; j < n; j++) {
    for (i = 0; i < m; i++) {
      a[j*lda + i] = drand48();
    }
  }
}

static double diff_time(struct timespec end, struct timespec start)
{
  double secs = (double)end.tv_sec - (double)start.tv_sec;
  double nsecs = (double)end.tv_nsec - (double)start.tv_nsec;
  return secs + 1e-9*nsecs;
}

/*
 * Benchmark the provided gemm implementation.
 * n: matrix size
 * gemm: Function pointer to gemm implementation
 * prints:
 *  n TIME FLOPs FLOPs/s
 */
static void bench(int n, gemm_fn_t gemm)
{
  double *a = NULL;
  double *b = NULL;
  double *c = NULL;
  struct timespec start, end;
  double time, flop;
  int repeats, i;
  int lda, ldb, ldc;

  alloc_matrix(n, n, &a);
  alloc_matrix(n, n, &b);
  alloc_matrix(n, n, &c);

  lda = n;
  ldb = n;
  ldc = n;

  random_matrix(n, n, a, lda);
  random_matrix(n, n, b, ldb);
  zero_matrix(n, n, c, ldc);

  flop = 2.0*(double)n*(double)n*(double)n;
  time = DBL_MAX;
    
  if (n*n < 10000) {
    /* For small matrices, run in a loop, to help with timing variability. */
    repeats = 50;
  } else {
    repeats = 2;
  }

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
  for (i = 0; i < repeats; i++) {
    gemm(n, n, n,
         (const double *)a, lda,
         (const double *)b, ldb,
         c, ldc);
  }
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
  time = diff_time(end, start) / repeats;
  printf("%d %g %g %g\n", n, time, flop, flop/time);
  free_matrix(&a);
  free_matrix(&b);
  free_matrix(&c);
}

int main(int argc, char **argv)
{
  int n;
  gemm_fn_t gemm;
  if (argc != 3) {
    fprintf(stderr, "Invalid arguments.\n");
    fprintf(stderr, "Usage: %s N version\n", argv[0]);
    fprintf(stderr, "Where N is the dimension of the matrices.\n");
    fprintf(stderr, "'version' is one of BASIC, TILED, or TILEDPACKED\n");
    return 1;
  }

  n = atoi(argv[1]);

  if (!strcmp(argv[2], "BASIC")) {
    gemm = &basic_gemm;
  } else if (!strcmp(argv[2], "TILED")) {
    gemm = &tiled_gemm;
  } else if (!strcmp(argv[2], "TILEDPACKED")) {
    gemm = &tiled_packed_gemm;
  } else {
    fprintf(stderr, "Unknown GEMM variant '%s'\n", argv[2]);
    return 1;
  }
  LIKWID_MARKER_INIT;
  LIKWID_MARKER_THREADINIT;
  LIKWID_MARKER_REGISTER("BASIC_GEMM");
  LIKWID_MARKER_REGISTER("TILED_GEMM");
  LIKWID_MARKER_REGISTER("TILED_PACKED_GEMM");
  bench(n, gemm);
  LIKWID_MARKER_CLOSE;
  return 0;
}
