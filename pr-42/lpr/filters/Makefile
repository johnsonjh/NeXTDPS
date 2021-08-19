#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1 (Berkeley) 5/15/85
#
#
# makefile for line printer filters and associated programs
#

CFLAGS=	-O -fwritable-strings -D__STRICT_BSD__ -DNeXT_MOD
DSTDIR= 
LIBDIR=	${DSTROOT}${DSTDIR}/usr/lib
CXREF=	/usr/ucb/ctags -x
SRCS=	lpf.c necf.c
#	chrtab.c rvsort.c rvcat.c vsort.c vcat.c vpf.c vpsf.c vdmp.c vpltdmp.c \
#	vfontinfo.c vwidth.c vfw.c fcvt.c rotate.c rotprt.c
#	vplotf.c
PRINTER=lpf necf
#VARIAN=	rvsort rvcat vsort vcat vpf vpsf vdmp vpltdmp vfontinfo vwidth vfw fcvt rotate rotprt
#	vplotf
all:	printer
#varian

printer: ${PRINTER}

#varian:	${VARIAN}

lpf:	lpf.c
	${CC} -o lpf ${CFLAGS} lpf.c

necf:	necf.c
	${CC} -o necf ${CFLAGS} necf.c

#rvsort:	rvsort.c
#	${CC} -o rvsort ${CFLAGS} rvsort.c
#
#rvcat:	rvcat.c
#	${CC} -o rvcat ${CFLAGS} rvcat.c
#
#vsort:	vsort.c
#	${CC} -o vsort ${CFLAGS} vsort.c
#
#vcat:	vcat.c
#	${CC} -o vcat ${CFLAGS} vcat.c
#
#vpf:	vpf.c
#	${CC} -o vpf ${CFLAGS} vpf.c
#
#vpsf:	vpsf.c
#	${CC} -o vpsf ${CFLAGS} vpsf.c
#
#vdmp: vdmp.c
#	${CC} -o vdmp ${CFLAGS} vdmp.c
#
#vpltdmp: vpltdmp.c
#	${CC} -o vpltdmp ${CFLAGS} vpltdmp.c
#
##vplotf: vplotf.c chrtab.o
##	${CC} -o vplotf ${CFLAGS} vplotf.c chrtab.o
#
#vfontinfo: vfontinfo.c
#	${CC} -o vfontinfo ${CFLAGS} vfontinfo.c
#
#vwidth: vwidth.c
#	${CC} -o vwidth ${CFLAGS} vwidth.c
#
#vfw: vfw.c
#	${CC} -o vfw ${CFLAGS} vfw.c
#
#fcvt: fcvt.c
#	${CC} -o fcvt ${CFLAGS} fcvt.c
#
#rotate: rotate.c
#	${CC} -o rotate ${CFLAGS} rotate.c
#
#rotprt: rotprt.c
#	${CC} -o rotprt ${CFLAGS} rotprt.c

install: ckdstroot ${LIBDIR} printer
	install -s lpf ${LIBDIR}/lpf
	install -s necf ${LIBDIR}/necf

#installv: ckdstroot ${LIBDIR} varian
#	install -s vfontinfo ${LIBDIR}/vfontinfo
#	install -s vwidth ${LIBDIR}/vwidth
#	install -s vfw ${LIBDIR}/vfw
#	install -s fcvt ${LIBDIR}/fcvt
#	install -s rotate ${LIBDIR}/rotate
#	install -s rotprt ${LIBDIR}/rotprt
#	install -s rvsort ${LIBDIR}/rvsort
#	install -s rvcat ${LIBDIR}/rvcat
#	install -s vsort ${LIBDIR}/vsort
#	install -s vcat ${LIBDIR}/vcat
#	install -s vpf ${LIBDIR}/vpf
#	ln ${LIBDIR}/vpf ${LIBDIR}/vpfW
#	install -s vpsf ${LIBDIR}/vpsf
#	install -s vdmp ${LIBDIR}/vdmp
#	install -s vpltdmp ${LIBDIR}/vpltdmp
##	install -s vplotf ${LIBDIR}/vplotf

ckdstroot:
	@if [ -n "${DSTROOT}" ]; \
	then \
		exit 0; \
	else \
		echo Must define DSTROOT; \
		exit 1; \
	fi

${LIBDIR}:
	mkdirs $@

clean:
	rm -f ${PRINTER} ${VARIAN} *.o

print:
	@pr makefile
	@${CXREF} *.c | pr -h XREF
	@pr *.h *.c
