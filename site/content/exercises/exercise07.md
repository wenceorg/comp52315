---
title: "Loop tiling matrix transpose"
weight: 7
katex: true
---

# The effect of loop tiling on matrix transpose

In lectures, we saw a model for throughput of a matrix transpose
operation. Here we're going to look at the effect on throughput of
loop tiling. I provide an implementation of matrix transpose
[with]({{< code-ref 7 "transpose-blocked.c" >}}) and [without]({{<
code-ref 7 "transpose.c" >}}) one level of loop tiling.

{{< hint info >}}
As usual, these live in the [repository]({{< repo >}}).
{{< /hint >}}

## Compile the code

We'll use the intel compiler to build this code. So after logging in
to Hamilton and downloading, load the relevant modules

```
module load gcc/8.2.0
module load intel/2019.5
```

The code can be compiled with `icc -O1 -std=c99 -o transpose
transpose.c` for the untiled version and `icc -O1 -std=c99 -o
transpose-blocked transpose-blocked.c`.

## Measure effective bandwidth as a function of matrix size

{{< exercise >}}
For both the blocked and unblocked code, measure the memory bandwidth
as a function of the number of rows and columns (using square matrices
is fine) from around \\(N = 100\\) to \\(N = 20000\\). Try both with \\(N\\)
a power of two, and \\(N\\) a multiple of ten.

{{< /exercise >}}

{{< question >}}
What do you observe comparing the blocked and unblocked performance?
{{< /question >}}

{{< question >}}
Do you notice anything different when using power of two sizes
compared to multiples of ten?
{{< /question >}}

The default blocking size is a \\(64 \times 64\\) tile. You can
override these sizes when compiling with `icc -O1 -std=c99 -o
transpose-blocked transpose-blocked.c -DRSTRIDE=X -DCSTRIDE=Y`, by
setting `X` and `Y` to appropriate numbers.

{{< question >}}

Given that a Hamilton CPU has a 32kB level one cache size. What is a
good tile size if you want to block for level one cache?

{{< /question >}}

{{< question >}}

Do you notice any performance changes if you change the tile size?

{{< /question >}}

## Measuring cache behaviour

The code is annotated with likwid markers (for use with
`likwid-perfctr`). So we can measure the cache behaviour. To do this,
recompile the two executables with `icc -O1 -std=c99 -DLIKWID_PERFMON
-o transpose transpose.c -llikwid` and `icc -O1 -std=c99
-DLIKWID_PERFMON -o transpose-blocked transpose-blocked.c -llikwid`
after loading the `likwid/5.0.1` module.

{{< exercise >}}

For a \\(4096 \times 4096\\) matrix, measure the main memory bandwidth
and data volume for both the blocked and unblocked cases with
`likwid-perfctr -g MEM -C 0 -m ...`.

{{< /exercise >}}

{{< question >}}

What do you observe about the measured data volume (reported by
likwid) compared to the effective data volume? 

What about if you change to a \\(5000 \times 5000\\) matrix? 

Can you explain what you see?
      
{{< /question >}}
