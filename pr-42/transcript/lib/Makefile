# transcript/lib/Makefile.bsd
#
# Copyright (c) 1985 Adobe Systems Incorporated. All Rights Reserved.
# GOVERNMENT END USERS: See notice of rights in Notice file in release directory.
# RCSID: $Header: Makefile.bsd,v 2.2 87/11/17 16:35:30 byron Rel $

FILES = banner.pro enscript.pro pstext.pro \
	ps4014.pro ps630.pro pscat.pro psplot.pro psdit.pro \
	ehandler.ps uartpatch.ps bogusmsg.ps \
	font.map Notice

PRG = psint.sh psbad.sh

programs all:
	rm -f psint.sh
	sed -e s,XPSLIBDIRX,$$PSLIBDIR,g \
	    -e s,XPSTEMPDIRX,$$PSTEMPDIR,g \
	    -e s,XBANNERFIRSTX,$$BANNERFIRST,g \
	    -e s,XBANNERLASTX,$$BANNERLAST,g \
	    -e s,XREVERSEX,$$REVERSE,g \
	    -e s,XVERBOSELOGX,$$VERBOSELOG,g \
		psint.proto >psint.sh

install: 
	mkdirs $(DSTROOT)/$$PSLIBDIR
	cd $(DSTROOT)/$$PSLIBDIR; rm -rf - ${FILES} ${PRG} ps?f psbad *.afm 
	cp ${FILES} ${PRG} *.afm $(DSTROOT)/$$PSLIBDIR
	cd $(DSTROOT)/$$PSLIBDIR; chown $$OWNER ${FILES} ${PRG} *.afm; \
		chgrp $$GROUP ${FILES} ${PRG} *.afm; \
		chmod 644 ${FILES} *.afm; \
		chmod 755 ${PRG}
	cd $(DSTROOT)/$$PSLIBDIR; \
		ln psint.sh psif; ln psint.sh psof; ln psint.sh psnf; \
		ln psint.sh pstf; ln psint.sh psgf; ln psint.sh psvf; \
		ln psint.sh psdf; ln psint.sh pscf; ln psint.sh psrf; \
		ln psbad.sh psbad
	-rm -rf $(DSTROOT)/$$PSLIBDIR/troff.font
	-rm -rf $(DSTROOT)/$$PSLIBDIR/ditroff.font
	-mkdirs $(DSTROOT)/$$PSLIBDIR/troff.font
	cp troff.font/* $(DSTROOT)/$$PSLIBDIR/troff.font
	chmod 555 $(DSTROOT)/$$PSLIBDIR/troff.font/makefontdir*
	-cd $(DSTROOT)/$$PSLIBDIR/troff.font; \
		make ${MFLAGS} startup $$FONTFAMILIES
	-cd $(DSTROOT)/$$PSLIBDIR/troff.font; \
		chown $$OWNER . * ; \
		chgrp $$GROUP . * ; \
		chmod a+r . *
	-mkdirs $(DSTROOT)/$$PSLIBDIR/ditroff.font
	cp ditroff.font/* $(DSTROOT)/$$PSLIBDIR/ditroff.font
#	-cd $(DSTROOT)/$$PSLIBDIR/ditroff.font; \
#		make ${MFLAGS} startup $$FONTFAMILIES install
#	-cd $(DSTROOT)/$$PSLIBDIR/ditroff.font; \
#		make ${MFLAGS} -f Make.family doit $(DSTROOT)/$$FONTFAMILIES
	-cd $(DSTROOT)/$$PSLIBDIR/ditroff.font; \
		chown $$OWNER . * ; \
		chgrp $$GROUP . * ; \
		chmod a+r . *

clean:
	rm -f - *.BAK *.CKP .emacs_[0-9]*
	cd troff.font; make ${MFLAGS} clean
	cd ditroff.font; make ${MFLAGS} clean
