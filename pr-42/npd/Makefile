#
# Generic top-level makefile for multi-file commands
# (c) NeXT, Inc.  1987
#
# makefile for npd
#

ROOT=root
DAEMON=daemon
SPGRP=daemon
  
UFLAGS =
#DEFINES= -DDEBUG -DNeXT_MOD -DNeXT_NFS
DEFINES= -DNeXT_MOD -DNeXT_NFS
CFLAGS = -g -O -Wimplicit ${DEFINES} ${UFLAGS}
IFLAGS= -s -o ${ROOT} -g ${SPGRP} -m 6755

LIBS = -lNeXT_s -lsys_s
#LIBS =  -L/usr/local/src/debug/MallocDebug -lMallocDebug -lNeXT_s -lsys_s

BINDIR=	/usr/lib/NextPrinter

PRODUCT= npd

DSTDIRS= ${DSTROOT}${BINDIR}

# Source files.
# All these files will be installed with "make installsrc"

HFILES= log.h mach_ipc.h netinfo.h npd_prot.h protocol1.h atomopen.h paper.h \
	Daemon.h DocInfo.h LocalJob.h NextPrinter.h \
	NpdDaemon.h PSDevice.h PrintJob.h PrintSpooler.h \
	SpoolingJob.h StringManager.h
CFILES= mach_ipc.c atomopen.c
MFILES= log.m netinfo.m protocol1.m \
	Daemon.m DocInfo.m LocalJob.m NextPrinter.m \
	NpdDaemon.m PSDevice.m PrintJob.m PrintSpooler.m \
	SpoolingJob.m StringManager.m
PSWFILES = 
AUX_FILES = Makefile banner.pro
DERIVED = 

SRCS=	${HFILES} ${CFILES} ${MFILES} ${PSWFILES}

GARBAGE= TAGS tags *_vers.c

all:	${PRODUCT}

#
# Building npd
#

NPD_OBJS = ${CFILES:.c=.o} ${MFILES:.m=.o} ${PSWFILES:.psw=.o} npd_vers.o
${PRODUCT}: .depend_done ${NPD_OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ ${NPD_OBJS} ${LIBS}

npd_vers.c: $(NPD_OBJS)
	-rm -f $@
	vers_string -c npd > $@

clean:	ALWAYS
	-rm -f *.o
	-rm -f ${PRODUCT} ${DERIVED} ${INTERMEDIATES} ${GARBAGE}
	-rm -f Makedep .depend_done
	-rm -f *~
	-rm -f #*#
	touch Makedep

install: DSTROOT ${DSTDIRS} all
	mkdirs $(DSTROOT)$(BINDIR)
	install ${IFLAGS} npd ${DSTROOT}${BINDIR}/npd
	install -o ${ROOT} -g ${SPGRP} -m 644 banner.pro \
	  ${DSTROOT}${BINDIR}/banner.pro

installsrc: SRCROOT
	-rm -rf ${SRCROOT}/npd
	mkdirs ${SRCROOT}/npd
	cp $(SRCS) ${SRCROOT}/npd
	cp $(AUX_FILES) $(SRCROOT)/npd
	touch $(SRCROOT)/npd/Makedep

DSTROOT LCLROOT SRCROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

${DSTDIRS}:
	mkdirs $@

depend .depend_done: ${DERIVED} Makefile
	rm -f Makedep .depend_done
	${CC} -M ${DEFINES} ${CFILES} ${MFILES} ${SFILES} ${DERIVED} | \
	    awk '{ if ($$1 != prev) { if (rec != "") print rec; \
		rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec }' >Makedep
	-touch .depend_done


tags:	ALWAYS
	etags ${SRCS}
	ctags ${SRCS}

ALWAYS:

include Makedep

