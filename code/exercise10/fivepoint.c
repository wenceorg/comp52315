#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <float.h>
#include <string.h>

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


typedef enum LoopMode {NORMAL, TILED} LoopMode;

#define idx(i, j) ((i)*rsize + (j))

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static double timestamp()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec * 1.e-9;
}

static double run_jacobi_normal(double * restrict x, double * restrict y, size_t rsize, size_t csize, size_t maxiter)
{
  size_t i, j, iter = 0;
  double *x_old, *x_new;
  double start, end;

  LIKWID_MARKER_START("UNTILED");
  start = timestamp();
  while (iter < maxiter) {
    if (iter%2) {
      x_old = x;
      x_new = y;
    } else {
      x_old = y;
      x_new = x;
    }
    for (i = 1; i < csize - 1; i++) {
      for (j = 1; j < rsize - 1; j++) {
        x_new[idx(i, j)] = 0.25*((x_old[idx(i-1, j)] +
                                  x_old[idx(i+1, j)] +
                                  x_old[idx(i, j-1)] +
                                  x_old[idx(i, j+1)] -
                                  4*x_old[idx(i, j)]));
      }
    }
    iter++;
  }
  end = timestamp();

  LIKWID_MARKER_STOP("UNTILED");
  return end - start;
}

static double run_jacobi_tiled(double * restrict x, double * restrict y, size_t rsize, size_t csize, size_t blocksize, size_t maxiter)
{
  size_t i, jb, j, iter = 0;
  double *x_old, *x_new;
  double start, end;
  LIKWID_MARKER_START("TILED");

  start = timestamp();
  while (iter < maxiter) {
    if (iter%2) {
      x_old = x;
      x_new = y;
    } else {
      x_old = y;
      x_new = x;
    }
    for (jb = 1; jb < rsize - 1; jb += blocksize) {
      for (i = 1; i < csize - 1; i++) {
        for (j = jb; j < MIN(jb + blocksize, rsize - 1); j++) {
          x_new[idx(i, j)] = 0.25*((x_old[idx(i-1, j)] +
                                    x_old[idx(i+1, j)] +
                                    x_old[idx(i, j-1)] +
                                    x_old[idx(i, j+1)] -
                                    4*x_old[idx(i, j)]));
        }
      }
    }
    iter++;
  }
  end = timestamp();
  LIKWID_MARKER_STOP("TILED");
  return end - start;
}

int main(int argc, char **argv)
{
  size_t rsize, csize, blocksize, maxiter = 1, i, j;
  double *x = NULL, *y = NULL;
  double runtime = 0;
  
  LoopMode mode;

  if ( argc < 4 || argc > 5) {
    printf("Usage: %s <MODE> <rows> <cols> [<tile size>]\n", argv[0]);
    printf("  Where MODE is one of NORMAL or TILED\n");
    printf("  If MODE is TILED then a tile size must be specified\n");
    exit(EXIT_SUCCESS);
  } else {
    rsize = atoi(argv[2]);
    csize = atoi(argv[3]);
    if (!strcmp(argv[1], "TILED")) {
      if (argc != 5) {
        printf("Must provide tile size when selecting TILED mode\n");
        exit(EXIT_SUCCESS);
      }
      blocksize = atoi(argv[4]);
      mode = TILED;
    } else if(!strcmp(argv[1], "NORMAL")) {
      mode = NORMAL;
    } else {
      printf("Unknown loop mode '%s', expecting one of NORMAL or TILED\n", argv[1]);
      exit(EXIT_SUCCESS);
    }
  }

  LIKWID_MARKER_INIT;
  LIKWID_MARKER_THREADINIT;
  LIKWID_MARKER_REGISTER("TILED");
  LIKWID_MARKER_REGISTER("UNTILED");
  x = malloc(rsize * csize * sizeof(*x));
  y = malloc(rsize * csize * sizeof(*y));
  for(i=0; i < csize; i++) {
    for(j=0; j < rsize; j++) {
      x[idx(i, j)] = (double)(i*j);
      y[idx(i, j)] = (double)(i*j);
    }
  }	  


  do {
    switch (mode) {
    case NORMAL:
      runtime = run_jacobi_normal(x, y, rsize, csize, maxiter);
      break;
    case TILED:
      runtime = run_jacobi_tiled(x, y, rsize, csize, blocksize, maxiter);
      break;
    }
    maxiter *= 2;
  } while (runtime < 1.5);
  maxiter /= 2;

  switch (mode) {
  case NORMAL:
    printf("NORMAL ");
    break;
  case TILED:
    printf("TILED(%zu) ", blocksize);
  }
  printf("size: (%zu, %zu) time: %lf iterations: %zu MLUP/s: %lf\n", rsize, csize,
         runtime, maxiter, 1e-6*maxiter*(rsize-2)*(csize-2)/runtime);
  free(x);
  free(y);

  LIKWID_MARKER_CLOSE;
  return 0;
}
