/*  PostScript generic image module

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
Ivor Durham: Thu Mar 24 08:41:45 1988
Bill Paxton: Tue Mar 29 09:21:53 1988
Paul Rovner: Wednesday, January 27, 1988 4:27:33 PM
Jim Sandman: Tue Apr 25 09:37:31 1989
Joe Pasqua: Tue Feb 28 10:58:50 1989
Ed Taft: Wed Dec 13 18:17:57 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include DEVICETYPES
#include DEVPATTERN
#include DEVIMAGE
#include EXCEPT

#include "imagepriv.h"

#define BPP 2

public procedure ImS12D12NoTfr(items, t, run, args)
  /* 2 bit per pixel source */
  integer items;
  DevTrap *t;
  DevRun *run;
  ImageArgs *args; 
  {
  DevMarkInfo *info = args->markInfo;
  DevImage *image = args->image;
  DevImageSource *source = image->source;
  register uchar *sLoc;
  register PSCANTYPE pixVals;
  uchar *srcLoc;
  DevTrap trap;
  Cd c;
  DevShort *buffptr;
  integer y, ylast, scrnRow, mpwCBug, scrnCol, pairs,
    xoffset, yoffset, txl, txr, wbytes, screenwidth, xx, yy, units,
    pixelX, pixelXr, bitr, srcXprv, srcYprv, lastPixel, wb, srcPix;
  DevPoint grayOrigin;
  DevFixedPoint sv, svy;
  Fixed fxdSrcX, fxdSrcY, fX, fY, srcLX, srcLY, srcGX, srcGY, endX, endY;
  boolean leftSlope, rightSlope;
  PSCANTYPE destunit, destbase;
  SCANTYPE maskl, maskr;
  uchar *sScrnRow;
  register integer sPix;
  boolean needData;
  
  if (args->bitsPerPixel != 2 || args->gray.n != 4 ||
    args->gray.delta != 1 || args->gray.first != 0 || 
    (image->transfer != NULL && image->transfer->white != NULL))
    RAISE(ecLimitCheck, (char *)NIL);
  
  pixVals = GetPixBuffers();

  /* srcXprv srcYprv are integer coordinates of source pixel */
  srcXprv = image->info.sourcebounds.x.l;
  srcYprv = image->info.sourcebounds.y.l;
  /* srcLX, srcLY, srcGX, srcGY give fixed bounds of source in image space */
  srcLX = Fix(srcXprv);
  srcLY = Fix(srcYprv);
  srcGX = Fix(image->info.sourcebounds.x.g);
  srcGY = Fix(image->info.sourcebounds.y.g);
  /* srcLoc points to source pixel at location (srcXprv, srcYprv) */
  srcLoc = source->samples[IMG_GRAY_SAMPLES];
  wbytes = source->wbytes;
  srcLoc += (srcYprv - image->info.sourceorigin.y) * wbytes;
  srcLoc += (srcXprv - image->info.sourceorigin.x) >> 2;
  srcPix = (srcXprv - image->info.sourceorigin.x) & 3;
  /* image space fixed point vector for (1,0) in device */
  sv.x = pflttofix(&image->info.mtx->a);
  sv.y = pflttofix(&image->info.mtx->b);
  /* image space fixed point vector for (0,1) in device */
  svy.x = pflttofix(&image->info.mtx->c);
  svy.y = pflttofix(&image->info.mtx->d);
  {
  register uchar i, j, m;  
  j = 0; /* j is the source pix value */
  while (j <= 3) {
    uchar val = (args->gray.first == 0) ? 3 - j : j;
    for (i = 0; i <= 6; i += 2) { /* i is the left shift amount */
      m = j << i; /* byte value after mask */
      pixVals[m] = val;
      }
    j++;
    }
  } /* end of register block */
  xoffset = info->offset.x;
  yoffset = info->offset.y;
  (*args->procs->Begin)(args->data);
  while (--items >= 0) {
    if (run != NULL) {
      y = run->bounds.y.l;
      ylast = run->bounds.y.g - 1;
      buffptr = run->data;
      }
    else if (t != NULL) {
      trap = *t;
      y = trap.y.l;
      ylast = trap.y.g - 1;
      leftSlope = (trap.l.xl != trap.l.xg);
      rightSlope = (trap.g.xl != trap.g.xg);
      txl = trap.l.xl + xoffset;
      txr = trap.g.xl + xoffset;
      if (leftSlope) trap.l.ix += Fix(xoffset);
      if (rightSlope) trap.g.ix += Fix(xoffset);
      }
    else CantHappen();
    y += yoffset; ylast += yoffset;
    /* fX and fY are fixed point location in source in image coords */
    /* fX and fY correspond to integer coords (xx, yy) in device space */
    xx = xoffset; yy = y;
    c.x = 0.5; c.y = y - yoffset + 0.5; /* add .5 for center sampling */
    TfmPCd(c, image->info.mtx, &c);
    fX = pflttofix(&c.x); fY = pflttofix(&c.y);
    while (true) {
      pairs = (run != NULL) ? *(buffptr++) : 1;
      while (--pairs >= 0) {
        { register int xl, xr;
          if (run != NULL) {
            xl = *(buffptr++) + xoffset; xr = *(buffptr++) + xoffset; }
          else { xl = txl; xr = txr; }
	  if (xl != xx) { /* update fX, fY for change in device x coord */
            register int diff = xl - xx;
            switch (diff) {
              case  1: fX += sv.x; fY += sv.y; break;
	      case -1: fX -= sv.x; fY -= sv.y; break;
	      default: fX += diff * sv.x; fY += diff * sv.y;
	      }
            xx = xl;
	    }
          if (y != yy) { /* update fX, fY for change in device y coord */
            register int diff = y - yy;
            switch (diff) {
              case  1: fX += svy.x; fY += svy.y; break;
	      default: fX += diff * svy.x; fY += diff * svy.y;
	      }
            yy = y;
	    }
          /* (fX, fY) now gives image space coords for (xl, y) in device */
	  { register integer X, Y;
            X = fX; Y = fY;
	    /* increase xl until (X,Y) is inside bounds of source data */
            until (xl == xr || (
              X >= srcLX && X < srcGX && Y >= srcLY && Y < srcGY)) {
              X += sv.x; Y += sv.y; xl++;
              }
            fxdSrcX = X; fxdSrcY = Y;
	    /* (fxdSrcX, fxdSrcY) = image coords for (xl, y) in device */
            X = X + sv.x * (xr - xl - 1);
            Y = Y + sv.y * (xr - xl - 1);
	    /* X Y now gives image coords for (xr-1, y) */
	    /* decrease xr until last pixel is inside bounds of source data */
            until (xl == xr || (
              X >= srcLX && X < srcGX && Y >= srcLY && Y < srcGY)) {
              X -= sv.x; Y -= sv.y; xr--;
              }
            endX = X; endY = Y;
	    /* (endX, endY) = image coords for (xr-1, y) in device */
            }
          if (xl == xr) continue; /* nothing left for this pair */
          destbase = (*args->procs->GetWriteScanline)(args->data, y, xl, xr);
          pixelX = (xl & SCANMASK >> 1) << 1;
          pixelXr = (xr & SCANMASK >> 1) << 1;
          { register int unitl;
            unitl = xl >> (SCANSHIFT-1);
            destunit = destbase + unitl;
            units = (xr >> (SCANSHIFT-1)) - unitl;
            }
          }
        maskl = leftBitArray[pixelX];
        if (pixelXr == 0) {
	  units--; pixelXr = SCANUNIT; maskr = -1; bitr = SCANUNIT;
	  }
        else {
	  bitr = pixelXr; maskr = rightBitArray[bitr];
	  };
	lastPixel = (units > 0) ? SCANUNIT : pixelXr;
	sLoc = srcLoc;
        { register int sY;
          sY = IntPart(fxdSrcY);
          if (sY != srcYprv) {
            register int diff;
            switch (diff = sY - srcYprv) {
              case  1: sLoc += wbytes; break;
              case -1: sLoc -= wbytes; break;
              default: sLoc += diff * wbytes;
              }
            srcYprv = sY;
            }
          }
	sPix = srcPix;
        { register int sX;
          sX = IntPart(fxdSrcX);
          if (sX != srcXprv) {
            register int diff;
	    switch (diff = sX - srcXprv) {
              case -1:
                if (--sPix < 0) {
                  sPix = 3; sLoc--; }
                break;
	      case  1:
	        if (++sPix > 3) {
	          sPix = 0; sLoc++; }
	        break;
	      default:
                if (diff > 0) {
                  sLoc += diff >> 2; sPix += (diff & 3);
                  if (sPix > 3) { sPix -= 4; sLoc++; }
                  }
                else {
                  diff = -diff; sLoc -= diff >> 2; sPix -= (diff & 3);
                  if (sPix < 0) { sPix += 4; sLoc--; }
                  }
              }
            srcXprv = sX; srcPix = sPix;
            }
          }
        srcLoc = sLoc;
        if (IntPart(fxdSrcX) == IntPart(endX)) { /* vertical source */
          register int b;
          register SCANTYPE temp;
          register Fixed ey, dy;
          register unsigned char sData;
	  register unsigned char sMsk = 0xC0 >> (sPix << 1);
	  needData = true;
	  temp = *destunit & ~maskl;
	  if (sv.y > 0) {
            dy = sv.y; wb = wbytes; ey = ONE - (fxdSrcY & LowMask); }
          else {
            dy = -sv.y; wb = -wbytes; ey = (fxdSrcY & LowMask) + 1; }
          while (true) { /* units loop */
            b = lastPixel - pixelX - 2;
            do { /* pixelX loop */
              if (needData) {
                sData = pixVals[*sLoc & sMsk];
                needData = false;
                }
              temp |= SHIFTPIXEL(sData,b);
              ey -= dy;
              if (ey <= 0) {
                do sLoc += wb; while ((ey += ONE) <= 0);
		needData = true;
		}
              } while ((b -= 2) >= 0); /* end of pixel loop */
            switch (--units) {
              case 0: lastPixel = pixelXr; break;
              case -1:
                if (pixelXr == SCANUNIT) *destunit = temp;
		else {
                  temp LSHIFTEQ (SCANUNIT - bitr);
		  if (pixelX > 0) {
                    temp &= maskl;
		    temp |= *destunit & ~maskl;
		    }
                  *destunit = temp | (*destunit & ~maskr);
		  }
                goto NextPair;
	      }
            *destunit++ = temp;
            temp = 0; pixelX = 0;
            } /* end of units loop */
          }
        else if (IntPart(fxdSrcY) == IntPart(endY)) { /* horizontal source */
          register int b;
          register SCANTYPE temp;
          register Fixed ex, dx;
          register unsigned char sData;
	  register unsigned char sMsk = 0xC0 >> (sPix << 1);
	  temp = *destunit & ~maskl;
	  dx = sv.x;
	  needData = true;
	  if (dx > 0) ex = ONE - (fxdSrcX & LowMask);
          else ex = (fxdSrcX & LowMask) + 1;
          while (true) { /* units loop */
            b = lastPixel - pixelX - 2;
	    if (dx > 0) {
              do { /* pixelX loop */
		if (needData) {
                  sData = pixVals[*sLoc & sMsk];
		  needData = false;
		  }
                temp |= SHIFTPIXEL(sData,b);
                ex -= dx;
                if (ex <= 0) {
                  do {
		    sMsk >>= 2;
		    if (sMsk == 0) { ++sLoc; sMsk = 0xC0; }
		    } while ((ex += ONE) <= 0);
                  needData = true;
                  }
                } while ((b -= 2) >= 0); /* end of pixel loop */
              }
            else { /* dx <= 0 */
              do { /* pixelX loop */
		if (needData) {
                  sData = pixVals[*sLoc & sMsk];
		  needData = false;
		  }
                temp |= SHIFTPIXEL(sData,b);
                ex += dx;
                if (ex <= 0) {
                  do {
		    if (sMsk == 0xC0) { --sLoc; sMsk = 0x03; }
		    else sMsk <<= 2;
		    } while ((ex += ONE) <= 0);
                  needData = true;
                  }
                } while ((b -= 2) >= 0); /* end of pixel loop */
              }
            switch (--units) {
              case 0: lastPixel = pixelXr; break;
              case -1:
                if (pixelXr == SCANUNIT) *destunit = temp;
		else {
                  temp LSHIFTEQ (SCANUNIT - bitr);
		  if (pixelX > 0) {
                    temp &= maskl;
		    temp |= *destunit & ~maskl;
		    }
                  *destunit = temp | (*destunit & ~maskr);
		  }
                goto NextPair;
	      }
            *destunit++ = temp;
            temp = 0; pixelX = 0;
            } /* end of units loop */
          }
        else { /* general rotation case */
          register int b;
          register SCANTYPE temp;
          register Fixed ex, ey;
          register unsigned char sData;
	  register unsigned char sMsk = 0xC0 >> (sPix << 1);
	  Fixed dx, dy;
          temp = *destunit & ~maskl;
	  dx = sv.x;
	  needData = true;
	  ex = (dx > 0) ? ONE - (fxdSrcX & LowMask) :
				 (fxdSrcX & LowMask) + 1;
	  if (sv.y > 0) {
            dy = sv.y; wb = wbytes; ey = ONE - (fxdSrcY & LowMask); }
          else {
            dy = -sv.y; wb = -wbytes; ey = (fxdSrcY & LowMask) + 1; }
          while (true) { /* units loop */
            b = lastPixel - pixelX - 2;
	    if (dx > 0) {
              do { /* pixelX loop */
		if (needData) {
                  sData = pixVals[*sLoc & sMsk];
		  needData = false;
		  }
                temp |= SHIFTPIXEL(sData,b);
                ey -= dy;
                if (ey <= 0) {
                  do sLoc += wb; while ((ey += ONE) <= 0);
                  ex -= dx;
                  if (ex <= 0) do {
                    sMsk >>= 2;
		    if (sMsk == 0) { ++sLoc; sMsk = 0xC0; }
		    } while ((ex += ONE) <= 0);
                  needData = true;
                  }
                else {
                  ex -= dx;
                  if (ex <= 0) {
                    do {
                      sMsk >>= 2;
		      if (sMsk == 0) { ++sLoc; sMsk = 0xC0; }
                      } while ((ex += ONE) <= 0);
                    needData = true;
                    }
                  }
                } while ((b -= 2) >= 0); /* end of pixel loop */
              }
            else { /* dx <= 0 */
              do { /* pixelX loop */
		if (needData) {
                  sData = pixVals[*sLoc & sMsk];
		  needData = false;
		  }
                temp |= SHIFTPIXEL(sData,b);
                ey -= dy;
                if (ey <= 0) {
                  do sLoc += wb; while ((ey += ONE) <= 0);
                  ex += dx;
                  if (ex <= 0) do {
		    if (sMsk == 0xC0) { --sLoc; sMsk = 0x03; }
		    else sMsk <<= 2;
		    } while ((ex += ONE) <= 0);
                  needData = true;
                  }
                else {
                  ex += dx;
                  if (ex <= 0) {
                    do {
		    if (sMsk == 0xC0) { --sLoc; sMsk = 0x03; }
		    else sMsk <<= 2;
		      } while ((ex += ONE) <= 0);
                    needData = true;
		    }
                  }
                } while ((b -= 2) >= 0); /* end of pixel loop */
              }
            switch (--units) {
              case 0: lastPixel = pixelXr; break;
              case -1:
                if (pixelXr == SCANUNIT) *destunit = temp;
		else {
                  temp LSHIFTEQ (SCANUNIT - bitr);
		  if (pixelX > 0) {
                    temp &= maskl;
		    temp |= *destunit & ~maskl;
		    }
                  *destunit = temp | (*destunit & ~maskr);
		  }
                goto NextPair;
	      }
            *destunit++ = temp;
            temp = 0; pixelX = 0;
            }
          }
        NextPair: continue;
        }
      if (++y > ylast) break;
      if (t != NULL) {
        if (y == ylast) {
          txl = trap.l.xg + xoffset;
          txr = trap.g.xg + xoffset;
          }
        else {
          if (leftSlope) { txl = IntPart(trap.l.ix); trap.l.ix += trap.l.dx; }
          if (rightSlope) { txr = IntPart(trap.g.ix); trap.g.ix += trap.g.dx; }
          }
        }
      }
    if (t != NULL) t++;
    }
  (*args->procs->End)(args->data);
  }
