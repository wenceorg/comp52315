---
title: "Roofline analysis"
weight: 4
katex: true
---

# A roofline analysis of matrix-vector multiplication

The goal of this exercise is to perform a roofline analysis of
matrix-vector multiplication. We will look at the effect that compiler
flags have on the performance of generated code. Next, we will see if
the performance that we obtain is independent of the problem size.
Finally, we will investigate loop blocking.

## Background

I provide [an implementation]({{< code-ref 4 "dmvm.c" >}})[^1] in `code/exercise04/dmvm.c`, in C, of double-precision
matrix-vector multiplication, which computes

$$
\vec{y} = A \vec{x}
$$

for a \\(n_\text{row} \times n_\text{col}\\)
rectangular matrix. It takes two command-line arguments, the number of
rows and the number of columns. When run, it creates the matrices,
performs the computation and prints out information about the
performance. For example, after compiling and running with
`./dmvm 1000 2000` you might see output similar to
`1019 1000 2000 6491.23`. The four columns are:
1. The number of iterations the test was run for;
1. The number of rows the matrix had;
1. The number of columns the matrix had;
1. The performance in MFLOPs/s.

[^1]: This code is taken from
      [examples](https://github.com/RRZE-HPC/Code-teaching) developed
      at [RRZE](https://www.rrze.fau.de/)

{{< hint info >}}
Instead of downloading these code individually you should [clone the
repository]({{< repo >}}) on Hamilton and work in the appropriate
`code/exerciseXX` subdirectory.
{{< /hint >}}

## Setup

As usual, you should do this on [Hamilton]({{< ref
"exercise01.md#setup-logging-in-to-hamilton" >}}).

## Downloading and compiling the code

We'll use the Intel C compiler for this exercise, which requires us to
load some extra modules. To get the most recent version, run `module
load intel/2019.5` and `module load gcc/8.2.0`. In addition, we will want
to make performance measurements with likwid, so also `module load
likwid/5.0.1`.

You can use `wget` to download the code when logged in.

{{< hint warning >}}
Note: `wget` does not work on the compute nodes, so you should
download any material on the login (frontend) node.
{{< /hint >}}

### Compilation without any optimisation

First, we will compile without any optimisations.

{{< hint warning >}}
Normally you should not do this, but we want to see what the effect
is!
{{< /hint >}}

```
icc -std=c11 -O0 -o dmvm dmvm.c
```

You should now be able to run the `dmvm` binary with `./dmvm 4000
4000` for a \\(4000 \times 4000\\) matrix.

## Obtain the machine and code characteristics

To perform a roofline analysis, we need the machine and code
characteristics.

{{< exercise >}}
 Measure the main memory streaming memory bandwidth using
the `triad` benchmark
of `likwid-bench`.
{{< /exercise >}}

{{< exercise >}}
Compute the peak floating point throughput of a compute node. A single
core on the Hamilton nodes runs at 2.9GHz and can issues two double
precision FMAs (fused multiply-add instructions) per cycle.
{{< /exercise >}}

{{< exercise >}}
Read the code, in particular the `dmvm` function and
determine the best case arithmetic intensity by counting data accesses
(using the perfect cache model) and floating point operations.
{{< /exercise >}}

## Produce a roofline plot of `-O0` compiled code

{{< exercise >}}

Using a fixed number of columns \\(n_\text{col} = 10000\\), measure the performance of your `dmvm` binary over a range
of row sizes from 1000 to 100000. Plot the resulting data using a
roofline plot.

{{< /exercise >}}

{{< question >}}
What do you think your next step in optimising the code
should be?
{{< /question >}}

## Better compiler options

We'll stop crippling the compiler. Try these sets of compiler options:
1. `icc -std=c11 -O1 -o dmvm dmvm.c`
1. `icc -std=c11 -O2 -o dmvm dmvm.c`
1. `icc -std=c11 -O3 -o dmvm dmvm.c`
1. `icc -std=c11 -xCORE_AVX2 -unroll16 -mfma -O3 -o dmvm dmvm.c`

{{< exercise >}}
Add the results you get from these runs to your roofline
plots. What do you see? 
{{< /exercise >}}

{{< question >}}
Is performance for any of these
options independent of the problem size? What do you think the
bottleneck for this algorithm is?
{{< /question >}}

## Loop blocking for out-of-cache performance

You should have observed that when the number of rows gets too large,
the performance of the code drops by almost a factor of two. When the
number of rows is very large, some of the vector data (which is
accessed more than once) no longer fits in cache.

{{< exercise >}}

Use the pessimal cache model to obtain a worst case arithmetic
intensity and add these new data points to your roofline plot.

{{< /exercise >}}

A mechanism to solve this problem is to traverse the data in an order
that is aware of the cache. For loop-based code, this is called _loop
blocking_ or _tiling_. I provide an implementation of [a simple
scheme]({{< code-ref 4 "dmvm-blocked.c" >}}) in
`code/exercise04/dmvm-blocked.c`. Compile it, using the set of
compiler options that worked best for the original code. Use `-o
dmvm-blocked` to produce a new binary.

{{< exercise >}}
As before, using a fixed number of columns \\(n_\text{col} =
10000\\), measure the performance of your
`dmvm-blocked` binary over a range of row sizes from 1000
to 100000. You should observe that the performance is now largely
insensitive to the number of rows. Plot the resulting data using a
roofline plot. 
{{< /exercise >}}

{{< question >}}
What do you think your next step (if any) in optimising
the code should be?
{{< /question >}}

