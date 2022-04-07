/*
 * =======================================================================================
 *
 *      Author:   Jan Eitzinger (je), jan.eitzinger@fau.de
 *      Copyright (c) 2019 RRZE, University Erlangen-Nuremberg
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a copy
 *      of this software and associated documentation files (the "Software"), to deal
 *      in the Software without restriction, including without limitation the rights
 *      to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *      copies of the Software, and to permit persons to whom the Software is
 *      furnished to do so, subject to the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included in all
 *      copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *      FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *      AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *      LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *      OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *      SOFTWARE.
 *
 * =======================================================================================
 */
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

double dmvm(
            double * restrict y,
            const double * restrict a,
            const double * restrict x,
            int N_rows,
            int N_cols,
            int iter
            )
{
  double S, E;

  S = getTimeStamp();
  LIKWID_MARKER_START("bench");

  for(int j = 0; j < iter; j++) {
    for (int c=0; c<N_cols; c++) {
      for (int r=0; r<N_rows; r++) {
        y[r] = y[r] + a[c*N_rows+r] * x[c];
      }
    }
    if (a[N_rows-1] > 2000) printf("Ai = %f\n",a[N_rows-1]);
  }
  LIKWID_MARKER_STOP("bench");
  E = getTimeStamp();

  return E-S;
}

double dmvm_test(
                 double * restrict y,
                 const double * restrict a,
                 const double * restrict x,
                 int N_rows,
                 int N_cols,
                 int iter
                 )
{
  double S, E;

  S = getTimeStamp();
  for(int j = 0; j < iter; j++) {
    for (int c=0; c<N_cols; c++) {
      for (int r=0; r<N_rows; r++) {
        y[r] = y[r] + a[c*N_rows+r] * x[c];
      }
    }
    if (a[N_rows-1] > 2000) printf("Ai = %f\n",a[N_rows-1]);
  }
  E = getTimeStamp();

  return E-S;
}


int main (int argc, char** argv)
{
  size_t bytesPerWord = sizeof(double);
  size_t N_rows = 0;
  size_t N_cols = 0;
  size_t iter = 1;
  double *a, *x, *y;
  double E, S;
  double times[2];
  double walltime;

  if ( argc > 2 ) {
    N_rows = atoi(argv[1]);
    N_cols = atoi(argv[2]);
  } else {
    printf("Usage: %s <N rows> <N columns>\n",argv[0]);
    exit(EXIT_SUCCESS);
  }

  LIKWID_MARKER_INIT;
  LIKWID_MARKER_REGISTER("bench");

  posix_memalign((void**) &a, ARRAY_ALIGNMENT, N_rows * N_cols * bytesPerWord );
  posix_memalign((void**) &x, ARRAY_ALIGNMENT, N_cols * bytesPerWord );
  posix_memalign((void**) &y, ARRAY_ALIGNMENT, N_rows * bytesPerWord );

  for (int i=0; i<N_rows; i++) {
    y[i] = 3.0 * (double) i/N_rows;

    for (int j=0; j<N_cols; j++) {
      x[j] = 2.0 * (double) i/N_rows;
      a[i*N_cols + j] = (double) i * j/(N_rows*N_cols);
    }
  }

  times[0] = 0.0;
  times[1] = 0.0;

  while ( times[0] < 0.6 ){
    times[0] = dmvm_test(y, a, x, N_rows, N_cols, iter);
    if ( times[0] > 0.2 ) break;
    double factor = 0.6 / (times[0] - times[1]);
    iter *= (int) factor;
    times[1] = times[0];
  }

  walltime = dmvm(y, a, x, N_rows, N_cols, iter);

  double flops = (double) 2.0 * N_cols * N_rows * iter;
  printf("%zu %zu %zu %.2f\n", iter, N_rows, N_cols, 1.0E-06 * flops/walltime);

  LIKWID_MARKER_CLOSE;
  return EXIT_SUCCESS;
}
