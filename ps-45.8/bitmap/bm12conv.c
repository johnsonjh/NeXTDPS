/*****************************************************************************

    bmconv12.c
    Converts subrectangles of bitmaps from four standard depths to 2 bpp.

    CONFIDENTIAL
    Copyright (c) 1989 NeXT, Inc. as an unpublished work.
    All Rights Reserved.
	
    Created: 12Jul90 Ted Cohn from mp/convert.c

    Modifications:

******************************************************************************/

#import PACKAGE_SPECS
#import DEVICE
#import DEVPATTERN
#import BINTREE
#import BITMAP

/* From mpdev.c */
extern PatternHandle mpPattern;
extern DevHalftone mpDevHalftone;

/* gray = .3R + .59G + .11B */

#if 1
/* 3 ands, 5 shifts, 3 adds, 16 total time units */
#define CONVERT16TO8(V,R) \
  R = (((V&MASKR)>>11) + ((V&MASKG)>>6)+((V&MASKG)>>8) + ((V&MASKB)>>4))
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

static uint *mpBrick;

static void BM12InitBrick()
{
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
	    ((DevColorVal *)&mi.color)->white = i
	    Gry4Of4Setup(mpPattern, &mi, &data);
	    sp = (uint *)data.start;
	    n = 8;
	    if (data.constant) while(--n >= 0) *dp++ = data.value;
	    else	       while(--n >= 0) *dp++ = *sp++;
	}
}

/*****************************************************************************
    BM12Convert8to2
    Converts a portion of an 8-bit bitmap to a 2-bit bitmap
    sp: src bits pointer, tp: src abits pointer
    dp: dst bits pointer, ap: dst abits pointer
******************************************************************************/
void BM12Convert8to2(uchar *sp, uchar *tp, uint *dp, uint *ap, DevPoint size,
		    DevPoint srcOffset, int srcRowBytes, 
		    DevPoint dstOffset, int dstRowBytes,
		    DevPoint phase)
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
    
    sinc = (srcOffset.y)*(srcRowBytes) + srcOffset.x;
    sp += sinc;
    sinc = srcRowBytes - w;
 
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
    n = (phase.x & 15) << 1;
    brickwrap = brick + 8;
    brick += (dstOffset.y - phase.y) & 7;

    if(!ap) { /* No alpha */
	do {
	    i = w;
	    data2 = *dp & lmask;
	    mask = smask;
	    val = *sp++;
	    do {
		/* Find scanline in brick and roll right with x offset */
		oldval = val;
		data8 = *(uint *)((uchar *)brick + (val<<5));
		RollRight(data8,n);
		do {
		    data2 |= data8 & mask;
		    if((mask>>=2)==0) {*dp++=data2; data2=0; mask=0xc0000000;}
		} while(--i != 0 && (val = *sp++) == oldval);
	    } while(i != 0);

	    if(mask != 0xc0000000) { *dp = (*dp&rmask) | data2; dp++; }
    	    sp += sinc;
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
	    aval = *tp++;
	    do {
		/* Find scanline in brick and roll right with x offset */
		oldaval = aval;
	        oldval = val;
		alpha8 = ~aval;			/* alpha8 = 255 - A */
		data8 = val + alpha8;		/* data8  = 255 - (A - D) */
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
		} while(--i != 0 && (val = *sp++) == oldval &&
			(aval = *tp++) == oldaval);
	    } while(i != 0);
    
	    /* Write out remains of last longword */
	    if(mask != 0xc0000000) {
		*dp = (*dp&rmask) | data2;  dp++;
		*ap = (*ap&rmask) | alpha2; ap++;
	    }
    
	    /* Increment source, destination, and brick pointers */
	    sp += sinc;
	    tp += sinc;
	    dp = (uint *)((uchar *)dp + dinc);
	    ap = (uint *)((uchar *)ap + dinc);
    	    if(++brick >= brickwrap) brick -= 8;
	} while(--h != 0);
    }
}

/*****************************************************************************
    BM12Convert16to2
    Converts a portion of a 16 bit bitmap to a 2 bit bitmap
******************************************************************************/
void BM12Convert16to2(unsigned short *sp, uint *dp, uint *ap, DevPoint size,
		    DevPoint srcOffset, int srcRowBytes, 
		    DevPoint dstOffset, int dstRowBytes,
		    DevPoint phase)
{
    uint mask,smask,lmask,rmask,val,oldval;
    uint data8,alpha8,data4,alpha4,data2,alpha2;
    uint *brick,*brickwrap;
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
    if(ap) ap += dinc;
    dinc = dstRowBytes - (((n+w*2+31)>>5)<<2);

    /* Compute the 4->2 bit conversion table on first time through */
    if (!mpBrick) BM12InitBrick();

    /* Start our brick table at y offset */
    brick = mpBrick;
    n = (phase.x & 15) << 1;
    brickwrap = brick + 8;
    brick += (dstOffset.y - phase.y) & 7;

    if(!ap) { /* No alpha */
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
		data8 = *(uint *)((uchar *)brick + (data8<<5));
		RollRight(data8,n);
		do {
		    data2 |= data8 & mask;
		    if((mask>>=2)==0) {*dp++=data2; data2=0; mask=0xc0000000;}
		} while(--i != 0 && (val = *sp++) == oldval);
	    } while(i != 0);

	    if(mask != 0xc0000000) { *dp = (*dp&rmask) | data2; dp++; }
    	    sp = (unsigned short *)((uchar *)sp + sinc);
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
		CONVERT16TO4(val,data4);
		data8 = (data4<<4)|data4;
		alpha4 = 15 - (val & MASKA);	/* alpha4 = 15 - A */
		alpha8 = (alpha4<<4)|alpha4;	/* duff alpha to 8 bits */
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
	    sp = (unsigned short *)((uchar *)sp + sinc);
	    dp = (uint *)((uchar *)dp + dinc);
	    ap = (uint *)((uchar *)ap + dinc);
    	    if(++brick >= brickwrap) brick -= 8;
	} while(--h != 0);
    }
}

/*****************************************************************************
    BM12Convert32to2
    Converts a portion of a 32 bit bitmap to a 2 bit bitmap
******************************************************************************/
void BM12Convert32to2(uint *sp, uint *dp, uint *ap, DevPoint size,
		    DevPoint srcOffset, int srcRowBytes, 
		    DevPoint dstOffset, int dstRowBytes,
		    DevPoint phase)
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
    n = (phase.x & 15) << 1;
    brickwrap = brick + 8;
    brick += (dstOffset.y - phase.y) & 7;

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

