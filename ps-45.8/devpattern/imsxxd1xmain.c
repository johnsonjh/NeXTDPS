/* PostScript generic image module

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

Original version: Doug Brotz: Sept. 13, 1983
Edit History:
Doug Brotz: Mon Aug 25 14:41:39 1986
Chuck Geschke: Mon Aug  6 23:25:04 1984
Ed Taft: Tue Jul  3 12:55:59 1984
Ivor Durham: Wed Mar 23 13:11:32 1988
Bill Paxton: Tue Mar 29 09:25:41 1988
Paul Rovner: Fri Dec 29 11:07:01 1989
Jim Sandman: Wed Mar 28 15:50:45 1990

End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include DEVICETYPES
#include DEVIMAGE
#include DEVPATTERN
#include EXCEPT
#include FP
#include PSLIB


#include "imagepriv.h"
#include "patternpriv.h"

private Fixed Wr, Wg, Wb;
private boolean initedReals;

private integer twoBitSample[4] = {0, 85, 170, 255};

typedef enum {
  s11, s21, s41, s81, s13I, s23I, s43I, s83I, s14I, s24I, s44I, s84I,
  s13N, s23N, s43N, s83N, s14N, s24N, s44N, s84N, sBad
  } ImSourceType;
  
public procedure PROCNAME(items, t, run, args)
  /* 4 bit per pixel source */
  integer items;
  DevTrap *t;
  DevRun *run;
  ImageArgs *args; 
  {
  DevMarkInfo *info = args->markInfo;
  DevImage *image = args->image;
  DevImageSource *source = image->source;
  register unsigned char *sLoc, *screenElt, *eScrnRow;
  unsigned char *srcLoc;
  DevTrap trap;
  Cd c;
  DevScreen *screen = info->halftone->white;
  DevShort *buffptr;
  integer y, ylast, scrnRow, mpwCBug, scrnCol, pairs,
    xoffset, yoffset, txl, txr, wbytes, screenwidth, xx, yy, units,
    pixelX, pixelXr, bitr, srcXprv, srcYprv, lastPixel, wb, srcPix;
  DevPoint grayOrigin;
  DevFixedPoint sv, svy;
  Fixed fxdSrcX, fxdSrcY, fX, fY, srcLX, srcLY, srcGX, srcGY, endX, endY;
  boolean leftSlope, rightSlope;
  register PSCANTYPE pixVals, pixThresholds;
  PSCANTYPE destunit, destbase;
  SCANTYPE maskl, maskr;
  unsigned char *sScrnRow;
  ImSourceType sourceType = sBad;
  DECLAREVARS;
  
  
  SETUPPARAMS(args);

  if (!ValidateTA(screen)) CantHappen();
  if (!initedReals) {
    real r;
    r = 0.3;
    Wr = pflttofix(&r);
    r = 0.59;
    Wg = pflttofix(&r);
    Wb = ONE - Wr - Wg;
    initedReals = true;
    }
    
  pixVals = GetPixBuffers();
  pixThresholds = pixelThresholds;

  {
  DevImage *im = args->image;
  DevImageSource *source = im->source;
  wbytes = source->wbytes;
  /* srcXprv srcYprv are integer coordinates of source pixel */
  srcXprv = im->info.sourcebounds.x.l;
  srcYprv = im->info.sourcebounds.y.l;
  /* srcLX, srcLY, srcGX, srcGY give fixed bounds of source in image space */
  srcLX = Fix(srcXprv);
  srcLY = Fix(srcYprv);
  srcGX = Fix(im->info.sourcebounds.x.g);
  srcGY = Fix(im->info.sourcebounds.y.g);
  /* image space fixed point vector for (1,0) in device */
  sv.x = pflttofix(&im->info.mtx->a);
  sv.y = pflttofix(&im->info.mtx->b);
  /* image space fixed point vector for (0,1) in device */
  svy.x = pflttofix(&im->info.mtx->c);
  svy.y = pflttofix(&im->info.mtx->d);
  {
  PCard8 tfrFcn = (im->transfer == NULL) ? NULL : im->transfer->white;
  Set8BitValues(
    args->gray, tfrFcn, pixVals, pixThresholds,
    &source->decode[IMG_GRAY_SAMPLES]);
  } /* end of register block */
  xoffset = info->offset.x;
  yoffset = info->offset.y;
  switch (source->bitspersample) {
    case 1:
      if (source->interleaved) {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s11; break;
          case DEVRGB_COLOR_SPACE: sourceType = s13I; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s14I; break;
          }
        }
      else {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s11; break;
          case DEVRGB_COLOR_SPACE: sourceType = s13N; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s14N; break;
          }
        }
      break;
    case 2:
      if (source->interleaved) {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s21; break;
          case DEVRGB_COLOR_SPACE: sourceType = s23I; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s24I; break;
          }
        }
      else {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s21; break;
          case DEVRGB_COLOR_SPACE: sourceType = s23N; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s24N; break;
          }
        }
      break;
    case 4:
      if (source->interleaved) {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s41; break;
          case DEVRGB_COLOR_SPACE: sourceType = s43I; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s44I; break;
          }
        }
      else {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s41; break;
          case DEVRGB_COLOR_SPACE: sourceType = s43N; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s44N; break;
          }
        }
      break;
    case 8:
      if (source->interleaved) {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s81; break;
          case DEVRGB_COLOR_SPACE: sourceType = s83I; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s84I; break;
          }
        }
      else {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s81; break;
          case DEVRGB_COLOR_SPACE: sourceType = s83N; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s84N; break;
          }
        }
      break;
    }
  }
  if (sourceType == sBad)
    CantHappen();
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
    grayOrigin.x = info->offset.x + info->screenphase.x;
    grayOrigin.y = info->offset.y + info->screenphase.y;
    scrnRow = y - grayOrigin.y;
    /* scrnRow can be negative so cannot use % directly */
    mpwCBug = (scrnRow / screen->height);
    mpwCBug = mpwCBug * screen->height;
    scrnRow = scrnRow - mpwCBug;
