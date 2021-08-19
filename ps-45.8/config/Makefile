
CONFIGS = mc68020 040fpe sl

.PHONY: $(CONFIGS) help

help:
	@echo Configurations supported are: $(CONFIGS)

$(CONFIGS):
	rm -f ../make-config
	cd .. ; ln  config/make-config-$@ make-config
	mkdirs ../DEVELOP-$@ ../EXPORT-$@
