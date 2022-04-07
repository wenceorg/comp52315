#define MR 1
#define NR 1

void micro_kernel(int kc,
                  const double * restrict A,
                  const double * restrict B,
                  double * restrict AB)
{
  int i, j, l;
  for (l = 0; l < kc; ++l) {
    for (j = 0; j < NR; ++j) {
      for (i = 0; i < MR; ++i) {
        AB[i + j*MR] += A[i] * B[j];
      }
    }
    A += MR;
    B += NR;
  }
}
