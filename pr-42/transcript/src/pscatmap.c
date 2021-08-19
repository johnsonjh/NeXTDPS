#ifndef lint
#define _NOTICE static char
_NOTICE N1[] = "Copyright (c) 1985,1987 Adobe Systems Incorporated";
_NOTICE N2[] = "GOVERNMENT END USERS: See Notice file in TranScript library directory";
_NOTICE N3[] = "-- probably /usr/lib/ps/Notice";
_NOTICE RCSID[]="$Header: pscatmap.c,v 2.2 87/11/17 16:51:05 byron Rel $";
#endif
/* pscatmap.c
 *
 * Copyright (C) 1985,1987 Adobe Systems Incorporated. All rights reserved.
 * GOVERNMENT END USERS: See Notice file in TranScript library directory
 * -- probably /usr/lib/ps/Notice
 *
 * Build font width files for troff and correspondence tables
 * for pscat.
 *
 * The correspondence tables are intended to be created by humans for
 * describing a set of 4 fonts to be used by troff: detailing the
 * mapping from troff & C/A/T codes to output actions for pscat
 * These actions take one of three forms:
 *	PFONT	- map to a character in a PostScript font
 *	PLIG	- map to a "fake" ligature
 *	PPROC	- map to a named PostScript procedure
 *	PNONE	- no action
 *
 * PFONT is straightforward, the mapping specifies which character in
 * which PostScript font is desired and the character width is
 * collected from the PostScript "afm" font metrics file format.
 * Note that ascender and descender information is "wired in" to the code;
 * it might be a better idea to "compute" it from the character
 * bounding box information.
 *
 * PLIG is provided so that the correct width for the ligature may
 * be "computed" by this program, rather than hand done by the user.
 * There are 3 ligature types:
 *	0126	- build "ff" out of "f" and "f"
 *	0131	- build "ffi" out of "f" and "fi"
 *	0130	- build "ffl" out of "f" and "fl"
 * This list should probably be expanded to span the space.
 *
 * PPROC provides a general callback mechanism so that users can
 * create and character definition with a PostScript procedure.
 * The procedures are "named" with user-specified numbers, and are
 * called with all information available to them (current position,
 * font size, intended width, railmag, ...).  Such procedures are used
 * for troff characters not in any PostScript font (rules, boxes,
 * constructed braces, etc.), but may also be used as a general
 * "escape hatch" for incorporating ANY PostScript routine into a
 * troff document -- a logo, a scanned image, etc.  The exact
 * calling rules are:
 *	an absolute moveto is performed to the position for this character
 *	The following numbers are pushed on the PS stack:
 *	pointsize
 *	troff character code
 *	"railmag"
 *	character width
 *	procedure "number"
 *	x offset
 *	y offset
 *	width
 *
 *	then a procedure named "PSn" where n is the procedure number
 *	is executed.  It is that procedure's responsibility to image
 *	the character and move by the appropriate width.
 *
 * RCSLOG:
 * $Log:	pscatmap.c,v $
 * Revision 2.2  87/11/17  16:51:05  byron
 * Release 2.1
 * 
 * Revision 2.1.1.6  87/11/12  13:40:54  byron
 * Changed Government user's notice.
 * 
 * Revision 2.1.1.5  87/09/16  11:19:44  byron
 * Fixed uninitialized variable (status).  Came up on IBM RT.
 * 
 * Revision 2.1.1.4  87/04/23  16:00:26  byron
 * Added @INCLUDE.
 * 
 * Revision 2.1.1.3  87/04/23  10:26:04  byron
 * Copyright notice.
 * 
 * Revision 2.1.1.2  86/04/30  08:53:56  shore
 * fixed exit code -- broke installation on some systems
 * 
 * Revision 2.1.1.1  86/03/25  14:51:57  shore
 * parse both new and old AFM files
 * 
 * Revision 2.1  85/11/24  11:50:07  shore
 * Product Release 2.0
 * 
 * Revision 1.3  85/11/20  00:32:45  shore
 * support for System V
 * short names, non ".c" output, better arith.
 * 
 * Revision 1.2  85/05/14  11:24:04  shore
 * 
 * 
 *
 */

