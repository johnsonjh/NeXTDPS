# start NeXT additions

SOURCES = 
PSSYSNAMES = ./systemnames.ps
SRC = makefile dpsfriends.h lexer.l lexfix main.c psw.c psw.h pswdict.c \
	pswdict.h pswfile.c pswparser.y pswpriv.h pswsemantics.c \
	pswsemantics.h pswstring.c pswtypes.h pswversion.h sysnames2c.awk \
	systemnames.c systemnames.ps yaccfix yyerror.c pswrap.1
AWK = awk
CFLAGS = -c -g -Dos_unix -Dos_mach

# end NeXT additions

OBJECTFILES = \
	main.o\
	pswparser.o\
	lexer.o\
	pswdict.o\
	pswfile.o\
	yyerror.o\
	systemnames.o\
	psw.o\
	pswstring.o\
	pswsemantics.o

TIDYLIST = $(SOURCES)*% *% *~ core y.output

MACLIST = \
	$(SOURCES)systemnames.r \
	$(SOURCES)macparser.r \
	$(SOURCES)lexer1.c\
	$(SOURCES)lexerdata.c

SCRATCHLIST = \
	$(TIDYLIST) \
	$(MACLIST) \
	*.o \
	lexer.c \
	pswparser.c \
	y.tab.h \
	sysnames_gen.c \
	pswrap

TAGSLIST = $(SOURCES)*.h $(SOURCES)*.c
FRIENDSFILE = '"<dpsclient/dpsfriends.h>"'
#FRIENDSFILE = '"\"dpsfriends.h\""'

main.o : $(SOURCES)main.c
	${CC} $(CFLAGS) $(SOURCES)main.c

lexer.o : lexer.c y.tab.h
	${CC} $(CFLAGS) lexer.c

psw.o :	$(SOURCES)psw.c \
	$(SOURCES)pswdict.h $(SOURCES)psw.h $(SOURCES)pswpriv.h $(SOURCES)pswtypes.h
	${CC} $(CFLAGS) $(SOURCES)psw.c

pswdict.o: $(SOURCES)pswdict.c $(SOURCES)pswdict.h $(SOURCES)pswtypes.h
	${CC} $(CFLAGS) $(SOURCES)pswdict.c

pswfile.o: $(SOURCES)pswfile.c $(SOURCES)pswversion.h
	${CC} $(CFLAGS) -DFRIENDSFILE=$(FRIENDSFILE) $(SOURCES)pswfile.c

pswparser.o : pswparser.c $(SOURCES)psw.h
	${CC} $(CFLAGS) pswparser.c

pswsemantics.o : $(SOURCES)pswsemantics.c $(SOURCES)pswdict.h $(SOURCES)psw.h \
	$(SOURCES)pswpriv.h $(SOURCES)pswtypes.h 
	${CC} $(CFLAGS) $(SOURCES)pswsemantics.c

pswstring.o : $(SOURCES)pswstring.c
	${CC} $(CFLAGS) $(SOURCES)pswstring.c

systemnames.o : $(SOURCES)systemnames.c $(SOURCES)sysnames_gen.c
	${CC} $(CFLAGS) $(SOURCES)systemnames.c

yyerror.o : $(SOURCES)yyerror.c
	${CC} $(CFLAGS) $(SOURCES)yyerror.c

lexer.c : $(SOURCES)lexer.l $(SOURCES)pswpriv.h $(SOURCES)pswtypes.h
	${LEX} $(SOURCES)lexer.l
	${MV} lex.yy.c $@

y.tab.h pswparser.c : $(SOURCES)pswparser.y
	${YACC} -d $(SOURCES)pswparser.y
	${MV} y.tab.c pswparser.c

$(SOURCES)sysnames_gen.c: ${PSSYSNAMES} $(SOURCES)sysnames2c.awk
	${AWK} -f $(SOURCES)sysnames2c.awk ${PSSYSNAMES} > $@

$(SOURCES)systemnames.r: $(PSSYSNAMES)
	$(SOURCES)sysnames2r $(PSSYSNAMES) > $(SOURCES)systemnames.r

$(SOURCES)macparser.r: pswparser.c
	${AWK} -f yaccfix pswparser.c > $(SOURCES)macparser.r
	cat $(SOURCES)macparserinit.c >> $(SOURCES)macparser.c

lexer1.c $(SOURCES)lexerdata.c: $(SOURCES)lexer.c
	$(SOURES)lexfix

#NeXT additions below

all pswrap : ${OBJECTFILES}
	${CC} -g -o pswrap ${OBJECTFILES} 

clean :
	rm -f $(SCRATCHLIST)

install: pswrap
	mkdirs $(DSTROOT)/usr/bin
	install -c -s pswrap $(DSTROOT)/usr/bin
	mkdirs $(DSTROOT)/usr/local/doc/pswrap/man
	install -c pswrap.1 $(DSTROOT)/usr/local/doc/pswrap/man

installsrc:
	mkdirs $(SRCROOT)
	cp $(SRC) $(SRCROOT)
