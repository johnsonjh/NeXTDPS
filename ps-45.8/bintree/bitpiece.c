/*****************************************************************************

    bitpiece.c
    BitPiece implementation of the bintree window system.

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 23Apr86 Leo

    Modified:

    21Sep87 Leo   Absolute max on storage pools
    06Jan88 Leo   Straight C, removed old modified log
    26Mar88 Leo   CompositeOperation structure
    23May88 Leo   Don't blindly dereference cop->src!
    23May88 Leo   Protect against NONRETAINED source in BPCompositeFrom
    13Jun88 Leo   Made visFlag an unsigned char
    01Mar89 Ted   Ongoing work on post 1.0.
    16May89 Ted   ANSI C function prototypes
    13Nov89 Terry CustomOps conversion
    05Dec89 Ted   Integratathon!
    08Jan90 Ted   Removed alphabits (yeah!) Now part of bitmap
    10Jan90 Ted   Implemented BPApplyBoundsProc & BPFindMaxDepth
    11Jan90 Ted   Changed srcD->src, dstD->dst, removed register usage
    02Feb90 Terry Changed BPFindMaxDepth to BPFindFormat
    13Feb90 Terry Set up markinfo offset before all BAGCompositeFrom calls
    16Feb90 Terry Careful setting of markinfo offset in BPAdjust
    27Feb90 Terry BPMark now uses dest bitmap's device instead of bp's
    27Feb90 Terry BPFindFormat now uses bitmap's device instead of bp's
    01Mar90 Terry Added RedrawWindow call to generate screenChanged event
    08Mar90 Terry Avoid doing a BPCopyback for Retained windows with A_BITS
    09May90 Ted   Pass bags down through cop instead of bitmaps
    24Jun90 Ted   New API: Removed FIXED window type
    28Jun90 Ted   Removed BPFindFormat (obsolete) and BPIsOffscreen (dead code)
    28Jun90 Ted   Removed BPIsVisible (optimization)
    31Jul90 Ted   Added BPRenderInBounds callback for NXRenderInBounds.

******************************************************************************/

#import PACKAGE_SPECS
#import "bintreetypes.h"
#import "bitmap.h"

static char *bitPiecePool;		/* Blind ptr to BitPiece storage */
static CompositeOperation flushCo;	/* For flushing retained windows */


/*****************************************************************************
    BPInitialize
    Create pool for bitpieces.
******************************************************************************/
void BPInitialize()
{
    copyCO(&flushCo);
    flushCo.dstCH = VISCHAN;
    flushCo.srcCH = BACKCHAN;
    bitPiecePool = (char *) os_newpool(sizeof(BitPiece), 0, 0);
}


/*****************************************************************************
    BPNewAt
    New bitpiece.  Caller provides all fields.
******************************************************************************/
BitPiece *BPNewAt(NXBag *bag, int channel, Bounds bounds, int vis, Layer *layer,
    NXDevice *device, NXDevice *conv)
{
    BitPiece *bp;

    bp = (BitPiece *) os_newelement(bitPiecePool);
    bp->type = BITPIECE;
    bp->visFlag = vis;
    bp->device = device;
    bp->bounds = bounds;
    bp->layer = layer;
    bp->conv = conv;
    bp->chan = channel;
    bp->bag = bag;
    if (bag) BAGDup(bag);
    return bp;
}


