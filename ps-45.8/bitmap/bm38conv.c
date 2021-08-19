/*
    bmconv38.c
    32 bit conversion routines:
    	BM38Convert2to32
	BM38Convert8to32
	BM38Convert16to32

    Created 11Jul90 Ted Cohn
    
    Modifications:
    13Jul90 Ted   Added BM38Convert16to32 and BM38Convert8to32
    18Jul90 Ted   Rewrote BM38Convert2to32 to accept arbitrary area of interest
    26Jul90 Terry Made expand2to8 + expand2to32 const
    10Aug90 Ted   Bounds arguments were switched in BM38Convert16to32... sigh...

*/

#import PACKAGE_SPECS
#import "bitmap.h"
#import "bm38.h"

static const uint expand2to8[4]  = {0, 0x55, 0xAA, 0xFF};
static const uint expand2to32[4] = {0, 0x55555500, 0xAAAAAA00, 0xFFFFFF00};

/* 16 to 32 DUFF: 4 shifts, 4 ors, 5 ands, 13 total time units */
#define CONVERT16TO32(R,V)\
	R = (V&0xF)|((V<<4)&0xFF0)|((V<<8)&0xFF000)|((V<<12)&0xFF00000)|\
		((V<<16)&0xF0000000);

void BM38Convert2to32(LocalBitmap *dbm, LocalBitmap *sbm, Bounds db, Bounds sb)
{
    int w, h, i, lshift, shift, sinc, dinc;
    uint *osp, *oap, *sp, *ap, *dp, data2, alpha2;
    DevPoint srcOffset, dstOffset;
    
    w = db.maxx - db.minx;
    h = db.maxy - db.miny;
    srcOffset.x = sb.minx - sbm->base.bounds.minx;
    srcOffset.y = sb.miny - sbm->base.bounds.miny;
    dstOffset.x = db.minx - dbm->base.bounds.minx;
    dstOffset.y = db.miny - dbm->base.bounds.miny;
    
    lshift = (2*srcOffset.x)&0x1f;

    sinc = (srcOffset.y)*(sbm->rowBytes>>2) + (srcOffset.x>>4);
    osp = sp = sbm->bits + sinc;
    ap = sbm->abits;
    if (ap) ap += sinc;
    oap = ap;
    sinc = (sbm->rowBytes>>2);
    
    dinc = (dstOffset.y)*(dbm->rowBytes>>2) + dstOffset.x;
    dp = dbm->bits + dinc;
    dinc = dbm->rowBytes - w*4;
    
    lshift = 30-lshift;
    if (!ap) {
	do {
	    i = w;
	    data2 = *sp++;
	    shift = lshift;
	    do {
		*dp++ = expand2to32[(data2>>shift) & 0x3] | 0xFF;
		if ((shift -= 2) < 0) {shift=30; data2=*sp++;}
	    } while (--i > 0);
    	    sp = osp = osp + sinc;
	    dp = (uint *)((uchar *)dp + dinc);
	} while (--h != 0);
    } else {
	do {
	    i = w;
	    data2 = *sp++;
	    alpha2 = *ap++;
	    shift = lshift;
	    do {
		*dp++ = expand2to32[(data2>>shift) & 0x3] |
			expand2to8[(alpha2>>shift) & 0x3];
		if ((shift -= 2) < 0) {shift=30; data2=*sp++; alpha2=*ap++;}
	    } while (--i > 0);
	    sp = osp = osp + sinc;
	    ap = oap = oap + sinc;
	    dp = (uint *)((uchar *)dp + dinc);
	} while (--h != 0);
    }
}

void BM38Convert16to32(LocalBitmap *dbm, LocalBitmap *sbm, Bounds db, Bounds sb)
{
    unsigned int *dp, d, s;
    unsigned short *sp;
    int w, h, x, y, sinc, dinc;

    w = sb.maxx - sb.minx;
    h = sb.maxy - sb.miny;
    sp = (unsigned short *)sbm->bits;
    dp = dbm->bits;

    sinc = sbm->rowBytes>>1;
    sp += sb.minx-sbm->base.bounds.minx +
	  sinc*(sb.miny-sbm->base.bounds.miny);
    sinc -= w;

    dinc = dbm->rowBytes>>2;
    dp += db.minx - dbm->base.bounds.minx +
	  dinc*(db.miny - dbm->base.bounds.miny);
    dinc -= w;

    for (y = h; --y>=0; ) {
	for (x = w; --x>=0; ) {
	    s = (unsigned int) *sp++;
	    CONVERT16TO32(d,s);
	    *dp++ = d;
	}
	sp += sinc;
	dp += dinc;
    }
}

void BM38Convert8to32(LocalBitmap *dbm, LocalBitmap *sbm, Bounds db, Bounds sb)
{
    unsigned int *dp;
    unsigned char *sp, *ap, s;
    int w, h, x, y, sinc, dinc;

    w = sb.maxx - sb.minx;
    h = sb.maxy - sb.miny;
    sp = (unsigned char *)sbm->bits;
    ap = (unsigned char *)sbm->abits;
    dp = dbm->bits;

    sinc = sbm->rowBytes;
    x = sb.minx - sbm->base.bounds.minx +
	  sinc*(sb.miny - sbm->base.bounds.miny);
    sp += x;
    if (ap) ap += x;
    sinc -= w;

    dinc = dbm->rowBytes>>2;
    dp += sb.minx - dbm->base.bounds.minx +
	  dinc*(db.miny - dbm->base.bounds.miny);
    dinc -= w;

    if (ap) {
	for (y = h; --y>=0; ) {
	    for (x = w; --x>=0; ) {
		s = *sp++;
		*dp++ = (s<<24)|(s<<16)|(s<<8)|(*ap++);
	    }
	    ap += sinc;
	    sp += sinc;
	    dp += dinc;
	}
    } else {
	for (y = h; --y>=0; y) {
	    for (x = w; --x>=0; ) {
		s = *sp++;
		*dp++ = (s<<24)|(s<<16)|(s<<8)|0xFF;
	    }
	    sp += sinc;
	    dp += dinc;
	}
    }
}
