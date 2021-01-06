#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

static char *format_binary(uintptr_t x, char *b)
{
  /* Assumes little-endian */
  const unsigned nbits = sizeof(uintptr_t)*CHAR_BIT;
  const unsigned nseps = nbits / 8;
  b += nbits + nseps - 1;
  *b = '\0';
  for (int z = 0; z < nbits; z++) {
    if (z && !(z % 8)) {
      *(--b) = '_';
    }
    *(--b) = '0' + ((x>>z) & 0x1);
  }
  return b;
}

int main(int argc, char **argv)
{
  int a = 1;
  double *b = malloc(4*sizeof(*b));
  int *c = malloc(4*sizeof(*c));
  char buf[sizeof(uintptr_t)*CHAR_BIT + (sizeof(uintptr_t)*CHAR_BIT)/8];

  printf("Address of a    is: %p %s\n", &a,
         format_binary((uintptr_t)&a, buf));
  printf("\n");
  for (int i = 0; i < 4; i++) {
    printf("Address of b[%d] is: %p %s\n", i, &b[i],
           format_binary((uintptr_t)&b[i], buf));
  }
  printf("\n");
  for (int i = 0; i < 4; i++) {
    format_binary((uintptr_t)&c[i], buf);
    printf("Address of c[%d] is: %p %s\n", i, &c[i],
           format_binary((uintptr_t)&c[i], buf));
  }
  free(b);
  return 0;
}
