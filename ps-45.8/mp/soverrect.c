/*  
	soverrect.c
  
	CONFIDENTIAL
	Copyright (c) 1989 NeXT, Inc. as an unpublished work.
	All Rights Reserved.
	
	C code to Sover a rectangle of non-overlapping
	bits from one place onto another.

	Created: 26Aug89 from copyrect.c Terry

*/

#import PACKAGE_SPECS
#import DEVICE
#import DEVPATTERN
#import BINTREE
#import "mp12.h"
#import "mp.h"

#define LMASK 0x55555555 /* 01010101010101010101010101010101 */
#define HMASK 0xaaaaaaaa /* 10101010101010101010101010101010 */

#define Sover(s,a,dp) do { uint d;			\
  d = *dp; DoSover(s,a,d); *dp = d;			\
} while(0)

#define SoverMask(s,a,dp,mask) do { uint d;		 \
  d = *dp; s &= mask; a &= mask; DoSover(s,a,d); *dp = d;\
} while(0)

#define DoSover(s,a,d) do { uint m;      		 \
  m = (a^(a+a)) & (d^(d+d)) & HMASK;		         \
  d &= ~a;                                               \
  if(m) { d ^= m>>1; d &= ~m; }				 \
  d += s;						 \
} while(0)

void SoverRect(LineOperation *alop, LineOperation *slop, int srcRowBytes, 
	       int dstRowBytes, int height)
{
    uint *dstPtr,*srcPtr,*alPtr;
    uint lmask,rmask,us,s,ua,a;
    int n,numInts,shift,rShift;

    if (height == 0) return;
    alPtr   = alop->source.data.bm.pointer;
    srcPtr  = slop->source.data.bm.pointer;
    dstPtr  = slop->loc.dstPtr;
    lmask   = slop->loc.leftMask;
    rmask   = slop->loc.rightMask;
    numInts = slop->loc.numInts;
    shift   = slop->source.data.bm.leftShift;
    rShift  = 32 - shift;

    /* Undo memory offsets */
    if (use_wf_hardware) {
	if ((mpAddr <= dstPtr) && (dstPtr < mpAddr + screenOffsets[0]))
	    dstPtr -= screenOffsets[WF1-1] >> 2;
	else
	    dstPtr -= memoryOffsets[WF1-1] >> 2;
    }

    if(numInts == 0) {
	srcRowBytes -= 4;
	if (lmask >> shift) {
	    do {
		s = (*srcPtr++)<<shift; s |= (*srcPtr)>>rShift;
		a = (*alPtr++)<<shift;  a |= (*alPtr)>>rShift;
	        SoverMask(s,a,dstPtr,lmask);
	        dstPtr = (uint *)(((uchar *)dstPtr)+ dstRowBytes);
    	        srcPtr = (uint *)(((uchar *)srcPtr)+ srcRowBytes);
    	        alPtr  = (uint *)(((uchar *)alPtr) + srcRowBytes);
	    } while(--height);
	}
	else {
	    do {
		s  = (*++srcPtr)>>rShift;
		a  = (*++alPtr)>>rShift;
	        SoverMask(s,a,dstPtr,lmask);
	        dstPtr = (uint *)(((uchar *)dstPtr)+ dstRowBytes);
    	   	srcPtr = (uint *)(((uchar *)srcPtr)+ srcRowBytes);
    	        alPtr  = (uint *)(((uchar *)alPtr) + srcRowBytes);
	    } while(--height);
	}
    }	   
    else {
	dstRowBytes -= (n = numInts << 2);
	srcRowBytes -= n+4;
	do {
	    if (lmask >> shift) {
		s  = (*srcPtr++)<<shift;  us = *srcPtr++;  s |= us>>rShift;
		a  = (*alPtr++)<<shift;   ua = *alPtr++;   a |= ua>>rShift;
	    }
	    else {
		srcPtr++;  us = *srcPtr++;  s  = us>>rShift;
		alPtr++;   ua = *alPtr++;   a  = ua>>rShift;
	    }

	    SoverMask(s,a,dstPtr,lmask); dstPtr++;
	    n = numInts;
	    while(--n > 0) {
		s  = us<<shift;  us = *srcPtr++;  s |= us>>rShift;
		a  = ua<<shift;  ua = *alPtr++;   a |= ua>>rShift;
		Sover(s,a,dstPtr); dstPtr++;
	    }
	    s = us<<shift; a = ua<<shift;
	    if (rmask<<rShift) {s |= (*srcPtr)>>rShift; a |= (*alPtr)>>rShift;}
	    SoverMask(s,a,dstPtr,rmask);
	    
	    dstPtr = (uint *)(((uchar *)dstPtr)  + dstRowBytes);
    	    srcPtr = (uint *)(((uchar *)srcPtr)  + srcRowBytes);
    	    alPtr  = (uint *)(((uchar *)alPtr)   + srcRowBytes);
	} while(--height);
    }
}





