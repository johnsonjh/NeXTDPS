include make-config

DEPENDFILES := $(addsuffix /depend, $(PACKAGES) product)
TAGFILES := $(addsuffix /tagfiles, $(PACKAGES) product)

.PHONY: $(PACKAGES) product all install installsrc installbin installheaders \
	installdoc local remote clean SRCROOT depend $(DEPENDFILES) \
	changedfiles tidy tags $(TAGFILES) codesize

PS = EXPORT-$(OBJTAG)/PS
VM = EXPORT-$(OBJTAG)/PS.VM
PRINTING = product/printing.ps

help:
	@more doc/make.help
all:
	$(MAKE) product -$(MAKEFLAGS) STAGE=DEVELOP
	$(MAKE) product -$(MAKEFLAGS) STAGE=EXPORT

product: $(PACKAGES)
	$(MAKE) -C product -$(MAKEFLAGS)

depend: $(DEPENDFILES)

$(DEPENDFILES):
	rm -f $@
	$(MAKE) -C $(@D) $(@F) -$(MAKEFLAGS)

$(PACKAGES):
	$(MAKE) -C $@ -$(MAKEFLAGS)

install: DSTROOT all installheaders installbin installdoc 
	mkdirs $(DSTROOT)/usr/local/BOMs
	install -c -m 444 ps.user_bom $(DSTROOT)/usr/local/BOMs/ps.user_bom

installdoc: DSTROOT
	mkdirs -m 755 $(DSTROOT)/usr/local/RelNotes
	install -c -m 444 doc/WindowServerNotes.rtf \
		$(DSTROOT)/usr/local/RelNotes/WindowServerNotes.rtf

installtest: DSTROOT
	@echo Moving WindowServer to .old in $(DSTROOT)/usr/lib/NextStep ...
	mkdirs -m 755 $(DSTROOT)/usr/lib/NextStep
	-mv $(DSTROOT)/usr/lib/NextStep/WindowServer \
	    $(DSTROOT)/usr/lib/NextStep/WindowServer.old
	@echo Moving PS.VM to PS.VM.old in $(DSTROOT)/usr/lib/NextStep ...
	-mv -f $(DSTROOT)/usr/lib/NextStep/PS.VM \
	    $(DSTROOT)/usr/lib/NextStep/PS.VM.old
	@echo Copying PS to $(DSTROOT)/usr/lib/NextStep ...
	install -m 755 -c $(PS) $(DSTROOT)/usr/lib/NextStep/WindowServer
	@echo Copying PS.VM to $(DSTROOT)/usr/lib/NextStep ...
	install -c -m 644 $(VM) $(DSTROOT)/usr/lib/NextStep/PS.VM
	install -c -m 444 $(PRINTING) $(DSTROOT)/usr/lib/NextStep/printing.ps

installbin: DSTROOT
	@echo Copying WindowServer to $(DSTROOT)/usr/lib/NextStep ...
	mkdirs -m 755 $(DSTROOT)/usr/lib/NextStep
	install -m 755 -c $(PS) $(DSTROOT)/usr/lib/NextStep/WindowServer
#	strip -s $(SOURCES)striplist $(DSTROOT)/usr/lib/NextStep/WindowServer
	strip -x $(DSTROOT)/usr/lib/NextStep/WindowServer
	@echo Copying PS.VM to $(DSTROOT)/usr/lib/NextStep ...
	install -c -m 644 $(VM) $(DSTROOT)/usr/lib/NextStep/PS.VM
	install -c -m 444 $(PRINTING) $(DSTROOT)/usr/lib/NextStep/printing.ps
	mkdirs -m 755 $(DSTROOT)/usr/lib/NextStep/Displays

