#ifndef lint
#define _NOTICE static char
_NOTICE N1[] = "Copyright (c) 1985,1987 Adobe Systems Incorporated";
_NOTICE N2[] = "GOVERNMENT END USERS: See Notice file in TranScript library directory";
_NOTICE N3[] = "-- probably /usr/lib/ps/Notice";
_NOTICE RCSID[]="$Header: pscat.c,v 2.2 87/11/17 16:50:54 byron Rel $";
#endif
/* pscat.c
 *
 * Copyright (C) 1985,1987 Adobe Systems Incorporated. All rights reserved.
 * GOVERNMENT END USERS: See Notice file in TranScript library directory
 * -- probably /usr/lib/ps/Notice
 *
 * troff C/A/T file to PostScript converter
 *
 * RCSLOG:
 * $Log:	pscat.c,v $
 * Revision 2.2  87/11/17  16:50:54  byron
 * Release 2.1
 * 
 * Revision 2.1.1.6  87/11/12  13:40:46  byron
 * Changed Government user's notice.
 * 
 * Revision 2.1.1.5  87/07/08  18:36:16  byron
 * Fixed default correspondence table name. Times.ct -> font.ct.
 * 
 * Revision 2.1.1.4  87/04/23  10:25:57  byron
 * Copyright notice.
 * 
 * Revision 2.1.1.3  87/04/22  10:04:23  byron
 * Normal exit now returns 0 explicitly.
 * 
 * Revision 2.1.1.2  87/04/21  13:28:22  byron
 * 1. The first char on a page was occasionally painted in the wrong place.
 *    Added code to invalidate curPS* in BOP().
 * 2. PPROC's are occasionally painted in the wrong place.  Changed how
 *    the PPROC case of CATForw() handled pendmove and curPS*.
 * Jeff Gilliam reported these problems and fixes.
 * 
 * Revision 2.1.1.1  86/03/25  14:01:17  shore
 * change CATy page offset for BSD systems
 * 
 * Revision 2.1  85/11/24  11:49:56  shore
 * Product Release 2.0
 * 
 * Revision 1.4  85/11/21  14:25:44  shore
 * removed bogus check on options processing
 * 
 * Revision 1.3  85/11/20  00:29:35  shore
 * support for System V
 * better options processing (getopt!)
 * int/long clarifications
 * 
 * Revision 1.2  85/05/14  11:23:17  shore
 * fixed y initialization
 * fixed init EOP case
 * 
 *
 */

#include <stdio.h>
#include <pwd.h>
#ifdef SYSV
extern struct passwd *getpwuid();
#include <string.h>
#else
#include <strings.h>
#endif

#include "transcript.h"
#include "psspool.h"
#include "action.h"

extern char *ctime();
extern long time();

extern char *optarg;
extern int optind;
extern int opterr;

#ifdef DEBUG
#define debug1(s) printf s
#else
#define debug1(s) 
#endif

#define USAGE "pscat [-F fonttable] [-i prologuefile] [-l pagelength] [-x distance] [-y distance] [file]"

/* tolerable error (in big CAT's)
 * between CAT position and PostScript position before
 * we force an absolute moveto
 * (should be a user-specified global?)
 */

#define tolerableError 18L

#define CATperinch 432
#define PSperinch 4320L
#define PointsperPS 60		/* for font sizes only (10) */
#define PSperCAT 10L
#define CATperpoint 6

private char *prog;			/* argv[0] - program name */

/* current CAT state */
private int	railmag;		/* current rail, mag (and tilt) */
private int	escd;			/* direction of escapement */
private int	verd;			/* direction of leading */
private int	mcase;			/* current case */
private long	CATx, CATy;		/* current cat position */
private struct sizeTable *CATfontsize;	/* current font size/offset pointer */


/* current PS state */
private int	PSfont;			/* current PS font */
private long	curPSx, curPSy;		/* current PS position in PSunits */
private long	wantPSx, wantPSy;	/* wanted  PS position in PSunits */
private int	PSsize;			/* current font size in PSunits */

private long	pagelength = (11L * CATperinch);
private int	pagenumber = 1;
private int	donepage = 0;
private long	xoffset = 0;		/* user page offset in cats */
private long	yoffset = 0;

