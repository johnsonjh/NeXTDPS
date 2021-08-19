 
/* mp12.c -- MegaPixel implementation of 2-bit gray bitmaps */

#import PACKAGE_SPECS
#import "device.h"
#import "except.h"
#import "bitmap.h"
#import "mp.h"
#import "mp12.h"

extern PatternHandle GryPat4Of4();
extern PMarkProcs wdMarkProcs;
extern PImageProcs fmImageProcs;
extern int framebytewidth;
extern unsigned int *framebase;

DevHalftone mpDevHalftone;
static DevScreen mpDevScreen;
static const unsigned char mpThresholds[64] = {
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

static const SCANTYPE logray0[2] 	= { 0xeeeeeeee, 0xbbbbbbbb } ;
static const SCANTYPE logray1[2] 	= { 0xbbbbbbbb, 0xeeeeeeee } ;
static const SCANTYPE medgray0[2] 	= { 0x99999999, 0x66666666 } ;
static const SCANTYPE medgray1[2] 	= { 0x66666666, 0x99999999 } ;
static const SCANTYPE higray0[2]  	= { 0x44444444, 0x11111111 } ;
static const SCANTYPE higray1[2]  	= { 0x11111111, 0x44444444 } ;
static const PatternData MP12Grays[6] = {
    /* 	start,		end,			width,	other */
    { 	&logray0[0], 	&logray0[0] + 2, 	1, 	0, 0, 0 },
    { 	&logray1[0], 	&logray1[0] + 2, 	1, 	0, 0, 0 },
    { 	&medgray0[0], 	&medgray0[0] + 2, 	1, 	0, 0, 0 },
    { 	&medgray1[0], 	&medgray1[0] + 2, 	1, 	0, 0, 0 },
    { 	&higray0[0], 	&higray0[0] + 2, 	1,	0, 0, 0 },
    { 	&higray1[0], 	&higray1[0] + 2, 	1,	0, 0, 0 },
};

static Bitmap *MP12New(BMClass *class, Bounds *bounds, int local)
{
    Bitmap *bm;
    Bounds b = *bounds;

    /* i.e. `super new' */
    /* FIX: should use screen left edge for alignment instead of assuming
     * coordinate system is aligned on 16-pixel boundaries */
    b.minx &= ~0xf; 
    bm = (Bitmap *) (*((BMClass *)localBM)->new)(class, &b, local);
    bm->colorSpace = NX_ONEISBLACKCOLORSPACE;
    bm->bps = 2;
    bm->spp = 1;
    /*bm->vram = 0;*/
    bm->planar = 1;
    bm->type = NX_OTHERBMTYPE;
    if (use_wf_hardware) {
	LocalBitmap *lbm = (LocalBitmap *)bm;
	MP12SetBitmapExtent(lbm->bits, (char *) lbm->bits + lbm->byteSize);
    }
    return bm;
}

static Bitmap *MP12NewFromData(LocalBMClass *class, Bounds *b, void *bits,
			       void *abits, int byteSize, int rowBytes,
			       int vram, int freeFlag)
{
    Bitmap *bm;

    bm = (Bitmap *) (*((LocalBMClass*)localBM)->newFromData)
		    (class, b, bits, abits, byteSize, rowBytes, vram,
		     freeFlag);
    bm->colorSpace = NX_ONEISBLACKCOLORSPACE;
    bm->bps = 2;
    bm->spp = 1;
    bm->vram = vram;
    bm->alpha = (abits != NULL);
    bm->planar = 1;
    bm->type = NX_OTHERBMTYPE;
    if (use_wf_hardware) {
	LocalBitmap *lbm = (LocalBitmap *)bm;
	MP12SetBitmapExtent(lbm->bits, (char *) lbm->bits + lbm->byteSize);
    }
    return bm;
}

static void MP12SizeBits(BMClass *class, Bounds *b, int *size, int *rowBytes)
{
    *rowBytes = (((b->maxx - b->minx - 1)>>4) + 1)*4;  /* bytes per scanline */
    *size = (*rowBytes)*(b->maxy-b->miny);
}

/* override newAlpha to perform cache disabling on allocated bits */
static void MP12NewAlpha(Bitmap *bm, int initialize)
{
    LocalBitmap *lbm = (LocalBitmap *)bm;

    if (bm->alpha || bm->vram) return;
    (*localBM->newAlpha)(bm, initialize);	/* `super' newAlpha */
    bm->spp = 2;
    if (use_wf_hardware)
	MP12SetBitmapExtent(lbm->abits, (char *) lbm->abits + lbm->byteSize);
}

void MP12SetupBits(LocalBitmap *ebm, Bounds *bounds, int leftShift, int
    topToBottom, int leftToRight, BitsOrPatInfo *dinfo, BitsOrPatInfo *ainfo,
    DevMarkInfo *info)
{   
    int n, offset;
    
    n = ebm->rowBytes / 4;
    leftShift = MP12scanmask & (bounds->minx - ebm->base.bounds.minx - leftShift);
    offset = ((bounds->miny - ebm->base.bounds.miny) * n) +
	((bounds->minx - ebm->base.bounds.minx - leftShift) >> MP12scanshift);
    if (!topToBottom) {
	offset += (bounds->maxy - bounds->miny - 1) * n;
	n = -n;
    }

    dinfo->type	         	= bitmapType;
    dinfo->data.bm.pointer	= ((uint *) ebm->bits) + offset;
    dinfo->data.bm.n		= n;
    dinfo->data.bm.leftShift = leftShift;
    if (ebm->abits) {
	ainfo->type		 = bitmapType;
	ainfo->data.bm.pointer	 = ((uint *) ebm->abits) + offset;
	ainfo->data.bm.n	 = n;
	ainfo->data.bm.leftShift = leftShift;
    }
}


void MP12SetupPat(Pattern *pat, DevPoint bmOffset, Bounds *bounds,
    int usealpha, int topToBottom, BitsOrPatInfo *info)
{
    int k, m;
    PatternData pData;
    DevMarkInfo grayInfo;
    NextGSExt extrec, *extp;
    
    grayInfo.halftone = (pat->halftone) ? pat->halftone : &mpDevHalftone;
    grayInfo.screenphase = pat->phase;
    grayInfo.offset.x = bmOffset.x; /* .y not used by SetupPattern */

    if (!usealpha && pat->alpha != OPAQUE) { /* to premult in MPSetupPattern */ 
	extrec.alpha = pat->alpha;
	extp = &extrec;
	grayInfo.priv = (PCard8)&extp;
    } else
	grayInfo.priv = NULL;
    /* invert alpha so 0 turns on all bits, as for gray */
    ((DevColorVal *)&grayInfo.color)->white = 
	usealpha ? ~pat->alpha : ((DevColorVal *)&pat->color)->white;
    SetupPattern(_mp12.base.bmPattern, &grayInfo, &pData);
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
	k = (bounds->minx - grayInfo.screenphase.x) >> MP12scanshift;
	m = pData.width;
	k = k - (k/m)*m; /* Broken-out implementation of % */
	if (k < 0)
	    k += m;
	info->data.pat.xInitial += k;
    }
}

