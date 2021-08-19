/*****************************************************************************

    windowgraphics.c

    This file contains routines for managing graphic operations on windows
    within the PostScript implementation.
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.
    
    Created 31Jul86 Leo
    
    Modified:
    
    17Nov87 Jack  add sGS global for new Pattern workings
    20Jan88 Jack  trim log, remove NewGrayOrigin, must be at bitmap level
    24Mar88 Leo	  Just pass the alpha value in, let bintree cause error
    28Mar88 Leo	  New CompositePriv w/special rectangular case
    30Mar88 Leo	  Special case matrices in CompositePriv
    21Apr88 Jack  INCOMPATIBLE VERSION changes in WindowAlpha, etc.
    25Apr88 Jack  generalize PSReadImage for framebitdepth 
    27Apr88 Jack  add defaultHalftone in SetExposureColor
    24May88 Jack  eliminate gray argument to PSCompositeRect
    01Jun88 Jack  remove defaultHalftone and SetExposureColor
    09Jun88 Jack  avoid o'flow w/ return test in CompositePriv's clip compare
    10Jun88 Jack  must have that defaultHalftone in CompositePriv
    07Sep88 Jack  remove scanType baggage
    14Jan89 Leo	  Re-enabled Jim Sandman's local RealEq1 defs
    18Jan89 Jack  Move PSAlphaImage here from image.c
    27Jan89 Jack  remove initwindowalpha from API
    16Feb89 Jack  convert PSReadImage to premultiply data toward black
    02Mar89 Jack  slightly larger malloc of expandedLines
    20Mar89 Jack  fix CompositeTraps to get correct values for last scanline
    14Jun89 Jack  CheckWindow in PSAlphaImage, TfmPCd replaces UserToDevice
    16Jun89 Leo	  Restore contents of *cti when exiting CompositeRun or
		  CompositeTrap; and, in CompositePriv, in hard case where
		  you are taking apart clip, use inner loop var thisGraphic
    02Nov89 Ted	  Various mods for color and compositing.
    16Oct89 Terry CustomOps conversion
    15Nov89 Ted	  ANSI C Prototyping, formatting, ONEMINUSALPHA -> alpha
    29Nov89 Terry IBM compatibility modifications
    04Dec89 Ted   Integratathon!
    13Dec89 Ted   Sorted routines, cleanup
    21Dec89 Ted   Removed "nextgs.h" import, defines moved to windowdevice.h
    08Jan90 Ted   Modified LCopyBitsFrom to do both data AND alpha at once.
    22Jan90 Terry Allow interleaved case in PSAlphaImage to pass through
    31Jan90 Terry Modified LSetExposureColor to not use defaultHalftone
    23Feb90 Terry Integrated Leo's fillwindow operator
    04May90 Ted   Fixed bug 4175: store greater alpha precision in gstate
    29May90 Terry Added #ifdef IEEEFLOAT fast floating point case
    29May90 Terry Reduced code size from 5064 to 4944 bytes
    04Jun90 Ted   Removed NEWBITS/FREEBITS macros; replaced with real code.
    04Jun90 Jack  Fixed bug 5889, use local values of ix in CompositeRect
    05Jun90 Jack  Fixed bug 6017, x versus y typo in CompositePriv
    24Jun90 Ted   Ripped out *accuracy operators for new API
    24Jun90 Ted   Finally removed old PSInitWindowAlpha() from code (27Jan89)
    
******************************************************************************/

#import PACKAGE_SPECS
#import CUSTOMOPS
#import EXCEPT
#import FP
#import EVENT
#import BINTREE
#import WINDOWDEVICE
#import "framedev.h"
#import "ipcscheduler.h"
#import "bitmap.h"

extern PWindowDevice windowBase;	/* Root window */
extern int lastWID;			/* Last window ID created */
extern DevPrim *GetDevClipPrim();
extern PWindowDevice ID2Wd();
extern Layer *ID2Layer();

/* Forward declarations */
private procedure CompositeTraps();

/* Doing (int)MyCeil(f) is a cheapo way of getting the ceil function */
#define MyCeil(f) ((f)+(float)0.999999)

