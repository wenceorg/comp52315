---
title: "Models and measurements"
weight: 5
---

# Verifying a model with measurements

The goal of this exercise is to verify our model for the number of
loads and stores in a stream benchmark using performance counters,
accessed via
[`likwid-perfctr`](https://github.com/RRZE-HPC/likwid/wiki/likwid-perfctr).

## Background

I provide [an implementation]({{< code-ref 5 "stream.c" >}}) (in
`code/exercise05/stream.c`) written in C
of the [STREAM TRIAD](https://www.cs.virginia.edu/stream/) benchmark.
It provides scalar, SSE, and AVX implementations of the loop

```c {linenos=false}
double *a, *b, *c;
...
for (i = 0; i &lt; N; i++) {
  c[i] = c[i] + a[i] * b[i];
}
```

We will measure the number of loads and stores for this loop using
`likwid-perfctr`. So our first task is to compile the code
appropriately.

## Compiling with likwid annotations enabled

The code is annotated with [likwid-specific
markers](https://github.com/RRZE-HPC/likwid/wiki/likwid-perfctr#using-the-marker-api)
around the relevant loops. This way we can ensure that we're only
measuring counters for the bit of the code we're interested in. We
therefore need to enable these when compiling. As before we load the
likwid module with `module load likwid/5.0.1`.

We can safely compile this code with GCC, load the GCC module with
`module load gcc/9.3.0` and run:

```
gcc -std=c99 -mfma -O1 -DLIKWID_PERFMON -fno-inline -march=native -o stream stream.c -llikwid
```

The flag `-DLIKWID_PERFMON` adds a new symbol in the
preprocessor which will turn on the likwid markers. We then also need
to link against the likwid runtime library with `-llikwid`.
To ensure that this library is available when you run the code, you
should load the likwid module on the compute node (or run `module
load likwid/5.0.1` in your batch submission script).

For this exercise, `likwid-perfctr` does not give us an appropriate
predefined group. Instead, we must use a low-level counter directly.
For loads the relevant counter is `MEM_UOPS_RETIRED_LOADS_ALL`, for
stores it is `MEM_UOPS_RETIRED_STORES_ALL`. We must specify a [_group
string_](https://github.com/RRZE-HPC/likwid/wiki/likwid-perfctr#using-custom-event-sets)
which consists of the name of the counter and the register to save it
in. For memory operations we can use the registers `PMC0` and `PMC1`
(possibly others, but these will suffice).

So, a complete command line to measure the number of loads is
```
likwid-perfctr -m -g "MEM_UOPS_RETIRED_LOADS_ALL:PMC0" -C 0 ./stream 1000000 sca
```

This says to enable the marker API (with `-m`), to count
the `MEM_UOPS_RETIRED_LOADS_ALL` event in
`PMC0`, and to pin the executable to core zero (with
`-C 0`). This final part is necessary so that the operating
system does not move the executable half way through, breaking our
measurements. You should see output similar to the following

```
--------------------------------------------------------------------------------
CPU name: Intel(R) Xeon(R) CPU E5-2650 v4 @ 2.20GHz CPU type: Intel
Xeon Broadwell EN/EP/EX processor CPU clock: 2.20 GHz
--------------------------------------------------------------------------------
Sleeping longer as likwid_sleep() called without prior initialization
sca loop, sum 3.33328e+17
--------------------------------------------------------------------------------
Region Scalar, Group 1: Custom
+-------------------+----------+
|    Region Info    |  Core 0  |
+-------------------+----------+
| RDTSC Runtime [s] | 0.001473 |
|     call count    |        1 |
+-------------------+----------+

+----------------------------+---------+--------------+
|            Event           | Counter |    Core 0    |
+----------------------------+---------+--------------+
|     Runtime (RDTSC) [s]    |   TSC   | 1.472779e-03 |
| MEM_UOPS_RETIRED_LOADS_ALL |   PMC0  |      3001190 |
|      INSTR_RETIRED_ANY     |  FIXC0  |      4629691 |
|    CPU_CLK_UNHALTED_CORE   |  FIXC1  |      3247484 |
|    CPU_CLK_UNHALTED_REF    |  FIXC2  |      3247574 |
+----------------------------+---------+--------------+
```

This measurement at least, aligns with what we expected, since we see
about 300000 loads.

{{< question >}}
1. How does the number of loads change if you use the SSE, AVX, or FMA
   versions of the code?
2. How many stores do you measure?
3. Can you find a way of measuring the stores and loads in one go?
{{< /question >}}

{{< question >}}
Try compiling the code with:

```
gcc -std=c99 -mfma -O1 -DLIKWID_PERFMON -fno-inline -o stream stream.c -llikwid
```

Do you still measure the same instruction counts for all cases?

To understand what is going on, we can inspect the assembly code for
these two cases by getting the compiler to spit that out instead:

```
gcc -std=c99 -mfma -O1 -DLIKWID_PERFMON -fno-inline -S -o stream-no-march.s stream.c -llikwid

gcc -std=c99 -mfma -O1 -DLIKWID_PERFMON -fno-inline -march=native -S -o stream-march-native.s stream.c -llikwid
```

Compare the loop bodies for the functions, do you observe any
differences in the assembly that might explain things?
{{< /question >}}
