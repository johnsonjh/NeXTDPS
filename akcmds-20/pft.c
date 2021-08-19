/*

	pft.c
	
	A program to communicate with the PostScript server.
	
	trey
	September 18,1986
	Copyright 1986, NeXT, Inc.
	
	Modified:
	
	23Sep86 Leo 	Added command line option for port number
	14Dec86 Trey	updated to reflect psprim changes in streams
	14Apr87 Trey	totally changed to use psprims procs
	05May87 Leo     Added command line option for host
*/

#include <stdlib.h>
#include <dpsclient/dpsclient.h>
#include <objc/error.h>
#include <string.h>

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

extern int read();

static char *Host = NULL;	/* the host to hookup to */
static char *Name = 0;		/* the server name to hookup to */
static char *File = NULL;	/* a file to run on startup */
static int justSource = FALSE;	/* source a file and leave? */
static int loadMacros = TRUE;	/* load special = macros? */
DPSContext Ctxt;

static void stdinProc();
static int eventProc();
static void asciiProc();
static void usage();
static void gobbleArgs();
static void parseArgs();

void main(argc, argv)
int             argc;
char           *argv[];
{
    NXEvent e;

    parseArgs( &argc, argv );
    NX_DURING
	Ctxt = DPSCreateContext(Host, Name, asciiProc, NULL);
	printf("Connection to PostScript established.\n");

	if( loadMacros ) {
	    DPSPrintf(Ctxt, "/= [ /= load /exec cvx /flush cvx ] cvx def\n");
	    DPSPrintf(Ctxt, "/== [ /== load /exec cvx /flush cvx ] cvx def\n");
	    DPSPrintf(Ctxt,
		"/pstack [ /pstack load /exec cvx /flush cvx ] cvx def\n");
	}
      /* source a file for them if they wish */
	if( File ) {
	    printf("running file %s\n", File );
	    DPSPrintf( Ctxt, "(%s) run\n", File );
	}
	if( justSource ) {
	    DPSWaitContext(Ctxt);
	    DPSDestroyContext(Ctxt);
	    exit(0);
	}
    
	DPSAddFD( 0, stdinProc, NULL, 1 );
	DPSSetEventFunc( Ctxt, eventProc );
    NX_HANDLER
	switch( NXLocalHandler.code ) {
	    case dps_err_ps:
		fprintf( stderr, "pft: PostScript error while connecting:\n" );
		DPSPrintError( stdout, NXLocalHandler.data2 );
		break;
	    case dps_err_cantConnect:
		fprintf( stderr, "pft: Could not form connection.\n" );
		break;
	    default:
		fprintf( stderr, "pft: Fatal error #%d while connecting.\n",
							NXLocalHandler.code );
	}
	exit(-1);
    NX_ENDHANDLER

restartLoop:
    NX_DURING
	DPSGetEvent( DPS_ALLCONTEXTS, &e, -1, NX_FOREVER, 0 );
	fprintf(stderr, "pft: DPSGetEvent returned unexpectedly.\n");
        exit(-1);
    NX_HANDLER
	switch( NXLocalHandler.code ) {
	    case dps_err_ps:
		DPSPrintError( stderr, NXLocalHandler.data2 );
		goto restartLoop;
	    case dps_err_resultTagCheck:
	    case dps_err_resultTypeCheck:
		fprintf( stderr, "Binary Object returned\n" );
		goto restartLoop;
	    case dps_err_write:
	    case dps_err_connectionClosed:
		fprintf( stderr, "pft: Connection to Window Server closed\n" );
		exit(-1);
	    default:
		fprintf( stderr, "pft: Fatal exception raised #%d\n",
						NXLocalHandler.code );
		exit(-1);
	}
    NX_ENDHANDLER
}

/* a little proc to handle activity on stdin */
void stdinProc( fd, data )
int fd;
char *data;
{
    char buf[256];
    int  num_chars;

    if ((num_chars = read(0, buf, 255)) > 0) {
	DPSWriteData(Ctxt, buf, num_chars);
	DPSFlushContext(Ctxt);
    } else {
	DPSDestroyContext(Ctxt);
	exit(0);
    }
}

static void
asciiProc( ctxt, buf, count )
DPSContext ctxt;
char *buf;
long unsigned int count;
{
    fwrite( buf, sizeof(char), count, stdout );
}