#include <stdio.h>
#ifdef SYSV
#include <string.h>
#else
#include <strings.h>
#endif
#include "transcript.h"
#include "action.h"

#define LFONT 1
#define LMAP 2
#define LNONE 3

private char linebuf[200];
private char *libdir;		/* place for AFM files */
#ifdef NeXT_MOD
private char *blddir;		/* DSTROOT during cross builds */
#endif NeXT_MOD

private struct chAction chAction [] = {
    PFONT,	"PFONT",	/* character in PostScript font */
    PLIG,	"PLIG",		/* fake ligatures: ff ffi ffl */
    PPROC,	"PPROC",	/* PostScript procedure outcall */
    PNONE,	"0",		/* null action */
    0,		0
};

#define MAXFONTS 25
private int nfonts;
private struct font {
    char mapname[20];	/* short name in map file e.g., "ROMAN" */
    char afmname[80];	/* name of AFM file for this font (in map file) */
    char psname[80];	/* PS font name from AFM file */
    int  widths[256];   /* PostScript widths of encoded chars (1000/em) */
    /* more here */
} fonts[MAXFONTS];
private char	faces[] = "RIBS0"; /* order important! */

private char	familyname[80];
private char	facenames[4][80];

/* there should be on the order of 4 * 128 characters, so 1000 is generous */

#define MAXCHARS 1000
private struct ctab {
    int		trcode;			/* troff char code */
    int		catcode;		/* CAT char code */
    int		wid;			/* table-driven width */
    int		action;			/* action type */
    int		x, y;			/* x and y offset */
    int		font;			/* PostScript font */
    int		pschar;			/* PS character code */
    int		pswidth;		/* PS width (/1000) */
    char 	*descr;			/* short print description */
} ctab[MAXCHARS];
private int	nchars;

/* Describes an "include" file we are/were reading from */
typedef struct _INCFILE {
    struct _INCFILE *next;    /* The next include file in the list */
    FILE            *incfile; /* The include file we were reading from */
    } incfile;

#define MAXINCLNEST  10  /* Maximum nesting level for include files */
private int includelevel;    /* The number of include files we are nested in */

/* Array of include files (including stdin). curfile[indexlevel]=current file */
private FILE *curfile[MAXINCLNEST];

#ifdef NeXT_MOD
private BuildTable();
private GetFont ();
private ReadAFM();
private GetMap();
private int compCTent();
private WriteTable();
private compTRcode();
private DoWidths();
private dofont();
#endif NeXT_MOD


private BuildTable()
{
    char *c;
    int status = LNONE;
    int gotfamily, gotfaces, gotfonts, gotmap;

    gotfamily = gotfaces = gotfonts = gotmap = 0;

    while( includelevel >= 0 ) {
	while(fgets(linebuf, sizeof linebuf, curfile[includelevel]) != NULL) {
	    /* strip off newline */
	    if ((c = INDEX(linebuf, '\n')) == 0) {
		fprintf(stderr, "line too long \"%s\"\n",linebuf);
		exit(1);
	    }
	    *c = '\0';
	    /* ignore blank or comment (%) lines */
	    if ((*linebuf == '%') || (*linebuf == '\0')) continue;
	    if (*linebuf == '@') {
		/* "command" line */
		/* printf("C: %s\n", linebuf); */
		if (strncmp(&linebuf[1], "FAMILYNAME", 10) == 0) {
		    if (sscanf(linebuf,"@FAMILYNAME %s",familyname) != 1) {
			fprintf(stderr,"bad familyname %s\n",familyname);
			exit(1);
		    }
		    gotfamily++;
		}
		if (strncmp(&linebuf[1], "FACENAMES", 9) == 0) {
		    if (sscanf(linebuf,"@FACENAMES %s %s %s %s",
			    facenames[0],facenames[1],
			    facenames[2],facenames[3]) != 4) {
			fprintf(stderr,"must be four facenames\n");
			exit(1);
		    }
		    gotfaces++;
		}
		if (strncmp(&linebuf[1], "INCLUDE", 7) == 0) {
		    newinclude();
		    continue;
		}
		if (strcmp(&linebuf[1], "BEGINFONTS") == 0) {
		    if ((!gotfamily) || (!gotfaces)) {
			fprintf(stderr,"FAMILYNAME and FACENAMES must come before BEGINFONTS\n");
			exit(1);
		    }
		    VOIDC strcpy(fonts[0].mapname,"0");
		    VOIDC strcpy(fonts[0].afmname,"0");
		    VOIDC strcpy(fonts[0].psname,"0");
		    nfonts = 1;
		    status = LFONT;
		    continue;
		}
		if (strcmp(&linebuf[1], "ENDFONTS") == 0) {
		    gotfonts++;
		    status = LNONE;
		    continue;
		}
		if (strcmp(&linebuf[1], "BEGINMAP") == 0) {
		    if (!gotfonts) {
			fprintf(stderr,"BEGINFONTS/ENDFONTS must come before map\n");
			exit(1);
		    }
		    status = LMAP;
		    continue;
		}
		if (strcmp(&linebuf[1], "ENDMAP") == 0) {
		    status = LNONE;
		    gotmap++;
		    continue;
		}
	    }
	    switch (status) {
		case LFONT:
		    GetFont();
		    break;
		case LMAP:
		    GetMap();
		    break;
		default:
		    break;
	    }
	}
    popinclude();   /* Hit EOF -- pop out one include level */
    }
    if (!gotmap) {
	fprintf(stderr,"missing @ENDMAP\n");
	exit(1);
    }
}

