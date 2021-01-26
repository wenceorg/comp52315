---
title: "Compiler feedback and the BLIS DGEMM"
weight: 9
---

# Getting compilers to the right thing

We're going to look at how to convince the compiler to
vectorise a loop the way we want it to.

As our starting point we'll use a C version of the [GEMM
micro-kernel]({{< code-ref 9 "micro-kernel.c" >}}) used in the [BLIS
framework](https://github.com/flame/blis/).

Rather than doing this on Hamilton, since we're not actually going to
run the code, we will use the [Compiler
explorer](https://gcc.godbolt.org) which is an online frontend to
trying out lots of different compilers. I've set up a [pre-filled
version](https://gcc.godbolt.org/z/aTnfZE).

## Does vectorisation occur

Use `-xBROADWELL -O2` as the optimsation flags. What does the
vectorisation report say? Which loop was vectorised?

The default blocking factors (`MR` and `NR`) are both 1. Given that
AVX registers can hold four doubles, it makes sense to use larger
blocks.

{{< exercise >}}

Try with some larger blocks and `-O3` instead of `-O2`

{{< /exercise >}}

{{< question >}}

Which loop, if any, was vectorised this time?

{{< /question >}}

## Vectorising the correct loops

Since the `i` loop is stride-1, it really makes sense to vectorise the
innermost loop. Try and convince the compiler to do so.

{{< question >}}

Compare the estimated speedup the compiler reports for the original
loop vectorisation choice and the new one.

What do you observe?

{{< /question >}}

## Providing more detailed information

Part of the problem is that the compiler assumes that the array
accesses are not aligned to cache line boundaries (or vector
registers). Its cost model knows that these are more expensive than if
the data are aligned. However, this drives some bad decisions. We can
help the compiler by letting it know the byte alignment of the arrays.
Use [`__assume_aligned(PTR,
64)`](https://software.intel.com/en-us/articles/data-alignment-to-assist-vectorization)
to promise to the compiler that the given pointer is 64-byte aligned.

{{< question >}}

Try adding alignment assumptions before the start of the loop nest.

What happens to the estimated speedup?

{{< /question >}}

## Trying to maximise throughput

One thing you may have noticed is that the compiler performs a _loop
interchange_. We have a triply nested loop

```c
for (l = 0; l < ...; l++) /* loop 1 */
  for (j = 0; j < ...; j++) /* loop 2 */
    for (i = 0; i < ...; i++) /* loop 3 */
```

But when I run things, I see that the compiler reports it reordered
the loops from order `(1 2 3)` to `(2 1 3)`. It then attempts to
vectorise the innermost loop and unroll the middle loop. This loop is
now not fixed length.

Again, this is an example where the compiler's cost model is
incorrect. We can help it out again by telling it to unroll the `j`
loop with `#pragma unroll`.

We now need to pick good values for the `NR` and `MR` parameters.
Broadwell chips have 16 AVX registers, which is enough to store 64
double precision numbers. Given this information, what do you think
the best blocking factors might be. Note that since `B[j]` is not
stride-1 in the innermost loop, to vectorise the FMA, we need to
replicate the value of `B[j]` over all lanes of the vector register.
That is, inside the `i` loop, we need as many registers for `B` as the
unroll factor of the `j` loop.

{{< question >}}

What's the maximum estimated speedup you can achieve?

{{< /question >}}

## Putting it together

The subdirectory `code/exercise09/blis-gemm/` contains a complete
implementation of this scheme (I don't have optimal parameters
though). You can edit the `parameters.h` file to set the blocking
parameters `MR` and `NR`, you'll also need to edit `micro-kernel.c` to
annotate the loops with the pragmas you found to be useful on the
compiler explorer.

{{< exercise >}}

Set all blocking five blocking parameters in `parameters.h` to 1, and
compile the code. Keep track of the values for `MC`, `KC`, and `NC`
since we'll use them later.

Run for a range of matrix sizes between 100 and 2000. What performance
do you observe?

{{< /exercise >}}

{{< exercise >}}

Now restore the values for `MC`, `KC`, and `NC`, and use your good
parameters for `MR` and `NR`. Recompile and rerun the benchmarking.
What performance do you observe now?

{{< /exercise >}}

{{< exercise >}}
In addition to having good parameters, now add the pragmas to the
micro kernel in `micro-kernel.c` and rerun the same experiments again.
What performance do you observe now?

Last time I tried this, I found that I needed to stop the
`micro_kernel` being inlined by the Intel compiler for really good
performance, so change the signature to be

```c
__attribute__((noinline))
static void micro_kernel(...)
```

Rerun again, do you observe any further differences?

How close to peak performance does the code get?
{{< /exercise >}}
