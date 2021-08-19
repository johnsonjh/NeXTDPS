
/* bm38.c 
 * BMClass : LocalBMClass : BM38Class
 * implementation of 24-bit RGB bitmaps */

#import PACKAGE_SPECS
#import BINTREE
#import WINDOWDEVICE
#import "bitmap.h"
#import "bm38.h"
#import "devmark.h"

extern Card8 framelog2BD;
extern int framebytewidth;
extern PMarkProcs wdMarkProcs;
extern unsigned int *framebase;
extern PImageProcs fmImageProcs;

#define WHITE 	0xffffffff
#define LTGRAY 	0xaaaaaaff
#define DKGRAY	0x555555ff
#define BLACK	0x00000000

static const SCANTYPE logray0[4] 	= { BLACK, DKGRAY, DKGRAY, BLACK };
static const SCANTYPE logray1[4] 	= { DKGRAY, BLACK, BLACK, DKGRAY };
static const SCANTYPE medgray0[4] 	= { DKGRAY, LTGRAY, LTGRAY, DKGRAY };
static const SCANTYPE medgray1[4] 	= { LTGRAY, DKGRAY, DKGRAY, LTGRAY };
static const SCANTYPE higray0[4] 	= { LTGRAY, WHITE, WHITE, LTGRAY };
static const SCANTYPE higray1[4] 	= { WHITE, LTGRAY, LTGRAY, WHITE };
static PatternData BM38Grays[6] = {
    { &logray0[0],	&logray0[0] + 4,  2, 0, 0, 0 },
    { &logray1[0],	&logray1[0] + 4,  2, 0, 0, 0 },
    { &medgray0[0],	&medgray0[0] + 4, 2, 0, 0, 0 },
    { &medgray1[0],	&medgray1[0] + 4, 2, 0, 0, 0 },
    { &higray0[0],	&higray0[0] + 4,  2, 0, 0, 0 },
    { &higray1[0],	&higray1[0] + 4,  2, 0, 0, 0 },
};
	

static Bitmap *BM38New(BMClass *class, Bounds *b, int local)
{
    Bitmap *bm;

    /* i.e. `super new' */
    bm = (Bitmap *) (*((BMClass *)localBM)->new)(class, b, local);
    bm->colorSpace = NX_RGBCOLORSPACE;
    bm->bps = 8;
    bm->spp = 3;
    /* The following needn't be cleared explicitly since a calloc is done...
    bm->alpha = 0;
    bm->vram = 0;
    bm->planar = 0;
    */
    bm->type = NX_TWENTYFOURBITRGB;
    return bm;
}
    
static Bitmap *BM38NewFromData(LocalBMClass *class, Bounds *b, void *bits,
			       void *abits, int byteSize, int rowBytes,
			       int vram, int freeMethod)
{
    Bitmap *bm;

    bm = (Bitmap *) (*((LocalBMClass *)localBM)->newFromData)
		    (class, b, bits, abits, byteSize, rowBytes,
		     vram, freeMethod);
    bm->colorSpace = NX_RGBCOLORSPACE;
    bm->bps = 8;
    bm->spp = 3;
    bm->vram = vram;
    /*bm->alpha = 0;*/
    /*bm->planar = 0;*/
    bm->type = NX_TWENTYFOURBITRGB;
    return bm;
}    

static void BM38SizeBits(BMClass *class, Bounds *b, int *size, int *rowBytes)
{
    *rowBytes = ((b->maxx - b->minx)*4 + 7)&~7;	/* double alignment */
    *size = *rowBytes * (b->maxy - b->miny);
}

static void BM38NewAlpha(Bitmap *bm, int initialize)
{
    if (bm->alpha || bm->vram) return;
    /* Don't set canFreeAlpha */
    bm->alpha = 1;
    bm->spp = 4;
    if (initialize) {
        int size;
	unsigned int *sp;
	sp = ((LocalBitmap *)bm)->bits;
	size = ((LocalBitmap *)bm)->byteSize >> 2;
	while (--size >= 0) *sp = *sp|0xFF, sp++;
    }
}

