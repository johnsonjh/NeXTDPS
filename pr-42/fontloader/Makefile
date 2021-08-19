#
# Makefile for fontloader
# (c) NeXT, Inc.  1987
#
# makefile for font downloading command-line utility
#

NAME = fontloader

CFLAGS = -g -Wall -O

IFLAGS = -s -m 755

LIBS = 

BINDIR=	/usr/bin

DSTDIRS= ${DSTROOT}${BINDIR}

# Source files.
CFILES= main.c
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
	install ${IFLAGS} $(NAME) ${DSTROOT}${BINDIR}/$(NAME)

installsrc: SRCROOT
	-rm -rf ${SRCROOT}
	mkdirs ${SRCROOT}
	cp $(SRCS) Makefile Makedep ${SRCROOT}

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

depend .depend_done: ${DERIVED} ${CFILES}
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