#if SWAPBITS
static const uchar reverse_pixels_in_byte[256] = {
  0x00,0x40,0x80,0xc0,0x10,0x50,0x90,0xd0,0x20,0x60,0xa0,0xe0,0x30,0x70,0xb0,
  0xf0,0x04,0x44,0x84,0xc4,0x14,0x54,0x94,0xd4,0x24,0x64,0xa4,0xe4,0x34,0x74,
  0xb4,0xf4,0x08,0x48,0x88,0xc8,0x18,0x58,0x98,0xd8,0x28,0x68,0xa8,0xe8,0x38,
  0x78,0xb8,0xf8,0x0c,0x4c,0x8c,0xcc,0x1c,0x5c,0x9c,0xdc,0x2c,0x6c,0xac,0xec,
  0x3c,0x7c,0xbc,0xfc,0x01,0x41,0x81,0xc1,0x11,0x51,0x91,0xd1,0x21,0x61,0xa1,
  0xe1,0x31,0x71,0xb1,0xf1,0x05,0x45,0x85,0xc5,0x15,0x55,0x95,0xd5,0x25,0x65,
  0xa5,0xe5,0x35,0x75,0xb5,0xf5,0x09,0x49,0x89,0xc9,0x19,0x59,0x99,0xd9,0x29,
  0x69,0xa9,0xe9,0x39,0x79,0xb9,0xf9,0x0d,0x4d,0x8d,0xcd,0x1d,0x5d,0x9d,0xdd,
  0x2d,0x6d,0xad,0xed,0x3d,0x7d,0xbd,0xfd,0x02,0x42,0x82,0xc2,0x12,0x52,0x92,
  0xd2,0x22,0x62,0xa2,0xe2,0x32,0x72,0xb2,0xf2,0x06,0x46,0x86,0xc6,0x16,0x56,
  0x96,0xd6,0x26,0x66,0xa6,0xe6,0x36,0x76,0xb6,0xf6,0x0a,0x4a,0x8a,0xca,0x1a,
  0x5a,0x9a,0xda,0x2a,0x6a,0xaa,0xea,0x3a,0x7a,0xba,0xfa,0x0e,0x4e,0x8e,0xce,
  0x1e,0x5e,0x9e,0xde,0x2e,0x6e,0xae,0xee,0x3e,0x7e,0xbe,0xfe,0x03,0x43,0x83,
  0xc3,0x13,0x53,0x93,0xd3,0x23,0x63,0xa3,0xe3,0x33,0x73,0xb3,0xf3,0x07,0x47,
  0x87,0xc7,0x17,0x57,0x97,0xd7,0x27,0x67,0xa7,0xe7,0x37,0x77,0xb7,0xf7,0x0b,
  0x4b,0x8b,0xcb,0x1b,0x5b,0x9b,0xdb,0x2b,0x6b,0xab,0xeb,0x3b,0x7b,0xbb,0xfb,
  0x0f,0x4f,0x8f,0xcf,0x1f,0x5f,0x9f,0xdf,0x2f,0x6f,0xaf,0xef,0x3f,0x7f,0xbf,
  0xff
};
#endif

private procedure CompositeRun(DevRun *run, CompositeInfo *cti)
{
    register int y, pairs, lines;
    register DevShort *buffptr;
    register int xoffset = cti->offset.x;
    register int yoffset = cti->offset.y;
    Bounds bounds;
    
    y = run->bounds.y.l;
    buffptr = run->data;
    lines = run->bounds.y.g - run->bounds.y.l;
    while (--lines >= 0) {
	pairs = *(buffptr++);
	while (--pairs >= 0) {
	    cti->offset.x = (bounds.minx = *(buffptr++)) - xoffset;
	    bounds.maxx = *(buffptr++);
	    cti->offset.y = y - yoffset;
	    bounds.maxy = (bounds.miny = y) + 1;
	    LCompositeFrom(cti, bounds);
	}
	y++;
    }
    /* Leo 16Jun89 restore offsets */
    cti->offset.x = xoffset;
    cti->offset.y = yoffset;
}

