
#
#
# Makefile for Screen AFM files
#
# This makefile assumes the bitmap files can be found in
# ../bitmapSources/<printerfontname>/<size>.<shortname>,
# e.g., ../bitmapSources/Helvetica/08.Helve.
#
# Each of the families is declared to depend on the plain
# 10-point instance of the corresponding bitmap source
# files.  Although this is not a complete make rule, it
# does allow the rebuilding to be somewhat conditional.
#
# Assuming this, off it goes.
#

default :
	@echo You can make all, clean, install, or installsrc here.

all : Helvetica-Family Times-Family Courier-Family Lexi Symbol Ohlfs

clean : 
	/bin/rm -f Screen-*.afm

install : all
	mkdirs $(DSTROOT)/NextLibrary/Fonts/afm
	chmod 777 $(DSTROOT)/NextLibrary/Fonts/afm
	cp Screen-*.afm $(DSTROOT)/NextLibrary/Fonts/afm
	chmod 444 $(DSTROOT)/NextLibrary/Fonts/afm/Screen-*.afm
	chmod 555 $(DSTROOT)/NextLibrary/Fonts/afm

installsrc : 
	mkdirs $(SRCROOT)/afm.screen
	cp Makefile $(SRCROOT)/afm.screen

#
# Pick up the SOURCES macros for each font face
include ../bitmapSources/SourcesList

Helvetica-Family : Screen-Helvetica.10.afm

Screen-Helvetica.10.afm : ../bitmapSources/Helvetica/10.Helve
	../bin/screenafm Helvetica $(HELVE_SOURCES)
	../bin/screenafm Helvetica-Bold $(HELVEBOL_SOURCES)
	../bin/screenafm Helvetica-Oblique $(HELVEOBL_SOURCES)
	../bin/screenafm Helvetica-BoldOblique $(HELVEBOLOBL_SOURCES)

Times-Family : Screen-Times-Roman.10.afm

Screen-Times-Roman.10.afm : ../bitmapSources/Times/10.TimesRom
	../bin/screenafm Times-Roman $(TIMESROM_SOURCES)
	../bin/screenafm Times-Bold $(TIMESBOL_SOURCES)
	../bin/screenafm Times-Italic $(TIMESITA_SOURCES)
	../bin/screenafm Times-BoldItalic $(TIMES_BOLITA_SOURCES)

Courier-Family : Screen-Courier.10.afm

Screen-Courier.10.afm : ../bitmapSources/Courier/10.Couri
	../bin/screenafm Courier $(COURIER_SOURCES)
	../bin/screenafm Courier-Bold $(COURIERBOL_SOURCES)
	../bin/screenafm Courier-Oblique $(COURIEROBL_SOURCES)
	../bin/screenafm Courier-BoldOblique $(COURIERBOLOBL_SOURCES)

Lexi : Screen-Lexi.10.afm

Screen-Lexi.10.afm : ../bitmapSources/Lexi/10.Lexi
	../bin/screenafm Lexi $(LEXI_SOURCES)

Symbol : Screen-Symbol.10.afm

Screen-Symbol.10.afm : ../bitmapSources/Symbol/10.Symbo
	../bin/screenafm Symbol $(SYMBOL_SOURCES)

Ohlfs : Screen-Ohlfs.10.afm

Screen-Ohlfs.10.afm : ../bitmapSources/Ohlfs/10.Ohlfs
	../bin/screenafm Ohlfs $(OHLFS_SOURCES)


