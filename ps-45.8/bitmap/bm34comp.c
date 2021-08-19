/*****************************************************************************

	bmcomp34.c
	Low-level 16-bit rect compositing code.

	CONFIDENTIAL
	Copyright (c) 1988 NeXT, Inc. as an unpublished work.
	All Rights Reserved.

	Created: 20Oct89 Ted Cohn

	Modified:
	7/10 pgraff taken from ND composite32
	7/18 mpaque Quickly hacked from bmcomp38.
	7/21 pgraff Sped up COPY compositing.

******************************************************************************/

#import PACKAGE_SPECS
#import BITMAP
#import "bm34.h"

#define ALPHA(s)	((s) & AMASK)
#define NEGALPHA(s)	((~(s)) & AMASK)
#define COLOR(s)	((s) & RGBMASK)

#define MUL(a,b) ((((((a) & RBMASK)>>4)*(b) + GAMASK) & RBMASK) | \
		 (((((a) & GAMASK)*(b)+GAMASK)>>4) & GAMASK))

/* a slightly better version than nextdev/td.h */
#define LOOP32(nwords, op) { \
        register short n = (nwords)>>5; \
        switch ((nwords)&31) { \
        case 0:  while (--n != -1){ op; \
        case 31: op; case 30: op; case 29: op; case 28: op; case 27: op; \
        case 26: op; case 25: op; case 24: op; case 23: op; case 22: op; \
        case 21: op; case 20: op; case 19: op; case 18: op; case 17: op; \
        case 16: op; case 15: op; case 14: op; case 13: op; case 12: op; \
        case 11: op; case 10: op; case 9: op; case 8: op; case 7: op; \
        case 6: op; case 5: op; case 4: op; case 3: op; case 2: op; \
        case 1: op; \
                } \
        } \
}

static pixel_t ADDC(unsigned int a, unsigned int b)
{
    pixel_t rb, ga, rbmask=RBMASK, gamask=GAMASK;
    rb = ((a & rbmask)>>4) + ((b & rbmask)>>4);
    ga = (a & gamask) + (b & gamask);
    a = (rb & rbmask) | ((ga & rbmask)>>4);
    a |= a<<1; a |= a<<2;
    return (rb<<4) | ga | a;
}

#define isAligned(ptr) (!((int)ptr&3))
#define doShort(s,d) *d++ = *s++
/* Believe it or not... GCC generates movel a0@+,a1@+ for the following: */
#define doLong(s,d)  *((int *)d) = *((int *)s), s+=2, d+=2
#define doSkip(p,rb)  p = (pixel_t *)((char *)p + rb)

//#define SwapMoveLow(output, sw_input) asm("swap %1": "=d" (

