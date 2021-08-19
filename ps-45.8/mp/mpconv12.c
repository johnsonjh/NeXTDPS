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
	05Oct90 pgraff added masking berfore subtraction in 2to2 conversion

******************************************************************************/

#import PACKAGE_SPECS
#import DEVICE
#import DEVPATTERN
#import EXCEPT
#import BINTREE
#import "mp12.h"

static unsigned int *mpBrick;

/* From mpdev.c */
extern PatternHandle mpPattern;
extern DevHalftone mpDevHalftone;

/* gray = .3R + .59G + .11B */

#define MASKR38 0xFF000000
#define MASKG38 0x00FF0000
#define MASKB38 0x0000FF00
#define MASKA38 0x000000FF

#define MASKR34 0xF000
#define MASKG34 0x0F00
#define MASKB34 0x00F0
#define MASKA34 0x000F

#if 1
/* 3 ands, 5 shifts, 3 adds, 16 total time units */
#define CONVERT16TO8(V,R) \
  R = (((V&MASKR34)>>11) + ((V&MASKG34)>>6)+((V&MASKG34)>>8) + \
  ((V&MASKB34)>>4))
#endif

#if 1
/* 3 ands, 5 shifts, 3 adds, 16 total time units */
#define CONVERT32TO8(V,R) \
  R = (((V&MASKR38)>>23) + ((V&MASKG38)>>14)+((V&MASKG38)>>16) + \
  ((V&MASKB38)>>8))>>3
#endif

#if 0
/* 3 ands, 9 shifts, 8 adds, 31 total time units */
#define CONVERT32TO8(V,R)					\
  R = (((V&MASKR38)>>20)+((V&MASKR38)>>22)-((V&MASKR38)>>24) +	\
       ((V&MASKG38)>>11)+((V&MASKG38)>>13)-((V&MASKG38)>>15) +	\
       ((V&MASKB38)>>5)-((V&MASKB38)>>7)+((V&MASKB38)>>8))>>6
#endif

#if 0
/* 3 ands, 7 shifts, 5 adds, 22 total time units */
#define CONVERT32TO8(V,R) do {			\
  unsigned int T1 = ((V&MASKR38)>>20) + ((V&MASKG38)>>11);	\
  unsigned int T2 = (T1>>2) + ((V&MASKB38)>>5);		\
  R = (T1 + T2 - (T2>>2) + ((V&MASKB38)>>8))>>6;	\
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

static void MP12InitBricks()
{
    DevMarkInfo mi;
    PatternData data;
    unsigned int *dp, *sp;
    int i, n;
    
    /* mpPattern is 8 high, 8 wide, and uses Gry4of4Pattern */
    mi.halftone = &mpDevHalftone;
    mi.priv = NULL;
    mi.offset.x = mi.offset.y = mi.screenphase.x = mi.screenphase.y = 0;
    dp = mpBrick = (unsigned int *)malloc(256 * 8 * sizeof(unsigned int));
    for (i = 0; i < 256; i++) {
	((DevColorVal *)&mi.color)->white = i;
	Gry4Of4Setup(((LocalBMClass *)mp12)->bmPattern, &mi, &data);
	sp = (unsigned int *)data.start;
	n = 8;
	if (data.constant)
	    while(--n >= 0) *dp++ = data.value;
	else
	    while(--n >= 0) *dp++ = *sp++;
    }
}

