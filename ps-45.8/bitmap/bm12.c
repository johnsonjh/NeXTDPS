 
/* bm12.c
 * BMClass : LocalBMClass : BM12Class
 * 2-bit bitmap class implementation (1 is WHITE)
 * Note: This does not use built-in write function hardware for compositing.
 * Created 20Jul90 Ted Cohn
 */

#import PACKAGE_SPECS
#import "device.h"
#import "bitmap.h"
#import "bm12.h"

extern PatternHandle GryPat4Of4();
extern PMarkProcs wdMarkProcs;
extern PImageProcs fmImageProcs;
extern int framebytewidth;
extern unsigned int *framebase;

DevHalftone bm12DevHalftone;
static DevScreen bm12DevScreen;
static const unsigned char bm12Thresholds[64] = {
      4,195, 52,243,   16,207, 64,255,
    131, 68,179,116,  143, 80,191,128,
     36,227, 20,211,   48,239, 32,223,
    163,100,147, 84,  175,112,159, 96,
     12,203, 60,251,    8,199, 56,247,
    139, 76,187,124,  135, 72,183,120,
     44,235, 28,219,   40,231, 24,215,
    171,108,155, 92,  167,104,151, 88
};

/* dataOp gives the MoveRect mode to be used to get the dest data
 * from the source data given a source and dest alpha state and a
 * particular compositing operation.
 */
static const char dataOp[NCOMPOSITEOPS][2][2] = {
    /* [mode][srcAS][dstAS] */
    {{F0(0),F0(0)},{F0(0),F0(0)}},	/* CLEAR */
    {{F1(2),F1(2)},{F1(2),F1(2)}},	/* COPY */
    {{F2(0),F2(0)},{F1(2),F1(2)}},	/* SOVER */
    {{F2(3),F1(2)},{F2(3),F1(2)}},	/* SIN */
    {{F2(2),F0(1)},{F2(2),F0(1)}},	/* SOUT */
    {{F3(0),F2(0)},{F2(3),F1(2)}},	/* SATOP */
    {{F2(6),F0(3)},{F2(6),F0(3)}},	/* DOVER */
    {{F1(1),F1(1)},{F0(1),F0(1)}},	/* DIN */
    {{F1(3),F1(3)},{F0(0),F0(0)}},	/* DOUT */
    {{F3(3),F1(1)},{F2(2),F0(0)}},	/* DATOP */
    {{F3(2),F1(3)},{F2(2),F0(0)}},	/* XOR */
    {{F1(5),F1(5)},{F1(5),F1(5)}},	/* PLUSD (PLUS) */
    {{F0(4),F0(4)},{F0(4),F0(4)}},	/* HIGHLIGHT */
    {{F1(8),F1(8)},{F1(8),F1(8)}},	/* PLUSL */
    /* DISSOLVE handled in special code */
};

static const char alphaOp[NCOMPOSITEOPS][2] = {
    /* [mode][srcAS] */
    {F0(0),F0(0)},	/* CLEAR */
    {F1(2),F0(1)},	/* COPY */
    {F1(6),F0(1)},	/* SOVER */
    {F1(1),F0(3)},	/* SIN */
    {F1(0),F0(2)},	/* SOUT */
    {F0(3),F0(3)},	/* SATOP */
    {F1(6),F0(1)},	/* DOVER */
    {F1(1),F0(1)},	/* DIN */
    {F1(3),F0(0)},	/* DOUT */
    {F1(2),F0(1)},	/* DATOP */
    {F1(4),F0(2)},	/* XOR */
    {F1(5),F0(1)},	/* PLUSD (PLUS) */
    {F0(4),F0(4)},	/* HIGHLIGHT */
    {F1(5),F0(1)},	/* PLUSL */
    /* DISSOLVE handled in special code */
};

/* s*FromAlphaToData flag the rarer cases where s[*], used in creating
   new dest DATA, comes from ALPHA (from data being more common). */

static const int s0FromAlphaToData[2][2] = {
    /* [srcAS][dstAS] */
    {1<<DIN|1<<DOUT, 1<<DIN|1<<DOUT|1<<DATOP|1<<XOR|1<<DISSOLVE},
    {0, 0}
};