private GetFont () {
    char   *eqp;
    register int    i;
    char   afmpath[200];
#ifdef SYSV
    char   shortname[40];
#endif

    if ((eqp = INDEX(linebuf, '=')) == 0) {
	fprintf(stderr, "bad FONTS line:%s\n",linebuf);
	exit(1);
    }
    *eqp++ = '\0';
    for (i = 0; i < nfonts; i++) {
	if (strcmp (fonts[i].mapname, linebuf) == 0) {
	    fprintf(stderr, "duplicate entry for font %s\n", linebuf);
	    exit(1);
	}
    }
    if (nfonts >= MAXFONTS) {
	fprintf(stderr, "Too many FONTS\n");
	exit(1);
    }
    if (strlen(linebuf) > (sizeof fonts[0].mapname)) {
	fprintf(stderr, "FONT name too long %s\n", linebuf);
	exit(1);
    }
    if (strlen(eqp) > (sizeof fonts[0].afmname)) {
	fprintf(stderr, "FONT name too long %s\n", eqp);
	exit(1);
    }
    VOIDC strcpy(fonts[nfonts].mapname, linebuf);
    VOIDC strcpy(fonts[nfonts].afmname, eqp);

    /* read the font's .afm file to get real widths */
    if (*eqp == '/') {
	VOIDC strcpy(afmpath,eqp);
    }
    else {
#ifdef NeXT_MOD
	VOIDC strcpy(afmpath,blddir);
	VOIDC strcat(afmpath,libdir);
#else
	VOIDC strcpy(afmpath,libdir);
#endif NeXT_MOD
	VOIDC strcat(afmpath,"/");
#ifdef SYSV
	mapname(eqp,shortname);
	VOIDC strcat(afmpath,shortname);
#else
	VOIDC strcat(afmpath, eqp);
#endif	
	VOIDC strcat(afmpath,".afm");
    }
    ReadAFM(afmpath);
    nfonts++;
    return;
}