private char	*fonttable = NULL;	/* name of font charmap table */
private int	havemap = 0;		/* true if we read a charmap table */
private char	*prologfile = NULL;	/* name of prolog file */
private char 	*FileName = NULL;	/* name of current input file */
private int	SeenFile = 0;		/* true if seen an input file */

#define SHOWBUFSIZ 256
private char showbuf[SHOWBUFSIZ+4];		/* string for show */
private int showind = 0;		/* current index into showstr */

private char tempfile[512];

private  struct sizeTable {
    int code;
    int pointSize;	/* in points */
    int catoffset;
} SizeTable [] = {
    0010,	 6,	0,
    0000,	 7,	0,
    0001,	 8,	0,
    0007,	 9,	0,
    0002,	10,	0,
    0003,	11,	0,
    0004,	12,	0,
    0005,	14,	0,
    0011,	16,   -55,
    0006,	18,	0,
    0012,	20,   -55,
    0013,	22,   -55,
    0014,	24,   -55,
    0015,	28,   -55,
    0016,	36,   -55,
    0000,	 0,	0
};

/* pending moveto's for display  ORDER IMPORTANT! */

#define pendNONE	0
#define pendX		1
#define pendY		2
#define pendXY		3

private long	pendingX, pendingY;

/* movetypes are used in generating show commands 
 * indexed by pendingmove (NONE, X, Y, XY)
 * thus R is just show
 * S is setx and show
 * T is sety and show
 * U is setxy and show
 */
private char movetypes[] = "RSTU";
private int pendingmove = pendXY;

#define MAXFONTS 25
private char fonts[MAXFONTS][80];
private int nfonts;

#define MAXCHARS 1000
private struct map map[MAXCHARS];

private int	maxcc;	/* max catcode in map */

#ifdef NeXT_MOD
private VOID xCATfile();
private DoEOP();
private DoBOP();
private CATChangeSize();
private CATBack();
private FlushBack();
private CATForw();
private long labs();
private OutChar();
private SetFont();
private ShowChar();
private FlushShow();
private MoveTo();
private LoadMap();
private int ReadDistanceinPoints();
#endif NeXT_MOD

/* xCATfile:
 *	copy (translate) the standard input (cat file) to the
 *	standard output (PostScript file)
 */


