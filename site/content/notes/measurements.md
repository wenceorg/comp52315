---
title: "Measurement and profiling"
weight: 4
katex: true
---

# Performance measurements

So far, we've seen the [roofline]({{< ref "roofline.md" >}}) model,
and observed that for floating point code, it allows us to get a
high-level view of what coarse step we should be taking to improve the
performance of our algorithm.

Now suppose that we do a roofline analysis for our code, we observe
that it should be limited by floating point throughput, but we are
nowhere near the roof.

This putative code is ripe for optimisation, we think, but where do we
start? We need to drill down a get more information about what is
causing the bottleneck: this includes figuring out _where_ the code
spends most of its time (if indeed there is a single place), and,
importantly confirming out initial hypothesis from the roofline
analysis. This data-driven approach will allow us to determine which
of our bag of optimisations we should be applying.

Our conclusion from this thought experiment is that we are going to
need to _measure_ the behaviour of our code, and perhaps match it up
with a model of performance.

{{< hint warning >}}

#### A caveat

This can get you a long way, but one thing to keep in the back of our
mind is that measurements can only tell us about the algorithm we're
using.

This kind of analysis might lead you to conclude that you need a
better algorithm (say), but it can't tell you that.

For example, measuring the performance of a code will only count the
data you _actually_ moved, not the data you could have moved.

Knowing what the right algorithms are requires that we have some
knowledge of the application domain.

{{< /hint >}}

## Performance counters

Modern hardware contains a (finite) number of special-purpose
registers which can be used to measure an almost overwhelming number
of different things about the behaviour of the hardware. This includes
things like the number of floating point instructions executed (split
by type); how often the caches at various levels were hit (or missed);
how often the branch predictor was correct; and many others.

We will typically use them to confirm a hypothesis generated from some
model, but we will also look tools that use (combinations of) these
performance counters can provide guided profiling and optimisation
advice.

### Grouping metrics

While it is possible to read low-level hardware counters directly, for
exampke, "how many floating point instructions were executed?", it is
much more useful to group them into higher-level or more "abstract"
metrics.

These are much easier to compare across hardware, and easier to
interpret. For example, suppose we have some floating point intensive
code and we determine that it runs for 2 seconds and executes
1241284912 floating point instructions. What are we to make of this? I
have no idea.

On the other hand, if I also know that the CPU executes at 2.9GHz,
then I can determine the instructions per cycle (IPC) as
$$
\frac{1241284912}{2 \times 2.9} \approx 0.2 \text{IPC}.
$$

On Intel hardware, for which I know that I can execute up to 2
floating point instructions per cycle, I can then easily see that my
code is not close to the performance limit [^1].

[^1]: This completely made up example came out right first time when I
    mashed the keyboard to get these numbers.


The tools we will look at, specifically
[`likwid-perfctr`](https://github.com/RRZE-HPC/likwid/wiki/likwid-perfctr),
have builtin abstract metrics for many of the obvious things you would
like to measure (we'll see how to access them), along with a
relatively easy way to add more.

`likwid-perfctr` offers a (reasonably) friendly command-line interface
and provides access both to the raw counters and many useful
predefined groups.

### A first example

Recall we previously saw the
[STREAM](https://www.cs.virginia.edu/stream/) benchmark when we were
looking at [realistic performance bounds]({{< ref
"roofline.md#memory-bandwidth" >}}) for the roofline model. Recall the
core loop we measure is

```c
for (int i = 0; i < N; i++) {
  c[i] = a[i]*b[i] + c[i];
}
```

If we compile this code on a chip that supports AVX2 instructions,
there are four differently vectorised implementations that are
available to us.

#### Scalar

```asm
LBB1_2:
	vmovsd	(%rsi,%rdi,8), %xmm0
	vmulsd	(%rdx,%rdi,8), %xmm0, %xmm0
	vaddsd	(%rcx,%rdi,8), %xmm0, %xmm0
	vmovsd	%xmm0, (%rcx,%rdi,8)
	incq	%rdi
	cmpq	%rdi, %rax
	jne	LBB1_2
```
#### SSE

```asm
LBB2_2:
	vmovupd	(%rsi,%rdi,8), %xmm0
	vmulpd	(%rdx,%rdi,8), %xmm0, %xmm0
	vaddpd	(%rcx,%rdi,8), %xmm0, %xmm0
	vmovupd	%xmm0, (%rcx,%rdi,8)
	addq	$2, %rdi
	cmpq	%rax, %rdi
	jl	LBB2_2
```

#### AVX

```asm
LBB3_2:
	vmovupd	(%rsi,%rdi,8), %ymm0
	vmulpd	(%rdx,%rdi,8), %ymm0, %ymm0
	vaddpd	(%rcx,%rdi,8), %ymm0, %ymm0
	vmovupd	%ymm0, (%rcx,%rdi,8)
	addq	$4, %rdi
	cmpq	%rax, %rdi
	jl	LBB3_2
```

#### AVX2

```asm
LBB4_2:
	vmovupd	(%rsi,%rdi,8), %ymm0
	vmovupd	(%rdx,%rdi,8), %ymm1
	vfmadd213pd	(%rcx,%rdi,8), %ymm0, %ymm1
	vmovupd	%ymm1, (%rcx,%rdi,8)
	addq	$4, %rdi
	cmpq	%rax, %rdi
	jl	LBB4_2
```

{{< question >}}

Did I really need to look at the assembly to count loads and stores,
or could I somehow have managed it by looking at the C code?

{{< /question >}}

Let's construct a model for how many load and store instructions this
loop executes (for each of the different variants) as a function of
the total vector length $N$.

{{< question >}}

For each different loop count the number of

1. loads
2. stores

For a vector width $v \in \{1, 2, 4\}$ how many loads and stores do
you expect to measure if the total loop length is $N$?

{{< /question >}}


{{< exercise >}}

Having constructed our model, let's actually [do this]({{< ref
"exercise05.md" >}}) to convince ourselves we understand what is going
on.

{{< /exercise >}}