static const int s1FromAlphaToData[2][2] = {
    /* [srcAS][dstAS] */
    {1<<SOVER|1<<SIN|1<<SOUT|1<<SATOP|1<<DOVER|1<<DATOP|1<<XOR|1<<DISSOLVE,
     1<<SOVER|1<<SATOP},
    {1<<SIN|1<<SOUT|1<<SATOP|1<<DOVER|1<<DATOP|1<<DISSOLVE, 0}
};

static const int s2FromAlphaToData[2][2] = {
    /* [srcAS][dstAS] */
    {1<<SATOP|1<<DATOP|1<<XOR|1<<DISSOLVE, 0},
    {0, 0}
};

/* sFromDestToData flag the rarer cases where s[*], used in creating
   new dest DATA, comes from DEST (from source being more common).
   They apply to both s[0] & s[1] and are indexed by [dstAState]. */

static const int sFromDestToData[2] = /* [dstAS] */
    {1<<SIN|1<<SOUT|1<<SATOP|1<<DOVER|1<<DATOP|1<<XOR|1<<DISSOLVE, 0};

static SCANTYPE logray[2] 	= { 0x44444444, 0x11111111 } ;
static SCANTYPE medgray[2] 	= { 0x99999999, 0x66666666 } ;
static SCANTYPE higray[2]  	= { 0xeeeeeeee, 0xbbbbbbbb } ;
static PatternData BM12Grays[3] = {
    /* 	start,		end,			width,	other */
    { 	&logray[0], 	&logray[0] + 2, 	1, 	0, 0, 0 },
    { 	&medgray[0], 	&medgray[0] + 2, 	1, 	0, 0, 0 },
    { 	&higray[0], 	&higray[0] + 2, 	1,	0, 0, 0 },
};

static Bitmap *BM12New(BMClass *class, Bounds *bounds)
{
    Bitmap *bm;
    Bounds b = *bounds;

    /* i.e. `super new' */
    /* FIX: should use screen left edge for alignment instead of assuming
     * coordinate system is aligned on 16-pixel boundaries */
    b.minx &= ~0xf;
    bm = (Bitmap *) (*((BMClass *)localBM)->new)(class, &b);
    bm->colorSpace = NX_ONEISWHITECOLORSPACE;
    bm->bps = BM12BPS;	/* 2 */
    bm->spp = BM12SPP;	/* 1 */
    bm->hasAlpha = 0;
    bm->isPlanar = 1;
    bm->type = NX_TWOBITGRAY;
    return bm;
}

static Bitmap *BM12NewFromData(LocalBMClass *class, Bounds *b, void *bits,
			       void *abits, int byteSize, int rowBytes,
			       int vram, int freeMethod)
{
    Bitmap *bm;

    bm = (Bitmap *) (*((LocalBMClass*)localBM)->newFromData)(class, b,
	bits, abits, byteSize, rowBytes, vram, freeMethod);
    bm->colorSpace = NX_ONEISWHITECOLORSPACE;
    bm->bps = BM12BPS;	/* 2 */
    bm->spp = BM12SPP;	/* 1 */
    bm->hasAlpha = (abits != NULL);
    bm->isPlanar = 1;
    bm->type = NX_TWOBITGRAY;
    return bm;
}

static void BM12SizeBits(BMClass *class, Bounds *b, int *size, int *rowBytes)
{
    *rowBytes = (((b->maxx - b->minx - 1)>>4) + 1)*4;   /* bytes per scanline */
    *size = (*rowBytes)*(b->maxy-b->miny);
}

/* override newAlpha to perform cache disabling on allocated bits */
static void BM12NewAlpha(Bitmap *bm)
{
    LocalBitmap *lbm = (LocalBitmap *)bm;

    /* `super' NewAlpha */
    (*localBM->newAlpha)(bm);
}