/*****************************************************************************
    BPAdjust
    Move bitpiece of layer's tree in global coordinates.
    (The backing store has already been adjusted.)
    If bitpiece is visible, moves it on screen (which may imply
    conversion if across screen boundaries).
******************************************************************************/
void BPAdjust(BitPiece *bp, short dx, short dy)
{
    NXBag *oldbag;
    CompositeOperation co;
    int doblit = (!(bp->bounds.minx==bp->bounds.maxx || bp->bounds.miny==bp->bounds.maxy));

    DebugAssert(bp->type == BITPIECE);
    oldbag = bp->bag;
    copyCO(&co);
    co.srcBounds = bp->bounds;
    co.info.offset.x = bp->layer->bounds.minx;
    co.info.offset.y = bp->layer->bounds.miny;
    if (bp->conv) {
	LRedraw(bp->layer, &bp->bounds, REDRAW_CHANGED);
	bp->device = bp->conv;
	bp->conv = NULL;
	bp->bag = BAGFind(bp->layer, bp->device);
	/* Convert backing store. For retained windows, the backing
	 * store has already been validated by BPObscureInside.
	 */
	if (doblit && bp->layer->layerType != NONRETAINED) {
	    /* Since the backing bitmaps have not been adjusted yet and
	     * neither has the layer bounds, we should pass in the old
	     * layer bounds as the markInfo offsets.
	     */
	    co.src.bag = oldbag;
	    co.dst.bag = bp->bag;
	    co.srcCH = co.dstCH = BACKCHAN;				
	    BAGCompositeFrom(&co);
	}
	if (bp->visFlag == VISIBLE && doblit) {
	    /* Since the visible bitmaps don't get adjusted, and the
	     * the layer bounds haven't been adjusted yet, we should pass
	     * in the new layer bounds as the markInfo offsets.
	     */
	    co.delta.cd.x = dx;
	    co.delta.cd.y = dy;
	    co.info.offset.x += dx;
	    co.info.offset.y += dy;
	    if (bp->layer->layerType != NONRETAINED) {
		co.src.bag = co.dst.bag = bp->bag;
		co.srcCH = BACKCHAN;
		co.dstCH = VISCHAN;
	    } else {
		co.src.bag = oldbag;
		co.dst.bag = bp->bag;
		co.srcCH = co.dstCH = VISCHAN;					
	    }
	    BAGCompositeFrom(&co);
	}
	BAGDelete(oldbag);
    } else if (bp->visFlag == VISIBLE && doblit) {
    	/* Always move visible onscreen bits if within same screen */
	co.src.bag = co.dst.bag = bp->bag;
	co.srcCH = co.dstCH = VISCHAN;				
	co.delta.cd.x = dx;
	co.delta.cd.y = dy;
	co.info.offset.x += dx;
	co.info.offset.y += dy;
	BAGCompositeFrom(&co);
    }
    OFFSETBOUNDS(bp->bounds, dx, dy);
}


/*****************************************************************************
    BPAllocBag
    Used to create a layer.  A layer is split up over the devices it
    lies on.  This routine creates a "bit bag" for each device and sets
    each bitpiece to the appropriate one.
******************************************************************************/
void BPAllocBag(BitPiece *bp)
{
    if (bp->device == NULL) {
	bp->device = bp->conv;
	bp->conv = NULL;
    }
    bp->bag = BAGFind(bp->layer, bp->device);
    bp->chan = BACKCHAN;
}


/*****************************************************************************
    BPApplyBounds
    Callback proc for NXApplyBounds.  Calls the user-supplied proc in bundle.
******************************************************************************/
void BPApplyBounds(BitPiece *bp, Bounds *bounds, void *data)
{
    Bounds irect;
    register BoundsBundle *bundle = (BoundsBundle *)data;
    
    if (!sectBounds(&bp->bounds, bounds, &irect)) return;
    (*bundle->proc)(bp->bag, (int)bp->visFlag, irect, *bounds, bundle->data); 
}


/*****************************************************************************
    BPApplyBoundsProc
    Call the desired procedure with the given data now that we're at a
    leaf node (BitPiece).
******************************************************************************/
void BPApplyBoundsProc(BitPiece *bp, Bounds *bounds, void *data,
    void (*proc)())
{
    (*proc)(bp, bounds, data);
}


/*****************************************************************************
    BPApplyProc
    Call the procedure.
******************************************************************************/
void BPApplyProc(BitPiece *bp, void (*proc)())
{
    (*proc)(bp);
}


