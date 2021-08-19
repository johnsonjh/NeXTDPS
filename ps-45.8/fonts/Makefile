ROOT = ..
LIBRARY = 	fonts
SRCS = 	cachedev.c cacheimpl.c cicache.c compshow.c fastshow.c flex.c \
	fontbuild.c fontcache.c fontdisk.c fontmatrix.c fontrun.c fontshow.c \
	glbclr.c midcache.c selectfont.c

DEVELOP_SRCS = debug.c

HDRS = 	atm.h cache.h cacheimpl.h diskcache.h fontbuild.h fontcache.h \
	fontdata.h fontrun.h fontshow.h fontsnames.h fontspriv.h \
	glbclr.h	

MISC = Makefile 

OTHERSWITCHES = -DMERCURY=0

include $(ROOT)/makefile.common

default-goal: lib

ops_fonts1.c ops_fonts2.c ops_fontsInternal.c : fontsnames.h
	$(ROOT)/bin/update_registered_names $(LIBRARY)names.h
