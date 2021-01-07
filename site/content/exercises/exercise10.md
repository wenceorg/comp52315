---
title: "Stencil layer conditions"
weight: 10
katex: true
---

# Loop tiling for stencil codes

We're going to investigate the _layer condition_ loop tiling
guidance for stencil codes. We'll use as an exemplar the five-point
finite difference stencil for the Laplacian on a [regular grid]({{<
code-ref 10 "fivepoint.c" >}}) in `code/exercise10/fivepoint.c`.

The layer condition model was developed by a group at Erlangen, and
they provide an [interactive
calculator](https://rrze-hpc.github.io/layer-condition/).

As usual, we're going to use the Intel compiler on Hamilton, so after
logging in and downloading the code, load the relevant modules

```
module load gcc/8.2.0
module load intel/2019.5
```

The code can be compiled with `icc -xBROADWELL -O3 -o fivepoint
fivepoint.c`.

## Throughput of untiled loops

The code runs and reports performance in MLUP/s (millions of lattice
updates per second). You can control both the number of rows of the
grid (the innermost loop) and the number of columns. 

For fewer than 10,000 rows, set the number of columns equal to the
number of rows. For more than 10,000 rows, keep the product of rows
and columns approximately constant at \\(10^9\\).

{{< question >}}

What happens to the performance when you vary the number of rows from
1000 up to 100,000,000?

{{< /question >}}

{{< exercise >}}

Produce a plot of MLUP/s against row size. 

{{< /exercise >}}

{{< question >}}

Do you observe any tell-tale drops in performance?

{{< /question >}}

## Throughput of tiled loops

{{< exercise >}}

Repeat the above study, this time with the tiled version. Try
calculating appropriate blocking sizes for both L2 and L3 caches using
the layer condition we discussed in lectures. Add the performance you
achieve with the two different blocking factors to your plot of the
untiled code.

{{< /exercise >}}

{{< question >}}

What do you observe?

{{< /question >}}

## Validation of the data movement model

Load the `likwid/5.0.1` module, and recompile the code with
likwid-perfctr support. `icc -xBROADWELL -O3 -DLIKWID_PERFMON -o
fivepoint fivepoint.c -llikwid`. The update loop is instrumented with
likwid markers. Measure the `MEM_DP` group. 

{{< question >}}

What is the measured number of bytes of data required per LUP? 

Does it match your expectations?

{{< /question >}}

## Vectorisation

Look again at the performance counters. Did the compiler manage to
vectorise this code at all? Ask the intel compiler for an optimisation
report to see if you can work out why it was, or was not, vectorised.
If no vectorisation was enabled, see if you can convince the compiler
to vectorise through judicious use of `#pragma omp simd` annotations.

{{< question >}}
Does managing to enable vectorisation change any of  the results of
the previous sections at all?

{{< /question >}}


