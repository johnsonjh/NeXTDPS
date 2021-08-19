
# standard bsd make(1) makefile, just dispatches to gnumake
#
#	The real guts of the system wide makes are in GNUmakefile
#	which is gnumake's default.
#

all: 
	gnumake $(MFLAGS) -f GNUmakefile all LOAD=gmon

install: 
	gnumake $(MFLAGS) -f GNUmakefile install DSTROOT=$(DSTROOT) LOAD=gmon

installsrc:
	gnumake $(MFLAGS) -f GNUmakefile installsrc SRCROOT=$(SRCROOT)

clean:
	gnumake $(MFLAGS) -f GNUmakefile clean