static void BM38Composite(Bitmap *bm, BMCompOp *bcop)
{
    int srw, drw, bmx, bmy;
    RectOp ro;
    LocalBitmap *dbm = (LocalBitmap *) bm;
    extern BMComposite38(RectOp *);

    bzero(&ro, sizeof(RectOp));
    ro.op = BMCompositeShortcut[bcop->op][bcop->srcAS][bcop->dstAS];

    if (ro.op < 0) return;
    ro.delta = bcop->dissolveFactor;
    ro.srcType = (bcop->srcType == BM_PATTERNSRC);

    ro.height = bcop->srcBounds.maxy - bcop->srcBounds.miny;
    ro.width = bcop->srcBounds.maxx - bcop->srcBounds.minx;

    /* DESTINATION */
    
    drw = dbm->rowBytes/4;
    bmx = bcop->dstBounds.minx - bm->bounds.minx;
    bmy = bcop->dstBounds.miny - bm->bounds.miny;
    ro.dstPtr = (int *) dbm->bits + bmy*drw + bmx;
    ro.dstRowBytes = drw - ro.width;
    ro.dstInc = 1;

    /* SOURCE */
    
    switch(bcop->srcType) {
    case BM_NOSRC:
	break;
    case BM_BITMAPSRC:
    {
	LocalBitmap *sbm = (LocalBitmap *) bcop->src.bm;
	int sbmx = bcop->srcBounds.minx - sbm->base.bounds.minx;
	int sbmy = bcop->srcBounds.miny - sbm->base.bounds.miny;
	srw = sbm->rowBytes/4; 
	ro.srcPtr = (int *)sbm->bits + sbmy * srw + sbmx;
	ro.srcRowBytes = srw - ro.width;
	ro.srcInc = 1;

	/* See if we can do operations in our favorite order */
	ro.ttob = ((dbm != sbm) || (bcop->dstBounds.miny -
				    bcop->srcBounds.miny < 0));
	ro.ltor = ((dbm != sbm) || (bcop->dstBounds.miny !=
				    bcop->srcBounds.miny)
		    || (bcop->dstBounds.minx - bcop->srcBounds.minx <= 0));

	
    /* Check for BottomToTop and/or RightToLeft cases and
	adjust if needed */
	if (!ro.ttob) {
	    ro.dstPtr += (ro.height - 1) * drw;
	    ro.srcPtr += (ro.height - 1) * srw;
	    if (ro.ltor) {
		ro.dstRowBytes = -drw - ro.width;
		ro.srcRowBytes = -srw - ro.width;
	    } else {
		ro.dstRowBytes = -ro.dstRowBytes;
		ro.srcRowBytes = -ro.srcRowBytes;
	    }
	}
	if (!ro.ltor) {
	    ro.srcInc = -ro.srcInc;
	    ro.dstInc = -ro.dstInc;
	    ro.srcPtr += ro.width - 1;
	    ro.dstPtr += ro.width - 1;
	    if (ro.ttob) {
		ro.dstRowBytes = drw + 1;
		ro.srcRowBytes = srw + 1;
	    }
	}
	/* if we're copying from vram, make sure to mask out the
	    phony alpha */
	if (ro.op == COPY && sbm->base.vram && !dbm->base.vram)
	ro.mask = RGBMASK;	/* mask off alpha */
	break;
    }
    case BM_PATTERNSRC:
    {
	Pattern *pat = bcop->src.pat;
	ro.srcValue = (*((int *) &pat->color) & RGBMASK) |
	    (pat->alpha & AMASK);
	break;
    }
    } /* switch */
    BMComposite38(&ro);
}

