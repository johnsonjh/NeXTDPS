
#
#  NeXT Internal Library Makefile
#  Copyright 1987, NeXT, Inc.
#
#  This Makefile is makes the library of trivial wraps that take
#  an extra context parameter.
#

.SUFFIXES: .origpsw

PRODUCTS = libdpsops.a dpsops.h dpswraps.h

STDPSWFILES = dpscolorops.psw dpscontrolops.psw dpsctxtops.psw dpsdataops.psw \
		dpsfontops.psw dpsgstateops.psw dpsioops.psw \
		dpsmathops.psw dpsmatrixops.psw dpsmiscops.psw \
		dpsopstack.psw dpspaintops.psw dpspathops.psw \
		dpssysops.psw dpswinops.psw
NEXTPSWFILES = DPSmousekey.psw DPSevent.psw DPSmisc.psw DPSwindow.psw \
		DPSgraphics.psw
PSWFILES = $(NEXTPSWFILES) $(STDPSWFILES)
SRC = makeExplicitWraps.c Makefile
SRCDIR = $(SRCROOT)/ctxtWraps
OFILES = $(PSWFILES:.psw=.o)

CFLAGS = -O -I.. -Wimplicit
PSWFLAGS = -a -r

install: all
	mkdirs $(DSTROOT)/usr/lib
	install -q -c -r -m 644 libdpsops.a $(DSTROOT)/usr/lib
	mkdirs $(DSTROOT)/usr/include/dpsclient
	install -q -c -m 444 dpswraps.h dpsops.h \
				$(DSTROOT)/usr/include/dpsclient

all: $(PRODUCTS)

clean:
	rm -f *.o $(PRODUCTS) $(PSWFILES) $(PSWFILES:.psw=.c) \
						$(PSWFILES:.psw=.h)

installsrc:
	mkdirs $(SRCDIR)
	chmod 755 $(SRCDIR)
	tar cf - $(SRC) | (cd $(SRCDIR); tar xf -)
	cd $(SRCDIR);			\
	for i in $(ORIGSTDPSWFILES:.psw=); do		\
		ln -s ../$$i.psw d$$i.origpsw;		\
	done;						\
	for i in $(ORIGNEXTPSWFILES:.psw=); do		\
		ln -s ../$$i.psw D$$i.origpsw;		\
	done;


libdpsops.a: $(OFILES)
	ar ruv $@ $(OFILES)
	ranlib $@

dpswraps.h: $(NEXTPSWFILES:.psw=.c)
	echo '#import "dpsops.h"' > $@
	cat $(NEXTPSWFILES:.psw=.h) >> $@

dpsops.h: $(STDPSWFILES:.psw=.c)
	cat $(STDPSWFILES:.psw=.h) > $@

makeExplicitWraps: makeExplicitWraps.c
	$(CC) $(CFLAGS) -o makeExplicitWraps makeExplicitWraps.c

ORIGSTDPSWFILES ORIGNEXTPSWFILES:
	@if [ -n "${$@}" ]; then exit 0; \
	else echo Must define $@; exit 1; fi

$(OFILES): makeExplicitWraps

.origpsw.c:
	./makeExplicitWraps $*.origpsw > $*.psw
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.c $*.psw

.origpsw.psw:
	./makeExplicitWraps $*.origpsw > $*.psw

.psw.c:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.c $*.psw


