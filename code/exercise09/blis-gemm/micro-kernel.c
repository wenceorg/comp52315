#include "parameters.h"

static inline void micro_kernel(int kc,
                                const double * restrict A,
                                const double * restrict B,
                                double * restrict AB)
{
  /* Compute a little MR x NR output block in C. */
  int i, j, l;

  /*
    Different compilers have different unrolling/vectorisation pragmas

    Unrolling:

    - Intel: #pragma unroll
    - Clang: #pragma clang loop unroll_count(n)
    - GCC:   #pragma GCC unroll(n)

    For SIMD vectorisation, you can say

    #pragma omp simd

    Then clang and gcc need the commandline flag -fopenmp-simd

    Intel needs -Qopenmp-simd
  */

  /* For every "block" column */
  for (l = 0; l < kc; ++l)
    for (j = 0; j < NR; ++j)
      for (i = 0; i < MR; ++i)
        /* Multiply row of A into column of B. */
        AB[i + j*MR] += A[i + MR*l] * B[j + NR*l];
}
