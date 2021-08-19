static char    *filename = "afm.m";

/*
	afm.m
	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
  	
	This file provides functions for dealing with font metrics
*/


#import <stdio.h>
#import "appkit/nextstd.h"
#import "Font.h" /* NOTE: We have local copy of this to isolate us */
#import "afmprivate.h"

 /*
  * This little constant determines whether we change tabs to spaces when
  * we read them in, or test for both all the time along the way.  Its 100
  * bytes less code to screen the tabs, and maybe a millisecond slower. 
  */
#define SCREEN_TABS

 /* # chars in an encoding vector */
#define ENCODING_SIZE  256

#define BUFMAX  512
static char     Buffer[BUFMAX];
static char    *BufCurr;

static int      lineDone;
static NXFontMetrics *metricsParam;
static int      getFlags;

static FILE    *inFile;

static float	unitSize;	/* The size of a unit font in the units used
				   in the afm file.  This defaults to 1000.0
				   for printer afm files, but is 1.0 for
				   screen afm files. */
#define DEFAULTUNITSIZE (1000.0)

static char    *getNextFloat();
static char    *getNextName(),*mallocString();
static int	parseIt(),doOneString(),getLine(),nextKey();
/* extern double   atof(); Removed for cc-6 Leo 09Jan89 */

 /* skips c over all spaces and tabs */
#ifndef SCREEN_TABS
#define SKIP_WS(c)  while( *(c) == ' ' || *(c) == '\t' ) (c)++;
#else SCREEN_TABS
#define SKIP_WS(c)  while( *(c) == ' ' ) (c)++;
#endif SCREEN_TABS

 /* skips c to next space or tab */
#ifndef SCREEN_TABS
#define SKIP_TO_WS(c)  while( *(c) != ' ' && *(c) != '\t' ) (c)++;
#else SCREEN_TABS
#define SKIP_TO_WS(c)  while( *(c) != ' ' ) (c)++;
#endif SCREEN_TABS

 /* skips c through next semi-colon */
#define SKIP_THRU_SC(c)  while( *(c)++ != ';') ;

 /* is this character white space? */
#ifndef SCREEN_TABS
#define IS_WS(c)   ((c) == ' ' || (c) == '\t')
#else SCREEN_TABS
#define IS_WS(c)   ((c) == ' ')
#endif SCREEN_TABS


 /*
  * heres the entry point into this file. Pass it a ptr to a NXFontMetrics
  * where it can store the metrics it reads.  This storage must be all
  * zeroed out.  If this ptr is null, it will malloc its own space. 
  */
NXFontMetrics  *
DPSReadFontInfo(stuff, flags, file)
    NXFontMetrics  *stuff;
    int             flags;
    FILE           *file;
{
    if (!stuff) {
	if (!NX_MALLOC(stuff, NXFontMetrics, 1))
	    return NULL;
	bzero(stuff, sizeof(NXFontMetrics));
    }
    unitSize = DEFAULTUNITSIZE;
    if (getFlags = flags) {
	inFile = file;
	metricsParam = stuff;
	lineDone = YES;
	if (parseIt(stuff) == -1) {
	    fprintf(stderr, "getFontInfo: out of memory, you're a pig.\n");
	    return NULL;
	}
    }
    return stuff;
}


