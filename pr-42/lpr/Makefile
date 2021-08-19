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
DAEMON=daemon
SPGRP=daemon

CDEFS= -DNeXT_MOD -DNeXT_NFS -D__STRICT_BSD__
#CDEFS= -DNeXT_MOD -DNeXT_NFS -D__STRICT_BSD__ -DNeXT_DEBUG
CFLAGS=	-O -g -fwritable-strings $(CDEFS) 
IFLAGS= -s -o ${ROOT} -g ${SPGRP} -m 6711
LIBS= -ltermcap -lsys_s -ltext

DSTDIR= /
BINDIR=	$(DSTDIR)/usr/ucb
LIBDIR= $(DSTDIR)/usr/lib
#SPOOLDIR= $(DSTDIR)/usr/spool/lpd
ETCDIR=	$(DSTDIR)/usr/etc

PRODUCTS= lpd lpr lpq lprm lpc lptest pac

#DSTDIRS= ${DSTROOT}${BINDIR} ${DSTROOT}${ETCDIR} ${DSTROOT}${LIBDIR} \
#	${DSTROOT}${SPOOLDIR}
DSTDIRS= ${DSTROOT}${BINDIR} ${DSTROOT}${ETCDIR} ${DSTROOT}${LIBDIR}

HFILES=	lp.h lp.local.h lpc.h
CFILES= cmds.c cmdtab.c common.c displayq.c lpc.c lpd.c lpdchar.c lpq.c \
	lpr.c lprm.c lptest.c pac.c printcap.c printjob.c recvjob.c \
	rmjob.c startdaemon.c
SFILES=
YFILES=
SRCS=	${HFILES} ${YFILES} ${CFILES} ${SFILES}

FILES=	${SRCS} Makefile Makedep etc.printcap
INTERMEDIATES=
DERIVED=
GARBAGE= TAGS tags *_vers.c

all:	${PRODUCTS} FILTERS

#
# Building lpd
#
LPD_OBJS= lpd.o printjob.o recvjob.o displayq.o rmjob.o\
	startdaemon.o lpdchar.o common.o printcap.o

lpd.NEW:	${LPD_OBJS} lpd_vers.o
	$(CC) $(CFLAGS) -o $@ ${LPD_OBJS} lpd_vers.o ${LIBS}

lpd_vers.c: ${DOVERS} ${LPD_OBJS}
	@rm -f $@
	vers_string -c lpd > $@

#
# Building lpr
#
LPR_OBJS= lpr.o startdaemon.o printcap.o defaults.o

lpr.NEW:	${LPR_OBJS} lpr_vers.o
	$(CC) $(CFLAGS) -o $@ ${LPR_OBJS} lpr_vers.o ${LIBS}

lpr_vers.c: ${DOVERS} ${LPR_OBJS}
	@rm -f $@
	vers_string -c lpr > $@

#
# Building lpq
#
LPQ_OBJS= lpq.o displayq.o common.o printcap.o defaults.o

lpq.NEW:	${LPQ_OBJS} lpq_vers.o
	$(CC) $(CFLAGS) -o $@ ${LPQ_OBJS} lpq_vers.o ${LIBS}

lpq_vers.c: ${DOVERS} ${LPQ_OBJS}
	@rm -f $@
	vers_string -c lpq > $@

#
# Building lprm
#
LPRM_OBJS= lprm.o rmjob.o startdaemon.o common.o printcap.o defaults.o

lprm.NEW:	${LPRM_OBJS} lprm_vers.o
	$(CC) $(CFLAGS) -o $@ ${LPRM_OBJS} lprm_vers.o ${LIBS}

lprm_vers.c: ${DOVERS} ${LPRM_OBJS}
	@rm -f $@
	vers_string -c lprm > $@

#
# Building lpc
#
LPC_OBJS= lpc.o cmds.o cmdtab.o startdaemon.o common.o printcap.o

lpc.NEW:	${LPC_OBJS} lpc_vers.o
	$(CC) $(CFLAGS) -o $@ ${LPC_OBJS} lpc_vers.o ${LIBS}

lpc_vers.c: ${DOVERS} ${LPC_OBJS}
	@rm -f $@
	vers_string -c lpc > $@

#
# Building lptest
#
LPTEST_OBJS= lptest.o

lptest.NEW:	${LPTEST_OBJS} lptest_vers.o
	$(CC) $(CFLAGS) -o $@ ${LPTEST_OBJS} lptest_vers.o ${LIBS}

lptest_vers.c: ${DOVERS} ${LPTEST_OBJS}
	@rm -f $@
	vers_string -c lptest > $@

#
# Building pac
#
PAC_OBJS= pac.o printcap.o defaults.o

pac.NEW:	${PAC_OBJS} pac_vers.o
	$(CC) $(CFLAGS) -o $@ ${PAC_OBJS} pac_vers.o ${LIBS}

pac_vers.c: ${DOVERS} ${PAC_OBJS}
	@rm -f $@
	vers_string -c pac > $@

#
# Non-standard targets
#

FILTERS:
	cd filters; make ${MFLAGS}

#
# Standard targets
#
${PRODUCTS}:	.depend_done $$@.NEW

clean:	ALWAYS
	-rm -f *.o *.NEW
	-rm -f ${PRODUCTS} ${DERIVED} ${INTERMEDIATES} ${GARBAGE}
	-rm -f Makedep .depend_done
	touch Makedep
	cd filters; $(MAKE) clean

install: DSTROOT ${DSTDIRS} all
	install ${IFLAGS} lpd.NEW ${DSTROOT}${LIBDIR}/lpd
	install ${IFLAGS} lpr.NEW ${DSTROOT}${BINDIR}/lpr
	install ${IFLAGS} lpq.NEW ${DSTROOT}${BINDIR}/lpq
	install ${IFLAGS} lprm.NEW ${DSTROOT}${BINDIR}/lprm
	install -s -g ${SPGRP} -m 2711 lpc.NEW ${DSTROOT}${ETCDIR}/lpc
	install -s lptest.NEW ${DSTROOT}${BINDIR}/lptest
	install -s pac.NEW ${DSTROOT}${ETCDIR}/pac
	cd filters; make ${MFLAGS} install
#	-chown ${DAEMON} ${DSTROOT}${SPOOLDIR}
#	chgrp ${SPGRP} ${DSTROOT}${SPOOLDIR}
#	chmod 775 ${DSTROOT}${SPOOLDIR}

#installsrc: SRCROOT
#	-rm -rf ${SRCROOT}/lpr
#	mkdirs ${SRCROOT}/lpr ${SRCROOT}/lpr/filters
#	cp ${FILES} ${SRCROOT}/lpr
#	cp filters/* ${SRCROOT}/lpr/filters
	
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
