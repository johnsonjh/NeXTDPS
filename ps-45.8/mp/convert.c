/*****************************************************************************
    convert.c
    Converts subrectangles of bitmaps from one format to another

    CONFIDENTIAL
    Copyright (c) 1989 NeXT, Inc. as an unpublished work.
    All Rights Reserved.
	

    Created: 06Feb90 Terry from copyrect.c and mp.c

    Modifications:
	21Apr90 Terry Optimized for speed and code size (3256 to 1880 bytes)
	07Jun90 Terry Fixed stitching problem with 32->2 conversion.
	15Jun90 Terry Reduced code size (1956 to ???? bytes).

******************************************************************************/

#import PACKAGE_SPECS
#import DEVICE
#import DEVPATTERN
#import EXCEPT
#import BINTREE
#import "mp.h"

/* From mpdev.c */
extern PatternHandle mpPattern;
extern DevHalftone mpDevHalftone;

/* gray = .3R + .59G + .11B */

#if 1
/* 3 ands, 5 shifts, 3 adds, 16 total time units */
#define CONVERT32TO8(V,R) \
  R = (((V&MASKR)>>23) + ((V&MASKG)>>14)+((V&MASKG)>>16) + ((V&MASKB)>>8))>>3
#endif

#if 0
/* 3 ands, 9 shifts, 8 adds, 31 total time units */
#define CONVERT32TO8(V,R)					\
  R = (((V&MASKR)>>20)+((V&MASKR)>>22)-((V&MASKR)>>24) +	\
       ((V&MASKG)>>11)+((V&MASKG)>>13)-((V&MASKG)>>15) +	\
       ((V&MASKB)>>5)-((V&MASKB)>>7)+((V&MASKB)>>8))>>6
#endif

#if 0
/* 3 ands, 7 shifts, 5 adds, 22 total time units */
#define CONVERT32TO8(V,R) do {			\
  uint T1 = ((V&MASKR)>>20) + ((V&MASKG)>>11);	\
  uint T2 = (T1>>2) + ((V&MASKB)>>5);		\
  R = (T1 + T2 - (T2>>2) + ((V&MASKB)>>8))>>6;	\
} while(0)
#endif

#if __GNUC__ && ISP == isp_mc68020
#if DEVICE_CONSISTENT
#define RollRight(data,n) asm("rorl %2,%0": "=d" (data): "0d" (data), "d" (n))
#else
#define RollRight(data,n) asm("roll %2,%0": "=d" (data): "0d" (data), "d" (n))
#endif
#else
#define RollRight(d,n) d = (d RSHIFT n) | (d LSHIFT (32-n))
#endif

/*****************************************************************************
	MPConvert32to2
	Converts a portion of a 32 bit bitmap to a 2 bit bitmap
******************************************************************************/

