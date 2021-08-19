/*
  main.c

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Andrew Shore
Edit History:
Andrew Shore: Wednesday, July 13, 1988 11:25:37
Bill Bilodeau: Thu Sep  8 18:14:07 PDT 1988
	added the -r option
Richard Cohn: Fri Oct 21 15:37:13 1988
	merge MPW & UNIX versions
End Edit History.

*/

#ifdef os_mpw
#include <Types.h>
#include <ErrMgr.h>
#include <Errors.h>
#include <CursorCtl.h>
#endif

#include <stdio.h>
#ifdef os_mpw
#include <string.h>
#define rindex strrchr
#define SLASH ':'
#else
#include <strings.h>
#define SLASH '/'
#endif
#include <ctype.h>

#define MIN_MAXSTRING 80	/* min allowable value of maxstring */

/* global data */
char	*prog;			/* program name */
char	*hfile = NULL;		/* name of -h file */
char	*ofile = NULL;		/* name of -o file */
char	*ifile = NULL;		/* name of input file */
int	gotInFile = 0;		/* got an explicit input file name */
int	doANSI = 0;			/* got -a (ansi) flag for -h file */
/* defaults to 1 on NeXT system -trey */
int pad = 1;			/* got -p (padding) flag */
int	reentrant = 0;		/* automatic vars for generated BOS */
int	bigFile = 0;		/* got -b flag => call free */
FILE	*header;		/* stream for -h file output */
char	headid[200];		/* id for header file #include */
int	maxstring = 2000;	/* -s max string length to scan */
char	*string_temp;		/* string buffer of above size */
int	outlineno = 1;		/* output line number */
int	nWraps = 0;		/* total number of wraps */
#ifdef os_mach
char	*shlibInclude = NULL;	/* special file to be #included at top of */
                      			/* file.  Used only when building shlibs */
#endif
#ifdef os_mpw
char	*rfile = NULL;		/* name of -r file */
char	*resID = "128";		/* resource id */
char	*pswtmpfile;		/* name of res data file */
FILE	*rdata, *rtype;		/* stream for resource data and type description */
int	sysWrap = 0;		/* got -p flag (get wrap from PostScript.VM) */
char	errorBuffer[256];	/* space to store text from GetSysErrText */
#endif

extern char *malloc();
extern char *psw_malloc();
extern InitWellKnownPSNames();
#ifdef os_mpw
extern char *MakeTempFilename();	/* for res data file */
#endif

/* MPW performance monitoring */
#ifdef mpw_perf
#include <Perf.h>
TP2PerfGlobals perfGlobals = NULL;
#endif

main(argc,argv)
int argc; char *argv[];
{
	static void ScanArgs();
	static void Usage();
	static void FatalError();
    extern int	errorCount;	/* non-fatal errs */
    int		retval;		/* return from yyparse */
	
#ifdef os_mpw
    InitCursorCtl(NULL);
#endif

    ScanArgs(argc, argv);

    if (ifile == NULL)
	ifile = "stdin";
    else {
	gotInFile = 1;
	if (freopen(ifile,"r",stdin) == NULL)
	    FatalError("can't open %s for input",ifile);
    }
    if ((string_temp = (char *) malloc((unsigned) (maxstring+1))) == 0)
	FatalError("can't allocate %d char string; try a smaller -s value",maxstring);
    if (ofile == NULL)
		ofile = "stdout";
    else {
#ifdef os_mach
 		(void)unlink(ofile);
#endif os_mach
    	if (freopen(ofile,"w",stdout) == NULL)
			FatalError("can't open %s for output",ofile);
	}
    InitOFile();

#ifdef os_mpw
    if (rfile == NULL) {
	char *period = rindex(ofile, '.');
	int periodStart = period ? period - ofile : strlen(ofile);
	rfile = psw_malloc(periodStart+3);
	strncpy(rfile, ofile, periodStart);
	strcpy(&rfile[periodStart], ".r");
    }
    if ((rtype = fopen(rfile,"w")) == NULL)
	FatalError("can't open %s for output\n",rfile);
    if ((pswtmpfile = MakeTempFilename("pswrap%d.tmp")) == NULL)
	FatalError("can't open temporary file\n");
    if ((rdata = fopen(pswtmpfile,"w+")) == NULL)
	FatalError("can't open %s for output\n",pswtmpfile);
    InitRFiles(resID);
#endif
    
    if (hfile != NULL) {
#ifdef os_mach
 		(void)unlink(hfile);
#endif os_mach
 		if ((header = fopen(hfile,"w")) == NULL)
 	    	FatalError("can't open %s for output",hfile);
    }
    if (header != NULL)	InitHFile(headid);

    InitWellKnownPSNames();

#ifdef os_mpw
    yy_init();
#endif

#ifdef mpw_perf
    if (! INITPERF(&perfGlobals, 4 /* sample rate (ms) */, 8 /* bucket size(bytes) */,
	true, true, "\pCODE", 0, "\p", false, 0, 0xFFFFF, 16))
	FatalError("INITPERF failed\n");
    PerfControl(perfGlobals, true);
#endif

    if ((retval = yyparse()) != 0)
	fprintf(stderr,"%s: error in parsing %s\n",prog,ifile);
    else if (errorCount != 0) {
	fprintf(stderr,"%s: errors were encountered\n",prog);
	retval = errorCount;
    }

#ifdef mpw_perf
    PerfControl(perfGlobals, false);
    if (PERFDUMP(perfGlobals, "\pperform.out", true, 80))
	fprintf(stderr, "%s: errors during dump\n", prog);
    TermPerf(perfGlobals);
#endif
	
#ifdef os_mpw
    yy_cleanup();
    FinishFiles(resID);
#endif

    if (hfile != NULL) FinishHFile(headid);

    exit (retval);
}