/* little procs to handle events, errors, and tokens */
int eventProc( e )
NXEvent *e;
{
    FILE *fp = stdout;
    static char *eNames[NX_LASTEVENT+1] = {
					"NullEvent",
					"LMouseDown",
					"LMouseUp",
					"RMouseDown",
					"RMouseUp",
					"MouseMoved",
					"LMouseDragged",
					"RMouseDragged",
					"MouseEntered",
					"MouseExited",
					"KeyDown",
					"KeyUp",
					"FlagsChanged",
					"WinChanged",
					"SysDefined",
					"AppDefined",
					"Timer",
					"CursorUpdate"
					};

    if( e->type < sizeof(eNames)/sizeof(char*) )
	fprintf( fp, "%s", eNames[e->type] );
    else
	fprintf( fp, "Unknown" );
    fprintf( fp, " at: %.1f,%.1f", e->location.x, e->location.y );
    fprintf( fp, " time: %d", e->time );
    fprintf( fp, " flags: %#x", e->flags );
    fprintf( fp, " win: %hu", e->window );
    fprintf( fp, " cxn: %x", e->ctxt );
    switch(e->type) {
	case NX_LMOUSEDOWN:
	case NX_LMOUSEUP:
	case NX_RMOUSEDOWN:
	case NX_RMOUSEUP:
	    fprintf( fp, " data: %hd,%d\n", e->data.mouse.eventNum,
						e->data.mouse.click);
	    break;
	case NX_KEYDOWN:
	case NX_KEYUP:
	    fprintf( fp, " data: %hd,%hu,%hu,%hu,%hd\n", e->data.key.repeat,
		     e->data.key.charSet, e->data.key.charCode,
		     e->data.key.keyCode, e->data.key.keyData );
	    break;
	case NX_MOUSEENTERED:
	case NX_MOUSEEXITED:
	    fprintf( fp, " data: %hd,%d,%x\n", e->data.tracking.eventNum,
					e->data.tracking.trackingNum,
					e->data.tracking.userData);
	    break;
	case NX_MOUSEMOVEDMASK:		/* these dont use the data union */
	case NX_LMOUSEDRAGGEDMASK:
	case NX_RMOUSEDRAGGEDMASK:
	case NX_FLAGSCHANGEDMASK:
	    break;
	default:
	    fprintf( fp, " data: %hd,%x,%x\n", e->data.compound.subtype,
						e->data.compound.misc.L[0],
						e->data.compound.misc.L[1] );
	    break;
	}
    return 0;
}


/* routine to parse all the command line args */
static void
parseArgs( argc, argv )
int *argc;
char **argv;
{
    argv++;		/* skip the command itself */
    while( *argv )	    /* while there are more args */
	if( **argv == '-' ) {  /* if its a flag (starts with a '-') */
	    if( !strcmp( *argv, "-f" )) {
		File = *(argv+1);
		gobbleArgs( 2, argc, argv );
	    } else if( !strcmp( *argv, "-NXHost" ) ||
			!strcmp( *argv, "-Host" ) ||
			!strcmp( *argv, "-host" )) {
		Host = *(argv+1);
		gobbleArgs( 2, argc, argv );
	    } else if( !strcmp( *argv, "-r" )) {
		loadMacros = FALSE;
		gobbleArgs( 1, argc, argv );
	    } else if( !strcmp( *argv, "-NXPSName" )) {
		Name = *(argv+1);
		gobbleArgs( 2, argc, argv );
	    } else if( !strcmp( *argv, "-s" )) {
		gobbleArgs( 1, argc, argv );
		justSource = TRUE;
	    } else			/* else its one of our flags */
		usage();
	} else			/* else its not flag at all */
	    usage();
}


/* a little accessory routine to the command line arg parsing that
   removes some args from the argv vector.
*/
static void
gobbleArgs( numToEat, argc, argv )
register int numToEat;
int *argc;
register char **argv;
{
    register char **copySrc;

  /* copy the other args down, overwriting those we wish to gobble */
    for( copySrc = argv+numToEat; *copySrc; *argv++ = *copySrc++ );
    *argv = NULL;		/* leave the list NULL terminated */
    *argc -= numToEat;    
}


/* print out some help and bail out */
static void
usage()
{
    fprintf( stderr, "Usage: pft [optional args...]\n");
    fprintf( stderr,
	    "	-NXHost sets the machine where the PS server is located\n");
    fprintf( stderr,
	    "	-f <file> sources a file of PostScript commands\n");
    fprintf( stderr,
	    "	-s means to source the file and exit\n");
    fprintf( stderr,
	    "	-NXPSName sets the name to lookup for the PS server\n");
    fprintf( stderr,
	    "	-r suppresses loading new definitions for =, == and pstack\n");
    exit(-1);
}

