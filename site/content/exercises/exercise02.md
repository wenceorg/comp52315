---
title: "Caches"
weight: 2
---

# Measuring memory bandwidth in the memory hierarchy

The goal is to determine the memory bandwidth as a function of how
much data we are moving on the Hamilton cores.

Again, as for [the first exercise]({{< ref "exercise01" >}}) we will
do this with `likwid-bench`. This time, however, we will use three
different benchmarks

1. `clcopy`: Double-precision cache line copy, only touches first
   element of each cache line.
2. `clload`: Double-precision cache line load, only loads first
   element of each cache line.
3. `clstore`: Double-precision cache line store, only stores first
   element of each cache line.

These benchmarks do the minimal amount of work while moving data in
cache lines (64 bytes at a time), and therefore exercise the memory
bandwidth bottlenecks (rather than instruction issue or similar).

## Setup

### Logging in to Hamilton

As before, you should do this on a compute node on Hamilton. See the
[quickstart in exercise 1]({{< ref
"exercise01.md#setup-logging-in-to-hamilton" >}}) if you can't
remember how to do this.

### Running the benchmarks

As mentioned, this time we want to measure memory bandwidth with the
`clcopy`, `clload`, and `clstore`
benchmarks. We're interested in the (highlighted) MByte/s output of
`likwid-bench`. For example running `likwid-bench -t
clcopy -w S0:1kB:1` produces output.

```txt {linenos=false,hl_lines=[24]}
Allocate: Process running on core 0 (Domain S0) - Vector length 496 Offset 0 Alignment 512
Allocate: Process running on core 0 (Domain S0) - Vector length 496 Offset 0 Alignment 512
--------------------------------------------------------------------------------
LIKWID MICRO BENCHMARK
Test: clcopy
--------------------------------------------------------------------------------
Using 1 work groups
Using 1 threads
--------------------------------------------------------------------------------
Group: 0 Thread 0 Global Thread 0 running on core 0 - Vector length 62 Offset 0
--------------------------------------------------------------------------------
Cycles:                 3057482699
CPU Clock:              2199869128
Cycle Clock:            2199869128
Time:                   1.389848e+00 sec
Iterations:             268435456
Iterations per thread:  268435456
Inner loop executions:  2
Size:                   992
Size per thread:        992
Number of Flops:        0
MFlops/s:               0.00
Data volume (Byte):     266287972352
MByte/s:                191595.10
Cycles per update:      0.183710
Cycles per cacheline:   1.469679
Loads per update:       1
Stores per update:      1
Load/store ratio:       1.00
Instructions:           2952790032
UOPs:                   3758096384
--------------------------------------------------------------------------------
```

{{< exercise >}}
Produce a plot of memory bandwidth as a function of the size vector
being streamed from 1kB up to 1GB for each of the three different
benchmarks.

As before, you can script this data collection with a [bash loop]({{<
ref "exercise01.md#scripting-the-data-collection" >}}).
{{< /exercise >}}

{{< question >}}
What do you observe about the available memory bandwidth? Is the
bandwidth the same for 1kB and 1GB vectors?
{{< /question >}}

{{< exercise >}}
Use `likwid-topology` to find out about the
different sizes of cache available on the system. You can find
out how to use it by providing the `-h`
command-line flag. The graphical output is most useful.
{{< /exercise >}}
{{< question >}}
Can you use the output from `likwid-topology` to explain and
understand your memory bandwidth results?
{{< /question >}}
