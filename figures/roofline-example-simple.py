import argparse

import numpy
from matplotlib import pyplot

parser = argparse.ArgumentParser()
parser.add_argument("output", type=str)

args, _ = parser.parse_known_args()

FONTSIZE = 16
MARKERSIZE = 12

# PEAK_BW = 119.4 * (num_processes // 24)      # GB/s
STREAM_TRIAD = 11.6  # GB/s
PEAK_FLOPS = 46.4  # GFLOP/s


fig = pyplot.figure(figsize=(9, 5), frameon=False)
ax = fig.add_subplot(111)
ax.set_title("Hamilton node single-core roofline", fontsize=FONTSIZE)
ax.set_xscale("log", base=2)
ax.set_yscale("log", base=2)

ax.set_xlabel("Arithmetic intensity [FLOPs/byte]", fontsize=FONTSIZE)
ax.set_ylabel("Double precision GFLOPs/s", fontsize=FONTSIZE)

ax.set_xlim([2 ** -6, 2 ** 8])
ax.set_ylim([2 ** -3, 2 ** 7])


def add_roofline(BW, FLOPS):
    xes = [2 ** n for n in range(-6, 9)]
    xes = numpy.insert(xes, numpy.searchsorted(xes, FLOPS / BW), FLOPS / BW)
    yes = [min(FLOPS, BW * x) for x in xes]
    ax.plot(xes, yes, linewidth=2, color="grey", zorder=1)


for i in [1, 16]:
    add_roofline(STREAM_TRIAD, PEAK_FLOPS / i)

props = dict(facecolor="white", alpha=0.5, edgecolor="white")

ax.text(
    2 ** -2.8,
    STREAM_TRIAD * 2 ** -2.3,
    "Triad BW %.0f GB/s" % (STREAM_TRIAD),
    horizontalalignment="left",
    rotation=37,
    verticalalignment="bottom",
    bbox=props,
    zorder=2,
)

ax.text(
    2 ** 7.5,
    PEAK_FLOPS * 1.15,
    "Best case vectorised: %.0f Gflop/s" % PEAK_FLOPS,
    horizontalalignment="right",
    verticalalignment="bottom",
    bbox=props,
    zorder=2,
)

ax.text(
    2 ** 7.5,
    PEAK_FLOPS * 1.15 / 16,
    "Scalar code: %.0f Gflop/s" % (PEAK_FLOPS / 16),
    horizontalalignment="right",
    verticalalignment="bottom",
    bbox=props,
    zorder=2,
)

ax.plot(2 / 32, 0.72, label="Code A", markersize=MARKERSIZE, marker="o")
ax.plot(2 ** 0, 5, label="Code B", markersize=MARKERSIZE, marker=">")
ax.plot(2 ** 4, 0.83, label="Code C", markersize=MARKERSIZE, marker="<")


handles, labels = ax.get_legend_handles_labels()

legend = fig.legend(
    handles,
    labels,
    loc=9,
    bbox_to_anchor=(0.5, 1.1),
    bbox_transform=fig.transFigure,
    ncol=2,
    handlelength=4,
    fontsize=FONTSIZE,
    numpoints=1,
    frameon=False,
)


fig.savefig(
    args.output,
    orientation="landscape",
    transparent=True,
    bbox_inches="tight",
    bbox_extra_artists=[legend],
)