/*****************************************************************************
    MP12Convert2to2
    Converts specified bounds of source 2-bit bitmap from premultiplied
    towards black to premultiplied towards white and vice versa, putting
    result into specified bounds of destination 2-bit bitmap.
******************************************************************************/
void MP12Convert2to2(LocalBitmap *sbm, LocalBitmap *dbm, Bounds sb,
    Bounds db)
{
    unsigned int *dstD, *dstA, *srcD, *srcA;
    unsigned int lmask, rmask, us, s, usa, sa;
    int srcRowBytes, dstRowBytes, height, n, numInts, shift, rShift;

    dstRowBytes = dbm->rowBytes;
    srcRowBytes = sbm->rowBytes;

    numInts = ((sb.miny - sbm->base.bounds.miny) * (srcRowBytes>>2)) +
	      ((sb.minx - sbm->base.bounds.minx) >> 4);
    srcD    = sbm->bits  + numInts;
    srcA    = sbm->abits + numInts;

    numInts = ((db.miny - dbm->base.bounds.miny) * (dstRowBytes>>2)) +
	      ((db.minx - dbm->base.bounds.minx) >> 4);
    dstD    = dbm->bits  + numInts;
    dstA    = dbm->abits + numInts;

    numInts = (n = sb.maxx - sb.minx) >> 4;
    shift   = (db.minx - dbm->base.bounds.minx) & 0xf;
    MRMasks(&lmask, &rmask, shift, &numInts, n & 0xf);
    shift   = ((sb.minx - sbm->base.bounds.minx - shift) & 0xf)*2;
    height  = sb.maxy - sb.miny;
    
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
		dstD = (unsigned int *)(((unsigned char *)dstD)+dstRowBytes);
		srcD = (unsigned int *)(((unsigned char *)srcD)+srcRowBytes);
	    } while(--height);
	else
	    do {
		*dstD = ((*dstD & ~lmask) |
			 ((lmask & *srcA) - (lmask & *srcD++)));
		dstD++;
		*dstA = ((*dstA & ~lmask) | (lmask & *srcA++));
		dstA++;
		if (n = numInts) {
		    while(--n > 0) {*dstD++=(*srcA-*srcD++); *dstA++=*srcA++;}
		    *dstD = ((*dstD & ~rmask) |
			     ((rmask & *srcA) - (rmask & *srcD)));
		    *dstA = ((*dstA & ~rmask) | (rmask & *srcA));
		}
		dstD = (unsigned int *)(((unsigned char *)dstD) + dstRowBytes);
		srcD = (unsigned int *)(((unsigned char *)srcD) + srcRowBytes);
		dstA = (unsigned int *)(((unsigned char *)dstA) + dstRowBytes);
		srcA = (unsigned int *)(((unsigned char *)srcA) + srcRowBytes);
	    } while(--height);
    } else {
	rShift = 32 - shift;
	if (lmask RSHIFT shift)
	    srcRowBytes -= 4;
    	if (!sbm->abits) /* No alpha, unaligned source */
	    do {
	        s = 0;
	        if (lmask RSHIFT shift)
		    s = (*srcD++) LSHIFT shift;
		us = *srcD++;
		s |= us RSHIFT rShift;
		*dstD = (*dstD & ~lmask) | (~s & lmask); dstD++;
		if (n = numInts) {
		    while(--n > 0) {
			s  = us LSHIFT shift;
			us = *srcD++;
			*dstD++ = ~(s | (us RSHIFT rShift));
		    }
		    s  = us LSHIFT shift;
		    s |= *srcD RSHIFT rShift;
		    *dstD = (*dstD & ~rmask) | (~s & rmask);
		}
		dstD = (unsigned int *)(((unsigned char *)dstD) + dstRowBytes);
		srcD = (unsigned int *)(((unsigned char *)srcD) + srcRowBytes);
	    } while(--height);
	else		/* Alpha, unaligned source */
	    do {
	        s = sa = 0;
		if (lmask RSHIFT shift) {
		    s  = (*srcD++) LSHIFT shift;
		    sa = (*srcA++) LSHIFT shift;
		}
		us = *srcD++;
		s |= us RSHIFT rShift;
 		usa = *srcA++;
		sa |= usa RSHIFT rShift;
		sa &= lmask;
		s  &= lmask;
		*dstD = (*dstD & ~lmask) | (sa-s); dstD++;
		*dstA = (*dstA & ~lmask) | sa;     dstA++;
		if (n = numInts) {
		    while(--n > 0) {
			s   = us LSHIFT shift;
			us  = *srcD++;
			s  |= us RSHIFT rShift;
			sa  = usa LSHIFT shift;
			usa = *srcA++;
			sa |= usa RSHIFT rShift;
			*dstD++ = sa-s;
			*dstA++ = sa;
		    }
		    s   = us LSHIFT shift;
		    sa  = usa LSHIFT shift;
		    s  |= *srcD RSHIFT rShift;
		    sa |= *srcA RSHIFT rShift;
		    sa &= rmask;
		    s &= rmask;
		    *dstD = (*dstD & ~rmask) | (sa-s);
		    *dstA = (*dstA & ~rmask) | sa;
		}
		dstD = (unsigned int *)(((unsigned char *)dstD) + dstRowBytes);
		srcD = (unsigned int *)(((unsigned char *)srcD) + srcRowBytes);
		dstA = (unsigned int *)(((unsigned char *)dstA) + dstRowBytes);
		srcA = (unsigned int *)(((unsigned char *)srcA) + srcRowBytes);
	    } while(--height);
    }
}