void BM12SetupBits(LocalBitmap *lbm, Bounds *bounds, int leftShift, int
    topToBottom, int leftToRight, BitsOrPatInfo *dinfo, BitsOrPatInfo *ainfo,
    DevMarkInfo *info)
{   
    int n, offset;
    
    n = lbm->rowBytes >> 2;
    leftShift = BM12scanmask & (bounds->minx - lbm->base.bounds.minx -
	leftShift);
    offset = ((bounds->miny - lbm->base.bounds.miny) * n) +
	((bounds->minx - lbm->base.bounds.minx - leftShift) >> BM12scanshift);
    if (!topToBottom) {
	offset += (bounds->maxy - bounds->miny - 1) * n;
	n = -n;
    }
    dinfo->type	         	= bitmapType;
    dinfo->data.bm.pointer	= ((uint *) lbm->bits) + offset;
    dinfo->data.bm.n		= n;
    dinfo->data.bm.leftShift	= leftShift;
    if (lbm->abits) {
	ainfo->type		 = bitmapType;
	ainfo->data.bm.pointer	 = ((uint *) lbm->abits) + offset;
	ainfo->data.bm.n	 = n;
	ainfo->data.bm.leftShift = leftShift;
    }
}


void BM12SetupPat(Pattern *pat, DevPoint bmOffset, Bounds *bounds,
    int usealpha, int topToBottom, BitsOrPatInfo *info)
{
    int k, m;
    PatternData pData;
    DevMarkInfo grayInfo;
    NextGSExt extrec, *extp;
    
    grayInfo.halftone = (pat->halftone) ? pat->halftone : &bm12DevHalftone;
    grayInfo.screenphase = pat->phase;
    grayInfo.offset.x = bmOffset.x; /* .y not used by SetupPattern */

    if (!usealpha && pat->alpha != OPAQUE) { /* premult in BM12SetupPattern */ 
	extrec.alpha = pat->alpha;
	extp = &extrec;
	grayInfo.priv = (PCard8)&extp;
    } else
	grayInfo.priv = NULL;
    /* invert alpha so 0 turns on all bits, as for gray */
    ((DevColorVal *)&grayInfo.color)->white = 
	usealpha ? ~pat->alpha : ((DevColorVal *)&pat->color)->white;
    SetupPattern(_bm12.base.bmPattern, &grayInfo, &pData);
    if (pData.constant) {
	info->type = constantType;
	info->data.cons.value = pData.value;
	return;
    }
    info->type = patternType;
    if (!topToBottom) {
	info->data.pat.yBase = (uint *) pData.end - pData.width;
	info->data.pat.n = -pData.width;
	info->data.pat.yLimit = (uint *) pData.start - pData.width;
	k = bounds->maxy - bmOffset.y - grayInfo.screenphase.y - 1;
    } else { 
	info->data.pat.yBase = (uint *) pData.start;
	info->data.pat.n = pData.width;
	info->data.pat.yLimit = (uint *) pData.end;
	k = bounds->miny - bmOffset.y - grayInfo.screenphase.y;
    }
    m = (pData.end - pData.start) / pData.width;
    k = k - (k/m)*m; /* Broken-out implementation of % b/c of negs */
    if (k < 0)
	k += m;
    info->data.pat.xInitial = info->data.pat.yInitial = 
	(uint *)(pData.start + (k * pData.width));
    if (pData.width > 1) {
	k = (bounds->minx - grayInfo.screenphase.x) >> BM12scanshift;
	m = pData.width;
	k = k - (k/m)*m; /* Broken-out implementation of % */
	if (k < 0)
	    k += m;
	info->data.pat.xInitial += k;
    }
}

