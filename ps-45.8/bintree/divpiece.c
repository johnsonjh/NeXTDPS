/*****************************************************************************

    divpiece.c
    DivPiece implementation of bintree window system

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 23Apr86 Leo

    Modified:

    21Sep87 Leo   Absolute max on storage pools
    16Dec87 Leo   Removed old change log; converted to procedure-based
		  window list and cause set
    06Jan88 Leo   Straight C
    26Mar88 Leo   CompositeOperation structure
    23May88 Leo   Don't dereference src blindly!
    28Feb89 Ted   Modify for mfbs
    16May89 Ted   ANSI C function prototypes
    17May89 Ted   Fixed CSAdd() bug in DPDivideAt
    13Nov89 Terry CustomOps conversion
    05Dec89 Ted   Integratathon!
    30May90 Ted   Removed DPCopy() for code reduction: not used
    24Jun90 Ted   Renamed PieceApplyLeafProc to PieceApplyProc.

******************************************************************************/

#import PACKAGE_SPECS
#import "bintreetypes.h"

static char *divPiecePool;    /* Blind pointer to free DivPiece pool */


/*****************************************************************************
    DPInitialize
    Create a divpiece pool.
******************************************************************************/
void DPInitialize()
{
    /* Create the storage pool we'll use for these guys */
    divPiecePool = (char *) os_newpool(sizeof(DivPiece), 0, 0);
}


/*****************************************************************************
    DPNewAt
    New divpiece.  Caller provides fields.
******************************************************************************/
DivPiece *DPNewAt(int coord, int cause, Piece lesser, Piece greater,
    Layer *layer, unsigned char newOrient)
{
    DivPiece *dp;

    dp = (DivPiece *) os_newelement(divPiecePool);
    dp->type = DIVPIECE;
    dp->orient = newOrient;
    dp->divCoord = coord;
    dp->causes = CSNew();
    dp->causes = CSAdd(dp->causes, cause);
    dp->l = lesser;
    dp->g = greater;
    return dp;
}


/*****************************************************************************
    DPAdjust
    Move the divpiece over in global coordinates and recurse.
******************************************************************************/
void DPAdjust(DivPiece *dp, short dx, short dy)
{
    short myDiff;

    DebugAssert(dp->type == DIVPIECE);
    myDiff = (dp->orient == H ? dy : dx);
    dp->divCoord += myDiff;
    if (myDiff < 0) {
	PieceAdjust(dp->l, dx, dy);
	PieceAdjust(dp->g, dx, dy);
    } else {
	PieceAdjust(dp->g, dx, dy);
	PieceAdjust(dp->l, dx, dy);
    }
}


/*****************************************************************************
    DPApplyBoundsProc
    Apply procedure over BitPieces which fall within the given bounds.
******************************************************************************/
void DPApplyBoundsProc(DivPiece *dp, Bounds *bounds, void *data,
    void (*proc)())
{
    DebugAssert(dp->type == DIVPIECE);
    if (*MinBound(bounds, dp->orient) <= dp->divCoord) /* hits lesser */
	PieceApplyBoundsProc(dp->l, bounds, data, proc);
    if (*MaxBound(bounds, dp->orient) > dp->divCoord) /* hits greater */
	PieceApplyBoundsProc(dp->g, bounds, data, proc);
}


/*****************************************************************************
    DPApplyProc
    Apply procedure over both branches of the DivPiece.
******************************************************************************/
void DPApplyProc(DivPiece *dp, void (*proc)())
{
    DebugAssert(dp->type == DIVPIECE);
    PieceApplyProc(dp->l, proc);
    PieceApplyProc(dp->g, proc);
}


