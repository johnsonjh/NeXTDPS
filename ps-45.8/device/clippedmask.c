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
Jim Sandman: Fri Jul 28 11:06:31 1989
Paul Rovner: Wed Nov 15 13:44:06 1989
Ivor Durham: Tue Jul  5 16:59:25 1988
End Edit History.

 */

#include PACKAGE_SPECS
#include DEVICE
#include EXCEPT

#include "devmark.h"
#include "framemaskdev.h"
#if (MASKCOMPRESSION)
#include "compress.h"
#endif

#define IsBlack (graySwitch == 0)
#define IsWhite (graySwitch < 0)
#define IsConstant (graySwitch == 1)
#define IsGray (graySwitch == 2)

#if (MULTICHROME == 1)
private PSCANTYPE expandedScanLine;
private integer expandedScanLineLength = 0;

private PSCANTYPE ExpandOneLine(base, width) PSCANTYPE base; integer width; {
  unsigned char *sourcebyte;
  SCANTYPE t, *destunit;
#if (MULTICHROME == 1)
  integer wunits = ((width + SCANUNIT - 1) >> SCANSHIFT) << framelog2BD;
#else
  integer wunits = (width + SCANUNIT - 1) >> SCANSHIFT;
#endif
  
  if (expandedScanLineLength < wunits) {
    if (expandedScanLine) os_free(expandedScanLine);
    expandedScanLine = (PSCANTYPE)os_sureMalloc(wunits*sizeof(SCANTYPE));
    expandedScanLineLength = wunits;
  }
  destunit = expandedScanLine;
  sourcebyte = (unsigned char *)base;

  switch (framelog2BD) {
  
    case 1:
      do {
	t  = *((PCard16)((char *)source2bits + (*sourcebyte++ * 2))) << 16;
	t |= *((PCard16)((char *)source2bits + (*sourcebyte++ * 2)));
	*(destunit++) = t;
      } while (--wunits != 0);
      break;
      
    case 2:
      do {
        *(destunit++) = *((PSCANTYPE)((char *)source4bits + (*sourcebyte++ * 4)));
      } while (--wunits != 0);
      break;
      
    case 3:
      do {
	PSCANTYPE sourcebits = (PSCANTYPE)((char *)source8bits + (*sourcebyte++ * 8));
	*(destunit++) = *(sourcebits);
	*(destunit++) = *(sourcebits+1);
      } while ((wunits -= 2) != 0);
      break;
      
    case 4:
      do {
	SCANTYPE du; unsigned char su; integer i;
	su = *(sourcebyte++);
        for (i = 0; i < 4; i++) {
	  du = 0;
	  if (su & 0x80) du = 0xFFFF0000; su <<= 1; 
	  if (su & 0x80) du |= 0xFFFF; su <<= 1; *(destunit++) = du;
	}
      } while ((wunits -= 4) != 0);
      break;
      
    case 5:
      do {
	unsigned char su; integer i;
	su = *(sourcebyte++);
        for (i = 0; i < 8; i++) {
	  if (su & 0x80) *(destunit++) = 0xFFFFFFFF; else *(destunit++) = 0L;
	  su <<= 1;
	}
      } while ((wunits -= 8) != 0);
      break;
      

    default:  CantHappen();
      
  }

  return expandedScanLine;
}
#endif