BM34MoveRect(pixel_t *sp,int srb,pixel_t *dp,int drb,
	     int width,int dy,int ltor)
{
    static volatile int which = 3;
    /* debugging :
       which
       0	old code
       1	new code for aligned cases
       2	aligned code for all cases (unaligned memory access)
       3	all aligned memory access
       */
       
    if(width == 0 || dy == 0)
	return(1);
    if(which==0)
	return(0);
    /* get real byte advance */
    drb <<= 1;
    srb <<= 1;
    if(ltor) {
	if((isAligned(sp) == isAligned(dp)) || which == 2) {
	    /* long word aligned */
	    int intw = isAligned(sp) ? (width >> 1) :  (--width >> 1);
	    for(;--dy>=0; doSkip(dp,drb), doSkip(sp,srb)) {
		int dx;
		if(!isAligned(sp))
		    doShort(sp,dp);
		LOOP32(intw, doLong(sp,dp));
		if(width & 1)
		    doShort(sp,dp);
	    }
	    return(1);

	} else if (which == 3) {
#define HIWORD(a) ((unsigned short)((a) >> 16))
#define LOWORD(a) ((unsigned short)((a)&0xffff))
#if __GNUC__ && ISP == isp_mc68020 
#define SWAP(a) asm("swap %0" : "=d" (a) : "0d" (a))
#define ASSIGNLO(a,b) asm("movew %2,%0" : "=r" (a) : "0r" (a) , "r" (b))
#define STOREIT(p,v)  asm("movel %2, %0@+" : "=a" (p) : "0a" (p) , "r" (v))
#else
#define SWAP(a)   (((a) << 16) | ((a) >> 16))
#define ASSIGNLO(a,b)  a = ((a)&0xffff0000) | ((b) & 0x0000ffff)
#endif
	    unsigned int last, next;

	    int intw = isAligned(dp) ? (width >> 1) :  (--width >> 1);
	    for(;--dy>=0; doSkip(dp,drb), doSkip(sp,srb)) {
		short dx;
		if(isAligned(dp)) {
		    last = *sp++;
		    SWAP(last);
		} else {
		    last = *(unsigned int *)sp, sp +=2;
		    SWAP(last);
		    *dp++ = last;
		}
		for(dx = intw; --dx != -1;) {
		    next = *(unsigned int *)sp, sp +=2;
		    SWAP(next);
		    ASSIGNLO(last,next);
		    STOREIT(dp,last);
		    last = next;
		}
		if(width & 1) 
		    *dp++ = HIWORD(last);
		else
		    sp--;
	    }
	    return(1);
	}
    }
    return(0);
}

