
#import PACKAGE_SPECS
#import BITMAP

static unsigned char expand2to4[4]   = {0x0, 0x5, 0xA, 0xF};
static unsigned short expand2to16[4] = {0x000, 0x5550, 0xAAA0, 0xFFF0};

void BM34Convert2to16(LocalBitmap *dbm, LocalBitmap *sbm, Bounds db, Bounds sb)
{
    unsigned short *dp;
    DevPoint srcOffset, dstOffset;
    int w, h, i, lshift, shift, sinc, dinc;
    unsigned int *osp, *oap, *sp, *ap, data2, alpha2;
    
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
    
    dinc = (dstOffset.y)*(dbm->rowBytes>>1) + dstOffset.x;
    dp = (unsigned short *)dbm->bits + dinc;
    dinc = dbm->rowBytes - w*2;
    
    lshift = 30-lshift;
    if (!ap) {
	do {
	    i = w;
	    data2 = *sp++;
	    shift = lshift;
	    do {
		*dp++ = expand2to16[(data2>>shift) & 0x3] | 0xF;
		if ((shift -= 2) < 0) {shift=30; data2=*sp++;}
	    } while (--i > 0);
    	    sp = osp = osp + sinc;
	    dp = (unsigned short *)((uchar *)dp + dinc);
	} while (--h != 0);
    } else {
	do {
	    i = w;
	    data2 = *sp++;
	    alpha2 = *ap++;
	    shift = lshift;
	    do {
		*dp++ = expand2to16[(data2>>shift) & 0x3] |
			expand2to4[(alpha2>>shift) & 0x3];
		if ((shift -= 2) < 0) {shift=30; data2=*sp++; alpha2=*ap++;}
	    } while (--i > 0);
	    sp = osp = osp + sinc;
	    ap = oap = oap + sinc;
	    dp = (unsigned short *)((uchar *)dp + dinc);
	} while (--h != 0);
    }
}

void BM34Convert32to16(LocalBitmap *dbm, LocalBitmap *sbm, Bounds db,
    Bounds sb)
{
    unsigned short *dp;
    unsigned int s, *sp;
    int w, h, x, y, sinc, dinc;

    w = sb.maxx - sb.minx;
    h = sb.maxy - sb.miny;
    sp = (unsigned int *)sbm->bits;
    dp = (unsigned short *)dbm->bits;

    sinc = sbm->rowBytes >> 2;	// Divide by sizeof (unsigned int)
    sp += sb.minx-sbm->base.bounds.minx +
	  sinc*(sb.miny-sbm->base.bounds.miny);
    sinc -= w;

    dinc = dbm->rowBytes >> 1;	// Divide by sizeof (unsigned short)
    dp += db.minx - dbm->base.bounds.minx +
	  dinc*(db.miny - dbm->base.bounds.miny);
    dinc -= w;

    // The following should be dithered since it represents a loss of precision
    for (y = h; --y>=0; ) {
	for (x = w; --x>=0; ) {
	    s = *sp++;
	    *dp++ = (s >> 4)  & 0xf | 
		    (s >> 8)  & 0xf0 |
		    (s >> 12) & 0xf00 |
		    (s >> 16) & 0xf000;
	}
	sp += sinc;
	dp += dinc;
    }
}

void BM34Convert8to16(LocalBitmap *dbm, LocalBitmap *sbm, Bounds db, Bounds sb)
{
    unsigned short *dp;
    unsigned char *sp, *ap, s;
    int w, h, x, y, sinc, dinc;

    w = sb.maxx - sb.minx;
    h = sb.maxy - sb.miny;
    sp = (unsigned char *)sbm->bits;
    ap = (unsigned char *)sbm->abits;
    dp = (unsigned short *)dbm->bits;

    sinc = sbm->rowBytes;
    x = sb.minx - sbm->base.bounds.minx +
	  sinc*(sb.miny - sbm->base.bounds.miny);
    sp += x;
    if (ap) ap += x;
    sinc -= w;

    dinc = dbm->rowBytes >> 1;	// Divide by sizeof (unsigned short)
    dp += sb.minx - dbm->base.bounds.minx +
	  dinc*(db.miny - dbm->base.bounds.miny);
    dinc -= w;

    // The following should be dithered, as it represents a loss of precision
    if (ap) {
	for (y = h; y>=0; y--) {
	    for (x = w; x>=0; x--) {
		s = (*sp++) >> 4 ;
		*dp++ = (s<<12)|(s<<8)|(s<<4) | ((*ap++) >> 4);
	    }
	    ap += sinc;
	    sp += sinc;
	    dp += dinc;
	}
    } else {
	for (y = h; y>=0; y--) {
	    for (x = w; x>=0; x--) {
		s = (*sp++) >> 4 ;
		*dp++ = (s<<12)|(s<<8)|(s<<4) | 0xF;
	    }
	    sp += sinc;
	    dp += dinc;
	}
    }
}


