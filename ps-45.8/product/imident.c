/*****************************************************************************
    imident.c 
    
    Copyright (c) 1988 NeXT Incorporated.
    All rights reserved.

    THIS MODULE DEPENDS ON BYTE-ALIGNED LONGWORD ACCESS

    Edit History:
	26Sep88 Jack	created using outer loop structure of ims12d12notfr.c
	18Apr90 Terry	Optimized code size (2372 to 1888 bytes)
	
*****************************************************************************/

#import PACKAGE_SPECS
#import DEVICE
#import EXCEPT		/* for DebugAssert */
#import DEVPATTERN
#import DEVIMAGE

/* Variable declarations from framedev.h in device package */
extern integer framebytewidth;
extern PSCANTYPE framebase;
extern Card8 framelog2BD;

#define uint unsigned int
#define uchar unsigned char

extern PImageProcs fmImageProcs;
void MP12Convert32to2(uint *, uint *, uint *, DevPoint, DevPoint, int, 
		      DevPoint, int, DevPoint phase);

/* Identity matrix, no transfer function, we invert source samples */
public procedure ImIdent(int items, DevTrap *t, DevRun *run, ImageArgs *args)
{
    DevMarkInfo *info = args->markInfo;
    DevImage *image = args->image;
    DevImageSource *source = image->source;
    unsigned int *destbase, lMask, rMask;
    unsigned char *srcByteBase;
    DevShort *buffptr;
    int y, ylast, pairs, xoffset, xl, xr, xSrcOffset, ySrcOffset;
    boolean leftSlope, rightSlope, spanChange;
    DevTrap trap;
    Mtx *mtx = image->info.mtx;
    real err;
    int unitl, numInts, lShift, rShift, ymin, ymax, xmin, xmax;
    
    DebugAssert((mtx->b == 0.0) && (mtx->c == 0.0)
		&& (args->procs == fmImageProcs)
		&& (err = mtx->a-1) < .0001 && err > -.0001
		&& (err = mtx->d-1) < .0001 && err > -.0001);
    DebugAssert( /* If one of thes fails, it may increase my understanding */
	(image->info.sourcebounds.x.l == image->info.sourceorigin.x) &&
	(image->info.sourcebounds.y.l == image->info.sourceorigin.y) &&
	(image->info.sourcebounds.x.l == 0));

  xoffset = info->offset.x;
  ySrcOffset = (integer)mtx->ty - image->info.sourcebounds.y.l;
  xSrcOffset = (integer)mtx->tx - xoffset; /* both in pixels */
  xSrcOffset <<= framelog2BD; /* now bits */
  ymin = -ySrcOffset;
  ymax = ymin +
      (image->info.sourcebounds.y.g - image->info.sourcebounds.y.l) - 1;
  xmin = -xSrcOffset;
  xmax = xmin + (image->info.sourcebounds.x.g << framelog2BD);
  lShift = SCANMASK & xSrcOffset;
  rShift = SCANUNIT - lShift;
  while (--items >= 0) {
    if (run != NULL) {
      y = run->bounds.y.l;
      ylast = run->bounds.y.g - 1;
      buffptr = run->data;
  } else if (t != NULL) {
      trap = *t;
      y = trap.y.l;
      ylast = trap.y.g - 1;
      if (leftSlope = (trap.l.xl != trap.l.xg))
	trap.l.ix += Fix(xoffset);
      if (rightSlope = (trap.g.xl != trap.g.xg))
	trap.g.ix += Fix(xoffset);
      xl = trap.l.xl + xoffset;
      xr = trap.g.xl + xoffset;
    } else
      CantHappen();
    spanChange = 1;
    if (y<ymin)
      if (ylast<ymin) break; else y = ymin;
    if (ylast>ymax)
      if (y>ymax) break; else ylast = ymax;
    srcByteBase = source->samples[IMG_GRAY_SAMPLES]
      + (y + ySrcOffset) * source->wbytes + ((xSrcOffset >> SCANSHIFT) << 2);
    destbase = (uint *)((uchar *)framebase +(y+info->offset.y)*framebytewidth);
    while (true) { /* loop through all scanlines */
      pairs = (run != NULL) ? *(buffptr++) : 1;
      while (--pairs >= 0) {
        if (spanChange) {
	    if (run != NULL) {
		xl = xoffset + *buffptr++;
		xr = xoffset + *buffptr++;
	    }
	    else spanChange = leftSlope | rightSlope;

	    xl <<= framelog2BD;  xr <<= framelog2BD;
	    if (xl<xmin) {
		if (xr<xmin) {
		    /* Rectangular trap completely clipped.  Bail... */
		    if (!spanChange) goto bail;
		    continue;
		}
		else xl = xmin;
	    }
	    if (xr>xmax) {
		if (xl>xmax) {
		    /* Rectangular trap completely clipped.  Bail... */
		    if (!spanChange) goto bail;
		    continue;
		}
		else xr = xmax;
	    }
	    unitl = xl >> SCANSHIFT;
	    numInts = (xr >> SCANSHIFT) - unitl;
	    lMask = leftBitArray[xl & SCANMASK];
	    rMask = rightBitArray[xr & SCANMASK];
	    if (numInts == 0)
		lMask &= rMask;
	}
	{
	  uint *sCur = (unsigned int *)srcByteBase + unitl;
	  uint *destPtr = destbase + unitl;
	  int msk;
	  
	  if (lShift) { /* Unaligned case */
	      uint us, s;

	  if (lMask >> lShift) {
	    s   = ~(*sCur++) << lShift;
	    us  = ~(*sCur++);
	    s  |= us >> rShift;
	  } else {
	    sCur++;
	    us = ~(*sCur++);
	    s = us >> rShift;
	  }
	  *destPtr = (*destPtr & ~lMask) | (s & lMask); destPtr++;
	  for (msk = numInts - 1; msk > 0; msk--) {
	    s = us << lShift;
	    us = ~(*sCur++);
	    *destPtr++ = s | (us >> rShift);
	  }
	  if (numInts > 0) {
	    if (rMask << rShift)
	      *destPtr = (*destPtr & ~rMask) |
		(((us << lShift) | (~(*sCur) >> rShift)) & rMask);
	    else
	      *destPtr = (*destPtr & ~rMask) | ((us << lShift) & rMask);
	  }
	} else { /* Aligned case */
	  *destPtr = (*destPtr & ~lMask) | (~(*sCur++) & lMask); destPtr++;
	  for (msk = numInts - 1; msk > 0; msk--)
	    *destPtr++ = ~(*sCur++);
	  if (numInts > 0 && rMask)
	      *destPtr = (*destPtr & ~rMask) | (~(*sCur) & rMask);
	}
	}
      }
      if (++y > ylast)
	break;
      srcByteBase += source->wbytes;
      destbase = (uint *)((uchar *)destbase + framebytewidth);
      if (t != NULL && spanChange) {
	if (y == ylast) {
	  xl = trap.l.xg + xoffset;
	  xr = trap.g.xg + xoffset;
	} else {
	  if (leftSlope) {
	    xl = IntPart(trap.l.ix);
	    trap.l.ix += trap.l.dx;
	  }
	  if (rightSlope) {
	    xr = IntPart(trap.g.ix);
	    trap.g.ix += trap.g.dx;
	  }
	}
      }
    }
bail:
    if (t != NULL)
      t++;
  }
}