/*****************************************************************************
    BPBecomeDivAt
    Create divpiece from bitpiece.
******************************************************************************/
DivPiece *BPBecomeDivAt(BitPiece *bp, int coord, unsigned char thisOrient)
{
    DebugAssert(bp->type == BITPIECE);
    return BPDivideAt(bp, coord, thisOrient, NOREASON);
}


/*****************************************************************************
    BPCompositeFrom
    Source is a BitPiece, DivPiece, Pattern or Nil.
    Destination is a BitPiece.
******************************************************************************/
void BPCompositeFrom(CompositeOperation *cop)
{
    NXBag *bag;
    BitPiece *bp = cop->dst.bp;

    /* Check for empty bounds case */
    DebugAssert(bp);
    DebugAssert(bp->type == BITPIECE);
    if (bp->bounds.minx==bp->bounds.maxx || bp->bounds.miny==bp->bounds.maxy)
	return;

#if STAGE == DEVELOP
    {
	Bounds dstBounds = cop->srcBounds;
	if (cop->src.any) {
	    DebugAssert(cop->src.any->type == BITPIECE ||
			cop->src.any->type == DIVPIECE ||
			cop->src.any->type == PATTERN);
	}
	if (cop->delta.i)
	    OFFSETBOUNDS(dstBounds, cop->delta.cd.x, cop->delta.cd.y);
	if (!withinBounds(&dstBounds, &bp->bounds)) {
	    os_fprintf(os_stderr,
	    "bitpiece(%x)(win:%x): BPCompositeFrom: bitBlt out of bounds!\n",
		cop->dst.bp, cop->dst.bp->layer);
	    CantHappen();
	}
    }
#endif

    /* If dst is offscreen and Nonretained, we can't draw on it so return.
     * If source is a DivPiece, then we must further divide it.
     */
    if (bp->layer->layerType == NONRETAINED && bp->visFlag != VISIBLE)
	return;
    else if (cop->src.any && (cop->src.any->type == DIVPIECE))
	DPCompositeTo(cop);
    else if ((!cop->instancing) || (bp->visFlag == VISIBLE)) {
	flushCo.mode = 0;
	/* PROCESS DESTINATION */
	cop->dst.bag = bp->bag;
	if (cop->dstCH == BPCHAN) {
	    cop->dstCH = bp->chan;
	    if (!cop->instancing && cop->dstCH==VISCHAN &&
	    bp->layer->layerType==RETAINED && (bp->layer->alphaState==A_BITS
	    || bp->bag->mismatch)) {
		cop->dstCH = BACKCHAN;
		if (bp->visFlag == VISIBLE) {
		    flushCo.mode = COPY;
		    flushCo.src.bag = flushCo.dst.bag = bp->bag;
		    flushCo.srcBounds = cop->srcBounds;
		    if (cop->delta.i)
			OFFSETBOUNDS(flushCo.srcBounds, cop->delta.cd.x,
			    cop->delta.cd.y);
		}
	    }
	}
	/* Set mark offset to upper left corner of destination layer */
	cop->info.offset.x = bp->layer->bounds.minx;
	cop->info.offset.y = bp->layer->bounds.miny;
	/* PROCESS SOURCE */
	if (cop->srcIsDst) cop->src.bp = bp;
	if (cop->src.any && cop->src.any->type == BITPIECE) {
	    BitPiece *sbp = cop->src.bp;
	    if (sbp->layer->layerType==NONRETAINED && sbp->visFlag!=VISIBLE) {
		cop->src.pat = sbp->layer->exposure;
	    } else {
		cop->src.bag = sbp->bag;
		if (cop->srcCH == BPCHAN) {
		    /* Note: even if instancing, we still take source from
		     * backing
		     */
		    cop->srcCH = sbp->chan;
		    if (cop->srcCH==VISCHAN && sbp->layer->layerType==RETAINED
		    && (sbp->layer->alphaState==A_BITS || sbp->bag->mismatch))
			cop->srcCH = BACKCHAN;
		}
	    }
	}
	BAGCompositeFrom(cop);
	if (flushCo.mode) BAGCompositeFrom(&flushCo);
    }
}