private VOID xCATfile()
{
    register int b;
    int esc; /* escapement */
    int lead;
    char hostname[255];		/* should be defined somewhere! */
    char *libdir;
    struct passwd *pswd;
    long clock;

    int i;

    if (!havemap) {
#ifdef NeXT_DEBUG
        printf( "%%%% calling LoadMap\n" );
#endif
	LoadMap(mstrcat(tempfile, TroffFontDir, "font.ct", sizeof tempfile));
    }

    /* generate comment header */
    printf("%%!%s\n",COMMENTVERSION);
    printf("%%%%Creator: cheese");
    pswd = getpwuid(getuid());
    if (gethostname(hostname, (int) (sizeof hostname))) {
	pexit(prog, THROW_AWAY);
    }
    printf("%s:%s (%s)\n",hostname, pswd->pw_name,
    	pswd->pw_gecos);
    printf("%%%%Title: %s\n", FileName);
    printf("%%%%CreationDate: %s",(VOIDC time(&clock),ctime(&clock)));
    printf("%%%%DocumentFonts:");
    for (i = 1; i < nfonts; i++) {
	printf(" %s",fonts[i]);
    }
    printf("\n%%%%Pages: (atend)\n%%%%EndComments\n");

    /* now the fixed prolog */
    if (prologfile) {
	if (copyfile(prologfile, stdout) != 0) {
	    fprintf(stderr,"%s: can't copy prolog file %s\n",prog,prologfile);
	    exit(THROW_AWAY);
	}
    }
    else {
	if ((libdir = envget("PSLIBDIR")) == NULL) libdir = LibDir;
	mstrcat(tempfile, libdir, CATPRO, sizeof tempfile);
	if ((copyfile(tempfile, stdout)) != 0) {
	    fprintf(stderr,"%s: can't copy prolog file %s\n",prog, tempfile);
	    exit(THROW_AWAY);
	}
    }

    /* do the "variable" part of the prolog -- the font list */
    if ((xoffset != 0) || (yoffset != 0)) {
	printf("/xo %ld def /yo %ld def\n",xoffset, yoffset);
    }
    printf("/catfonts [\n");
    for (i = 1; i < nfonts; i++) {
	printf("\t/%s findfont\n",fonts[i]);
    }
    printf("\t] def\n");
    printf("%%%%EndProlog\n");

    while ((b = getchar()) != EOF) {
	if (b == 0) continue;
	if (b & 0200) {
	    /* escapement */
	    esc = (~b) & 0177;
	    if (escd) esc = -esc;
	    debug1(("%% esc %d\n",esc));
	    CATx += esc;
	    continue;
	}
	if ((b & 0377) < 0100)	{
	    /* normal character */
	    if (!donepage) {
		DoBOP();
		donepage = 1;
	    }
	    if (escd) {
		CATBack((b & 077) | mcase);
	    }
	    else {
		CATForw((b & 077) | mcase, CATx, CATy, CATfontsize, railmag);
	    }
	    continue;
	}
	else switch (b) {
	    case 0100:	/* initialize */
	        debug1(("%% init\n"));
		/* take care of internal inits - RKF@washington */
		if (donepage) DoEOP();

	    	CATx = 0;
		/* HELP! the correct initial value of CATy seems to
		 * be fuzzy; different troff's seem to have different
		 * notions of where the page starts!!!
		 */
#ifdef SYSV	
		CATy = 0; /* -159? bogus page offset */
#else
		CATy = -159; /* for BSD systems ? */
#endif
		esc = escd = verd = mcase = railmag = 0;
		CATfontsize = &SizeTable[4];
		donepage = 0;
		break;
	    case 0101:	/* lower rail */
	        debug1(("%% lrail\n"));
		railmag &= ~01;
		continue;
	    case 0102:	/* upper rail */
	        debug1(("%% urail\n"));
		railmag |= 01;
		continue;
	    case 0103:		/* upper mag */
	        debug1(("%% umag\n"));
	    	railmag |= 02;
		continue;
	    case 0104:		/* lower mag */
	        debug1(("%% lmag\n"));
		railmag &= ~02;
		continue;
	    case 0105:		/* lower case */
	        debug1(("%% lcase\n"));
		mcase = 0;
		continue;
	    case 0106:		/* upper case */
	        debug1(("%% ucase\n"));
		mcase = 0100;
		continue;
	    case 0107:		/* escape forward */
	        debug1(("%% forward\n"));
		FlushBack();	/* flush any backwards characters we had */
		escd = 0;
		continue;
	    case 0110:		/* escape backward */
	        debug1(("%% back\n"));
		FlushShow(); 	/* flush any forward characters we had */
		escd = 1;
		continue;
	    case 0111:		/* stop */
	        debug1(("%% stop\n"));
	    	continue;
	    case 0112:		/* lead forward */
	        debug1(("%% lead forward\n"));
		verd = 0;
		continue;
	    case 0113:		/* undefined */
	        debug1(("%% 0113 undef\n"));
		continue;
	    case 0114:		/* lead backward */
	        debug1(("%% lead back\n"));
		verd = 1;
		continue;
	    case 0115:		/* undefined */
	        debug1(("%% 0115 undef\n"));
	    	continue;
	    case 0116:		/* tilt up - not really used */
	        debug1(("%% tilt up\n"));
	    	railmag |= 04;
		continue;
	    case 0117:		/* tilt down - not really used */
	        debug1(("%% tilt down\n"));
	    	railmag &= ~04;
	    	continue;

	    default:
	    	if ((b & 0340) == 0140) {/* leading */
		    lead = (~b) & 037;
		    if (verd) lead = -lead;
		    debug1(("%% lead %d\n", lead));
		    CATy += lead * 3;
		    if (CATy > pagelength) {
			DoEOP();
			CATy -= pagelength;
		    }
		    continue;
		}
		if ((b & 0360) == 0120) {/* point size change */
		    CATChangeSize(b & 017);
		    debug1(("%% size %o\n", b & 017));
		    continue;
		}
		if (b & 0300) continue;
	}
    }
    DoEOP();
    printf("%%%%Trailer\n");
    printf("pscatsave end restore\n");
    printf("%%%%Pages: %d\n", pagenumber-1);
}

/* end of page */
private DoEOP() {
    FlushShow();
    FlushBack();
    if (donepage)
	printf("EP\n");
    donepage = 0;
}