/*****************************************************************************
    DPBecomeDivAt
    Find or creates a divpiece.
******************************************************************************/
DivPiece *DPBecomeDivAt(DivPiece *dp, int coord, unsigned char thisOrient)
{
    DivPiece *tmp;
    CauseSet *tmpCauses;

    DebugAssert(dp->type == DIVPIECE);
    if (dp->orient != thisOrient) {
	dp->l.dp = PieceBecomeDivAt(dp->l, coord, thisOrient);
	dp->g.dp = PieceBecomeDivAt(dp->g, coord, thisOrient);
	
	/* swap the corner subpieces */
	tmp = dp->l.dp->g.dp;
	dp->l.dp->g = dp->g.dp->l;
	dp->g.dp->l.dp = tmp;
	
	/* swap orient and coords */
	dp->orient = !dp->orient;
	dp->g.dp->orient = !(dp->g.dp->orient);
	dp->l.dp->orient = !(dp->l.dp->orient);
	dp->g.dp->divCoord = dp->divCoord;
	dp->divCoord = dp->l.dp->divCoord;
	dp->l.dp->divCoord = dp->g.dp->divCoord;
	
	/* Merge reasons */
	tmpCauses = dp->causes;
	dp->causes = dp->l.dp->causes;
	dp->causes = CSAddSet(dp->causes, dp->g.dp->causes);
	CSFree(dp->g.dp->causes);
	
	/* Make their causes like mine */
	dp->l.dp->causes = tmpCauses;
	dp->g.dp->causes = CSCopy(dp->l.dp->causes);
	return dp;
    } else {
	if (dp->divCoord == coord)
	    return dp;
	if (dp->divCoord < coord) {
	    dp->g.dp = PieceBecomeDivAt(dp->g, coord, thisOrient);
	    tmp = dp->g.dp;
	    dp->g = tmp->l;
	    tmp->l.dp = dp;
	    return tmp;
	} else {	   /* divCoord > coord */
	    dp->l.dp = PieceBecomeDivAt(dp->l, coord, thisOrient);
	    tmp = dp->l.dp;
	    dp->l = tmp->g;
	    tmp->g.dp = dp;
	    return tmp;
	}
    }
}


/*****************************************************************************
    DPCompositeFrom
    Do composite operation where:
	    dst is a divpiece.
	    src is a bitpiece, divpiece, bitmap or NULL.
******************************************************************************/
void DPCompositeFrom(CompositeOperation *cop)
{
    DivPiece *dp;
    int dmin, dmax, delta;
    CompositeOperation cog;

    dp = cop->dst.dp;
    DebugAssert(dp->type == DIVPIECE);

#if STAGE==DEVELOP
    if (cop->src.any)
	DebugAssert(cop->src.any->type == DIVPIECE ||
	    cop->src.any->type == BITPIECE ||
	    cop->src.any->type == PATTERN);
#endif

    delta = (dp->orient == H) ? cop->delta.cd.y : cop->delta.cd.x;
    dmin = *MinBound(&cop->srcBounds, dp->orient) + delta;

    /* If rect to be transferred is all above division, send it there. */
    if (dp->divCoord <= dmin) {
	cop->dst.dp = dp->g.dp;
	PieceCompositeFrom(cop);
	return;
    }
    /* If all below division, send it down there. */
    dmax = *MaxBound(&cop->srcBounds, dp->orient) + delta;
    if (dp->divCoord >= dmax) {
	cop->dst.dp = dp->l.dp;
	PieceCompositeFrom(cop);
	return;
    }

    /* Otherwise, we have to split up the rect.  Be sure to do the
       two subpieces in the 'correct' order to prevent bit wipeout. */
    cog = *cop; /* Duplicate the CompositeOperation */
    /* Now make *cop describe the lesser one, and cog the greater one. */
    cop->dst.dp = dp->l.dp;
    cog.dst.dp = dp->g.dp;
    divBoundsAt(&cop->srcBounds, &cop->srcBounds, &cog.srcBounds, dp->divCoord
	- delta, dp->orient);
    if (delta<0) {
	PieceCompositeFrom(cop);
	PieceCompositeFrom(&cog);
    } else {
	PieceCompositeFrom(&cog);
	PieceCompositeFrom(cop);
    }
}


