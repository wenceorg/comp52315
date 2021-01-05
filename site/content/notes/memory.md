---
title: "The memory hierarchy"
weight: 2
katex: true
---

# An overview of memory hierarchies

## Reduction benchmark

In [exercise 1]({{< ref "exercise01.md" >}}) you looked at the
performance of a vectorised and non-vectorised version of a very
simple loop computing the sum of an array of floating point numbers.

In doing so, you produced a plot of the performance (in terms of
floating point throughput) as a function of array size. You should
have observed something similar to that shown here.

FIXME: add figure

We see that the <abbr title="Single Instruction Multiple
Data">SIMD</abbr> (vectorised) code has four distinct performance
plateaus as a function of the array size, whereas the scalar code has
only two. 

On this hardware (Broadwell), the chip can issue up to one `ADD`
(scalar or vector) per cycle. The peak clock speed is 2.9GHz. So the
peak scalar throughput of addition is 2.9GFlops/s, while the peak
vector throughput is \\(2.9 \times 8 = 23.2\\)GFlops/s.

We can see that the vector code achieves peak throughput for small
vectors, but not large ones. Why is this?

Remember that as well as thinking about the [primary resource]({{< ref
"introduction.md#resource-bottleneck-instruction-throughput" >}}) of
instruction throughput, we also need to consider whether [data
transfers]({{< ref
"introduction.md#resource-bottleneck-data-transfers" >}}) are
producing the bottleneck. For this, we need to consider the memory
hierarchy.

## Memory hierarchy

In the von Neumann model, program code and data must be transferred
from memory to the CPU (and back again). To speed up computation we can
increase the speed at which instructions execute. We can also reduce
the time it takes to _move_ data between the memory and the CPU.

In an ideal world, to process lots of data very fast, we would have
_large_ (in terms of storage) and _fast_ (in terms of transfer speed)
memory. Unfortunately, physics gets in the way, and we can pick one of

1. _small_ and _fast_
2. _large_ and _slow_

In fact, there is a sliding scale here, as we make the storage
capacity smaller we can make the memory faster, and vice versa.

We have something close to the following picture

{{< manfig src="cachesketch.png"
    width="70%"
    caption="As memory gets larger, it must become slower, both in latency and bandwidth" >}}

