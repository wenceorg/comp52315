---
title: "Sum reductions"
weight: 1
description: "Benchmarking"
katex: true
---

# Benchmarking with `likwid-bench`

## Overview

We're going to look at the throughput of a very simple piece of code

```c
float reduce(int N, const double *restrict a)
{
  float c = 0;
  for (int i = 0; i < N; i++)
    c += a[i];
  return c;
}
```

when all of the data live in L1 cache.

We'll do so on an AVX-capable core (where the single-precision vector
width is 8).

There is a loop-carried dependency on the summation variable, so
without [unrolling](TODO LINK LATER), the execution stalls at every
add until the previous one completes.

Assembly pseudo-code looks something like

{{% columns %}}

#### Scalar code

```
LOAD r1.0 ← 0
i ← 0
loop:
  LOAD r2.0 ← a[i]
  ADD r1.0 ← r1.0 + r2.0
  i ← i + 1
  if i < N: loop
result ← r1.0
```

<--->

#### Vector code

```
LOAD [r1.0, ..., r1.7] ← 0
i ← 0
loop:
  LOAD [r2.0, ..., r2.7] ← [a[i], ..., a[i+7]]
  ADD r1 ← r1 + r2 ; SIMD ADD
  i ← i + 8
  if i < N: loop
result ← r1.0 + r1.1 + ... + r1.7
```

{{% /columns %}}

Reading the Intel documentation, `ADD` instructions have a latency of
one cycle. So we expect the scalar code to run at \\(\frac{1}{8}\\)th
of the `ADD` peak.

The goal of this exercise is to verify this claim.