/*****************************************************************************
    DPCompositeTo
    Do composite operation where:
	    src is a divpiece.
	    dst is a bitpiece, divpiece or bitmap.
******************************************************************************/
void DPCompositeTo(CompositeOperation *cop)
{
    DivPiece *dp;
    int smin, smax, dmin;
    CompositeOperation cog;

    dp = cop->src.dp;
    DebugAssert(dp->type == DIVPIECE);

#if STAGE==DEVELOP
    if (cop->dst.any)
	DebugAssert(cop->dst.any->type == DIVPIECE ||
	    cop->dst.any->type == BITPIECE);
#endif

    /* Same procedure as DPCompositeFrom: check for all above, all
       below, or split it up and do the two halves in the right order. */
    smin = *MinBound(&cop->srcBounds, dp->orient);
    if (dp->divCoord <= smin) {
	cop->src.dp = dp->g.dp;
	PieceCompositeTo(cop);
	return;
    }
    smax = *MaxBound(&cop->srcBounds, dp->orient);
    if (dp->divCoord >= smax) {
	cop->src.dp = dp->l.dp;
	PieceCompositeTo(cop);
	return;
    }
    dmin = smin + ((dp->orient == H) ? cop->delta.cd.y : cop->delta.cd.x);
    cog = *cop;
    cop->src.dp = dp->l.dp;
    cog.src.dp = dp->g.dp;
    divBoundsAt(&cop->srcBounds, &cop->srcBounds, &cog.srcBounds,
	dp->divCoord, dp->orient);
    if (smin > dmin) {
	PieceCompositeTo(cop);
	PieceCompositeTo(&cog);
    } else {
	PieceCompositeTo(&cog);
	PieceCompositeTo(cop);
    }
}


/*****************************************************************************
    DPDivideAt
    Causes a new division.
******************************************************************************/
DivPiece *DPDivideAt(DivPiece *dp, int coord, unsigned char newOrient,
    int cause)
{
    DebugAssert(dp->type == DIVPIECE);
    if (dp->orient == newOrient)
    {
	if (coord < dp->divCoord)
	    dp->l.dp = PieceDivideAt(dp->l, coord, newOrient, cause);
	else if (coord > dp->divCoord)
	    dp->g.dp = PieceDivideAt(dp->g, coord, newOrient, cause);
	else	   /* A new cause for an existing edge */
	    dp->causes = CSAdd(dp->causes, cause);
    } else {	/* We are a division of !orient */
	dp->l.dp = PieceDivideAt(dp->l, coord, newOrient, cause);
	dp->g.dp = PieceDivideAt(dp->g, coord, newOrient, cause);
    }
    return dp;
}


/*****************************************************************************
    DPFindPieceBounds
    Find bounds of bitpiece around pt.
******************************************************************************/
Bounds *DPFindPieceBounds(DivPiece *dp, Point p)
{
    DebugAssert(dp->type == DIVPIECE);
    return PieceFindPieceBounds(((((dp->orient == H) ? p.y : p.x) <
	dp->divCoord) ? dp->l : dp->g), p);
}


/*****************************************************************************
    DPFree
    Free the divpiece and its causes.  If flag is set, then dispose of
    bitmaps associated with bitpieces.
******************************************************************************/
void DPFree(DivPiece *dp)
{
    DebugAssert(dp->type == DIVPIECE);
    PieceFree(dp->l);
    PieceFree(dp->g);
    CSFree(dp->causes);
    dp->type = 0;	/* Mark it free! */
    os_freeelement(divPiecePool, dp);
}


/*****************************************************************************
    DPMark
    Mark recursively and return last bitpiece imaged in.
******************************************************************************/
BitPiece *DPMark(DivPiece *dp, MarkRec *mrec, Bounds *markBds,
    int instancing, int channel)
{
    BitPiece *bp;

    DebugAssert(dp->type == DIVPIECE);
    if (*MinBound(markBds, dp->orient) <= dp->divCoord) /* hits lesser */
	bp = PieceMark(dp->l, mrec, markBds, instancing, channel);
    if (*MaxBound(markBds, dp->orient) > dp->divCoord) /* hits greater */
	bp = PieceMark(dp->g, mrec, markBds, instancing, channel);
    return bp;
}


