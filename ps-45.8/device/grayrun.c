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
Jim Sandman: Tue Nov  8 11:28:55 1988
Bill Paxton: Wed Oct 21 07:52:51 1987
Paul Rovner: Tuesday, November 17, 1987 1:40:43 PM
Ivor Durham: Tue Jul  5 17:00:31 1988
Joe Pasqua: Tue Jan 10 14:32:08 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include DEVICE

#include "framedev.h"
#include "devmark.h"

public procedure GrayRunMark(run, args)
  DevRun *run; MarkArgs *args; {
  DevMarkInfo *info = args->markInfo;
  PatternData patData;
  DevShort y, pairs, lines, xoffset;
  register integer xl, xr;
  register DevShort *buffptr;
  register PSCANTYPE destunit, grayPtr, eGry;
  PSCANTYPE destbase;
  register SCANTYPE maskl, maskr;
  register integer units, unitl;
  PSCANTYPE patRow;

  patData = args->patData;
  y = run->bounds.y.l + info->offset.y;
  buffptr = run->data;
  patRow = GetPatternRow(info, &patData, (integer)y);
#if DPSXA
    destbase = (PSCANTYPE) ((integer) framebase
    	+ ((devXAOffset.x << framelog2BD) >> 3) 
		+ (y + devXAOffset.y) * framebytewidth);
#else DPSXA
    destbase = (PSCANTYPE) ((integer) framebase + y * framebytewidth);
#endif DPSXA
  lines = run->bounds.y.g - run->bounds.y.l;
  xoffset = info->offset.x;
  while (--lines >= 0) { /* foreach scanline */
    eGry = patRow + patData.width;
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
      grayPtr = patRow + (unitl % patData.width); 
      units = (xr >> SCANSHIFT) - unitl; 
      maskl = leftBitArray[xl & SCANMASK];
      maskr = rightBitArray[xr & SCANMASK];
      if (units == 0) maskl &= maskr; 
      *destunit = (*destunit & ~maskl) | (maskl & *grayPtr);
      if (units != 0) {
        destunit++;
        if (patData.width == 1) { /* take advantage of degenerate gray pattern */
          while (--units > 0) *(destunit++) = *grayPtr;
          }
        else {
          while (--units > 0) {
            if (++grayPtr >= eGry) grayPtr = patRow;
            *(destunit++) = *grayPtr;
            }
          if (++grayPtr >= eGry) grayPtr = patRow;
          }
        if (maskr) *destunit = (*destunit & ~maskr) | (maskr & *grayPtr);
        } /* end if (units != 0) */
      } /* end foreach interval */
    destbase = (PSCANTYPE)((integer)destbase + framebytewidth);
    patRow = eGry; if (patRow >= patData.end) patRow = patData.start;
    } /* end foreach scanline */
  }

