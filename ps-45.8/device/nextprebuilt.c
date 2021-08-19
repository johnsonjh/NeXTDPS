/*****************************************************************************

    nextprebuilt.c
    Integrated + nextified prebuilt code

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 24May90 by Peter + Terry from *prebuil*.c

    Modified:

    10Jul90 Terry Replaced hostdict used for character names with an array.
    18Oct90 pgraff made sure errors discovered by readPrebuiltFile propagated
    		   back to caller correctly.

******************************************************************************/

#import <string.h>
#import PACKAGE_SPECS
#import FP
#import ENVIRONMENT
#import DEVICETYPES
#import PSLIB
#import UNIXSTREAM
#import "nextprebuilt.h"

/* Dictionary  mapping fid's to name objects (non-purgeable dictionary)
 * and another mapping fid's to NextPrebuilt objects (purgeable dictionary)
 */
static HostDict *nameDict, *fontDict;

/* Cache of first prebuilt file's encoding vector */
static PrebuiltEncoding *standardEncoding;
static char standardCharSet[30];

/* Number of NextPrebuilt objects to cache */
#define CACHESIZE 4

static inline int PrebuiltPathSearch(char *fontName, int length)
{
    char filePath[256],fileName[256], fontNameBuf[256];

    if (!fontName) fontName = "";
    strncpy(fileName,fontName,length);
    fileName[length] = 0;
    strcpy(fontNameBuf, fileName); /* FontPathSearch needs null termination */
    strcat(fileName,PREBUILT_SUFFIX);
    return FontPathSearch(fontNameBuf,fileName,filePath,sizeof(filePath));
}

integer DevRgstPrebuiltFontInfo(int font, char *fontName, int length,
			boolean stdEncoding, CharNameProc proc, char *procData)
{
    if (!getHostDict(nameDict, (char *) font) &&
	PrebuiltPathSearch(fontName, length) >= 0) {
	char *name = (char *)malloc(length + 1);
	os_strncpy(name, fontName, length);
	name[length] = 0;
	defHostDict(nameDict, (char *)font, name);
    }
    return 0;
}

static void disposePrebuiltFont(NextPrebuiltFont *npf)
{
    if (npf->stm) fclose(npf->stm);
    if (npf->encoding != standardEncoding) free(npf->encoding);
    free((char *) npf);
}

void IniPreBuiltChars()
{
    nameDict = newHostDict(35, 0, 0, (PVoidProc)NULL);
    fontDict = newHostDict(CACHESIZE, 0, 0, disposePrebuiltFont);
}