In addition, it will give you an opportunity to gain familiarity with
some of the [likwid](https://github.com/RRZE-HPC/likwid/wiki) tools
that will reappear throughout the course.

## Setup: logging in to Hamilton

Likwid is installed on the [Hamilton
cluster](https://www.dur.ac.uk/cis/local/hpc/). Log in with

```
ssh yourusername@hamilton.dur.ac.uk
```

{{< hint info >}}
See the [tips & tricks]({{< ref configuration.md >}}) advice for
simplifying this process.
{{< /hint >}}

### Using the batch system

Hamilton is typical of a supercomputing cluster, in that the login (or
"frontend") node is shared between all users, and you use a batch
system to request resources and execute your code on the compute
nodes.

**DO NOT** do benchmarking on the frontend (login) nodes for two reasons
1. We do not want to overload them
2. Because other people are also using them, any results you get are
potentially highly variable (depending on what else is running at the
time).

Instead you either need to run a batch script that executes your
commands, or else request an interactive terminal on a compute node.
For details see the quickstart guide on DUO (as part of Core I), and
the [Hamilton
documentation](https://www.dur.ac.uk/cis/local/hpc/hamilton/slurm/).

To request an interactive node, run
```
srun --pty $SHELL
```

Note that Hamilton has a limited number of interactive compute nodes.
If too many people are using them, this command will wait until one
becomes available. You can cancel the command with
<kbd>control-c</kbd>. The alternative to using an interactive job is
to use a batch job, where you write a shell script that runs the
commands and submit it with `sbatch`. The Hamilton documentation has
some [template scripts you can
copy](https://www.dur.ac.uk/cis/local/hpc/hamilton/slurm-v2/templates/),
the serial script is probably a good start.

Hamilton uses the
[module](https://www.dur.ac.uk/cis/local/hpc/hamilton/modules/) system
to provide access to software. To gain access to the [likwid tools](https://hpc.fau.de/research/tools/likwid/), we
need to run
```
module load likwid/5.0.1
```

{{< hint info >}}
If you run in batch mode, you should execute this command as part of
the batch script. In interactive mode, you need to load the module
each time you request a new interactive job.
{{< /hint >}}


## Using `likwid-bench`

Verify that you have correctly loaded the likwid module by running
`likwid-bench -h`. You should see output like the following

```
Threaded Memory Hierarchy Benchmark --  Version  5.0


Supported Options:
-h       Help message
-a       List available benchmarks
-d       Delimiter used for physical core list (default ,)
-p       List available thread domains
         or the physical ids of the cores selected by the -c expression
-s <TIME>    Seconds to run the test minimally (default 1)
         If resulting iteration count is below 10, it is normalized to 10.
-i <ITERS>   Specify the number of iterations per thread manually.
-l <TEST>    list properties of benchmark
-t <TEST>    type of test
-w       <thread_domain>:<size>[:<num_threads>[:<chunk size>:<stride>]-<streamId>:<domain_id>[:<offset>]
-W       <thread_domain>:<size>[:<num_threads>[:<chunk size>:<stride>]]
         <size> in kB, MB or GB (mandatory)
For dynamically loaded benchmarks
-f <PATH>    Specify a folder for the temporary files. default: /tmp

Difference between -w and -W :
-w allocates the streams in the thread_domain with one thread and support placement of streams
-W allocates the streams chunk-wise by each thread in the thread_domain

Usage:
# Run the store benchmark on all CPUs of the system with a vector size of 1 GB
likwid-bench -t store -w S0:1GB
# Run the copy benchmark on one CPU at CPU socket 0 with a vector size of 100kB
likwid-bench -t copy -w S0:100kB:1
# Run the copy benchmark on one CPU at CPU socket 0 with a vector size of 100MB but place one stream on CPU socket 1
likwid-bench -t copy -w S0:100MB:1-0:S0,1:S1
```

### An example benchmark

`likwid-bench` has [detailed
documentation](https://github.com/RRZE-HPC/likwid/wiki/Likwid-Bench)
but for this exercise we just need a little bit of information.

We are going to run the `sum_sp` and
`sum_sp_avx` benchmarks. The former runs the scalar
single-precision sum reduction from the lecture, the latter runs the
SIMD sum reduction. We just need to choose the correct setting for the
`-w` argument.

Recall that our goal is to measure
single-thread performance of in-cache operations. We therefore need a
small vector size, 16kB suffices, and want to request just a single
thread. We therefore want `-w -S0:16kB:1`.

This requests a benchmark running on socket 0, for a vector of length
16kB (4000 single-precision entries), and one thread.

{{< hint info >}}
Later we will see how I determined that 16kB was an appropriate size
(and that socket 0 was ok).
{{< /hint >}}

Running the command `likwid-bench -t sum_sp -w S0:16kB:1` you should
see output like the following

```txt {linenos=false,hl_lines=[21]}
Allocate: Process running on core 0 (Domain S0) - Vector length 16000 Offset 0 Alignment 1024
--------------------------------------------------------------------------------
LIKWID MICRO BENCHMARK
Test: sum_sp
--------------------------------------------------------------------------------
Using 1 work groups
Using 1 threads
--------------------------------------------------------------------------------
Group: 0 Thread 0 Global Thread 0 running on core 0 - Vector length 4000 Offset 0
--------------------------------------------------------------------------------
Cycles:                 3187585613
CPU Clock:              2199965642
Cycle Clock:            2199965642
Time:                   1.448925e+00 sec
Iterations:             1048576
Iterations per thread:  1048576
Inner loop executions:  1000
Size:                   16000
Size per thread:        16000
Number of Flops:        4194304000
MFlops/s:               2894.77
Data volume (Byte):     16777216000
MByte/s:                11579.08
Cycles per update:      0.759980
Cycles per cacheline:   12.159674
Loads per update:       1
Stores per update:      0
Instructions:           7340032020
UOPs:                   10485760000
--------------------------------------------------------------------------------
```

We're interested in the highlighted MFlops/s line. We can see that this benchmark
runs at 2894MFlops/s. The CPU on Hamilton compute nodes is an [Intel
Broadwell E5-2650 v4](https://ark.intel.com/content/www/us/en/ark/products/91767/intel-xeon-processor-e5-2650-v4-30m-cache-2-20-ghz.html),
which Intel claim has a max clock frequency of 2.9GHz. So we can see
that this benchmark runs at almost exactly 1 scalar ADD per cycle.

{{< exercise >}}
Replicate the scalar sum reduction benchmark for
yourself, to check you get similar results. Compare the results to the
floating point performance obtained when you run the `sum_sp_avx`
benchmark instead of the `sum_sp` benchmark.
{{< /exercise >}}

{{< question >}}
What floating point throughput do you observe for the SIMD
(`sum_sp_avx`) case?
{{< /question >}}

<div id="vector-size"></div>
{{< exercise >}}

1. Study what happens to the performance (for both the
   `sum_sp` and `sum_sp_avx` benchmarks) when
   you vary the size of the vector from 1kB up to 128MB.
2. Produce a plot of performance with MFlops/s on the y axis and
   vector size on the x axis comparing the `sum_sp` and
   `sum_sp_avx` benchmarks.
{{< /exercise >}}

Rather than copying and pasting the output from every run into a data
file, you might find it useful to script the individual benchmarking
runs. For example, to extract the MFlops/s from the
`likwid-bench` output you can use

```
likwid-bench -t sum_sp -w S0:16kB:1 | grep MFlops/s | cut -f 3
```

{{< hint info >}}
#### Scripting the data collection
For different vector sizes, you'll need to change the size,
which can be done with a loop, for example to measure the
performance with vector sizes of 1, 2, and 4kB:

```bash
for size in 1kB 2kB 4kB; do
  echo $size $(likwid-bench -t sum_sp -w S0:$size:1 | grep MFlops/s | cut -f 3)
done
```
{{< /hint >}}

{{< question >}}
What do you observe about the performance of the scalar code and the
SIMD code?
{{< /question >}}
