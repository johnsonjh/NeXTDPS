
#  Application Makefile
#
#  This Makefile is used to maintain and install NeXT applications.
#
#  Fill in values for the various variables at the places marked by
#  **number**.  Any rules or dependencies of your own can be added
#  at the end of the file.  Some of the options have defaults or
#  examples you may need to change.
#  

#  **1**  -APPLICATION NAME-
#  The name of the application goes here.
NAME = NSWSd

#  **2**  -COMMAND OPTIONS-
#  These are passed as arguments to the assembler, loader, compiler, and 
#  pswrap.  Under CFLAGS, for each library that you use that supplies
#  class and phylum files, you must list a -I$(OBJDIR)/XXX, where XXX
#  is the base name of the library.  The entry for the window kit is
#  provided by default.
LDFLAGS = 
CFLAGS = -g -O

#  **3**  -LIBRARIES-
#  Here list the libraries that the program must be bound with.
#  Prefix the library you want with "-l".  The entry used to get
#  the Application Kit library is provided as a default.
LOADLIBES = -lsys_s

#  **4**  -SOURCE FILES-
#  Here list the .m, .c, .psw, .pswm, and .s files.
#  Also list any local include files used under INCFILES.
#  OTHER_SRCS is used for files with other suffixes that you wish to
#  be treated like source (i.e., printed with the source, copied with
#  the source, etc.).  For files in OTHER_SRCS, you need to specify
#  the corresponding .o file in OTHER_SRCS_OFILES.
MFILES = 
CFILES = NSWSd.c
PSWFILES = 
PSWMFILES = 
SFILES = 
INCFILES = 
OTHER_SRCS = NSWSd.8c Makefile
OTHER_SRCS_OFILES = 

# Beginning of stuff copied from /usr/lib/nib/Makefile.nibapp

# a little doc for when the "help" target is made
.DEFAULT help:
	@if (test "$<");		\
	then echo 'ERROR: Unrecognized Makefile target "$<"';	\
	fi
	echo '  $(NAME)-  to make the local copy of the application';\
	echo '  clean-    to remove all files but the source';\
	echo '  print-    to print out all the source files';\
	echo '  wc-       to get the line, word, and byte count of the source';\
	echo '  size-     to get the size of all object files';\
	echo '  tags-     to run ctags on the source';\

# this is the fruit of this project
PRODUCT = $(NAME)

#####  Heres the start of the stuff from Makefile.common

# command to use for printing source
PRINT = list

# all the object files
OFILES = $(MFILES:.m=.o) $(CFILES:.c=.o) $(PSWFILES:.psw=.o)	\
	$(PSWMFILES:.pswm=.o) $(SFILES:.s=.o) $(LFILES:.l=.o)	\
	$(LMFILES:.lm=.o) $(YFILES:.y=.o) $(YMFILES:.ym=.o)	\
	$(OTHER_SRCS_OFILES) 

# source files derived in the build process
DERIVED_SRC = $(PSWFILES:.psw=.c) $(PSWMFILES:.pswm=.m)

# files that will be removed on make clean, along with .o's
GARBAGE = $(BY_PRODUCTS) TAGS tags $(DERIVED_SRC) core

# all source code
SRCS = $(MFILES) $(CFILES) $(PSWFILES) $(PSWMFILES) $(SFILES) \
	$(LMFILES) $(LFILES) $(YFILES) $(YMFILES) $(OTHER_SRCS)

# removes all derived files
clean:
	-/bin/rm -f $(OFILES) $(GARBAGE) $(PRODUCT)

#  The tags target creates an index on the source files' functions.
tags:: $(MFILES) $(CFILES) $(PSWFILES) $(PSWMFILES) $(LFILES) \
		$(LMFILES) $(YFILES) $(YMFILES) $(HFILES)
	ctags $(MFILES) $(CFILES) $(PSWFILES) $(PSWMFILES) $(LFILES) \
		$(LMFILES) $(YFILES) $(YMFILES) $(HFILES)

#  prints out all source files
print:: $(SRCS)
	$(PRINT) $(SRCS)

# shows line count of source
wc: $(SRCS)
	wc $(SRCS)

# an application is made by first making all its components, and then
# linking the whole thing together.
$(PRODUCT): $(OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PRODUCT) $(OFILES) $(LOADLIBES)

# shows object code size
size: $(OFILES)
	@/bin/size $(OFILES) | awk '{ print; for( i = 0; ++i <= 4;) x[i] += $$i } \
	 END { print x[1] "\t" x[2] "\t" x[3] "\t" x[4] }'


# End of stuff from /usr/lib/nib/Makefile.nibapp

#  any extra rules or dependencies can be added here.

install : NSWSd
	mkdirs $(DSTROOT)/usr/lib/NextStep
	install -c -m 555 -s ./NSWSd $(DSTROOT)/usr/lib/NextStep/NSWSd
	mkdirs $(DSTROOT)/usr/local/doc/NSWSd_proj/man
	install -c -m 666 ./NSWSd.8c $(DSTROOT)/usr/local/doc/NSWSd_proj/man

installsrc : 
	cp $(SRCS) $(INCFILES) $(SRCROOT)