/* beginning of page */
private DoBOP() {
    donepage = 0;
    PSfont = PSsize = -1;	/* force setfont on new page */
    curPSx = curPSy = -1;   /* Jeff Gilliam -- Invalidate current position */
    printf("%%%%Page: ? %d\nBP\n", pagenumber++);
}

private CATChangeSize(sizeCode)
register int sizeCode;
{
    register struct sizeTable *st;
    for (st = SizeTable; st->pointSize != 0; st++)
    	if (st->code == sizeCode) break;

    CATfontsize = st;
}

private struct {
    int chr;	/* character to show */
    long bx, by;	/* position of show CAT */
    struct sizeTable *size;	/* CAT point size */
    int rm;	/* railmag */
} Backbuf[SHOWBUFSIZ];

private int backind = 0;

private CATBack(ch)
int	ch;
{
    if (backind >= SHOWBUFSIZ)
    	FlushBack();
    Backbuf[backind].chr = ch;
    Backbuf[backind].bx = CATx;
    Backbuf[backind].by = CATy;
    Backbuf[backind].size = CATfontsize;
    Backbuf[backind].rm = railmag;
    backind++;
}

private FlushBack() {
    if (backind == 0) return;
    do {
	backind--;
	CATForw(Backbuf[backind].chr,Backbuf[backind].bx,
		Backbuf[backind].by, Backbuf[backind].size,
		Backbuf[backind].rm);
    } while (backind > 0);
    FlushShow();
}

private CATForw(ch, x, y, size, rm)
int	ch;	/* character code */
long 	x,y;	/* position in CATs */
struct sizeTable *size;	/* font size / offset */
int 	rm;	/* railmag */
{
    struct map *cd;		/* character descriptor */
    int pointsize, offset;

/*    debug1(("%% cat %o %o\n", railmag, ch)); */
    ch += (rm << 7);	/* complete character code */
    cd = &map[ch];
    pointsize = size->pointSize;
    offset = size->catoffset;

    wantPSx = (x + offset) * PSperCAT +  (cd->x * pointsize * PSperCAT)/6;
    wantPSy = y * PSperCAT + (cd->y * pointsize * PSperCAT)/6;

    switch(cd->action) {
	case PFONT:
	    OutChar(cd, pointsize);
	    break;
	case PLIG:
	    switch(ch&0177) {
		case 0126: /* ff */
		    OutChar(&map[(rm<<7)+0014],pointsize); /* f */
		    wantPSx = curPSx;    /* Inhibit moveto for next char */
		    OutChar(&map[(rm<<7)+0014],pointsize); /* f */
		    break;
		case 0131: /* ffi */
		    OutChar(&map[(rm<<7)+0014],pointsize); /* f */
		    wantPSx = curPSx;    /* Inhibit moveto for next char */
		    OutChar(&map[(rm<<7)+0124],pointsize); /* fi */
		    break;
		case 0130: /* ffl */
		    OutChar(&map[(rm<<7)+0014],pointsize); /* f */
		    wantPSx = curPSx;    /* Inhibit moveto for next char */
		    OutChar(&map[(rm<<7)+0125],pointsize); /* fl */
		    break;
		default:
		    fprintf(stderr, "%s: bad ligature 0%o!\n",prog,ch&0177);
		    exit(THROW_AWAY);
	    }
	    break;
	case PPROC:
	    FlushShow();
	    SetFont(cd->font,pointsize);
	    printf("%ld %ld M\n", wantPSx / PSperCAT, wantPSy / PSperCAT);
	    printf("%d %d %d %d %d %d %d %d PS%d\n",
	    	pointsize, ch&0177, rm, cd->pswidth, cd->pschar,
		cd->x, cd->y, cd->wid, cd->pschar);
	    curPSx = curPSy = -1;   /* Jeff Gilliam -- Force "moveto" */
	    break;
	default:
	    break;
    }
}

private long labs(n)
long n;
{
    if (n < 0L) return ((long) -n);
    else return((long) n);
}

