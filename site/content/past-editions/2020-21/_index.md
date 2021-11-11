---
title: 2020/21
weight: 1
draft: false
---

## Annotated slides 2020/21 edition

Slides as annotated during the live lectures. Recordings of the live
sessions are available if you're appropriately logged in. If you think
you should have access but don't, please [get in
touch](mailto:lawrence.mitchell@durham.ac.uk).

- [Session 1]({{< static-ref "lecture-slides/2020-21/01.pdf" >}}), [video](https://web.microsoftstream.com/video/47a15875-eddc-48b3-ac17-6b68ee46a8d6).
- [Session 2]({{< static-ref "lecture-slides/2020-21/02.pdf" >}}), [video](https://web.microsoftstream.com/video/ecf53b07-636e-4ffa-abe8-fe697230d56c).
- [Session 3]({{< static-ref "lecture-slides/2020-21/03.pdf" >}}), and the [roofline
  paper]({{< static-ref
  "lecture-slides/2020-21/williams2009-roofline" >}}.pdf),
  [video part I](https://web.microsoftstream.com/video/ba7827a0-3146-4396-afde-b51082729f8c)
  and [part II](https://web.microsoftstream.com/video/7c448687-e366-4492-8434-d0e98b5f556d).
- [Session 4]({{< static-ref "lecture-slides/2020-21/04.pdf" >}}), [video](https://web.microsoftstream.com/video/7c448687-e366-4492-8434-d0e98b5f556d).
  I got a bit confused towards the end in the exercises determining
  why likwid was reporting load counts different from those we were
  expecting. We worked this out (thanks Finlay) by the next session:
  the compiler was inlining some code and producing some extra moves.
- [Session 5]({{< static-ref "lecture-slides/2020-21/05.pdf" >}}), [video](https://web.microsoftstream.com/video/86b11649-7c68-4739-a3e9-4727ee9d0621).
- [Session 6]({{< static-ref "lecture-slides/2020-21/06.pdf" >}}), [video](https://web.microsoftstream.com/video/61b6b896-af83-4a6f-b7b0-0d7f968cf789).
  I went over the end of the cache blocking for matrix-matrix
  multiplication again, and then we looked briefly at the performance
  we obtained on Hamilton. We then looked at how we can convince our
  compiler to generate the code we know is right. As mentioned, in my
  experience, of Intel, GCC, and Clang, the Intel compiler is the best
  in terms of optimisation reports. Flags for Intel are in the slides.
  For GCC, we can get some reports with `-fopt-info` (see [this
  page](https://gcc.gnu.org/onlinedocs/gcc/Developer-Options.html) and
  search for `-fopt-info`). For Clang, we can get some reports with
  `-Rpass=vec` and `-Rpass-missed=vec` (see [this
  page](https://clang.llvm.org/docs/UsersManual.html#options-to-emit-optimization-reports)).
- [Session 7]({{< static-ref "lecture-slides/2020-21/07.pdf" >}}), I also annotated one page from
  the [BLIS paper]({{< static-ref
  "lecture-slides/2020-21/vanzee2015-blis" >}}.pdf), [video](https://web.microsoftstream.com/video/994cf8c0-6403-48a0-85fd-b0d6cec9bc62).

  The two articles I took examples from for the dimension-lifted
  transposition approach are [Henretty et al. (2011), _Data Layout
  Transformation for Stencil Computations on Short-Vector SIMD
  Architectures_](https://web.cs.ucla.edu/~pouchet/doc/cc-article.11.pdf),
  and [Boyle et al. (2015), _Grid: A next generation data parallel C++
  QCD library_](https://arxiv.org/pdf/1512.03487.pdf). One example of
  the same approach applied to unstructured problems is shown in [Sun
  et al. (2020), _A study of vectorization for matrix-free finite
  element methods_](https://arxiv.org/pdf/1903.08243.pdf).
- [Session 8]({{< static-ref "lecture-slides/2020-21/08.pdf" >}}),
  [video](https://web.microsoftstream.com/video/1bc1a757-9a7a-4b08-b88b-5ef523819a0c)

  We did a walkthrough of doing a roofline analysis. I got a bit
  confused with the data in the plots from the slides, so we did it
  live with the BLIS GEMM implementation (see [exercise 9]({{< ref
  "exercise09.md" >}})). Those are the drawn-in points in the
  annotated slides.

  I then looked at application of some of the ideas in the course to
  the [Firedrake](https://www.firedrakeproject.org/) finite element
  system. We did the dimension-lifted transposition approach on
  unstructured data (rather than the structured grids that we looked
  at last time).

  Finally we tried to build the coursework code to check that it will
  work when you do it. Only to find it didn't, so I updated the PETSc
  install on Hamilton and rebuilt things, so I hope that the same
  approach should work for you. Please get in touch if it doesn't (or
  there are things that you don't understand).
