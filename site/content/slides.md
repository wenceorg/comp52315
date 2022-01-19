---
title: "Slides"
weight: 4
---

# Slides 2021/22 edition

Slides for the live lectures. These will be augmented with annotated
versions, links to recordings, and some short commentary after the
fact. If you think you should have access to the recordings but don't,
please [get in touch]((mailto:lawrence.mitchell@durham.ac.uk)).

The long form notes add words in between the bullet points.

- [Session 1]({{< static-ref "lecture-slides/01.pdf" >}}),
  [annotated]({{< static-ref "lecture-slides/2021-22/01.pdf" >}}),
  [video](https://durham.cloud.panopto.eu/Panopto/Pages/Viewer.aspx?id=052e8585-00a0-444a-829a-ae1900b6870e).
  
  We introduced some ideas of computer architecture and talked about
  the motivation for the course. There is a focus on trying to build
  predictive models for the speed that code runs at.
  
  We finished by working through the [first exercise]({{< ref
  "exercise01.md" >}}) in the round. But did not quite finish. Have a
  go at [producing the plots]({{< ref "exercise01.md#vector-size" >}})
  the last part of the exercise requires of you. We will discuss the
  results in the next session
  
- [Session 2]({{< static-ref "lecture-slides/02.pdf" >}}),
  [annotated]({{< static-ref "lecture-slides/2021-22/02.pdf" >}}),
  [video](https://durham.cloud.panopto.eu/Panopto/Pages/Viewer.aspx?id=deba6d06-5908-49af-bf74-ae1a0143da57).
  
  I realised while we were doing the second exercise why I couldn't
  reproduce the plot from the slides. I started at 1024kB, rather than
  1kB. I should have looped from `seq 0 17` rather than `seq 10 17`.
  The corrected script to collect all the data we need is
  
  ```sh
  #!/bin/bash
  
  # 1 core
  #SBATCH -n 1
  #SBATCH --job-name="collect-bw"
  #SBATCH -o collect-bw.%J.out
  #SBATCH -e collect-bw.%J.err
  #SBATCH -t 00:20:00
  #SBATCH -p par7.q
  
  source /etc/profile.d/modules.sh
  
  module load likwid/5.0.1
  
  for n in $(seq 0 17)
  do
      size=$((2 ** n))
      mflops_scalar=$(likwid-bench -t sum_sp -w S0:${size}kB:1 2>/dev/null | grep MFlops/s | cut -f 3)
      mflops_avx=$(likwid-bench -t sum_sp_avx -w S0:${size}kB:1 2>/dev/null | grep MFlops/s | cut -f 3)
      echo $size $mflops_scalar $mflops_avx
  done
  ```
  
  We didn't quite finish all of the slides, so we'll pick up where we
  left off and use the results of the second exercise to build a
  predictive model for the performance of the vectorised sum reduction
  at the beginning of the next session.
  
- [Session 3]({{< static-ref "lecture-slides/03.pdf" >}}),
  [annotated]({{< static-ref "lecture-slides/2021-22/03.pdf" >}}),
  [video](https://durham.cloud.panopto.eu/Panopto/Pages/Viewer.aspx?id=4f7435e0-1b80-4bd6-a9a3-ae2000c78ca7).
  
  We used the results of our benchmarking of the cache hierarchy to
  construct a model for how fast the sum reductions should run as a
  function of the vector size. It works pretty well! This was the end
  of slides from session 2, I have updated the annotated version
  above. Then I talked, probably for a bit long (sorry), about memory
  bandwidth, resource restrictions and some philosophy of how to go
  about thinking about optimising code. 
  
  We finished by talking about the [roofline
  model](https://en.wikipedia.org/wiki/Roofline_model),
  introduced in [Williams, Waterman, and Patterson
  (2009)](https://people.eecs.berkeley.edu/~kubitron/cs252/handouts/papers/RooflineVyNoYellow.pdf).
  Please read this paper before the session tomorrow and note any
  questions or discussion points you might have on it, we'll read
  through the paper in class and discuss it further then.

- [Session 4]({{< static-ref "lecture-slides/04.pdf" >}}),
  [annotated]({{< static-ref "lecture-slides/2021-22/04.pdf" >}}),
  [annotated roofline paper]({{< static-ref
  "lecture-slides/2021-22/williams2009-roofline.pdf" >}}), [video](https://durham.cloud.panopto.eu/Panopto/Pages/Viewer.aspx?id=a5d68405-a795-42c3-bc7b-ae21014beb02)
  
  We spent the first half of the session going over the roofline paper
  and pointing out some key ideas.
  
  Then I started talking about performance counters and how to access
  them. We finished by trying to confirm our hypotheses about some
  simple stream code and how many loads and stores we would observe.
  We didn't manage to do so in all cases, so we'll try and figure
  things out next time.

- [Session 5]({{< static-ref "lecture-slides/05.pdf" >}})
- [Session 6]({{< static-ref "lecture-slides/06.pdf" >}})
- Session 7 -- coming soon
- Session 8 -- coming soon
