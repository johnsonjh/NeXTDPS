# transcript/man/Makefile
#
# Copyright 1985,1987 Adobe Systems Incorporated. All rights reserved.
# GOVERNMENT END USERS: See notice of rights in Notice file in release directory.
# RCSID: $Header: Makefile,v 2.2 87/11/17 16:46:29 byron Rel $

.SUFFIXES: .1 .1p .7 .7p .8 .8p

PAGES1 = transcript.1 enscript.1 ps4014.1 ps630.1 pscat.1 psdit.1 psplot.1 \
	psrev.1 psroff.1 ptroff.1 psfonts.1 pssymbols.1
PAGES7 = afm.7 postscript.7
PAGES8 = pscatmap.8 pscomm.8

PAGES = ${PAGES1} ${PAGES7} ${PAGES8}

programs all: ${PAGES}

.1p.1 .7p.7 .8p.8:
	sed	-e s,XPSLIBDIRX,$$PSLIBDIR,g \
		-e s,XTROFFFONTDIRX,$$TROFFFONTDIR,g \
		-e s,XTROFFFONTDIRX,$$TROFFFONTDIR,g \
		-e s,XPSTEMPDIR,$$PSTEMPDIR,g \
		-e s,XDITDIRX,$$DITDIR,g \
		$< > $@

install: ${PAGES}
	mkdirs $(DSTROOT)/$$MAN1 $(DSTROOT)/$$MAN7 $(DSTROOT)/$$MAN8
	test -d $(DSTROOT)/$$MAN1
	cd $(DSTROOT)/$$MAN1; rm -f ${PAGES1}
	./installman 1 $(DSTROOT)/$$MAN1
	test -d $(DSTROOT)/$$MAN7
	cd $(DSTROOT)/$$MAN7; rm -f ${PAGES7}
	./installman 7 $(DSTROOT)/$$MAN7
	test -d $(DSTROOT)/$$MAN8
	cd $(DSTROOT)/$$MAN8; rm -f ${PAGES8}
	./installman 8 $(DSTROOT)/$$MAN8

clean:
	rm -f - ${PAGES} *.BAK *.CKP .emacs_[0-9]*
