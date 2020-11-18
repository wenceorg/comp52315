import argparse

import numpy
from matplotlib import pyplot

parser = argparse.ArgumentParser()
parser.add_argument("output", type=str)

args, _ = parser.parse_known_args()

FONTSIZE = 16
MARKERSIZE = 12

fig = pyplot.figure(figsize=(9, 5), frameon=False)
ax = fig.add_subplot(111)
ax.set_xlabel("Cores", fontsize=FONTSIZE)
ax.set_ylabel("Performance (arbitrary units)", fontsize=FONTSIZE)


data = numpy.asarray(
    [[1, 1.5], [2, 3], [3, 4.5], [4, 6], [5, 7.5], [6, 9], [7, 10.5], [8, 12],]
)

ax.plot(
    data[:, 0],
    data[:, 1],
    markersize=MARKERSIZE,
    marker="o",
    linestyle="-",
)

fig.savefig(
    args.output,
    orientation="landscape",
    transparent=True,
    bbox_inches="tight",
)