/*****************************************************************************
    BPCompositeTo
    Source is a BitPiece.
    Destination is a BitPiece or DivPiece.
******************************************************************************/
void BPCompositeTo(CompositeOperation *cop)
{
    BitPiece *bp = cop->src.bp, *dbp = NULL;
    DebugAssert(bp);
    DebugAssert(bp->type == BITPIECE);
    /* Check for empty bounds case */
    if ((bp->bounds.minx==bp->bounds.maxx)||(bp->bounds.miny==bp->bounds.maxy))
	return;
#if STAGE==DEVELOP
    if (!withinBounds(&cop->srcBounds, &(bp->bounds))) {
	os_fprintf(os_stderr, "bitpiece(%x)(win:%x): BPCompositeTo: "
	"blit out of bounds!\n", bp, bp->layer);
	CantHappen();
    }
    DebugAssert(cop->dst.any);
    DebugAssert(cop->dst.any->type==DIVPIECE || cop->dst.any->type==BITPIECE);
#endif
    /* Don't want to reference data if the layer is Nonretained and the
       bitpiece is offscreen! */
    if (bp->layer->layerType == NONRETAINED && bp->visFlag != VISIBLE)
	return;
    /* If the dst is a DivPiece, must still go and divide it down. */
    if (cop->dst.any->type == DIVPIECE)
	DPCompositeFrom(cop);
    else {
	dbp = cop->dst.bp;
	/*At this point both src and dst are BitPieces */
	flushCo.mode = 0;
	if ((!cop->instancing) || (dbp->visFlag == VISIBLE)) {
	    /* SOURCE */
	    cop->src.bag = bp->bag;
	    if (cop->srcCH == BPCHAN) {
		cop->srcCH = bp->chan;
		if (cop->srcCH==VISCHAN && bp->layer->layerType==RETAINED
		&& (bp->layer->alphaState==A_BITS || bp->bag->mismatch))
		    cop->srcCH = BACKCHAN;
	    }
	    /* DESTINATION */
	    cop->dst.bag = dbp->bag;
	    if (cop->dstCH == BPCHAN) {
		cop->dstCH = dbp->chan;
		if (!cop->instancing && cop->dstCH==VISCHAN &&
		(dbp->layer->layerType==RETAINED) &&
		(dbp->layer->alphaState==A_BITS || dbp->bag->mismatch)) {
		    cop->dstCH = BACKCHAN;
		    flushCo.mode = COPY;
		    flushCo.src.bag = flushCo.dst.bag = dbp->bag;
		    flushCo.srcBounds = cop->srcBounds;
		    if (cop->delta.i)
			OFFSETBOUNDS(flushCo.srcBounds, cop->delta.cd.x,
			    cop->delta.cd.y);
		}
	    }
	    /* Set mark offset to upper left corner of destination window */
	    cop->info.offset.x = dbp->layer->bounds.minx;
	    cop->info.offset.y = dbp->layer->bounds.miny;
	    BAGCompositeFrom(cop);
	    if (flushCo.mode) BAGCompositeFrom(&flushCo);
	}
    }
}


/*****************************************************************************
    BPCopy
    Return duplicate bitpiece.
******************************************************************************/
BitPiece *BPCopy(BitPiece *frombp)
{
    BitPiece *bp;

    DebugAssert(frombp->type == BITPIECE);
    bp = (BitPiece *) os_newelement(bitPiecePool);
    *bp = *frombp;
    if (bp->bag) BAGDup(bp->bag);
    return bp;
}


/*****************************************************************************
    BPCopyback
    Copies visible bits of bitpiece to backing store. BPCopybackBounds()
    is called from FindDiffBounds() which is called from here.
    Should only be called on RETAINED windows.
******************************************************************************/
static void BPCopybackBounds(Bounds *bounds, CompositeOperation *cop)
{
    cop->srcBounds = *bounds;
    BAGCompositeFrom(cop);
}