To explore these latencies in more depth (and see how they've changed
over time), see [latency numbers every programmer should
know](https://colin-scott.github.io/personal_website/research/interactive_latency.html).

## Caches

Having identified the high level problem that we can't make large,
fast memory, what can chip designers do about it? The answer (on CPUs
at least) is _caches_. 

The idea is that we add a hierarchy of small, fast memory. These keep
a copy of _frequently used_ data and are used to speed up access.
Except in certain special cases, it's not possible to know _which_
data will be used frequently. As a consequence, caches rely on a
_principle of locality_.

FIXME: add details on caches

## Measurement

As well as using
[likwid-bench](https://github.com/RRZE-HPC/likwid/wiki/Likwid-Bench)
to measure floating point throughput of some simple loops, we can also
use it to measure memory bandwidth. In [exercise 2]({{< ref
"exercise02.md" >}}) you should do this to determine the cache
and main memory bandwidth on the Hamilton cores. We will use this to
construct a predictive model of the floating point throughput of the
reduction from [exercise 1]({{< ref "exercise01.md" >}}).

FIXME: add results

## A predictive model for reductions

Let us remind ourselves of the code we want to predict the performance
of

{{% columns %}}

#### C code
```c
float reduce(int N, 
    const double *restrict a)
{
  float c = 0;
  for (int i = 0; i < N; i++)
    c += a[i];
  return c;
}
```

<--->

#### Vectorised pseudo-assembly

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

The accumulation parameter `c` is held in a register. At each
iteration of the vectorised loop, we load eight elements of `a` into a
recond register. Since each `float` value takes 4 bytes, this means
that each iteration of the loop requires 32 bytes of data.

Recall that we can run one `ADD` per cycle. To keep up with the
addition, the memory movement must therefore deliver 32 bytes/cycle.

At 2.9GHz, this translates to a sustained load bandwidth of

$$
32 \text{bytes/cycle} \times 2.9 \times 10^9 \text{cycle/s} = 92.8\text{Gbyte/s}.
$$

Let's match this up with our measurements.

### L1 bandwidth

The smallest (and fastest) cache is the level one (or L1) cache. On
this hardware, we observe a sustained load bandwidth of around
300Gbyte/s. Hence, when the data fit in L1 (less than 32KB), the speed
of executing the `ADD` instruction is the limit.

### L2 bandwidth

The next level of cache is level two (L2). This provides around
80Gbyte/s or 27bytes/cycle. Since \\( 27 < 32 \\), we can't reach the
floating point peak when the data fit in L2. The best we can hope for
is

$$
2.9 \times 8 \times \frac{27}{32} = 19.6\text{GFlops/s}.
$$

### L3 and main memory bandwidth

We apply the same idea to the level three (L3) cache and main memory.
L3 provides around 36Gbyte/s or 12bytes/cycle. We obtain an upper
limit of

$$
2.9 \times 8 \times \frac{12}{32} = 8.7\text{GFlops/s}
$$

For main memory, the memory bandwidth is around 13Gbyte/s or
4.5bytes/cycle, and the peak is approximately 3.25GFlops/s.


Let's redraw our floating point throughput graph, this time annotating
it with these predicted performance limits.

We can see that this simple model does a pretty good job of predicting
the performance of our test code.

## Stepping back

This idea of predicting performance based on resource limits is a
powerful one, and we will return to it through the rest of the course.

As a practice, see if you can come up with throughput limits for the
following piece of code

```c
void stream_triad(int N, float * restrict a,
                  const float * restrict b,
                  const float * restrict c,
                  const float alpha)
{
  for (int i = 0; i < N; i++)
    a[i] = b[i]*alpha + c[i];
}
```

The Broadwell chips on Hamilton can execute up to two loads and one
store per cycle. To determine the floating point limit (assuming no
memory constraints) note that this operation perfectly matches a
"fused multiply add"

```
a_i ← b_i * alpha + c_i
```

Which is implemented as a single instruction `FMA`. Broadwell chips
can execute up to two `FMA` instructions per cycle.

We will revisit this in a [later exercise]({{< ref "exercise05.md" >}}).

## Scalable and saturating resources

Although most of the focus in this course is on single core
performance, it is worthwhile taking a little time to consider how
resource use changes when we involve more cores. All modern chips have
more than one core on them. Some of the resources on a chip are
therefore private to each core, and some are shared. In particular,
the floating point units are private: adding more cores increasing the
total floating point throughput. In contrast, the main memory is
shared between cores, so adding more cores _does not_ increase the
memory bandwidth.

We can ask likwid, using
[`likwid-topology`](https://github.com/RRZE-HPC/likwid/wiki/likwid-topology)
to provide us some information on the layout of the system we are
running on. It produces a schematic of the core and memory layout in
ASCII, similar to the diagram below

{{< manfig src="cacheschematic.png"
    width="70%"
    caption="Example layout of caches and memory for a 4 core system."
    >}}
    
Although in this course we will spend most of our time focussing on
_single core_ performance, in practice, most scientific computing
algorithms will be parallel.

To understand how parallelisation will affect the performance on real
hardware, we need to know if will be limited by a resource which is
scalable, or saturating.

{{% columns %}}
### Scalable resources

These resources are private to each core/chip. For example, CPU cores
themselves are a _scalable_ resource. Adding a second core doubles the
number of floating point operations we can perform.

As a consequence, if our code is limited by the floating point
throughput, adding more cores is a useful thing to do.

{{< autofig src="scalable-resource.png"
    width="100%"
    caption="Prototypical performance of a scalable resource" >}}

<--->

### Saturating/shared resources

These resources are shared between cores. The typical example is main
memory bandwidth. In the diagram above, we see that the main memory
interface is shared between the four cores. This is typical for modern
CPUs.

On a single chip, if our code is limited by the main memory bandwidth,
adding more cores is _not_ useful. Instead we would need to add
another chip (with another memory system).

{{< autofig src="saturating-resource.png"
    width="100%"
    caption="Prototypical performance of a saturating resource" >}}

{{% /columns %}}

You should explore this on Hamilton in [exercise 3]({{< ref
"exercise03" >}})

## Summary: challenges for writing high performance code

At a high level, the performance of an algorithm is dependent on:

1. how many instructions are required to implement the algorithm;
1. how efficiently those instructions can be executed on a processor;
1. and what the runtime contribution of the required data transfers
   is.
   
Given an optimal _algorithm_, converting that to an optimal
_implementation_ requires addressing all of these points in tandem.
This is made complicated by the complexity and parallelism of modern
hardware. A typical Intel server offers

1. Socket-based parallelism: 1-4 CPUs on a typical motherboard;
1. Core-based parallelism: 4-32 cores in a typical CPU;
1. SIMD/Vectorisation: vector registers capable of holding 2-16 single;
   precision elements on each core;
1. Superscalar execution: typically 2-8 instructions per cycle per core.

To limit the scope to something reasonable, we will focus mainly on
the SIMD and superscalar parts of this picture. The rationale for this
is that we should aim for good single core performance _before_
looking at parallelism. If we don't, we might be led in the wrong
direction by a "false" idea of the performance limits.