/*****************************************************************************
    DPObscureBecause
    Obscure not based on area.
******************************************************************************/
DivPiece *DPObscureBecause(DivPiece *dp, int cause)
{
    DebugAssert(dp->type == DIVPIECE);
    PieceObscureBecause(dp->l, cause);
    PieceObscureBecause(dp->g, cause);
    return dp;
}


/*****************************************************************************
    DPObscureInside
    Hides bits inside rect r and creates divpieces as necessary.
******************************************************************************/
DivPiece *DPObscureInside(DivPiece *dp, Bounds r, int cause)
{
    DebugAssert(dp->type == DIVPIECE);
    if ((*MinBound(&r, dp->orient) == dp->divCoord)
    ||  (*MaxBound(&r, dp->orient) == dp->divCoord))
	dp->causes = CSAdd(dp->causes, cause);
    if (*MinBound(&r, dp->orient) < dp->divCoord)
	dp->l = PieceObscureInside(dp->l, r, cause);
    if (*MaxBound(&r, dp->orient) > dp->divCoord)
	dp->g = PieceObscureInside(dp->g, r, cause);
    return dp;
}


/*****************************************************************************
    DPPrintOn
    Print bintree info.
******************************************************************************/
void DPPrintOn(DivPiece *dp, int blanks)
{
    int i;

    DebugAssert(dp->type == DIVPIECE);
    if (blanks)
	for (i = 0; i<blanks; i++)
	    os_fprintf(os_stdout, " ");
    os_fprintf(os_stdout, "DP(%x) at %d", (int)dp, dp->divCoord);
    if (dp->orient == H)
	os_fprintf(os_stdout, "H");
    else
	os_fprintf(os_stdout, "V");
    os_fprintf(os_stdout, " causes[");
    CSPrintOn(dp->causes);
    os_fprintf(os_stdout, "].\n");
    PiecePrintOn(dp->l, blanks + 1);
    PiecePrintOn(dp->g, blanks + 1);
}


/*****************************************************************************
	DPRevealBecause
	Reveal not based on area.
******************************************************************************/
DivPiece *DPRevealBecause(DivPiece *dp, int cause)
{
    DebugAssert(dp->type == DIVPIECE);
    PieceRevealBecause(dp->l, cause);
    PieceRevealBecause(dp->g, cause);
    return dp;
}