void BPCopyback(BitPiece *bp)
{
    CompositeOperation co;
    DebugAssert(bp->type == BITPIECE);
    /* If bitpiece isn't visible, don't need to copy back.  Also don't need to
     * if the bag mismatch is set.
     */
    if (bp->visFlag != VISIBLE || bp->bag->mismatch) return;
    if (bp->bounds.minx==bp->bounds.maxx||bp->bounds.miny==bp->bounds.maxy)
    	return;
    /* Copy bits from screen to backing store */
    copyCO(&co);      
    co.src.bag = co.dst.bag = bp->bag;
    co.srcCH = VISCHAN;
    co.dstCH = BACKCHAN;
    co.srcBounds = bp->bounds;

    /* Set mark offset to upper left corner of dest window */
    co.info.offset.x = bp->layer->bounds.minx;
    co.info.offset.y = bp->layer->bounds.miny;

    /* If layer has instancing, don't copyback the instancing bounds */
    if (bp->layer->instanceSet)
	FindDiffBounds(&bp->layer->instance, &bp->bounds,
	    BPCopybackBounds, &co);
    else
	BAGCompositeFrom(&co);
}


/*****************************************************************************
    BPCopyBitsFrom
    We are called for each bitpiece that happens to intersect the desired
    bounds. When called, we basically make the bitpiece's bitmap public and
    copy it in to our destination bitmap passed in data. After all bitpieces
    have been copied to our destination, LCopyBitsFrom is through.
******************************************************************************/
void BPCopyBitsFrom(BitPiece *bp, Bounds *bounds, void *data)
{
    Bitmap *sbm, *pbm=NULL, *dbm;
    BMCompOp *bcop;
 
    dbm = ((CBStruct *)data)->bm;
    bcop = ((CBStruct *)data)->bcop;
    if (!sectBounds(bounds, &bp->bounds, &bcop->srcBounds)) return;
    
    /* Assume we're reading the backing store */
    bcop->srcType = BM_BITMAPSRC;
    sbm = bp->bag->backbits;
    switch(bp->layer->layerType) {
    case NONRETAINED:
        if (bp->chan == BACKCHAN) {
	    bcop->src.pat = bp->layer->exposure;
	    bcop->srcType = BM_PATTERNSRC;
	    bm_composite(dbm, bcop);
	    return;
	} else {
	    sbm = bp->bag->visbits;
	}
	break;
    case RETAINED:
	if (bp->chan == VISCHAN && !(bp->layer->alphaState == A_BITS
	|| bp->bag->mismatch))
	    sbm = bp->bag->visbits;
	break;
    }
    bcop->src.bm = sbm;
    /* If the isa's of src and dst are different, then should make the
     * source public before compositing to the dst.
     */
    if (sbm->isa != dbm->isa) {
	pbm = bcop->src.bm = bm_makePublic(sbm, &bcop->srcBounds,
	    bp->layer->currentDepth);
	if (pbm->type != dbm->type) {
	    bm_convertFrom(dbm, pbm, &bcop->dstBounds,
		&bcop->srcBounds, bcop->info.screenphase);
	    goto Cleanup;
	}
    }
    bm_composite(dbm, bcop);
 Cleanup:
    if (pbm) bm_delete(pbm);
}


/*****************************************************************************
    BPDivideAt
    Split given bitpiece into a divpiece.
******************************************************************************/
DivPiece *BPDivideAt(BitPiece *bp, int coord, unsigned char newOrient,
    int cause)
{
    Piece g, p;

    DebugAssert(bp->type == BITPIECE);
    g.bp = BPCopy(p.bp = bp);
    *MaxBound(&(bp->bounds), newOrient) = *MinBound(&(g.bp->bounds), newOrient) = coord;
    return DPNewAt(coord, cause, p, g, bp->layer, newOrient);
}


