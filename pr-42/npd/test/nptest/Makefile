#
# Generic top-level makefile for multi-file commands
# (c) NeXT, Inc.  1987
#
# makefile for line printer spooling system
#
# Ownerships {see note at the top of lpd.c}
# ROOT		a user that can access any and all files
# DAEMON	someone special
# SPGRP		the group id of the spooling programs
#
ROOT=root
GROUP=wheel

#CDEFS=
CDEFS= -DDEBUG
CFLAGS=	-O -g -fwritable-strings $(CDEFS) 
IFLAGS= -s -o ${ROOT} -g ${GROUP} -m 755
LIBS=

DSTDIR= /
ETCDIR=	$(DSTDIR)/usr/etc

PRODUCTS= nptest_print nptest_select

DSTDIRS= ${DSTROOT}${ETCDIR}

HFILES=	
CFILES= nptest_print.c nptest_select.c
SFILES=
YFILES=
SRCS=	${HFILES} ${YFILES} ${CFILES} ${SFILES}

FILES=	${SRCS} Makefile Makedep
INTERMEDIATES=
DERIVED=
GARBAGE= TAGS tags *_vers.c *~ #*#

all:	${PRODUCTS}

#
# Building nptest_print
#
NPTEST_PRINT_OBJS= nptest_print.o

nptest_print:	.depend_done ${NPTEST_PRINT_OBJS} nptest_print_vers.o
	$(CC) $(CFLAGS) -o $@ ${NPTEST_PRINT_OBJS} nptest_print_vers.o ${LIBS}

nptest_print_vers.c: ${DOVERS} ${NPTEST_PRINT_OBJS}
	@rm -f $@
	vers_string -c nptest_print > $@

#
# Building nptest_select
#
NPTEST_SELECT_OBJS= nptest_select.o

nptest_select:	.depend_done ${NPTEST_SELECT_OBJS} nptest_select_vers.o
	$(CC) $(CFLAGS) -o $@ ${NPTEST_SELECT_OBJS} nptest_select_vers.o ${LIBS}

nptest_select_vers.c: ${DOVERS} ${NPTEST_SELECT_OBJS}
	@rm -f $@
	vers_string -c nptest_select > $@

#
# Standard targets
#
clean:	ALWAYS
	-rm -f *.o *.NEW
	-rm -f ${PRODUCTS} ${DERIVED} ${INTERMEDIATES} ${GARBAGE}
	-rm -f Makedep .depend_done
	touch Makedep

install: DSTROOT ${DSTDIRS} all
	install ${IFLAGS} nptest_print ${DSTROOT}${ETCDIR}/nptest_print
	install ${IFLAGS} nptest_select ${DSTROOT}${ETCDIR}/nptest_select


#installsrc: SRCROOT
#	-rm -rf ${SRCROOT}/nptest
#	mkdirs ${SRCROOT}/nptest
#	cp ${FILES} ${SRCROOT}/nptest
	
DSTROOT:
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
	${CC} -M ${CDEFS} ${CFILES} ${SFILES} ${DERIVED} | \
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

-include Makedep
