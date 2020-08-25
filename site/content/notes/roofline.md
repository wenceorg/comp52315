---
title: "Performance models: roofline"
weight: 3
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

{{< manfig src="optimisationworkflow.png"
    width="30%"
    caption="Simplified flow diagram for deciding on next steps when optimising code" >}}

The idea is that we decide that the time to solution is too long, and
must therefore optimise the code. We _profile_ the code to determine
where it spends all (or most) of its time, and then construct a model
that explains that time. With a model in hand, we can make a
prediction about the best optimisation to apply. 

This allows us to focus our optimisation efforts where they will be
most effective.

To do this, we need to construct some models, we'll see a number of
approaches in this course, the first one we'll consider is the
[_roofline model_](https://doi.org/10.1145/1498765.1498785). This is a
simple model for loopy heavy code.

## Roofline model


