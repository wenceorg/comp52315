#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <float.h>

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

#define ARRAY_ALIGNMENT 64

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

double getTimeStamp()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec * 1.e-9;
}

#ifndef CSTRIDE
#define CSTRIDE 64
#endif
#ifndef RSTRIDE
#define RSTRIDE 64
#endif

static inline void single_transpose(double * restrict b,
                                    const double  * restrict a,
                                    size_t Nr, size_t Nc)
{
  size_t c_, c, r_, r;
  for (c_ = 0; c_ < Nc; c_ += CSTRIDE)
    for (r_ = 0; r_ < Nr; r_ += RSTRIDE)
      for (c = c_; c < MIN(c_ + CSTRIDE, Nc); c++)
        for (r = r_; r < MIN(r_ + RSTRIDE, Nr); r++)
          b[c*Nr + r] = a[r*Nc + c];
}

double transpose(double * restrict b,
                 const double * restrict a,
                 size_t Nr, size_t Nc, size_t iter)
{
  double S, E;
  size_t j;

  S = getTimeStamp();
  LIKWID_MARKER_START("transpose");

  for(j = 0; j < iter; j++) {
    single_transpose(b, a, Nr, Nc);
  }
  LIKWID_MARKER_STOP("transpose");
  E = getTimeStamp();

  return E-S;
}

double transpose_test(double * restrict b,
                      const double * restrict a,
                      size_t Nr, size_t Nc, size_t iter)
{
  double S, E;
  size_t j;

  S = getTimeStamp();
  for(j = 0; j < iter; j++) {
    single_transpose(b, a, Nr, Nc);
  }
  E = getTimeStamp();

  return E-S;
}


int main (int argc, char** argv)
{
  size_t bytesPerWord = sizeof(double);
  size_t Nr = 0;
  size_t Nc = 0;
  size_t i, j;
  size_t iter = 1;
  double *a, *b;
  double times[2];
  double walltime, bytes;

  if ( argc > 2 ) {
    Nr = atoi(argv[1]);
    Nc = atoi(argv[2]);
  } else {
    printf("Usage: %s <N rows> <N columns>\n",argv[0]);
    exit(EXIT_SUCCESS);
  }

  LIKWID_MARKER_INIT;
  LIKWID_MARKER_REGISTER("transpose");

  posix_memalign((void**) &a, ARRAY_ALIGNMENT, Nr * Nc * bytesPerWord );
  posix_memalign((void**) &b, ARRAY_ALIGNMENT, Nc * Nr * bytesPerWord );

  for (i=0; i<Nr; i++) {
    for (j=0; j<Nc; j++) {
      a[i*Nc + j] = (double) i * j/(Nr*Nc);
    }
  }

  times[0] = 0.0;
  times[1] = 0.0;

  while ( times[0] < 0.6 ){
    double factor;
    times[0] = transpose_test(b, a, Nr, Nc, iter);
    if ( times[0] > 0.2 ) break;
    factor = 0.6 / (times[0] - times[1]);
    iter *= (int) factor;
    times[1] = times[0];
  }

  walltime = transpose(b, a, Nr, Nc, iter);

  bytes = (double) bytesPerWord * Nc * Nr * iter;
  printf("Nrow Ncol EffectiveBW EffectiveLoadMBytes EffectiveStoreMBytes\n");
  printf("%zu %zu %.2f %.2f %.2f\n", Nr, Nc, 3 * 1.0E-06 * bytes /walltime, 2*bytes*1e-6, bytes*1e-6);

  LIKWID_MARKER_CLOSE;
  return EXIT_SUCCESS;
}
