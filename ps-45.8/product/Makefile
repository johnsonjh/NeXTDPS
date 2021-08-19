ROOT = ..
LIBRARY = product
SRCS =  coordinates.c event.c init_table.c imident.c imstretch.c \
	ipcscheduler.c ipcstream.c \
	listener.c mappedstream.c miscops.c machportdevice.c \
	mouse_driver.c nextmain.c nscontexts.c pscontext.c \
	timelog.c windowdevice.c windowgraphics.c windowimage.c \
	windowops.c wbcontext.c windowbitmap.c dynaloader.c

HDRS =  ipcstream.h ipcscheduler.h nscontexts.h timelog.h \
	timeslice.h wbcontext.h wberror.h wbprotocol.h

# -I$(ROOT)/device needed by machportdevice.c 
#			windowdevice.c windowgraphics.c for framedev.h
OTHERSWITCHES = -I$(ROOT)/device # -DPROF=1

MISC = Makefile

include $(ROOT)/makefile.common

default-goal: $(LIBDIR)/PS.VM

all:
	$(MAKE)  -$(MAKEFLAGS) STAGE=DEVELOP
	$(MAKE)  -$(MAKEFLAGS) STAGE=EXPORT

STREAMKEY = 121201
VERSION = 2000.6
CUSTOMDICTNAME = nextdict
COPYRIGHT = $(ROOT)/include/copyright.h
LINOTYPE = 0

SOURCES = $(ROOT)/$(LIBRARY)
INIT = $(ROOT)/init
INIT_FILES = \
  $(SOURCES)/errs.ps \
  $(INIT)/print.ps \
  $(INIT)/flex.ps \
  $(SOURCES)/frags.ps \
  $(INIT)/unixmain.ps \
  $(SOURCES)/stdencode.ps \
  $(SOURCES)/nextfindfont.ps \
  $(SOURCES)/service.ps \
  $(SOURCES)/nextconstants.ps \
  $(SOURCES)/version.ps \
  $(SOURCES)/unix.ps \
  $(INIT)/stats.ps \
  $(INIT)/systemnames.ps \
  exec_copyright.ps \
  registernames.ps
	
ifeq "$(STAGE)" "DEVELOP"
    VMFLAGS = -i - -S 600 -U 200 -N 60 -C 300 -T 50 -o 100000 -g 640 -G 640
    PSFLAGS = -u _PrintSOP
else
    VMFLAGS = EXPORT -i - -S 500 -U 300 -N 60 -C 300 -T 50 -o 175000 -g 640 -G 640
    PSFLAGS = -e start
endif

exec_copyright.ps : $(COPYRIGHT) Makefile
	rm -f $@
	@echo '% This is a derived file -- do not edit it' > $@
	@echo -n 'execdict /copyright (' >> $@
	/lib/cpp $(COPYRIGHT) -DLINOTYPE=$(LINOTYPE) -DSTUTTER=1 -DBRIEF \
	  $(SWITCHES) -I../include |\
	  sed -n -e '/^".*";/{' -e 'p' -e 'd' -e '}' -e '/^"/,/";/p' |\
	  sed -e 's/^"//' -e 's/";//' >> $@
	@echo ') readonly put' >> $@

NAMES := $(foreach dir,$(PACKAGES),$(wildcard $(ROOT)/$(dir)/$(dir)names.ps))

registernames.ps : $(ROOT)/make-config Makefile
	rm -f $@
	@echo '% registernames.ps' > $@
	@echo '% this is a derived file -- do not edit it' >> $@
	@echo "$(patsubst %,(%) run\n,$(NAMES))" >> $@

LIBS = $(LIB) $(patsubst %, $(LIBDIR)/%.a, $(PACKAGES))
SYS  = -u libsys_s -lsys_s

ifeq "$(STAGE)" "EXPORT"
ifeq "$(LOAD)" ""
else
ifeq "$(LOAD)" "what"
    ORDER = -whatsloaded | fgrep -v libsys | fgrep -v "ld: warning" > ../product/whatsloaded; nm -onjls __TEXT __text `cat ../product/whatsloaded` > ../product/start.order
