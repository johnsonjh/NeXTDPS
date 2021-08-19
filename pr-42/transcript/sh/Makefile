# transcript/sh/Makefile
#
# Copyright 1985,1987 Adobe Systems Incorporated. All rights reserved.
# GOVERNMENT END USERS: See notice of rights in Notice file in release directory.
# RCSID: $Header: Makefile,v 2.2 87/11/17 16:48:32 byron Rel $

programs all: ptroff psroff

ptroff: ptroff.sh
	rm -f ptroff
	sed	-e s,TROFFFONTDIR,$$TROFFFONTDIR,g \
		ptroff.sh >ptroff

psroff: psroff.sh
	rm -f psroff
	-if test x$$DWB20 = x1; then \
	sed	-e s,DITDIR,$$DITDIR,g \
		-e s,DITFLAGS,,g \
		psroff.sh >psroff; else \
	sed	-e s,DITDIR,$$DITDIR,g \
		-e s,DITFLAGS,-t,g \
		psroff.sh >psroff; fi

install: psroff ptroff
	mkdirs $(DSTROOT)/$$BINDIR
	cp ptroff psroff $(DSTROOT)/$$BINDIR
	-chown $$OWNER $(DSTROOT)/$$BINDIR/ptroff $(DSTROOT)/$$BINDIR/psroff
	-chgrp $$GROUP $(DSTROOT)/$$BINDIR/ptroff $(DSTROOT)/$$BINDIR/psroff
	chmod 755 $(DSTROOT)/$$BINDIR/ptroff $(DSTROOT)/$$BINDIR/psroff

clean:
	-rm -f - ptroff psroff *.BAK *.CKP .emacs_[0-9]*
