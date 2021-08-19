/* bmpattern.c -- implementation of 2-bit gray dither patterns

   bitmap implementations provide the 3 PatternData structures to be
   used for rendering menu grays.
 */

#import PACKAGE_SPECS
#import BINTREE
#import "devmark.h"
#import "bitmap.h"

typedef struct {
    PatternProcsRec procs;
    int numpats;
    PatternData *pd;	/* numpats worth of static patterns */
    int alphaval;
} NXGrayPatRec;

/* setupPattern vector for nextgray patterns.  We just copy the precomputed
   pattern data into the passed pointer
 */
static void
NXGrayPatSetup(PatternHandle h, DevMarkInfo *markInfo, PatternData *data)
{
    PNextGSExt ep;
    int grayindex;

    if(markInfo->priv) {
	if((ep = *(PNextGSExt *)markInfo->priv) &&
	   (ep->graypatstate != NOGRAYPAT)) {
	    grayindex = (ep->graypatstate - 1) * 2 +
	                ((markInfo->offset.x + markInfo->screenphase.x) & 1);
	    *data = ((NXGrayPatRec *)h)->pd[grayindex];
	}
	else
	    CantHappen();
    } else {
	/* marking alpha in a planar bitmap */
	data->start = &data->value;
	data->end = data->start + 1;
	data->width = 1;
	data->constant = true;
	data->value = ((NXGrayPatRec *)h)->alphaval;
	data->id = 0;
    }
}

static void
NXGrayPatDestroy(PatternHandle h)
{
    free(h);
}

/* this vector should never be called (this pattern is not used during
   imaging.)
 */
static integer
NXGrayPatInfo(PatternHandle h, DevColorData *red, DevColorData *green,
		   DevColorData *blue, DevColorData *gray, int firstColor)
{
    CantHappen();
    return 0;
}

/* Create an NXGrayPattern from a precomputed array of patterns.  Patterns
 * are assumed to only have two x phases.  Each pattern should have two 
 * PatternData elements, the first for xphase 0, and the second for xphase 1.
 * numpats should be the number of patterns, not the number of PatternData
 * elements.  (i.e. to get pattern n with x phase x, use pats[2*n+(x&1)]).
 */
PatternHandle NXGrayPat(int numpats, PatternData *pats, int alpha)
{
    NXGrayPatRec *gp = (NXGrayPatRec *) calloc(sizeof(NXGrayPatRec),1);
    gp->numpats = numpats;
    gp->alphaval = alpha;
    gp->pd = pats;
    gp->procs.setupPattern = NXGrayPatSetup;
    gp->procs.destroyPattern = NXGrayPatDestroy;
    gp->procs.patternInfo = NXGrayPatInfo;
    return((PatternHandle)gp);
}    