/*****************************************************************************
    MP12Convert16to2
    Converts a portion of a 16 bit bitmap to a 2 bit bitmap
******************************************************************************/
void MP12Convert16to2(unsigned short *sp, unsigned int *dp, unsigned int *ap,
    DevPoint size, DevPoint srcOffset, int srcRowBytes, DevPoint dstOffset,
    int dstRowBytes, DevPoint phase)
{
    unsigned int mask,smask,lmask,rmask,val,oldval;
    unsigned int data8,alpha8,data2,alpha2;
    unsigned int *brick,*brickwrap;
    int i,n,w,h,sinc,dinc;
    
    w = size.x;
    h = size.y;
    n = (2*dstOffset.x)&0x1f;
    smask = 0xc0000000 >> n;
    lmask = ~leftBitArray[n];
    rmask = ~rightBitArray[(n+w*2)&0x1f];
    
    sinc = (srcOffset.y)*(srcRowBytes>>1) + srcOffset.x;
    sp += sinc;
    sinc = srcRowBytes - w*2;
 
    dinc = (dstOffset.y)*(dstRowBytes>>2) + (dstOffset.x>>4);
    dp += dinc;
    if (ap) ap += dinc;
    dinc = dstRowBytes - (((n+w*2+31)>>5)<<2);

    /* Compute the 4->2 bit conversion table on first time through */
    if (!mpBrick) MP12InitBricks();

    /* Start our brick table at y offset */
    brick = mpBrick;
    n = (phase.x & 15) << 1;
    brickwrap = brick + 8;
    brick += (dstOffset.y - phase.y) & 7;

    if (!ap) { /* No alpha */
	do {
	    i = w;
	    data2 = *dp & lmask;
	    mask = smask;
	    val = *sp++;
	    do {
		/* Find scanline in brick and roll right with x offset */
		oldval = val;
		CONVERT16TO8(val,data8);
		data8 = (data8<<1)+(data8>>3);
		data8 = *(unsigned int *)((unsigned char *)brick + (data8<<5));
		RollRight(data8,n);
		do {
		    data2 |= data8 & mask;
		    if ((mask>>=2)==0) {*dp++=data2; data2=0; mask=0xc0000000;}
		} while(--i != 0 && (val = *sp++) == oldval);
	    } while(i != 0);

	    if (mask != 0xc0000000) { *dp = (*dp&rmask) | data2; dp++; }
    	    sp = (unsigned short *)((unsigned char *)sp + sinc);
	    dp = (unsigned int *)((unsigned char *)dp + dinc);
    	    if (++brick >= brickwrap) brick -= 8;
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
	        unsigned int alpha4;
		/* Find scanline in brick and roll right with x offset */
	        oldval = val;
		CONVERT16TO8(val,data8);
		data8 = (data8<<1)+(data8>>3);
		alpha4 = 15 - (val & MASKA34);	/* alpha4 = 15 - A */
		alpha8 = (alpha4<<4)|alpha4;	/* duff alpha to 8 bits */
		data8 += alpha8;		/* data8  = 255 - (A - D) */
		if (data8 > 255) data8 = 255;	/* In case of illegal data */
		data8  = *(unsigned int *)((unsigned char *)brick+(data8<<5));
		alpha8 = *(unsigned int *)((unsigned char *)brick+(alpha8<<5));
		RollRight(data8,n);
		RollRight(alpha8,n);
		do {
		    data2  |= data8  & mask;
		    alpha2 |= alpha8 & mask;

		    /* Shift mask, write buffers if end of longword */
		    if ((mask>>=2) == 0) {
			*dp++ = data2;  data2  = 0;
			*ap++ = alpha2; alpha2 = 0;
			mask = 0xc0000000;
		    }
		} while(--i != 0 && (val = *sp++) == oldval);
	    } while(i != 0);
    
	    /* Write out remains of last longword */
	    if (mask != 0xc0000000) {
		*dp = (*dp&rmask) | data2;  dp++;
		*ap = (*ap&rmask) | alpha2; ap++;
	    }
    
	    /* Increment source, destination, and brick pointers */
	    sp = (unsigned short *)((unsigned char *)sp + sinc);
	    dp = (unsigned int *)((unsigned char *)dp + dinc);
	    ap = (unsigned int *)((unsigned char *)ap + dinc);
    	    if (++brick >= brickwrap) brick -= 8;
	} while(--h != 0);
    }
}


