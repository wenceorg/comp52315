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

ax.set_xlim([2 ** -4, 2 ** 0])
ax.set_ylim([2 ** -3, 2 ** 3])


def add_roofline(BW, FLOPS):
    xes = [2 ** n for n in range(-6, 9)]
    xes = numpy.insert(xes, numpy.searchsorted(xes, FLOPS / BW), FLOPS / BW)
    yes = [min(FLOPS, BW * x) for x in xes]
    ax.plot(xes, yes, linewidth=2, color="grey", zorder=1)


for i in [1, 2, 4, 16]:
    add_roofline(STREAM_TRIAD, PEAK_FLOPS / i)

props = dict(facecolor="white", alpha=0.5, edgecolor="white")

ax.text(
    2 ** -2.8,
    STREAM_TRIAD * 2 ** -2.3,
    "Triad BW %.0f GB/s" % (STREAM_TRIAD),
    horizontalalignment="left",
    rotation=20,
    verticalalignment="bottom",
    bbox=props,
    zorder=2,
)

ax.text(
    2 ** 7.5,
    PEAK_FLOPS * 1.15,
    "2 AVX FMA/cycle: %.0f Gflop/s" % PEAK_FLOPS,
    horizontalalignment="right",
    verticalalignment="bottom",
    bbox=props,
    zorder=2,
)

ax.text(
    2 ** 7.5,
    PEAK_FLOPS * 1.15 / 2,
    "1 AVX FMA/cycle %.0f Gflop/s" % (PEAK_FLOPS / 2),
    horizontalalignment="right",
    verticalalignment="bottom",
    bbox=props,
    zorder=2,
)

ax.text(
    2 ** 7.5,
    PEAK_FLOPS * 1.15 / 4,
    "1 AVX MUL or 1 AVX ADD/cycle %.0f Gflop/s" % (PEAK_FLOPS / 4),
    horizontalalignment="right",
    verticalalignment="bottom",
    bbox=props,
    zorder=2,
)

ax.text(
    2 ** 0,
    PEAK_FLOPS * 1.15 / 16,
    "1 scalar op/cycle %.0f Gflop/s" % (PEAK_FLOPS / 16),
    horizontalalignment="right",
    verticalalignment="bottom",
    bbox=props,
    zorder=2,
)


def ai(nrows, ncols):
    return 2 * ncols * nrows / (8 * (2 * ncols + nrows + nrows * ncols))


O0data = numpy.asarray(
    [
        [1000, 10000, 587.24],
        [2500, 10000, 587.02],
        [5000, 10000, 578.34],
        [10000, 10000, 576.25],
        [25000, 10000, 583.59],
        [50000, 10000, 587.02],
        [100000, 10000, 585.00],
    ]
)

O3data = numpy.asarray(
    [
        [1000, 10000, 3236],
        [2500, 10000, 2828],
        [5000, 10000, 2804],
        [10000, 10000, 2815],
        [25000, 10000, 1964],
        [50000, 10000, 1949],
        [100000, 10000, 1949],
    ]
)


O3datablocked = numpy.asarray(
    [
        [1000, 10000, 3157],
        [2500, 10000, 2848],
        [5000, 10000, 2909],
        [10000, 10000, 2808],
        [25000, 10000, 2808],
        [50000, 10000, 2838],
        [100000, 10000, 2870],
    ]
)

aidata = numpy.asarray(list(ai(r, c) for (r, c, _) in O0data))
ax.plot(
    aidata,
    O0data[:, 2] * 1e-3,
    label="O0",
    markersize=MARKERSIZE,
    marker="o",
    linestyle="none",
)
ax.plot(
    aidata,
    O3data[:, 2] * 1e-3,
    label="O3",
    markersize=MARKERSIZE,
    marker=">",
    linestyle="none",
)
ax.plot(
    aidata,
    O3datablocked[:, 2] * 1e-3,
    label="O3 blocked",
    markersize=MARKERSIZE,
    marker=">",
    linestyle="none",
)


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
