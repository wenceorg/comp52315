---
title: "Coursework: fast finite elements"
weight: 5
katex: true
---

# Fast finite elements

## Introduction

The finite element method is a popular and flexible method for the
numerical solution of partial differential equations. In this
coursework, you will study and benchmark some performance
optimisations for a prototypical finite element problem.

In particular, we will study the solution of the simplest possible
finite element problem, to solve the linear system

$$
A u = f
$$

where $A$ is the mass matrix. We will do this in a matrix-free manner,
that is, rather than creating the matrix $A$ and inverting it, we will
use a diagonally preconditioned [Conjugate
Gradient](https://en.wikipedia.org/wiki/Conjugate_gradient_method)
method to compute $u = A^{-1} f$. This only needs matrix-vector
products and the inverse of the diagonal of $A$.

This problem is the same as the first two ["bake-off"
problems](https://ceed.exascaleproject.org/bps/) proposed by the [CEED
project](https://ceed.exascaleproject.org).

A wide-scale study of these problems in a number of finite element
codes is presented in [this paper](https://arxiv.org/abs/2004.06722).
You may find this useful to set the scene and get some ideas as to
what kinds of performance to look for.

We will use their code to perform the benchmarking and the performance
study. You can obtain the code from [their github
repository](https://github.com/ceed/libceed/). You will also need a
recent version of [PETSc](https://gitlab.com/petsc/petsc/), build and
compile it following their instructions and set the `PETSC_DIR` and
`PETSC_ARCH` environment variables accordingly. 

{{< hint info >}}

On Hamilton, I provide a usable version of PETSc.

To use it, you'll need to use the following modules
```
gcc/9.3.0
intelmpi/gcc/2019.6
```
and set the environment variables
```
export PETSC_DIR=/ddn/data/vtdb72/petsc
export PETSC_ARCH=arch-linux2-c-opt
```

{{< /hint >}}

With PETSc available, you can build the CEED library following their [build
instructions](https://github.com/ceed/libceed/#building): the
simplest thing to do is run `make` in the `libceed` directory.

{{< hint info >}}

On Hamilton, you will need to provide the location of the BLAS
library when building libceed. If you are using my version of
PETSc as specified above you'll need to do:

```
make BLAS_LIB="-L$PETSC_DIR/$PETSC_ARCH/lib -Wl,-rpath,$PETSC_DIR/$PETSC_ARCH/lib -lopenblas"
```

In addition to any options you pass to `make`.

{{< /hint >}}

We will use the `bpsraw` benchmark program. Having built the library,
go to the `examples/petsc` subdirectory and do

```
$ cd examples/petsc
$ make bpsraw
```

We can now run the benchmark, it uses commandline options to control
which problem to run, along with the approximation degree and size of
the problem. We're interested in the `bp1` and `bp2` problems.

```
$ ./bpsraw -problem bp1

-- CEED Benchmark Problem 1 -- libCEED + PETSc --
  PETSc:
    PETSc Vec Type                     : seq
  libCEED:
    libCEED Backend                    : /cpu/self/xsmm/blocked
    libCEED Backend MemType            : host
    libCEED User Requested MemType     : none
  Mesh:
    Number of 1D Basis Nodes (P)       : 2
    Number of 1D Quadrature Points (Q) : 3
    Global nodes                       : 1331
    Process Decomposition              : 1 1 1
    Local Elements                     : 1000 = 10 10 10
    Owned nodes                        : 1331 = 11 11 11
    DoF per node                       : 1
  KSP:
    KSP Type                           : cg
    KSP Convergence                    : DIVERGED_ITS
    Total KSP Iterations               : 20
    Final rnorm                        : 3.042095e-08
  Performance:
    Pointwise Error (max)              : 1.088768e-02
    CG Solve Time                      : 0.00696532 (0.00696532) sec
    DoFs/Sec in CG                     : 3.82179 (3.82179) million
```

You might see different numbers and details, but the idea is the same.

You can control (approximately) the size of the problem by modifying
the number of `Local Elements`.

```
$ ./bpsraw -problem bp1 -local 10000
...
    Global nodes                       : 11466
    Process Decomposition              : 1 1 1
    Local Elements                     : 10000 = 20 20 25
...
```

Finally, we can change the computational cost of the main
matrix-vector product by modifying the polynomial degree (which is
equal to $P-1$ where $P$ is the number of 1D basis nodes shown above).

```
$ ./bpsraw -problem bp1 -degree 7
...
    Number of 1D Basis Nodes (P)       : 8
    Number of 1D Quadrature Points (Q) : 9
    Global nodes                       : 960
...
```

### Selecting backends

LibCEED comes with a number of different backends, you can select
these by specifying `-ceed BACKEND_STRING`. A short description is
given [here](https://github.com/ceed/libceed/#backends). If you used
the default build parameters then you have access to

- `/cpu/self/ref/serial`
- `/cpu/self/ref/blocked`
- `/cpu/self/avx/serial`
- `/cpu/self/avx/blocked`

For example, the run
```
$ ./bpsraw -ceed /cpu/self/ref/blocked`
```
selects the "reference" blocked backend.

We'll use these backends when benchmarking.

{{< hint info >}}
You may also wish to compile the library with
[libxsmm](https://github.com/hfp/libxsmm), in which case you'll also
have access to
- `/cpu/self/xsmm/serial`
- `/cpu/self/xsmm/blocked`
{{< /hint >}}

### Parallelism

The code is parallelised using MPI. So you can run it in parallel with

```
$ mpiexec -n NUMBER_OF_PROCESSES ./bpsraw ...
```

### Likwid annotations

If you want to add likwid annotations (for example to get floating
point throughput), you'll need to edit `bpsraw.c` to include the
likwid header and add appropriate `LIKWID_MARKER_XXX` calls. 

The main computational work is the section of code between the two `//
-- Performance logging` comments in `main`.

{{< hint info >}}

To make it easy to add these markers and compile both with, and
without, likwid, you could use the [`likwidinc.h`]({{< code-ref
"snippets/likwidinc.h" >}}) header we've used previously.

{{< /hint >}}

In addition, you'll then also need to add

```
CPPFLAGS += -DLIKWID_PERFMON
LDLIBS += -llikwid
```

In the `Makefile` after their definitions and rebuild.

{{< hint info >}}

This is rather fiddly with MPI parallelism, so you might only want to
do likwid annotation of serial runs.

{{< /hint >}}

## Task

You should perform a profiling and performance analysis of the code.

You should study:

1. How the performance varies for the different backends as a function
   of both the polynomial degree (the `-degree` argument, vary this
   from 1 to 16) and the problem size (the `-local` argument, vary
   this from 1000 to 500000). Which backend provides the best
   performance? What about the worst? Does it depend on the degree or
   problem size?
1. Do your findings/conclusions change when you move from `bp1` to
   `bp2`?
1. For the best and worst backends, how does the performance scale in
   parallel? You can restrict yourself to a single Hamilton node.
1. How close to the hardware performance limits you think the code is.
   For example, you could perform a roofline study.

You should write up your findings and a discussion in a short report.
It should describe the experiments you performed, and present (in an
appropriate manner) the data you collected along with the answers to
the above questions.


## Mark scheme and submission

You should submit, via ULTRA, a **PDF** of your report (max 5 pages,
including figures and tables), named by your CIS username.


| Artifact | Descriptor                                  | Marks |
|---------:|:--------------------------------------------|-------|
|   Report | Appropriate use of performance measurements | 20    |
|   Report | Appropriate use of performance models       | 20    |
|   Report | Analysis and presentation of data, writing  | 60    |

The reports will be marked with reference to the [descriptors for
written
work](https://durhamuniversity.sharepoint.com/teams/MScScientificComputingandDataAnalysis/SitePages/Written-Work-Descriptors-(Non-Dissertation).aspx)
on the MISCADA sharepoint site.