#if IEEEFLOAT
#define RtoI(r) (*(integer *)(&(r)))
#else
static int CheckUprightMtx(PMtx m)
{
    if (m->b==0. && m->c==0. && m->a==1.)
	return (m->d==1. ? 1 : m->d==-1. ? -1 : 0);
    return 0;
}
#endif

private int CompositePriv(Cd sourcePt, Cd userSize, PPSGState sourceGS,
    Cd destPt, int op, unsigned char alpha)
{
    unsigned char thresh;
    DevScreen scr;
    CompositeInfo cti;
    PPSGState sGS, dGS;
    PDevice sDevice;
    DevPrim *clip;
    PMtx sMtx, dMtx;
    DevMarkInfo mi;

    sGS = sourceGS;
    dGS = NULL;
    CheckWindow();
    sDevice = PSGetDevice(sGS);
    if (sDevice->procs != wdProcs) PSInvalidID();

    /* Initialize markInfo if it is needed in the composite call */
    if (op != HIGHLIGHT) {
	PSGetMarkInfo(NULL, &mi);
	cti.markInfo.halftone = mi.halftone; /* halftone could be NULL */
	cti.markInfo.screenphase = mi.screenphase;
	cti.markInfo.color = mi.color;
	if(!sourceGS) { /* compositerect */
	    cti.markInfo.offset.x = cti.markInfo.offset.y = 0;
	    cti.alpha = ALPHAVALUE(mi.priv);
	}
	else {
	    cti.markInfo.offset = mi.offset;
	    cti.alpha = alpha; /* only matters if op == DISSOLVE */
	}
    }
    cti.op = op;
    cti.dwin = Wd2Layer(PSGetDevice(NULL));
    cti.swin = sourceGS ? Wd2Layer(sDevice) : NULL;
    sMtx = PSGetMatrix(sGS);
    dMtx = PSGetMatrix(dGS);
    if (PSGetClip(&clip)) {	/* Is the current clip path a rectangle? */
	int f1, tmp, dstMtxState, srcMtxState;
	
#if IEEEFLOAT
	f1=0x3f800000;
	dstMtxState = RtoI(dMtx->d) ^ f1;
	srcMtxState = RtoI(sMtx->d) ^ f1;
        tmp = RtoI(dMtx->b) | RtoI(dMtx->c) | RtoI(sMtx->b) | RtoI(sMtx->c)
	      | dstMtxState | srcMtxState;
	tmp += tmp;
	if (!(tmp |= (RtoI(dMtx->a)^f1) | (RtoI(sMtx->a)^f1)))
#else
	if (!(dstMtxState = CheckUprightMtx(dMtx)) ||
	    !(srcMtxState = CheckUprightMtx(sMtx)))
#endif
	{
	    /* matrices unscaled (maybe with flipped y) do this on the cheap */
	    register float devmin, devmax; /* source device coords */
	    register int imin, imax, trans; /* from source to dest */
	    Bounds dstBounds;
	    
	    /* push destPt through its transform matrix */
	    destPt.x += dMtx->tx;
	    if (dstMtxState >= 0)
		destPt.y += dMtx->ty;
	    else
		destPt.y = dMtx->ty - destPt.y;
	    /* Do x coords of source & dstBounds */
	    /* Clip destination box to clip.bounds.  Return if no overlap */
	    /* Set offset to minimum device x & y of clipped source */
	    if (userSize.x > 0.0)
		devmax = (devmin = (sourcePt.x += sMtx->tx)) + userSize.x;
	    else
		devmin = (devmax = (sourcePt.x += sMtx->tx)) + userSize.x;
	    trans = (int)destPt.x - (int)sourcePt.x;
	    imin = (int)devmin + trans;
	    imax = (int)MyCeil(devmax) + trans;
	    if (imin < clip->bounds.x.l) {
		if (imax <= clip->bounds.x.l)
  		    return;
		dstBounds.minx = clip->bounds.x.l;
  	    } else
		dstBounds.minx = imin;
	    if (imax > clip->bounds.x.g) {
		if (imin >= clip->bounds.x.g)
  		    return;
		dstBounds.maxx = clip->bounds.x.g;
  	    } else
		dstBounds.maxx = imax;
	    cti.offset.x = dstBounds.minx - trans;

	    /* Do the same for y coords, reusing some variables */
	    if (srcMtxState >= 0)
		sourcePt.y += sMtx->ty;
	    else {
		sourcePt.y = sMtx->ty - sourcePt.y;
		userSize.y = -userSize.y;
	    }
	    if (userSize.y > 0.0)
		devmax = (devmin = sourcePt.y) + userSize.y;
	    else
		devmin = (devmax = sourcePt.y) + userSize.y;
	    trans = (int)destPt.y - (int)sourcePt.y;
	    imin = (int)devmin + trans;
	    imax = (int)MyCeil(devmax) + trans;
	    if (imin < clip->bounds.y.l) {
		if (imax <= clip->bounds.y.l)
  		    return;
		dstBounds.miny = clip->bounds.y.l;
  	    } else
		dstBounds.miny = imin;
	    if (imax > clip->bounds.y.g) {
		if (imin >= clip->bounds.y.g)
  		    return;
		dstBounds.maxy = clip->bounds.y.g;
  	    } else
		dstBounds.maxy = imax;
	    cti.offset.y = dstBounds.miny - trans;
	    
	    LCompositeFrom(&cti, dstBounds);
	    return;
	} /* if (matrices are just right) */
    } /* if (clip is rectangle) */
    
    /* Okay, we'll do it the hard way! */
    {
	register int items, clipItems, dx, dy;
	DevPrim *trapDP;
	boolean rect;
	DevTrap traps[7];
	register DevTrap *trap;
	register DevRun *run;
	DevPrim dp, *graphic;
    
	/* get offset from src to dest (all in window coords) */
	if (sourceGS) {		/* bounds in source window */
	    Cd devSrc, devDst;

	    TfmPCd(sourcePt, sMtx, &devSrc);
	    TfmPCd(destPt, dMtx, &devDst);
	    dx = cti.offset.x = (short)devDst.x - (short)devSrc.x;
	    dy = cti.offset.y = (short)devDst.y - (short)devSrc.y;
	    if (op == DISSOLVE)
		cti.alpha = alpha;
	} else {   /* virtual source, bounds in destination window */
	    sourcePt = destPt;
	    dx = dy = cti.offset.x = cti.offset.y = 0;
	}
	dp.type = trapType;
	dp.next = NULL;
	dp.items = 0;
	dp.maxItems = 7;
	dp.value.trap = traps;
	rect = PSReduceRect(sourcePt.x,sourcePt.y, userSize.x, userSize.y,
	    sMtx, &dp);
	if (dy != 0) {	/* shift to dest */
	    trap = &traps[0];
	    for (items = dp.items; items--; trap++) {
		trap->y.l += dy;
		trap->y.g += dy;
	    }
	    dp.bounds.y.l += dy;
	    dp.bounds.y.g += dy;
	}
	if (dx != 0) {	/* shift to dest */
	    Fixed fdx = Fix(dx);
	    trap = &traps[0];
	    for (items = dp.items; items--; trap++) {
		trap->l.xl += dx;
		trap->l.xg += dx;
		trap->l.ix += fdx;
		trap->g.xl += dx;
		trap->g.xg += dx;
		trap->g.ix += fdx;
	    }
	    dp.bounds.x.l += dx;
	    dp.bounds.x.g += dx;
	}
	graphic = &dp;
	if (PSGetClip(&clip) &&   /* True if clip is a rectangle */
	(DevBoundsCompare(&dp.bounds, &clip->bounds) == inside))
	    clip = NULL;
	else if (rect && (DevBoundsCompare(&clip->bounds, &dp.bounds)
	== inside)) {
	    graphic = clip;
	    clip = NULL;
	}
	
	if (!clip) /* graphic could be our traps or clip's (traps or run) */
	    for ( ; graphic; graphic = graphic->next)
	    switch (graphic->type) {
		case trapType:
		    CompositeTraps(graphic->value.trap, graphic->items, &cti);
		    break;
		case runType:
		    run = graphic->value.run;
		    for (items = graphic->items; items--; run++)
			CompositeRun (run, &cti);
		    break;
		case noneType:
		    break;
		default:
		    CantHappen();
		    break;
	    }
	else /* graphic must be our traps but clipper could be traps or run */
	for ( ; clip; clip = clip->next) {
	    DevPrim *thisGraphic;
	    
	    switch (clip->type) {
		case noneType:
		    break;
		case trapType:
		    for (thisGraphic=graphic; thisGraphic;
		    thisGraphic = thisGraphic->next) {
			if (!OverlapDevBounds(&clip->bounds,
			&thisGraphic->bounds))
			    continue;
			clipItems = clip->items;
			trap = clip->value.trap;
			switch (thisGraphic->type) {
			    case trapType:
				TrapTrapDispatch(trap, clipItems,
				    thisGraphic->value.trap,
				    thisGraphic->items,
				    &thisGraphic->bounds,
				    CompositeTraps,
				    &cti,
				    NULL);
				break;
			    case noneType:
				break;
			    default:
				CantHappen();
				break;
			}
		    }
		    break;
		case runType:
		    if (!OverlapDevBounds(&clip->bounds, &graphic->bounds))
			continue;
		    clipItems = clip->items;
		    run = clip->value.run;
		    switch (graphic->type) {
			case trapType:
			    for (; clipItems--; run++)
			    ClipRunTrapsDispatch(graphic->value.trap,
				graphic->items,
				&graphic->bounds, run, CompositeRun,
				CompositeTraps, &cti, NULL);
			    break;
			case noneType:
			    break;
			default:
			    CantHappen();
			    break;
		    }
		    break;
		default:
		    CantHappen();
		    break;
	    }
	} /* of variable block */
    } /* of variable block */
}

