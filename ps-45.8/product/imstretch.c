/*****************************************************************************
    imstretch.c 
    
    Copyright (c) 1988 NeXT Incorporated.
    All rights reserved.

    Edit History:
	29Sep88 Jack 	created from imident.c
	24Jan89 Jack 	added "- 1" to ymax computation
	15Jun89 Jack 	fix bogus test at "next source word needed"
	18Apr90 Terry 	Optimize code size (2088 to 1852 bytes)
	18Apr90 Terry 	Speed up and fix table generation
	15May90 Terry 	Fixed problems with 1->1 shrinking cases
	05Oct90 Terry 	Fixed problems with extra pixel being drawn in dest
	
*****************************************************************************/

#import PACKAGE_SPECS
#import DEVICE
#import EXCEPT		/* for DebugAssert */
#import DEVPATTERN
#import DEVIMAGE

#define uint unsigned int
#define uchar unsigned char

#if __GNUC__ && ISP == isp_mc68020 && !defined(i860)
#if DEVICE_CONSISTENT
#define rotate(data,left) asm("roll %2,%0": "=d" (data): "0d" (data),"d"(left))
#else
#define rotate(data,left) asm("rolr %2,%0": "=d" (data): "0d" (data),"d"(left))
#endif
#else
#define rotate(d,n) d = (d LSHIFT n) | (d RSHIFT (32-n))
#endif

/* Variable declarations from framedev.h in device package */
extern integer framebytewidth;
extern PSCANTYPE framebase;
extern Card8 framelog2BD;

extern PImageProcs fmImageProcs;

 /* No rotation, No transfer function, 1 bpp -> 1 bpp, 1 bpp -> 2 bpp,
  * and 1 bpp-> 2 bpp.
  * 3 stages of data movement - (sc) stretch characters, building a
  * library of 256 possible enlargments of real source bytes,  (bl) build line,
  * from all the bytes in one source scanline by assembling sc's at final
  * destination alignment, (ul) use line, trimming it into the final dest,
  * for possibly many destination lines.
  */