/*****************************************************************************
    BPFindPieceBounds
    Return pointer to the bitpiece bounds.
******************************************************************************/
Bounds *BPFindPieceBounds(BitPiece *bp, Point pt)
{
    DebugAssert(bp->type == BITPIECE);
    return &bp->bounds;
}



/*****************************************************************************
    BPFree
    Free bitpiece.
******************************************************************************/
void BPFree(BitPiece *bp)
{
    DebugAssert(bp->type == BITPIECE);
    if (bp->bag) BAGDelete(bp->bag);
    bp->type = 0; /* mark it free! */
    os_freeelement(bitPiecePool, bp);
}


/*****************************************************************************
    BPIsObscured
    Return 0 if this bitpiece is not obscured or return obscure count.
******************************************************************************/
int BPIsObscured(BitPiece *bp)
{
    DebugAssert(bp->type == BITPIECE);
    return (bp->visFlag == OFFSCREEN) ? 0 : bp->visFlag;
}


/*****************************************************************************
    BPMark
    Mark on this BitPiece.
******************************************************************************/
BitPiece *BPMark(BitPiece *bp, MarkRec *mrec, Bounds *markBds, int instancing,
    int channel)
{
    DebugAssert(bp->type == BITPIECE);
    if (bp->visFlag != VISIBLE && (bp->layer->layerType == NONRETAINED ||
    instancing))
	return bp;
    if (bp->bounds.minx>=bp->bounds.maxx||bp->bounds.miny>=bp->bounds.maxy)
	return bp;
    flushCo.mode = 0;
    if (channel == BPCHAN) channel = bp->chan;
    if (channel == VISCHAN && bp->layer->layerType == RETAINED &&
    !instancing && (bp->layer->alphaState == A_BITS || bp->bag->mismatch)) {
	channel = BACKCHAN;
	if (bp->visFlag == VISIBLE) {
	    flushCo.mode = COPY;
	    flushCo.src.bag = flushCo.dst.bag = bp->bag;
	    sectBounds(markBds, &bp->bounds, &flushCo.srcBounds);
	}
    }
    if (channel == VISCHAN) ShieldCursor(&bp->bounds);
    (*bp->bag->procs->Mark)(bp->bag, channel, mrec, markBds, &bp->bounds);
    if (channel == VISCHAN) UnShieldCursor(&bp->bounds);
    if (flushCo.mode) BAGCompositeFrom(&flushCo);
    return bp;
}


/*****************************************************************************
    BPObscureBecause
    Obscures unconditionally.
******************************************************************************/
BitPiece *BPObscureBecause(BitPiece *bp, int cause)
{
    NXBag *bag;
    DebugAssert(bp->type == BITPIECE);

    /* We allow the visFlag == OFFSCREEN case 'cuz inital window creation uses
     * an offscreen bitmap; we need to allow the BitPiece created offscreen
     * to become obscured by other windows before it's shown. The cause ==
     * OFFSCREENREASON case is used when removing a window from the screen.
     * We could also get a bitpiece here with no layer pointer because
     * EXBuildExtents and EXDummyScreens create bp's without layers and
     * then obscure them.
     * Only do a Copyback on retained layers with no alpha or matching depths.
     */
    if (bp->layer && CopybackRetained(bp->layer)) BPCopyback(bp);
    if ((bag = bp->bag) && (bag->hookMask & (1<<NX_OBSCUREWINDOW))) {
	hookData.type = NX_OBSCUREWINDOW;
	hookData.bag = bag;
	hookData.d.obscure.bounds = bp->bounds;
	(*bag->procs->Hook)(&hookData);
    }
    bp->chan = BACKCHAN;
    if (cause == OFFSCREENREASON)
	bp->visFlag = OFFSCREEN;
    else if (bp->visFlag == OFFSCREEN)
	bp->visFlag = VISIBLE + 1;
    else
	bp->visFlag++;
    return bp;
}


