ROOT = ..
LIBRARY = device
SRCS = 	blackmask.c blackrun.c clippedmask.c \
	constantmask.c constantrun.c constanttrap.c devbounds.c devcommon.c \
	devinit.c devprim.c framedev.c framemaskdev.c genericdev.c \
	graymask.c grayrun.c graytrap.c halftone.c hostdict.c \
	mark.c markimage.c maskcache.c nextprebuilt.c nulldev.c qintersect.c \
	tfrfcn.c trapprecision.c whitemask.c whiterun.c

ASRCS = blacktrap.030.s whitetrap.030.s

HDRS = 	devcommon.h devmark.h devprim.h framedev.h framemaskdev.h genericdev.h\
	hostdict.h maskcache.h nextprebuilt.h nulldev.h prebuiltformat.h 

MISC = Makefile 

OTHERSWITCHES= 	-DCONTROLLER_TYPE=cont_generic \
		-DSAMPLEDEVICE=1 \
		-DMASKCOMPRESSION=0 

include $(ROOT)/makefile.common

default-goal: lib
