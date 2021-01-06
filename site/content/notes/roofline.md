---
title: "Performance models: roofline"
weight: 3
katex: true
---

# Models of performance

If our goal is to improve the performance of some code we should take
a scientific approach. We must first define what we mean by
_performance_. So far, we've talked about floating point throughput
(GFlops/s) or memory bandwidth (GBytes/s). However, these are really
secondary characteristics to the primary metric of performance of a
code:

**How long do I have to wait until I get the answer?**

Therefore, we should not lose sight of the overall target of any
performance optimisation study, which is to _minimise the time to
solution_ for a given code.

Our goal is then to come up with a hypothesis-driven optimisation
cycle. A simple flow diagram is shown below

{{< manfig src="optimisationworkflow.svg"
    width="30%"
    caption="Simplified flow diagram for deciding on next steps when optimising code" >}}

The idea is that we decide that the time to solution is too long, and
must therefore optimise the code. We _profile_ the code to determine
where it spends all (or most) of its time, and then construct a model
that explains that time. With a model in hand, we can make a
prediction about the best optimisation to apply. 

{{< hint info >}}
Note that we might not always be able to optimise code such that it
can run any faster. So another goal of using a model is to determine
that our code really is running is fast as we might expect.

That is, if we start from a position of "my code is running too slowly
for my liking", we need to determine if that is due to a bad
implementation, or just [hard
cheese](https://dictionary.cambridge.org/dictionary/english/hard-tough-cheese).
{{< /hint >}}

This allows us to focus our optimisation efforts where they will be
most effective.

To do this, we need to construct some models, we'll see a number of
approaches in this course, the first one we'll consider is the
[_roofline model_](https://doi.org/10.1145/1498765.1498785). This is a
simple model for loop heavy code.

{{< exercise >}}

Go away and read that paper, it's quite approachable. We'll discuss it
in class.

{{< /exercise >}}

## Roofline model

The roofline model has a simple view of both hardware and software

{{< columns >}}

### Simple view of hardware

{{< manfig src="rooflinecpumodel.svg"
    width="100%" >}}

<--->

### Simple view of software

```c
/* Possibly nested loops */
for (i = 0; i < ...; i++)
 /* Complicated code doing
    - N FLOPs causing
    - B bytes of data transfer */
```

This allows us to characterise the code using a single number, its
_operational intensity_, measured in FLOPs/byte

$$
I_c = \frac{N}{B}
$$

{{< /columns >}}

To this characterisation of the software, we add two numbers that
characterise the hardware

1. The _peak floating point performance_ \\(P_\text{peak}\\) measured
   in FLOPs/s
1. The _peak streaming memory bandwidth_ \\(B_\text{peak}\\) measured
   in bytes/s
   
Our challenge is then to ask what the limit on the performance of a
piece of code is. We characterise performance by how fast work can be
done, measured in FLOPs/s. The roofline model says that the bottleneck
is either due to

1. execution of work, limited by \\(P_\text{peak}\\),
1. or the data path \\(I_c B_\text{peak}\\).

We therefore arrive at a bound on performance

$$
P_\text{max} = \text{min}(P_\text{peak}, I_c B_\text{peak})
$$

This is the simplest possible form of the roofline model. It is an
_optimistic_ model, everything happens as fast as it possibly can.

### Why "roofline"

Let's draw a sketch of the performance limits to understand

{{< manfig src="rooflinesketch.svg"
    width="70%"
    caption="Sketch of the performance limits for the roofline model" >}}

The achievable performance for _any code_ using this model lies
underneath the pitched "roof" (hence roofline). The broad idea is that
we characterise the hardware once (peak floating point performance,
and peak streaming memory bandwidth) and measure $I_c$ for our code
along with _its_ floating point performance. We can then plot the
performance on a roofline plot, which will give an idea of which
performance optimisations are likely to pay off.

### A simple example

For example, consider three exemplar codes, plotted on a roofline

{{< autofig src="roofline-example-simple.svg"
    width="70%"
    caption="Exemplar roofline plot" >}}
    
The roofline model suggests that there is not really any room to
improve the implementation of "Code A", it's a memory
bandwidth-limited code and achieving that limit.

"Code B" is also memory bandwidth limited, but is not achieving that
limit, so there is potentially some room for optimisation.

Finally "Code C" is limited by floating point throughput, and is not
close.

This model therefore suggests that we don't need to do anything with
"Code A", we should look at ways of improving the memory traffic for
"Code B", and we should look at ways of improving the floating point
performance of "Code C".

{{< hint warning >}}

One thing you may immediately have spotted is that if we only have a
roofline plot to hand, we can't answer our initial question.

There is no way to know how fast these codes get the answer.

Consequently, we should not use it to judge which of a number of
different algorithms for a given problem are best (since we may be led
to false conclusions).

This is a general rule with optimisation. We start out with lofty goals
of improving runtime, but when we start profiling we look at memory
bandwidth and floating point throughput. It's easy to forget that
we're really wanting to improve runtime.

{{< /hint >}}

### Determining hardware characteristics

To figure out the performance limits of any given hardware, we have
two options:

1. Look up values on a spec sheet
2. Perform some measurements

Recall that we need two pieces of information, the memory bandwidth
and the floating point throughput.

#### Memory bandwidth

To look up the maximum achievable bandwidth on a spec sheet, we need
to know the CPU model, as well as what type of RAM is installed. Let's
do an example with the chips on Hamilton. To determine the installed
chip on a compute node I execute an interactive batch job and look at
the contents of `/proc/cpuinfo`, looking for the `model name`

```
$ ssh hamilton
$ srun --pty $SHELL
$ cat /proc/cpuinfo | grep "model name" | tail -1
model name	: Intel(R) Xeon(R) CPU E5-2650 v4 @ 2.20GHz
```

{{< hint info >}}
Whenever you run the likwid tools they will also report the CPU you
were running on.
{{< /hint >}}

Then I go to [ark.intel.com](https://ark.intel.com) and search of
`E5-2650`, I get two results and pick the [one at
2.2GHz](https://ark.intel.com/content/www/us/en/ark/products/91767/intel-xeon-processor-e5-2650-v4-30m-cache-2-20-ghz.html)

I go down to the memory specifications and see that this chip supports
4-channel DDR memory with a frequency of up to 2.4GHz. To compute the
maximum achievable memory bandwidth we have

$$
4 \times 2.4GHz \times 8Byte = 76.8GByte/s
$$

{{< hint warning >}}

Although the memory system can in theory deliver this, it is not
achievable by a single core: we'll see this when we measure stuff
later.

If you want lots of details on this, there are some excellent
stackoverflow posts: [1](https://stackoverflow.com/a/47787231),
[2](https://stackoverflow.com/a/47714514),
[3](https://stackoverflow.com/a/43574756).

{{< /hint >}}

There are two issues with this:

1. I don't actually know if Hamilton has DDR2400 RAM chips installed
2. This is a "guaranteed not to exceed" limit, in practice, even if we
   do have the right RAM, it is not typically achieved.
   
The alternative approach, which nearly everyone uses, is to _measure_
the memory bandwidth using the
[STREAM](https://www.cs.virginia.edu/stream/) benchmark. This is what
we will typically do, and [exercise 4]({{< ref "exercise04.md" >}})
does exactly that.

#### Floating point throughput

Again, the guaranteed not to exceed peak can be determined from spec
sheet frequencies and some knowledge of hardware. To do this, let's
look a little at how a CPU core actually works.