/*****************************************************************************
    BPObscureInside
    Obscure bitpiece if inside r, else subdivide.
******************************************************************************/
Piece BPObscureInside(BitPiece *bp, Bounds rect, int cause)
{
    Piece p;
    Bounds innerBounds;
    BBoxCompareResult compare;

    DebugAssert(bp->type == BITPIECE);
    p.bp = bp; /* default return value */
  
    compare = IntersectAndCompareBounds(&bp->bounds, &rect, &innerBounds);
    /* Find intersection of us with rect (if null, complain) */
    if (compare == outside)
	return p;
    /* If we are completely within rect, just pass an obscure out */
    if (compare == inside) {
	if (ISCONVERTCAUSE(cause)) {
	    if (bp->device != deviceCause) {
		bp->conv = deviceCause;
		if (!bp->bag) return p;
		if (bp->layer && CopybackRetained(bp->layer))
		    BPCopyback(bp);
	    }
	} else 
	    BPObscureBecause(bp, cause);
	return p;
    }
    /* OK, we've got to divide up.  Find the first edge that doesn't match,
     * divide ourselves along it, and then pass the divide message to the
     * newly created DivPiece (recurse)
     */
    if (innerBounds.miny != bp->bounds.miny)
	p.dp = BPDivideAt(bp, innerBounds.miny, H, cause);
    else if (innerBounds.maxy != bp->bounds.maxy)
	p.dp = BPDivideAt(bp, innerBounds.maxy, H, cause);
    else if (innerBounds.minx != bp->bounds.minx)
	p.dp = BPDivideAt(bp, innerBounds.minx, V, cause);
    else if (innerBounds.maxx != bp->bounds.maxx)
	p.dp = BPDivideAt(bp, innerBounds.maxx, V, cause);
    p.dp = DPObscureInside(p.dp, rect, cause);
    return p;
}


/*****************************************************************************
    BPPointScreen
    Makes each bitpiece point to the screen device.
******************************************************************************/
void BPPointScreen(BitPiece *bp)
{
    if (bp->visFlag > VISIBLE) bp->chan = VISCHAN;
}


/*****************************************************************************
    BPPrintOn
    Print debug info.
    FIX: We need to output memory allocated for the bag.
******************************************************************************/
void BPPrintOn(BitPiece *bp, int blanks)
{
    int i;

    DebugAssert(bp->type == BITPIECE);
    if (blanks)
	for (i=0; i<blanks; i++)
	    os_fprintf(os_stdout, " ");
    os_fprintf(os_stdout, "BP(%x) ", (int)bp);
    os_fprintf(os_stdout, "dev(%x) con(%x) ", bp->device, bp->conv);
    switch (bp->visFlag) {
	case VISIBLE:
	    os_fprintf(os_stdout, "VIS ");
	    break;
	case OFFSCREEN:
	    os_fprintf(os_stdout, "OFF ");
	    break;
	default:
	    os_fprintf(os_stdout, "OBS(%d) ", bp->visFlag);
	    break;
    }
    os_fprintf(os_stdout, "bds(%d,%d,%d,%d)",
	  bp->bounds.minx, bp->bounds.maxx,
	  bp->bounds.miny, bp->bounds.maxy);
    os_fprintf(os_stdout, "\n");
}


/*****************************************************************************
    BPRenderInBounds
    Callback proc for NXRenderInBounds.  Will automatically decide which
    channel drawing should be directed to.  May flush if the layer is
    Retained and various conditions are satisifed.
******************************************************************************/
void BPRenderInBounds(BitPiece *bp, Bounds *bounds, void *data)
{
    int ch;
    BoundsBundle *bundle = (BoundsBundle *)data;
    
    flushCo.mode = 0;
    ch = bundle->chan;
    if (ch == BPCHAN) ch = bp->chan;
    if (ch == VISCHAN && bp->layer->layerType == RETAINED &&
    (bp->layer->alphaState==A_BITS || bp->bag->mismatch)) {
	ch = BACKCHAN;
	flushCo.mode = COPY;
	flushCo.src.bag = flushCo.dst.bag = bp->bag;
	sectBounds(bounds, &bp->bounds, &flushCo.srcBounds);
    }
    if (ch == BACKCHAN && bp->layer->layerType == NONRETAINED) return;
    (*bundle->proc)(bp->bag, (int)ch, bp->bounds, *bounds, bundle->data,
	(int)bp->visFlag); 
    if (flushCo.mode) BAGCompositeFrom(&flushCo);
}