static void ScanArgs(argc, argv)
    int argc;
    char *argv[];
{
	static void Usage();
    extern int	lexdebug;	/* debug flag for lexer */
    char	*slash;		/* index of last / in hfile */
    char	*c;		/* pointer into headid for conversion */
    int 	i = 0;

    prog = argv[i++];
    slash = rindex(prog,SLASH);
    if (slash)
	prog = slash + 1;
    while (i < argc) {
	if (*argv[i] != '-') {
	    if (ifile != NULL) Usage("Only one input file can be specified.");
	    else ifile = argv[i];
	} else {
	    switch (*(argv[i]+1)) {
	    case 'a':
		doANSI++;
		reentrant++;
		break;
	    case 'b':
		bigFile++;
		break;
#if PSWDEBUG
	    case 'd':
	    	lexdebug++;
		break;
#endif
	    case 'h':
		hfile = argv[++i];
		slash = rindex(hfile,SLASH);
		strcpy(headid, slash ? slash+1 : hfile);
		for (c = headid; *c != '\0'; c++) {
		    if (*c == '.') *c = '_';
		    else isascii(*c) && islower(*c) && (*c = toupper(*c));
		}
		break;
	    case 'o':
		ofile = argv[++i];
		break;
	    case 'r':
		reentrant++;
		break;
	    case 's':
		if ((maxstring = atoi(argv[++i])) < MIN_MAXSTRING) {
		    fprintf(stderr,"%s: -s %d is the minimum\n", prog, MIN_MAXSTRING);
		    maxstring = MIN_MAXSTRING;
		}
		break;
	    case 'w':
	    	break;
#ifdef os_mpw
	    case 'i':
		resID = argv[++i];
		break;
	    case 'v':
		sysWrap++;
		break;
	    case 'R':
		rfile = argv[++i];
		break;
#endif
#ifdef os_mach
 	    case 'S':
 		shlibInclude = argv[++i];
 		break;
#endif os_mach
		case 'p':
#ifdef NeXT
			pad = 0;
#else
			pad++;
#endif
			break;
	    default:
		Usage("bad option '-%c'", *(argv[i]+1));
		break;
	    } /* switch */
	} /* else */
	i++;
    } /* while */
} /* ScanArgs */

static void Usage(msg, arg1, arg2, arg3, arg4) 
	char *msg;
{
    fprintf(stderr,"%s:  ", prog);
    fprintf(stderr, msg, arg1, arg2, arg3, arg4);
    fprintf(stderr,"\nUsage:  pswrap [options] [input-file]\n");
    fprintf(stderr,"    -a              produce ANSI C procedure prototypes\n");
    fprintf(stderr,"    -b              process a big file\n");
    fprintf(stderr,"    -h filename     specify header filename\n");
#ifdef os_mpw
    fprintf(stderr,"    -i id           set resource id number\n");
#endif
    fprintf(stderr,"    -o filename     specify output C filename\n");
    fprintf(stderr,"    -r              make wraps re-entrant\n");
#ifdef os_mpw
    fprintf(stderr,"    -R filename     specify resource filename\n");
#endif
    fprintf(stderr,"    -s length       set maximum string length\n");
    exit(1);
}

/* a - d are optional args for fprintf */
static void FatalError(msg, a, b, c, d)
    char *msg;
{
    fprintf(stderr,"%s:  ", prog);
    fprintf(stderr, msg, a, b, c, d);
    fprintf(stderr, "\n");
    exit(1);
} /* FatalError */
