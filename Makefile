pyfigures = $(wildcard figures/*.py)
drawiofigures = $(wildcard figures/*.drawio)

drawiopng = $(patsubst %.drawio,%.png,$(drawiofigures))
drawiosvg = $(patsubst %.drawio,%.svg,$(drawiofigures))
drawiopdf = $(patsubst %.drawio,%.pdf,$(drawiofigures))
pypng = $(patsubst %.py,%.png,$(pyfigures))
pysvg = $(patsubst %.py,%.svg,$(pyfigures))
pypdf = $(patsubst %.py,%.pdf,$(pyfigures))

.PHONY: html pypng 

html: allcode pysvg
	(cd site; hugo --minify --cleanDestinationDir)

allslides: allfigures

site/static/images:
	mkdir -p $@

pypng: site/static/images $(pypng)
	rsync --delete -rupm figures/ site/static/images/auto/ --filter '+ */' --filter '+ *.png' --filter '- *'

drawiopng: site/static/images $(drawiopng)
	rsync --delete -rupm figures/ site/static/images/manual/ --filter '+ */' --filter '+ *.png' --filter '- *'

pysvg: site/static/images $(pysvg)
	rsync --delete -rupm figures/ site/static/images/auto/ --filter '+ */' --filter '+ *.svg' --filter '- *'

drawiosvg: site/static/images $(drawiosvg)
	rsync --delete -rupm figures/ site/static/images/manual/ --filter '+ */' --filter '+ *.svg' --filter '- *'

allpdf: $(drawiopdf) $(pypdf)

figures/%.png: figures/%.drawio
	drawio -s 2 -t -f png -x --crop -o $@ $<

figures/%.svg: figures/%.drawio
	drawio -s 2 -t -f svg -x --crop -o $@ $<

figures/%.pdf: figures/%.drawio
	drawio -f pdf -x --crop -o $@ $<

figures/%.png: figures/%.py
	python $< $@

figures/%.svg: figures/%.py
	python $< $@

figures/%.pdf: figures/%.py
	python $< $@

allcode:
	rsync --delete -rupm code/ site/static/code/ --filter '+ */' --filter '+ *.c' --filter '+ *.h' --filter '- *'