/*****************************************************************************
    BPReplaceBits
    If the bitpiece is obscured (visFlag > 0) then replace the bits
    with the corresponding bits of a screen-type piece like backPiece,
    alphaPiece or wsPiece. This is usefull for LPlaceAt when we want to       
    temporarily fool the reveal machinery to not draw the default backing
    store pattern over the existing (good) bits on the screen.
******************************************************************************/
void BPReplaceBits(BitPiece *bp)
{
    if (bp->visFlag > VISIBLE) bp->chan = BACKCHAN;
}


/*****************************************************************************
    BPRevealBecause
    Reveals unconditionally.
******************************************************************************/
BitPiece *BPRevealBecause(BitPiece *bp, int cause)
{
    NXBag *bag;
#if STAGE == DEVELOP
    DebugAssert(bp->type == BITPIECE);
    if (bp->visFlag == VISIBLE) {
	os_fprintf(os_stderr,
	"bitpiece(%x):BPRevealBecause: already showing!\n", bp);
	return bp;
    }
#endif

    /* ONSCREENREASON means reveal only OFFSCREEN pieces.  That is also
    the _only_ way to reveal OFFSCREEN pieces.  Otherwise, you actually
    copy it to the screen only when the visFlag reaches 0. */
   
    if (cause == ONSCREENREASON)
	if (bp->visFlag != OFFSCREEN)
	    return bp;
	else
	    ; /* fall through to copy it onscreen */
    else
	if (bp->visFlag == OFFSCREEN)
	    return bp; /* Only ONSCREENREASON reveals an OFFSCREEN piece */
	else
	    if (--bp->visFlag > 0)
		return bp; /* was obscured by more than one other layer */
    
    /* Flush the bits from the backing store to the screen */
    if (!(bp->bounds.minx==bp->bounds.maxx||bp->bounds.miny==bp->bounds.maxy))
    {
	if (bp->layer->layerType == NONRETAINED)
	    LRepaintIn(bp->layer, bp->bounds, bp->bag);
	else {
	    CompositeOperation co;
	    copyCO(&co);      
	    co.src.bag = co.dst.bag = bp->bag;
	    co.dstCH = VISCHAN;
	    co.srcCH = bp->chan; /* not VISCHAN b/c of LPlaceAt trick */
	    co.srcBounds = bp->bounds;
	    co.revealing = 1;
	    /* Set mark offset to upper left corner of dest window */
	    co.info.offset.x = bp->layer->bounds.minx;
	    co.info.offset.y = bp->layer->bounds.miny;
	    BAGCompositeFrom(&co);
	}
	if ((bag = bp->bag) && (bag->hookMask & (1<<NX_REVEALWINDOW))) {
	    hookData.type = NX_REVEALWINDOW;
	    hookData.bag = bag;
	    hookData.d.reveal.bounds = bp->bounds;
	    (*bag->procs->Hook)(&hookData);
	}

    }
    bp->chan = VISCHAN;
    bp->visFlag = VISIBLE;
    return bp;
}


/*****************************************************************************
    BPRevealInside
    Reveal bitpiece if inside rect or subdivide.
******************************************************************************/
BitPiece *BPRevealInside(BitPiece *bp, Bounds rect, int cause)
{
    DebugAssert(bp->type == BITPIECE);
    if (TOUCHBOUNDS(rect, bp->bounds) && !ISCONVERTCAUSE(cause))
	BPRevealBecause(bp, cause);
    return bp;
}