static int
parseIt(metrics)
    register NXFontMetrics *metrics;	/* place to put metrics */
{
    register char  *c;		/* every routine needs one of these */
    register int    i;		/* one of these too */
    register float *f;		/* one of these too */
    NXCharData     *cd;
    NXCompositeChar *ch;
    NXCompositePart *cp;
    NXLigature     *lig;
    NXKernTrack    *k;
    NXKernPair     *kp;
    int             currTrack = 0;	/* current kerning track to fill
					 * in */
    int             currPair = 0;	/* current kerning pair to fill in */
    int             currCompChar = 0;	/* current composite char to fill
					 * in */
    int             currCompPart = 0;	/* current composite char part to
					 * fill in */
    int             currChar;	/* current encoded char */
    int             quit = NO;
    int             doneFlags = getFlags;	/* which pieces are we
						 * through with? */

    while (!quit) {
	switch (nextKey()) {
	case START_FONT_METRICS:
	    if (doOneString(BufCurr, &metrics->formatVersion) == -1)
		return -1;
	    lineDone = YES;
	    break;
	case END_FONT_METRICS:
	    quit = YES;
	    break;
	case FONT_NAME:
	    if (doOneString(BufCurr, &metrics->name) == -1)
		return -1;
	    lineDone = YES;
	    break;
	case FULL_NAME:
	    if (doOneString(BufCurr, &metrics->fullName) == -1)
		return -1;
	    lineDone = YES;
	    break;
	case FAMILY_NAME:
	    if (doOneString(BufCurr, &metrics->familyName) == -1)
		return -1;
	    lineDone = YES;
	    break;
	case WEIGHT:
	    if (doOneString(BufCurr, &metrics->weight) == -1)
		return -1;
	    lineDone = YES;
	    break;
	case ITALIC_ANGLE:
	    metrics->italicAngle = atof(BufCurr);
	    lineDone = YES;
	    break;
	case IS_FIXED_PITCH:
	    c = BufCurr;
	    SKIP_WS(c);
	    metrics->isFixedPitch = (*c == 't');
	    lineDone = YES;
	    break;
	case IS_SCREEN_FONT:
	    c = BufCurr;
	    SKIP_WS(c);
	    if (metrics->isScreenFont = (*c == 't'))
	    	unitSize = 1.0;
	    lineDone = YES;
	    break;
	case SCREEN_FONT_SIZE:
	    metrics->screenFontSize = atoi(BufCurr);
	    lineDone = YES;
	    break;
	case FONT_BBOX:
	    c = BufCurr;
	    for (i = 0; i < 4; i++)
		c = getNextFloat(c, metrics->fontBBox + i);
	    lineDone = YES;
	    break;
	case UNDERLINE_POSITION:
	    metrics->underlinePosition = atof(BufCurr) / unitSize;
	    lineDone = YES;
	    break;
	case UNDERLINE_THICKNESS:
	    metrics->underlineThickness = atof(BufCurr) / unitSize;
	    lineDone = YES;
	    break;
	case VERSION:
	    if (doOneString(BufCurr, &metrics->version) == -1)
		return -1;
	    lineDone = YES;
	    break;
	case NOTICE:
	    if (doOneString(BufCurr, &metrics->notice) == -1)
		return -1;
	    lineDone = YES;
	    break;
	case ENCODING_SCHEME:
	    if (doOneString(BufCurr, &metrics->encodingScheme) == -1)
		return -1;
	    lineDone = YES;
	    break;
	case CAP_HEIGHT:
	    metrics->capHeight = atof(BufCurr) / unitSize;
	    lineDone = YES;
	    break;
	case X_HEIGHT:
	    metrics->xHeight = atof(BufCurr) / unitSize;
	    lineDone = YES;
	    break;
	case ASCENDER:
	    metrics->ascender = atof(BufCurr) / unitSize;
	    lineDone = YES;
	    break;
	case DESCENDER:
	    metrics->descender = atof(BufCurr) / unitSize;
	    lineDone = YES;
	    break;
	case START_COMPOSITES:
	    metrics->numCompositeChars = atoi(BufCurr);
	    if (!NX_MALLOC(metrics->compositeChars, NXCompositeChar,
			   metrics->numCompositeChars))
		return -1;
	    lineDone = YES;
	    break;
	case CC:
	    ch = metrics->compositeChars + currCompChar++;
	    if (!(c = getNextName(BufCurr, &ch->name)))
		return -1;
	    ch->numParts = atoi(c);
	    if (!NX_MALLOC(ch->parts, NXCompositePart, ch->numParts))
		return -1;
	    currCompPart = 0;
	    SKIP_THRU_SC(c);
	    BufCurr = c;
	    break;
	case PCC:
	    cp = metrics->compositeChars[currCompChar - 1].parts +
		  currCompPart++;
	    if (!(c = getNextName(BufCurr, &cp->name)))
		return -1;
	    c = getNextFloat(getNextFloat(c, &cp->dx), &cp->dy);
	    SKIP_THRU_SC(c);
	    BufCurr = c;
	    break;
	case END_COMPOSITES:
	    if (!(doneFlags &= ~NX_FONTCOMPOSITES))
		quit = YES;
	    break;
	case START_CHAR_METRICS:
	/* when we deal with unencoded chars, get an int here */
	/* now, we'll just read however many line there are */
	    if (!metrics->widths && (getFlags & NX_FONTWIDTHS)) {
		if (!NX_MALLOC(metrics->widths, float, ENCODING_SIZE))
		    return -1;
		{		/* Zero the widths so any undefined ones
				 * will have zero width */
		    register float *widthsPtr;
		    register int    i;

		    widthsPtr = metrics->widths;
		    for (i = 0; i < ENCODING_SIZE; i++)
			*widthsPtr++ = 0.0;
		}
	    }
	    if (!metrics->charData && (getFlags & NX_FONTCHARDATA))
		if (!NX_MALLOC(metrics->charData, NXCharData, ENCODING_SIZE))
		    return -1;
	    lineDone = YES;
	    break;
	case C:
	    currChar = atoi(c = BufCurr);
	    SKIP_THRU_SC(c);
	    BufCurr = c;
	    break;
	case WX:
	    if (currChar != -1)
		metrics->widths[currChar] = atof(c = BufCurr) / unitSize;
	    if (!(getFlags & NX_FONTCHARDATA)) {
		lineDone = YES;
		break;
	    }
	    SKIP_THRU_SC(c);
	    BufCurr = c;
	    break;
	case W:
	    if (currChar != -1) {
		metrics->hasYWidths = YES;
		if (!metrics->yWidths)
		    if (!NX_MALLOC(metrics->yWidths, float, ENCODING_SIZE))
			return -1;
		c = getNextFloat(
				 getNextFloat(BufCurr,
					      metrics->widths + currChar),
				 metrics->yWidths + currChar);
	    }
	    if (!(getFlags & NX_FONTCHARDATA)) {
		lineDone = YES;
		break;
	    }
	    SKIP_THRU_SC(c);
	    BufCurr = c;
	    break;
	case N:
	    if (currChar != -1)
		if (!(c = getNextName(BufCurr,
				      &metrics->charData[currChar].name)))
		    return -1;
	    SKIP_THRU_SC(c);
	    BufCurr = c;
	    break;
	case B:
	    if (currChar != -1) {
		c = BufCurr;
		f = metrics->charData[currChar].charBBox;
		for (i = 4; i--;)
		    c = getNextFloat(c, f++);
	    }
	    SKIP_THRU_SC(c);
	    BufCurr = c;
	    break;
	case L:
	    if (currChar != -1) {
		cd = metrics->charData + currChar;
		if (cd->numLigatures == 0) {
		    if (!NX_MALLOC(cd->ligatures, NXLigature, 1))
			return -1;
		} else {
		    if (!NX_REALLOC(cd->ligatures, NXLigature,
				    cd->numLigatures + 1))
			return -1;
		}
		lig = cd->ligatures + cd->numLigatures++;
		if (!(c = getNextName(
				   getNextName(BufCurr, &lig->successor),
				      &lig->result)))
		    return -1;
	    }
	    SKIP_THRU_SC(c);
	    BufCurr = c;
	    break;
	case END_CHAR_METRICS:
	    if (!(doneFlags &=
		  ~(NX_FONTCOMPOSITES | NX_FONTWIDTHS)))
	    /*
	     * If the width of a line feed hasn't been set, set it to the
	     * width of a space for Chris. 
	     */
		if (!metrics->widths[0x0A])
		    metrics->widths[0x0A] = metrics->widths[0x20];
	    quit = YES;
	    break;
	case START_TRACK_KERN:
	    metrics->numKTracks = atoi(BufCurr);
	    if (!NX_MALLOC(metrics->kernTracks, NXKernTrack,
			   metrics->numKTracks))
		return -1;
	    lineDone = YES;
	    break;
	case TRACK_KERN:
	    k = metrics->kernTracks + currTrack++;
	    k->degree = atoi(c = BufCurr);
	    SKIP_WS(c);
	    SKIP_TO_WS(c);
	    (void)getNextFloat(
			       getNextFloat(
					    getNextFloat(
					    getNextFloat(c, &k->minSize),
					    &k->minAmount), &k->maxSize),
			       &k->maxAmount);
	    lineDone = YES;
	    break;
	case START_KERN_PAIRS:
	    metrics->numKPairs = atoi(BufCurr);
	    if (!NX_MALLOC(metrics->kernPairs, NXKernPair, metrics->numKPairs))
		return -1;
	    lineDone = YES;
	    break;
	case KPX:
	    kp = metrics->kernPairs + currPair++;
	    if (!(c = getNextName(
			  getNextName(BufCurr, &kp->name1), &kp->name2)))
		return -1;
	    (void)getNextFloat(c, &kp->x);
	    kp->y = 0.0;
	    lineDone = YES;
	    break;
	case KP:
	    kp = metrics->kernPairs + currPair++;
	    if (!(c = getNextName(getNextName(BufCurr, &kp->name1),
				  &kp->name2)))
		return -1;
	    (void)getNextFloat(getNextFloat(c, &kp->x), &kp->y);
	    lineDone = YES;
	    break;
	case END_KERN_DATA:
	    if (!(doneFlags &= ~NX_FONTKERNING))
		quit = YES;
	default:
	    lineDone = YES;
	}
    }
    return(0);
} /* parseIt */


 /*
  * This routine gets one string (including blanks). 
  */
