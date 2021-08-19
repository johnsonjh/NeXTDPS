#
#
# Makefile for font installation
#
# 10Jun88 Leo
#
# 22Mar89  Leo   Added buildafmdir after install is complete
# 13Apr90  pgraff Changed over to type 1 fonts and new font directory
#	          structure.  For now, the install rule still lays down 
#		  the old structure, and the font_update_2.0 script converts
#		  to the new structure, leaving symbolic links behind.
# This Makefile must be run with $(DSTROOT) defined;
# it will then do make installs on the isoadobe files,
# the prebuilt files, and the afm files for both
# printer and screen fonts.
#
# NOTE: The ordering of these makes is important.
# bin contains utilities
# for building the bitmap files, the screen afm files
# and the font_update_2.0 program.
#

default : 
	@echo You can make install, installsrc, or clean.

install : 
	/bin/rm -rf $(DSTROOT)/NextLibrary/Fonts
	mkdirs $(DSTROOT)/NextLibrary/Fonts
	cd bitmapSources; make $(MFLAGS) install; cd ..
	cd outline;make $(MFLAGS) install; cd ..
	cd bin; make $(MFLAGS) install; cd ..
	cd bitmap;make $(MFLAGS) install;cd ..
	cd afm;make $(MFLAGS) install;cd ..
	cd afm.screen;make $(MFLAGS) install;cd ..
	chmod 777 $(DSTROOT)/NextLibrary/Fonts/afm
	chmod 777 $(DSTROOT)/NextLibrary/Fonts/outline
	chmod 777 $(DSTROOT)/NextLibrary/Fonts/bitmap
	bin/font_update_2.0 $(DSTROOT)/NextLibrary
	chmod 555 $(DSTROOT)/NextLibrary/Fonts/afm
	chmod 555 $(DSTROOT)/NextLibrary/Fonts/outline
	chmod 555 $(DSTROOT)/NextLibrary/Fonts/bitmap
	buildafmdir $(DSTROOT)/NextLibrary/Fonts
	-/usr/bin/cacheAFMData $(DSTROOT)/NextLibrary/Fonts


installsrc : 
	mkdirs $(SRCROOT)
	cp Makefile $(SRCROOT)
	cd bitmapSources; make $(MFLAGS) installsrc; cd ..
	cd outline;make $(MFLAGS) installsrc; cd ..
	cd bin; make $(MFLAGS) installsrc; cd ..
	cd bitmap;make $(MFLAGS) installsrc;cd ..
	cd afm;make $(MFLAGS) installsrc;cd ..
	cd afm.screen;make $(MFLAGS) installsrc;cd ..

clean : 
	cd bitmapSources; make $(MFLAGS) clean; cd ..
	cd outline;make $(MFLAGS) clean; cd ..
	cd bin; make $(MFLAGS) clean; cd ..
	cd bitmap;make $(MFLAGS) clean;cd ..
	cd afm;make $(MFLAGS) clean;cd ..
	cd afm.screen;make $(MFLAGS) clean;cd ..
	/bin/rm -f *~


