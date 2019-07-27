# This Makefile requires modern GNU Make

all: ../README.md

.PHONY: all pdf

../README.md: Makefile readme-body.md online-badges.md toc.md
	sed -e '/@TOC@/{r toc.md' -e 'd}' -e '/@BADGES@/{r online-badges.md' -e 'd}' readme-body.md > $@

toc.md: Makefile readme-body.md toc.sh
	./toc.sh --skip `grep '@TOC@' -nh readme-body.md  | cut -d ':' -f 1` readme-body.md $@

###############################################################################

pdf: README.pdf

# apt install texlive texlive-latex-base texlive-latex-recommended texlive-fonts-recommended texlive-lang-cyrillic texlive-fonts-extra
#             pandoc cm-super texlive-fonts-extra-doc texlive-fonts-recommended-doc texlive-latex-base-doc texlive-latex-recommended-doc
#             texlive-pstricks texlive-extra-utils texlive-font-utils lmodern
README.pdf: Makefile readme-body.md toc.md
	sed -e '/@TOC@/{r toc.md' -e 'd}' -e '/@BADGES@/d' readme-body.md | \
	pandoc --pdf-engine=pdflatex --pdf-engine-opt=--shell-escape --variable=inputenc:utf8 --variable=fontenc:T2A \
	--variable=babel-lang:russian --variable=babel-otherlangs:english --variable=listings:mathtext,svg \
	-f gfm+smart -o $@