void BM12Composite(Bitmap *bm, BMCompOp *bcop)
{
    DevPoint bmOffset;
    int lToR, tToB, op;
    int width, height, leftShift;
    int d_ofs, num_ints, num_lastbits;
    LocalBitmap *dbm = (LocalBitmap *)bm, *sbm;
    BitsOrPatInfo srcDI, srcAI, dstDI, dstAI, disAI, *s[3];

    bm_becomePSDevice(bm);
    /* Give the bcop->info our halftone if it doesn't have one already */
    if (!bcop->info.halftone) bcop->info.halftone = &bm12DevHalftone;

    /* Now window coords plus offset yields bitmap coords. */
    bmOffset.x = bcop->info.offset.x - bm->bounds.minx;
    bmOffset.y = bcop->info.offset.y - bm->bounds.miny;

    height = bcop->srcBounds.maxy - bcop->srcBounds.miny;
    width = bcop->srcBounds.maxx - bcop->srcBounds.minx;

    d_ofs = (bcop->dstBounds.minx - bm->bounds.minx) & BM12scanmask;
    num_ints = width >> BM12scanshift;
    num_lastbits = width & BM12scanmask;

    sbm = (LocalBitmap *)bcop->src.bm;
    if (bcop->srcType == BM_BITMAPSRC) {
	tToB = ((dbm != sbm) || (bcop->dstBounds.miny -
	    bcop->srcBounds.miny < 0));
	lToR = ((dbm != sbm) || (bcop->dstBounds.miny !=
	    bcop->srcBounds.miny) || (bcop->dstBounds.minx -
		bcop->srcBounds.minx <= 0));
    } else lToR = tToB = 1;

    /* SET UP DESTINATION DATABITS AND (POSSIBLY) ALPHABITS FROM BITMAP */
    leftShift = bcop->dstBounds.minx - bm->bounds.minx;
    BM12SetupBits(dbm, &bcop->dstBounds, leftShift, tToB, lToR, &dstDI, &dstAI,
	&bcop->info);

    if (bcop->srcType == BM_BITMAPSRC) {
	    /* Modify offset based on location of destination bitmap */
	    bcop->info.offset.x -= bcop->dstBounds.minx - (leftShift & 0xF);
	    bcop->info.offset.y -= bcop->dstBounds.miny;
	    BM12SetupBits(sbm, &bcop->srcBounds, leftShift, tToB,
			  lToR, &srcDI, &srcAI, &bcop->info);
	    bcop->info.offset.x += bcop->dstBounds.minx - (leftShift & 0xF);
	    bcop->info.offset.y += bcop->dstBounds.miny;

	    if (!sbm->abits)
		BM12SetupPat(blackpattern, bmOffset, &bcop->srcBounds, true,
			     tToB, &srcAI);
    } else if (bcop->srcType == BM_PATTERNSRC) { /* BM_PATTERNSRC */
	    BM12SetupPat(bcop->src.pat, bmOffset, &bcop->srcBounds, false,
			 tToB, &srcDI);
	    if (bcop->srcAS == A_BITS) {
		BM12SetupPat(bcop->src.pat, bmOffset, &bcop->srcBounds, true,
			     tToB, &srcAI);
	    }
    }

    /* Find final op, set up all (up to three) stages */
    if (bcop->op == DISSOLVE) {
	/* dissolve degree passed in bcop->alpha */
 	Pattern *pat = PNewColorAlpha(0, ~bcop->dissolveFactor,
				      bcop->info.halftone);
	BM12SetupPat(pat, bmOffset, &bcop->srcBounds, true, tToB, &disAI);
	PFree(pat);
	op = F3(3);
	s[0] = &srcDI;
	s[1] = s[2] = &disAI;
    } else {
	register int srcAS, dstAS, opMask;
	
	srcAS = bcop->srcAS;
	dstAS = bcop->dstAS;
	op = dataOp[bcop->op][srcAS][dstAS];
	if (op >= FIRSTF1OP) {
	    opMask = (1 << bcop->op);
	    s[0] = (opMask & s0FromAlphaToData[srcAS][dstAS]) ? &srcAI:&srcDI;
	    if (op >= FIRSTF2OP) {
		s[1] = (opMask & s1FromAlphaToData[srcAS][dstAS])
		    ? ((opMask & sFromDestToData[dstAS]) ? &dstAI : &srcAI)
		    : ((opMask & sFromDestToData[dstAS]) ? &dstDI : &srcDI);
		if (op >= FIRSTF3OP)
		    s[2] = (opMask & s2FromAlphaToData[srcAS][dstAS])
			? &srcAI : &srcDI;
	    }
	}
    }

    /* Yea!  Finally!  Write the data channel! */
    MRMoveRect(s, &dstDI, op, d_ofs, height, num_ints, num_lastbits, lToR);

    /* OK, data portion is done; do alpha if necessary */
    if (bcop->dstAS == A_BITS && dbm->abits) {
	if (bcop->op == DISSOLVE) {
	    s[0] = &srcAI;
	    s[1] = s[2] = &disAI;
	} else {
	    op = alphaOp[bcop->op][bcop->srcAS];
	    if (op >= FIRSTF1OP)
		s[0] = &srcAI;
	}
	MRMoveRect(s, &dstAI, op, d_ofs, height, num_ints, num_lastbits, lToR);
    }
}

