
#
#  NeXT Internal Library Makefile
#  Copyright 1987, NeXT, Inc.
#
#  This Makefile is used for maintaining and installing NeXT software
#  libraries.
#
#  Fill in values for the various variables at the places marked by
#  **number**.  Any rules or dependencies of your own can be added
#  at the end of the file.
#

#directory where make gets its includes.  Do not edit.
MAKE_DIR = /usr/local/lib/Makefiles

#  **1**  -LIBRARY NAME-
#  The base name of the library goes here. This should also be the same
#  name as the SGS project the library is built under.  For example, if
#  you enter "appkit" as the name, then the name of the SGS project
#  should be "appkit", and the library created will be "libappkit.a".
NAME = dpsclient

#  **2**  -SOURCE FILES-
#  Here list the .m, .c, .psw, .pswm, .s, .l, .lm, .y, .ym, .ps and
#  .spec files.  You may delete any lines that you don't use.
#  OTHER_SRCS is used for files with other suffixes that you wish
#  to be treated like source (i.e., printed with the source, copied
#  with the source, etc.).  For files in OTHER_SRCS, you need to
#  specify the corresponding .o file in OTHER_SRCS_OFILES.
CFILES = machContext.c streamContext.c outputContext.c \
	queues.c scanner.c input.c list.c utils.c namecode.c generic.c \
	stream.c globals.c pmon.c
NEXTPSWFILES = PSmousekey.psw PSevent.psw PSmisc.psw PSwindow.psw \
		PSgraphics.psw
STDPSWFILES = pscolorops.psw pscontrolops.psw psctxtops.psw psdataops.psw \
		psfontops.psw psgstateops.psw psioops.psw psmathops.psw \
		psmatrixops.psw psmiscops.psw psopstack.psw pspaintops.psw \
		pspathops.psw pssysops.psw pswinops.psw
PSWFILES =  PSNotSingleOps.psw $(NEXTPSWFILES) $(STDPSWFILES)
RELNOTES = DPSClientNotes.wn
OTHER_SRCS = makeNameTable.c systemnames.ps namecode.sed \
		makeDeadKeyTable.c deadKeyList
OTHER_SRCS_OFILES =

#  **3**  -COMMAND OPTIONS-
#  These are passed as arguments to the Objective-C preprocessor, the C
#  compiler, pswrap, the assembler, lex and yacc.  You may delete any
#  lines that you don't use.  All CFLAGS also get passed to Objective-C. 
#  DEBUGCFLAGS is substituted for CFLAGS when "debug" is made.
#  PROFCFLAGS is substituted for CFLAGS when "profile" is made.
#  Under CFLAGS, for each library that you use that supplies class
#  and message group files, you must use a -I$(OBJDIR)/XXX, where
#  XXX is the base name of the library.

COMMONCFLAGS = -I. -Wimplicit $(EXTERNAL_CFLAGS)
#			-DPMON -I/Net/harlie/mk/osdev/PMON/pmon/include
CFLAGS = -O $(COMMONCFLAGS)
DEBUGCFLAGS = -g $(COMMONCFLAGS)
PROFCFLAGS = -O -pg $(COMMONCFLAGS)
SHLIBCFLAGS = -O -g $(COMMONCFLAGS) -I$(SHLIBIDIR)
PSWFLAGS = -a -r -S shlib.h
LDFLAGS =

#  **4**  -INCLUDE FILES-
#  Private include files are used by the library's code, but not needed
#  by the library's users.  Public include files are needed by others who
#  will use the library, and must be installed along with the library.
PRIVATE_INCFILES = defs.h mask_ops.h
PUBLIC_INCFILES = dpsNeXT.h dpsclient.h dpsfriends.h event.h

#  **6**  -BY_PRODUCTS GENERATED FROM BUILDS-
#  If your build procedure creates any files other than .o files and the
#  .c and .m files left over from pswrap, you should list them here so
#  they are removed when a make clean is done.
BY_PRODUCTS = makeNameTable nameTable.c makeDeadKeyTable deadKeyTable.c \
		.protos_done $(PSWFILES:.psw=.h) wraps.h psops.h

