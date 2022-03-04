---
title: Cache blocking/tiling
weight: 5
katex: true
---

# Achieving reasonable performance for loopy code

Many of the algorithms we encounter in scientific computing have quite
"simple" data access patterns. Numerical code often has multiple
nested loops with regular array indexing. This is actually a reason
the roofline model is so successful: its optimistic assumptions are
not _too_ optimistic.

Despite this simplicity, on hardware with memory caches, we still need
to do some work to turn this "simple" code into something that runs
with reasonable performance.

What do we mean by "reasonable", perhaps 50% of the relevant resource
bottleneck (memory bandwidth or floating point throughput), and
_predictable_ performance without surprises. That is, we would hope to
achieve this 50% of peak throughout independent of the problem sizes.

{{< hint info >}}
This does not necessarily mean that our algorithm should have linear
runtime scaling with problem size. For example, it is possible to
achieve constant throughput (FLOPs/s) for matrix-matrix multiply. So
the performance is problem size independent, but since matrix-matrix
multiplication is an $\mathcal{O}(N^3)$ algorithm, bigger problems
will take more than linearly longer.
{{< /hint >}}

## What can go wrong?

We'll look at two exemplar problems that on the face of it are simple,
and we'll observe how their naive implementation _does not_ result in
"surprise-free" performance.

Both our our examples come from dense linear algebra.

1. Dense matrix transpose

   $$
   B_{ij} \gets A_{ji} \quad A, B \in \mathbb{R}^{N\times N}
   $$
   
2. Dense matrix-matrix multiply

   $$
   C_{ij} \gets C_{ij} + \sum_{k=1}^{N} A_{ik}B_{kj} \quad A, B, C \in
   \mathbb{R}^{N\times N}
   $$
   
These are at "opposite" ends of the roofline plot. Matrix
transposition has arithmetic intensity of zero (it does no floating
point operations), and is therefore definitely limited by streaming
bandwidth. Conversely, matrix-matrix multiplication does
$\mathcal{O}(N^3)$ floating point operations on $\mathcal{O}(N^2)$
data, for a arithmetic intensity of $\mathcal{O}(N)$ in the [perfect
cache
model]({{< ref "roofline.md#cache-models" >}}).

{{< hint info >}}
This is actually overly optimistic of what we can achieve, but
matrix-matrix multiplication is nonetheless still firmly in the
flop-limited regime.
{{< /hint >}}

## Matrix transposition

The naive implementation of transposition of a matrix looks like this

```c
void transpose(const double * restrict A, double * restrict B, int N)
{
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
      B[i*N + j] = A[j*N + i];
}
```

Since all this is doing is copying data, we might hope to see
performance close to the streaming memory bandwidth, independently of
$N$. Let's have a look. On my laptop, the STREAM triad memory
bandwidth is around 18GB/s.

Using the `transpose` code from [exercise 7]({{< ref "exercise07.md"
>}}), I can measure the achieved memory bandwidth for a range of
matrix sizes.

```
$ for N in 100 200 300 500 700 1000 1500 2000 2500 3000 4000 5000; do
> ./transpose $N $N
> done
```

This prints a bunch of output, which I elide, but we can put it in a
table

| $N$  | BW [MB/s] |
|------|-----------|
| 100  | 60000     |
| 200  | 46700     |
| 300  | 46600     |
| 500  | 30600     |
| 700  | 23600     |
| 1000 | 11900     |
| 1500 | 11800     |
| 2000 | 4800      |
| 2500 | 4300      |
| 3000 | 3800      |
| 4000 | 3300      |
| 5000 | 2900      |

So we start out in the cache, and observe bandwidth significantly
above the main memory bandwidth. However, as the matrices get larger,
this falls off, and we end up with throughput much below the streaming
bandwidth.

If we inspect the code, we notice that we have streaming access to `B`
(good!), but stride-N access to `A`. This latter access pattern is bad
for performance because we read a full cache line (64 bytes) each time
we load an entry of `A`, but only use one double precision entry (8
bytes). As a consequence, we need to hold $LN$ bytes of `A` in the
cache at once, where `L` is the number of elements that fit in one
cache line (here 8). Once the matrices get too large, this is no
longer possible.

Suppose that we estimate the time to write, or read, one byte as the
inverse of the streaming bandwidth