installheaders: DSTROOT
	@echo Copying header files to $(DSTROOT)/usr/local/include/ps ...
	mkdirs -m 755 $(DSTROOT)/usr/local/include/ps
	mkdirs -m 755 $(DSTROOT)/usr/local/include/windowserver
	mkdirs -m 755 $(DSTROOT)/usr/include/windowserver
	cd include; /bin/cp bintree.h \
	    bitmap.h \
	    customops.h \
	    devcreate.h \
	    device.h \
	    ../device/devmark.h \
	    devicetypes.h \
	    devimage.h \
	    devpattern.h \
	    environment.h \
	    except.h \
	    filesystem.h \
	    filetypes.h \
	    foreground.h \
	    fp.h \
	    postscript.h \
	    pslib.h \
	    publictypes.h \
	    stream.h \
	    unixstream.h \
	    $(DSTROOT)/usr/local/include/ps
	/bin/cp include/imagemessage.h $(DSTROOT)/usr/local/include/windowserver
	/bin/cp include/printmessage.h $(DSTROOT)/usr/include/windowserver
	chmod 444 $(DSTROOT)/usr/local/include/ps/*
	chmod 444 $(DSTROOT)/usr/local/include/windowserver/*
	chmod 444 $(DSTROOT)/usr/include/windowserver/*

installsrc: SRCROOT
	tar -cvf - $(PACKAGES) product doc bin include init config \
		Makefile make-config makefile.common ps.user_bom \
		GNUmakefile tags TAGS | \
	(cd $(SRCROOT); tar -xvf -)
	mkdirs -m 755 $(SRCROOT)/DEVELOP-$(OBJTAG) $(SRCROOT)/EXPORT-$(OBJTAG)

SRCROOT DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; \
	else echo Must define $@; exit 1; fi

# local - allows object and binary directories to live on local disk.
local: DSTROOT
	ln -s `pwd`/* $(DSTROOT)
	rm -rf $(DSTROOT)/DEVELOP-$(OBJTAG) $(DSTROOT)/EXPORT-$(OBJTAG)
	rm -rf DEVELOP-$(OBJTAG)/.gdbinit EXPORT-$(OBJTAG)/.gdbinit
	cp -rp DEVELOP-$(OBJTAG) EXPORT-$(OBJTAG) $(DSTROOT)
	rm -rf DEVELOP-$(OBJTAG) EXPORT-$(OBJTAG)
	ln -s $(DSTROOT)/DEVELOP-$(OBJTAG) $(DSTROOT)/EXPORT-$(OBJTAG) .
	ln -s ../config/gdbinit $(DSTROOT)/DEVELOP-$(OBJTAG)/.gdbinit
	ln -s ../config/gdbinit $(DSTROOT)/EXPORT-$(OBJTAG)/.gdbinit

remote:
	rm -rf DEVELOP-$(OBJTAG)/.gdbinit EXPORT-$(OBJTAG)/.gdbinit
	cp -rp DEVELOP-$(OBJTAG) .DEVELOP-$(OBJTAG)
	rm -rf DEVELOP-$(OBJTAG)
	mv .DEVELOP-$(OBJTAG) DEVELOP-$(OBJTAG)
	cp -rp EXPORT-$(OBJTAG) .EXPORT-$(OBJTAG)
	rm -rf EXPORT-$(OBJTAG)
	mv .EXPORT-$(OBJTAG) EXPORT-$(OBJTAG)
	ln -s ../config/gdbinit DEVELOP-$(OBJTAG)/.gdbinit
	ln -s ../config/gdbinit EXPORT-$(OBJTAG)/.gdbinit

tidy:
	rm -f *~ */*~ #*# */#*# *OLD */*OLD *MERGE */*MERGE *NEW */*NEW
clean: tidy
	rm -rf DEVELOP-$(OBJTAG)/* EXPORT-$(OBJTAG)/* $(DEPENDFILES)

changedfiles:
	diff_clone -fP

codesize:
	-@rm -f doc/codesize
	@cd EXPORT-$(OBJTAG); /bin/csh -fc "../bin/codesize |& tee ../doc/codesize"

tags: $(TAGFILES)
	-@rm -f TAGS tags
	@echo include/*.h >> tagfiles
	-ctags -f tagfiles
	-xargs etags -ea < tagfiles
	-@rm -f tagfiles

$(TAGFILES):
	$(MAKE) -C $(@D) $(@F) -$(MAKEFLAGS)
