
/* bm34.c --- BMClass : LocalBMClass : BM34Class */

#import PACKAGE_SPECS
#import BINTREE
#import WINDOWDEVICE
#import "bitmap.h"
#import "bm34.h"
#import "devmark.h"

extern Card8 framelog2BD;
extern int framebytewidth;
extern PMarkProcs wdMarkProcs;
extern unsigned int *framebase;
extern PImageProcs fmImageProcs;

DevHalftone bm34DevHalftone;
static DevScreen bm34DevScreen;
static const unsigned char bm34Thresholds[64] = {
      4,195, 52,243,   16,207, 64,255,
    131, 68,179,116,  143, 80,191,128,
     36,227, 20,211,   48,239, 32,223,
    163,100,147, 84,  175,112,159, 96,
     12,203, 60,251,    8,199, 56,247,
    139, 76,187,124,  135, 72,183,120,
     44,235, 28,219,   40,231, 24,215,
    171,108,155, 92,  167,104,151, 88
};

static const SCANTYPE logray0[2] 	= { 0x000f555f, 0x555f000f };
static const SCANTYPE logray1[2] 	= { 0x555f000f, 0x000f555f };
static const SCANTYPE medgray0[2]	= { 0x555faaaf, 0xaaaf555f };
static const SCANTYPE medgray1[2]	= { 0xaaaf555f, 0x555faaaf };
static const SCANTYPE higray0[2]	= { 0xaaafffff, 0xffffaaaf };
static const SCANTYPE higray1[2]	= { 0xffffaaaf, 0xaaafffff };

static const PatternData BM34Grays[3] = {
    /* 	start,		end,			width,	other */
    { 	&logray0[0], 	&logray0[0] + 2, 	1, 	0, 0, 0 },
    { 	&logray1[0], 	&logray1[0] + 2, 	1, 	0, 0, 0 },
    { 	&medgray0[0], 	&medgray0[0] + 2, 	1, 	0, 0, 0 },
    { 	&medgray1[0], 	&medgray1[0] + 2, 	1, 	0, 0, 0 },
    { 	&higray0[0], 	&higray0[0] + 2, 	1,	0, 0, 0 },
    { 	&higray1[0], 	&higray1[0] + 2, 	1,	0, 0, 0 },
};
	
static Bitmap *BM34New(BMClass *class, Bounds *b, int local)
{
    Bitmap *bm;

    /* i.e. `super new' */
    bm = (Bitmap *) (*((BMClass *)localBM)->new)(class, b, local);
    bm->colorSpace = NX_RGBCOLORSPACE;
    bm->bps = 4;
    bm->spp = 3;
    /*
    bm->alpa = 0;
    bm->vram = 0;
    bm->planar = 0;
    */
    bm->type = NX_TWELVEBITRGB;
    return bm;
}
    
static Bitmap *BM34NewFromData(LocalBMClass *class, Bounds *b, void *bits,
			       void *abits, int byteSize, int rowBytes,
			       int vram, int freeMethod)
{
    Bitmap *bm;

    bm = (Bitmap *) (*((LocalBMClass *)localBM)->newFromData)
		    (class, b, bits, abits, byteSize, rowBytes, vram,
		     freeMethod);
    bm->colorSpace = NX_RGBCOLORSPACE;
    bm->bps = 4;
    bm->spp = 3;
    bm->vram = vram;
    /*
    bm->alpha = 0;
    bm->planar = 0;
    */
    bm->type = NX_TWELVEBITRGB;
    return bm;
}    

static void BM34SizeBits(BMClass *class, Bounds *b, int *size, int *rowBytes)
{
    *rowBytes = ((b->maxx - b->minx)*2+7)&~7;	/* double align */
    *size = *rowBytes * (b->maxy - b->miny);
}

static void BM34NewAlpha(Bitmap *bm, int initialize)
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
	while (--size >= 0) *sp = *sp|0x000F000F, sp++;
    }
}

static void BM34Composite(Bitmap *bm, BMCompOp *bcop)
{
    int srw, drw, bmx, bmy;
    RectOp ro;
    LocalBitmap *dbm = (LocalBitmap *) bm;
    extern BMComposite34(RectOp *);

    bzero(&ro, sizeof(RectOp));
    ro.op = BMCompositeShortcut[bcop->op][bcop->srcAS][bcop->dstAS];

    if (ro.op < 0) return;
    ro.delta = bcop->dissolveFactor;
    ro.srcType = (bcop->srcType == BM_PATTERNSRC);

    ro.height = bcop->srcBounds.maxy - bcop->srcBounds.miny;
    ro.width = bcop->srcBounds.maxx - bcop->srcBounds.minx;

    /* DESTINATION */
    
    drw = dbm->rowBytes/sizeof (pixel_t);	// Scale from bytes to pixels
    bmx = bcop->dstBounds.minx - bm->bounds.minx;
    bmy = bcop->dstBounds.miny - bm->bounds.miny;
    ro.dstPtr = (pixel_t *)dbm->bits + bmy*drw + bmx;
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
	srw = sbm->rowBytes/sizeof (pixel_t); 	// Scale from bytes to pixels
	ro.srcPtr = (pixel_t *)sbm->bits + sbmy * srw + sbmx;
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
	unsigned int s;
	
	s = (*((int *) &pat->color) & 0xFFFFFF00) | (pat->alpha & 0xFF);
	ro.srcValue =   ((s >> 4)  & 0x000f) | 
			((s >> 8)  & 0x00f0) |
			((s >> 12) & 0x0f00) |
			((s >> 16) & 0xf000);
	ro.srcValue |= ro.srcValue << 16;
	break;
    }
    } /* switch */
    BMComposite34(&ro);
}

