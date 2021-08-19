/*  
	copyrect.c
  
	CONFIDENTIAL
	Copyright (c) 1989 NeXT, Inc. as an unpublished work.
	All Rights Reserved.
	
	C code to move a rectangle of non-overlapping
	bits from one place to another.

	Created: 11Aug89 from copyline.c Terry

	Modifications:
	
	06Dec89 Ted Integratathon, ANSI C Prototyping, reformatting.
*/

#import PACKAGE_SPECS
#import BINTREE
#import "mp.h"

void CopyRect(LineOperation *lop, int srcRowBytes, int dstRowBytes, int height)
{
    uint *dstPtr, *srcPtr;
    uint lmask, rmask, us, s;
    int n, numInts, shift, rShift;

    if (height == 0) return;
    srcPtr  = lop->source.data.bm.pointer;
    dstPtr  = lop->loc.dstPtr;
    lmask   = lop->loc.leftMask;
    rmask   = lop->loc.rightMask;
    numInts = lop->loc.numInts;

    if (lop->source.type == SBMA)
    {
	if (numInts < 1)
	{
	    do {
		*dstPtr = (((*dstPtr) & (~lmask)) | (lmask & *srcPtr));
		dstPtr  = (uint *)(((uchar *)dstPtr) + dstRowBytes);
		srcPtr  = (uint *)(((uchar *)srcPtr)  + srcRowBytes);
	    } while(--height);
	}
	else if (numInts == 1)
	{
	    dstRowBytes -= 4;
	    srcRowBytes -= 4;
	    do {
		*dstPtr = (((*dstPtr) & ~lmask) | (lmask & *srcPtr++));
		dstPtr++;
		*dstPtr = (((*dstPtr) & ~rmask) | (rmask & *srcPtr));
		dstPtr  = (uint *)(((uchar *)dstPtr) + dstRowBytes);
		srcPtr  = (uint *)(((uchar *)srcPtr) + srcRowBytes);
	    } while(--height);
	}
	else {
	    dstRowBytes -= numInts << 2;
	    srcRowBytes -= numInts << 2;	
	    numInts--;
	    do {
		*dstPtr = (((*dstPtr) & (~lmask)) | (lmask & *srcPtr++)); 
		dstPtr++;
		n = numInts;
		do *dstPtr++ = *srcPtr++; while(--n);
		*dstPtr = (((*dstPtr) & ~rmask) | (rmask & *srcPtr));
		dstPtr  = (uint *)(((uchar *)dstPtr) + dstRowBytes);
		srcPtr  = (uint *)(((uchar *)srcPtr) + srcRowBytes);
	    } while(--height);
	}
    } else { /* Unaligned source */
	shift  = lop->source.data.bm.leftShift;
	rShift = 32 - shift;
	if (numInts == 0)
	{
	    srcRowBytes -= 4;
	    if (lmask RSHIFT shift)
	    {
		do {
		    s  = (*srcPtr++) LSHIFT shift;
		    s |= (*srcPtr) RSHIFT rShift;
		    *dstPtr = (*dstPtr & ~lmask) | (s & lmask);
		    dstPtr  = (uint *)(((uchar *)dstPtr) + dstRowBytes);
		    srcPtr  = (uint *)(((uchar *)srcPtr) + srcRowBytes);
		} while(--height);
	    }
	    else {
		do {
		    s = (*++srcPtr) RSHIFT rShift;
		    *dstPtr = (*dstPtr & ~lmask) | (s & lmask);
		    dstPtr = (uint *)(((uchar *)dstPtr) + dstRowBytes);
		    srcPtr = (uint *)(((uchar *)srcPtr) + srcRowBytes);
		} while(--height);
	    }
	}	   
	else {
	    dstRowBytes -= numInts << 2;
	    srcRowBytes -= (numInts << 2) + 4;
	    do {
		if (lmask RSHIFT shift)
		{
		    s  = (*srcPtr++) LSHIFT shift;
		    us = *srcPtr++;
		    s |= us RSHIFT rShift;
		}
		else {
		    srcPtr++;
		    us = *srcPtr++;
		    s  = us RSHIFT rShift;
		}
		*dstPtr = (*dstPtr & ~lmask) | (s & lmask);
		dstPtr++;
		n = numInts;
		while(--n > 0) 
		{
		    s  = us LSHIFT shift;
		    us = *srcPtr++;
		    *dstPtr = s | (us RSHIFT rShift);
		    dstPtr++;
		}
		if (rmask LSHIFT rShift)
		    *dstPtr = (*dstPtr & ~rmask) | (((us LSHIFT shift) |
			((*srcPtr) RSHIFT rShift)) & rmask);
		else
		    *dstPtr = (*dstPtr & ~rmask) | ((us LSHIFT shift) & rmask);
		dstPtr = (uint *)(((uchar *)dstPtr) + dstRowBytes);
		srcPtr = (uint *)(((uchar *)srcPtr) + srcRowBytes);
	    } while(--height);
	}
    }
}