public procedure ClippedMasksMark(t, run, masks, items, args)
  DevTrap *t; DevRun *run;
  DevMask *masks; integer items; MarkArgs *args; {
  /* either trap is NULL or run is NULL. */
  DevMarkInfo *info = args->markInfo;
  PatternData patData;
  PSCANTYPE patRow;
  PMask mask;
  integer ylast, y, xl, xr, xmin, xmax, ymin, unitl, txl, txr;
  integer shiftl, shiftr, desttodc, xoffset, yoffset;
  integer wunits, wbytes;
  boolean leftSlope, rightSlope;
  PSCANTYPE eGry, destbase;
  register PSCANTYPE grayPtr, destunit, sourceunit, sourcebase, srcbase;
  register SCANTYPE destbits, maskbits, maskl, maskr;
  register int units;
  DevShort *buffptr;
  integer cxmin, cxmax, cy, clast, pairs, graySwitch;
  Fixed lftx, rhtx;
  DevTrap trap;
  SCANTYPE expand[25];

  patData = args->patData;
  xoffset = info->offset.x;
  yoffset = info->offset.y;
  if (run != NULL) {
    cxmin = run->bounds.x.l + xoffset;
    cxmax = run->bounds.x.g + xoffset;
#if (MULTICHROME == 1)
    cxmin <<= framelog2BD;
    cxmax <<= framelog2BD;
#endif
    cy = run->bounds.y.l + yoffset;
    clast = run->bounds.y.g + yoffset - 1;
    }
  else if (t != NULL) {
    trap = *t;
    cy = trap.y.l + yoffset;
    clast = trap.y.g + yoffset - 1;
    trap.l.xl += xoffset;
    trap.l.xg += xoffset;
    trap.g.xl += xoffset;
    trap.g.xg += xoffset;
    cxmin = MIN(trap.l.xl, trap.l.xg);
    cxmax = MAX(trap.g.xl, trap.g.xg);
#if (MULTICHROME == 1)
    cxmin <<= framelog2BD;
    cxmax <<= framelog2BD;
#endif
    leftSlope = (trap.l.xl != trap.l.xg);
    rightSlope = (trap.g.xl != trap.g.xg);
    if (leftSlope) trap.l.ix += Fix(xoffset);
    if (rightSlope) trap.g.ix += Fix(xoffset);
    }
  else CantHappen();
  if (args->patData.constant)
    switch (args->patData.value) {
      case 0xFFFFFFFF: graySwitch = 0; break;
      case 0: graySwitch = -1; break;
      default: graySwitch = 1;
      }
  else graySwitch = 2;
  while (--items >= 0) {
#if (MASKCOMPRESSION)
    boolean decompressing = false;
#endif
    xmin = masks->dc.x + xoffset;
#if (MULTICHROME == 1)
    xmin <<= framelog2BD;
#endif
    if (xmin >= cxmax) goto nextMask;
    y = ymin = masks->dc.y + yoffset;
    if (clast < y) goto nextMask;
    mask = masks->mask;
#if (MULTICHROME == 1)
    xmax = xmin + (mask->width << framelog2BD);
#else
    xmax = xmin + mask->width;
#endif
    if (xmax <= cxmin) goto nextMask;
    ylast = y + mask->height - 1;
    if (cy > ylast) goto nextMask;
    if (run != NULL) {
      if (y <= cy) { y = cy; buffptr = run->data; }
      else if (cy < y) {
        buffptr = RunArrayRow(run, y - yoffset);
        }
      }
    else {
      lftx = trap.l.ix; rhtx = trap.g.ix;
      txl = trap.l.xl; txr = trap.g.xl;
      if (y < cy) y = cy;
      else if (cy < y) {
        integer dyToMask = y - cy;
        if (leftSlope) {
          lftx += trap.l.dx * dyToMask; txl = IntPart(lftx - trap.l.dx); }
        if (rightSlope) {
          rhtx += trap.g.dx * dyToMask; txr = IntPart(rhtx - trap.g.dx); }
        }
#if (MULTICHROME == 1)
      txl <<= framelog2BD;
      txr <<= framelog2BD; 
#endif
      }
    if (clast < ylast) ylast = clast;
    wunits = (mask->width + SCANUNIT - 1) >> SCANSHIFT;
    if (wunits == 0) goto nextMask;
    wbytes = wunits * sizeof(SCANTYPE);

    sourcebase = (PSCANTYPE)mask->data;
    if (mask->unused) ExpandMask(mask, sourcebase = expand);
    if (mask->maskID == BANDMASKID) {
      /* The given mask is being imaged for a band device */
#if (MASKCOMPRESSION)
      if (*sourcebase != -1) {
	decompressing = true;
	SetUpDecompression((PCard8)(sourcebase+1), y - ymin);
	sourcebase = DecompressOneLine();
        Assert(sourcebase);
	}
      else
#endif
        sourcebase = (PSCANTYPE)((PCard8)sourcebase + (y - ymin) * wbytes) + 1;
      }
    else
      sourcebase = (PSCANTYPE)((PCard8)sourcebase + (y - ymin) * wbytes);

#if (MULTICHROME == 1)
    if (framelog2BD == 0) 
      srcbase = sourcebase;
    else 
      srcbase = ExpandOneLine(sourcebase,mask->width);
#else
    srcbase = sourcebase;
#endif

    if (IsGray) {
      patRow = GetPatternRow(info, &patData, y);
      eGry = patRow + patData.width;
      }
#if DPSXA
		destbase = (PSCANTYPE) ((integer) framebase
    		+ ((devXAOffset.x << framelog2BD) >> 3) 
				+ (y + devXAOffset.y) * framebytewidth);
#else /* DPSXA */
    destbase = (PSCANTYPE)((integer)framebase + y * framebytewidth);
#endif /* DPSXA */
    shiftr = xmin & SCANMASK;
    shiftl = SCANUNIT - shiftr;
    while (true) {
      pairs = (run != NULL) ? *(buffptr++) : 1;
      while (--pairs >= 0) {
        if (run != NULL) {
          xl = *(buffptr++) + xoffset;
          xr = *(buffptr++) + xoffset;
#if (MULTICHROME == 1)
	  xl <<= framelog2BD;
	  xr <<= framelog2BD;
#endif
	  }
        else {
          xl = txl;
	  xr = txr;
	  }
        if (xl < xmin) xl = xmin;
        if (xr > xmax) xr = xmax;
        if (xl >= xr) continue; /* this run does not overlap the mask */
        unitl = xl >> SCANSHIFT;
        units = (xr >> SCANSHIFT) - unitl;
        maskl = leftBitArray[xl & SCANMASK];
        maskr = rightBitArray[xr & SCANMASK];
        if (units == 0) maskl &= maskr;
        destunit = destbase + unitl;
        desttodc = (unitl << SCANSHIFT) - xmin;
        sourceunit = srcbase + desttodc / SCANUNIT;
        if (desttodc < 0) sourceunit--;
        if (IsGray) {
          grayPtr = patRow + (unitl % patData.width);
          eGry = patRow + patData.width;
          }
        if (shiftr == 0) destbits = maskl & *sourceunit;
        else {
          destbits = *(sourceunit++);
          destbits LSHIFTEQ shiftl;
          maskbits = *sourceunit;
          destbits |= maskbits RSHIFT shiftr;
          destbits &= maskl;
          }
        if (!IsBlack) *destunit &= ~destbits;
        switch (graySwitch) {
          case 1: {
            destbits &= patData.value; break;
            }
          case 2: {
            destbits &= *grayPtr++;
            if (grayPtr >= eGry) grayPtr = patRow;
            }
          }
        if (!IsWhite) *destunit |= destbits;
        destunit++;
        if (units != 0) {
          while (--units > 0) {
            ++sourceunit;
            if (shiftr == 0) destbits = *sourceunit;
            else {
              destbits = maskbits;
              destbits LSHIFTEQ shiftl;
              maskbits = *sourceunit;
              destbits |= maskbits RSHIFT shiftr;
              }
            if (!IsBlack) *destunit &= ~destbits;
	    switch (graySwitch) {
	      case 1: {
		destbits &= patData.value; break;
		}
	      case 2: {
		destbits &= *grayPtr++;
		if (grayPtr >= eGry) grayPtr = patRow;
		}
	      }
            if (!IsWhite) *destunit |= destbits;
            destunit++;
            }
          ++sourceunit;
          if (shiftr == 0) destbits = maskr & *sourceunit;
          else {
            destbits = maskbits;
            destbits LSHIFTEQ shiftl;
            destbits |= *sourceunit RSHIFT shiftr;
            destbits &= maskr;
            }
          if (!IsBlack) *destunit &= ~destbits;
	  switch (graySwitch) {
	    case 1: {
	      destbits &= patData.value; break;
	      }
	    case 2: {
	      destbits &= *grayPtr;
	      }
	    }
          if (!IsWhite) *destunit |= destbits;
          }
        }
      if (++y > ylast) goto nextMask;
      if (t != NULL) {
        if (y == clast) {
          txl = trap.l.xg;
	  txr = trap.g.xg;
#if (MULTICHROME == 1)
	  txl <<= framelog2BD;
	  txr <<= framelog2BD;
#endif
	  }
        else {
          if (leftSlope) {
	    txl = IntPart(lftx);
#if (MULTICHROME == 1)
	    txl <<= framelog2BD;
#endif
	    lftx += trap.l.dx;
	    }
          if (rightSlope) {
	    txr = IntPart(rhtx);
#if (MULTICHROME == 1)
	    txr <<= framelog2BD;
#endif
	    rhtx += trap.g.dx;
	    }
          }
	}
      if (IsGray) {
        if ((patRow += patData.width) >= patData.end) patRow = patData.start;
        eGry = patRow + patData.width;
        }

#if (MASKCOMPRESSION)
      if (decompressing) {
        sourcebase = DecompressOneLine();
        Assert(sourcebase);
	} else
#endif
     sourcebase = (PSCANTYPE)((integer)sourcebase + wbytes);

#if (MULTICHROME == 1)
      if (framelog2BD == 0) 
	srcbase = sourcebase;
      else 
	srcbase = ExpandOneLine(sourcebase,mask->width);
#else
      srcbase = sourcebase;
#endif

      destbase = (PSCANTYPE)((integer)destbase + framebytewidth);
      }
    nextMask: masks++;
    }
  }
  
            