/*****************************************************************************
    DPRevealInside
    Shows bits inside rect r, may eliminate divpieces and merge bitpieces.
******************************************************************************/
Piece DPRevealInside(DivPiece *dp, Bounds r, int cause)
{
    Piece tmp;
    CauseSet *noCause;
    Piece returnVal;

    DebugAssert(dp->type == DIVPIECE);
    returnVal.dp = dp;
    if (cause != NOREASON) { /* If we're actually doing work */
	/* Work from the bottom up, children first while
	 * getting rid of the spurious DivPieces again.
	 */
	dp->l = PieceRevealInside(dp->l, r, cause);
	dp->g = PieceRevealInside(dp->g, r, cause);
	/* Now remove this cause from our cause Set.
	 * If we now have no causes, get rid of ourselves
	 */
	if (CSRemove(dp->causes, STRIPCONVERTCAUSE(cause)) != 0)
	    return returnVal;
    }
    /* OK, go about figuring out how to get rid of ourselves. */
    if ((dp->l.any->type == BITPIECE) && (dp->g.any->type == BITPIECE)) {
	/* BOTH HALVES ARE BITPIECES */
	Bounds newBounds;

	/* Make newBounds into concatentation of l and g's rects */
	newBounds = dp->l.bp->bounds;
	*MaxBound(&newBounds, dp->orient) = *MaxBound(&dp->g.bp->bounds, dp->orient);
	
	/* Case out on the visibilities of l and g.  Note that they should
	 * only be VISIBLE or obscured; edges due to the screen should never
	 * be eliminated.
	 */
	if (dp->l.bp->visFlag == VISIBLE) {
	    if (BPIsObscured(dp->g.bp))
		BPRevealBecause(dp->g.bp, cause);
	    returnVal.bp = BPCopy(dp->l.bp);
	    returnVal.bp->bounds = newBounds;
	} else if (dp->g.bp->visFlag == VISIBLE) {
	    BPRevealBecause(dp->l.bp, cause);
	    returnVal.bp = BPCopy(dp->g.bp);
	    returnVal.bp->bounds = newBounds;
	} else {	/* both not visible */
	    /* Temporarily make newPiece be the piece whose visFlag we want */
	    returnVal = ((BPIsObscured(dp->l.bp)<BPIsObscured(dp->g.bp)) ? dp->l : dp->g);
	    /* Copy that piece, then reset its bounds to newBounds */
	    returnVal.bp = BPCopy(returnVal.bp);
	    returnVal.bp->bounds = newBounds;
	}
	DPFree(dp);
	return returnVal;
    }
    
    if ((dp->g.any->type == DIVPIECE) && (dp->g.dp->orient == dp->orient)) {
	/* GREATER HALF IS DIVPIECE WITH SAME ORIENTATION. */
	tmp.dp = dp->g.dp;
	dp->g = dp->g.dp->l;
	tmp.dp->l = DPRevealInside(dp, r, NOREASON);
	return tmp;
    }
   
    if ((dp->l.any->type == DIVPIECE) && (dp->l.dp->orient == dp->orient)) {
	/* LESSER HALF IS DIVPIECE WITH SAME ORIENTATION. */
	tmp.dp = dp->l.dp;
	dp->l = dp->l.dp->g;
	tmp.dp->g = DPRevealInside(dp, r, NOREASON);
	return tmp;
    }
    
    /* One or both of the subpieces is a division of orientation !orient. 
    Divide the other by it's division (unless (orient == H) and there's
    a lower H division we have to promote to keep above a V division
    with a shared cause), and then rearrange the tree to
    make our division appear at the lower level. After that, just
    eliminate the lower level divisions. */
    
    noCause = dp->causes;
    if (dp->g.any->type == DIVPIECE)
	dp->l.dp = PieceBecomeDivAt(dp->l,dp->g.dp->divCoord,dp->g.dp->orient);
    else  /* l is a division of !orient */
	dp->g.dp = BPBecomeDivAt(dp->g.bp, dp->l.dp->divCoord,
	    dp->l.dp->orient);
   
    /* Take the OR of the causes. */
    dp->causes = dp->l.dp->causes;
    dp->causes = CSAddSet(dp->causes, dp->g.dp->causes);
    CSFree(dp->g.dp->causes);
   
    /* Swap the corner subpieces. */
    tmp = dp->l.dp->g;
    dp->l.dp->g = dp->g.dp->l;
    dp->g.dp->l = tmp;
    
    /* Swap orient and coords. */
    dp->orient = !dp->orient;
    dp->g.dp->orient = !(dp->g.dp->orient);
    dp->l.dp->orient = !(dp->l.dp->orient);
    dp->g.dp->divCoord = dp->divCoord;
    dp->divCoord = dp->l.dp->divCoord;
    dp->l.dp->divCoord = dp->g.dp->divCoord;
    
    /* l and g have no reason to exist (my causes already got set, above) */
    dp->l.dp->causes = noCause;
    dp->g.dp->causes = CSCopy(noCause);
    
    /* Eliminate lower level divisions. */
    dp->l = DPRevealInside(dp->l.dp, r, NOREASON);
    dp->g = DPRevealInside(dp->g.dp, r, NOREASON);
    returnVal.dp = dp;
    return returnVal;
}


/*****************************************************************************
    DPSwapCause
    Swap causes in piece tree.
******************************************************************************/
void DPSwapCause(DivPiece *dp, int newCause, int oldCause)
{
    DebugAssert(dp->type == DIVPIECE);
    CSSwapFor(dp->causes, newCause, oldCause);
    if (dp->l.any->type == DIVPIECE)
	DPSwapCause(dp->l.dp, newCause, oldCause);
    if (dp->g.any->type == DIVPIECE)
	DPSwapCause(dp->g.dp, newCause, oldCause);
}