private OutChar(cd,pointsize)
struct map *cd;
int pointsize;
{
    char c;
    long lx, ly;

    c = cd->pschar;
    SetFont(cd->font,pointsize);
    /* handle positioning error */
    lx = labs((long)(wantPSx - curPSx));    
    ly = labs((long)(wantPSy - curPSy));
#ifdef NeXT_DEBUG
    printf( "%% lx %ld, ly %ld   wantPSx %d, wantPSy %d, curPSx %d, curPSy %d\n",
     lx, ly, wantPSx, wantPSy, curPSx, curPSy );
#endif    
    if ((lx > tolerableError) || (ly > tolerableError)) {
	FlushShow();
	MoveTo(wantPSx, wantPSy);
	curPSx = wantPSx;
	curPSy = wantPSy;
       }
    ShowChar(c);
#ifdef NeXT_DEBUG
    printf( "%% action %d, character %c, curPSx %d", cd->action, c, curPSx );
#endif
    curPSx += (long) (((long) cd->pswidth * PSsize + 500L) / 1000L);
#ifdef NeXT_DEBUG
    printf( ", new curPSx %d, wantPSx %d, wantPSy %d\n", curPSx, wantPSx, wantPSy );
#endif
}

private SetFont(font,size)
int font;
int size;	/* size in POINTS */
{
    if (PSfont != font) {
	FlushShow();
	PSfont = font;
	printf("%d F\n", PSfont);
    }
    if (PSsize != size * PointsperPS) {
	FlushShow();
	PSsize = size * PointsperPS;	/* convert to PS units */
	printf("%ld Z\n",  PSsize/PSperCAT);
    }
/*    debug1(("%% FONT: %d %d\n", font, size)); */
}    

private ShowChar(cc)
int cc;
{
    if (showind == SHOWBUFSIZ) {FlushShow();}
    if ((cc < ' ') || ('~' < cc)) {
	showbuf[showind++] = '\\';
	showbuf[showind++] = ((cc>>6)&03)+'0';
	showbuf[showind++] = ((cc>>3)&07)+'0';
	showbuf[showind++] = (cc&07)+'0';
	}
    else if ((cc == '\\') || (cc == ')') || (cc == '(')) {
	showbuf[showind++] = '\\';
	showbuf[showind++] = cc;
    }
    else
	showbuf[showind++] = cc;
/*    debug1(("%% show: %c\n", cc)); */
}

private FlushShow() {
    if (showind == 0) return;
    showbuf[showind] = 0;
    switch (pendingmove) {
	case pendX: printf("%ld",pendingX / PSperCAT); break;
	case pendY: printf("%ld",pendingY / PSperCAT); break;
	case pendXY:
		printf("%ld %ld",pendingX / PSperCAT, pendingY / PSperCAT);
		break;
	case pendNONE: break;
	default: {
	    fprintf(stderr,"%s: can't happen\n", prog);
	    exit(THROW_AWAY);
	}
    }
    printf("(%s)%c\n",showbuf,movetypes[pendingmove]);
    pendingmove = pendNONE;
    showind = 0;
}

private MoveTo(x,y) long x,y;{
    if (x == curPSx) pendingmove &= (~pendX) & 03;
    else pendingmove |= pendX;
    if (y == curPSy) pendingmove &= (~pendY) & 03;
    else pendingmove |= pendY;
    pendingX = x;
    pendingY = y;
}

private LoadMap(mapfile)
char *mapfile; {
    FILE *mf; 	/* mapfile */
    int i;

#ifdef NeXT_DEBUG
   printf( "trying LoadMap %s\n", mapfile );
#endif
   
    if ((mf = fopen(mapfile, "r")) == NULL) {
	/* try prepending TroffFontDir to path */
	char libmapfile[512];
	mstrcat(libmapfile, TroffFontDir, mapfile, sizeof libmapfile);
#ifdef NeXT_DEBUG
   printf( "trying LoadMap %s\n", libmapfile );
#endif
   
	if ((mf = fopen(libmapfile, "r")) == NULL) {
	    fprintf(stderr,"%s: can't open ct file (%s) or (%s)\n",
	    		prog, mapfile, libmapfile);
	    exit(THROW_AWAY);
	}
    }

    nfonts = getw(mf);
    debug1(("%% nfonts %d  %d\n",nfonts, sizeof fonts[0]));
    if (fread((char *) fonts,sizeof fonts[0], nfonts, mf) != nfonts) {
	fprintf(stderr,"%s: trouble reading .ct file\n",prog);
	exit(THROW_AWAY);
    }
    for (i = 0; i < nfonts; i++) {
	debug1(("%% FONT: %d %s\n",i,fonts[i]));
    }
    maxcc = getw(mf);
    if (fread((char *) map, sizeof (struct map), maxcc, mf) != maxcc) {
	fprintf(stderr,"%s: trouble reading .ct file\n",prog);
	exit(THROW_AWAY);
    }
    havemap = 1;
    debug1(("%% map: %d\n",maxcc));
/*
This is debugging code
*/
#ifdef NeXT_DEBUG_LOAD_MAP
    printf( "%% character encoding\n" );

    for (i = 0; i < maxcc; i++) {
	printf("%% %d - %d %d %d %d\n",i,
		map[i].pswidth, map[i].font, map[i].pschar, map[i].x );

    }
#endif


}