void MPConvert32to2(uint *sp, uint *dp, uint *ap, DevPoint size,
		    DevPoint srcOffset, int srcRowBytes, 
		    DevPoint dstOffset, int dstRowBytes,
		    DevMarkInfo *markinfo)
{
    uint mask,smask,lmask,rmask,val,oldval,data8,alpha8,data2,alpha2;
    static uint *mpBrick;
    uint *brick,*brickwrap;
    int i,n,w,h,sinc,dinc;
    
    w = size.x;
    h = size.y;
    n = (2*dstOffset.x)&0x1f;
    smask = 0xc0000000 >> n;
    lmask = ~leftBitArray[n];
    rmask = ~rightBitArray[(n+w*2)&0x1f];
    
    sinc = (srcOffset.y)*(srcRowBytes>>2) + srcOffset.x;
    sp += sinc;
    sinc = srcRowBytes - w*4;
 
    dinc = (dstOffset.y)*(dstRowBytes>>2) + (dstOffset.x>>4);
    dp += dinc;
    if(ap) ap += dinc;
    dinc = dstRowBytes - (((n+w*2+31)>>5)<<2);

    /* Compute the 8->2 bit conversion table on first time through */
    if (!mpBrick) {
	DevMarkInfo mi;
	PatternData data;
	uint *dp, *sp;
	int i, n;
	
	/* mpPattern is 8 high, 8 wide, and uses Gry4of4Pattern */
	mi.halftone = &mpDevHalftone;
	mi.priv = NULL;
	mi.offset.x = mi.offset.y = mi.screenphase.x = mi.screenphase.y = 0;
	dp = mpBrick = (uint *)malloc(256 * 8 * sizeof(uint));
	for(i = 0; i < 256; i++) {
	    ((DevColorVal *)&mi.color)->white = i;
	    Gry4Of4Setup(mpPattern, &mi, &data);
	    sp = (uint *)data.start;
	    n = 8;
	    if (data.constant) while(--n >= 0) *dp++ = data.value;
	    else	       while(--n >= 0) *dp++ = *sp++;
	}
    }

    /* Start our brick table at y offset */
    brick = mpBrick;
    n = ((markinfo->offset.x + markinfo->screenphase.x) & 15) << 1;
    brickwrap = brick + 8;
    brick += (dstOffset.y - markinfo->offset.y - markinfo->screenphase.y) & 7;

    if(!ap) { /* No alpha */
	do {
	    i = w;
	    data2 = *dp & lmask;
	    mask = smask;
	    val = *sp++;
	    do {
		/* Find scanline in brick and roll right with x offset */
		oldval = val;
		CONVERT32TO8(val,data8);
		data8 = *(uint *)((uchar *)brick + (data8<<5));
		RollRight(data8,n);
		do {
		    data2 |= data8 & mask;
		    if((mask>>=2)==0) {*dp++=data2; data2=0; mask=0xc0000000;}
		} while(--i != 0 && (val = *sp++) == oldval);
	    } while(i != 0);

	    if(mask != 0xc0000000) { *dp = (*dp&rmask) | data2; dp++; }
    	    sp = (uint *)((uchar *)sp + sinc);
	    dp = (uint *)((uchar *)dp + dinc);
    	    if(++brick >= brickwrap) brick -= 8;
	} while(--h != 0);
    }
    else {  /* Alpha */
	do {
	    i = w;
	    data2  = *dp & lmask;
	    alpha2 = *ap & lmask;
	    mask = smask;
	    val = *sp++;
	    do {
		/* Find scanline in brick and roll right with x offset */
	        oldval = val;
		CONVERT32TO8(val,data8);
		alpha8 = 255 - (val & MASKA);	/* alpha8 = 255 - A */
		data8 += alpha8;		/* data8  = 255 - (A - D) */
		if (data8 > 255) data8 = 255;	/* In case of illegal data */
		data8  = *(uint *)((uchar *)brick +  (data8<<5));
		alpha8 = *(uint *)((uchar *)brick + (alpha8<<5));
		RollRight(data8,n);
		RollRight(alpha8,n);
		do {
		    data2  |= data8  & mask;
		    alpha2 |= alpha8 & mask;

		    /* Shift mask, write buffers if end of longword */
		    if((mask>>=2) == 0) {
			*dp++ = data2;  data2  = 0;
			*ap++ = alpha2; alpha2 = 0;
			mask = 0xc0000000;
		    }
		} while(--i != 0 && (val = *sp++) == oldval);
	    } while(i != 0);
    
	    /* Write out remains of last longword */
	    if(mask != 0xc0000000) {
		*dp = (*dp&rmask) | data2;  dp++;
		*ap = (*ap&rmask) | alpha2; ap++;
	    }
    
	    /* Increment source, destination, and brick pointers */
	    sp = (uint *)((uchar *)sp + sinc);
	    dp = (uint *)((uchar *)dp + dinc);
	    ap = (uint *)((uchar *)ap + dinc);
    	    if(++brick >= brickwrap) brick -= 8;
	} while(--h != 0);
    }
}

/*****************************************************************************
	MPConvert2to2
	Converts specified bounds of source 2-bit bitmap from premultiplied
	towards black to premultiplied towards white and vice versa, putting
	result into specified bounds of destination 2-bit bitmap.
******************************************************************************/

