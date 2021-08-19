#
#
# Makefile for isoadobe (outline font descriptions)
#
# This makefile allows installation of the adobe fonts.
#
# Leo 10Jun88
# pgraff 13Apr90  no changes to makefile, but data files are now type1 fonts
#		  (as opposed to type 4, diskbased)
#


install :
	mkdirs -m 777 $(DSTROOT)/NextLibrary/Fonts/outline
	cp * $(DSTROOT)/NextLibrary/Fonts/outline
	rm -f $(DSTROOT)/NextLibrary/Fonts/outline/Makefile*
	chmod 444 $(DSTROOT)/NextLibrary/Fonts/outline/*
	chmod 555 $(DSTROOT)/NextLibrary/Fonts/outline

installsrc :
	mkdirs $(SRCROOT)/outline
	cp * $(SRCROOT)/outline

clean : 
	/bin/rm -f *~