private procedure CompositeTraps(register DevTrap *tt, int items,
    CompositeInfo *cti)
{
    register int xl, xr, y, lines;
    int xoffset = cti->offset.x;
    int yoffset = cti->offset.y;
    Fixed lx, ldx, rx, rdx;
    register boolean leftSlope, rightSlope, dumpOld = false;
    Bounds bounds;
    
    while (--items >= 0) {
	bounds.miny = y = tt->y.l;
	lines = tt->y.g - y;
	if (lines <= 0) goto nextTrap;
	bounds.minx = xl = tt->l.xl;
	bounds.maxx = xr = tt->g.xl;
	if (lines == 1) goto rectangle;
	leftSlope = (tt->l.xg != xl);
	rightSlope = (tt->g.xg != xr);
	if (leftSlope) {
	    lx = tt->l.ix;
	    ldx = tt->l.dx;
	} else if (!rightSlope) goto rectangle;
	if (rightSlope) {
	    rx = tt->g.ix;
	    rdx = tt->g.dx;
	}
	while (true) {
	    if ((--lines <= 0) || dumpOld) {
dump:
		cti->offset.x = bounds.minx - xoffset;
		cti->offset.y = bounds.miny - yoffset;
		bounds.maxy = (dumpOld) ? y : y+1;
		LCompositeFrom(cti, bounds);
		bounds.miny = y;
		bounds.minx = xl;
		bounds.maxx = xr;
		if (!dumpOld) goto nextTrap;
		dumpOld = false;
		if (lines <= 0) goto dump;
	    }
	    if (leftSlope) {
		if (lines == 1)
		    xl = tt->l.xg;
		else {
		    xl = IntPart(lx);
		    lx += ldx;
		}
		if (xl != bounds.minx) dumpOld = true;
	    }
	    if (rightSlope) {
		if (lines == 1)
		    xr = tt->g.xg;
		else {
		    xr = IntPart(rx);
		    rx += rdx;
		}
		if (xr != bounds.maxx) dumpOld = true;
	    }
	    y++;
	}
rectangle:
	cti->offset.x = xl - xoffset;
	cti->offset.y = y - yoffset;
	bounds.maxy = y + lines;
	LCompositeFrom(cti, bounds);
nextTrap:
	tt++;
    }
    /* Leo 16Jun89 restore offsets */
    cti->offset.x = xoffset;
    cti->offset.y = yoffset;
}

