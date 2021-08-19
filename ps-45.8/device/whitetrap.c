/*

Copyright (c) 1983, '84, '85, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

 */

#include PACKAGE_SPECS
#include DEVICE

#include "framedev.h"
#include "devmark.h"

public procedure
WhiteTrapsMark(t, items, args)
  DevTrap *t;
  integer items;
  MarkArgs *args;
{
  DevMarkInfo *info = args->markInfo;
  register int xl, xr;
  register PSCANTYPE destbase;
  Fixed lx, ldx, rx, rdx, xoff, wsu;
  integer lines, xoffset, yoffset, xlt, xrt;
  boolean leftSlope, rightSlope;

#if (MULTICHROME == 1)
  wsu = ONE << (SCANSHIFT-framelog2BD);
#else
  wsu = ONE << SCANSHIFT;
#endif
  xoffset = info->offset.x;
  yoffset = info->offset.y;
  xoff = Fix(xoffset);
nextTrap:
  {
    register DevTrap *tt = t++;
    register Fixed w, xof = xoff;
    register int y, xo = xoffset;

    if (--items < 0)
      return;
    y = tt->y.l;
    lines = tt->y.g - y;
    if (lines <= 0)
      goto nextTrap;
    y += yoffset;
#if DPSXA
    destbase = (PSCANTYPE) ((integer) framebase
    	+ ((devXAOffset.x << framelog2BD) >> 3) 
		+ (y + devXAOffset.y) * framebytewidth);
#else DPSXA
    destbase = (PSCANTYPE) ((integer) framebase + y * framebytewidth);
#endif DPSXA
    xl = tt->l.xl + xo;
    xr = tt->g.xl + xo;
#if (MULTICHROME == 1)
    xr <<= framelog2BD;
    xl <<= framelog2BD;
#endif
    if (lines == 1)
      goto rect;
    xlt = tt->l.xg + xo;
    xrt = tt->g.xg + xo;
#if (MULTICHROME == 1)
    xrt <<= framelog2BD;
    xlt <<= framelog2BD;
#endif
    leftSlope = (xl != xlt);
    rightSlope = (xr != xrt);
    if (leftSlope) {
      lx = tt->l.ix + xoff;
      ldx = tt->l.dx;
    } else if (!rightSlope)
      goto rect;
    rx = tt->g.ix + xoff;
    rdx = tt->g.dx;
    if (!rightSlope || !leftSlope
        ||(ldx != rdx)        /* not a parallelogram */
        ||((w = rx - lx) > wsu)        /* can use whole-scanunit inner loop */
        ||(lines <= 3)        /* the slope may not be valid */
        ||(w < ONE))        /* unusually narrow one */
      goto genTrap;
    if (w == ONE && tt->l.xl + 1 == tt->g.xl && tt->l.xg + 1 == tt->g.xg)
      goto onePerRow; /* vector with single pixel per row */
    if (ldx < 0 && w == -ldx &&
        tt->l.xl == FTrunc(tt->g.ix) &&
        tt->g.xg == FTrunc(tt->l.ix + (lines-3)*(tt->l.dx))) goto onePerCol;
    if (ldx > 0 && w == ldx &&
        tt->g.xl == FTrunc(tt->l.ix) &&
	tt->l.xg == FTrunc(tt->g.ix + (lines-3)*(tt->g.dx))) goto onePerCol;
    goto genPara;
  }
rect:
  {
    register SCANTYPE maskl, maskr, allzeros;
    register integer rlines = lines;
    register PSCANTYPE destunit;

    maskl = leftBitArray[xl & SCANMASK];
    maskr = rightBitArray[xr & SCANMASK];
    xl = xl >> SCANSHIFT;        /* actually units here! */
    destbase += xl;
    xr = (xr >> SCANSHIFT) - xl;
    if (xr == 0) {
      maskl &= maskr;
      xl = framebytewidth;
      while (--rlines >= 0) {
        *destbase &= ~maskl;
        destbase = (PSCANTYPE) ((integer) destbase + xl);
      }
    } else if (xr == 1) {
      xl = framebytewidth - sizeof(SCANTYPE);
      while (--rlines >= 0) {
        *(destbase++) &= ~maskl;
        if (maskr) *destbase &= ~maskr;
        destbase = (PSCANTYPE) ((integer) destbase + xl);
      }
    } else {
      allzeros = 0;
      while (--rlines >= 0) {
        destunit = destbase;
        *(destunit++) &= ~maskl;
        xl = xr;
        while (--xl) *(destunit++) = allzeros;
        if (maskr) *destunit &= ~maskr;
        destbase = (PSCANTYPE) ((integer) destbase + framebytewidth);
      }
    }
    goto nextTrap;
  }
onePerRow:
  {
#if (MULTICHROME == 1)
    register PSCANTYPE ob = deepOnes[framelog2BD];
#else
    register PSCANTYPE ob = deepOnes[0];
#endif
    register integer fbw = framebytewidth, midlines = lines - 2;
    register Fixed lxx = lx, ldxx = ldx;

    *(destbase + (xl >> SCANSHIFT)) &= ~ob[xl & SCANMASK];
    while (midlines-- > 0) {
#if (MULTICHROME == 1)
      xl = IntPart(lxx) << framelog2BD;
#else
      xl = IntPart(lxx);
#endif
      lxx += ldxx;
      destbase = (PSCANTYPE) ((integer) destbase + fbw);
      *(destbase + (xl >> SCANSHIFT)) &= ~ob[xl & SCANMASK];
    }
    destbase = (PSCANTYPE) ((integer) destbase + fbw);
    xr = xlt;
    *(destbase + (xr >> SCANSHIFT)) &= ~ob[xr & SCANMASK];
    goto nextTrap;
  }
onePerCol:
  {
    register PSCANTYPE ba = leftBitArray, destunit;
    register SCANTYPE maskl, maskr;
    register DevShort unit;
    register Fixed ix, dx = ldx;
    register integer fbw = framebytewidth;

    if (dx < 0) {
      ix = lx;
      maskr = rightBitArray[xr & SCANMASK];
      xr >>= SCANSHIFT;	/* actually units here! */
      while (true) {
	maskl = ba[xl & SCANMASK];
	unit = xl >> SCANSHIFT;
	destunit = destbase + unit;
	if (xr == unit)
	  *destunit &= ~(maskl & maskr);
	else {
	  *destunit++ &= ~maskl;
	  if (maskr) *destunit &= ~maskr;
	}
	xr = unit;
	maskr = ~maskl;
	switch (--lines) {
	case 0:
	  goto nextTrap;
	case 1:   /* next line is the last line of the trap */
	  xl = xlt;
	  break;
	default:
#if (MULTICHROME == 1)
	  xl = IntPart(ix) << framelog2BD;
#else
	  xl = IntPart(ix);
#endif
	  ix += dx;
	}
	destbase = (PSCANTYPE) ((integer) destbase + fbw);
      }
    } else {
      ix = rx;
      maskl = leftBitArray[xl & SCANMASK];
      ba = rightBitArray;
      xl >>= SCANSHIFT;	/* actually units here! */
      while (true) {
	maskr = ba[xr & SCANMASK];
	unit = xr >> SCANSHIFT;
	destunit = destbase + xl;
	if (xl == unit)
	  *destunit &= ~(maskl & maskr);
	else {
	  *destunit++ &= ~maskl;
	  if (maskr) *destunit &= ~maskr;
	}
	xl = unit;
	maskl = ~maskr;
	switch (--lines) {
	case 0:
	  goto nextTrap;
	case 1:   /* next line is the last line of the trap */
	  xr = xrt;
	  break;
	default:
#if (MULTICHROME == 1)
	  xr = IntPart(ix) << framelog2BD;
#else
	  xr = IntPart(ix);
#endif
	  ix += dx;
	}
	destbase = (PSCANTYPE) ((integer) destbase + fbw);
      }
    }
  }
genPara:
  {
    register PSCANTYPE lba = leftBitArray, rba = rightBitArray;
    register SCANTYPE maskl, maskr;
    register int unitl;
    register PSCANTYPE destunit;
    register Fixed dx = ldx;

    while (true) {
      maskl = lba[xl & SCANMASK];
      maskr = rba[xr & SCANMASK];
      unitl = xl >> SCANSHIFT;
      destunit = destbase + unitl;
      if ((xr >> SCANSHIFT) == unitl)
        *destunit &= ~(maskl & maskr);
      else {
        *(destunit++) &= ~maskl;
        if (maskr) *destunit &= ~maskr;
      }
      switch (--lines) {
      case 0:
        goto nextTrap;
      case 1:           /* next line is the last line of the trap */
        xl = xlt;
        xr = xrt;
        break;
      default:
#if (MULTICHROME == 1)
        xl = IntPart(lx) << framelog2BD;
        xr = IntPart(rx) << framelog2BD;
#else
        xl = IntPart(lx);
        xr = IntPart(rx);
#endif
        lx += dx;
        rx += dx;
      }
      destbase = (PSCANTYPE) ((integer) destbase + framebytewidth);
    }
  }
genTrap:
  {
    register PSCANTYPE lba = leftBitArray, rba = rightBitArray;
    register SCANTYPE maskl, maskr;
    register int unitl, units;
    register PSCANTYPE destunit;

    while (true) {
      maskl = lba[xl & SCANMASK];
      maskr = rba[xr & SCANMASK];
      unitl = xl >> SCANSHIFT;
      destunit = destbase + unitl;
      units = (xr >> SCANSHIFT) - unitl;
      if (units == 0)
        *destunit &= ~(maskl & maskr);
      else {
        *(destunit++) &= ~maskl;
        maskl = 0;
        while (--units > 0)
          *(destunit++) = maskl;
        if (maskr) *destunit &= ~maskr;
      }
      switch (--lines) {
      case 0:
        goto nextTrap;
      case 1:           /* next line is the last line of the trap */
        xl = xlt;
        xr = xrt;
        break;
      default:
        if (leftSlope) {
#if (MULTICHROME == 1)
          xl = IntPart(lx) << framelog2BD;
#else
          xl = IntPart(lx);
#endif
          lx += ldx;
        }
        if (rightSlope) {
#if (MULTICHROME == 1)
          xr = IntPart(rx) << framelog2BD;
#else
          xr = IntPart(rx);
#endif
          rx += rdx;
        }
      }
      destbase = (PSCANTYPE) ((integer) destbase + framebytewidth);
    }
  }
}

