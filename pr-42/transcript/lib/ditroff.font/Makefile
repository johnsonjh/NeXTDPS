
# lib/ditroff.font/Makefile.bsd
#
# Copyright (c) 1984,1985,1987 Adobe Systems Incorporated. All Rights Reserved.
# GOVERNMENT END USERS: See notice of rights in Notice file in release directory.
# RCSID: $Header: Makefile.bsd,v 2.2 87/11/17 16:42:59 byron Rel $

# see README for more information
# In general, initail installation should be done like:
#     make startup <list of fontfamilies> install
# makedev is the ditroff program that builds device descriptions

# DESCFONTS fonts are premounted by ditroff automatically.
DESCFONTS = R I B BI H HB C CB S SS

MOREFONTS = HO HD CO CD TR TI TB TD
STD13FONTS = ${DESCFONTS} ${MOREFONTS}
FONTS2 = lu lU Lu LU sv sV Sv SV op oP Op OP ga gA Ga GA
PLUSFONTS = ag aG Ag AG bo bO Bo BO hn hN Hn HN nc nC Nc NC PA PI PB PX ZC

FONTNAMES = ${STD13FONTS} ${FONTS2} ${PLUSFONTS}
FONTDIR = $$DITDIR/Times              # Default font family
DEVDIR = $$DITDIR/Times/devpsc        # Font files.  Used in makeout.

programs:

install: DESC.out
	-rm /usr/lib/font/devpsc      # Get rid of it if it's a link
	-mv /usr/lib/font/devpsc /usr/lib/font/devpsc.old
	ln -s ${DEVDIR} /usr/lib/font/devpsc    # Link to default directory
	-rm -rf DESC DESC.out temp.*
	cd ${DEVDIR} ; chown $$OWNER . * ; chgrp $$GROUP . * ; \
		chmod 755 . ; chmod 644 *.aux *.map *.out DESC

clean:
	-rm -rf ${FONTNAMES} core DESC *.out *.font *.aux *.CKP *.BAK .emacs_[0-9]* temp*

startup: DESC
	test x"$$DITROFFFONTS" = x1
	rm -rf ${FONTDIR}
	mkdir ${FONTDIR}
	mkdir ${DEVDIR}
	cp DESC ${DEVDIR}

DESC.out: DESC
	cd ${DEVDIR}; $$MAKEDEV DESC

DESC: afmdit.awk Makefile charset devspecs
	rm -f DESC
	echo "# ditroff device description for PostScript" >> DESC
	echo "# PostScript is a registered trademark of Adobe Systems Incorporated">>DESC
	echo ${DESCFONTS} | awk '{print "fonts", NF, $$0}' >> DESC
	cat devspecs >>DESC
	echo "charset" >> DESC
	cat charset >> DESC

###############  The following font family targets cause the corresponding
# font files to be built.  These are NOT the rules that build the font
# family directories.

Standard13:  R.out  I.out  B.out  BI.out Times Helvetica Courier S.out SS.out
Times:       TR.out TI.out TB.out TD.out
Courier:     C.out  CO.out CB.out CD.out
Helvetica:   H.out  HO.out HB.out HD.out
HelvNarrow:  hn.out hN.out Hn.out HN.out
AvantGarde:  ag.out aG.out Ag.out AG.out
Bookman:     bo.out bO.out Bo.out BO.out
Garamond:    ga.out gA.out Ga.out GA.out
Lubalin:     lu.out lU.out Lu.out LU.out
NewCentury:  nc.out nC.out Nc.out NC.out
Optima:      op.out oP.out Op.out OP.out
Palatino:    PA.out PI.out PB.out PX.out
Souvenir:    sv.out sV.out Sv.out SV.out

Zapf2:       ZC.out

############### Special rules for special fonts.

C.out:
	./makeout $* nolig
	cd ${DEVDIR}; ln C CW; ln C.aux CW.aux; ln C.map CW.map; ln C.out CW.out

CO.out CB.out CD.out:		# Get rid of ligatures.
	./makeout $* nolig

############### Default rules for building font files.

.SUFFIXES: .out .map

# NOTE: In order for makedev to run, the DESC text file must be available.
# The .out file, then, depends on the DESC file.  Currently, the startup
# target takes care of this.
.map.out:
	./makeout $*