private ReadAFM(afmfile) char *afmfile; {
    char *c;
    FILE *afm;
    int gotMetrics, gotName, inChars, ccode, cwidth;
    char afmbuf[1000];

    if ((afm = fopen(afmfile, "r")) == NULL) {
	fprintf(stderr,"Can't open afm file %s\n",afmfile);
	exit(1);
    }
    inChars = gotMetrics = gotName = 0;
    while(fgets(afmbuf, sizeof afmbuf, afm) != NULL) {
	/* strip off newline */
	if ((c = INDEX(afmbuf, '\n')) == 0) {
	    fprintf(stderr, "AFM line too long %s\n", afmbuf);
	    exit(1);
	}
	*c = '\0';
	/* ignore blank lines */
	if (*afmbuf == '\0') continue;
	if (strncmp(afmbuf,"StartFontMetrics", 16) == 0) {
	    gotMetrics++;
	    continue;
	}
	if (strncmp(afmbuf,"FontName ", 9) == 0) {
	    VOIDC sscanf(afmbuf,"FontName %s",fonts[nfonts].psname);
	    gotName++;
	    continue;
	}
	if (strcmp(afmbuf,"EndCharMetrics") == 0) {
	    if (!inChars) {
		fprintf(stderr,"AFM: %s without StartCharMetrics\n",afmbuf);
		exit(1);
	    }
	    inChars++;
	    break;
	}
	if (strncmp(afmbuf,"StartCharMetrics", 16) == 0) {
	    inChars++;
	    continue;
	}
	if (inChars == 1) {
	    if (sscanf(afmbuf,"C %d ; WX %d ;",&ccode, &cwidth) != 2) {
		fprintf(stderr, "Bad Character in AFM file %s\n",afmbuf);
		exit(1);
	    }
	    if (ccode == -1) continue; /* skip unencoded chars */
	    if (ccode > 255) {
		fprintf(stderr, "Bad Character Code skipped %s\n", afmbuf);
		continue;
	    }
	    fonts[nfonts].widths[ccode] = cwidth;
	    continue;
	}
    }
    if ((inChars != 2) || (!gotMetrics) || (!gotName)) {
	fprintf(stderr,"improper AFM file %s\n",afmfile);
	exit(1);
    }
    VOIDC fclose(afm);
}


private GetMap(){
    int		trcode;
    char	trfont;
    int		catcode;
    int		wid;
    char	action[10];
    int		x,y;
    char	psfont[20];
    int		pschar;
    char	descr[100];

    char	*fp;
    int		trface;  /* 0 - 3  : R I B S */
    int		pf;
    struct ctab *ch;
    struct chAction *act;

    if (sscanf(linebuf,"%o %c %o %d %s %d %d %s %o \"%[^\"]\"",
        &trcode, &trfont, &catcode, &wid, action, &x, &y,
	psfont, &pschar, descr) != 10) {
	fprintf(stderr,"Bad line %s",linebuf);
    }

    /* verify the integrity of the data we got */
    if ((fp = INDEX(faces, trfont)) == 0) {
	fprintf(stderr, "Bad face code in %s\n", linebuf);
	exit(1);
    }
    trface = fp - faces;
    for (act = chAction; act->actName != 0; act++) {
	if (strcmp(action, act->actName) == 0) break;
    }
    if (act->actName == 0) {
	fprintf(stderr, "Bad action in %s\n", linebuf);
	exit(1);
    }
    for (pf = 0; pf < nfonts; pf++) {
	if (strcmp(fonts[pf].mapname, psfont) == 0) goto gotfont;
    }
    fprintf(stderr, "Bad font (%s) name %s\n", linebuf, psfont);
    exit(1);

    gotfont:
    
    if (nchars >= MAXCHARS) {
	fprintf(stderr,"Too many character definitions\n");
	exit(1);
    }
    ch = &ctab[nchars];
    ch->trcode = trcode;
    ch->catcode = (trface << 7) | catcode;
    ch->action = act->actCode;
    ch->x = x;
    ch->y = y;
    ch->font = pf;
    ch->pschar = pschar;
    if (descr[0]) {
	if ((ch->descr = malloc((unsigned) (strlen(descr)+1))) == NULL) {
	    fprintf(stderr,"malloc failed\n");
	    exit(1);
	}
	VOIDC strcpy(ch->descr, descr);
    }

    /* calculate width in cat units (432 per inch) of 6 pt
     * character (the way troff wants them).
     * 6 pts = 36 cat units
     */

    if (ch->action == PLIG) {/* fake ligature, fake width */
	switch (catcode) {
	    case 0126:	/* ff = f f */
		ch->pswidth = fonts[pf].widths['f'] * 2;
		break;
	    case 0131: /* ffi = f fi */
		ch->pswidth = fonts[pf].widths['f'] + fonts[pf].widths[0256];
		break;
	    case 0130: /* ffl = f fl */
	    	ch->pswidth = fonts[pf].widths['f'] + fonts[pf].widths[0257];
		break;
	    default:
		fprintf(stderr,"Unknown ligature 0%o\n",catcode);
		exit(1);
	}
    }
    else {
	ch->pswidth = fonts[pf].widths[pschar];
    }
    ch->wid = (wid >= 0) ? wid :
    	(int) ( (((float) ch->pswidth * 36.0 ) / 1000.0) + 0.5);
    if (ch->wid > 255) {
	fprintf(stderr,"Scaled width too big!\n");
	exit(1);
    }
    nchars++;
}