else
ORDER = -sectorder __TEXT __text ../product/final.order # -sectorder_detail
SCATTERDEPS = dead.order hand1.order hand2.order $(LOAD).order
START = rm -f final.order ; cat dead.order > final.order ; \
        fgrep -v "%" hand1.order >> final.order ; \
        fgrep -v "%" hand2.order >> final.order ; \
        fgrep -v -f hand1.order $(LOAD).order | \
        fgrep -v -f hand2.order | fgrep ".a:" >> final.order; 
# START = cat $(LOAD).order > final.order;
endif
endif
endif

# ConstRGBPattern needed for nd_proj
$(LIBDIR)/PS : $(LIBS) $(SCATTERDEPS)
	$(START) \
	cd $(LIBDIR) ; \
	$(LD) -o PS $(PSFLAGS) -u _ConstRGBPattern -u _packageInitProcedure \
	 -u _remapY -u _wdProcs -u _IniWdDevImpl -u _ImIdent -u _ImStretch \
	 -u _mappedStmProcs \
	 -i_LRenderInBounds:_NXRenderInBounds \
	 -i_os_abort:_abort -i_os_exit:_exit -i_os_bzero:_bzero \
	 -i_os_bcopy:_bcopy -i_os_index:_index -i_os_strcat:_strcat \
	 -i_os_strcmp:_strcmp -i_os_strcpy:_strcpy -i_os_strncpy:_strncpy \
	 -i_os_strlen:_strlen -i_os_calloc:_calloc -i_os_malloc:_malloc \
	 -i_os_realloc:_realloc -i_os_free:_free -i_PSYield:_ContextYield \
	 -i_FGEnterMonitor:_DevNoOp -i_FGExitMonitor:_DevNoOp \
	 -i_os_atan2:_atan2 -i_os_atof:_atof -i_os_ceil:_ceil -i_os_cos:_cos \
	 -i_os_ecvt:_ecvt -i_os_exp:_exp -i_os_fcvt:_fcvt -i_os_floor:_floor \
	 -i_os_frexp:_frexp -i_os_ldexp:_ldexp -i_os_log:_log \
	 -i_os_log10:_log10 -i_os_modf:_modf -i_os_pow:_pow -i_os_sin:_sin \
	 -i_os_sqrt:_sqrt -i_setjmp:__setjmp -i_longjmp:__longjmp \
	 -lpscrt0.o $(notdir $(LIBS)) \
          -seglinkedit -segprot __LINKEDIT rwx r \
	  $(SYS) $(ORDER) 
	size $@
# -y__stack_fork_child 
# -y__malloc_fork_parent
# -lpscrt0.o  -lcrt0.o ~tdonahue/pscrt0.o
# ~perf/bin/pagewatch 2148 /ps/EXPORT-mc68020/PS        -t 20 -d 10               0x00568000 0x00572000 0x00580000 0x00586000 &

DEVELOP_PS = $(ROOT)/DEVELOP-$(OBJTAG)/PS

$(LIBDIR)/PS.VM : $(DEVELOP_PS) $(LIBDIR)/PS makevm.ps $(INIT_FILES) Makefile
	cd $(LIBDIR); rm -f PS.VM makevm.ps
	sed -e "s#/INIT/#$(INIT)/#g" \
	    -e "s#/CUSTOMDICTNAME/#$(CUSTOMDICTNAME)#" \
	    -e "s#/SOURCES/#$(SOURCES)/#g" \
	    -e "/VAX ONLY/d" \
	    -e "/PRODUCT ONLY/d" \
	    makevm.ps > $(LIBDIR)/makevm.ps
	$(DEVELOP_PS) $(VMFLAGS) -K $(STREAMKEY) -V $(VERSION) -e  "{($(LIBDIR)/makevm.ps) run} \
	    stopped {handleerror abort} if ($@) makevm";
	rm -f $(LIBDIR)/makevm.ps

