---
title: "Profiling"
weight: 6
---

# Finding a hotspot and determining the execution limits

So far, we've only run very simple benchmarks. Now we're going to try
and find some information in a larger code. We will look at the
[`miniMD`](https://github.com/Mantevo/miniMD) application which has
been developed as part of the [Mantevo](https://mantevo.github.io)
project. This is a molecular dynamics code that implements algorithms
and data structures from a large research code, but in a small package
that is amenable to benchmarking and trying out different
optimisations.

The aim is to run and profile the code to determine where it spends
all its time, and then dig a little deeper using likwid markers and
the performance counters API.

## Download and compile

`miniMD` is maintained on
[GitHub](https://github.com/Mantevo/miniMD/), so after logging in to
Hamilton, you can get the source code with

```
git clone https://github.com/Mantevo/miniMD.git
```

The code is parallelised with MPI, so we need to load some modules to
make the right compilers available.

```
module load gcc/8.2.0
module load intel/2019.5
module load intelmpi/intel/2019.6
```

We will compile the "reference" implementation which is the in `ref`
subdirectory. The build system uses
[`make`](https://www.gnu.org/software/make/), so first we'll just
compile and check things work. Run `make intel`. You should see some
output, which ends with

```
size ../miniMD_intel
   text    data     bss     dec     hex filename
 219867   20880    2304  243051   3b56b ../miniMD_intel
make[1]: Leaving directory `.../miniMD/ref/Obj_intel'
```

You can check the compilation was successful with `make test`

## Compile and run with profiling enabled

Having verified the code runs correctly, we will now recompile with
profiling enabled. First run `make clean` to delete the
executable. You will need to edit the `Makefile.intel` file
to add `-pg` to the compile and link flags (do this by
modifying the `CCFLAGS` and `LINKFLAGS`
variables). Now run `make intel` again.

{{< hint "info" >}}
For more information on gprof, the HPC centre at Lawrence Livermore
have a [useful introductory
tutorial](https://hpc.llnl.gov/software/development-environment-software/gprof#documentation).
{{< /hint >}}

{{< exercise >}}
1. Profile the default run on a compute node. This should
   produce a `gmon.out` file.
1. Produce the gprofile output with `gprof ./miniMD_Intel gmon.out`
   (if it scrolls off the screen, you can redirect the output to a
   text file by appending `> SOMETEXTFILE.txt` to the
   command and then look at it in an editor)

{{< /exercise >}}

{{< question >}}

Where does the code spend most of its time?

{{< /question >}}

{{< exercise >}}

`miniMD` implements a few different algorithms
which can be selected with command line options and choosing
the right input file. Run profiling for the following sets of options.

1. `./miniMD_intel -i in.lj.miniMD --half_neigh 0`
1. `./miniMD_intel -i in.lj.miniMD --half_neigh 1`
1. `./miniMD_intel -i in.eam.miniMD --half_neigh 0`
1. `./miniMD_intel -i in.eam.miniMD --half_neigh 1`

{{< /exercise >}}

{{< question >}} 
Do you always see the same functions at the top of
the profile?

{{< /question >}}

{{< hint "warning" >}}

Note: a program instrumented with gprof will always write its output
to `gmon.out` (overwriting any previous information). So
you should make sure to move the `gmon.out` from each run
to a unique name (perhaps describing briefly what you did) before
running the next benchmark.

{{< /hint >}}

### Generating graphical call graphs from gprof output

Inspecting the gprof output just as a text file can be slightly hard
to understand. A clearly overview can often be obtained by creating a
visualisation of the call graph. We can do this with
[`gprof2dot`](https://github.com/jrfonseca/gprof2dot) which is a
Python package that turns gprof (and other profiling output) into the
[dot](https://graphviz.gitlab.io/documentation/) graph format. This
can then be converted to PDF, PNG, or other graphical formats (see the
[graphviz documentation](https://graphviz.gitlab.io/documentation/)
for more details).

Minimally, having installed `gprof2dot` and graphviz, you can generate
a call graph. First, you need to convert the `gmon.out` to the textual
format, with `gprof ./miniMD_intel gmon.out > gmon-output.txt`. Then
run `gprof2dot gmon-output.txt -o gmon-output.dot`. Finally, use `dot
-Tpdf gmon-output.dot -o callgraph.pdf` to generate a PDF.

## Instrumenting hotspot functions with likwid

Having determined which functions are the hotspots, we'll try and get
some more information about their performance. For this, we will use
`likwid-perfctr` and its [marker
API](https://github.com/RRZE-HPC/likwid/wiki/likwid-perfctr#using-the-marker-api).
We'll therefore need the likwid tools, so `module load likwid/5.0.1`
(remember that you will need to do this in the batch script, or on the
compute node too).

To do this, you will need to find the locations in the source files of
the functions you identified. A simple thing to do is to use
`grep` to find the locations. Running `grep -n
NAME_OF_FUNCTION *.cpp` will search for
`NAME_OF_FUNCTION` in all the cpp files and print matches,
along with line numbers.

For instrumentation with the marker API, we need to add start/stop
markers in the functions we care about, and also switch the markers on
in the main program. To get the relevant definitions in each file
you've identified as relevant, add the following after the existing
includes.

<div id="ref:includes">

```c
#ifdef LIKWID_PERFMON
#include <likwid.h>
#else
#define LIKWID_MARKER_START(a) do { (void)a; } while (0)
#define LIKWID_MARKER_STOP(a) do { (void)a; } while (0)
#define LIKWID_MARKER_INIT do { } while (0)
#define LIKWID_MARKER_THREADINIT do { } while (0)
#define LIKWID_MARKER_CLOSE do { } while (0)
#define LIKWID_MARKER_REGISTER(a) do { (void)a; } while (0)
#endif  /* LIKWID_PERFMON */
```

</div>

Then for each function you want to instrument in that file add
```c
LIKWID_MARKER_START("SOMEAPPROPRIATENAME");
```
at the beginning of the function and
```c
LIKWID_MARKER_STOP("SOMEAPPROPRIATENAME");
```
before the function returns.

{{< hint "warning" >}}
Be careful to check that the function
doesn't `return` anywhere before reaching
the `LIKWID_MARKER_STOP` call.
{{< /hint >}}

Finally, edit `ljs.cpp` and add [the includes]({{< ref "#ref:includes"
>}}) near the top of the file (as before). In the `main` function, add
```c
LIKWID_MARKER_INIT;
LIKWID_MARKER_THREADINIT;
```
at the beginning of the function and
```c
LIKWID_MARKER_CLOSE;
```
at the end (before the `return`).

Lastly, we need to add some more flags in the makefile, so edit
`Makefile.intel` and _remove_ `-pg` from both `CCFLAGS` and
`LINKFLAGS`. You'll then need to add `-DLIKWID_PERFMON` to `CCFLAGS` and
`-DLIKWID_PERFMON -llikwid` to `LINKFLAGS`. Finally, we're ready to
rebuild everything, so run `make intel`.

If you run `./miniMD_intel` on its own, everything should work, but
you should see `Running without Marker API. Activate Marker API with
-m on commandline.` being printed (this indicates that we managed to
successfully add all the performance monitoring, but are not yet using
`likwid-perfctr`).

{{< exercise >}} 

Run a profile of the memory and floating point performance using
`likwid-perfctr -C 0 -g MEM_DP -m ./miniMD_intel -i in.lj.miniMD
--half_neigh 1`.

{{< /exercise >}}

{{< question >}}

What computational intensity do you observe? For this computational
intensity, is the code at the roofline limit?
          
{{< /question >}}

{{< exercise >}}

Try the same profiling, but this time with with `--half_neigh 0` and
the `in.eam.miniMD` input file.

{{< /exercise >}}

{{< question >}}

Do you notice any differences in the profiles?

{{< /question >}}

{{< question >}}

Can you suggest some next steps to try and improve performance?

{{< /question >}}