static void BM38ConvertFrom(Bitmap *db, Bitmap *sb, Bounds *dBounds,
			    Bounds *sBounds, DevPoint phase)
{
    LocalBitmap *sbm, *dbm;
    
    
    /* create alpha if necessary */
    if(sb->alpha && !db->alpha)
	bm_newAlpha(db, false/*don't initialize alpha*/);
    sbm = (LocalBitmap *) sb;
    dbm = (LocalBitmap *) db;
    switch(sb->type) {
    case NX_OTHERBMTYPE:
	break;
    case NX_TWOBITGRAY:
    	BM38Convert2to32(dbm, sbm, *dBounds, *sBounds);
	break;
    case NX_EIGHTBITGRAY:
	BM38Convert8to32(dbm, sbm, *dBounds, *sBounds);
	break;
    case NX_TWELVEBITRGB:
	BM38Convert16to32(dbm, sbm, *dBounds, *sBounds);
	break;
    case NX_TWENTYFOURBITRGB:
    {
	BMCompOp bc;
	bc.srcType = BM_BITMAPSRC;
	bc.op = COPY;
	bc.src.bm = sb;
	bc.srcBounds = *sBounds;
	bc.dstBounds = *dBounds;
	bc.srcAS = bc.dstAS = 0;
	bm_composite(db, &bc);
	break;
    }
    } /* end switch */
}

static void BM38_Mark(Bitmap *bm, MarkRec *mrec, DevPrim *clip)
{
    framebase = ((LocalBitmap *)bm)->bits;
    framebytewidth = ((LocalBitmap *)bm)->rowBytes;
    /* framelog2BD set by becomePSDevice's setupMark */
    Mark(mrec->device, mrec->graphic, clip, &mrec->info);
}    
	

static void BM38SetupPattern(PatternHandle h, DevMarkInfo *markInfo,
			     PatternData *data)
{
    /* ignore h, we only have BM38Pattern for ims11d1x */
    data->start = &data->value;
    data->end = data->start + 1;
    data->width = 1;
    data->constant = true;
    data->value = (*((int *)&markInfo->color) & RGBMASK) |
	ALPHAVALUE(markInfo->priv);
    data->id = 0;
}

/* Note: this code assumes that it only executes once (i.e. don't subclass
 * bm38 without changing this code!)
 */
static void BM38InitClassVars(BMClass *class)
{
    LocalBMClass *localclass = (LocalBMClass *) class;
    PMarkProcs mp;
    ImageArgs *ia;

    /* initialize superclass */
    (*localBM->_initClassVars)(class);

    /* now override superclasses' functions where needed */
    /* bitmap overrides */
    class->newAlpha = BM38NewAlpha;
    class->sizeBits = BM38SizeBits;
    class->composite = BM38Composite;
    class->convertFrom = BM38ConvertFrom;
    
    /*local-bitmap overrides */
    localclass->newFromData = BM38NewFromData;
    localclass->_mark = BM38_Mark;
    localclass->setupPattern = BM38SetupPattern;
    localclass->bmMarkProcs = *wdMarkProcs; 
    localclass->log2bd = 5; 
    mp = &localclass->bmMarkProcs;  
    mp->SetupImageArgs = localclass->setupImageArgs;
    
    ia = &localclass->bmImArgs;
    ia->bitsPerPixel = 32;
    ia->red.n = ia->green.n = ia->blue.n = 256;
    ia->red.first = 255<<24;
    ia->red.delta = -(1<<24);
    ia->green.first = 255<<16;
    ia->green.delta = -(1<<16);
    ia->blue.first = 255<<8;
    ia->blue.delta = -(1<<8);
    ia->firstColor = 255; /* the const added to every pixel in im110.c */
    ia->gray.n = ia->gray.first = ia->gray.delta = 0;
    
    /* just called to init deepPixOnes[5] */
    localclass->bmPattern = ConstRGBPattern(ia->red, ia->green, ia->blue,
					    ia->gray, 255, 32);
    localclass->grayPattern = NXGrayPat(3, BM38Grays, 0);
    localclass->bmPattern->setupPattern = localclass->setupPattern;
    ia->pattern = localclass->bmPattern;
    ia->procs = fmImageProcs;
    ia->data = NIL;
}

BM38Class _bm38 = {
    {
	/* LocalBMClass */
	{
	    /* BMClass */
	    0,
	    BM38InitClassVars,
	    BM38New,
	},
	BM38NewFromData,
    },
};

