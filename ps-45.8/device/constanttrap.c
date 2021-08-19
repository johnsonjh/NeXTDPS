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
Jim Sandman: Mon Apr 24 16:09:02 1989
Paul Rovner: Tuesday, November 17, 1987 1:40:16 PM
Joe Pasqua: Mon Jan  9 16:55:46 1989
End Edit History.

 */

#include PACKAGE_SPECS
#include DEVICE

#include "devmark.h"
#include "framedev.h"

private procedure ConstantRectMark(y, lines, xl, xr, patValue)
  integer y, xl, xr; register integer lines; register Card32 patValue; {
  register SCANTYPE maskl, maskr;
  register integer unitl, units;
  register PSCANTYPE destunit;
  PSCANTYPE destbase;
  maskl = leftBitArray[xl & SCANMASK];
  maskr = rightBitArray[xr & SCANMASK];
  unitl = xl >> SCANSHIFT;
  units = (xr >> SCANSHIFT) - unitl;
  if (units == 0) maskl &= maskr;
  destbase = (PSCANTYPE)((integer)framebase + y * framebytewidth) + unitl;
  if (units == 0) {
    units = framebytewidth;
    while (--lines >= 0) {
      *destbase = (*destbase & ~maskl) | (maskl & patValue);
      destbase = (PSCANTYPE)((integer)destbase + units);
      }
    }
  else if (units == 1) {
    units = framebytewidth - sizeof(SCANTYPE);
    while (--lines >= 0) {
      *destbase = (*destbase & ~maskl) | (maskl & patValue);
      destbase++;
      if (maskr) *destbase = (*destbase & ~maskr) | (maskr & patValue);
      destbase = (PSCANTYPE)((integer)destbase + units);
      }
    }
  else {
    while (--lines >= 0) {
      destunit = destbase;
      *destunit = (*destunit & ~maskl) | (maskl & patValue);
      destunit++;
      unitl = units;
      while (--unitl > 0) *(destunit++) = patValue;
      if (maskr) *destunit = (*destunit & ~maskr) | (maskr & patValue);
      destbase = (PSCANTYPE)((integer)destbase + framebytewidth);
      }
    }
  return;
  } /* ConstantRectMark */

public procedure ConstantTrapsMark(t, items, args)
  DevTrap *t; integer items; MarkArgs *args; {
  DevMarkInfo *info = args->markInfo;
  register SCANTYPE maskl, maskr;
  register int unitl, units;
  register int xl, xg;
  register PSCANTYPE destunit, grayPtr;
  Fixed lx, ldx, gx, gdx, xoff;
  integer y, xoffset, yoffset, lines;
  boolean leftSlope, rightSlope;
  Card32 patValue = args->patData.value;
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
      ConstantRectMark(y, (integer)lines, xl, xg, patValue);
      goto nextTrap;
      }
    if (rightSlope) { gx = tt->g.ix + xoff; gdx = tt->g.dx; }
    }
    { register PSCANTYPE destbase;
    destbase = (PSCANTYPE)((integer)framebase + y * framebytewidth);
    while (true) {
      maskl = leftBitArray[xl & SCANMASK];
      maskr = rightBitArray[xg & SCANMASK];
      unitl = xl >> SCANSHIFT;
      destunit = destbase + unitl;
      units = (xg >> SCANSHIFT) - unitl;
      if (units == 0) maskl &= maskr;
      *destunit = (*destunit & ~maskl) | (maskl & patValue);
      if (units != 0) {
        destunit++;
	maskl = patValue;
	while (--units > 0) *(destunit++) = maskl;
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
      }
    }
    nextTrap: t++;
    }
  } /* ConstantTrapsMark */
