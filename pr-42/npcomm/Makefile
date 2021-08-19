#
# Generic top-level makefile for multi-file commands
# (c) NeXT, Inc.  1987
#
# makefile for npcomm
#
# Ownerships {see note at the top of lpd.c}

ROOT=root
DAEMON=daemon
SPGRP=daemon
  

CFLAGS =  -g -O -DNeXT_MOD -DNeXT_NFS
IFLAGS= -s -o ${ROOT} -g ${SPGRP} -m 6755

LIBS = -lsys_s

BINDIR=	/usr/lib/NextPrinter

PRODUCTS= npcomm

DSTDIRS= ${DSTROOT}${BINDIR}

# Source files.
# All these files will be installed with "make installsrc"
HFILES= npd.h
CFILES= npcomm.c asciitops.c port.c
PSWFILES=
AUX_FILES= Makefile
SRCS=	${HFILES} ${CFILES} $(PSWFILES)
GARBAGE= TAGS tags *_vers.c

all:	${PRODUCTS}

#
# Building NPCOMM
#
NPCOMM_OBJS = ${CFILES:.c=.o} $(PSWFILES:.psw=.o) npcomm_vers.o
npcomm: ${NPCOMM_OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ ${NPCOMM_OBJS} ${LIBS}

npcomm_vers.c: $(NPCOMM_OBJS)
	-rm -f $@
	vers_string -c npcomm > $@

${PRODUCTS}:	.depend_done $(OBJS)

clean:	ALWAYS
	-rm -f *.o
	-rm -f ${PRODUCTS} ${DERIVED} ${INTERMEDIATES} ${GARBAGE}
	-rm -f Makedep .depend_done
	-rm -f *~
	-rm -f #*#
	touch Makedep

install: DSTROOT ${DSTDIRS} all
	mkdirs $(DSTROOT)$(BINDIR)
	install ${IFLAGS} npcomm ${DSTROOT}${BINDIR}/npcomm
	install -m 755 -o root -g wheel comm ${DSTROOT}${BINDIR}/comm
	(cd ${DSTROOT}${BINDIR};\
		rm -f psif psnf pstf psgf psplot psvf pscf psdf psrf )
	(cd ${DSTROOT}${BINDIR};\
		ln -s ./comm psif;\
		ln -s ./comm psnf;\
		ln -s ./comm pstf;\
		ln -s ./comm psgf;\
		ln -s ./comm psplot;\
		ln -s ./comm psvf;\
		ln -s ./comm pscf;\
		ln -s ./comm psdf;\
		ln -s ./comm psrf )

installsrc: SRCROOT
	-rm -rf ${SRCROOT}/npcomm
	mkdirs ${SRCROOT}/npcomm
	cp $(SRCS) ${SRCROOT}/npcomm
	cp $(AUX_FILES) $(SRCROOT)/npcomm
	touch $(SRCROOT)/npcomm/Makedep

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

depend .depend_done: ${DERIVED}
	rm -f Makedep
	${CC} -M ${CFILES} ${SFILES} ${DERIVED} | \
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

