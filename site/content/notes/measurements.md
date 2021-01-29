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

## Profiling

Suppose that you have a code and run it, but don't really know
anything else about it. You want to know which bits take time. What to
do?

We use _profiling_ of some kind to determine hotspots (regions of the
code where the bulk of the time is spent). Our optimisation cycle
should focus on these first, because the largest gains are available
here (recall
[Amdahl](https://teaching.wence.uk/phys52015/notes/theory/scaling-laws/#amdahl)
from last term). The other parts of the code may become important
after doing some optimisation, but we should start with the largest
chunk.

There are broadly-speaking two variants of profiling, _sampling_-based
and _instrumentation_-based.

### Sample-based profiling

The basic idea here is that I run my code and periodically poke it to
ask "what part of the code is currently executing?". Depending on how
exactly you compiled code (did it include debug symbols: `-g` in the
compile flags or not?), you might only get information about which
lines of assembly are taking all the time, or you might also see a
call graph (with function names). Generally speaking, I therefore
recommend compiling with debug symbols.

{{< hint info >}}

You should make sure that optimisation flags come textually after
debug symbols in your compiler flags

```
icc -g -OPTIMISATION_FLAGS_HERE ...
```

{{< /hint >}}

Sample-based profiling records the state it observes the code in every
time it it prods it. It then builds a statistical model of the code
execution. That is, it estimates the probability distribution of
observing the code in a routine $A$ over its lifetime. The total
estimated time spent in each routine is then the runtime of the
program multiplied by the probability that the code is in routine $A$.

{{< manfig
    src="samplingprofile.svg"
    width="70%"
    caption="A sampling profiler interrupts the program periodically to check which function it is in." >}}

Sample-based profiling is very low overhead, and typically
non-intrusive. However, we do not observe the complete behaviour. It
is also not very suitable to short-running applications (although do
we care about their performance?). We might get call-graph
information, or an approximation thereto (that is, which function is
called from which).

In terms of tools, the [Intel
VTune](https://software.intel.com/content/www/us/en/develop/tools/oneapi/components/vtune-profiler.html)
suite (available on Hamilton) has a sample-based profiling mode.
On most Linux systems, the [perf](http://perf.wiki.kernel.org)
kernel-level profiler is a available (unfortunately not on Hamilton).
There is a large infrastructure of tools around it, I like the
[pmu-tools](https://github.com/andikleen/pmu-tools/) developed by Andi
Kleen.

### Instrumentation-based profiling

This is often the next step after sample-based profiling has allowed
you to narrow down to a particular region of the code. Reasons for
doing this are so that we can take more detailed measurements, or
avoid "irrelevant" measurements from other parts of the code polluting
our profile.

The high-level picture of how this works is that we annotate the
source code (either manually, or automatically with some tool) in the
places we think are "interesting".

{{< manfig
    src="tracingprofile.svg"
    width="70%"
    caption="Instrumentation-based profiles measure events that have been annotated as 'interesting'" >}}

For example, when using the likwid tool, we often want to know
performance-counter information at a loop level (rather than whole
program). The likwid approach to this is to have a ["marker
API"](https://github.com/RRZE-HPC/likwid/wiki/TutorialMarkerC), we
annotate the parts of the code we're interested in with open and close
tags. All of the counts within the tags are agglomerated into the same
region

```c
#include <likwid.h>
...
void some_expensive_function(...)
{
  LIKWID_MARKER_START("Loop1");
  first_loops;
  LIKWID_MARKER_END("Loop1");
  something_unimportant;
  LIKWID_MARKER_START("Loop2");
  second_loops;
  LIKWID_MARKER_END("Loop2");
```

Reaching to likwid is something we do if we're already reasonably sure
of where we want to look. For a universally-available approach to
instrumentation based profiling (at least for C/C++ programs), we can
always use [gprof](https://sourceware.org/binutils/docs/gprof/). The
workflow is reasonably straightforward

{{< hint info >}}

#### `gprof` workflow

1. Compile and link code with debug symbols (`-g`) and profiling
   information (`-pg`)
2. Run the code, which produces a file `gmon.out`
3. Postprocess data with the `gprof` command
4. Look at results
{{< /hint >}}

`gprof` can provide both a flat profile (which function takes the most
time?) and a call-graph profile (where were expensive functions called
from?). The latter is more useful in complex code, since it might be
the case that calls to a function are cheap from one location but
expensive from another.

For example, imagine a code that calls
[`dgemm`](http://www.netlib.org/lapack/explore-html/d1/d54/group__double__blas__level3_gaeda3cbd99c8fb834a60a6412878226e1.html)
in two locations, once with tiny matrices, the other time with large
ones.

```c
void expensive_call(...)
{
  ...;
  dgemm(REALLY_BIG_MATRICES);
}

void cheap_call(...)
{
  ...;
  dgemm(SMALL_MATRICES);
}

...
int main(...)
{
  expensive_call(...);
  cheap_call(...);
}
```

We want to know to focus our efforts on the calls that are made to
`dgemm` with large matrices. A flat profile hides this information.

{{< exercise >}}

For specific usage instructions, the [gprof
manual](https://sourceware.org/binutils/docs/gprof/) is a good guide. 

[Exercise 6]({{< ref "exercise06.md" >}}) has a walkthrough using
gprof to find the right functions to look at for more detailed
profiling in a small molecular dynamics application.

{{< /exercise >}}

### Where next

Having found the right parts of the code to look at, we can inspect
the code to understand what it is doing. It is often worthwhile at
this point running measurements for a range of input parameters, to
see if we can observe the algorithmic scaling. For example, suppose
there is some parameter that controls the resolution in the model
(perhaps the total number of degrees of freedom). It is interesting to
see how the profile changes when we vary this parameter. We might
observe that the same part of the code is expensive in all cases, or
maybe the performance bottleneck will move.

Let us suppose, though, that we have target parameters in mind. Having
pinned down where to look, we need to understand the algorithm and
should attempt to construct a model of how fast we think the code
could run. We have seen a few ways how to do this, the roofline model
provides a very coarse estimate. If we want more detail, we probably
need to construct problem-specific models: hopefully the numerical
method will be well-studied. Dense matrix-matrix multiplication is a
good example, and we saw some analysis in [lecture 5]({{< static-ref
"lecture-slides/05.pdf" >}}).