/*****************************************************************************
    MP12Convert32to2
    Converts a portion of a 32 bit bitmap to a 2 bit bitmap
******************************************************************************/
void MP12Convert32to2(unsigned int *sp, unsigned int *dp, unsigned int *ap, DevPoint size,
		      DevPoint srcOffset, int srcRowBytes, 
		      DevPoint dstOffset, int dstRowBytes,
		      DevPoint phase)
{
    unsigned int mask,smask,lmask,rmask,val,oldval,data8,alpha8,data2,alpha2;
    unsigned int *brick,*brickwrap;
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
    if (ap) ap += dinc;
    dinc = dstRowBytes - (((n+w*2+31)>>5)<<2);

    /* Compute the 8->2 bit conversion table on first time through */
    if (!mpBrick) MP12InitBricks();

    /* Start our brick table at y offset */
    brick = mpBrick;
    n = (phase.x & 15) << 1;
    brickwrap = brick + 8;
    brick += (dstOffset.y - phase.y) & 7;

    if (!ap) { /* No alpha */
	do {
	    i = w;
	    data2 = *dp & lmask;
	    mask = smask;
	    val = *sp++;
	    do {
		/* Find scanline in brick and roll right with x offset */
		oldval = val;
		CONVERT32TO8(val,data8);
		data8 = *(unsigned int *)((unsigned char *)brick + (data8<<5));
		RollRight(data8,n);
		do {
		    data2 |= data8 & mask;
		    if ((mask>>=2)==0) {*dp++=data2; data2=0; mask=0xc0000000;}
		} while(--i != 0 && (val = *sp++) == oldval);
	    } while(i != 0);

	    if (mask != 0xc0000000) { *dp = (*dp&rmask) | data2; dp++; }
    	    sp = (unsigned int *)((unsigned char *)sp + sinc);
	    dp = (unsigned int *)((unsigned char *)dp + dinc);
    	    if (++brick >= brickwrap) brick -= 8;
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
		alpha8 = 255 - (val & MASKA38);	/* alpha8 = 255 - A */
		data8 += alpha8;		/* data8  = 255 - (A - D) */
		if (data8 > 255) data8 = 255;	/* In case of illegal data */
		data8  = *(unsigned int *)((unsigned char *)brick+(data8<<5));
		alpha8 = *(unsigned int *)((unsigned char *)brick+(alpha8<<5));
		RollRight(data8,n);
		RollRight(alpha8,n);
		do {
		    data2  |= data8  & mask;
		    alpha2 |= alpha8 & mask;

		    /* Shift mask, write buffers if end of longword */
		    if ((mask>>=2) == 0) {
			*dp++ = data2;  data2  = 0;
			*ap++ = alpha2; alpha2 = 0;
			mask = 0xc0000000;
		    }
		} while(--i != 0 && (val = *sp++) == oldval);
	    } while(i != 0);
    
	    /* Write out remains of last longword */
	    if (mask != 0xc0000000) {
		*dp = (*dp&rmask) | data2;  dp++;
		*ap = (*ap&rmask) | alpha2; ap++;
	    }
    
	    /* Increment source, destination, and brick pointers */
	    sp = (unsigned int *)((unsigned char *)sp + sinc);
	    dp = (unsigned int *)((unsigned char *)dp + dinc);
	    ap = (unsigned int *)((unsigned char *)ap + dinc);
    	    if (++brick >= brickwrap) brick -= 8;
	} while(--h != 0);
    }
}