static void BM12ConvertFrom(Bitmap *d, Bitmap *s, Bounds *db, Bounds *sb,
    DevPoint phase)
{
    LocalBitmap *sbm, *dbm;
    
    /* create alpha if necessary */
    if(s->usesAlpha && !d->usesAlpha)
	bm_newAlpha(d);
    sbm = (LocalBitmap *) s;
    dbm = (LocalBitmap *) d;
    switch(s->type) {
    case NX_OTHERBMTYPE:
	break;
    case NX_TWOBITGRAY:
    {
	BMCompOp bc;
	bc.srcType = BM_BITMAPSRC;
	bc.op = COPY;
	bc.src.bm = s;
	bc.srcBounds = *sb;
	bc.dstBounds = *db;
	bc.srcAS = bc.dstAS = 0;
	bm_composite(d, &bc);
	break;
    }
    case NX_EIGHTBITGRAY:
	break;
    case NX_TWELVEBITRGB:	/* fall thru */
    case NX_TWENTYFOURBITRGB:
    {
	DevPoint size, dstOffset, srcOffset;
	size.x = sb->maxx - sb->minx;
	size.y = sb->maxy - sb->miny;
	srcOffset.x = sb->minx - s->bounds.minx;
	srcOffset.y = sb->miny - s->bounds.miny;
	dstOffset.x = db->minx - d->bounds.minx;
	dstOffset.y = db->miny - d->bounds.miny;
	if (s->type == NX_TWELVEBITRGB)
	    BM12Convert16to2((unsigned short *)sbm->bits, dbm->bits,
	        sbm->abits, size, srcOffset, sbm->rowBytes, dstOffset,
		dbm->rowBytes, phase);
	else
	    BM12Convert32to2(sbm->bits, dbm->bits, sbm->abits, size,
	    	srcOffset, sbm->rowBytes, dstOffset, dbm->rowBytes, phase);
    }
    break;
    }
}


void BM12_Mark(Bitmap *bm, MarkRec *mrec, DevPrim *clip)
{
    LocalBitmap *lbm;
    DevHalftone *mriht;

    lbm = (LocalBitmap *) bm;
    if (!(mriht = mrec->info.halftone)) mrec->info.halftone = &bm12DevHalftone;
    framebytewidth = lbm->rowBytes;
    
    if (lbm->abits) {
	DevImage *image, *origimage;
	int items, origitems, ncolors;
	unsigned char *samplesZero, saveWhite;
	DevPrivate *savePriv = mrec->info.priv;

	/* Set alpha color, offset and framedevice for alpha bitmap */
	saveWhite = ((DevColorVal *)&mrec->info.color)->white;
 	((DevColorVal *)&mrec->info.color)->white =
	    ALPHAVALUE(PSGetGStateExt(NULL));
	mrec->info.priv = NULL;
	framebase = lbm->abits;
	switch (mpmi->itype) {
	case none:
	    Mark(mrec->device, mrec->graphic, clip, &mrec->info);
	    break;
	case regular: /* use image's trapezoids as opaque area */
	    origimage = image = mrec->graphic->value.image;
	    origitems = mrec->graphic->items;
	    mrec->graphic->type = trapType;
	    for (items = origitems; items--; image++) {
		/* some waste using whole markBds in rare multi-item case */
		mrec->graphic->items = image->info.trapcnt;
		mrec->graphic->value.trap = image->info.trap;
		Mark(mrec->device, mrec->graphic, clip, &mrec->info);
	    }
	    mrec->graphic->type = imageType;
	    mrec->graphic->items = origitems;
	    mrec->graphic->value.image = origimage;
	    break;
	case alphaimage:
	    /* Need to draw into just the alpha plane */
	    image = mrec->graphic->value.image;
	    samplesZero = image->source->samples[0];
	    ncolors = image->source->nComponents;
	    
	    /* reuse and restore image data */
	    image->source->samples[0] = image->source->samples[ncolors];
	    image->source->nComponents = 1;
	    Mark(mrec->device, mrec->graphic, clip, &mrec->info);
	    image->source->samples[0] = samplesZero;
	    image->source->nComponents = ncolors;
	    break;
	}
	/* Restore white and priv fields */
	((DevColorVal *)&mrec->info.color)->white = saveWhite;
	mrec->info.priv = savePriv;
    }

    /* Mark in the data plane last and always */
    framebase = lbm->bits;
    Mark(mrec->device, mrec->graphic, clip, &mrec->info);

    /* Restore for other devices and clean up */
    if (!mriht) mrec->info.halftone = NULL;
}


