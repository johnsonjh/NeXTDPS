ROOT = ..
LIBRARY = mp
SRCS = 	copyline.c cursor.c funcline.c hlrect.c moverect.c \
	mp.c soverrect.c mp12.c mpconv12.c
ASRCS = copyrect.030.s
HDRS = 	devmark.h mp.h mp12.h
MISC = Makefile copyrect.c hlrect.030.s
OTHERSWITCHES = -DCONTROLLER_TYPE=cont_next1
include $(ROOT)/makefile.common

default-goal: lib

