.PHONY: all
.PHONY: figures
all: figures 01.pdf 02.pdf 03.pdf 04.pdf 05.pdf 06.pdf 07.pdf

%.pdf: %.tex
	latexmk -shell-escape -pdflua $<

figures:
	make -C .. allpdf
