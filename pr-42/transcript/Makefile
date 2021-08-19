SUBDIR=	src

# transcript/Makefile.bsd
#
# Copyright 1985,1987 (C) Adobe Systems Incorporated. All rights reserved.
# GOVERNMENT END USERS: See notice of rights in Notice file in release directory.
# RCSID: $Header: Makefile.bsd,v 2.2 87/11/17 16:53:10 byron Rel $
#
# to install transcript:
#
#	(Once for your system)
#		bsd
#		edit config & printer
#		make programs
#
#		(become super-user)
#		make install		( install files in system dirs )
#
#	(For each PostScript/TranScript printer)
#		cd etc
#		(become super-user)
#		mkprinter PRINTER TTY
#

all programs:
	. ./config; cd lib; make ${MFLAGS} programs
	. ./config; cd man; make ${MFLAGS} programs
	. ./config; cd sh;  make ${MFLAGS} programs
	. ./config; cd src; \
		rm -f Makefile; \
		sed -e s,SETCFLAGS,"$$SETCFLAGS",g <Make.proto >Makefile; \
		make ${MFLAGS} programs

explain:
	@cat doc/make.notes

install: all
	-. ./config; rm -rf $(DSTROOT)/$$PSLIBDIR.old
#	-. ./config; mv $(DSTROOT)/$$PSLIBDIR $(DSTROOT)/$$PSLIBDIR.old
	-. ./config; mkdirs $(DSTROOT)/$$PSLIBDIR; \
	    chown $$OWNER $(DSTROOT)/$$PSLIBDIR; \
	    chgrp $$GROUP $(DSTROOT)/$$PSLIBDIR; \
	    chmod 755 $(DSTROOT)/$$PSLIBDIR
	. ./config; cd src; make ${MFLAGS} install DSTROOT=$(DSTROOT)
	. ./config; cd sh;  make ${MFLAGS} install DSTROOT=$(DSTROOT)
	. ./config; BLDDIR=${DSTROOT}; export BLDDIR; \
	    cd lib; make ${MFLAGS} install DSTROOT=$(DSTROOT)
#	. ./config; cd man; make ${MFLAGS} install DSTROOT=$(DSTROOT)

clean:
	rm -f *BAK *CKP .emacs_[0-9]* #*# *~
	cd etc; make ${MFLAGS} clean
	cd lib; make ${MFLAGS} clean
	cd man; make ${MFLAGS} clean
	cd  sh; make ${MFLAGS} clean
	cd src; make ${MFLAGS} clean
