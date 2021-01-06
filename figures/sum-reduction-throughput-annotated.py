import argparse

import numpy
from matplotlib import pyplot

parser = argparse.ArgumentParser()
parser.add_argument("output", type=str)

args, _ = parser.parse_known_args()

L1_PEAK = 2.9 * 8 * 1e3
L2_PEAK = 2.9 * 8 * 27/32 * 1e3
L3_PEAK = 2.9 * 8 * 12/32 * 1e3
RAM_PEAK = 2.9 * 8 * 4.5/32 * 1e3

simd = numpy.asarray([
    [1e3, 20934.77],
    [2e3, 23240.25],
    [4e3, 22471.22],
    [8e3, 21988.17],
    [16e3, 22216.43],
    [32e3, 21881.05],
    [64e3, 17130.67],
    [96e3, 17975.12],
    [128e3, 17291.29],
    [160e3, 17507.30],
    [192e3, 14476.12],
    [224e3, 12656.81],
    [256e3, 11854.84],
    [512e3, 8585.19],
    [1e6, 8607.97],
    [2e6, 8609.45],
    [4e6, 8615.55],
    [8e6, 8622.28],
    [16e6, 8624.76],
    [32e6, 6814.45],
    [64e6, 3190.88],
    [128e6, 3153.69]])

scalar = numpy.asarray([
    [1e3, 2913.70],
    [2e3, 2893.67],
    [4e3, 2894.32],
    [8e3, 2894.57],
    [16e3, 2894.35],
    [32e3, 2889.47],
    [64e3, 2889.47],
    [128e3, 2891.86],
    [256e3, 2891.90],
    [512e3, 2887.25],
    [1e6, 2889.04],
    [2e6, 2888.07],
    [4e6, 2892.18],
    [8e6, 2892.26],
    [16e6, 2892.14],
    [32e6, 2651.69],
    [64e6, 2449.17],
    [128e6, 2445.51]])


fig, axes = pyplot.subplots(1)

axes.plot(simd[:, 0], simd[:, 1], "o-", label="SIMD (AVX)")

axes.axhline(L1_PEAK, color="r", linestyle="--", label="L1 Peak")
axes.axhline(L2_PEAK, color="black", linestyle=":", label="L2 Peak")
axes.axhline(L3_PEAK, color="magenta", linestyle="-", label="L3 Peak")
axes.axhline(RAM_PEAK, color="brown", linestyle="-.", label="RAM Peak")

axes.set_xscale("log", base=2)
axes.set_xlabel("Vector size [Bytes]")
axes.set_ylabel("Performance [MFlops/s]")
axes.legend(loc="best")

fig.savefig(args.output,
            orientation="landscape",
            transparent=True,
            bbox_inches="tight")