public procedure ImStretch(int items, DevTrap *t, DevRun *run, ImageArgs *args)
{
    DevMarkInfo *info = args->markInfo;
    DevImage *image = args->image;
    DevImageSource *source = image->source;
    uint *destbase, *buffer, *bufBase, lMask, rMask, *stchBytes;
    uchar *srcByteBase;
    DevShort *buffptr;
    int unitl, numInts, y, ylast, pairs, xoffset, xl, xr, xSrcOffset;
    boolean leftSlope, rightSlope, spanChange;
    int rShiftInit, pixPerChar,
        oldSrcY, newSrcY, srcY, wPerChar, wPerLine, ymax, xmin, xmax; 
    Fixed sy, dxByte, dy, fx, ySrcOffset, tgix, tlix;
    DevBounds *sb = &image->info.sourcebounds;
    PMtx imtx = image->info.mtx;
    uint *dp, m0, m1, m2, m3, m35, d, word[8], mask[8], lastmask;
    int i, j, w, sh, rot, screenh, nFields;
    static uint color[4] = { 0x00000000, 0x55555555, 0xaaaaaaaa, 0xffffffff};
    real temp;

    /* If one of these fails, it may increase my understanding */  
    DebugAssert(imtx->b == 0.0 && imtx->c == 0.0
		&& imtx->a > 0.0 && imtx->d > 0.0
		&& (source->bitspersample != 4)
		&& (args->bitsPerPixel == 1 || args->bitsPerPixel == 2)
		&& args->procs == fmImageProcs);
    DebugAssert((sb->x.l == image->info.sourceorigin.x) &&
		(sb->y.l == image->info.sourceorigin.y) &&
		(sb->x.l == 0));
    
    /* dy is # dest scanlines per source scanline (in fixed point) */
    temp = 1/imtx->d;
    dy = pflttofix(&temp);

    /* dxByte is # of dest bits per source byte (in fixed point) */
    temp = 8/imtx->a;
    dxByte = pflttofix(&temp);
    if(source->bitspersample == 1 && args->bitsPerPixel == 2)
        dxByte += dxByte;

    /* pixPerChar is ceiling of # of dest pixels per source byte */
    pixPerChar = IntPart((dxByte>>framelog2BD) + ONE - 1);
    if (pixPerChar == 0) return;
    
    /* wPerChar is # of dest longs per source byte */
    wPerChar = ((pixPerChar<<framelog2BD) + 31) >> 5;

    /* Image space fixed point vector for unit moves in device */
    xoffset = info->offset.x;
    sy = pflttofix(&imtx->d);
    ySrcOffset = pflttofix(&imtx->ty) + sy/2 - Fix(sb->y.l);
    xSrcOffset = (int)((imtx->tx / imtx->a) - .5) - xoffset;
    xSrcOffset <<= framelog2BD; /* 2 above in pixels, now xSrcOffset is bits */
    rShiftInit = SCANMASK & -xSrcOffset;
    ymax = (Fix(sb->y.g - sb->y.l) - ySrcOffset - 1) / sy;
    xmin = -xSrcOffset;
    xmax = (int)(((sb->x.g - imtx->tx) / imtx->a) + .5);
    xmax = (xmax + xoffset) << framelog2BD;

    /* Create table of all possible source bytes, containing stretched dest */
    if(wPerChar == 1 && source->bitspersample == 2 && framelog2BD == 1) {
	dp = stchBytes = (uint *) malloc(256 * sizeof(int));
	d = (pixPerChar>>2)<<1;
	m3 = ~((uint)0xffffffff >> (pixPerChar*2));
	m2 = m3 << d; if (pixPerChar&3) m2 <<= 2;
	m1 = m2 << d; if ((pixPerChar&3) == 3) m1 <<= 2;
	m0 = ~((uint)0xffffffff >> d);
	m3 ^= m2; m2 ^= m1; m1 ^= m0; m35 = m3 & 0x55555555;
	for (i = 15; i >= 0; i--) {
	    d = (m0&color[i>>2]) | (m1&color[i&3]);
	    for (j = 1; j >= 0; --j) {
		d ^= m2;
		*dp++=(d^=m3); *dp++=(d^=m35); *dp++=(d^=m3); *dp++=(d^=m35);
		d ^= m2&0x55555555;
		*dp++=(d^=m3); *dp++=(d^=m35); *dp++=(d^=m3); *dp++=(d^=m35);
	    }
	}
    }
    else {
	/* nFields is number of fields in a source byte
	 * j  is field number
	 * d  is width of field in pixels (rounded down)
	 * m0 is pixel offset of right edge of current field
	 * m1 is a bresenham variable for determining which fields to extend
	 * word[j] is index of longword of right edge of field j
	 * mask[j] is mask for word[j] for right edge of field j
	 */
	dp = stchBytes = (uint *) calloc(256 * wPerChar * sizeof(int), 1);
	if (source->bitspersample == 1) {
	    nFields = 8;
	    d = pixPerChar>>3;
	}
	else {
	    nFields = 4;
	    d = pixPerChar>>2;
	}
	for (j = 0, m0 = d, m1 = pixPerChar&(nFields-1); j < nFields;
	     j++, m0 += d, m1 += pixPerChar&(nFields-1)) {
	    if (m1 >= nFields) { m1 -= nFields; m0++; }
	    word[j] = (m0<<framelog2BD) >> SCANSHIFT;
	    mask[j] = rightBitArray[(m0<<framelog2BD)&SCANMASK];
	}
        if (mask[nFields-1] == 0) {
	    mask[nFields-1] = 0xffffffff;
	    word[nFields-1]--;
	}

	if (source->bitspersample == 2)
	    for (i = 255; i >= 0; i--) {
		for (m0 = 0xffffffff, w=j=0, sh = 6; j < nFields; j++, sh-=2) {
		    d = color[(i>>sh)&3];
		    if (w == word[j])
			dp[w] |= d & m0 & mask[j];
		    else {
			dp[w++] |= d & m0;
			while (w < word[j]) dp[w++] = d;
			dp[w] = d & mask[j];
		    }
		    m0 = ~mask[j];
		}
		dp += w+1;
	    }
	else
	    for (i = 255; i >= 0; i--) {
	    	sh = i << 23;
		for (m0 = 0xffffffff, w=j=0; j < nFields; j++) {
		    if ((sh += sh) < 0)
			if (w == word[j])
			    dp[w] |= m0 & mask[j];
			else {
			    dp[w++] |= m0;
			    while (w < word[j]) dp[w++] = 0xffffffff;
			    dp[w] = mask[j];
			}
		    w  = word[j];
		    m0 = ~mask[j];
		}
		dp += w+1;
	    }
    }

    /* Create buffer for a single stretched line */
    wPerLine = ((source->wbytes * pixPerChar) >> (SCANSHIFT-framelog2BD)) + 3;
    buffer = (uint *) malloc(wPerLine * sizeof(int));
    bufBase = buffer + ((xSrcOffset + rShiftInit) >> SCANSHIFT);

    /* Mask for the pixel after the last pixel in every source scanline.
     * lastmask is zero if source pixels exactly fill up the last source byte.
     */
    lastmask = 0;
    w = image->info.sourcebounds.x.g - image->info.sourcebounds.x.l;
    if (source->bitspersample == 1) {
        if (w &= 0x7) lastmask = 0x80 >> w;
    } else {
        if (w &= 0x3) lastmask = 0xc0 >> (w*2);
    }

  while (--items >= 0) {
    if (run != NULL) {
      y = run->bounds.y.l;
      ylast = run->bounds.y.g - 1;
      buffptr = run->data;
    } else if (t != NULL) {
      y = t->y.l;
      ylast = t->y.g - 1;
      if (leftSlope = (t->l.xl != t->l.xg))
	tlix = t->l.ix + Fix(xoffset);
      if (rightSlope = (t->g.xl != t->g.xg))
	tgix = t->g.ix + Fix(xoffset);
      xl = t->l.xl + xoffset;
      xr = t->g.xl + xoffset;
    } else
      CantHappen();
    spanChange = 1;
    if (ylast > ymax) ylast = ymax;
    while ((srcY = sy * y + ySrcOffset) < 0) y++; /* center sampling */
    newSrcY = IntPart(srcY);
    oldSrcY = newSrcY - 1; /* trigger "stretch a new line" below */
    srcByteBase = source->samples[IMG_GRAY_SAMPLES] + oldSrcY * source->wbytes;
    destbase = (uint *)((uchar *)framebase +(y+info->offset.y)*framebytewidth);

    while (true) { /* loop through all device scanlines */
       /* Here newSrcY is current, stretch a new line if needed */
       if (newSrcY != oldSrcY) {
	uchar *sByte;
	int unitlStch, numIntsStch, xlStch, xrStch;
	uint *destPtr = buffer;

	/* Move oldSrcY to newSrcY and sByte to new source scanline */
	while (oldSrcY < newSrcY) { srcByteBase += source->wbytes; ++oldSrcY; }
 	sByte = srcByteBase;
	
	fx = Fix(rShiftInit) + HALF; /* in bits */ 
	xrStch = rShiftInit;

	/* m0 is mask to convert bit offset into mask boundary
	 * for 1bpp, it is 0x1f, for 2bpp it is 0x1e (to ensure 2 bit boundary)
	 */
	m0 = SCANMASK - framelog2BD;
	if (wPerChar == 1) {
	    int closeto4 = IntPart(dxByte) > 16;
	    d = 0;
	    for (i = source->wbytes; i>0; i--) {
		uint unshftd, s;
		int rShift;
    
		rShift = xrStch & m0;
		d &= ~((uint)0xffffffff>>rShift);

		s = (uint) *sByte++;
		if (i == 1 && lastmask)
		    s = (s&~lastmask) | ((s>>args->bitsPerPixel) & lastmask);

		d |= (unshftd = *(stchBytes + s)) >> rShift;
		xrStch = IntPart(fx += dxByte);
		if ((xrStch & m0) < rShift ||
		    ((xrStch & m0) == rShift && closeto4)) {
		    *destPtr++ = d;
		    d = unshftd << (SCANUNIT - rShift);
		}
	    }
	    *destPtr = d;
	}
	else {
	    d = 0;
	    for (i = source->wbytes; i>0; i--) {
		uint *sCur;
		uint unshftd, shifted, s;
		int msk, lShift, rShift, rShiftNext = xrStch & m0;
	
		s = (uint) *sByte++;
		if (i == 1 && lastmask)
		    s = (s&~lastmask) | ((s>>args->bitsPerPixel) & lastmask);
		sCur   = stchBytes + s * wPerChar;
		xlStch = xrStch;
		xrStch = IntPart(fx += dxByte);
		rShift = rShiftNext;
		rShiftNext = xrStch & m0;
		lShift = SCANUNIT - rShift;
		d &= 0xffffffff << lShift;
		d |= (unshftd = *sCur++) >> rShift;
		unitlStch = xlStch >> SCANSHIFT;
		numIntsStch = (xrStch >> SCANSHIFT) - unitlStch;
		*destPtr++ = d;
		for (msk = numIntsStch - 1; msk > 0; msk--) {
		    shifted = unshftd << lShift;
		    unshftd = *sCur++;
		    *destPtr++ = shifted | (unshftd >> rShift);
		}
		/* partial fill but no need to mask */
		d = unshftd << lShift;
		if (rShiftNext > rShift)
		    d |= *sCur >> rShift;
	    }
	    *destPtr = d;
	}
      } /* end of line stretch */
      newSrcY = IntPart(srcY += sy); /* ready for next device scanline */
      pairs = (run != NULL) ? *buffptr++ : 1;
      while (--pairs >= 0) {
	if (spanChange) {
	    if (run != NULL) {
		xl = xoffset + *buffptr++;
		xr = xoffset + *buffptr++;
	    }
	    else spanChange = leftSlope | rightSlope;

	    xl <<= framelog2BD;  xr <<= framelog2BD;
	    if (xl<xmin)
		if (xr<xmin) continue; else xl = xmin;
	    if (xr>xmax)
		if (xl>xmax) continue; else xr = xmax;
	    unitl    = xl >> SCANSHIFT;
	    numInts  = (xr >> SCANSHIFT) - unitl;
	    lMask    = leftBitArray[xl & SCANMASK];
	    rMask    = rightBitArray[xr & SCANMASK];
	    if (numInts == 0) lMask &= rMask;
	}
	{
	    uint *sCur    = bufBase + unitl;
	    uint *destPtr = destbase + unitl;
	    int n;
    
	    *destPtr = (*destPtr & ~lMask) | (*sCur++ & lMask); destPtr++;
	    if (numInts > 0) {
		for (n = numInts - 1; n > 0; n--) *destPtr++ = *sCur++;
		*destPtr = (*destPtr & ~rMask) | (*sCur & rMask);
	    }
	}
      }
      if (++y > ylast)
	break;
      destbase = (uint *)((uchar *)destbase + framebytewidth);
      if ((t != NULL) && spanChange) {
	if (y == ylast) {
	  xl = t->l.xg + xoffset;
	  xr = t->g.xg + xoffset;
	} else {
	  if (leftSlope) {
	    xl = IntPart(tlix);
	    tlix += t->l.dx;
	  }
	  if (rightSlope) {
	    xr = IntPart(tgix);
	    tgix += t->g.dx;
	  }
	}
      }
    }
    if (t != NULL)
      t++;
  }
  free(stchBytes);
  free(buffer);
}





