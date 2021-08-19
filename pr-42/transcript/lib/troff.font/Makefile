# transcript/lib/troff.font/Makefile.bsd
#
# Copyright (C) 1985,1987 Adobe Systems Incorporated. All rights reserved.
# GOVERNMENT END USERS: See notice of rights in Notice file in release directory.
# RCSID: $Header: Makefile.bsd,v 2.2 87/11/17 16:28:02 byron Rel $
#
# Makefile for troff width tables and pscat correspondence tables
#
# See pscatmap(8) and the .map files for more information.
#
# If you add a font family to your PostScript/troff database,
# add it to the FAMILIES line in this makefile, then do a
# "make" of the family.  For example:
#          make Bookman

########### FAMILIES

Times: Times.ct
Courier: Courier.ct
Helvetica: Helvetica.ct
HelvNarrow: HelvNarrow.ct
AvantGarde: AvantGarde.ct
Bookman: Bookman.ct
Garamond: Garamond.ct
Lubalin: Lubalin.ct
NewCentury: NewCentury.ct
Optima: Optima.ct
Palatino: Palatino.ct
Souvenir: Souvenir.ct
Zapf2: Zapf.ct

Standard13: Times Helvetica Courier

startup:
	test x"$$TROFFFONTS" = x1

.SUFFIXES: .ct .map

# This creates the directory and all the files.
.map.ct:
	-rm -rf $*
	mkdir $*
	echo "../pscatmap "$*".map"
	../pscatmap $*.map
	./makefontdir $*

clean:
	rm -f - *.ct *.c *.o ft? ft?? *BAK *CKP .emacs_[0-9]*
