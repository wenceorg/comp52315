---
title: Introduction
draft: false
weight: 1
---

# COMP52315: Performance Engineering

This is the course webpage for the Performance Engineering part of
[COMP52315]({{< modulepage >}}). It collects the exercises, syllabus, and notes. The source
repository is [hosted on GitHub]({{< repo >}}).

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
   

## Syllabus

Fundamentals of performance engineering

Tools: CPU topology and affinity

The Roofline performance model

Tools: Performance counters

Technique: Vectorisation (SIMD programming)

Technique: Data layout transformations
