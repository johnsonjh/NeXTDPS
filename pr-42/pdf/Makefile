
all clean: 
	-rm -f *~

install: ${DSTROOT}
	mkdirs $(DSTROOT)/usr/lib/NextPrinter/pdf
	cp *.pdf $(DSTROOT)/usr/lib/NextPrinter/pdf
	chmod 644 $(DSTROOT)/usr/lib/NextPrinter/pdf/*
	/etc/chown root $(DSTROOT)/usr/lib/NextPrinter/pdf/*

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
