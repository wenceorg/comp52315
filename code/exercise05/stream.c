/* Implementation of c[i] = c[i] + a[i] * b[i] with different instruction sets */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <immintrin.h>
#include <time.h>

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

__attribute__((optimize("no-tree-vectorize")))
static void  scalar_loop(int n,
                         const double * restrict a,
                         const double * restrict b,
                         double * restrict c)
{
  LIKWID_MARKER_START("Scalar");
#pragma clang loop vectorize(disable)
#pragma novector
  for (int i = 0; i < n; i++) {
    c[i] = c[i] + a[i]*b[i];
  }
  LIKWID_MARKER_STOP("Scalar");
}

static void sse_loop(int n,
                     const double * restrict a,
                     const double * restrict b,
                     double * restrict c)
{
  __m128d a_, b_, c_;
  LIKWID_MARKER_START("SSE");
  for (int i = 0; i < n; i+= 2) {
    __m128d tmp;
    a_ = _mm_loadu_pd(a + i);
    b_ = _mm_loadu_pd(b + i);
    c_ = _mm_loadu_pd(c + i);
    tmp = _mm_mul_pd(a_, b_);
    c_ = _mm_add_pd(c_, tmp);
    _mm_storeu_pd(c + i, c_);
  }
  LIKWID_MARKER_STOP("SSE");
}

static void avx_loop(int n,
                     const double * restrict a,
                     const double * restrict b,
                     double * restrict c)
{
  __m256d a_, b_, c_;
  LIKWID_MARKER_START("AVX");
  for (int i = 0; i < n; i += 4) {
    __m256d tmp;
    a_ = _mm256_loadu_pd(a + i);
    b_ = _mm256_loadu_pd(b + i);
    c_ = _mm256_loadu_pd(c + i);
    tmp = _mm256_mul_pd(a_, b_);
    c_ = _mm256_add_pd(c_, tmp);
    _mm256_storeu_pd(c + i, c_);
  }
  LIKWID_MARKER_STOP("AVX");
}


static void fma_loop(int n,
                     const double * restrict a,
                     const double * restrict b,
                     double * restrict c)
{
  __m256d a_, b_, c_;
  LIKWID_MARKER_START("FMA");
  for (int i = 0; i < n; i += 4) {
    a_ = _mm256_loadu_pd(a + i);
    b_ = _mm256_loadu_pd(b + i);
    c_ = _mm256_loadu_pd(c + i);
    c_ = _mm256_fmadd_pd(a_, b_, c_);
    _mm256_storeu_pd(c + i, c_);
  }
  LIKWID_MARKER_STOP("FMA");
}

static void unaligned(int n,
                      const double * restrict a,
                      const double * restrict b,
                      double * restrict c)
{
  register __m256d a_, b_, c_;
  LIKWID_MARKER_START("UNALIGNED");
  for (int i = 0; i < n; i += 4) {
    register __m256d tmp;
    a_ = _mm256_loadu_pd(a + i);
    b_ = _mm256_loadu_pd(b + i);
    c_ = _mm256_loadu_pd(c + i);
    tmp = _mm256_mul_pd(a_, b_);
    c_ = _mm256_add_pd(c_, tmp);
    _mm256_storeu_pd(c + i, c_);
  }
  LIKWID_MARKER_STOP("UNALIGNED");
}

static void aligned(int n,
                    const double * restrict a,
                    const double * restrict b,
                    double * restrict c)
{
  register __m256d a_, b_, c_;
  LIKWID_MARKER_START("ALIGNED");
  for (int i = 0; i < n; i += 4) {
    register __m256d tmp;
    a_ = _mm256_load_pd(a + i);
    b_ = _mm256_load_pd(b + i);
    c_ = _mm256_load_pd(c + i);
    tmp = _mm256_mul_pd(a_, b_);
    c_ = _mm256_add_pd(c_, tmp);
    _mm256_store_pd(c + i, c_);
  }
  LIKWID_MARKER_STOP("ALIGNED");
}
                    

int main(int argc, char **argv)
{
  double *a = NULL;
  double *b = NULL;
  double *c = NULL;
  double sum = 0;
  int n;
  LIKWID_MARKER_INIT;
  LIKWID_MARKER_THREADINIT;
  LIKWID_MARKER_REGISTER("Scalar");
  LIKWID_MARKER_REGISTER("SSE");
  LIKWID_MARKER_REGISTER("AVX");
  LIKWID_MARKER_REGISTER("FMA");
  LIKWID_MARKER_REGISTER("UNALIGNED");
  LIKWID_MARKER_REGISTER("ALIGNED");

  if (argc != 3) {
    fprintf(stderr, "Usage: %s N LOOP_TYPE\n", argv[0]);
    fprintf(stderr, "Where LOOP_TYPE is one of:\n");
    fprintf(stderr, "  sca - scalar instructrions\n");
    fprintf(stderr, "  sse - sse instructrions\n");
    fprintf(stderr, "  avx - avx instructrions\n");
    fprintf(stderr, "  fma - avx + fma instructrions\n");
    fprintf(stderr, "  align - avx + 32byte aligned memory access\n");
    fprintf(stderr, "  unalign - avx + unaligned memory access\n");
    LIKWID_MARKER_CLOSE;
    return 1;
  }

  n = atoi(argv[1]);
  if (posix_memalign((void**)&a, 64, (n+1) * sizeof(*a)))
    return 1;
  if (posix_memalign((void**)&b, 64, (n+1) * sizeof(*b)))
    return 1;
  if (posix_memalign((void**)&c, 64, (n+1) * sizeof(*c)))
    return 1;

  for (int i = 0; i < n; i++) {
    a[i] = i;
    b[i] = i - 10;
    c[i] = 0;
  }
  if (!strcmp(argv[2], "sca")) {
    scalar_loop(n, a, b, c);
  } else if (!strcmp(argv[2], "sse")) {
    sse_loop(n, a, b, c);
  } else if (!strcmp(argv[2], "avx")) {
    avx_loop(n, a, b, c);
  } else if (!strcmp(argv[2], "fma")) {
    fma_loop(n, a, b, c);
  } else if (!strcmp(argv[2], "align")) {
    aligned(n, a, b, c);
  } else if (!strcmp(argv[2], "unalign")) {
    unaligned(n, a+1, b+1, c+1);
  } else {
    fprintf(stderr, "Unrecognised LOOP_TYPE: %s\n", argv[2]);
    free(a);
    free(b);
    free(c);
    LIKWID_MARKER_CLOSE;
    return 1;
  }
  for (int i = 0; i < n; i++) {
    sum += c[i];
  }
  LIKWID_MARKER_CLOSE;  
  printf("%s loop, sum %g\n", argv[2], sum);
  free(a);
  free(b);
  free(c);
  return 0;
}