public int PopNaturalMax(int max)
{
    int val = PSPopInteger();
    if (val<0 || val>max)
	PSRangeCheck();
    return val;
}

private procedure PSAlphaImage()
{
    int nProcs, colorSpace, nColors = PSPopInteger();
    boolean multiproc = PSPopBoolean();

    CheckWindow();
    switch (nColors) {
	case 1: colorSpace = DEVGRAY_COLOR_SPACE; break;
	case 3: colorSpace = DEVRGB_COLOR_SPACE; break;
	case 4: colorSpace = DEVCMYK_COLOR_SPACE; break;
	default: PSRangeCheck(); break;
    }
    nProcs = (multiproc) ? nColors+1 : 1;
    DoImage(nColors, colorSpace, nProcs, false, true);
}

private procedure PSComposite()
{
    PPSGState sourceGS;
    Cd sourcePt, userSize, destPt;
    int op;

    op = PopNaturalMax(PLUSL);
    if (op == HIGHLIGHT)
	PSRangeCheck();
    PSPopPCd(&destPt);
    sourceGS = PSPopGState();
    PSPopPCd(&userSize);
    PSPopPCd(&sourcePt);
    CompositePriv(sourcePt, userSize, sourceGS, destPt, op, 0);
}

private procedure PSCompositeRect()
{
    int op;
    Cd sourcePt, userSize;

    CheckWindow();
    op = PopNaturalMax(PLUSL);
    PSPopPCd(&userSize);
    PSPopPCd(&sourcePt);
    CompositePriv(sourcePt, userSize, NULL, sourcePt, op, 0);
}

