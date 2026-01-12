NAME = manuelForestimatorV0
NAME_FINAL = DocForestimatorMobile

FTEX=$(NAME).tex\
	#forestimatorGlossaireEntry.tex\

$(NAME).pdf : $(FTEX)
	pdflatex $(NAME)
	makeglossaries $(NAME)
	#bibtex $(NAME)
	pdflatex $(NAME)
	pdflatex $(NAME)

$(NAME).dvi : $(FTEX)
	latex $(NAME)

clean :
	find . -iname "*.aux" -exec rm {} +
		gs -dPrinted=false -sDEVICE=pdfwrite -dCompatibilityLevel=1.5 -dPDFSETTINGS=/ebook -dDownsampleColorImages=true -dColorImageResolution=300 -dNOPAUSE -dQUIET -dBATCH -sOutputFile=foo.pdf $(NAME).pdf
	rm -f $(NAME).blg $(NAME).toc $(NAME).bbl $(NAME).out $(NAME).log $(NAME).pdf $(NAME).synctex.gz $(NAME).gl* $(NAME).ist
	mv foo.pdf $(NAME_FINAL).pdf
