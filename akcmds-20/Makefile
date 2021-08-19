#
#  NeXT Makefile
#  Copyright 1987, NeXT, Inc.
#

SRCS = Makefile pft.c pft.1	\
	copypaste.c \
        buildafmdir.h buildafmdir.m buildafmdir.1 metrics.c metrics.h \
	FontFace.h FontFace.m \
	FontFamily.h FontFamily.m FontString.h FontString.m \
	FontStringList.h FontStringList.m \
	tiffutil.m tiffdump.m tiffutil.1 enable_service.m

BUILDAFMDIR_OBJS = buildafmdir.o metrics.o \
       FontFace.o FontFamily.o FontString.o FontStringList.o

TIFFUTIL_OBJS = tiffutil.o tiffdump.o

# -I needed so kit files in /usr/local/include can find public kit files
CFLAGS = -g -O -Wall -I/usr/include/appkit 
PSWFLAGS = -a -r
LDFLAGS = 
BINDIR = $(DSTROOT)/usr/bin


all: pft buildafmdir copy tiffutil makeimageserver enable_service
install: install_pft install_buildafmdir install_tiffutil install_copy install_imageserver install_enable_service

#
# make and install buildafmdir
# 
install_buildafmdir: buildafmdir
	mkdirs $(BINDIR)
	install -s -o root -m 755 buildafmdir $(BINDIR)

buildafmdir: $(BUILDAFMDIR_OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o buildafmdir $(BUILDAFMDIR_OBJS) -lsys_s

#
# make and install pft
# 
install_pft: pft
	mkdirs $(BINDIR)
	install -s -c -m 755 pft $(BINDIR)
#	mkdirs $(DSTROOT)/usr/local/doc/pft/man
#	install -c -m 444 pft.1 $(DSTROOT)/usr/local/doc/pft/man

pft: pft.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o pft pft.c -lNeXT_s -lsys_s

#
# make and install enable_service and disable_service
#

install_enable_service: enable_service
	mkdirs -m 755 $(BINDIR)
	install -m 755 -s enable_service $(BINDIR)
	-rm -f $(BINDIR)/disable_service
	ln $(BINDIR)/enable_service $(BINDIR)/disable_service

enable_service: enable_service.m
	$(CC) $(LDFLAGS) $(CFLAGS) -o enable_service enable_service.m -lNeXT_s -lsys_s

#
# make and install copypaste utility
#
copy: copypaste.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o copy copypaste.c -lNeXT_s -lsys_s

install_copy: copy
	mkdirs -m 755 $(BINDIR)
	install -m 555 -s copy $(BINDIR)
	-rm -f $(BINDIR)/paste
	ln $(BINDIR)/copy $(BINDIR)/paste

#
# make and install tiffutil
# 
install_tiffutil: tiffutil
	mkdirs -m 755 $(BINDIR)
	install -s -m 555 tiffutil $(BINDIR)

tiffutil: $(TIFFUTIL_OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o tiffutil $(TIFFUTIL_OBJS) -lNeXT_s -lsys_s

#
# install imageserver
# 
install_imageserver: makeimageserver
	(cd imageserver; make install DSTROOT=$(DSTROOT))

makeimageserver:
	(cd imageserver; make)

#
# other targets required by build process
# 
installsrc:
	mkdirs $(SRCROOT)
	cp $(SRCS) $(SRCROOT)
	chmod 444 $(SRCROOT)/*
	(cd imageserver; make installsrc SRCROOT=$(SRCROOT)/imageserver)

clean:
	rm -f pft buildafmdir copy tiffutil enable_service *.o
	(cd imageserver; make clean)

