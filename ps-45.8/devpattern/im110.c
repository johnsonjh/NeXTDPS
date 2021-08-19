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
Paul Rovner: Wednesday, January 27, 1988 4:28:47 PM
Jim Sandman: Wed Feb  1 10:07:41 1989
Jack Newlin 21Dec89 write opaque alpha when none supplied
Pete Graff.  7/20/90  hacked for 12bit RGB (no dithering )

End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include DEVICE
#include DEVIMAGE
#include DEVPATTERN
#include EXCEPT

#include "imagepriv.h"
#define TWELVEBITHACK 1

private integer twoBitSample[4] = {0, 85, 170, 255};

typedef enum {
  s11, s21, s41, s81,
  s12I, s22I, s42I, s82I, s13I, s23I, s43I, s83I,
  s14I, s24I, s44I, s84I, s15I, s25I, s45I, s85I, 
  s12N, s22N, s42N, s82N, s13N, s23N, s43N, s83N,
  s14N, s24N, s44N, s84N, s15N, s25N, s45N, s85N, sBad
  } ImSourceType;

 
public procedure Im110(items, t, run, args)
  integer items;
  DevTrap *t;
  DevRun *run;
  ImageArgs *args; 
  {
  DevMarkInfo *info = args->markInfo;
  DevImage *image = args->image;
  DevImageSource *source = image->source;
  DevTrap trap;
  Cd c;
  integer data0, data1, data2, data3, data4;
  integer y, ylast, pairs, wbytes,
    xoffset, yoffset, txl, txr, xx, yy, units,
    bitr, srcXprv, srcYprv, lastPixel;
  DevShort *buffptr;
  DevFixedPoint sv, svy;
  Fixed fxdSrcX, fxdSrcY, fX, fY, srcLX, srcLY, srcGX, srcGY, endX, endY;
  boolean leftSlope, rightSlope;
  PSCANTYPE destunit, destbase;
#if TWELVEBITHACK
  unsigned short *sh_destunit, *sh_destbase;
  int twelvebit = (args->bitsPerPixel == 16);
#endif
  PCard8 tfrRed, tfrGreen, tfrBlue;
  ImSourceType sourceType = sBad;
  boolean alpha = image->unused, ucRemove = (source->nComponents == 4);
  boolean tfr = (image->transfer != NULL);
  integer nChannels = source->nComponents + (alpha ? 1 : 0);

  wbytes = source->wbytes;

#if TWELVEBITHACK
  if ((args->bitsPerPixel != SCANUNIT && !twelvebit) ||
    args->green.n <= 15 || args->blue.n <= 15 || args->red.n <= 15)
    RAISE(ecLimitCheck, (char *)NIL);
#else
  if (args->bitsPerPixel != SCANUNIT ||
    args->green.n <= 15 || args->blue.n <= 15 || args->red.n <= 15)
    RAISE(ecLimitCheck, (char *)NIL);
#endif
  if (!tfr)
    tfrRed = tfrGreen = tfrBlue = NULL;
  else {
    tfrRed = image->transfer->red;
    tfrGreen = image->transfer->green;
    tfrBlue = image->transfer->blue;
    };

  /* srcXprv srcYprv are integer coordinates of source pixel */
  srcXprv = image->info.sourcebounds.x.l;
  srcYprv = image->info.sourcebounds.y.l;
  /* srcLX, srcLY, srcGX, srcGY give fixed bounds of source in image space */
  srcLX = Fix(srcXprv);
  srcLY = Fix(srcYprv);
  srcGX = Fix(image->info.sourcebounds.x.g);
  srcGY = Fix(image->info.sourcebounds.y.g);
  /* image space fixed point vector for (1,0) in device */
  sv.x = pflttofix(&image->info.mtx->a);
  sv.y = pflttofix(&image->info.mtx->b);
  /* image space fixed point vector for (0,1) in device */
  svy.x = pflttofix(&image->info.mtx->c);
  svy.y = pflttofix(&image->info.mtx->d);
  /* if this is null then use raw source data */
  xoffset = info->offset.x;
  yoffset = info->offset.y;
  switch (source->bitspersample) {
    case 1:
      if (source->interleaved) {
        switch (nChannels) {
          case 1: sourceType = s11; break;
          case 2: sourceType = s12I; break;
          case 3: sourceType = s13I; break;
          case 4: sourceType = s14I; break;
          case 5: sourceType = s15I; break;
          }
        }
      else {
        switch (nChannels) {
          case 1: sourceType = s11; break;
          case 2: sourceType = s12N; break;
          case 3: sourceType = s13N; break;
          case 4: sourceType = s14N; break;
          case 5: sourceType = s15N; break;
          }
        }
      break;
    case 2:
      if (source->interleaved) {
        switch (nChannels) {
          case 1: sourceType = s21; break;
          case 2: sourceType = s22I; break;
          case 3: sourceType = s23I; break;
          case 4: sourceType = s24I; break;
          case 5: sourceType = s25I; break;
          }
        }
      else {
        switch (nChannels) {
          case 1: sourceType = s21; break;
          case 2: sourceType = s22N; break;
          case 3: sourceType = s23N; break;
          case 4: sourceType = s24N; break;
          case 5: sourceType = s25N; break;
          }
        }
      break;
    case 4:
      if (source->interleaved) {
        switch (nChannels) {
          case 1: sourceType = s41; break;
          case 2: sourceType = s42I; break;
          case 3: sourceType = s43I; break;
          case 4: sourceType = s44I; break;
          case 5: sourceType = s45I; break;
          }
        }
      else {
        switch (nChannels) {
          case 1: sourceType = s41; break;
          case 2: sourceType = s42N; break;
          case 3: sourceType = s43N; break;
          case 4: sourceType = s44N; break;
          case 5: sourceType = s45N; break;
          }
        }
      break;
    case 8:
      if (source->interleaved) {
        switch (nChannels) {
          case 1: sourceType = s81; break;
          case 2: sourceType = s82I; break;
          case 3: sourceType = s83I; break;
          case 4: sourceType = s84I; break;
          case 5: sourceType = s85I; break;
          }
        }
      else {
        switch (nChannels) {
          case 1: sourceType = s81; break;
          case 2: sourceType = s82N; break;
          case 3: sourceType = s83N; break;
          case 4: sourceType = s84N; break;
          case 5: sourceType = s85N; break;
          }
        }
      break;
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
#if TWELVEBITHACK
	  if(!twelvebit) {
	      destbase = (*args->procs->GetWriteScanline)(args->data,
							  y, xl, xr);
	      destunit = destbase + xl;
	  } else {
	      sh_destbase = (unsigned short *)
		  (*args->procs->GetWriteScanline)(args->data, y, xl, xr);
	      sh_destunit = sh_destbase + xl;
	  }
#else
          destbase = (*args->procs->GetWriteScanline)(args->data, y, xl, xr);
          destunit = destbase + xl;
#endif
          units = xr - xl;
          }
        { /* general rotation case */
          Fixed ex, ey;
          integer sData;
          integer srcPix;
	  unsigned char *sample0, *sample1, *sample2 , *sample3 , *sample4;
	  Fixed dx, dy, xStep, yStep;
	  boolean needData;
          sample0 = source->samples[0];
          sample1 = source->samples[1];
          sample2 = source->samples[2];
          sample3 = source->samples[3];
          sample4 = source->samples[4];
	  needData = true;
	  if (sv.x > 0) {
            dx = sv.x; xStep = ONE; ex = ONE - (fxdSrcX & LowMask); }
          else {
            dx = -sv.x; xStep = -ONE; ex = (fxdSrcX & LowMask) + 1; }
	  if (sv.y > 0) {
            dy = sv.y; yStep = ONE; ey = ONE - (fxdSrcY & LowMask); }
          else {
            dy = -sv.y; yStep = -ONE; ey = (fxdSrcY & LowMask) + 1; }
          while (units--) { /* units loop */
	    if (needData) {
	      unsigned char *srcLoc;
	      integer x, y, loc;
	      needData = false;
	      x = FTrunc(fxdSrcX) - image->info.sourceorigin.x;
	      y = FTrunc(fxdSrcY) - image->info.sourceorigin.y;
	      loc = y * wbytes;
	      switch (sourceType) {
		case s11: {
		  srcLoc = sample0 + loc + (x >> 3);
		  data0 = (*srcLoc & (0x80 >> (x & 7))) ? 255 : 0;
		  break;
		  }
		case s21: {
		  x <<= 1;
		  srcLoc = sample0 + loc + (x >> 3);
		  data0 = twoBitSample[3 & (*srcLoc >> (6 - (x & 7)))];
		  break;
		  }
		case s41: {
		  x <<= 2;
		  srcLoc = sample0 + loc + (x >> 3);
		  data0 = *srcLoc;
		  if ((x & 7) == 0)
		    data0 >>= 4;
		  data0 |= data0 << 4;
		  break;
		  }
		case s81: {
		  srcLoc = sample0 + loc + x;
		  data0 = *srcLoc;
		  break;
		  }
		case s12I: {
		  x *= 2;
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
		  break;
		  }
		case s22I: {
		  x *= 2;
		  x <<= 1;
		  srcLoc = sample0 + loc + (x >> 3);
		  srcPix = 6 - (x & 7);
		  sData = *srcLoc;
		  data0 = twoBitSample[3 & (sData >> srcPix)];
		  srcPix -= 2;
		  if (srcPix == 0) {
		    srcLoc++;
		    sData = *srcLoc;
		    srcPix = 6;
		    }
		  data1 = twoBitSample[3 & (sData >> srcPix)];
		  break;
		  }
		case s42I: {
		  x *= 2;
		  srcLoc = sample0 + loc + (x >> 1);
		  sData = *srcLoc;
		  data0 = sData >> 4;
		  data1 = sData & 0xf;
		  data0 |= data0 << 4;
		  data1 |= data1 << 4;
		  break;
		  }
		case s82I: {
		  srcLoc = sample0 + loc + x*2;
		  data0 = *srcLoc++;
		  data1 = *srcLoc;
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
		  if (srcPix == 0) {
		    srcLoc++;
		    sData = *srcLoc;
		    srcPix = 6;
		    }
		  data1 = twoBitSample[3 & (sData >> srcPix)];
		  srcPix -= 2;
		  if (srcPix == 0) {
		    srcLoc++;
		    sData = *srcLoc;
		    srcPix = 6;
		    }
		  data2 = twoBitSample[3 & (sData >> srcPix)];
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
		  break;
		  }
		case s83I: {
		  srcLoc = sample0 + loc + x*3;
		  data0 = *srcLoc++;
		  data1 = *srcLoc++;
		  data2 = *srcLoc;
		  break;
		  }
		case s14I: {
		  srcLoc = sample0 + loc + (x >> 1);
		  sData = *srcLoc;
		  if (x & 1) {
		    data0 = (8 & sData) ? 255 : 0;
		    data1 = (4 & sData) ? 255 : 0;
		    data2 = (2 & sData) ? 255 : 0;
		    data3 = (1 & sData) ? 255 : 0;
		    }
		  else {
		    data0 = (0x80 & sData) ? 255 : 0;
		    data1 = (0x40 & sData) ? 255 : 0;
		    data2 = (0x20 & sData) ? 255 : 0;
		    data3 = (0x10 & sData) ? 255 : 0;
		    }
		  break;
		  }
		case s24I: {
		  srcLoc = sample0 + loc + x;
		  sData = *srcLoc;
		  data0 = twoBitSample[sData >> 6];
		  data1 = twoBitSample[3 & (sData >> 4)];
		  data2 = twoBitSample[3 & (sData >> 2)];
		  data3 = twoBitSample[3 & sData];
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
		  break;
		  }
		case s84I: {
		  srcLoc = sample0 + loc + (x << 2);
		  data0 = *srcLoc++;
		  data1 = *srcLoc++;
		  data2 = *srcLoc++;
		  data3 = *srcLoc;
		  break;
		  }
		case s15I: {
		  x *= 5;
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
		  srcPix >>= 1;
		  if (srcPix == 0) {
		    srcLoc++;
		    sData = *srcLoc;
		     srcPix = 0x80;
		    }
		  data3 = (sData & srcPix) ? 255 : 0;
		  srcPix >>= 1;
		  if (srcPix == 0) {
		    srcLoc++;
		    sData = *srcLoc;
		     srcPix = 0x80;
		    }
		  data4 = (sData & srcPix) ? 255 : 0;
		  break;
		  }
		case s25I: {
		  x *= 5;
		  x <<= 1;
		  srcLoc = sample0 + loc + (x >> 3);
		  srcPix = 6 - (x & 7);
		  sData = *srcLoc;
		  data0 = twoBitSample[3 & (sData >> srcPix)];
		  srcPix -= 2;
		  if (srcPix == 0) {
		    srcLoc++;
		    sData = *srcLoc;
		    srcPix = 6;
		    }
		  data1 = twoBitSample[3 & (sData >> srcPix)];
		  srcPix -= 2;
		  if (srcPix == 0) {
		    srcLoc++;
		    sData = *srcLoc;
		    srcPix = 6;
		    }
		  data2 = twoBitSample[3 & (sData >> srcPix)];
		  srcPix -= 2;
		  if (srcPix == 0) {
		    srcLoc++;
		    sData = *srcLoc;
		    srcPix = 6;
		    }
		  data3 = twoBitSample[3 & (sData >> srcPix)];
		  srcPix -= 2;
		  if (srcPix == 0) {
		    srcLoc++;
		    sData = *srcLoc;
		    srcPix = 6;
		    }
		  data4 = twoBitSample[3 & (sData >> srcPix)];
		  break;
		  }
		case s45I: {
		  x *= 5;
		  srcLoc = sample0 + loc + (x >> 1);
		  if (x & 1) {
		    data0 = *srcLoc++ & 0xf;
		    sData = *srcLoc++;
		    data1 = sData >> 4;
		    data2 = sData & 0xf;
		    sData = *srcLoc;
		    data3 = sData >> 4;
		    data4 = sData & 0xf;
		    }
		  else {
		    sData = *srcLoc++;
		    data0 = sData >> 4;
		    data1 = sData & 0xf;
		    sData = *srcLoc++;
		    data2 = sData >> 4;
		    data3 = sData & 0xf;
		    data4 = *srcLoc >> 4;
		    }
		  data0 |= data0 << 4;
		  data1 |= data1 << 4;
		  data2 |= data2 << 4;
		  data3 |= data3 << 4;
		  data4 |= data4 << 4;
		  break;
		  }
		case s85I: {
		  srcLoc = sample0 + loc + x*5;
		  data0 = *srcLoc++;
		  data1 = *srcLoc++;
		  data2 = *srcLoc++;
		  data3 = *srcLoc++;
		  data4 = *srcLoc;
		  break;
		  }
		case s12N: {
		  loc += (x >> 3);
		  srcPix = 0x80 >> (x & 7);
		  data0 = (*(sample0 + loc) & srcPix) ? 255 : 0;
		  data1 = (*(sample1 + loc) & srcPix) ? 255 : 0;
		  break;
		  }
		case s22N: {
		  x <<= 1;
		  loc += (x >> 3);
		  srcPix = 6 - (x & 7);
		  data0 = twoBitSample[(*(sample0+loc) >> srcPix) & 3];
		  data1 = twoBitSample[(*(sample1+loc) >> srcPix) & 3];
		  break;
		  }
		case s42N: {
		  loc += x >> 1;
		  if (x & 1) {
		    data0 = *(sample0 + loc) & 0xf;
		    data1 = *(sample1 + loc) & 0xf;
		    }
		  else {
		    data0 = *(sample0 + loc) >> 4;
		    data1 = *(sample1 + loc) >> 4;
		    }
		  data0 |= data0 << 4;
		  data1 |= data1 << 4;
		  break;
		  }
		case s82N: {
		  loc += x;
		  data0 = *(sample0 + loc);
		  data1 = *(sample1 + loc);
		  break;
		  }
		case s13N: {
		  loc += (x >> 3);
		  srcPix = 0x80 >> (x & 7);
		  data0 = (*(sample0 + loc) & srcPix) ? 255 : 0;
		  data1 = (*(sample1 + loc) & srcPix) ? 255 : 0;
		  data2 = (*(sample2 + loc) & srcPix) ? 255 : 0;
		  break;
		  }
		case s23N: {
		  x <<= 1;
		  loc += (x >> 3);
		  srcPix = 6 - (x & 7);
		  data0 = twoBitSample[(*(sample0+loc) >> srcPix) & 3];
		  data1 = twoBitSample[(*(sample1+loc) >> srcPix) & 3];
		  data2 = twoBitSample[(*(sample2+loc) >> srcPix) & 3];
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
		  break;
		  }
		case s83N: {
		  loc += x;
		  data0 = *(sample0 + loc);
		  data1 = *(sample1 + loc);
		  data2 = *(sample2 + loc);
		  break;
		  }
		case s14N: {
		  loc += (x >> 3);
		  srcPix = 0x80 >> (x & 7);
		  data0 = (*(sample0 + loc) & srcPix) ? 255 : 0;
		  data1 = (*(sample1 + loc) & srcPix) ? 255 : 0;
		  data2 = (*(sample2 + loc) & srcPix) ? 255 : 0;
		  data3 = (*(sample3 + loc) & srcPix) ? 255 : 0;
		  break;
		  }
		case s24N: {
		  x <<= 1;
		  loc += (x >> 3);
		  srcPix = 6 - (x & 7);
		  data0 = twoBitSample[(*(sample0+loc) >> srcPix) & 3];
		  data1 = twoBitSample[(*(sample1+loc) >> srcPix) & 3];
		  data2 = twoBitSample[(*(sample2+loc) >> srcPix) & 3];
		  data3 = twoBitSample[(*(sample3+loc) >> srcPix) & 3];
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
		  break;
		  }
		case s84N: {
		  loc += x;
		  data0 = *(sample0 + loc);
		  data1 = *(sample1 + loc);
		  data2 = *(sample2 + loc);
		  data3 = *(sample3 + loc);
		  break;
		  }
		case s15N: {
		  loc += (x >> 3);
		  srcPix = 0x80 >> (x & 7);
		  data0 = (*(sample0 + loc) & srcPix) ? 255 : 0;
		  data1 = (*(sample1 + loc) & srcPix) ? 255 : 0;
		  data2 = (*(sample2 + loc) & srcPix) ? 255 : 0;
		  data3 = (*(sample3 + loc) & srcPix) ? 255 : 0;
		  data4 = (*(sample4 + loc) & srcPix) ? 255 : 0;
		  break;
		  }
		case s25N: {
		  x <<= 1;
		  loc += (x >> 3);
		  srcPix = 6 - (x & 7);
		  data0 = twoBitSample[(*(sample0+loc) >> srcPix) & 3];
		  data1 = twoBitSample[(*(sample1+loc) >> srcPix) & 3];
		  data2 = twoBitSample[(*(sample2+loc) >> srcPix) & 3];
		  data3 = twoBitSample[(*(sample3+loc) >> srcPix) & 3];
		  data4 = twoBitSample[(*(sample4+loc) >> srcPix) & 3];
		  break;
		  }
		case s45N: {
		  loc += x >> 1;
		  if (x & 1) {
		    data0 = *(sample0 + loc) & 0xf;
		    data1 = *(sample1 + loc) & 0xf;
		    data2 = *(sample2 + loc) & 0xf;
		    data3 = *(sample3 + loc) & 0xf;
		    data4 = *(sample4 + loc) & 0xf;
		    }
		  else {
		    data0 = *(sample0 + loc) >> 4;
		    data1 = *(sample1 + loc) >> 4;
		    data2 = *(sample2 + loc) >> 4;
		    data3 = *(sample3 + loc) >> 4;
		    data4 = *(sample4 + loc) >> 4;
		    }
		  data0 |= data0 << 4;
		  data1 |= data1 << 4;
		  data2 |= data2 << 4;
		  data3 |= data3 << 4;
		  data4 |= data4 << 4;
		  break;
		  }
		case s85N: {
		  loc += x;
		  data0 = *(sample0 + loc);
		  data1 = *(sample1 + loc);
		  data2 = *(sample2 + loc);
		  data3 = *(sample3 + loc);
		  data4 = *(sample4 + loc);
		  break;
		  }
		}
	      if (nChannels<3) {
		if (!alpha)
		  data1 = 255;
	        if (!tfr)
		  data0 = (data0<<24) | (data0<<16) | (data0<<8) | data1;
		else {
		  data3 = ((tfrBlue) ? tfrBlue[data0] : data0) << 8;
		  data2 = ((tfrGreen) ? tfrGreen[data0] : data0) << 16;
		  data0 = ((tfrRed) ? tfrRed[data0] : data0) << 24;
		  data0 |= data2 | data3 | data1;
		}
	      } else {
		if (ucRemove) {
		  data0 = 255 - data0 - data3;
		  data1 = 255 - data1 - data3;
		  data2 = 255 - data2 - data3;
		  if (data0 < 0) data0 = 0;
		  if (data1 < 0) data1 = 0;
		  if (data2 < 0) data2 = 0;
		  if (alpha)
		    data3 = data4;
		}
		if (!alpha)
		  data3 = 255;
	        if (tfr) {
		  if (tfrRed)
		    data0 = tfrRed[data0];
		  if (tfrGreen)
		    data1 = tfrGreen[data1];
		  if (tfrBlue)
		    data2 = tfrBlue[data2];
		}
#if TWELVEBITHACK
		data0 = (twelvebit)
		    ? (((data0<<8)&0xf000) | ((data1<<4)&0xf00) |
		       (data2&0xf0) | (data3>>4))
		    : (data0<<24) | (data1<<16) | (data2<<8) | data3;
#else
		data0 = (data0<<24) | (data1<<16) | (data2<<8) | data3;
#endif
	      }
	    }
#if TWELVEBITHACK
	    if(twelvebit)
		*sh_destunit++ = data0;
	    else
		*destunit++ = data0;		
#else
	    *destunit++ = data0;
#endif
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
	    }
	  }
        } /* end of pairs loop */
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
  } /* end Image4 */