static int readPrebuiltFile(NextPrebuiltFont *npf, char *path)
{
    char *charSet, *names;
    PrebuiltFile *file;
    int i;
    
    /* Relying on our map_fd customization to os_fopen in unixstream.c */
    if(!(npf->stm = os_fopen(path, "r"))) return -1;
    npf->file = file = (PrebuiltFile *)npf->stm->base;
#if SWAPBITS
    if(file->byteOrder != 0 || file->version != PREBUILTFILEVERSION) {
#else
    if(file->byteOrder == 0 || file->version != PREBUILTFILEVERSION) {
#endif
	fclose(npf->stm);
	npf->file = 0;
	return -1;
    }

    npf->widths = (PrebuiltWidth *) ((char *)file + file->widths);
    charSet = (char *)file + file->characterSetName;
    
    if (strncmp(standardCharSet, charSet, 30)) {
        int base, length;
	PrebuiltEncoding *enc;
	char startc, c, *cp;
	
	/* Names are lexicographically sorted so the minimum value
	 * for the first character is *names, and the maximum value for
	 * the first character is the first character of the last name.
	 * Make the length a multiple of 4 so that alignment works out right.
	 */
	names = (char *)file + file->names;
	base = *names;
	for (cp = names + file->lengthNames - 2; *cp; cp--) ;
	length = ((*(cp+1) - base) & ~0x3) + 4;
	enc = (PrebuiltEncoding *) malloc(sizeof(PrebuiltEncoding) + 
			(sizeof(ushort)+sizeof(uchar))*length);
	enc->base     = base;
	enc->length   = length;
	enc->startpos = (ushort *) ((int)enc + sizeof(PrebuiltEncoding));
	enc->code     = (uchar *) ((int)enc->startpos + sizeof(ushort)*length);

	/* Every time we reach a name that starts with a new first character,
	 * record its offset into the names character array and its character
	 * code.
	 */
	for (i = 0, startc = 0, cp = names; i < file->numberChar; i++) {
	    if ((c = *cp) != startc) {
	        startc = c;
		c -= base;
		enc->startpos[c] = cp - names;
		enc->code[c]     = i;
	    }
	    while (*cp++);
	}
	
	npf->encoding = enc;
	if (!standardEncoding) {
	    strncpy(standardCharSet, charSet, 30);
	    standardEncoding = enc;
	}
    }
    else npf->encoding = standardEncoding;
    
    npf->matrices = (PrebuiltMatrix *) ((char *)file + file->matrices);
    npf->vertWidths = (PrebuiltVertWidths *) (file->vertWidths ?
			((char *)file + file->vertWidths) : NULL);
    return 0;
}

NextPrebuiltFont *getPrebuiltFont(unsigned long int fid)
{
    NextPrebuiltFont *npf;

    if (!(npf = (NextPrebuiltFont *) getHostDict(fontDict, (char *) fid))) {
	char filePath[256],fileName[128];
	char *font = (char *) getHostDict(nameDict, (char *) fid);
  
	/* Find full pathname for prebuilt file */
	if (!font) font = "NullFont";
	strcpy(fileName, font);
	strcat(fileName, PREBUILT_SUFFIX);
	if (FontPathSearch(font,fileName,filePath,sizeof(filePath)) < 0)
	    return 0;
		
	npf = (NextPrebuiltFont *) malloc(sizeof(NextPrebuiltFont));
	if(readPrebuiltFile(npf, filePath)) {
	    free(npf);
	    return 0;
	}
	defHostDict(fontDict, (char *)fid, (char *)npf);
    }
    return npf;
}

static inline int findMatrixNumber(NextPrebuiltFont *npf, Mtx *m)
{
    Fixed size;
    int i;
    
    size = pflttofix(&m->a);
    if (!(i = npf->file->numberMatrix) ||
	size < npf->matrices[0].a ||
	size >= npf->matrices[i-1].a + (npf->matrices[i-1].a >> 2))
	return -1;
    
    while(size < npf->matrices[i].a) i--;
    return i;
}

static boolean buildPrebuilt(PreBuiltArgs *args, NextPrebuiltFont *npf,
			int code, int maskID)
{
    PrebuiltFile *file;
    PrebuiltVertMetrics *vertMetrics;
    PrebuiltMatrix *m;
    PrebuiltMask *pm;
    PMask mask;
    float tx, ty;
    Fixed hx, hy, vx, vy, tmp;
    int mnum, width, height, nBytes, c;
    
    if (!(file = npf->file) ||
	 (mnum = findMatrixNumber(npf, args->matrix)) < 0)
	return 0;
	
    c = code + mnum * file->numberChar;
    pm  = (PrebuiltMask *)((char *)file+file->masks + c*sizeof(PrebuiltMask));	

    if (!(mask = (PMask)MCGetMask())) return 0;
    args->mask = mask;
    mask->maskID = maskID;
    mask->unused = 1;	/* Indicates this is a byte-aligned prebuilt mask */
    mask->cached = true;
    mask->width  = width = pm->width;
    mask->height = height = pm->height;
    
    if (height == 0 || width == 0) {
        mask->width = mask->height = 0;
	mask->data = NIL;
    }
    else {
	nBytes = ((width + 7) >> 3) * height;
	if (nBytes > args->cacheThreshold) {
	    mask->data = (uchar *)MCGetTempBytes(nBytes);
	    mask->cached = false;
	}
	else 
	    MCGetCacheBytes(mask, nBytes);
	if (mask->data == NULL) {
	    MCFreeMask(mask);
	    return 0;
	}
	bcopy((char *)file + pm->maskData, mask->data, nBytes);
    }
  
    m = &npf->matrices[mnum];
    vertMetrics = 0;
    if (file->vertMetrics)
	vertMetrics = (PrebuiltVertMetrics *) ((char *)file + 
	    file->vertMetrics + code*sizeof(PrebuiltVertMetrics));

    if (args->bitmapWidths) {
	hx = ((long) pm->maskWidth.hx) << 16;
	hy = ((long) pm->maskWidth.hy) << 16;
	if (vertMetrics) {
	    vx = ((long) vertMetrics->width.vx) << 16;
	    vy = ((long) vertMetrics->width.vy) << 16;
	}
	else vx = vy = 0;
    } else {
	hx = fixmul(npf->widths[code].hx, m->a) + m->tx;
	hy = fixmul(npf->widths[code].hy, m->d) + m->ty;
	if (npf->vertWidths) {
	    vx = fixmul(npf->vertWidths[code].vx, m->a);
	    vy = fixmul(npf->vertWidths[code].vy, m->d);
	}
	vx += m->tx;
	vy += m->ty;
    }

    if ((tmp = pflttofix(&args->matrix->a)) != m->a) {
        tmp = fixdiv(tmp, m->a);
	hx = fixmul(hx, tmp);
	hy = fixmul(hy, tmp);
	vx = fixmul(vx, tmp);
	vy = fixmul(vy, tmp);
    }
    args->horizMetrics->incr.x =  hx;
    args->horizMetrics->incr.y = -hy;
    args->vertMetrics->incr.x =  vx;
    args->vertMetrics->incr.y = -vy;

    vx = vy = 0;
    if ((tx = args->matrix->tx) != 0.) vx = pflttofix(&tx) & 0xffff0000;
    if ((ty = args->matrix->ty) != 0.) vy = pflttofix(&ty) & 0xffff0000;
    args->horizMetrics->offset.x = (vx - (((long) pm->maskOffset.hx) << 16));
    args->horizMetrics->offset.y = (vy - (((long) pm->maskOffset.hy) << 16));
    if (vertMetrics) {
        vx -= ((long) vertMetrics->offset.vx) << 16;
        vy -= ((long) vertMetrics->offset.vy) << 16;
    }
    args->vertMetrics->offset.x  = vx;
    args->vertMetrics->offset.y  = vy;
    return 1;
}

static inline boolean CheckPrebuiltMatrix(Mtx *m)
{
    float a = m->a;
    return (m->b==0. && m->c==0. && a<MAXIMUMSIZE && a>MINIMUMSIZE &&
	    a==-m->d);
}	

int GetPreBuiltChar(int bpp, int maskID, PreBuiltArgs *args)
{
    NextPrebuiltFont *npf;
    PrebuiltEncoding *enc;
    char *np;
    int code, index;
    
    /* Create mapping from fid to font object, if necessary */
    if (!CheckPrebuiltMatrix(args->matrix) ||
        !(npf = getPrebuiltFont((unsigned long int)args->font)))
	return 0;
    
    /* Lookup character code in prebuilt encoding using its name */
    enc = npf->encoding;
    index = *args->name - enc->base;
    if (index >= 0 && index < enc->length) {
	np = (char *)npf->file + npf->file->names + enc->startpos[index];
	for (code = enc->code[index]; *args->name == *np; code++) {
	    if (strncmp(args->name, np, args->nameLength) ||
	    	np[args->nameLength]) {
	        while (*np++) ;
	        continue;
	    } 
	    return buildPrebuilt(args, npf, code, maskID);
	}
    }
    
    /* Failed to find the name in the prebuilt encoding vector */
    return 0;
}
