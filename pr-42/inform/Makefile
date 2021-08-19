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

NAME = Inform

ROOT=root
DAEMON=daemon
SPGRP=daemon
  
LFLAGS = -segcreate __NIB PrinterAlert nib/PrinterAlert.nib

#CFLAGS = -g -O -DNeXT_MOD -DNeXT_NFS -Wimplicit
CFLAGS = -g -O -DNeXT_MOD -DNeXT_NFS
IFLAGS= -s -o ${ROOT} -g ${SPGRP} -m 755

LIBS =  -lNeXT_s -lsys_s

BINDIR=	/usr/lib/NextPrinter

DSTDIRS= ${DSTROOT}${BINDIR}

# Source files.
HFILES= GrowAlert.h npd.h
CFILES= port.c alert.c
MFILES = main.m GrowAlert.m
YFILES=
SRCS=	${HFILES} ${YFILES} ${CFILES} ${SFILES} $(MFILES) $(PSWFILES)

#
# Rules.
#
OBJFILES = $(MFILES:.m=.o) $(CFILES:.c=.o) $(PSWFILES:.psw=.o) 
DERIVED = $(PSWFILES:.psw=.c) 
GARBAGE = $(DERIVED) core errs 

all:	$(NAME)

$(NAME): $(OBJFILES) 
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $(OBJFILES) $(LIBS) 

clean: 
	-rm -f #*#
	-rm -f *~
	-rm -f *.o $(NAME) $(DERIVED) $(GARBAGE) 


install: DSTROOT ${DSTDIRS} all
	mkdirs $(DSTROOT)$(BINDIR)
	install ${IFLAGS} Inform ${DSTROOT}${BINDIR}/Inform

local: LCLROOT ${LCLDIRS} all
	install ${IFLAGS} Inform ${LCLROOT}${BINDIR}

installsrc: SRCROOT
	-rm -rf ${SRCROOT}
	mkdirs ${SRCROOT} $(SRCROOT)/nib
	cp nib/* $(SRCROOT)/nib
	cp $(SRCS) Makefile ${SRCROOT}

DSTROOT LCLROOT SRCROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

${DSTDIRS} ${LCLDIRS}:
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

