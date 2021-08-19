CFLAGS = -g -O

PREBUILDSRCS = prebuild.c dict.h prebuiltformat.h dict.c prebuild.1
SCREENAFMSRCS = screenafm.m afm.m afmprivate.h font.h Font.h screenafm.1
FONTUPDATESRCS= font_update_2.0 font_update_2.0.1

all: prebuild screenafm

installsrc:
	mkdirs $(SRCROOT)/bin
	cp $(PREBUILDSRCS) $(SCREENAFMSRCS) $(FONTUPDATESRCS) \
	makefile $(SRCROOT)/bin

install: prebuild screenafm
	mkdirs $(DSTROOT)/usr/bin
	mkdirs $(DSTROOT)/usr/local/doc/fonts_proj/man
	install -c -m 755 -o bin -s prebuild $(DSTROOT)/usr/bin
	install -c -m 755 -o bin -s screenafm $(DSTROOT)/usr/bin
	install -c -m 444 prebuild.1 $(DSTROOT)/usr/local/doc/fonts_proj/man
	install -c -m 444 screenafm.1 $(DSTROOT)/usr/local/doc/fonts_proj/man
	install -c -m 755 -o bin font_update_2.0 $(DSTROOT)/usr/bin
	install -c -m 444 font_update_2.0.1 \
	 $(DSTROOT)/usr/local/doc/fonts_proj/man

clean: 
	/bin/rm -f *~ *.o prebuild screenafm

prebuild: prebuild.o dict.o prebuiltformat.h dict.h makefile
	$(CC) $(CFLAGS) -o prebuild prebuild.o dict.o -lsys_s

prebuild.o: prebuild.c dict.h prebuiltformat.h

dict.o: dict.c dict.h

screenafm : screenafm.o afm.o
	$(CC) $(CFLAGS) -o screenafm screenafm.o afm.o -lNeXT_s -lsys_s

screenafm.o : screenafm.m font.h

afm.o : afm.m afmprivate.h



