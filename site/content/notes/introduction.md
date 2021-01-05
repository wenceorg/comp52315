---
title: "Introduction"
weight: 1
katex: true
---

# Resources in stored program computers

To understand the performance of a code, we need to have an
understanding of what hardware resources it _uses_, and what resources
the hardware provides.

All modern general purpose hardware uses a [von Neumann
architecture](https://en.wikipedia.org/wiki/Von_Neumann_architecture).
That is, there is a memory which stores both the program code to be
executed and the data for the program. This is attached to a processor
(CPU) which contains execution units for executing individual
instructions in the program code along with further parts of logical
control and load/store of data.

{{< manfig src="CPU.png"
   width="50%"
   caption="High level view of processor architecture and resource use" >}}

From a (simplified) hardware designer's point of view, the primary
resource of the processor is _instruction execution_. The primary
hardware design goal is to _increase_ instruction throughput
(instructions/second).

Hence, all instructions are considered "work" by processor designers.
Unfortunately, there is a mistmatch here, because not all instructions
are considered to be "work" by software developers (you!).

## Resource bottleneck: instruction throughput

As a simple example, consider some code to add two arrays together

```c
void add_arrays(int N, double * restrict a, const double * restrict b)
{
  for (int i = 0; i < N; i++)
    a[i] = a[i] + b[i];
}
```

{{% columns %}}

## Programmer view

This loop does \\(N\\) floating point additions, the work is
therefore \\(N\\) flops.

<--->

## Processor view

When compiled, this loop is translated into (approximately) the
following sequence of instructions (in assembly "pseudo-code")

```asm
.top
LOAD r1 with a[i]
LOAD r2 with b[i]
ADD r2 to r1 into r1
STORE a[i] with r1
INCREMENT i
GOTO .top IF i less than N
```

So the processor sees the work as \\(6 N\\) instructions.

{{% hint "info" %}}
The actual loop, when compiled with GCC looks like this
```asm
.L3:
movsd   (%rsi,%rax,8), %xmm0
addsd   (%rdx,%rax,8), %xmm0
movsd   %xmm0, (%rsi,%rax,8)
addq    $1, %rax
cmpq    %rdi, %rax
jne     .L3
```

You can explore this in more detail, if you like, on the [compiler explorer](https://gcc.godbolt.org/z/5MqKqr)
{{% /hint %}}
{{% /columns %}}

## Resource bottleneck: data transfers

From the point of view of instruction execution, data movement (from
the memory to the CPU and back again) is a _consequence_ of
instruction execution and therefore considered a secondary resource.
The throughput is measured in bytes/second (or bandwidth) and is
determined by two things

1. Hardware limitations
2. The rate at which load and store instructions (which move data) can
   be executed.
   
For example, returning to our simple example and just considering the
instructions which move memory, we can see that there are three such
instructions per loop iteration.

```asm
LOAD r1 with a[i]
LOAD r2 with b[i]
STORE a[i] with r1
```

As written, the code adds double precision floating point numbers,
each of which is 8 bytes. The loop therefore requires 24 bytes of data
movement per loop iteration.

## Understanding performance limits

To understand the performance of a code, and more importantly its
performance _limits_, we must to first order answer the question

{{% question %}}

What is the resource bottleneck?

{{% /question %}}

In this simple model it is either data transfer or instruction
execution. In the rest of the course we'll see how to answer these
question through a combination of measurements and models.

## Real hardware and abstract models

Programming languages abstract away details of the hardware (for
example how many registers, or how much memory the chip has). The
abstract model adopted is the von Neumann model. We logically consider
a sequential stream of instructions and the abstract hardware
sequentially executes instructions. In this model each instruction
completes before the next one starts

{{< manfig src="vonneumann.png"
   width="70%"
   caption="Diagram of the abstract machine model for the von Neumann architecture" >}}
   
This was a reasonable match with reality when first introduced in the
1940s, however the hardware in modern CPUs does not match the abstract
model nearly as well. For a simple example, most instructions in
modern chips have a _latency_ of more than one clock cycle.

{{% hint "info" %}}

To make our discussion independent of the frequency at which a chip is
executing, we'll count time in terms of clock "cycles". For example, a
1GHz processor runs at one billion cycles per second.

{{% /hint %}}

Let's consider our addition loop as a simple example again

{{% columns %}}

```asm
LOAD r1 with a[i]
LOAD r2 with b[i]
ADD r2 to r1 into r1
STORE a[i] with r1
INCREMENT i
```

<--->

Suppose that our CPU can execute one instruction per cycle. If every
instruction has a latency of one cycle, then there are no "wasted"
cycles in this loop.

On the other hand, if `ADD` has a latency of three cycles, then there
are two wasted cycles when the CPU is doing nothing (between the `ADD`
and the `STORE`). This is shown graphically below.

{{% /columns %}}

{{< manfig src="addlatency.png"
    width="70%"
    caption="Comparison of throughput for different `ADD` latencies">}}


### Strategies for faster chips

Trying to maintain the illusion offered by the abstract model, while
squeezing more performance out of the actual hardware, has been a
driving goal in chip design for a while. There are a number of
different ways this can be achieved with the end goal of increasing
instruction throughput. The simplest, and nicest for the programmer,
is to increase clock speed.

Suppose that I change the base frequency of a chip from 1GHz to 2GHz,
and leave everything else the same. The instruction throughput
doubles. This is wonderful for me as a programmer, all the code I
write just suddenly goes faster.

Unfortunately, this approach has not really been viable for [15
years](http://www.gotw.ca/publications/concurrency-ddj.htm). The
reason is that when I run a chip at higher frequency it generates more
heat, which must be dissipated. So we've run into physical limitations
on cooling.

What chip designers have been turning to is to provide more
_parallelism_ on the chip. For example, they can't give you one 4GHz
CPU, but they can easily give you four 1GHz CPUs. This will have the
same throughput _if_ there are no dependencies between all the work
(and you can split it up). Unfortunately, this has moved the problem
from the hardware designer onto the programmer.

### Handling latency

There are some tricks available to the hardware designer that they use
to increase instruction throughput. These are all examples of how the
actual hardware is doing something more complicated than the
abstraction von Neumann model.

We'll briefly mention three:

1. Superscalar execution
2. Pipelining
3. Out of order execution

Typical CPUs can issue _more than one_ instruction per cycle (modern
Intel CPUs can issue up to four per cycle). As long are there are no
data dependencies between instructions, we can therefore increase
throughput.

For our simple addition example, the two loads are independent, so
superscalar execution might look something like the figure below.

{{< manfig src="addsuperscalar.png"
   width="70%"
   caption="Superscalar execution saves one cycle." >}}

Achieving peak performance actually requires that the code we write
utilises this facility, we'll see that when we look at floating point
throughput [later](TODO LINK).

Another mechanism to fight instruction latency is _pipelining_. Most
instructions have a latency of more than one cycle, meaning that if we
issue and wait for the result, we have lots of wasted cycles. Chips
therefore tend to have a _pipeline_ for instructions, on the
assumption that codes will typically issue a large number of
instructions that are the same (just operating on different data). 

Here's an example with a pipeline of length four. Each individual
instruction takes four cycles to execute, but we can have four
instructions "in flight" simultaneously (in the pipeline). So once the
pipeline is full, we observe _throughput_ of one instruction per cycle
(rather than one instruction every four cycles).

{{< manfig src="pipeline.png"
   width="70%"
   caption="Pipelines can hide latency if many identical instructions are issued." >}}

Finally, the technique in a hardware designer's toolbox that takes us
the furthest away from the von Neumann model is that of out-of-order
execution. Our program feeds in instructions in some order to the CPU
one at a time, however, the CPU does not typically execute those
instructions in the order they are provided. Instead, instructions are
_reordered_ based on availability of input data and execution units
(while still requiring that data dependencies are obeyed).

This is a powerful technique to keep execution units busy and avoid
`NOOP` bubbles in the instruction stream. As an example, consider two
iterations of our addition loop, where we recall that the `ADD`
instruction has a three cycle latency. By interleaving the
instructions from the second iteration before the store of the first
iteration, we can completely hide the `ADD` latency.

{{< manfig src="addoutoforder.png"
   width="70%"
   caption="Out of order execution hides instruction latency" >}}

## Data parallelism

In this course we will focus almost exclusively on single-core
performance. The rationale for this is that it single-core and
multi-core + distributed memory parallelism are somewhat orthogonal.
It's best to start "at the bottom".

Let's remind ourselves of the problem, we'll take our toy example
summing arrays

```c
void add_arrays(int N, double * restrict a, const double * restrict b)
{
  for (int i = 0; i < N; i++)
    a[i] = a[i] + b[i];
}
```

We've seen that instruction throughput can be a bottleneck here. One
way that chip designers have fixed this is to make individual
instructions operate on more data at once. This is termed
_vectorisation_.

Intel CPUs have vector registers, exactly which ones depends a little
on the vintage, that are either 128, 256, or 512 bits wide. What does
this mean?

If we restrict ourselves to thinking about double precision values
(which take up 64 bits) then each register can hold between two and
eight such values. The slots in the register are called _vector
lanes_.

{{< manfig src="registerwidth.png"
   width="70%"
   caption="Vector registers." >}}

The idea is that these registers hold multiple values, and the
instructions in which they take part operate on multiple values in one
go.

{{% columns %}}

#### Scalar addition

Standard scalar operations for this addition loop produce one output
element per instruction

{{< manfig src="scalaradd.png"
   width="100%" >}}
   
<--->

#### AVX addition

AVX addition (256 bit registers) produces four output elements per
instruction.

{{< manfig src="vectoradd.png"
   width="100%" >}}

{{% /columns %}}

## Putting things into practice

With all that, we're now going to look at what this means for the
execution speed of simple code. [Exercise 1]({{< ref "exercise01.md"
>}}) walks through an example of a simple "reduction". You can think
of this as a proxy for computing the dot product of two vectors. We'll
look at the effect of vectorisation on throughput.
