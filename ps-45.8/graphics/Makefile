ROOT = ..
LIBRARY = graphics
SRCS = 	colorops1.c cscan.c euserpath.c fastfillquad.c fflattenpath.c \
	graphics.c graphmark.c graphpak.c gray.c graypriv.c gstates.c \
	hitdetect.c image.c maketrap.c matrix.c nulldevice.c pathbuild.c \
	pathextraops.c pathops.c pathpriv.c pathreduce.c qreducer.c rect.c \
	reducer.c stroke.c strokeops.c transferimpl.c ucache.c userpath.c \
	viewclip.c

HDRS = 	atm.h graphdata.h graphicsnames.h graphicspriv.h gray.h gstack.h \
	image.h path.h reducer.h stroke.h userpath.h viewclip.h 

MISC = Makefile 

OTHERSWITCHES = -DDPSONLY=1 -DMERCURY=0

include $(ROOT)/makefile.common

default-goal: lib
