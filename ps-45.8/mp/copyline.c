/*****************************************************************************
	copyline.c
	Low-level MegaPixel-specific line-by-line compositing code.

	CONFIDENTIAL
	Copyright (c) 1988 NeXT, Inc. as an unpublished work.
	All Rights Reserved.

	Created: 29Jan88 from moveline.c Leo

	Modified:
	31Jan88 Jack  Convert shifts from PIXELS
	25Jan89 Jack  Changed framelog2BD to MP12LOG2BD
	01Mar89 Ted   Diffed from v006/v001
	14May90 Terry Reduced code size from 672 to 572 bytes

******************************************************************************/

#import PACKAGE_SPECS
#import BINTREE
#import "bitmap.h"
#import "mp12.h"

/*****************************************************************************
	WCOPYBmULine
	Do left-to-right processing from unaligned bitmap.
******************************************************************************/

void WCOPYBmULine(LineOperation *lop)
{
    uint *sCur, unshftd, shifted, *dstPtr;
    int shift, rShift, msk;
    
    dstPtr = lop->loc.dstPtr;
    sCur = lop->source.data.bm.pointer;
    shift = lop->source.data.bm.leftShift << MP12LOG2BD;
    rShift = MP12SCANUNIT-shift;
    msk = lop->loc.leftMask;
    shifted = 0;
    if (msk >> shift) shifted = *sCur << shift;
    sCur++;
    unshftd = *sCur++;
    shifted |= unshftd >> rShift;
    *dstPtr = (*dstPtr & ~msk) | (shifted & msk); dstPtr++;

    if (msk = lop->loc.numInts) {
	while (--msk != 0) {
	    shifted = unshftd<<shift;
	    unshftd = *sCur++;
	    *dstPtr++ = shifted | (unshftd>>rShift);
	}
	msk = lop->loc.rightMask;
	unshftd <<= shift;
	if (msk << rShift) unshftd |= *sCur >> rShift;
	*dstPtr = (*dstPtr & ~msk) | (unshftd & msk);
    }
}

/*****************************************************************************
	WCOPYBmALine
	Do left-to-right processing from aligned bitmap.
******************************************************************************/

void WCOPYBmALine(LineOperation *lop)
{
    uint *dstPtr, *sCur, msk;
    int n;

    sCur = lop->source.data.bm.pointer;
    dstPtr = lop->loc.dstPtr;
    msk = lop->loc.leftMask;
    *dstPtr = (*dstPtr & ~msk) | (*sCur++ & msk); dstPtr++;
    if (n = lop->loc.numInts) {
        while (--n != 0) *dstPtr++ = *sCur++;
	msk = lop->loc.rightMask;
	*dstPtr = (*dstPtr & ~msk) | (*sCur & msk);
    }
}

/*****************************************************************************
	WCOPYPatLine
	Do left-to-right processing from a pattern.
******************************************************************************/

void WCOPYPatLine(LineOperation *lop)
{
    uint *dstPtr, *patPtr, *patLimit, msk;
    int n;
    
    patPtr = lop->source.data.bm.pointer;
    patLimit = lop->source.data.pat.limit;
    dstPtr = lop->loc.dstPtr;
    msk = lop->loc.leftMask;
    *dstPtr = (*dstPtr & ~msk) | (*patPtr++ & msk); dstPtr++;
    if (patPtr >= patLimit)
	patPtr = lop->source.data.pat.base;
    if (n = lop->loc.numInts) {
        while (--n != 0) {
	    *dstPtr++ = *patPtr++;
	    if (patPtr >= patLimit) patPtr = lop->source.data.pat.base;
	}
	msk = lop->loc.rightMask;
	*dstPtr = (*dstPtr & ~msk) | (*patPtr & msk);
    }
}

/*****************************************************************************
	WCOPYConLine
	Do left-to-right processing from a constant.
******************************************************************************/

void WCOPYConLine(LineOperation *lop)
{
    uint *dstPtr, s, msk;
    int n;
    
    s = lop->source.data.cons.value;
    dstPtr = lop->loc.dstPtr;
    msk = lop->loc.leftMask;
    *dstPtr = (*dstPtr & ~msk) | (s & msk); dstPtr++;
    if (n = lop->loc.numInts) {
        while (--n != 0) *dstPtr++ = s;
	msk = lop->loc.rightMask;
	*dstPtr = (*dstPtr & ~msk) | (s & msk);
    }
}

/*****************************************************************************
	RWCOPYBmULine
	Do right-to-left processing from unaligned bitmap.
******************************************************************************/

void RWCOPYBmULine(LineOperation *lop)
{
    uint *sCur, shifted, unshftd, *dstPtr, msk;
    int shift, rShift, n;
    
    dstPtr = lop->loc.dstPtr;
    sCur = lop->source.data.bm.pointer;
    shift = lop->source.data.bm.leftShift << MP12LOG2BD;
    rShift = MP12SCANUNIT - shift;
    msk = lop->loc.rightMask;
    shifted = 0;
    if (msk << rShift) shifted = *sCur >> rShift;
    if (n = lop->loc.numInts) {
	unshftd = *--sCur;
	*dstPtr = (*dstPtr & ~msk) | (((unshftd << shift) | shifted) & msk);
	shifted = unshftd >> rShift;
	while (--n != 0) {
	    unshftd = *--sCur;
	    *--dstPtr = (unshftd << shift) | shifted;
	    shifted = unshftd >> rShift;
	}
	--dstPtr;
    }
    msk = lop->loc.leftMask;
    if (msk >> shift) shifted |= *--sCur << shift;
    *dstPtr = (*dstPtr & ~msk) | (shifted & msk);
}

/*****************************************************************************
	RWCOPYBmALine
	Do right-to-left processing from aligned bitmap.
******************************************************************************/

void RWCOPYBmALine(LineOperation *lop)
{
    uint *dstPtr, *sCur, msk;
    int n;
    
    sCur = lop->source.data.bm.pointer;
    dstPtr = lop->loc.dstPtr;
    if (n = lop->loc.numInts) {
        msk = lop->loc.rightMask;
	*dstPtr = (*dstPtr & ~msk) | (*sCur & msk);
	while (--n != 0) *--dstPtr = *--sCur;
        --dstPtr; --sCur;
    }
    msk = lop->loc.leftMask;
    *dstPtr = (*dstPtr & ~msk) | (*sCur & msk);
}


