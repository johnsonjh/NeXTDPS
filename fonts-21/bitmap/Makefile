#
#
# Makefile for prebuilt font files
#
# Created 10Jun88 Leo
#
# Modified:
# 02Sep88 Make it really build the prebuilt files
# 12Feb89 Leo  Use new prebuild utility
#

all: 
	@echo You can make clean, install, installsrc, or bitmap_fonts here

PREBUILD = ../bin/prebuild

install: bitmap_fonts
	mkdirs $(DSTROOT)/NextLibrary/Fonts/bitmap
	cp *.[bl]epf $(DSTROOT)/NextLibrary/Fonts/bitmap
	chmod 444 $(DSTROOT)/NextLibrary/Fonts/bitmap/*.[bl]epf
	chmod 555 $(DSTROOT)/NextLibrary/Fonts/bitmap

clean:
	rm -f *.[bl]epf temp.*

installsrc : 
	mkdirs $(SRCROOT)/bitmap
	cp Makefile $(SRCROOT)/bitmap

# Pick up the macros for the bitmap distribution file lists
include ../bitmapSources/SourcesList

# 16782596H # Times-Roman
# 16782603H # Times-Bold
# 16782611H # Times-Italic
# 16782635H # Helvetica
# 16782639H # Bold
# 16782642H # Oblique
# 16782645H # BoldOblique
# 16785072H # Times-BoldItalic
# 16788784H # Courier
# 16788788H # Courier-Bold
# 16788793H # Courier-0blique
# 16788798H # Courier-BoldOblique
# 16784075H # Symbol uid 6859
# 50331650H # Ohlfs type 3 unique id 2
# 50331651H # Lexi type 3 unique id 3

bitmap_fonts: Helvetica_bitmaps Times_bitmaps Courier_bitmaps Lexi_bitmaps Symbol_bitmaps Ohlfs_bitmaps

Helvetica_bitmaps : Helvetica.bepf Helvetica-Oblique.bepf \
				Helvetica-Bold.bepf Helvetica-BoldOblique.bepf 

Helvetica.bepf : $(HELVE_SOURCES)
	$(PREBUILD) Helvetica 1 5419 ISOLatin1CharacterSet $(HELVE_SOURCES)


Helvetica-Bold.bepf : $(HELVEBOL_SOURCES)
	$(PREBUILD) Helvetica-Bold 1 5423 ISOLatin1CharacterSet $(HELVEBOL_SOURCES)


Helvetica-Oblique.bepf : $(HELVEOBL_SOURCES)
	$(PREBUILD) Helvetica-Oblique 1 5426 ISOLatin1CharacterSet \
							$(HELVEOBL_SOURCES)


Helvetica-BoldOblique.bepf : $(HELVEBOLOBL_SOURCES)		
	$(PREBUILD) Helvetica-BoldOblique 1 5429 ISOLatin1CharacterSet \
						$(HELVEBOLOBL_SOURCES)


Times_bitmaps : Times-Roman.bepf Times-Bold.bepf Times-Italic.bepf \
						Times-BoldItalic.bepf

Times-Roman.bepf : $(TIMESROM_SOURCES)
	$(PREBUILD) Times-Roman 1 5380 ISOLatin1CharacterSet \
						$(TIMESROM_SOURCES)

Times-Bold.bepf : $(TIMESBOL_SOURCES)
	$(PREBUILD) Times-Bold 1 5387 ISOLatin1CharacterSet \
						$(TIMESBOL_SOURCES)

Times-Italic.bepf : $(TIMESITA_SOURCES)
	$(PREBUILD) Times-Italic 1 5395 ISOLatin1CharacterSet \
						$(TIMESITA_SOURCES)

Times-BoldItalic.bepf : $(TIMES_BOLITA_SOURCES)
	$(PREBUILD) Times-BoldItalic 1 7856 ISOLatin1CharacterSet \
						$(TIMES_BOLITA_SOURCES)


Courier_bitmaps : Courier.bepf Courier-Bold.bepf Courier-Oblique.bepf \
						Courier-BoldOblique.bepf

Courier.bepf : $(COURIER_SOURCES)
	$(PREBUILD) Courier 1 11568 ISOLatin1CharacterSet \
						$(COURIER_SOURCES)

Courier-Bold.bepf : $(COURIERBOL_SOURCES)
	$(PREBUILD) Courier-Bold 1 11572 ISOLatin1CharacterSet \
						$(COURIERBOL_SOURCES)

Courier-Oblique.bepf : $(COURIEROBL_SOURCES)
	$(PREBUILD) Courier-Oblique 1 11577 ISOLatin1CharacterSet \
						$(COURIEROBL_SOURCES)

Courier-BoldOblique.bepf : $(COURIERBOLOBL_SOURCES)
	$(PREBUILD) Courier-BoldOblique 1 11582 ISOLatin1CharacterSet \
						$(COURIERBOLOBL_SOURCES)

Symbol_bitmaps :  Symbol.bepf

Symbol.bepf : $(SYMBOL_SOURCES)
	$(PREBUILD) Symbol 1 6859 Symbol $(SYMBOL_SOURCES)


Lexi_bitmaps : Lexi.bepf

Lexi.bepf : $(LEXI_SOURCES)
	$(PREBUILD) Lexi 3 3 Lexi $(LEXI_SOURCES)

Ohlfs_bitmaps: Ohlfs.bepf

Ohlfs.bepf : $(OHLFS_SOURCES)
	$(PREBUILD) Ohlfs 3 2 ISOLatin1CharacterSet $(OHLFS_SOURCES)