ofiles:: .protos_done

#this file included for standard functionality
include $(MAKE_DIR)/Makefile.lib

#  **7**  -ADDITIONAL INSTALLS-
#  If there are other actions that you need to do during an installation,
#  add them as actions under the below target, "additional_installs".
#  These actions will be carried out right after the library, include
#  files, class, and message group files have been installed.
all: psops.h wraps.h other_wraps
install: common_install wraps_header_install \
	other_wraps_install relnotes_install


#  any extra rules or dependencies can be added after this line
#include Makefile.depends

nameTable.c: makeNameTable.c makeNameTable systemnames.ps namecode.sed
	-rm -f nameTable.c
	sed -f namecode.sed systemnames.ps | ./makeNameTable > nameTable.c

makeNameTable: makeNameTable.c
	$(CC) -O -g $(COMMONCFLAGS) -o makeNameTable makeNameTable.c

namecode.o: nameTable.c


deadKeyTable.c: makeDeadKeyTable.c makeDeadKeyTable deadKeyList
	-rm -f deadKeyTable.c
	./makeDeadKeyTable deadKeyList > deadKeyTable.c

makeDeadKeyTable: makeDeadKeyTable.c
	$(CC) -O -g $(COMMONCFLAGS) -o makeDeadKeyTable makeDeadKeyTable.c

scanner.o: deadKeyTable.c


wraps_header_install: wraps.h
	install -m 444 psops.h wraps.h $(INCDIR)

.protos_done: $(PSWFILES:.psw=.h) $(PSWMFILES:.pswm=.h) wraps.h
	rm -f .protos_done
	touch .protos_done


clean installsrc::
	cd ctxtWraps; $(MAKE) $@ "ORIGSTDPSWFILES=$(STDPSWFILES)" \
					"ORIGNEXTPSWFILES=$(NEXTPSWFILES)"

other_wraps:
	cd ctxtWraps; $(MAKE) all "ORIGSTDPSWFILES=$(STDPSWFILES)" \
					"ORIGNEXTPSWFILES=$(NEXTPSWFILES)"

other_wraps_install:
	cd ctxtWraps; $(MAKE) install "ORIGSTDPSWFILES=$(STDPSWFILES)" \
					"ORIGNEXTPSWFILES=$(NEXTPSWFILES)"


wraps.h: $(NEXTPSWFILES:.psw=.c) psops.h
	-rm -f wraps.h
	echo '#import "psops.h"' > wraps.h
	cat $(NEXTPSWFILES:.psw=.h) >> wraps.h

psops.h: $(STDPSWFILES:.psw=.c)
	-rm -f psops.h
	cat $(STDPSWFILES:.psw=.h) > psops.h

.psw.o:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.c $*.psw
	$(CC) $(CFLAGS) -c $*.c -o $(OFILE_DIR)/$*.o
	
.psw.c:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.c $*.psw

.psw.h:
	$(PSWRAP) $(PSWFLAGS) -h $*.h -o $*.c $*.psw

pmon.c: pmon/pmon_target_lib.c pmon/pmon_targetu_lib.c pmon/pmon_error.c

# nop line so we dont get complaints when these files arent around, since we
# dont install them with the source (since they shoud never ship)
pmon/pmon_target_lib.c pmon/pmon_targetu_lib.c pmon/pmon_error.c:

installsrc::
	if [ -f $(RELNOTES) ];			\
	then					\
	    mkdirs $(SRCROOT);			\
	    cp $(RELNOTES) $(SRCROOT);		\
	fi


relnotes_install:
	if [ -f $(RELNOTES) ];					\
	then							\
	    mkdirs /usr/local/RelNotes;				\
	    install -m 444 $(RELNOTES) /usr/local/RelNotes;	\
	fi