private int compCTent(a,b)
struct ctab *a, *b;
{
    if (a->catcode < b->catcode) return -1;
    if (a->catcode > b->catcode) return 1;
    return 0;
}


private WriteTable() {
    int	i, curcode;
    struct ctab *ct;
    struct map map, emptymap;
    char outname[84];
    FILE *mapfile;

    /* write out font mapping */
    if (familyname[0] == 0) {
	fprintf(stderr,"No FAMILYNAME specified!\n");
	exit(1);
    }
    VOIDC strcpy(outname, familyname);
    VOIDC strcat(outname, ".ct");
    if ((mapfile = fopen(outname, "w")) == NULL) {
	fprintf(stderr,"can't open output file %s\n", mapfile);
	exit(1);
    }
    VOIDC putw(nfonts, mapfile);
    for (i = 0; i < nfonts; i++) {
	VOIDC fwrite(fonts[i].psname, sizeof fonts[0].psname, 1, mapfile);
    }
    /* sort ctab by catcode */
    qsort((char *) ctab, nchars, sizeof (struct ctab), compCTent);
    /* write it out */
    VOIDC putw(ctab[nchars-1].catcode,mapfile);
    VOIDC fflush(mapfile);
    emptymap.wid = emptymap.x = emptymap.y = emptymap.font =
    emptymap.pschar = emptymap.action = emptymap.pswidth = 0;

    ct = &ctab[0];
    curcode = 0;
    for (i = 0; i < MAXCHARS; i++) {
	while (curcode < ct->catcode) {
	    VOIDC write(fileno(mapfile), &emptymap, sizeof map);
	    curcode++;
	}
	while ((ct->catcode < curcode) && (ct <= &ctab[nchars])) {
	    ct++;
	    i++;
	}
	if (ct >= &ctab[nchars]) break;
	map.wid = ct->wid;
	map.pswidth = ct->pswidth;
	map.x = ct->x;
	map.y = ct->y;
	map.action = ct->action;
	map.pschar = ct->pschar;
	map.font = ct->font;
	VOIDC write(fileno(mapfile), &map, sizeof map);
	ct++;
	curcode++;
    }
    VOIDC fclose(mapfile);
}


/* called by qsort to compare troff codes */
/* if troff codes are the same, use facecode to decide */

private compTRcode(a,b)struct ctab *a, *b;
{
    register int i;
    i = a->trcode - b->trcode;
    if (i == 0) return ((a->catcode>>7) - (b->catcode>>7));
    return(i);
}


private DoWidths() {
    int f; /* font index 0-3 */
    
    qsort((char *) ctab, nchars, sizeof(struct ctab), compTRcode);
    for (f = 0; f < 4; f++) {
	dofont(f);
    }
}


#define ASCENDER 0200
#define DESCENDER 0100

/* note that both is 0300 */
/* ascenders are:
 *	b d f h i j k l t beta gamma delta zeta theta lambda xi
 *	phi psi Gamma Delta  Theta Lambda Xi Pi Sigma
 *	Upsilon Phi Psi Omega gradient
 */

private char Ascenders[] = "bdfhijklt\231\232\233\235\237\242\245\254\256\260\261\262\263\264\264\265\266\270\271\272\273\326";

/* descenders are:
 *	g j p q y beta zeta eta mu xi
 *	rho phi chi psi terminal-sigma
 */

private char Descenders[] = "gjpqy\231\235\236\243\245\250\254\255\256\275";


