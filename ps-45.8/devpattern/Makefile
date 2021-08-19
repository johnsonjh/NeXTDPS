ROOT = ..
LIBRARY = devpattern
SRCS = 	colorpattern.c constclrpat.c constgraypat.c graypattern.c \
	grypat4of4.c im110.c ims11d11.c ims11d12.c ims11d1x.c \
	ims12d12notfr.c ims1xd11.c ims1xd12.c imsupport.c \
	imsxxd11.c imsxxd12.c maskpattern.c monopattern.c patternimpl.c 

HDRS = 	imagepriv.h patternpriv.h

MISC = Makefile 

OTHERSWITCHES= -DCONTROLLER_TYPE=cont_generic

include $(ROOT)/makefile.common

default-goal: lib
