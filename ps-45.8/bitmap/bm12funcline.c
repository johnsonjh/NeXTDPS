/*****************************************************************************
    funcline.c

    Versions of the Line-transfer routines that do the
    five diadic write functions for 2 bit/pixel.
    
    CONFIDENTIAL
    Copyright (c) 1989 NeXT, Inc. as an unpublished work.
    All Rights Reserved.
    
    Created: 08Aug89 from copyLine.c Terry
    
    Modified:

    20Nov89 Ted   ANSI C Prototyping.
    07Dec89 Ted   Integratathon, reformatting.
    26Feb90 Terry Added WF4, which does a PLUSL
    14May90 Terry Avoid reading first longword of source if it is not needed
    07Jun90 Terry Reduced code size from 7172 to 3312
    20Jul90 Ted   bm12funcline.c

******************************************************************************/

#import PACKAGE_SPECS
#import BINTREE
#import "bitmap.h"
#import "bm12.h"
#import "bm12funcline.h"

#define BmULine(wf)						\
void WF##wf##BmULine(LineOperation *lop)			\
{								\
    uint *sCur, *dstPtr, src0, src1, mask;			\
    int shift, rShift, n;					\
								\
    dstPtr = lop->loc.dstPtr;					\
    sCur   = lop->source.data.bm.pointer;			\
    shift  = lop->source.data.bm.leftShift << BM12LOG2BD;	\
    n      = lop->loc.numInts;					\
    mask   = lop->loc.leftMask;					\
    rShift = BM12SCANUNIT - shift;				\
    src0 = 0;							\
    if (mask >> shift) src0 = *sCur;				\
    sCur++;							\
    do {							\
	do {							\
	    src1 = src0 << shift;				\
	    src1 |= (src0 = *sCur++) >> rShift;			\
	    WRITEMASK(DoWF##wf,src1,dstPtr,mask); dstPtr++;	\
	    mask = 0xffffffff;					\
	} while (--n > 0);					\
        mask = lop->loc.rightMask;				\
    } while (n == 0);						\
}

#define BmALine(wf)						\
void WF##wf##BmALine(LineOperation *lop)			\
{								\
    uint *dstPtr, *sCur, s, mask;				\
    int n;							\
    								\
    sCur   = lop->source.data.bm.pointer;			\
    dstPtr = lop->loc.dstPtr;					\
    n      = lop->loc.numInts;					\
    mask   = lop->loc.leftMask;					\
    do {							\
	do {							\
	    s = *sCur++;					\
	    WRITEMASK(DoWF##wf,s,dstPtr,mask); dstPtr++;	\
	    mask = 0xffffffff;					\
	} while (--n > 0);					\
        mask = lop->loc.rightMask;				\
    } while (n == 0);						\
}

#define BmRLine(wf)						\
void WF##wf##BmRLine(LineOperation *lop)			\
{								\
    uint *sCur, us, s, mask, *dstPtr;				\
    int shift, rShift, n;					\
    								\
    sCur    = lop->source.data.bm.pointer;			\
    dstPtr  = lop->loc.dstPtr + 1;				\
    n       = lop->loc.numInts;					\
    shift   = lop->source.data.bm.leftShift << BM12LOG2BD;	\
    mask    = lop->loc.rightMask;				\
    rShift  = BM12SCANUNIT - shift;				\
    if (shift) s = *sCur;					\
    else sCur++;						\
    do {							\
	if (n == 0) mask = lop->loc.leftMask;			\
	do {							\
	    us = s >> rShift;					\
	    s = *--sCur;					\
	    us |= s << shift;					\
	    dstPtr--; WRITEMASK(DoWF##wf,us,dstPtr,mask);	\
	    mask = 0xffffffff;					\
	} while (--n > 0);					\
    } while (n == 0);						\
}

#define PatLine(wf)						\
void WF##wf##PatLine(LineOperation *lop)			\
{								\
    uint *dstPtr, *patPtr, *patLimit, mask, pat;		\
    int n;							\
    								\
    patPtr   = lop->source.data.bm.pointer;			\
    patLimit = lop->source.data.pat.limit;			\
    dstPtr   = lop->loc.dstPtr;					\
    n        = lop->loc.numInts;				\
    mask     = lop->loc.leftMask;				\
    do {							\
	do {							\
	    pat = *patPtr++;					\
	    WRITEMASK(DoWF##wf,pat,dstPtr,mask); dstPtr++;	\
	    if (patPtr >= patLimit)				\
	    	patPtr = lop->source.data.pat.base;		\
	    mask = 0xffffffff;					\
	} while (--n > 0);					\
        mask = lop->loc.rightMask;				\
    } while (n == 0);						\
}

#define ConLine(wf)						\
void WF##wf##ConLine(LineOperation *lop)			\
{								\
    uint *dstPtr, mask, s;					\
    int n;							\
    								\
    s      = lop->source.data.cons.value;			\
    dstPtr = lop->loc.dstPtr;					\
    n      = lop->loc.numInts;					\
    mask   = lop->loc.leftMask;					\
    do {							\
	do {							\
	    WRITEMASK(DoWF##wf,s,dstPtr,mask); dstPtr++;	\
	    mask = 0xffffffff;					\
	} while (--n > 0);					\
        mask = lop->loc.rightMask;				\
    } while (n == 0);						\
}

BmULine(0)
BmALine(0)
BmRLine(0)
PatLine(0)
ConLine(0)

BmULine(1)
BmALine(1)
BmRLine(1)
PatLine(1)
ConLine(1)

BmULine(2)
BmALine(2)
BmRLine(2)
PatLine(2)
ConLine(2)

BmULine(3)
BmALine(3)
BmRLine(3)
PatLine(3)
ConLine(3)

BmULine(4)
BmALine(4)
BmRLine(4)
PatLine(4)
ConLine(4)