private procedure PSCurrentAlpha()
{
    PNextGSExt ep;

    ep = *((PNextGSExt *)PSGetGStateExt(NULL));
    PSPushPReal(&(ep->realalpha));
}

private procedure PSCurrentWindowAlpha()
{
    /* In ps-36 we changed the internal representation of alphaState to be
     * 1 bit (instead of 2 bits).  LCurrentAlphaState returns
     * 0 when alpha is present, and 1 when alpha is not present.
     * Our public API (what we push onto the stack here) is to return
     * 0 if alpha is present, and 2 if alpha is not present.
     * Therefore we shift our result left 1 to convert to our public API.
     */
    PSPushInteger(LCurrentAlphaState(ID2Layer(PSPopInteger()))<<1);
}

private procedure PSDissolve()
{
    int delta;
    PPSGState sourceGS;
    real val, largeVal;
    Cd sourcePt, userSize, destPt;
    
    PSPopPReal(&val);
    largeVal = (real) (OPAQUE * val);
    RRoundP(&largeVal, &largeVal);
    delta = (int) largeVal;
    if (delta < 0) delta = 0;
    else if (delta > OPAQUE) delta = OPAQUE;
    PSPopPCd(&destPt);
    sourceGS = PSPopGState();
    PSPopPCd(&userSize);
    PSPopPCd(&sourcePt);
    CompositePriv(sourcePt, userSize, sourceGS, destPt, DISSOLVE, delta);
}

private procedure PSFillWindow()
{
    integer	otherNum,op;
    Layer	*win,*otherWin;
    WindowDevice *wd;

    wd = ID2Wd(PSPopInteger());
    win = wd->layer;
    otherNum = PSPopInteger();
    op = PSPopInteger();
    if (op < -1 || op > 1) PSRangeCheck();
    if (!wd->exists) PSInvalidID();
    otherWin = (op && otherNum) ? (Layer *)ID2Layer(otherNum) : NULL;
    LFill(win, op, otherWin);
}

private procedure PSFlushGraphics()
{
    CheckWindow();
    LFlushBits(Wd2Layer(PSGetDevice(NULL)));
}

