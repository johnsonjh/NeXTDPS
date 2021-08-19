/*
  blackrun.c

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
Jim Sandman: Mon Nov  7 17:04:46 1988
Bill Paxton: Wed Oct 21 07:59:03 1987
Paul Rovner: Tuesday, November 3, 1987 8:51:50 AM
End Edit History.
*/

#include PACKAGE_SPECS
#include DEVICE

#include "devmark.h"
#include "framedev.h"

public procedure BlackRunMark(run, args)
  DevRun *run; MarkArgs *args; {
  DevShort y, pairs, lines;
  register SCANTYPE maskl, maskr;
  register int units, xl, xr, unitl;
  int xoffset;
  register PSCANTYPE destunit, destbase;
  register DevShort *buffptr;
  register PSCANTYPE lba;
  DevMarkInfo *info = args->markInfo;
  PSCANTYPE rba;
  y = run->bounds.y.l + info->offset.y;
  buffptr = run->data;
#if DPSXA
  destbase = (PSCANTYPE) ((integer) framebase
    	+ ((devXAOffset.x << framelog2BD) >> 3) 
		+ (y + devXAOffset.y) * framebytewidth);
#else /* DPSXA */
  destbase = (PSCANTYPE)((integer)framebase + y * framebytewidth);
#endif /* DPSXA */
  lines = run->bounds.y.g - run->bounds.y.l;
  xoffset = info->offset.x;
  lba = leftBitArray; rba = rightBitArray;
  while (--lines >= 0) {
    pairs = *(buffptr++);
    while (--pairs >= 0) {
#if (MULTICHROME == 1)
      xl = (*(buffptr++) + xoffset) << framelog2BD;
      xr = (*(buffptr++) + xoffset) << framelog2BD;
#else
      xl = (*(buffptr++) + xoffset);
      xr = (*(buffptr++) + xoffset);
#endif
      maskl = lba[xl & SCANMASK];
      maskr = rba[xr & SCANMASK];
      unitl = xl >> SCANSHIFT;
      destunit = destbase + unitl;
      units = (xr >> SCANSHIFT) - unitl;
      if (units == 0) *destunit |= maskl & maskr;
      else {
        *(destunit++) |= maskl;
        maskl = -1;
        while (--units > 0) *(destunit++) = maskl;
        if (maskr) *destunit |= maskr;
        }
      }
    destbase = (PSCANTYPE)((integer)destbase + framebytewidth);
    }
  }