void BMComposite34(RectOp *ro)
{
    int dy, dx, drb, srb, stype, width, si, di;
    volatile pixel_t *dp, *sp;
    pixel_t d, s, mask;

    if ((stype = ro->srcType) == SRCCONSTANT) {
	s = ro->srcValue;
    } else {
	sp = (pixel_t *)ro->srcPtr;
	srb = ro->srcRowBytes;
	si = ro->srcInc;
    }
    dy = ro->height;
    drb = ro->dstRowBytes;	// Really in terms of pixels
    dp = (pixel_t *)ro->dstPtr;
    di = ro->dstInc;
    width = ro->width;
    mask = ro->mask;

    switch(ro->op) {

    	case CLEAR:	/* zero */
	    s = 0;
	    stype = SRCCONSTANT;
	    /* Fall through to COPY */
	case COPY:
	    if (stype == SRCBITMAP) {
		if (mask) {
		    if (ro->ltor) {
			for (; dy>0; dy--, dp+=drb, sp+=srb)
			    for (dx = width; dx>0; dx--)
				*dp++ = ((*sp++) & mask) | (*dp & (~mask));
		    } else {
			for (; dy>0; dy--, dp+=drb, sp+=srb)
			    for (dx = width; dx>0; dx--)
				*dp-- = ((*sp--) & mask) | (*dp & (~mask));
		    }
		} else {
#if 1
		    if(BM34MoveRect(sp,srb,dp,drb,width,dy,ro->ltor))
			return;
#endif

		    if (ro->ltor) {
			for (; dy>0; dy--, dp+=drb, sp+=srb)
			    for (dx = width; dx>0; dx--)
				*dp++ = *sp++;
		    } else {
			for (; dy>0; dy--, dp+=drb, sp+=srb)
			    for (dx = width; dx>0; dx--)
				*dp-- = *sp--;
		    }
		}
	    } else {	/* constant */
		    if (mask) {
			s &= mask;
			for (; dy>0; dy--, dp+=drb)
			    for (dx = width; dx>0; dx--)
				*dp++ = s | (*dp & (~mask));
		    } else
			for (; dy>0; dy--, dp+=drb)
			    /*for (dx = width; dx>0; dx--)
				*dp++ = s;*/
			    LOOP32(width, *dp++ = s);
		}
	    return;
	case SOVER:	/* dst' = src + dst*(1-srcA) */
	{
	    register unsigned int sa;
	    if (stype == SRCBITMAP)
		for (; dy>0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx>0; dx--, dp+=di, sp+=si) {
			s = *sp;
			sa = NEGALPHA(s);
			d = *dp;
			*dp = s + MUL(d, sa);
		    }
	    else {
		sa = NEGALPHA(s);
		for (; dy>0; dy--, dp+=drb)
		    for (dx = width; dx>0; dx--) {
			d = *dp;
		        *dp++ = s + MUL(d, sa);
		    }
	    }
	    return;
	}
	case SIN:	/* dst' = src * dstA */
	{
	    register pixel_t da;
	    if (stype == SRCBITMAP)
		for (; dy>0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx>0; dx--, dp+=di, sp+=si) {
			s = *sp;
			da = ALPHA(*dp);
		        *dp = MUL(s, da);
		    }
	    else
		for (; dy>0; dy--, dp+=drb)
		    for (dx = width; dx>0; dx--)
		    {
			da = ALPHA(*dp);
			*dp++ = MUL(s, da);
		    }
	    return;
	}
	case SOUT:	/* dst' = src * (1-dstA) */
	{
	    register pixel_t da;
	    if (stype == SRCBITMAP)
		for (; dy>0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx>0; dx--, dp+=di, sp+=si) {
			s = *sp;
			da = NEGALPHA(*dp);
			*dp = MUL(s, da);
		    }
	    else
		for (; dy>0; dy--, dp+=drb)
		    for (dx = width; dx>0; dx--) {
			da = NEGALPHA(*dp);
			*dp++ = MUL(s, da);
		    }
	    return;
	}
	case SATOP:	/* dst' = src*dstA + dst*(1-srcA) */
	{
	    register pixel_t sa, da;
	    if (stype == SRCBITMAP)
		for (; dy>0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx>0; dx--, dp+=di, sp+=si) {
			s = *sp;
			sa = NEGALPHA(s);
			d = *dp;
			da = ALPHA(d);
			*dp = MUL(s, da) + MUL(d, sa);
		    }
	    else {
		sa = NEGALPHA(s);
		for (; dy>0; dy--, dp+=drb)
			for (dx = width; dx>0; dx--) {
			d = *dp;
			da = ALPHA(d);
			*dp++ = MUL(s, da) + MUL(d, sa);
		    }
	    }
	    return;
	}
	case DOVER:	/* dst' = src*(1-dstA) + dst */
	{
	    register pixel_t da;
	    if (stype == SRCBITMAP)
		for (; dy>0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx>0; dx--, dp+=di, sp+=si) {
			s = *sp;
			d = *dp;
			da = NEGALPHA(d);
			*dp = d + MUL(s, da);
		    }
	    else 
		for (; dy>0; dy--, dp+=drb)
		    for (dx = width; dx>0; dx--) {
			d = *dp;
			da = NEGALPHA(d);
			*dp++ = d + MUL(s, da);
		    }
	    return;
	}
	case DIN:	/* dst' = dst * srcA */
	{
	    register pixel_t sa;
	    if (stype == SRCBITMAP)
		for (; dy>0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx>0; dx--, dp+=di, sp+=si) {
			sa = ALPHA(*sp);
			d = *dp;
			*dp = MUL(d, sa);
		    }
	    else {
		sa = ALPHA(s);
		for (; dy>0; dy--, dp+=drb)
		    for (dx = width; dx>0; dx--) {
			d = *dp;
			*dp++ = MUL(d, sa);
		    }
	    }
	    return;
	}
	case DOUT:	/* dst' = dst*(1-srcA) */
	{
	    register pixel_t sa;
	    if (stype == SRCBITMAP)
		for (; dy>0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx>0; dx--, dp+=di, sp+=si) {
			sa = NEGALPHA(*sp);
			d = *dp;
			*dp = MUL(d, sa);
		    }
	    else {
		sa = NEGALPHA(s);
		for (; dy>0; dy--, dp+=drb)
		    for (dx = width; dx>0; dx--) {
			d = *dp;
			*dp++ = MUL(d, sa);
		    }
	    }
	    return;
	}
	case DATOP: {	/* dst' = src*(1-dstA) + dst*srcA */
	    register pixel_t sa, da;
	    if (stype == SRCBITMAP)
		for (; dy>0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx>0; dx--, dp+=di, sp+=si) {
			s = *sp;
			sa = ALPHA(s);
			d = *dp;
			da = NEGALPHA(d);
			*dp = MUL(d, sa) + MUL(s, da);
		    }
	    else {
		sa = ALPHA(s);
		for (; dy>0; dy--, dp+=drb)
		    for (dx = width; dx>0; dx--) {
			d = *dp;
			da = NEGALPHA(d);
			*dp++ = MUL(d, sa) + MUL(s, da);
		    }
	    }
	    return;
	}
	case XOR:	/* dst' = src*(1-dstA) + dst*(1-srcA) */
	{
	    register pixel_t sa, da;
	    if (stype == SRCBITMAP)
		for (; dy>0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx>0; dx--, dp+=di, sp+=si) {
			s = *sp;
			sa = NEGALPHA(s);
			d = *dp;
			da = NEGALPHA(d);
			*dp = MUL(d, sa) + MUL(s, da);
		    }
	    else {
		sa = NEGALPHA(s);
		for (; dy>0; dy--, dp+=drb)
		    for (dx = width; dx>0; dx--) {
			d = *dp;
			da = NEGALPHA(d);
			*dp++ = MUL(d, sa) + MUL(s, da);
		    }
	    }
	    return;
	}
	case PLUSD:	/* dstD' = 1-((1-dstD)+^(1-srcD)) */
	    if (stype == SRCBITMAP)
		for (; dy>0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx>0; dx--, dp+=di, sp+=si) {
			s = *sp;
			s = COLOR(~s) | ALPHA(s);
			d = *dp;
			d = COLOR(~d) | ALPHA(d);
			d = ADDC(s, d);
			*dp = COLOR(~d) | ALPHA(d);
		    }
	    else {
		s = COLOR(~s) | ALPHA(s);
		for (; dy>0; dy--, dp+=drb)
		    for (dx = width; dx>0; dx--) {
			d = *dp;
			d = COLOR(~d) | ALPHA(d);
			d = ADDC(s, d);
			*dp++ = COLOR(~d) | ALPHA(d);
		    }
	    }
	    return;
	case HIGHLIGHT:	/* swap white and lightgray */
	    for (; dy>0; dy--, dp+=drb)
		for (dx = width; dx>0; dx--) {
		    d = COLOR(*dp);
		    if (d == RGB_WHITE)
			*dp++ = RGB_LTGRAY;
		    else if (d == RGB_LTGRAY)
			*dp++ = RGB_WHITE;
		    else dp++;
		}
	    return;
	case DISSOLVE: {	/* dst' = src*f + dst*(1-f) */
	    register pixel_t fn, f;
	    f = ALPHA(ro->delta);
	    fn = NEGALPHA(f);
	    if (ro->ltor) {
		for (; dy>0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx>0; dx--) {
			s = *sp++;
			d = *dp;
			*dp++ = MUL(s, f) + MUL(d, fn);
		    }
	    } else {
		for (; dy>0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx>0; dx--) {
			s = *sp--;
			d = *dp;
			*dp-- = MUL(s, f) + MUL(d, fn);
		    }
	    }
	    return;
	}
	case PLUSL: {	/* dstD' = dstD +^ srcD */
	    if (stype == SRCBITMAP)
		for (; dy>0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx>0; dx--, dp+=di, sp+=si) {
			s = *sp;
			d = *dp;
			*dp = ADDC(s, d);
		    }
	    else
		for (; dy>0; dy--, dp+=drb)
		    for (dx = width; dx>0; dx--) {
			d = *dp;
			*dp++ = ADDC(s, d);
		    }
	    return;
	}
	default:
	    CantHappen();
    }
}