private procedure PSHideInstance()
{
    Bounds hideBounds;

    CheckWindow();
    PopBounds(PSGetMatrix(NULL), &hideBounds);
    LHideInstance(Wd2Layer(PSGetDevice(NULL)), hideBounds);
}

private procedure PSNewInstance()
{
    CheckWindow();
    LNewInstance(Wd2Layer(PSGetDevice(NULL)));
}

private procedure PSReadImage()
{
    LocalBitmap *lbm;
    Bounds bounds, winBounds;
    PSObject proc[4], aProc, dProc, s;
    unsigned char *sp, *ap, *dp;
    int c, numProcs, wchars, h, dh, line, i;
    Layer *layer = Wd2Layer(PSGetDevice(NULL));
    boolean readAlpha;
    
    /* Get args and compute window-relative bounds of readimage area */
    CheckWindow();
    readAlpha = PSPopBoolean();
    PSPopTempObject(dpsStrObj,&s);
    for(numProcs = 0; numProcs < 4 ; numProcs++) {
	if(PSGetOperandType() != dpsArrayObj) break;
	PSPopTempObject(dpsArrayObj, &proc[numProcs]);
    }

    PopBounds(PSGetMatrix(NULL), &bounds); /* window */
    GetWinBounds(layer, &winBounds); /* screen */
    OFFSETBOUNDS(winBounds, -winBounds.minx, -winBounds.miny);
    sectBounds(&bounds, &winBounds, &bounds);
    h = bounds.maxy - bounds.miny;
    if (h <= 0 || (bounds.maxx - bounds.minx) <= 0) return;

    /* Copy the bits in from the layer within the given bounds */
    if (!(lbm = LCopyBitsFrom(layer, bounds, readAlpha)))
	return;
    switch(lbm->base.type) {
  	case NX_TWOBITGRAY:
	    /* Check that numProcs is correct, assign them to data and alpha */
	    if (readAlpha) {
		if (numProcs != 2) PSTypeCheck();
		aProc = proc[0];
		dProc = proc[1];
	    } else {
		if (numProcs != 1) PSTypeCheck();
		dProc = proc[0];
	    }
    
	    /* Make sure string is long enough for single scanline */
	    wchars = (((bounds.maxx - bounds.minx) * 2) + 7) / 8;
	    if (!(dh = s.length / wchars)) PSLimitCheck();
    
	    /* Since LCopyBitsFrom copies lines aligned to 32-bit boundaries,
	     * and PostScript returns images aligned to 8-bit boundaries,
	     * we must drop out characters at the ends of lines to align
	     * it to 8-bit boundaries in the final string. 
	     */
	    sp = (unsigned char *) lbm->bits;
	    ap = (unsigned char *) lbm->abits;
	    for (; h > 0; h -= dh) {
		if (dh > h) dh = h;
		s.length = wchars*dh;
    
		/* Dropout out extra chars at line ends */
		dp = (string)s.val.strval;
		for (line = dh; line>0; line--) {
		    for (c = 0; c<wchars; c++) {
#if SWAPBITS
			*(dp + c) = reverse_pixels_in_byte[(uint)*(sp+c)];
#else
			*(dp + c) = *(sp + c);
#endif
		    }
		    sp += lbm->rowBytes;
		    dp += wchars;
		}
		PSPushObject(&s);
		if (PSExecuteObject(&dProc)) goto Error;

		if (readAlpha) {
		    /* If no stored alpha, set to opaque, else copy it in */
		    if (lbm->abits) {
#if SWAPBITS
			/* Reverse all pixels in each byte */
			dp = s.val.strval;
			for (line = dh; line>0; line--) {
			    for (c = 0; c<wchars; c++)
				*(dp+c) =reverse_pixels_in_byte[(uint)*(ap+c)];
			    ap += lbm->rowBytes;
			    dp += wchars;
			}
#else
			/* Dropout extra chars at end of lines */
			dp = s.val.strval;
			for (line = dh; line>0; line--) {
			    bcopy(ap, dp, wchars);
			    ap += lbm->rowBytes;
			    dp += wchars;
			}
#endif
		    }
		    else
		        memset(s.val.strval, 0xff, s.length);
		    PSPushObject(&s);
		    if (PSExecuteObject(&aProc)) goto Error;
		}
	    }
	    break;

	case NX_TWENTYFOURBITRGB:
	{
	    int w, k, width, skip;
	    /* Check that numProcs is right, assign them to data and alpha */
	    if(numProcs != 1) PSTypeCheck();
	    dProc = proc[0];
    
	    /* If we want alpha, wchars is 4 bytes per pixel, else 3 */
	    width = bounds.maxx-bounds.minx;
	    skip = lbm->rowBytes - width*4;
	    wchars = (width) * (readAlpha ? 4 : 3);
    
	    /* Make sure string is long enough for single scanline */
	    if (!(dh = s.length / wchars)) PSLimitCheck();
    
	    sp = (unsigned char *) lbm->bits;
	    s.length = wchars*dh;
	    for (; h > 0; h -= dh) {
		if (h < dh) dh = h;
		/* If no dest alpha, remove it, putting result in string */
		dp = (unsigned char *) s.val.strval;
		if (readAlpha) {
		    for (k=dh; k>0; k--) {
			for(w=width; w>0; w--) {
			    *(unsigned int *)dp = *(unsigned int *)sp;
			    dp+=4; sp+=4;
			}
			sp += skip;
		    }
		} else {
		    for (k=dh; k>0; k--) {
			for (w=width; w>0; w--)
			    {*dp++=*sp++; *dp++=*sp++; *dp++=*sp++; sp++;}
			sp += skip;
		    }
		}
		PSPushObject(&s);
		if (PSExecuteObject(&dProc)) goto Error;
	    }
	    break;
	}
	default: CantHappen();
    }
Error:
    bm_delete(lbm);
}