static int
doOneString(c, s)
    register char  *c;
    register char **s;
{
    register int    len;
    register char  *end;

    if (!*s) {			/* no-op if we've already read this string */
	SKIP_WS(c);
	for (end = c; *end != '\n';)
	    end++;
	len = end - c;
	if (!NX_MALLOC(*s, char, len + 1))
	    return -1;
	(void)strncpy(*s, c, len);
	(*s)[len] = '\0';
    }
    return(0);
}


 /*
  * gets one float from the string c.  c should point to the float or some
  * whitespace before it.  Routine returns ptr to first char after the
  * float. 
  */
static char    *
getNextFloat(c, f)
    register char  *c;
    float          *f;
{
    SKIP_WS(c);
    *f = atof(c) / unitSize;
    SKIP_TO_WS(c);
    return c;
}


 /*
  * gets one name from the string c.  c should point to the name or some
  * whitespace before it.  It returns an updated c that point to the first
  * char after the name. 
  */
static char    *
getNextName(c, n)
    register char  *c;
    register char **n;
{
    register char  *end;

    if (!c)			/* this lets us chain these calls */
	return NULL;
    SKIP_WS(c);
    end = c;
    SKIP_TO_WS(end);
    if (!(*n = mallocString(end - c + 1)))
	return NULL;
    (void)strncpy(*n, c, end - c);
    (*n)[end - c] = '\0';
    return end;
}


