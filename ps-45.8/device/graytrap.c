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

Original version:
Edit History:
Jim Sandman: Mon Apr 24 16:08:23 1989
Paul Rovner: Tuesday, November 17, 1987 1:40:16 PM
Ivor Durham: Tue Jul  5 17:01:33 1988
Joe Pasqua: Tue Jan 10 14:33:34 1989
End Edit History.

 */

#include PACKAGE_SPECS
#include DEVICE

#include "devmark.h"
#include "framedev.h"

private procedure GrayRectMark(y, lines, xl, xr, args)
  integer y, xl, xr; register integer lines; MarkArgs *args; {
  DevMarkInfo *info = args->markInfo;
  PatternData patData;
  PSCANTYPE patRow;
  register SCANTYPE maskl, maskr, gray;
  register integer unitl, units;
  register PSCANTYPE destunit, grayPtr, eGry;
  PSCANTYPE destbase;
  integer grayOffset;
  patData = args->patData;
  maskl = leftBitArray[xl & SCANMASK];
  maskr = rightBitArray[xr & SCANMASK];
  unitl = xl >> SCANSHIFT;
  units = (xr >> SCANSHIFT) - unitl;
  if (units == 0) maskl &= maskr;
#if DPSXA
    destbase = (PSCANTYPE) ((integer) framebase
    	+ ((devXAOffset.x << framelog2BD) >> 3) 
		+ (y + devXAOffset.y) * framebytewidth)
		+ unitl;
#else /* DPSXA */
  destbase = (PSCANTYPE)((integer)framebase + y * framebytewidth) + unitl;
#endif /* DPSXA */
  patRow = GetPatternRow(info, &patData, (integer)y);
  if (patData.width == 1) {
    grayPtr = patRow; eGry = patData.end;
    if (units == 0) {
      units = framebytewidth;
      while (--lines >= 0) {
        *destbase = (*destbase & ~maskl) | (maskl & *(grayPtr++));
        destbase = (PSCANTYPE)((integer)destbase + units);
        if (grayPtr >= eGry) grayPtr = patData.start;
        }
      }
    else if (units == 1) {
      units = framebytewidth - sizeof(SCANTYPE);
      while (--lines >= 0) {
        *destbase = (*destbase & ~maskl) | (maskl & *grayPtr);
        destbase++;
        if (maskr) *destbase = (*destbase & ~maskr) | (maskr & *(grayPtr));
        destbase = (PSCANTYPE)((integer)destbase + units);
        if (++grayPtr >= eGry) grayPtr = patData.start;
        }
      }
    else {
      while (--lines >= 0) {
        destunit = destbase;
        gray = *(grayPtr++);
        *destunit = (*destunit & ~maskl) | (maskl & gray);
        destunit++;
        unitl = units;
        while (--unitl > 0) *(destunit++) = gray;
        if (maskr) *destunit = (*destunit & ~maskr) | (maskr & gray);
        destbase = (PSCANTYPE)((integer)destbase + framebytewidth);
        if (grayPtr >= eGry) grayPtr = patData.start;
        }
      }
    return;
    }
  grayOffset = unitl % patData.width;
  while (--lines >= 0) {
    eGry = patRow + patData.width;
    grayPtr = patRow + grayOffset;
    destunit = destbase;
    *destunit = (*destunit & ~maskl) | (maskl & *grayPtr);
    if (units != 0) {
      destunit++;
      unitl = units;
      while (--unitl > 0) {
        if (++grayPtr >= eGry) grayPtr = patRow;
        *(destunit++) = *grayPtr;
        }
      if (++grayPtr >= eGry) grayPtr = patRow;
      if (maskr) *destunit = (*destunit & ~maskr) | (maskr & *grayPtr);
      }
    destbase = (PSCANTYPE)((integer)destbase + framebytewidth);
    patRow = eGry; if (patRow >= patData.end) patRow = patData.start;
    }
  } /* GrayRectMark */

public procedure GrayTrapsMark(t, items, args)
  DevTrap *t; integer items; MarkArgs *args; {
  DevMarkInfo *info = args->markInfo;
  PatternData patData;
  register SCANTYPE maskl, maskr;
  register int unitl, units;
  register int xl, xg;
  register PSCANTYPE destunit, grayPtr;
  Fixed lx, ldx, gx, gdx, xoff;
  integer y, xoffset, yoffset, lines;
  boolean leftSlope, rightSlope;
  PSCANTYPE patRow;
  patData = args->patData;
  xoffset = info->offset.x;
  yoffset = info->offset.y;
  xoff = Fix(xoffset);
  while (--items >= 0) {
    { register DevTrap *tt = t;
    y = tt->y.l;
    lines = tt->y.g - y;
    if (lines <= 0) goto nextTrap;
    y += yoffset;
    leftSlope = (tt->l.xg != tt->l.xl);
    rightSlope = (tt->g.xg != tt->g.xl);
    xl = tt->l.xl + xoffset;
    xg = tt->g.xl + xoffset;
#if (MULTICHROME == 1)
    xl <<= framelog2BD;
    xg <<= framelog2BD;
#endif
    if (leftSlope) { lx = tt->l.ix + xoff; ldx = tt->l.dx; }
    else if (!rightSlope) {
      GrayRectMark(y, (integer)lines, xl, xg, args);
      goto nextTrap;
      }
    if (rightSlope) { gx = tt->g.ix + xoff; gdx = tt->g.dx; }
    }
    { register PSCANTYPE destbase, eGry;
    destbase = (PSCANTYPE)((integer)framebase + y * framebytewidth);
    patRow = GetPatternRow(info, &patData, (integer)y);
    while (true) {
      eGry = patRow + patData.width;
      maskl = leftBitArray[xl & SCANMASK];
      maskr = rightBitArray[xg & SCANMASK];
      unitl = xl >> SCANSHIFT;
      destunit = destbase + unitl;
      grayPtr = patRow + (unitl % patData.width);
      units = (xg >> SCANSHIFT) - unitl;
      if (units == 0) maskl &= maskr;
      *destunit = (*destunit & ~maskl) | (maskl & *grayPtr);
      if (units != 0) {
        destunit++;
        if (patData.width == 1) {
          maskl = *grayPtr;
          while (--units > 0) *(destunit++) = maskl;
          }
        else {
          while (--units > 0) {
            if (++grayPtr >= eGry) grayPtr = patRow;
            *(destunit++) = *grayPtr;
            }
          if (++grayPtr >= eGry) grayPtr = patRow;
          maskl = *grayPtr;
          }
        if (maskr) *destunit = (*destunit & ~maskr) | (maskr & maskl);
        }
      switch (--lines) {
        case 0: goto nextTrap;
        case 1:
          xl = t->l.xg + xoffset;
          xg = t->g.xg + xoffset;
#if (MULTICHROME == 1)
          xl <<= framelog2BD;
          xg <<= framelog2BD;
#endif
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
	    xg = IntPart(gx) << framelog2BD;
#else
	    xg = IntPart(gx);
#endif
	    gx += gdx;
	    }
        }
      destbase = (PSCANTYPE)((integer)destbase + framebytewidth);
      if ((patRow = eGry) >= patData.end) patRow = patData.start;
      }
    }
    nextTrap: t++;
    }
  }