void MPConvert2to2(ExpBitmap *sbm,ExpBitmap *dbm,Bounds sBounds,Bounds dBounds)
{
    uint *dstD, *dstA, *srcD, *srcA;
    uint lmask, rmask, us, s, usa, sa;
    int srcRowBytes, dstRowBytes, height, n, numInts, shift, rShift;

    dstRowBytes = dbm->rowBytes;
    srcRowBytes = sbm->rowBytes;

    numInts = ((sBounds.miny - sbm->b.bounds.miny) * (srcRowBytes>>2)) +
	      ((sBounds.minx - sbm->b.bounds.minx) >> 4);
    srcD    = sbm->bits  + numInts;
    srcA    = sbm->abits + numInts;

    numInts = ((dBounds.miny - dbm->b.bounds.miny) * (dstRowBytes>>2)) +
	      ((dBounds.minx - dbm->b.bounds.minx) >> 4);
    dstD    = dbm->bits  + numInts;
    dstA    = dbm->abits + numInts;

    numInts = (n = sBounds.maxx - sBounds.minx) >> 4;
    shift   = (dBounds.minx - dbm->b.bounds.minx) & 0xf;
    MRMasks(&lmask, &rmask, shift, &numInts, n & 0xf);
    shift   = ((sBounds.minx - sbm->b.bounds.minx - shift) & 0xf)*2;
    height  = sBounds.maxy - sBounds.miny;
    
    dstRowBytes -= (n = numInts ? numInts << 2 : 4);
    srcRowBytes -= n;
    if (shift == 0) { 
	if (!sbm->abits) /* No alpha, aligned source */
	    do {
		*dstD = (((*dstD) & (~lmask)) | (lmask & ~*srcD++)); dstD++;
		if (n = numInts) {
		    while(--n > 0) *dstD++ = ~*srcD++;
		    *dstD = (((*dstD) & ~rmask) | (rmask & ~*srcD));
		}
		dstD  = (uint *)(((uchar *)dstD) + dstRowBytes);
		srcD  = (uint *)(((uchar *)srcD) + srcRowBytes);
	    } while(--height);
	else
	    do {
		*dstD = ((*dstD & ~lmask) | (lmask & (*srcA-*srcD++))); dstD++;
		*dstA = ((*dstA & ~lmask) | (lmask & *srcA++));         dstA++;
		if (n = numInts) {
		    while(--n > 0) {*dstD++=(*srcA-*srcD++); *dstA++=*srcA++;}
		    *dstD = ((*dstD & ~rmask) | (rmask & (*srcA-*srcD)));
		    *dstA = ((*dstA & ~rmask) | (rmask & *srcA));
		}
		dstD  = (uint *)(((uchar *)dstD) + dstRowBytes);
		srcD  = (uint *)(((uchar *)srcD) + srcRowBytes);
		dstA  = (uint *)(((uchar *)dstA) + dstRowBytes);
		srcA  = (uint *)(((uchar *)srcA) + srcRowBytes);
	    } while(--height);
    } else {
	rShift = 32 - shift;
	srcRowBytes -= 4;
    	if (!sbm->abits) /* No alpha, unaligned source */
	    do {
		s  = *srcD++ LSHIFT shift;
		us = *srcD++;
		s |= us RSHIFT rShift;
		*dstD = (*dstD & ~lmask) | (~s & lmask); dstD++;
		if (n = numInts) {
		    while(--n > 0) 
		    {
			s  = us LSHIFT shift;
			us = *srcD++;
			*dstD++ = ~(s | (us RSHIFT rShift));
		    }
		    s  = us LSHIFT shift;
		    s  |= *srcD RSHIFT rShift;
		    *dstD = (*dstD & ~rmask) | (~s & rmask);
		}
		dstD = (uint *)(((uchar *)dstD) + dstRowBytes);
		srcD = (uint *)(((uchar *)srcD) + srcRowBytes);
	    } while(--height);
	else		/* Alpha, unaligned source */
	    do {
		s   = *srcD++ LSHIFT shift;
		us  = *srcD++;
		s  |= us RSHIFT rShift;
		sa  = *srcA++ LSHIFT shift;
		usa = *srcA++;
		sa |= usa RSHIFT rShift;
		*dstD = (*dstD & ~lmask) | ((sa-s) & lmask); dstD++;
		*dstA = (*dstA & ~lmask) | (sa & lmask);     dstA++;
		if (n = numInts) {
		    while(--n > 0) 
		    {
			s   = us LSHIFT shift;
			us  = *srcD++;
			s  |= us RSHIFT rShift;
			sa  = usa LSHIFT shift;
			usa = *srcA++;
			sa |= usa RSHIFT rShift;
			*dstD++ = sa-s;
			*dstA++ = sa;
		    }
		    s  = us LSHIFT shift;
		    sa = usa LSHIFT shift;
		    s  |= *srcD RSHIFT rShift;
		    sa |= *srcA RSHIFT rShift;
		    *dstD = (*dstD & ~rmask) | ((sa-s) & rmask);
		    *dstA = (*dstD & ~rmask) | (sa & rmask);
		}
		dstD = (uint *)(((uchar *)dstD) + dstRowBytes);
		srcD = (uint *)(((uchar *)srcD) + srcRowBytes);
		dstA = (uint *)(((uchar *)dstA) + dstRowBytes);
		srcA = (uint *)(((uchar *)srcA) + srcRowBytes);
	    } while(--height);
    }
}