$$
t_\text{write} = t_\text{read} = \frac{1}{18} \text{ s/GB}
$$

Remembering that when we are streaming from main memory, that we only
use an eighth of the effective bandwidth for the load of `A`, then a
model for the total time for transposing an $N\times N$ matrix is
$$
T = N^2(8t_\text{read} + t_\text{write}).
$$
In other words, our model says that in the limit of large $N$ we will
only observe $\frac{1}{9}\text{th}$ of the streaming memory bandwidth.

Here's a picture of what is going on. The data are laid out typewriter
style, and the red boxes indicate loads that provoke the load of a
full cache line. We see that when writing to `B`, we always use all of
the loaded cache line, but the same is not true for `A`.

{{< columns >}}

{{< manfig
    src="strideoneaccess.svg"
    width="100%"
    caption="Access to `B` is stride-1, so works well." >}}
    
<--->

{{< manfig
    src="stridenaccess.svg"
    width="100%"
    caption="Access to `A` is stride-N, and so for large matrices we do not get reuse." >}}

{{< /columns >}}


### Cache tiling for better performance

What we need to do is to reorder the loops so that we run over little
blocks, transposing them while keeping them in cache. The techniques
we use are called _strip mining_ and _loop reordering_. We might also
hear the terms _cache blocking_, _loop tiling_, or _loop blocking_.
The idea is to break loops into blocks of consecutive elements and
then reorder for better spatial locality.

{{< columns >}}
#### Original loops


```c
for (int i = 0; i < N; i++)
  for (int j = 0; j < N; j++)
    B[i*N + j] = A[j*N + i];
```
<--->
#### After loop blocking

```c
for (int ii = 0; ii < N; ii += stridei)
  for (int jj = 0; jj < N; jj += stridej)
    for (int i = ii; i < min(N, ii+stridei); i++)
      for (int j = jj; j < min(N, jj+stridej); j++)
        B[i*N + j] = A[j*N + i];
```

The loop over `i` is split into two, with the outer loop strided by
`stridei`, the `j` loop is similarly split (with `stridej`).
{{< /columns >}}

This changes the iteration order over the matrices

{{< columns >}}

#### Before loop blocking

{{< manfig 
    src="originalrowmajororder.svg"
    width="100%"
    caption="Iteration over `B` is good" >}}
    
{{< manfig 
    src="originalcolmajororder.svg"
    width="100%"
    caption="Iteration over `A` does not get cache reuse" >}}
    
<--->
#### After loop blocking

{{< manfig 
    src="tiledrowmajororder.svg"
    width="100%"
    caption="Iteration over `B` is still good" >}}
    
{{< manfig 
    src="tiledcolmajororder.svg"
    width="100%"
    caption="Iteration over `A` now gets cache reuse" >}}

{{< /columns >}}

[Exercise 7]({{< ref "exercise07.md" >}}) also provides a blocked
version. So we can run the same experiment as before, except this time
with loop tiling

| $N$  | BW (original) | BW (tiled) | BW (OpenBLAS) |
|------|---------------|------------|---------------|
| 100  | 60000         | 56200      | 63200         |
| 200  | 46700         | 45300      | 57900         |
| 300  | 46600         | 47800      | 58600         |
| 500  | 30600         | 41600      | 54500         |
| 700  | 23600         | 19200      | 35900         |
| 1000 | 11900         | 12700      | 27000         |
| 1500 | 11800         | 10600      | 24100         |
| 2000 | 4800          | 10500      | 23500         |
| 2500 | 4300          | 10400      | 23700         |
| 3000 | 3800          | 10800      | 26600         |
| 4000 | 3300          | 11200      | 22300         |
| 5000 | 2900          | 12600      | 19000         |

So now we get consistent performance outside of the cache, with around
60-70% of streaming bandwidth. I also added the bandwidth obtained
from using an optimised transpose from the
[OpenBLAS](https://www.openblas.net) library. We see that this is even
slightly higher than the STREAM triad number. I suspect this is because they
have implemented this copy with a [non-temporal (or streaming)
store](https://vgatherps.github.io/2018-09-02-nontemporal/).

## Matrix-matrix multiplication

Now let's look at the matrix multiplication problem. To analyse this
problem, we will postulate a two-level memory system with a small fast
memory, and a large slow memory. We start out with all the data in
slow memory.
