
all clean: 
	-rm -f *~

install: ${DSTROOT}
	mkdirs $(DSTROOT)/usr/lib/NextPrinter
	cp *.snd $(DSTROOT)/usr/lib/NextPrinter
	chmod 755 $(DSTROOT)/usr/lib/NextPrinter/*.snd
	/etc/chown root $(DSTROOT)/usr/lib/NextPrinter/*.snd

installsrc: ${SRCROOT}
	tar cf - . | (cd ${SRCROOT}; tar xfBp -)


SRCROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1;
	fi

$(SRCROOT):
	mkdirs $@

${DSTROOT}:
	mkdirs -m 755 -o root -g wheel  $@
