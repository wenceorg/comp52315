import argparse

import numpy
from matplotlib import pyplot

parser = argparse.ArgumentParser()
parser.add_argument("output", type=str)

args, _ = parser.parse_known_args()

FONTSIZE = 16
MARKERSIZE = 12

STREAM_TRIAD = 12  # GB/s
PEAK_FLOPS = 46.4  # GFLOP/s


fig = pyplot.figure(figsize=(9, 5), frameon=False)
ax = fig.add_subplot(111)
ax.set_xscale("log", base=2)
ax.set_yscale("log", base=2)

ax.set_xlabel("Arithmetic intensity [FLOPs/byte]", fontsize=FONTSIZE)
ax.set_ylabel("Double precision GFLOPs/s", fontsize=FONTSIZE)

ax.set_xlim([2 ** -4, 2 ** 7])
ax.set_ylim([2 ** -3, 2 ** 6])


def add_roofline(BW, FLOPS):
    xes = [2 ** n for n in range(-4, 8)]
    xes = numpy.insert(xes, numpy.searchsorted(xes, FLOPS / BW), FLOPS / BW)
    yes = [min(FLOPS, BW * x) for x in xes]
    ax.plot(xes, yes, linewidth=2, color="grey", zorder=1)


perfect_cache_data = 8 * 3 * 1000 ** 2
flops = 2 * 1000 ** 3
pessimal_cache_data = 8 * 3 * 1000 ** 3

orig = 1.831
add_roofline(STREAM_TRIAD, PEAK_FLOPS)

ax.plot(
    21.32, orig, label="Original", markersize=MARKERSIZE, marker="o", linestyle="none"
)

ax.plot(
    flops / perfect_cache_data,
    orig,
    label="Perfect cache (original)",
    markersize=MARKERSIZE,
    marker=">",
    linestyle="none",
)

just_tiles = 7.6
ax.plot(
    27, just_tiles, label="Tiling", markersize=MARKERSIZE, marker="o", linestyle="none"
)
ax.plot(
    flops / perfect_cache_data,
    just_tiles,
    label="Perfect cache (tiling)",
    markersize=MARKERSIZE,
    marker=">",
    linestyle="none",
)

tiling_vector = 19.5
ax.plot(
    38,
    tiling_vector,
    label="Tiling + vectorise",
    markersize=MARKERSIZE,
    marker="o",
    linestyle="none",
)
ax.plot(
    flops / perfect_cache_data,
    tiling_vector,
    label="Perfect cache (tiling + vectorise)",
    markersize=MARKERSIZE,
    marker=">",
    linestyle="none",
)

handles, labels = ax.get_legend_handles_labels()

legend = fig.legend(
    handles,
    labels,
    loc=9,
    bbox_to_anchor=(0.5, 1.2),
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
