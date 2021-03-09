---
title: "Memory bandwidth"
weight: 3
katex: true
---

# Measuring multi-core memory bandwidth

The goal of this exercise is to measure the memory bandwidth for
various vector sizes as a function of the number of cores used to
process the vector.

Again, we will do this with `likwid-bench`. This time, we will use the
`clload` benchmark.

## Setup

As usual, you should do this on a compute node on Hamilton. See the
[quickstart in exercise 1]({{< ref
"exercise01.md#setup-logging-in-to-hamilton" >}}) if you can't
remember how to do this.

## Compute node topology

{{< exercise >}}
The first thing we need to do is figure out what the _topology_ of the
node we're running on is. We can do that by running `likwid-topology
-g`. We can use this to guide appropriate choices of vectors.
{{< /exercise >}}

{{< question >}}
How many sockets are there on a node on Hamilton?
{{< /question >}}

{{< question >}}
On each socket, how many cores are there?
{{< /question >}}

{{< question >}}
How large are the caches (L1, L2, L3) on each socket? Which levels of
the cache are private, and which are shared?
{{< /question >}}

Having answered these questions, you should be able to pick
appropriate vector sizes to check the parallel memory
bandwidth of L1, L2, and L3 caches, and main memory.

## Interlude: more on `likwid-bench`
### Selecting the number of cores to use

You should have determined that a Hamilton node has two sockets, each
with 12 cores, and that the L1 cache is 32KB and private to each core
(the remaining answers are left blank!). Let's look at how to
benchmark in multi-core mode with `likwid-bench`.

To select the number of cores to allocate to the benchmark we have to
adapt the workgroup string. Previously we just said `-w
S0:size:1` which means

`S0`
: The memory socket on which to _allocate_ the vector.

`size`
: As before, the size of the vector.

`1`
: The _number_ of cores to use.

To change the number of cores, we replace `1` by our choice
(say `2`). The vector size is the global vector size, so if
we run with a vector size $S$ on
$N$ cores, then each core gets
$\frac{S}{N}$ elements.

For example, to run on one socket with 4 cores, such that each core
handles 4KB of data, we run `likwid-bench -t clload -w
S0:16kB:4`, which should produce output like the below

```txt {linenos=false,hl_lines=[26]}
Allocate: Process running on core 0 (Domain S0) - Vector length 16000 Offset 0 Alignment 512
--------------------------------------------------------------------------------
LIKWID MICRO BENCHMARK
Test: clload
--------------------------------------------------------------------------------
Using 1 work groups
Using 4 threads
--------------------------------------------------------------------------------
Group: 0 Thread 0 Global Thread 0 running on core 0 - Vector length 2000 Offset 0
Group: 0 Thread 1 Global Thread 1 running on core 1 - Vector length 2000 Offset 480
Group: 0 Thread 2 Global Thread 2 running on core 2 - Vector length 2000 Offset 960
Group: 0 Thread 3 Global Thread 3 running on core 3 - Vector length 2000 Offset 1440
--------------------------------------------------------------------------------
Cycles:                 1756452960
CPU Clock:              2199997930
Cycle Clock:            2199997930
Time:                   7.983885e-01 sec
Iterations:             134217728
Iterations per thread:  33554432
Inner loop executions:  62
Size:                   16000
Size per thread:        4000
Number of Flops:        0
MFlops/s:               0.00
Data volume (Byte):     536870912000
MByte/s:                672443.23
Cycles per update:      0.026173
Cycles per cacheline:   0.209386
Loads per update:       1
Stores per update:      0
Instructions:           14562623504
UOPs:                   12482248704
--------------------------------------------------------------------------------
```

`likwid-bench` reports which cores it used at the top and then
provides the same information as before. Again, we are interested in
the highlighted memory bandwidth line.

### Benchmarking on multi-socket systems

If the node has more than one socket, we need to make sure that we
allocate the vector on the correct socket. For example, if we have two
12-core sockets and want to benchmark main memory bandwidth on all 24
cores we should write

```
likwid-bench -t clload -w S0:1GB:12 -w S1:1GB:12
```

This tells `likwid-bench` to allocate two vectors each of 1GB, one on
each socket, and to use 12 cores on each socket.


## Measuring the memory bandwidth

{{< exercise >}}
You should now produce plots of memory bandwidth as a function of the
number of cores for data at different levels in the memory hierarchy.

For the private caches (L1, L2), pick a vector size such that the
vector fills about half the cache on each core.

For the L3 cache, pick a vector size to fill around two-thirds of the
cache.

For the main memory, pick a vector size of around 1GB/socket.

You should produce plots of the memory bandwidth as a function of the
number of cores for each of these different vector sizes.

{{< /exercise >}}

{{< question >}}
Do you observe any difference in the _scalability_ of the memory
bandwidth when you change the size of the vectors?

Can you explain what you see based on the notion of shared and
scalable resources?
{{< /question >}}
