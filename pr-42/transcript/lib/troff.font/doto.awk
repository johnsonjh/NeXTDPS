# transcript/lib/troff.font/doto.awk
#
# Copyright (C) 1985,1987 Adobe Systems Incorporated. All rights reserved.
#
# Gets used by the Makefile in this directory.
#
# This short "awk" program generates a list of font files by
# searching its input (a ".map" file) for the @FACENAMES line.
# This list is used as the argument string to a "make" so that
# the troff width tables can be built automatically from the ".map"
# file.

/^@FACENAMES /	{print "ft" $2, "ft" $3, "ft" $4, "ft" $5
		exit}