static void BM12SetupPattern(PatternHandle h, DevMarkInfo *markInfo,
    PatternData *data)
{
    PNextGSExt ep;
    DevColorVal *miColor = NULL;
    unsigned int orig, gray;

#if 0
    miColor = (DevColorVal *)&markInfo->color;
    gray = orig = miColor->white;
    if (markInfo->priv
      && (ep = *(PNextGSExt *)markInfo->priv) && (ep->alpha != OPAQUE)) {
	/* A gs extension exists so may have to premultiply */
	DebugAssert(gray <= ep->alpha); /* so next expr can't overflow */
	gray += OPAQUE - ep->alpha;
	miColor->white = gray;
    }
#endif
    if (gray != 0 && gray != 85 && gray != 170 && gray != 255)
	Gry4Of4Setup(h, markInfo, data);
    else {
	gray |= gray << 8;
	gray |= gray << 16;
	data->value = gray;		/* was ~gray in mp12 */
	data->start = &data->value;
	data->end = &data->value + 1;
	data->constant = true;
	data->width = 1;
    }
#if 0
    miColor->white = orig;
#endif
}

/* Note: this code assumes that it only executes once (i.e. don't subclass
 * bm12 without changing this code!)
 */
static void BM12InitClassVars(BMClass *class)
{
    PMarkProcs mp;
    ImageArgs *ia;
    DevColorData devColorData;
    LocalBMClass *localclass = (LocalBMClass *)class;

    /* initialize superclass */
    (*localBM->_initClassVars)(class);

    /* override appropriate superclass method */
    /* bitmap overrides */
    class->sizeBits = BM12SizeBits;
    class->composite = BM12Composite;
    class->convertFrom = BM12ConvertFrom;
    class->newAlpha = BM12NewAlpha;
    /* don't override makePublic */

    /* override local bitmap methods */
    bm12DevScreen.width = bm12DevScreen.height = 8;
    bm12DevScreen.thresholds = (unsigned char *)&bm12Thresholds[0];
    bm12DevHalftone.red = bm12DevHalftone.green = bm12DevHalftone.blue =
	bm12DevHalftone.white = &bm12DevScreen;
    /* protect static halftones from the grim reaper */
    DevAddHalftoneRef(&bm12DevHalftone);
    localclass->newFromData = BM12NewFromData;
    localclass->_mark = BM12_Mark;
    localclass->setupPattern = BM12SetupPattern;
    localclass->bmMarkProcs = *wdMarkProcs; 
    localclass->log2bd = BM12LOG2BD; 
    localclass->defaultHalftone = &bm12DevHalftone; 
    mp = &localclass->bmMarkProcs;  
    mp->SetupImageArgs = localclass->setupImageArgs;
    
    ia = &localclass->bmImArgs;
    ia->bitsPerPixel = BM12BPP;
    ia->red.first = ia->green.first = ia->blue.first = 0;
    ia->red.n = ia->green.n = ia->blue.n = 0;
    ia->red.delta = ia->green.delta = ia->blue.delta = 0;
    ia->gray.n = BM12GRAYS;
    ia->gray.first = 3;
    ia->gray.delta = -1;
    
    devColorData.first = 3;
    devColorData.n = BM12GRAYS;
    devColorData.delta = -1;
    localclass->bmPattern = GryPat4Of4(devColorData, BM12BPP);
    localclass->grayPattern = NXGrayPat(3, BM12Grays, 0xffffffff);
    localclass->bmPattern->setupPattern = localclass->setupPattern;
    
    ia->pattern = localclass->bmPattern;
    ia->procs = fmImageProcs;
    ia->data = NIL;
    MRInitialize();	/* initialize moverect */
}

BM12Class _bm12 = {
    {
	/* LocalBMClass */
	{
	    /* BMClass */
	    0,
	    BM12InitClassVars,
	    BM12New,
	},
	BM12NewFromData,
    },
};