/* internally represent distances in 432 units per inch (cats) */
private struct units {
    char *name;
    double conv;
} units [] = {
    "",			6.0,	/* points */
    "yards",	    15552.0,
    "feet",	     5184.0,
    "foot",	     5184.0,
    "inches",         432.0,
    "inch",	      432.0,
    "meters",	    17007.9,
    "meter",	    17007.9,
    "metres",	    17007.9,
    "m",    	    17007.9,
    "centimeters",    170.079,
    "centimeter",     170.079,
    "centimetres",    170.079,
    "cm",             170.079,
    "millimeters",     17.0079,
    "millimeter",      17.0079,
    "millimetres",     17.0079,
    "mm",              17.0079,
    "points",		6.0,
    "point",		6.0,
    "cats",		1.0,
    "cat",		1.0,
    "micas",		0.170079,
    "um",		0.0170079,
    0,                  0.0,
};

/* convert a distance spec (no units == points) into cats */
private int ReadDistanceinPoints(dist)
char *dist;
{
    register int i;
    register char *p;
    double value = 0.0;
    double scale = 1.0;
    int seenAny = FALSE;
    int seenPoint = FALSE;
    int negative = FALSE;

    for (p = dist; ; p++) switch(*p) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	    if (!seenPoint) value *= 10.0;
	    else scale /= 10.0;
	    value += scale * (*p - '0');
	    seenAny = TRUE;
	    break;
	case '-':
	    if (seenAny) goto done;
	    negative = seenAny = TRUE;
	    break;
	case '+':
	    if (seenAny) goto done;
	    seenAny = TRUE;
	    break;
	case '.':
	    if (seenPoint) goto done;
	    seenPoint = seenAny = TRUE;
	    break;
	default: goto done;
    }
    done:;

    if (negative) value = -value;

    for (i = 0;  units[i].name != 0;  i++) {
	if (strcmp(units[i].name,p) == 0)
	    return (value * units[i].conv);
    }

    fprintf(stderr,"%s: no such units as \"%s\"\n",prog, p);
    exit(2);
}

main(argc, argv)
int argc;
char *argv[];
{
    register int c;
    prog = *argv;
    opterr = 0; /* no reporting from getopt */
    while ((c = getopt(argc, argv, "F:i:l:x:y:")) != EOF) {
	switch (c) {
	    case 'F':
		if (fonttable != NULL) {
		    fprintf(stderr,"%s: only one -F\n", prog);
		    exit(THROW_AWAY);
		}
		fonttable = optarg;
		LoadMap(fonttable);
		break;
	    case 'i':
		prologfile = optarg;
		break;
	    case 'l':
		pagelength = ReadDistanceinPoints(optarg);
		break;
	    case 'x':
		xoffset = ReadDistanceinPoints(optarg);
		break;
	    case 'y':
		yoffset = ReadDistanceinPoints(optarg);
		break;
	    case '?':
	    default:
	        fprintf(stderr,"usage: %s\n",USAGE);
		exit(THROW_AWAY);
	}
    }
    for (; optind < argc; optind++) {
	FileName = argv[optind];
	if (freopen(FileName, "r", stdin) == NULL) {
	    fprintf (stderr,"%s: can't open input file %s\n", prog, FileName);
	    exit(THROW_AWAY);
	}
	xCATfile();
	VOIDC fclose(stdin);
	SeenFile = 1;
    }
    if (!SeenFile) {
	FileName = "stdin";
	xCATfile();
	VOIDC fclose(stdin);
    }
    exit(0);
}