static char    *
mallocString(len)
    int             len;
{
    typedef char   *_NXCharPtr;

/* better not have any names bigger than this or else */
#define BUF_SIZE 256
#define BUF_HYST 4
    static int      charCount;
    register char  *ret;
    register NXFontMetrics *metrics = metricsParam;

    if (metrics->numBuffers == 0)
	charCount = BUF_SIZE;
    if (charCount + len > BUF_SIZE) {
	if (metrics->numBuffers == 0) {
	    if (!NX_MALLOC(metrics->buffers, _NXCharPtr, BUF_HYST))
		return NULL;
	} else if (!(metrics->numBuffers % BUF_HYST))
	    if (!NX_REALLOC(metrics->buffers, _NXCharPtr,
			    metrics->numBuffers + BUF_HYST))
		return NULL;
	metrics->numBuffers++;
	if (!NX_MALLOC(metrics->buffers[metrics->numBuffers - 1], char, BUF_SIZE))
	    return NULL;
	charCount = 0;
    }
    ret = metrics->buffers[metrics->numBuffers - 1] + charCount;
    charCount += len;
    return ret;
}


static int
nextKey()
{
#define IS_MATCH(nm, len, fl) ((getFlags & (fl)) && !strncmp(bc,(nm),(len)))
#define NO_MATCH_FOUND	if(bc == Buffer) /* if failed at start of line */   \
			    lineDone = YES;	/* bail out of this line */ \
			else			/* else go on to next ; */  \
			    SKIP_THRU_SC(bc);	/* separated item */

    register char  *bc = BufCurr;

    for (;;) {			/* loop until we find something useful */
	if (lineDone) {
	    if (!getLine()) {
#ifdef DEBUG
		fprintf(stderr, "No END_FONT_METRICS found\n");
#endif
		return END_FONT_METRICS;	/* EOF before
						 * END_FONT_METRICS */
	    }
	    bc = BufCurr = Buffer;
	} else
	    SKIP_WS(bc);
	lineDone = NO;
	switch (*bc) {
	case 'A':
	    if (IS_MATCH("Ascender ", 9, NX_FONTMETRICS)) {
		BufCurr = bc + 9;
		return ASCENDER;
	    }
	    break;
	case 'B':
	    if ((getFlags & NX_FONTCHARDATA) && IS_WS(bc[1])) {
		BufCurr = bc + 2;
		return B;
	    }
	    break;
	case 'C':
	    if ((getFlags & (NX_FONTCHARDATA | NX_FONTWIDTHS)) &&
		IS_WS(bc[1])) {
		BufCurr = bc + 2;
		return C;
	    } else if ((getFlags & NX_FONTCOMPOSITES) &&
		       bc[1] == 'C' && IS_WS(bc[2])) {
		BufCurr = bc + 3;
		return CC;
	    } else if (IS_MATCH("CapHeight ", 10, NX_FONTMETRICS)) {
		BufCurr = bc + 10;
		return CAP_HEIGHT;
	    }
	    break;
	case 'D':
	    if (IS_MATCH("Descender ", 10, NX_FONTMETRICS)) {
		BufCurr = bc + 10;
		return DESCENDER;
	    }
	    break;
	case 'E':
	    if (IS_MATCH("EncodingScheme ", 15, NX_FONTHEADER)) {
		BufCurr = bc + 15;
		return ENCODING_SCHEME;
	    } else if (!strncmp("EndCharMetrics", bc, 14)) {
		BufCurr = bc + 14;
		return END_CHAR_METRICS;
	    } else if (!strncmp("EndFontMetrics", bc, 14)) {
		BufCurr = bc + 14;
		return END_FONT_METRICS;
	    } else if (!strncmp("EndKernData", bc, 11)) {
		BufCurr = bc + 11;
		return END_KERN_DATA;
	    } else if (!strncmp("EndComposites", bc, 13)) {
		BufCurr = bc + 13;
		return END_COMPOSITES;
	    }
	    break;
	case 'F':
	    if (IS_MATCH("FamilyName ", 11, NX_FONTHEADER)) {
		BufCurr = bc + 11;
		return FAMILY_NAME;
	    } else if (IS_MATCH("FontBBox ", 9, NX_FONTMETRICS)) {
		BufCurr = bc + 9;
		return FONT_BBOX;
	    } else if (IS_MATCH("FontName ", 9, NX_FONTHEADER)) {
		BufCurr = bc + 9;
		return FONT_NAME;
	    } else if (IS_MATCH("FullName ", 9, NX_FONTHEADER)) {
		BufCurr = bc + 9;
		return FULL_NAME;
	    }
	    break;
	case 'I':
	    if (IS_MATCH("IsFixedPitch ", 13, NX_FONTMETRICS)) {
		BufCurr = bc + 13;
		return IS_FIXED_PITCH;
	    } else if (IS_MATCH("ItalicAngle ", 12, NX_FONTMETRICS)) {
		BufCurr = bc + 12;
		return ITALIC_ANGLE;
	    } else if (IS_MATCH("IsScreenFont ", 13, NX_FONTMETRICS)) {
		BufCurr = bc + 13;
		return IS_SCREEN_FONT;
	    }
	    break;
	case 'K':
	    if (IS_MATCH("KP ", 3, NX_FONTKERNING)) {
		BufCurr = bc + 3;
		return KP;
	    } else if (IS_MATCH("KPX ", 4, NX_FONTKERNING)) {
		BufCurr = bc + 4;
		return KPX;
	    }
	    break;
	case 'L':
	    if ((getFlags & NX_FONTCHARDATA) && IS_WS(bc[1])) {
		BufCurr = bc + 2;
		return L;
	    }
	    break;
	case 'N':
	    if ((getFlags & NX_FONTCHARDATA) && IS_WS(bc[1])) {
		BufCurr = bc + 2;
		return N;
	    } else if (IS_MATCH("Notice ", 8, NX_FONTHEADER)) {
		BufCurr = bc + 8;
		return NOTICE;
	    }
	    break;
	case 'S':
	    if (!strncmp(bc, "Start", 5)) {
		bc += 5;
		if (IS_MATCH("CharMetrics ", 12,
			     (NX_FONTWIDTHS | NX_FONTCHARDATA))) {
		    BufCurr = bc + 12;
		    return START_CHAR_METRICS;
		} else if (IS_MATCH("Composites ", 11,
				    NX_FONTCOMPOSITES)) {
		    BufCurr = bc + 11;
		    return START_COMPOSITES;
		} else if (IS_MATCH("ScreenFontSize ", 15,
				    NX_FONTCOMPOSITES)) {
		    BufCurr = bc + 15;
		    return SCREEN_FONT_SIZE;
		} else if (!strncmp("FontMetrics ", bc, 12)) {
		    BufCurr = bc + 12;
		    return START_FONT_METRICS;
		} else if (IS_MATCH("KernPairs ", 10,
				    NX_FONTKERNING)) {
		    BufCurr = bc + 10;
		    return START_KERN_PAIRS;
		} else if (IS_MATCH("TrackKern ", 10,
				    NX_FONTKERNING)) {
		    BufCurr = bc + 10;
		    return START_TRACK_KERN;
		}
		bc -= 5;
	    }
	    break;
	case 'T':
	    if (IS_MATCH("TrackKern ", 10, NX_FONTKERNING)) {
		BufCurr = bc + 10;
		return TRACK_KERN;
	    }
	    break;
	case 'U':
	    if (IS_MATCH("UnderlinePosition ", 18, NX_FONTMETRICS)) {
		BufCurr = bc + 18;
		return UNDERLINE_POSITION;
	    } else if (IS_MATCH("UnderlineThickness ", 19,
				NX_FONTMETRICS)) {
		BufCurr = bc + 19;
		return UNDERLINE_THICKNESS;
	    }
	    break;
	case 'V':
	    if (IS_MATCH("Version ", 8, NX_FONTHEADER)) {
		BufCurr = bc + 8;
		return VERSION;
	    }
	    break;
	case 'W':
	    if ((getFlags & NX_FONTCHARDATA) && IS_WS(bc[1])) {
		BufCurr = bc + 2;
		return W;
	    } else if ((getFlags & NX_FONTWIDTHS) &&
		       bc[1] == 'X' && IS_WS(bc[2])) {
		BufCurr = bc + 3;
		return WX;
	    } else if (IS_MATCH("Weight ", 7, NX_FONTHEADER)) {
		BufCurr = bc + 7;
		return WEIGHT;
	    }
	    break;
	case 'X':
	    if (IS_MATCH("XHeight ", 8, NX_FONTMETRICS)) {
		BufCurr = bc + 8;
		return X_HEIGHT;
	    }
	    break;
	    break;
	case '\n':
	    lineDone = YES;
	}
	if (*bc != '\n')	/* Do this for any case of bc but \n */
	    NO_MATCH_FOUND;
    }
}


 /* gets next line of the file */
static int
getLine()
{
    register char   c;
    register char  *cp = Buffer;
    register int    i = BUFMAX;
    register FILE  *fp = inFile;

    do {
	if ((c = getc(fp)) == EOF)
	    return 0;
#ifdef SCREEN_TABS
	if (c == '\t')		/* change tabs to spaces */
	    c = ' ';
#endif SCREEN_TABS
	*cp++ = c;
    } while (c != '\n' && --i);
    return 1;
}