void MP12Composite(Bitmap *bm, BMCompOp *bcop)
{
    DevPoint bmOffset;
    int lToR, tToB, op;
    int width, height, leftShift;
    int d_ofs, num_ints, num_lastbits;
    LocalBitmap *dbm = (LocalBitmap *)bm, *sbm;
    BitsOrPatInfo srcDI, srcAI, dstDI, dstAI, disAI, *s[3];

    bm_becomePSDevice(bm);
    /* Give the bcop->info our halftone if it doesn't have one already */
    if (!bcop->info.halftone) bcop->info.halftone = &mpDevHalftone;

    /* Now window coords plus offset yields bitmap coords. */
    bmOffset.x = bcop->info.offset.x - bm->bounds.minx;
    bmOffset.y = bcop->info.offset.y - bm->bounds.miny;

    height = bcop->srcBounds.maxy - bcop->srcBounds.miny;
    width = bcop->srcBounds.maxx - bcop->srcBounds.minx;

    d_ofs = (bcop->dstBounds.minx - bm->bounds.minx) & MP12scanmask;
    num_ints = width >> MP12scanshift;
    num_lastbits = width & MP12scanmask;

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
    MP12SetupBits(dbm, &bcop->dstBounds, leftShift, tToB, lToR, &dstDI, &dstAI,
	&bcop->info);

    if (bcop->srcType == BM_BITMAPSRC) {
	    /* Modify offset based on location of destination bitmap */
	    bcop->info.offset.x -= bcop->dstBounds.minx - (leftShift & 0xF);
	    bcop->info.offset.y -= bcop->dstBounds.miny;
	    MP12SetupBits(sbm, &bcop->srcBounds, leftShift, tToB,
			  lToR, &srcDI, &srcAI, &bcop->info);
	    bcop->info.offset.x += bcop->dstBounds.minx - (leftShift & 0xF);
	    bcop->info.offset.y += bcop->dstBounds.miny;

	    if (!sbm->abits)
		MP12SetupPat(blackpattern, bmOffset, &bcop->srcBounds, true,
			     tToB, &srcAI);
    } else if (bcop->srcType == BM_PATTERNSRC) { /* BM_PATTERNSRC */
	    MP12SetupPat(bcop->src.pat, bmOffset, &bcop->srcBounds, false,
			 tToB, &srcDI);
	    if (bcop->srcAS == A_BITS) {
		MP12SetupPat(bcop->src.pat, bmOffset, &bcop->srcBounds, true,
			     tToB, &srcAI);
	    }
    }

    /* Find final op, set up all (up to three) stages */
    if (bcop->op == DISSOLVE) {
	/* dissolve degree passed in bcop->alpha */
 	Pattern *pat = PNewColorAlpha(0, ~bcop->dissolveFactor,
				      bcop->info.halftone);
	MP12SetupPat(pat, bmOffset, &bcop->srcBounds, true, tToB, &disAI);
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

static void MP12ConvertFrom(Bitmap *d, Bitmap *s, Bounds *db,
			    Bounds *sb, DevPoint phase)
{
    LocalBitmap *sbm, *dbm;
    
    /* create alpha if necessary */
    if(s->alpha && !d->alpha)
	bm_newAlpha(d, false);
    sbm = (LocalBitmap *) s;
    dbm = (LocalBitmap *) d;
    switch(s->type) {
    case NX_OTHERBMTYPE:
	break;
    case NX_TWOBITGRAY:
    	MP12Convert2to2(sbm, dbm, *sb, *db);
	break;
    case NX_EIGHTBITGRAY:
	break;
    case NX_TWELVEBITRGB:
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
	    MP12Convert16to2((unsigned short *)sbm->bits, dbm->bits,
	        dbm->abits, size, srcOffset, sbm->rowBytes, dstOffset,
		dbm->rowBytes, phase);
	else
	    MP12Convert32to2(sbm->bits, dbm->bits, dbm->abits, size,
	    	srcOffset, sbm->rowBytes, dstOffset, dbm->rowBytes, phase);
    }
    break;
    }
}

static Bitmap *MP12MakePublic(Bitmap *bm, Bounds *hintBounds, int hintDepth)
{
    LocalBitmap *pbm;
    /* FIXME: For now, just use mp12 class and invert the data.
     * Later, should use the bm12 class before inverting the data.
     */
    pbm = (LocalBitmap *) bm_new(mp12, hintBounds, false);
    if (bm->alpha) bm_newAlpha((Bitmap *)pbm, false);
    MP12Convert2to2((LocalBitmap *) bm, pbm, *hintBounds, *hintBounds);
    pbm->base.type = NX_TWOBITGRAY;
    pbm->base.colorSpace = NX_ONEISWHITECOLORSPACE;
    return (Bitmap *)pbm;
}


/* This is a helper routine for MP12_Mark */
static void MP12MarkDataAndAlpha(MPMarkInfo *mpmi)
{
    if (mpmi->abits) {
	DevImage *image, *origimage;
	int items, origitems, ncolors;
	unsigned char *samplesZero, realWhite;
	DevPrivate *realPriv = mpmi->mrec->info.priv;

	/* Set alpha color, offset and framedevice for alpha bitmap */
	realWhite = ((DevColorVal *)&mpmi->mrec->info.color)->white;
 	((DevColorVal *)&mpmi->mrec->info.color)->white = 
		OPAQUE-ALPHAVALUE(PSGetGStateExt(NULL));
	mpmi->mrec->info.priv = NULL;
	framebase = mpmi->abits;
	switch (mpmi->itype) {
	case none:
	    Mark(mpmi->mrec->device, mpmi->mrec->graphic, mpmi->clip,
		&mpmi->mrec->info);
	    break;
	case regular: /* use image's trapezoids as opaque area */
	    origimage = image = mpmi->mrec->graphic->value.image;
	    origitems = mpmi->mrec->graphic->items;
	    mpmi->mrec->graphic->type = trapType;
	    for (items = origitems; items--; image++) {
		/* some waste using whole markBds in rare multi-item case */
		mpmi->mrec->graphic->items = image->info.trapcnt;
		mpmi->mrec->graphic->value.trap = image->info.trap;
		Mark(mpmi->mrec->device, mpmi->mrec->graphic, mpmi->clip,
		    &mpmi->mrec->info);
	    }
	    mpmi->mrec->graphic->type = imageType;
	    mpmi->mrec->graphic->items = origitems;
	    mpmi->mrec->graphic->value.image = origimage;
	    break;
	case alphaimage:
	    image = mpmi->mrec->graphic->value.image;
	    samplesZero = image->source->samples[0];
	    ncolors = image->source->nComponents;
	    
	    /* reuse and restore image data */
	    image->source->samples[0] = image->source->samples[ncolors];
	    image->source->nComponents = 1;
	    Mark(mpmi->mrec->device, mpmi->mrec->graphic, mpmi->clip,
		&mpmi->mrec->info);
	    image->source->samples[0] = samplesZero;
	    image->source->nComponents = ncolors;
	    break;
	}
	((DevColorVal *)&mpmi->mrec->info.color)->white = realWhite;
	mpmi->mrec->info.priv = realPriv;
    }

    framebase = mpmi->bits;
    Mark(mpmi->mrec->device, mpmi->mrec->graphic, mpmi->clip,
	&mpmi->mrec->info);
}

void MP12_Mark(Bitmap *bm, MarkRec *mrec, DevPrim *clip)
{
    MPMarkInfo mpmi;
    DevHalftone *mriht;
    DevImageSource *source;
    int ncolor, alphaC, origwbytes;
    DevPoint offset = mrec->info.offset;
    unsigned char *origSamples[5], *newSamples[5], *intSamples = NULL;

    if (!(mriht = mrec->info.halftone))
	mrec->info.halftone = &mpDevHalftone;
    framebytewidth = ((LocalBitmap *)bm)->rowBytes;
    mpmi.mrec  = mrec;
    mpmi.clip  = clip;
    mpmi.bits  = ((LocalBitmap *)bm)->bits;
    mpmi.abits = ((LocalBitmap *)bm)->abits;
    
    /* Characterize image type and correct premultiplication for alphaimage */
    if (mrec->graphic->type!=imageType||mrec->graphic->value.image->imagemask)
	mpmi.itype = none;
    else if (!mrec->graphic->value.image->unused)
	mpmi.itype = regular;
    else {
	int nchar, items;
	unsigned int alast, dlast, nlast, *dntrue;
	unsigned int *ap, *dp, *an, *dn, d, d2, mask, bps, oflow;

	mpmi.itype = alphaimage;
	items  = mrec->graphic->items;
	source = mrec->graphic->value.image->source;
	bps    = source->bitspersample;
        alphaC = source->nComponents;
	if (alphaC != 1 && alphaC != 3) PSRangeCheck();
	if (source->interleaved) {
	    int i,h;
	    unsigned char *ip;
	    unsigned int offset, shift, step, slot;
	    
	    /* Make image planar for premultiplication and marking
	     * Divide wbytes by number of components (2 or 4), rounding up
	     * to the nearest multiple of 4.
	     */
	    source->interleaved = false;
	    origwbytes = source->wbytes;
	    source->wbytes = ((((source->wbytes+3)/4)+alphaC)/(alphaC+1))*4;
	    nchar = source->height * source->wbytes;
	    intSamples = source->samples[IMG_INTERLEAVED_SAMPLES];
	    mask = (-1) << (32-bps);
	    step = bps * (alphaC+1);
	    for (ncolor = alphaC; ncolor>=0; ncolor--) {
		source->samples[ncolor]=newSamples[ncolor]=origSamples[ncolor]=
			(unsigned char *)malloc(nchar+4);
		ip = (unsigned char *)intSamples;
		dn = (unsigned int *)origSamples[ncolor];
		offset = bps * ncolor;
		for (h = 0; h < source->height; h++) {
		    d2 = slot = 0;
		    for (i = origwbytes; i > 0; i -= 4) {
		        if (i < 4) {
			    d = *ip++<<24;
			    if (i>1) d |= *ip++<<16;
			    if (i>2) d |= *ip++<<8;
			}
			else {d=*ip++<<24;d|=*ip++<<16; d|=*ip++<<8; d|=*ip++;}
			for (shift=offset; shift<32; shift+=step, slot+=bps)
			    d2 |= ((d<<shift)&mask)>>slot;
			if (slot == 32) { *dn++ = d2; d2 = slot = 0; }
		    }
		    if (slot) *dn++ = d2;
		}
	    }
	} else {
	    nchar = source->height * source->wbytes;
	    for (ncolor = alphaC; ncolor>=0; ncolor--) {
		origSamples[ncolor] = source->samples[ncolor];
		newSamples[ncolor] = (unsigned char *)malloc(nchar);
		source->samples[ncolor] = newSamples[ncolor];
	    }
	}
	/* alphaimage with non-interleaved data
	 * convert premultiplied data from externally premult toward 0
	 * to premult toward 1 (white) with the equation: 
	 * d1 = d0 + (1-alpha)   (saturating addition)
	 * We can add 32 bits at a time if we allow space to catch and
	 * correct overflow.  So we operate on 2 set of alternate
	 * pixels, 1 at a time leaving a 0 pixel above each real one.
	 */ 
	mask = ~(-1<<bps); /* mask for last pixel */
	for (d = bps*2; d < 32; d *= 2)
	    mask |= mask << d; /* alternate pixels, e.g. 0x00ff00ff */
	for (ncolor = alphaC-1; ncolor>=0; ncolor--) {
	    /* count on unaligned extraction of 4 bytes at a time */
	    dp = (unsigned int *)(origSamples[ncolor]);
	    ap = (unsigned int *)(origSamples[alphaC]);
	    dn = (unsigned int *)(newSamples[ncolor]);
	    for (nchar = source->height * source->wbytes;
		nchar > 0;
		nchar -= sizeof(int))
	    {
		if (nchar < sizeof(int)) {
		    /* Do last few bytes carefully with side storage */
		    bcopy(ap, &alast, nchar);
		    ap = &alast;	    
		    bcopy(dp, &dlast, nchar);
		    dp = &dlast; dntrue = dn; dn = &nlast;	    
		}
		/* shift 1st set of alternate pixels */
		d = ((*dp >> bps) & mask) +
		    (mask - ((*ap >> bps) & mask));
		if (oflow = d & ~mask) {
		    /* 1 or more zero-pixels not zero! */
		    for ( ; bps>0; bps--)
			d |= oflow >> bps;
		    /* reload the bps */
		    bps = source->bitspersample;
		    d &= mask; /* remove overflow bits */
		}
		d2 = d << bps;
		/* don't shift the 2nd set of alternate pixels */
		d = (*dp & mask) + (mask - (*ap & mask));
		if (oflow = d & ~mask) {
		    /* 1 or more zero-pixels not zero! */
		    for ( ; bps>0; bps--)
			d |= oflow >> bps;
		    /* reload the bps */
		    bps = source->bitspersample;
		    d &= mask; /* remove overflow bits */
		}
		*dn = d | d2;
		if (nchar < sizeof(int))   /* copy side store to real bytes */
		    bcopy(&nlast, dntrue, nchar);	    
		ap++; dp++; dn++;
	    }
	}
	/* Invert alpha component */
	ap = (unsigned int *)(origSamples[alphaC]);
	dn = (unsigned int *)(newSamples[alphaC]);
	for (nchar = source->height * source->wbytes;
	     nchar > 0;
	     nchar -= sizeof(int))
	{
	    if (nchar<sizeof(int)) {
		/* Carefully, side storage */
		bcopy(ap, &alast, nchar);
		ap = &alast; dntrue = dn; dn = &nlast;	    
	    }
	    *dn++ = ~(*ap++); /* so 0 turns on bits as does for data */
	    if (nchar<sizeof(int)) /* copy to real bytes */
		bcopy(&nlast, dntrue, nchar);	    
	}
    }
    MP12MarkDataAndAlpha(&mpmi);
    /* Restore for other devices and clean up */
    if (!mriht) mrec->info.halftone = NULL;
    if (mpmi.itype == alphaimage) {
        /* Free malloc'ed samples and restore source image to original state */
	for (ncolor = alphaC; ncolor >= 0; ncolor--)
	    free(newSamples[ncolor]);
        if (intSamples) {
	    source->samples[IMG_INTERLEAVED_SAMPLES] = intSamples;
	    source->interleaved = true;
	    source->wbytes = origwbytes;
	} else
	    for (ncolor = alphaC; ncolor >= 0; ncolor--)
		source->samples[ncolor] = origSamples[ncolor];
    }
}


static void MP12SetupPattern(PatternHandle h, DevMarkInfo *markInfo,
			     PatternData *data)
{
    PNextGSExt ep;
    DevColorVal *miColor = NULL;
    unsigned int orig, gray;

    miColor = (DevColorVal *)&markInfo->color;
    gray = orig = miColor->white;
    if (markInfo->priv
      && (ep = *(PNextGSExt *)markInfo->priv) && (ep->alpha != OPAQUE)) {
	/* A gs extension exists so may have to premultiply */
	DebugAssert(gray <= ep->alpha); /* so next expr can't overflow */
	gray += OPAQUE - ep->alpha;
	miColor->white = gray;
    }
    if (gray != 0 && gray != 85 && gray != 170 && gray != 255)
	Gry4Of4Setup(h, markInfo, data);
    else {
	gray |= gray << 8;
	gray |= gray << 16;
	data->value = ~gray;
	data->start = &data->value;
	data->end = &data->value + 1;
	data->constant = true;
	data->width = 1;
    }
    miColor->white = orig;
}

/* Note: this code assumes that it only executes once (i.e. don't subclass
 * mp12 without changing this code!)
 */
static void MP12InitClassVars(BMClass *class)
{
    PMarkProcs mp;
    ImageArgs *ia;
    DevColorData devColorData;
    LocalBMClass *localclass = (LocalBMClass *)class;

    /* initialize superclass */
    (*localBM->_initClassVars)(class);

    /* override appropriate superclass method */
    /* bitmap overrides */
    class->sizeBits = MP12SizeBits;
    class->composite = MP12Composite;
    class->makePublic = MP12MakePublic;
    class->convertFrom = MP12ConvertFrom;
    class->newAlpha = MP12NewAlpha;

    /* override local bitmap methods */
    mpDevScreen.width = mpDevScreen.height = 8;
    mpDevScreen.thresholds = (unsigned char *)&mpThresholds[0];
    mpDevHalftone.red = mpDevHalftone.green = mpDevHalftone.blue =
			mpDevHalftone.white = &mpDevScreen;
    /* protect static halftones from the grim reaper */
    DevAddHalftoneRef(&mpDevHalftone);
    localclass->newFromData = MP12NewFromData;
    localclass->_mark = MP12_Mark;
    localclass->setupPattern = MP12SetupPattern;
    localclass->bmMarkProcs = *wdMarkProcs; 
    localclass->log2bd = MP12LOG2BD; 
    localclass->defaultHalftone = &mpDevHalftone; 
    mp = &localclass->bmMarkProcs;  
    mp->SetupImageArgs = localclass->setupImageArgs;
    
    ia = &localclass->bmImArgs;
    ia->bitsPerPixel = MP12BPP;
    ia->red.first = ia->green.first = ia->blue.first = 0;
    ia->red.n = ia->green.n = ia->blue.n = 0;
    ia->gray.n = MP12GRAYS;
    ia->gray.first = 0;
    ia->gray.delta = 1;
    
    devColorData.first = 0;
    devColorData.n = MP12GRAYS;
    devColorData.delta = 1;
    localclass->bmPattern = GryPat4Of4(devColorData, MP12BPP);
    localclass->grayPattern = NXGrayPat(3, MP12Grays, 0xffffffff);
    localclass->bmPattern->setupPattern = localclass->setupPattern;
    
    ia->pattern = localclass->bmPattern;
    ia->procs = fmImageProcs;
    ia->data = NIL;
    MRInitialize();	/* initialize moverect */
}

MP12Class _mp12 = {
    {
	/* LocalBMClass */
	{
	    /* BMClass */
	    0,
	    MP12InitClassVars,
	    MP12New,
	},
	MP12NewFromData,
    },
};