/* WAS    scrnRow = scrnRow - (scrnRow / screen->height) * screen->height; */
    if (scrnRow < 0) scrnRow += screen->height;
    screenwidth = screen->width;
    sScrnRow = screen->thresholds + scrnRow * screenwidth;
    /* fX and fY correspond to integer coords (xx, yy) in device space */
    xx = xoffset; yy = y;
    c.x = 0.5; c.y = y - yoffset + 0.5; /* add .5 for center sampling */
    TfmPCd(c, args->image->info.mtx, &c);
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
          pixelX = (xl & SMASK) << LOG2BPP;
          pixelXr = (xr & SMASK) << LOG2BPP;
          scrnCol = xl - grayOrigin.x;
          { register int unitl;
            unitl = xl >> SSHIFT;
            destunit = destbase + unitl;
            units = (xr >> SSHIFT) - unitl;
            }
          }
        maskl = leftBitArray[pixelX];
        if (pixelXr == 0) {
	  units--; pixelXr = SCANUNIT; maskr = -1; bitr = SCANUNIT;
	  }
        else {
	  bitr = pixelXr; maskr = rightBitArray[bitr];
	  };
        eScrnRow = sScrnRow + screenwidth;
        { register int m, k;
          m = screenwidth; k = scrnCol; /* k may be negative so cannot use % */
          k = k - (k/m)*m; if (k < 0) k += m;
          screenElt = sScrnRow + k;
          }
	lastPixel = (units > 0) ? SCANUNIT : pixelXr;
        { 
          register integer b, g, t;
          register SCANTYPE temp;
          Fixed ex, ey;
          integer sData;
          integer data0, data1, data2, data3;
          unsigned char *sample0, *sample1, *sample2, *sample3;
	  Fixed dx, dy, xStep, yStep;
	  boolean needData;
	  integer sOrigX = args->image->info.sourceorigin.x;
	  integer sOrigY = args->image->info.sourceorigin.y;
          temp = *destunit & ~maskl;
          {
          DevImageSource *source = args->image->source;
          sample0 = source->samples[0];
          sample1 = source->samples[1];
          sample2 = source->samples[2];
          sample3 = source->samples[3];
          }
	  needData = true;
	  if (sv.x > 0) {
            dx = sv.x; xStep = ONE; ex = ONE - (fxdSrcX & LowMask); }
          else {
            dx = -sv.x; xStep = -ONE; ex = (fxdSrcX & LowMask) + 1; }
	  if (sv.y > 0) {
            dy = sv.y; yStep = ONE; ey = ONE - (fxdSrcY & LowMask); }
          else {
            dy = -sv.y; yStep = -ONE; ey = (fxdSrcY & LowMask) + 1; }
          while (true) { /* units loop */
            b = lastPixel - pixelX - BPP;
	    do { /* pixelX loop */
	      if (needData) {
	        unsigned char *srcLoc;
	        integer x, y, loc;
	        needData = false;
	        x = FTrunc(fxdSrcX) - sOrigX;
	        y = FTrunc(fxdSrcY) - sOrigY;
		loc = y * wbytes;
		switch (sourceType) {
		  case s11: {
		    srcLoc = sample0 + loc + (x >> 3);
		    sData = (*srcLoc & (0x80 >> (x & 7))) ? 255 : 0;
		    break;
		    }
		  case s21: {
		    x <<= 1;
		    srcLoc = sample0 + loc + (x >> 3);
		    sData = twoBitSample[(*srcLoc >> (6 - (x & 7))) & 3];
		    break;
		    }
		  case s41: {
		    x <<= 2;
		    srcLoc = sample0 + loc + (x >> 3);
		    sData = (*srcLoc >> (4 - (x & 7))) & 0xf;
		    sData |= sData << 4;
		    break;
		    }
		  case s81: {
		    srcLoc = sample0 + loc + x;
		    sData = *srcLoc;
		    break;
		    }
		  case s13I: {
		    x *= 3;
		    srcLoc = sample0 + loc + (x >> 3);
		    srcPix = 0x80 >> (x & 7);
		    sData = *srcLoc;
		    data0 = (sData & srcPix) ? 255 : 0;
		    srcPix >>= 1;
		    if (srcPix == 0) {
		      srcLoc++;
		      sData = *srcLoc;
		      srcPix = 0x80;
		      }
		    data1 = (sData & srcPix) ? 255 : 0;
		    srcPix >>= 1;
		    if (srcPix == 0) {
		      srcLoc++;
		      sData = *srcLoc;
 		      srcPix = 0x80;
		      }
		    data2 = (sData & srcPix) ? 255 : 0;
		    sData = FRound(Wr * data0 + Wg * data1 + Wb * data2 );
		    if (sData > 255) sData = 255;
		    break;
		    }
		  case s23I: {
		    x *= 3;
		    x <<= 1;
		    srcLoc = sample0 + loc + (x >> 3);
		    srcPix = 6 - (x & 7);
		    sData = *srcLoc;
		    data0 = twoBitSample[3 & (sData >> srcPix)];
		    srcPix -= 2;
		    if (srcPix < 0) {
		      srcLoc++;
		      sData = *srcLoc;
		      srcPix = 6;
		      }
		    data1 = twoBitSample[3 & (sData >> srcPix)];
		    srcPix -= 2;
		    if (srcPix < 0) {
		      srcLoc++;
		      sData = *srcLoc;
 		      srcPix = 6;
		      }
		    data2 = twoBitSample[3 & (sData >> srcPix)];
		    sData = FRound(Wr * data0 + Wg * data1 + Wb * data2);
		    if (sData > 255) sData = 255;
		    break;
		    }
		  case s43I: {
		    x *= 3;
		    srcLoc = sample0 + loc + (x >> 1);
		    if (x & 1) {
		      data0 = *srcLoc++ & 0xf;
		      sData = *srcLoc;
		      data1 = sData >> 4;
		      data2 = sData & 0xf;
		      }
		    else {
		      sData = *srcLoc++;
		      data0 = sData >> 4;
		      data1 = sData & 0xf;
		      data2 = *srcLoc >> 4;
		      }
		    data0 |= data0 << 4;
		    data1 |= data1 << 4;
		    data2 |= data2 << 4;
		    sData = FRound(Wr * data0 + Wg * data1 + Wb * data2);
		    if (sData > 255) sData = 255;
		    break;
		    }
		  case s83I: {
		    srcLoc = sample0 + loc + x*3;
		    data0 = *srcLoc++;
		    data1 = *srcLoc++;
		    data2 = *srcLoc;
		    sData = FRound(Wr * data0 + Wg * data1 + Wb * data2);
		    if (sData > 255) sData = 255;
		    break;
		    }
		  case s14I: {
		    srcLoc = sample0 + loc + (x >> 1);
		    sData = *srcLoc;
		    if (x & 1) {
		      data0 = (8 & sData) ? 0 : 255;
		      data1 = (4 & sData) ? 0 : 255;
		      data2 = (2 & sData) ? 0 : 255;
		      data3 = (1 & sData) ? 255 : 0;
		      }
		    else {
		      data0 = (0x80 & sData) ? 0 : 255;
		      data1 = (0x40 & sData) ? 0 : 255;
		      data2 = (0x20 & sData) ? 0 : 255;
		      data3 = (0x10 & sData) ? 255 : 0;
		      }
		    sData = FRound(
		      Wr * data0 + Wg * data1 + Wb * data2) - data3;
		    if (sData < 0) sData = 0;
		    if (sData > 255) sData = 255;
		    break;
		    }
		  case s24I: {
		    srcLoc = sample0 + loc + x;
		    sData = *srcLoc;
		    data0 = twoBitSample[sData >> 6];
		    data1 = twoBitSample[3 & (sData >> 4)];
		    data2 = twoBitSample[3 & (sData >> 2)];
		    data3 = twoBitSample[3 & sData];
		    sData = FRound(
		      Wr * (255 - data0) + Wg * (255 - data1) +
		      Wb * (255 - data2)) - data3;
		    if (sData < 0) sData = 0;
		    if (sData > 255) sData = 255;
		    break;
		    }
		  case s44I: {
		    srcLoc = sample0 + loc + (x << 1);
		    sData = *srcLoc++;
		    data0 = sData >> 4;
		    data1 = sData & 0xf;
		    sData = *srcLoc;
		    data2 = sData >> 4;
		    data3 = sData & 0xf;
		    data0 |= data0 << 4;
		    data1 |= data1 << 4;
		    data2 |= data2 << 4;
		    data3 |= data3 << 4;
		    sData = FRound(
		      Wr * (255 - data0) + Wg * (255 - data1) +
		      Wb * (255 - data2)) - data3;
		    if (sData < 0) sData = 0;
		    if (sData > 255) sData = 255;
		    break;
		    }
		  case s84I: {
		    srcLoc = sample0 + loc + (x << 2);
		    data0 = *srcLoc++;
		    data1 = *srcLoc++;
		    data2 = *srcLoc++;
		    data3 = *srcLoc;
		    sData = FRound(
		      Wr * (255 - data0) + Wg * (255 - data1) +
		      Wb * (255 - data2)) - data3;
		    if (sData < 0) sData = 0;
		    if (sData > 255) sData = 255;
		    break;
		    }
		  case s13N: {
		    loc += (x >> 3);
		    srcPix = 0x80 >> (x & 7);
		    data0 = (*(sample0 + loc) & srcPix) ? 255 : 0;
		    data1 = (*(sample1 + loc) & srcPix) ? 255 : 0;
		    data2 = (*(sample2 + loc) & srcPix) ? 255 : 0;
		    sData = FRound(Wr * data0 + Wg * data1 + Wb * data2);
		    if (sData > 255) sData = 255;
		    break;
		    }
		  case s23N: {
		    x <<= 1;
		    loc += (x >> 3);
		    srcPix = 6 - (x & 7);
		    data0 = 
		      twoBitSample[(*(sample0+loc) >> srcPix) & 3];
		    data1 =
		      twoBitSample[(*(sample1+loc) >> srcPix) & 3];
		    data2 =
		      twoBitSample[(*(sample2+loc) >> srcPix) & 3];
		    sData = FRound(Wr * data0 + Wg * data1 + Wb * data2);
		    if (sData > 255) sData = 255;
		    break;
		    }
		  case s43N: {
		    loc += x >> 1;
		    if (x & 1) {
		      data0 = *(sample0 + loc) & 0xf;
		      data1 = *(sample1 + loc) & 0xf;
		      data2 = *(sample2 + loc) & 0xf;
		      }
		    else {
		      data0 = *(sample0 + loc) >> 4;
		      data1 = *(sample1 + loc) >> 4;
		      data2 = *(sample2 + loc) >> 4;
		      }
		    data0 |= data0 << 4;
		    data1 |= data1 << 4;
		    data2 |= data2 << 4;
		    sData = FRound(Wr * data0 + Wg * data1 + Wb * data2);
		    if (sData > 255) sData = 255;
		    break;
		    }
		  case s83N: {
		    loc += x;
		    data0 = *(sample0 + loc);
		    data1 = *(sample1 + loc);
		    data2 = *(sample2 + loc);
		    sData = FRound(Wr * data0 + Wg * data1 + Wb * data2);
		    if (sData > 255) sData = 255;
		    break;
		    }
		  case s14N: {
		    loc += (x >> 3);
		    srcPix = 0x80 >> (x & 7);
		    data0 = (*(sample0 + loc) & srcPix) ? 0 : 255;
		    data1 = (*(sample1 + loc) & srcPix) ? 0 : 255;
		    data2 = (*(sample2 + loc) & srcPix) ? 0 : 255;
		    data3 = (*(sample3 + loc) & srcPix) ? 255 : 0;
		    sData = FRound(
		      Wr * data0 + Wg * data1 + Wb * data2) - data3;
		    if (sData < 0) sData = 0;
		    if (sData > 255) sData = 255;
		    break;
		    }
		  case s24N: {
		    x <<= 1;
		    loc += (x >> 3);
		    srcPix = 6 - (x & 7);
		    data0 = 
		      twoBitSample[(*(sample0+loc) >> srcPix) & 3];
		    data1 =
		      twoBitSample[(*(sample1+loc) >> srcPix) & 3];
		    data2 =
		      twoBitSample[(*(sample2+loc) >> srcPix) & 3];
		    data3 =
		      twoBitSample[(*(sample3+loc) >> srcPix) & 3];
		    sData = FRound(
		      Wr * (255 - data0) + Wg * (255 - data1) +
		      Wb * (255 - data2)) - data3;
		    if (sData < 0) sData = 0;
		    if (sData > 255) sData = 255;
		    break;
		    }
		  case s44N: {
		    loc += x >> 1;
		    if (x & 1) {
		      data0 = *(sample0 + loc) & 0xf;
		      data1 = *(sample1 + loc) & 0xf;
		      data2 = *(sample2 + loc) & 0xf;
		      data3 = *(sample3 + loc) & 0xf;
		      }
		    else {
		      data0 = *(sample0 + loc) >> 4;
		      data1 = *(sample1 + loc) >> 4;
		      data2 = *(sample2 + loc) >> 4;
		      data3 = *(sample3 + loc) >> 4;
		      }
		    data0 |= data0 << 4;
		    data1 |= data1 << 4;
		    data2 |= data2 << 4;
		    data3 |= data3 << 4;
		    sData = FRound(
		      Wr * (255 - data0) + Wg * (255 - data1) +
		      Wb * (255 - data2)) - data3;
		    if (sData < 0) sData = 0;
		    if (sData > 255) sData = 255;
		    break;
		    }
		  case s84N: {
		    loc += x;
		    data0 = *(sample0 + loc);
		    data1 = *(sample1 + loc);
		    data2 = *(sample2 + loc);
		    data3 = *(sample3 + loc);
		    sData = FRound(
		      Wr * (255 - data0) + Wg * (255 - data1) +
		      Wb * (255 - data2)) - data3;
		    if (sData < 0) sData = 0;
		    if (sData > 255) sData = 255;
		    break;
		    }
		  }
		g = pixVals[sData];
		t = pixThresholds[sData];
	        }
	      if (t < *screenElt++)
	        temp |= SHIFTPIXEL(g,b);
	      else temp |= SHIFTPIXEL((g - PIXDELTA),b);
	      if (screenElt >= eScrnRow) screenElt -= screenwidth;
	      ey -= dy;
	      if (ey <= 0) {
		do fxdSrcY += yStep; while ((ey += ONE) <= 0);
		needData = true;
		}
	      ex -= dx;
	      if (ex <= 0) {
		do fxdSrcX += xStep; while ((ex += ONE) <= 0);
		needData = true;
		}
	      } while ((b -= BPP) >= 0); /* end of pixel loop */
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
      if (++scrnRow == screen->height) {
        scrnRow = 0; sScrnRow = screen->thresholds; }
      else sScrnRow += screenwidth;
      }
    if (t != NULL) t++;
    }
  (*args->procs->End)(args->data);
  } /* end Image4 */

