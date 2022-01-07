---
title: Introduction
draft: false
weight: 1
---

# COMP52315: Performance Engineering

This is the course webpage for the Performance Engineering part of
[COMP52315]({{< modulepage >}}). It collects the exercises, syllabus,
and notes. The source repository is [hosted on GitHub]({{< repo >}}).

The primary goal of the course is to equip you with tools and
techniques to answer the question

{{< question >}}

Given some code, which I would like to run faster, how do I know
_what_ to do?

{{< /question >}}

This is a large and open-ended question, so we will focus on a subset
of all the possible approaches. The philosophy is that of treating the
computer, and the code we run on it, as an _experimental system_. Our
goal is to

1. Perform measurements (of performance) on this sytems;
2. Construct _models_ that explain the behaviour;
3. Use these models to determine, and then apply, appropriate
   optimisations.

## Course Organisation

The course will run over four weeks starting on 10th January 2022.
Each week there will be two sessions in MCS2094, scheduled at 9am
on Mondays and 4pm on Tuesdays (UK time).

{{< hint warning >}}

In the first week of term (week beginning 10th January), the lectures
are **online only**.

{{< /hint >}}


{{< hint info >}}

You can attend remotely over [zoom](https://durhamuniversity.zoom.us/j/95150134609?pwd=eW5hTXRsZlQ0cnlBWlA0Z0F6Yjh4dz09), and will need to be
authenticated with your Durham account.

Meeting ID: 951 5013 4609  
Passcode: 166174

{{< /hint >}}

There are a bunch of exercises which we'll mostly work on in the live
sessions, their aim is to familiarise you with the tools we'll use for
performance measurements and modelling throughout the course.

As well as the listed exercises, the notes also contain shorter
exercises which you should attempt. We can discuss them in the live
sessions.

{{< exercise >}}
Exercises look like this.
{{< /exercise >}}

## Syllabus

- Fundamentals of performance engineering
- Tools: CPU topology and affinity
- The Roofline performance model
- Tools: Performance counters
- Technique: Vectorisation (SIMD programming)
- Technique: Data layout transformations

[Slides]({{< ref "slides.md" >}}) will appear before the sessions, and
the annotated versions will appear after the sessions. The long form
notes run over much of the same ground, but with more words.

## Lecturer

- [Lawrence Mitchell](mailto:lawrence.mitchell@durham.ac.uk)