private dofont(fontno)
int  fontno; /* troff font number 0-3 */
{
    char ftfilename[100];
    FILE *ftfile;	/* generated font widths file */
    int		wid, curcode;
    struct	ctab *ct, *cc;
    int		asde;

#ifdef NeXT_MOD
    VOIDC sprintf(ftfilename, "ft%s", facenames[fontno]);
#else
#ifdef SYSV
    VOIDC sprintf(ftfilename, "ft%s", facenames[fontno]);
#else
    VOIDC sprintf(ftfilename, "ft%s.c", facenames[fontno]);
#endif
#endif

    if ((ftfile = fopen(ftfilename, "w")) == NULL) {
	perror(ftfilename);
	exit(1);
    }
#ifdef NeXT_MOD
    {
      char null_buf[32];
      bzero( null_buf, 32 );
      fwrite( null_buf, 1, 32, ftfile );
    }
#else
#ifdef BSD
    fprintf(ftfile, "/* troff width file for %s - %s (%c) */\n",
    	familyname, facenames[fontno], faces[fontno]);
    fprintf(ftfile, "char ft%s[256-32] = {\n", facenames[fontno]);
#endif
#endif
    /* write out entry for each troff character code */
    /* codes 0-31 (decimal) are not used */
    /* the first interesting code is 32 (040) - space */

    ct = ctab;
    for (curcode = 32; curcode < 256; curcode++) {
	while ((ct < &ctab[nchars]) && (ct->trcode < curcode) ||
		((ct->trcode == curcode) && ((ct->catcode>>7) < fontno))) {
		    ct++;
		}
	asde = 0;
	if ((ct >= &ctab[nchars]) || (curcode != ct->trcode) ||
	    ((ct->catcode>>7) != fontno)) {
		/* not found */
		wid = 0;
		cc = 0;
	}
	else {
	    wid = ct->wid;
	    cc = ct;
	    /* calculate ascender/descender info (heuristic) */
	    if (((curcode >= '0') && (curcode <= '9')) ||
	    	((curcode >= 'A') && (curcode <= 'Z'))) asde |= ASCENDER;
	    if (INDEX(Ascenders, curcode)) asde |= ASCENDER;
	    if (INDEX(Descenders, curcode)) asde |= DESCENDER;
	}
#ifdef NeXT_MOD
	putc(0377&(wid+asde),ftfile);
#else
#ifdef SYSV
	/* for system V we write a binary file of the width bytes */
	putc(0377&(wid+asde),ftfile);
#else
	/* for BSD we write a "c" array to be compiled */
	fprintf(ftfile, "%d", wid);
	if (asde) fprintf(ftfile,"+0%o",asde);
	fprintf(ftfile, ",\t%s/* %s */\n",
		asde?"":"\t",cc?cc->descr:"NULL");
#endif
#endif
    }
#ifndef NeXT_MOD
#ifdef BSD
    fprintf(ftfile,"};\n");
#endif
#endif
    VOIDC fclose(ftfile);
}


/* Start reading from an include file */
newinclude()
{
    char      filename[256]; /* Name of the include file */
    FILE     *newfile;       /* The new include file */

    if (sscanf(linebuf,"@INCLUDE %s",filename) != 1) {
	fprintf(stderr,"@INCLUDE syntax error\n");
	exit(1);
    }
    includelevel++;   /* New "include file" level */
    if( includelevel == MAXINCLNEST ) {
	fprintf(stderr,"@INCLUDE file nested too deep: %s\n",filename);
	exit(1);
    }
    if ((newfile = fopen(filename, "r")) == NULL) {
	fprintf(stderr,"Can't open include file %s\n",filename);
	exit(1);
    }
    curfile[includelevel] = newfile;
}


/* Got EOF while reading -- pop out one "include file" level */
popinclude()
{
    if( includelevel > 0 )      /* Don't close stdin */
	fclose(curfile[includelevel]);   /* Get rid of current file */
    includelevel--;
}


/* Initialize the include file stuff */
initinclude()
{
    includelevel = 0;
    curfile[includelevel] = stdin;
}


main(argc, argv)
int argc;
char **argv;
{
    if (argc != 2) {
	fprintf(stderr,"usage: pscatmap mapfile\n");
	exit(1);
    }
    if (freopen(*++argv,"r",stdin) == NULL) {
	perror("pscatmap");
	exit(1);
    }

#ifdef NeXT_MOD
    if ((blddir = envget("BLDDIR")) == NULL) blddir = "";
#endif NeXT_MOD
    if ((libdir = envget("PSLIBDIR")) == NULL) libdir = LibDir;
    
    initinclude();

    BuildTable();
    WriteTable();
    DoWidths();
    exit(0);
}

