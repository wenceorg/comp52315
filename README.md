This repository was updated until March 2022 when I left Durham University. The materials herein are therefore not necessarily still in date.

Source repository for
[COMP52315](https://www.dur.ac.uk/postgraduate.modules/module_description/?year=2020&module_code=COMP52315).

Go to https://teaching.wence.uk/comp52315/ for the hosted material.

The pages can be built with [hugo](https://gohugo.io). For generation
of figures you need the standalone version of
[diagrams.net](https://www.diagrams.net) as well as a Python
environment with [numpy](https://numpy.org) and
[matplotlib](https://matplotlib.org). If these are in place, `make
html` builds everything and puts it in `site/public`. In the `site`
subdirectory `hugo server` will launch a local server that autobuilds
the webpages (not figures) on change.
