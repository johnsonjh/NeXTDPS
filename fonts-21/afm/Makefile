#
#
# Makefile for afm files
#
#
# The afm files here are transferred from Adobe, and
# thus we can't really re-'make' them.  The real purpose
# of this file is to support installing them.
#
install : 
	mkdirs $(DSTROOT)/NextLibrary/Fonts/afm
	chmod 777 $(DSTROOT)/NextLibrary/Fonts/afm
	cp *.afm $(DSTROOT)/NextLibrary/Fonts/afm
	chmod 444 $(DSTROOT)/NextLibrary/Fonts/afm/*.afm
	chmod 555 $(DSTROOT)/NextLibrary/Fonts/afm

clean : 

installsrc : 
	mkdirs $(SRCROOT)/afm
	cp * $(SRCROOT)/afm