static void BM34ConvertFrom(Bitmap *db, Bitmap *sb, Bounds *dBounds,
			    Bounds *sBounds, DevPoint phase)
{
    LocalBitmap *sbm, *dbm;
    
    sbm = (LocalBitmap *) sb;
    dbm = (LocalBitmap *) db;
    switch(sb->type) {
    case NX_OTHERBMTYPE:
	break;
    case NX_TWOBITGRAY:
    	BM34Convert2to16(dbm, sbm, *dBounds, *sBounds);
	break;
    case NX_EIGHTBITGRAY:
	BM34Convert8to16(dbm, sbm, *dBounds, *sBounds);
	break;
    case NX_TWENTYFOURBITRGB:
	BM34Convert32to16(dbm, sbm, *dBounds, *sBounds);
	break;
    case NX_TWELVEBITRGB:
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

static void BM34_Mark(Bitmap *bm, MarkRec *mrec, DevPrim *clip)
{
    framebase = ((LocalBitmap *)bm)->bits;
    framebytewidth = ((LocalBitmap *)bm)->rowBytes;
    /* Remember: framelog2BD set by becomePSDevice's setupMark */
    Mark(mrec->device, mrec->graphic, clip, &mrec->info);
}    
	

static void BM34SetupPattern(PatternHandle h, DevMarkInfo *markInfo,
			     PatternData *data)
{
    int a,c;
    /* ignore h, we only have BM34Pattern for ims11d1x */
    data->start = &data->value;
    data->end = data->start + 1;
    data->width = 1;
    data->constant = true;
    c = (*((unsigned int *)&markInfo->color) & 0xffffff00);
    a = ALPHAVALUE(markInfo->priv);
    data->value = (a >> 4) & 0xf | 
	    		(c >> 8) & 0xf0 |
			(c >> 12) & 0xf00 |
			(c >> 16) & 0xf000;
    data->value |= data->value << 16;

    data->id = 0;
}

/* Note: this code assumes that it only executes once (i.e. don't subclass
 * bm34 without changing this code!)
 */
static void BM34InitClassVars(BMClass *class)
{
    LocalBMClass *localclass = (LocalBMClass *) class;
    PMarkProcs mp;
    ImageArgs *ia;

    /* initialize superclass */
    (*localBM->_initClassVars)(class);

    /* now override superclasses' functions where needed */
    /* bitmap overrides */
    class->newAlpha = BM34NewAlpha;
    class->sizeBits = BM34SizeBits;
    class->composite = BM34Composite;
    class->convertFrom = BM34ConvertFrom;
    
    bm34DevScreen.width = bm34DevScreen.height = 8;
    bm34DevScreen.thresholds = (unsigned char *)&bm34Thresholds[0];
    bm34DevHalftone.red = bm34DevHalftone.green = bm34DevHalftone.blue =
			bm34DevHalftone.white = &bm34DevScreen;
    /* protect static halftones from the grim reaper */
    DevAddHalftoneRef(&bm34DevHalftone);
    /*local-bitmap overrides */
    localclass->newFromData = BM34NewFromData;
    localclass->_mark = BM34_Mark;
    localclass->setupPattern = BM34SetupPattern;
    localclass->defaultHalftone = &bm34DevHalftone; 
    localclass->bmMarkProcs = *wdMarkProcs; 
    localclass->log2bd = 4; 
    mp = &localclass->bmMarkProcs;  
    mp->SetupImageArgs = localclass->setupImageArgs;
    
    ia = &localclass->bmImArgs;
    ia->bitsPerPixel = 16;
    ia->red.n = ia->green.n = ia->blue.n = 16; 
    ia->red.first = 15<<12;
    ia->red.delta = -(1<<12);
    ia->green.first = 15<<8;
    ia->green.delta = -(1<<8);
    ia->blue.first = 15<<4;
    ia->blue.delta = -(1<<4);
    ia->firstColor = 15; /* the const added to every pixel in im110.c */
    ia->gray.n = ia->gray.first = ia->gray.delta = 0;
    
    /* just called to init deepPixOnes[5] */
    localclass->bmPattern = RGBPattern(ia->red, ia->green, ia->blue,
	ia->gray, 15, 16);
    localclass->bmPattern->setupPattern = localclass->setupPattern;
    localclass->grayPattern = NXGrayPat(3, BM34Grays, 0);
    ia->pattern = localclass->bmPattern;
    ia->procs = fmImageProcs;
    ia->data = NIL;
}

/* NOTES FROM PHONE CONVERSION WITH JIM SANDMAN 7/18/90:

 1. Use RGBPattern to dither in 12 bit rgb.
 2. Use ConstRGBPattern to not dither in 12 bit rgb.
 3. Use the current megapixel's threshold table for 12 bit rgb.
 4. Unconvered potential bug.  Jack seems to be overriding setuppattern
 */

BM34Class _bm34 = {
    {
	/* LocalBMClass */
	{
	    /* BMClass */
	    0,
	    BM34InitClassVars,
	    BM34New,
	},
	BM34NewFromData,
    },
};

