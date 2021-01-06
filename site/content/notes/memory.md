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

{{< autofig
    src="sum-reduction-throughput.svg"
    width="60%"
    caption="Scalar and AVX-enabled throughput of vector dot product as measured with `likwid-bench`." >}}

We see that the <abbr title="Single Instruction Multiple
Data">SIMD</abbr> (vectorised) code has four distinct performance
plateaus as a function of the array size, whereas the scalar code has
only two. 

On this hardware (Broadwell), the chip can issue up to one floating
point `ADD` (scalar or vector) per cycle. The peak clock speed is
2.9GHz. So the peak scalar throughput of addition is 2.9GFlops/s,
while the peak vector throughput is \\(2.9 \times 8 = 23.2\\)GFlops/s.

{{< hint info >}}

How do I know the instruction issue rate? It is advertised in some of
the Intel optimisation manuals. It's also in [Agner
Fog's](https://www.agner.org) reference on [instruction
latency](https://www.agner.org/optimize/instruction_tables.pdf), see
the table for Intel Broadwell, starting on page 232. Alternatively, I
can go to [μops.info](https://uops.info) and look at their interactive
[table](https://uops.info/table.html).

{{< /hint >}}

We can see that the vector code achieves peak throughput for small
vectors, but not large ones. Let us try and understand why.

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

{{< manfig src="cachesketch.svg"
    width="70%"
    caption="As memory gets larger, it must become slower, both in latency and bandwidth" >}}

Typical CPUs have three or more levels of cache that get both larger
and slower as we get closer to main memory (and further in latency
from the CPU). So we'll often refer to L1 or L2 cache (or similar).

{{< hint info >}}
To explore these latencies in more depth (and see how they've changed
over time), see [latency numbers every programmer should
know](https://colin-scott.github.io/personal_website/research/interactive_latency.html).
{{< /hint >}}

## Caches

Having identified the high level problem that we can't make large,
fast memory, what can chip designers do about it? The answer (on CPUs
at least) is _caches_. As usual, wikipedia has a pretty [detailed
description](https://en.wikipedia.org/wiki/CPU_cache), we'll cover the
main points here.

The idea is that we add a hierarchy of small, fast memory. These keep
a copy of _frequently used_ data and are used to speed up access.
Except in certain special cases, it's not possible to know _which_
data will be used frequently. As a consequence, caches rely on a
_principle of locality_.

As an analogy, consider implementing an algorithm from a textbook. You have
the textbook on the shelf, but the first time you go to find something
you need to get the book, search its index, flip to the correct page
and start reading. Now, you think you understand, so you put the
textbook away again on the shelf. Then you carry on with your
implementation and realise you need to refer to the next page in the
book, so you go back to the shelf, grab the book, flip to the correct
page and reacquaint yourself with notation and continue working. This
is slow. A more efficient thing to do would be to leave the textbook
on your desk until you're definitely finished with it (or your desk
becomes full and you need space for another one). In analogy with
memory accesses, this is exactly what caches enable: fast lookup to
frequently used information.

### Principle of locality

It is normally not possible to decide before execution exactly which
data will be needed frequently (and is therefore suitable for
caching). In practice, most programs either do (or could) exhibit
_locality_ of data access. If we want our code to run fast, then we
need to restructure any algorithm to make best use of this locality.

There are two types of locality that caches exploit

#### Temporal locality

This can be summarised pithily as:

{{< hint info >}}

If I access data at some memory address, it is likely that I will do
so again "soon".

{{< /hint >}}


The idea is that the first time we access an address, it is loaded
from main memory _and_ stored in the cache. We pay a small penalty
for the first load (because the store to cache is not completely
free), but subsequent accesses to that address use the copy in the
cache and are much faster.

As an example, consider a simple loop

```c
float s[16] = {0};
for (int i = 0; i < N; i++) {
  s[i%16] += a[i];
}
```

where $N \gg 16$. In this case, each entry in `s` will be accessed
multiple times, exhibiting _temporal locality_, and it makes sense to
keep all of `s` in cache.

#### Spatial locality

Pithily summarised as

{{< hint info >}}
If I access data at some memory address, it is likely that I will
access neighbouring memory addresses.
{{< /hint >}}

Again, when accessing an address for the first time, we load it from
main memory _and_ store it in the cache. Additionally, we guess (or
hope) that neighbouring addresses will also be used. So if we loaded
an address `a`, we also load and store the data at addresses `a+1`,
`a+2` (say). We pay a penalty for the first load (because we're moving
more data), but hope that the next load is for `a+1` which will be
fast.

Consider the same loop again.
```c
float s[16] = {0};
for (int i = 0; i < N; i++) {
  s[i%16] += a[i];
}
```

Access to `a` exhibits spatial locality. It makes sense when loading
`a[i]` to also load `a[i+1]` since it will be used in the next
iteration.


### High-level design of caches

To understand how to write software that deals with caches
efficiently, it is helpful to understand a little of how they work in
hardware.

Each piece of data in our program is uniquely identifiable by its
_address_ in memory. This is a 32 or (these days usually) 64bit
value which refers to a single byte in memory.

Let's look at this. In C we can use the `%p` format specifier to print
the address of a pointer in hexadecimal. We can also, with a bit of
trickery, print it in binary.

{{< code-include "snippets/print-address.c" "c" >}}

Compile and run it with

```
$ cc -o print-address print-address.c
$ ./print-address
Address of a    is: 0x7ffee015022c 00000000_00000000_01111111_11111110_11100000_00010101_00000010_00101100

Address of b[0] is: 0x7fa6e0405820 00000000_00000000_01111111_10100110_11100000_01000000_01011000_00100000
Address of b[1] is: 0x7fa6e0405828 00000000_00000000_01111111_10100110_11100000_01000000_01011000_00101000
Address of b[2] is: 0x7fa6e0405830 00000000_00000000_01111111_10100110_11100000_01000000_01011000_00110000
Address of b[3] is: 0x7fa6e0405838 00000000_00000000_01111111_10100110_11100000_01000000_01011000_00111000

Address of c[0] is: 0x7fa6e0405840 00000000_00000000_01111111_10100110_11100000_01000000_01011000_01000000
Address of c[1] is: 0x7fa6e0405844 00000000_00000000_01111111_10100110_11100000_01000000_01011000_01000100
Address of c[2] is: 0x7fa6e0405848 00000000_00000000_01111111_10100110_11100000_01000000_01011000_01001000
Address of c[3] is: 0x7fa6e040584c 00000000_00000000_01111111_10100110_11100000_01000000_01011000_01001100
```

You may get different values.

We see that each array has values with contiguous addresses. In
building our cache, we need to decide how to store a value at a given
address in our cache.

The simplest form of cache is a _direct-mapped_ cache (more
complicated caches are built out of these). Suppose it can store $2^N$
bytes of data. We divide it into blocks, each of $2^M$ bytes ($M <
N$). Each address references one byte, so we need to use $N$ _bits_ of
the address to select which slot in the cache to use. A picture can
help

{{< manfig
    src="cache-layout.svg"
    width="70%"
    caption="Schematic of a cache with block size $2^M$ and capacity $2^N$" >}}

We reserve the lowest $N$ bits of the address (the rightmost) to
compute a 2D cache location. Each address is mapped into one of the
$2^{N-M}$ _blocks_. The correct byte is located with the lowest $M$
bits. Finally the high bits of the address are used as a key.

Data are loaded into the cache one _block_ at a time (these are also
called _cache lines_). So if we access an address that access byte 4 of
block 2 (say), then we load data from an address that lives at byte 0
of block 2 up to byte $2^M$.

{{< manfig
    src="cache-line-load.svg"
    width="70%"
    caption="Data are loaded in full cache lines, even when we only request a single byte." >}}

By loading data in cache lines, we immediately exploit _spatial
locality_. The optimal size of the block is a function of the total
cache size and the particular data access patterns. So almost all CPUs
have arrived at the same tradeoff of using a 64 Byte cache line.

{{< hint info >}}
A consequence of this loading strategy is that _cache-friendly_
algorithms work on cache line sized chunks of data at a time. That is,
we should endeavour, when loading data, to use the entire cache line.
{{< /hint >}}

#### Eviction from caches

Recall that our cache is much _smaller_ than the total size of main
memory. So it is possible that we will want to load more data than fit
in a cache. If two different addresses have the same low bit pattern,
they will be mapped to the same location in the cache. We only have
space to store one of them, so we have a _conflict_.

Since the more recently requested data is required _now_, our
resolution is to _evict_ the older data and replace it with the new
address (this is an LRU (least recently used) eviction policy).

Let's think about what can go wrong with this policy. Consider a
simple example where we perform the following loop.

```c
int a[64];
int b[64];
int r = 0;
for (int i = 0; i < 100; i++) {
  for (int j = 0; j < 64; j++) {
    r += a[j] + b[j];
  }
}
```

Let's imagine executing this on a system where each `int` requires
four bytes and we have a single direct-mapped cache with total size
1KB and a 32 byte cache line. So we have $N = 10$ and $M=5$ and there
are 32 lines in the cache.

Each cache line therefore holds eight `int`s, and we need eight cache
lines for `a` and eight cache lines for `b`. This is 16 total lines
which is less than the cache size (so everything should be fine?).

A problem occurs if entries in `a` and `b` map to the _same_ cache
lines. In this circumstance, an entry from `a` will be loaded, filling
a line, then an entry from `b` will be loaded, filling the same line
and evicting `a`. Then we move to the next iteration where we load
`a`, evicting `b` and so forth.

In the worst case, `a` and `b` map to exactly the same lines and we
reduce the _effective_ size of a cache from 1KB to 32bytes.

This phenomenon is known as _cache thrashing_, there's a nice worked
example
[here](https://cs233.github.io/thrashingandassociativityexample.html),
that page also has some other [worked
examples](https://cs233.github.io/otherresources.html) on mapping
arrays into caches.

To avoid mitigate against this problem, actual CPUs normally have
slightly more complicated caches. Typically they use $k$-way set
associative caches.

A $k$-way set associative cache behave like k "copies" of a direct
mapped cache. Each block of main memory can map to one of $k$ cache
lines, which are termed _sets_. Usually, hardware designers choose $k
\in \\{2, 4, 8, 16\\}$. For example, [Intel Skylake chips](https://en.wikichip.org/wiki/intel/microarchitectures/skylake_(server)#Memory_Hierarchy) have $k = 8$
for level 1 caches, $k = 16$ for level 2, and $k = 11$ for level 3.

A $k$-way cache, can handle up to $k$ different addresses mapping to
the same cache location without a reduction in the perceived size of
the cache. This comes at a cost of increased complexity in
the chip design, and marginally increased latency: looking up and
loading data from a direct-mapped cache is slightly faster than
looking it up in a $k$-way cache.

However, they are not a silver bullet, as soon as we are handling more
than $k$ addresses that might map to the same location, we have the
same eviction problem.

### Programming cache friendly algorithms

To mitigate against some of these effects, if we know that the data
we'll be working on should fit in cache it is sometimes beneficial to
explicitly copy it into an appropriate sized buffer and then work on
the buffer. We will see an example of this in [exercise 8]({{< ref
"exercise08.md" >}}).

## Measurement of cache bandwidth

As well as providing lower latency data access, caches also provide
larger memory bandwidth (the rate at which data can be fetched from
the cache to the CPU).

We can use
[likwid-bench](https://github.com/RRZE-HPC/likwid/wiki/Likwid-Bench)
to measure memory bandwidth. In [exercise 2]({{< ref "exercise02.md"
>}}) you should do this to determine the cache and main memory
bandwidth on the Hamilton cores. We will use this to construct a
predictive model of the floating point throughput of the reduction
from [exercise 1]({{< ref "exercise01.md" >}}).

{{< exercise >}}

Now is a good time to attempt [exercise 2]({{< ref "exercise02.md"
>}}).

{{< /exercise >}}

FIXME: add results

## A predictive model for reductions

Let us remind ourselves of the code we want to predict the performance
of

{{< columns >}}

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

{{< /columns >}}

The accumulation parameter `c` is held in a register. At each
iteration of the vectorised loop, we load eight elements of `a` into a
recond register. Since each `float` value takes 4 bytes, this means
that each iteration of the loop requires 32 bytes of data.

{{< hint info >}}

If you don't know the size (in bytes) of C datatypes off the top of
your head, you can always determine them by writing some C code to
print them out using
[sizeof](https://en.cppreference.com/w/c/language/sizeof):

```c
#include <stdio.h>
int main(void)
{
  printf("Size of float: %lu\n", sizeof(float));
}
```

{{< details "Size in bits" >}}
If we want to know how many bits there are in a byte, we should
technically check the value of the macro `CHAR_BIT` (defined in
`<limits.h>`). On all systems you are likely to encounter, it is 8.
See [CPP reference on
sizeof](https://en.cppreference.com/w/c/language/sizeof) for more
information.
{{< /details >}}
{{< /hint >}}

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

{{< autofig src="sum-reduction-throughput-annotated.svg"
    width="60%"
    caption="AVX-enabled throughput of sum reduction as measured with `likwid-bench`. Annotated with simple performance limits from our measurements of cache bandwidth." >}}


We can see that this simple model does a pretty good job of predicting
the performance of our test code.

## Stepping back

This idea of predicting performance based on resource limits is a
powerful one, and we will return to it through the rest of the course.

{{< exercise >}}
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

{{< /exercise >}}

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

{{< manfig src="cacheschematic.svg"
    width="70%"
    caption="Example layout of caches and memory for a 4 core system."
    >}}
    
Although in this course we will spend most of our time focussing on
_single core_ performance, in practice, most scientific computing
algorithms will be parallel.

To understand how parallelisation will affect the performance on real
hardware, we need to know if will be limited by a resource which is
scalable, or saturating.

{{< columns >}}
### Scalable resources

These resources are private to each core/chip. For example, CPU cores
themselves are a _scalable_ resource. Adding a second core doubles the
number of floating point operations we can perform.

As a consequence, if our code is limited by the floating point
throughput, adding more cores is a useful thing to do.

{{< autofig src="scalable-resource.svg"
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

{{< autofig src="saturating-resource.svg"
    width="100%"
    caption="Prototypical performance of a saturating resource" >}}

{{< /columns >}}

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
