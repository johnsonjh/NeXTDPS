/* PostScript Graphics Package

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

Edit History:
Jim Sandman: Mon Nov  7 17:05:17 1988
Bill Paxton: Wed Oct 21 07:52:51 1987
Paul Rovner: Tuesday, November 17, 1987 1:40:43 PM
End Edit History.
*/

#include PACKAGE_SPECS
#include DEVICE

#include "devmark.h"
#include "framedev.h"

public procedure ConstantRunMark(run, args)
  DevRun *run; MarkArgs *args; {
  DevMarkInfo *info = args->markInfo;
  DevShort y, pairs, lines, xoffset;
  register integer xl, xr;
  register DevShort *buffptr;
  register PSCANTYPE destunit;
  PSCANTYPE destbase;
  register SCANTYPE maskl, maskr;
  register integer units, unitl;
  Card32 patValue = args->patData.value;

  y = run->bounds.y.l + info->offset.y;
  buffptr = run->data;
  destbase = (PSCANTYPE)((integer)framebase + y * framebytewidth);
  lines = run->bounds.y.g - run->bounds.y.l;
  xoffset = info->offset.x;
  while (--lines >= 0) { /* foreach scanline */
    pairs = *(buffptr++);
    while (--pairs >= 0) { /* foreach interval on this scanline */
      xl = *(buffptr++) + xoffset;
      xr = *(buffptr++) + xoffset;
#if (MULTICHROME == 1)
      xl <<= framelog2BD;
      xr <<= framelog2BD;
#endif
      unitl = xl >> SCANSHIFT;
      destunit = destbase + unitl;
      units = (xr >> SCANSHIFT) - unitl;
      maskl = leftBitArray[xl & SCANMASK];
      maskr = rightBitArray[xr & SCANMASK];
      if (units == 0) maskl &= maskr; /* entire interval inside one unit */
      *destunit = (*destunit & ~maskl) | (maskl & patValue);
      if (units != 0) {
        destunit++;
        while (--units > 0) *(destunit++) = patValue;
        if (maskr) *destunit = (*destunit & ~maskr) | (maskr & patValue);
        } /* end if (units != 0) */
      } /* end foreach interval */
    destbase = (PSCANTYPE)((integer)destbase + framebytewidth);
    } /* end foreach scanline */
  }

