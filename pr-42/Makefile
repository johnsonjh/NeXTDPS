#SUBDIR= npcomm npd lpr inform pdf sounds fontloader
SUBDIR=	transcript npcomm npd lpr inform pdf sounds fontloader

RELNOTES= RelNotes.rtf
USER_BOM = pr.user_bom

all depend tags: ALWAYS
	-rm -f *~
	@echo '###############' make $@ for $(DIR) '###############'
	@for i in ${SUBDIR}; \
	do \
		echo =============== make $@ for $$i ===============; \
		(cd $$i; ${MAKE} $@ || exit 1); \
	done
	@echo '###############' make $@ for $(DIR) done '###############'

clean: ALWAYS
	-rm -f *~
	@echo '###############' make $@ for $(DIR) '###############'
	@for i in ${SUBDIR}; \
	do \
		echo =============== make $@ for $$i ===============; \
		(cd $$i; ${MAKE} $@ || exit 1); \
	done
	@echo '###############' make $@ for $(DIR) done '###############'

install: DSTROOT ALWAYS
	@echo '###############' make $@ for $(DIR) '###############'
	@CWD=`pwd`; cd ${DSTROOT}; DSTROOT=`pwd`; cd $$CWD; \
	if [ -f $(RELNOTES) ]; \
	then \
	    mkdirs $(DSTROOT)/usr/local/RelNotes ; \
	    install -m 444 $(RELNOTES) \
		$(DSTROOT)/usr/local/RelNotes/Printing.rtf ; \
	fi
	for i in ${SUBDIR}; \
	do \
		echo =============== make $@ for $$i ===============; \
		(cd $$i; ${MAKE} DSTROOT=$$DSTROOT $@ || exit 1); \
	done
	mkdirs $(DSTROOT)/usr/local/BOMs
	install -m 644 $(USER_BOM) $(DSTROOT)/usr/local/BOMs/pr.user_bom
	@echo '###############' make $@ for $(DIR) done '###############'

installsrc: SRCROOT
	mkdirs ${SRCROOT}
	tar cf - . | (cd ${SRCROOT}; tar xfBp -)


DSTROOT SRCROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

ALWAYS:
