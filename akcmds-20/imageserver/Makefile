#
# Makefile for building /usr/etc/imageserver
#

CFLAGS=-O -g 

CFILES=a_cdr.c a_dcr.c blk_cdrs.c cdr_dcr_commons.c coord_hi.c do_inter.c \
	hufutil.c initarrays.c strip_io.c vutil.c xformc.c \
	nxjpeg.c 
MFILES=imageServer.m
HEADERS= config.h imagemessage.h  nxjpeg.h vservice.h c3sim.h  \
		gStruct.h  newHeader.h vista.h
OTHER= hufftab.dat an_init.dat Makefile 
SRCS= $(CFILES) $(MFILES) $(HEADERS) $(OTHER)

JPEGOBJ=$(CFILES:.c=.o)

BINDIR = $(DSTROOT)/usr/etc

LIBS=jpeglib.a -lm -lNeXT_s -lsys_s 


LDFLAGS=-segcreate __PRIV hufftab hufftab.dat -segcreate __PRIV an_init \
	an_init.dat 

all:	imageserver
install: install_imageserver 
#
# other targets required by build process
# 
installsrc:
	mkdirs $(SRCROOT)
	cp $(SRCS) $(SRCROOT)
	chmod 444 $(SRCROOT)/*

#
# make and install imageserver
# 
install_imageserver: imageserver
	mkdirs $(BINDIR)
	install -s -o root -m 755 imageserver $(BINDIR)

imageserver: imageServer.o jpeglib.a
	cc -o imageserver $(CFLAGS) imageServer.o $(LDFLAGS) $(LIBS)

jcompress: jcompress.o jpeglib.a
	cc -o jcompress $(CFLAGS) jcompress.o $(LDFLAGS) $(LIBS)

	
jpeglib.a: $(JPEGOBJ)
	ar r jpeglib.a  $? ; ranlib jpeglib.a

clean:
	rm -f *.o jpeglib.a imageserver jcompress