public procedure ImIdent32(int items, DevTrap *t, DevRun *run, ImageArgs *args)
{
  DevMarkInfo *info = args->markInfo;
  DevImage *image = args->image;
  DevImageSource *source = image->source;
  DevShort *buffptr;
  int y, ylast, pairs, xoffset, yoffset, xl, xr, xSrcOffset, ySrcOffset;
  boolean leftSlope, rightSlope;
  DevTrap trap;
  Mtx *mtx = image->info.mtx;
  real err;
  int ymin, ymax, xmin, xmax;
  DevPoint srcOffset, dstOffset, size;

  DebugAssert((mtx->b == 0.0) && (mtx->c == 0.0)
		&& (args->procs == fmImageProcs)
		&& (err = mtx->a-1) < .0001 && err > -.0001
		&& (err = mtx->d-1) < .0001 && err > -.0001);
  DebugAssert( /* If one of thes fails, it may increase my understanding */
	(image->info.sourcebounds.x.l == image->info.sourceorigin.x) &&
	(image->info.sourcebounds.y.l == image->info.sourceorigin.y) &&
	(image->info.sourcebounds.x.l == 0));

  xoffset = info->offset.x;
  yoffset = info->offset.y;
  ySrcOffset = (integer)mtx->ty - image->info.sourcebounds.y.l;
  xSrcOffset = (integer)mtx->tx - xoffset; /* both in pixels */

  ymin = -ySrcOffset;
  ymax = ymin +
    (image->info.sourcebounds.y.g - image->info.sourcebounds.y.l) - 1;
  xmin = -xSrcOffset;
  xmax = xmin + image->info.sourcebounds.x.g;

  while (--items >= 0) {
    if (run != NULL) {
      y = run->bounds.y.l;
      ylast = run->bounds.y.g - 1;
      buffptr = run->data;
    } else if (t != NULL) {
      trap = *t;
      y = trap.y.l;
      ylast = trap.y.g - 1;
      leftSlope = (trap.l.xl != trap.l.xg);
      rightSlope = (trap.g.xl != trap.g.xg);
      xl = trap.l.xl + xoffset;
      xr = trap.g.xl + xoffset;
      if (xl<xmin)
	if (xr<xmin) xl = xr = xmin; else xl = xmin;
      if (xr>xmax)
	if (xl>xmax) xl = xr = xmax; else xr = xmax;
      if (leftSlope)
	trap.l.ix += Fix(xoffset);
      if (rightSlope)
	trap.g.ix += Fix(xoffset);
    } else
      CantHappen();
    if (y<ymin)
      if (ylast<ymin) break; else y = ymin;
    if (ylast>ymax)
      if (y>ymax) break; else ylast = ymax;

    /* Special case rectangular trapezoid image */
    if(t && !leftSlope && !rightSlope) {	
	srcOffset.x = xSrcOffset + xl;
	srcOffset.y = ySrcOffset + y;
	dstOffset.x = xl;
	dstOffset.y = yoffset + y;
	size.x      = xr - xl;
	size.y      = ylast + 1 - y;
	MP12Convert32to2((uint *) source->samples[IMG_INTERLEAVED_SAMPLES],
		       (uint *) framebase, (uint *) NULL, size,
		       srcOffset, source->wbytes,
		       dstOffset, framebytewidth, info->screenphase);
    }
    else
	while (true) { /* loop through all scanlines */
	    pairs = (run != NULL) ? *buffptr++ : 1;
	    while (--pairs >= 0) {
		if (run != NULL) {
		    xl = xoffset + *buffptr++;
		    xr = xoffset + *buffptr++;
		}

		if (xl<xmin)
		    if (xr<xmin) continue; else xl = xmin;
		if (xr>xmax)
		    if (xl>xmax) continue; else xr = xmax;
		if(xl==xr) continue;
		srcOffset.x = xSrcOffset + xl;
		srcOffset.y = ySrcOffset + y;
		dstOffset.x = xl;
		dstOffset.y = yoffset + y;
		size.x      = xr - xl;
		size.y      = 1;
		MP12Convert32to2((uint *)
		    source->samples[IMG_INTERLEAVED_SAMPLES]
			       ,(uint *) framebase, (uint *) NULL, size,
			       srcOffset, source->wbytes,
			       dstOffset, framebytewidth, info->screenphase);
	    }
	    if (++y > ylast) break;
	    if (t) {
		if (y == ylast) {
		    xl = trap.l.xg + xoffset;
		    xr = trap.g.xg + xoffset;
		} else {
		    if (leftSlope) {
			xl = IntPart(trap.l.ix);
			trap.l.ix += trap.l.dx;
		    }
		    if (rightSlope) {
			xr = IntPart(trap.g.ix);
			trap.g.ix += trap.g.dx;
		    }
		}
	    }
        }
     if (t != NULL) t++;
  }
}