private procedure PSSetAlpha()
{
    real val;
    PNextGSExt ep;

    /* No alpha available on printers */
    if (PSBuildMasks())
	PSUndefined();

    PSPopPReal(&val);
    if (val<0) val = 0.0;
    if (val>1) val = 1.0;
    ep = *((PNextGSExt *)PSGetGStateExt(NULL));
    ep->realalpha = val;
    ep->alpha = (unsigned char)(((int)(65535*val))>>8);
    SetDevColor(ChangeColor());
}

private procedure PSSetGrayPattern()
{
    real val;
    PNextGSExt ep;
    int pat;

    CheckWindow();

    pat = PSPopInteger();
    if(pat < 1 || pat > 3)
	PSRangeCheck();
    ep = *((PNextGSExt *)PSGetGStateExt(NULL));
    ep->graypatstate = pat;
    ep->patternpending = 1;	/* alert convertcolor that a pattern's
				   been set */
    SetDevColor(ChangeColor());	/* this will call windowdevice's convert
				   color proc */
}

private procedure PSSetExposureColor()
{
    CheckWindow();
    LSetExposureColor(Wd2Layer(PSGetDevice(NULL)));
}

private procedure PSSetInstance()
{
    INSTANCING(PSGetGStateExt(NULL)) = PSPopBoolean();
}

private readonly RgOpTable cmdWindowGraphics = {
    "alphaimage", PSAlphaImage,
    "composite", PSComposite,
    "compositerect", PSCompositeRect,
    "currentalpha", PSCurrentAlpha,
    "currentwindowalpha", PSCurrentWindowAlpha,
    "dissolve", PSDissolve,
    "fillwindow", PSFillWindow,
    "flushgraphics", PSFlushGraphics,
    "hideinstance", PSHideInstance,
    "newinstance", PSNewInstance,
    "readimage", PSReadImage,
    "setalpha", PSSetAlpha,
    "setexposurecolor", PSSetExposureColor,
    "setinstance", PSSetInstance,
    "NXsetgraypattern", PSSetGrayPattern,
    NIL};

public procedure IniWindowGraphics(int reason)
{
    if (reason==1)
	PSRgstOps(cmdWindowGraphics);
}

















