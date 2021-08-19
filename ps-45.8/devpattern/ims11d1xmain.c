/* PostScript generic image module

Copyright (c) 1983, '84, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
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
Ivor Durham: Sun Feb  7 14:02:56 1988
Bill Paxton: Tue Mar 29 09:20:13 1988
Paul Rovner: Monday, November 23, 1987 10:11:58 AM
Jim Sandman: Wed Aug  2 12:16:07 1989
Joe Pasqua: Tue Feb 28 11:26:06 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include DEVIMAGE
#include DEVPATTERN
#include EXCEPT

#include "imagepriv.h"

#define MAXCOLOR 255

/* ForePaintType */
typedef int ForePaintType;
#define blackUp 0
#define whiteUp 1
#define gray1Up 2
#define grayNUp 3
#define blackDown 4
#define whiteDown 5
#define gray1Down 6
#define grayNDown 7

public procedure PROCNAME(items, t, run, args)
  /* 1 bit per pixel source */
  integer items;
  DevTrap *t;
  DevRun *run;
  ImageArgs *args; 
  {
  DevMarkInfo info;
  DevTrap trap;
  DevShort *buffptr;
  integer y, xl, xr, ylast, scrnRow, pairs, xoffset, yoffset;
  integer txl, txr, xx, yy;
  Fixed fX, fY, fxdSrcX, fxdSrcY, srcLX, srcLY, srcGX, srcGY, endX, endY;
  DevPoint grayOrigin;
  DevFixedPoint sv, svy;
  boolean leftSlope, rightSlope, isMask;
  PSCANTYPE destbase;
  PSCANTYPE grayPtr;
  integer srcX, srcY, diff, srcXprv, srcYprv, k, m;
  Cd c;
  integer wbytes, x0, x1, cb, srcBit;
  uchar foreground, background;
  PatternData forePatData, backPatData;
  DevColorVal foreColor, backColor;
  PSCANTYPE foreRowStart;
  PSCANTYPE foreRowEnd;
  uchar *srcLoc;
  boolean invert;
  SCANTYPE *onebit;
  ForePaintType paintType;
  DECLAREVARS;
   
  SETUPPARAMS(args);

  info = *args->markInfo;
  
  /* the following source is not dependent on bits per pixel.
     Appropriate macros have been defined to remove dependencies. */
  
  onebit = deepPixOnes[LOG2BPP];

  {
  DevImage *im = args->image;
  uchar *tfrGry;
  /* srcXprv srcYprv are integer coordinates of source pixel */
  srcXprv = im->info.sourcebounds.x.l;
  srcYprv = im->info.sourcebounds.y.l;
  /* srcLX, srcLY, srcGX, srcGY give fixed bounds of source in image space */
  srcLX = Fix(srcXprv);
  srcLY = Fix(srcYprv);
  srcGX = Fix(im->info.sourcebounds.x.g);
  srcGY = Fix(im->info.sourcebounds.y.g);
  /* srcLoc points to source pixel at location (srcXprv, srcYprv) */
  srcLoc = im->source->samples[IMG_GRAY_SAMPLES];
  wbytes = im->source->wbytes;
  srcLoc += (srcYprv - im->info.sourceorigin.y) * wbytes;
  srcLoc += ((srcXprv - im->info.sourceorigin.x) >> 3);
  srcBit = (srcXprv - im->info.sourceorigin.x) & 7;
  /* image space fixed point vector for (1,0) in device */
  sv.x = pflttofix(&im->info.mtx->a);
  sv.y = pflttofix(&im->info.mtx->b);
  /* image space fixed point vector for (0,1) in device */
  svy.x = pflttofix(&im->info.mtx->c);
  svy.y = pflttofix(&im->info.mtx->d);

  isMask = im->imagemask;
  tfrGry = (im->transfer == NULL) ? NULL : im->transfer->white;
  xoffset = info.offset.x;
  yoffset = info.offset.y;
  
  (*args->procs->Begin)(args->data);
  
  if (isMask) { /* imagemask operator */
    invert = !(im->invert);
    foreColor = *((DevColorVal *)&info.color);
    }
  else { /* 1 bit per pixel image */
    if (tfrGry == NULL) {
      background = MAXCOLOR;
      foreground = 0;
      invert = true;
      }
    else {
      if (tfrGry[MAXCOLOR] == 255) {
	background = 255;
	foreground = tfrGry[0];
	invert = true;
	}
      else if (tfrGry[0] == 255) {
	background = 255;
	foreground = tfrGry[MAXCOLOR];
	invert = false;
	}
      else if (tfrGry[MAXCOLOR] == 0) {
	background = 0;
	foreground = tfrGry[0];
	invert = true;
	}
      else {
	background = tfrGry[0];
	foreground = tfrGry[MAXCOLOR];
	invert = false;
	}
      }
    foreColor.red = foreColor.green = foreColor.blue = foreColor.white =
      foreground;
    backColor.red = backColor.green = backColor.blue = backColor.white =
      background;
    info.color = *((DevColor *)&backColor);
    SetupPattern(args->pattern, &info, &backPatData);
    }
  }
    
  info.color = *((DevColor *)&foreColor);
  SetupPattern(args->pattern, &info, &forePatData);
  if (forePatData.constant && forePatData.value == LASTSCANVAL)
    paintType = blackUp;
  else if (forePatData.constant && forePatData.value == 0) 
    paintType = whiteUp;
  else if (forePatData.width == 1)
    paintType = gray1Up;
  else
    paintType = grayNUp;
  if (sv.x < 0) paintType += 4;
  
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

    foreRowStart = GetPatternRow(&info, &forePatData, (integer)y);
    foreRowEnd = foreRowStart + forePatData.width;
    
    xx = xoffset; yy = y;
    c.x = 0.5; c.y = y - yoffset + 0.5; /* add .5 for center sampling */
    TfmPCd(c, args->image->info.mtx, &c);
    fX = pflttofix(&c.x); fY = pflttofix(&c.y);
    while (true) {
      pairs = (run != NULL) ? *(buffptr++) : 1;
      while (--pairs >= 0) {
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
        if (isMask)
          destbase =
            (*args->procs->GetReadWriteScanline)(args->data, y, xl, xr);
        else { /* paint xl to xr with background color */
          register PSCANTYPE destunit;
	  register int units;
	  register SCANTYPE maskl, maskr;
	  register int unitl;
          unitl = xl >> SSHIFT;
          destbase = (*args->procs->GetWriteScanline)(args->data, y, xl, xr);
          destunit = destbase + unitl;
          units = (xr >> SSHIFT) - unitl;
          maskl = leftBitArray[(xl & SMASK) << LOG2BPP];
          maskr = rightBitArray[(xr & SMASK) << LOG2BPP];
          if (backPatData.constant)
            switch (backPatData.value) {
	      case 0:
		if (units == 0) *destunit &= ~(maskl & maskr);
		else {
		  *destunit++ &= ~maskl;
		  maskl = 0;
		  while (--units > 0) *(destunit++) = maskl;
		  if (maskr) *destunit &= ~maskr;
		  }
		break;
	      case LASTSCANVAL:
		if (units == 0) *destunit |= maskl & maskr;
		else {
		  *destunit++ |= maskl;
		  maskl = -1;
		  while (--units > 0) *(destunit++) = maskl;
		  if (maskr) *destunit |= maskr;
		  }
		break;
	      default: {
	        register SCANTYPE value = backPatData.value;
		if (units == 0) maskl &= maskr;
	        *destunit = (*destunit & ~maskl) | (maskl & value);
		if (units != 0) {
		  destunit++;
		  while (--units > 0) *(destunit++) = value;
		  if (maskr)
		    *destunit = (*destunit & ~maskr) | (maskr & value);;
		  }
		break;
	        }
	      }
	  else {
	    register PSCANTYPE grayPtr2, eG2, sG2;
	    if (!forePatData.constant) {
	      info.color = *((DevColor *)&backColor);
	      SetupPattern(args->pattern, &info, &backPatData);
              }
            sG2 = GetPatternRow(&info, &backPatData, (integer)y);
            eG2 = sG2 + backPatData.width;
	    grayPtr2 = sG2 + (unitl % backPatData.width);
	    if (units == 0) maskl &= maskr;
	    *destunit = (*destunit & ~maskl) | (maskl & *grayPtr2);
	    if (units != 0) {
	      destunit++;
	      if (backPatData.width == 1) {
		maskl = *grayPtr2;
		while (--units > 0) *(destunit++) = maskl;
		}
	      else {
		while (--units > 0) {
		  if (++grayPtr2 >= eG2) grayPtr2 = sG2;
		  *(destunit++) = *grayPtr2;
		  }
		if (++grayPtr2 >= eG2) grayPtr2 = sG2;
		maskl = *grayPtr2;
		}
	      if (maskr) *destunit = (*destunit & ~maskr) | (maskr & maskl);
	      }
	    if (!forePatData.constant) {
	      info.color = *((DevColor *)&foreColor);
	      SetupPattern(args->pattern, &info, &forePatData);
              foreRowStart = GetPatternRow(&info, &forePatData, (integer)y);
              foreRowEnd = foreRowStart + forePatData.width;
              }
	    }
          }
        { /* paint foreground color */
	register uchar *sLoc = srcLoc;
        PSCANTYPE destunit;
	integer units;
        { register int unitl;
          unitl = xl >> SSHIFT;
          destunit = destbase + unitl;
          units = (xr >> SSHIFT) - unitl;
	  grayPtr = foreRowStart + (unitl % forePatData.width);
          }
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
        { register int sX;
          sX = IntPart(fxdSrcX);
          if (sX != srcXprv) {
            register int diff, sBit = srcBit;
            switch (diff = sX - srcXprv) {
              case -1: if (--sBit < 0) { sBit = 7; sLoc--; } break;
              case  1: if (++sBit > 7) { sBit = 0; sLoc++; } break;
              default:
                if (diff > 0) {
                  sLoc += diff >> 3; sBit += diff & 7;
                  if (sBit > 7) { sBit -= 8; sLoc++; }
                  }
                else {
                  diff = -diff; sLoc -= diff >> 3; sBit -= diff & 7;
                  if (sBit < 0) { sBit += 8; sLoc--; }
                  }
              }
            srcXprv = sX; srcBit = sBit;
            }
          }
        srcLoc = sLoc;
        { /* inner loop for painting foreground */
           /* this should be assembly coded to minimize the memory refs */
	   /* the statements of the form
                   if (sData & sBit)
                     temp |= onebit[(SUNIT - 1) - destbit];
              should be replaced by assembly code using bit test and set */
           /* ideally the inner bit loop should only make memory refs to
              read the image source data (though *sLoc), to store
	      the output (though *destunit), and, in the gray cases,
	      to read the gray pattern. */
          SCANTYPE temp;
          uchar *dx, *dy;
          uchar sData, sInvert = invert ? 0xFF : 0;
	  int sBit = 0x80 >> srcBit;
	  int destbit = (SUNIT) - (xl & SMASK);
          Fixed ex, ey;
          uchar *wb;
          uchar *needData;
	  int cnt = xr - xl - 1;
          dx = (uchar *)sv.x;
          if (sv.y > 0) {
            dy = (uchar *)sv.y;
            wb = (uchar *)wbytes;
            ey = ONE - (fxdSrcY & LowMask);
            }
          else {
            dy = (uchar *)-sv.y;
            wb = (uchar *)-wbytes;
            ey = (fxdSrcY & LowMask) + 1;
            }
          ex = ((Fixed)dx > 0) ? ONE - (fxdSrcX & LowMask) :
                                 (fxdSrcX & LowMask) + 1;
          sData = *sLoc ^ sInvert;
          needData = sLoc;
	  temp = 0;
	  switch (paintType) {
	    case blackUp: {
	      if ((sData & sBit) != 0) {
		while(true) { 
		  temp |= onebit[SUNIT - destbit];
		  if (--destbit <= 0) {
		    *destunit++ |= temp;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex -= (Fixed)dx;
		    if (ex <= 0) do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
			}
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex -= (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
		        needData = sLoc;
			}
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) == 0) goto WL1;
		  BL1: {}
		  }
		}
	      else {
		while(true) { 
		  if (--destbit <= 0) {
		    *destunit++ |= temp;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex -= (Fixed)dx;
		    if (ex <= 0) do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
			}
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex -= (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
		        needData = sLoc;
			}
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) != 0) goto BL1;
		  WL1: {}
		  }
		}
	      *destunit |= temp;
	      break;
	      }
	    case blackDown: { /* dx <= 0 */
	      if ((sData & sBit) != 0) {
		while(true) { 
		  temp |= onebit[SUNIT - destbit];
		  if (--destbit <= 0) {
		    *destunit++ |= temp;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex += (Fixed)dx;
		    if (ex <= 0) do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex += (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			needData = sLoc;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) == 0) goto WL2;
		  BL2: {}
		  }
		}
	      else {
		while(true) { 
		  if (--destbit <= 0) {
		    *destunit++ |= temp;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex += (Fixed)dx;
		    if (ex <= 0) do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex += (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			needData = sLoc;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) != 0) goto BL2;
		  WL2: {}
		  }
		}
	      *destunit |= temp;
	      break;
	      }
	    case whiteUp: {
	      if ((sData & sBit) != 0) {
		while(true) { 
		  temp |= onebit[SUNIT - destbit];
		  if (--destbit <= 0) {
		    *destunit++ &= ~temp;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex -= (Fixed)dx;
		    if (ex <= 0) do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
			}
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex -= (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
		        needData = sLoc;
			}
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) == 0) goto WL3;
		  BL3: {}
		  }
		}
	      else {
		while(true) { 
		  if (--destbit <= 0) {
		    *destunit++ &= ~temp;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex -= (Fixed)dx;
		    if (ex <= 0) do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
			}
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex -= (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
		        needData = sLoc;
			}
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) != 0) goto BL3;
		  WL3: {}
		  }
		}
	      *destunit &= ~temp;
	      break;
	      }
	    case whiteDown: { /* dx <= 0 */
	      if ((sData & sBit) != 0) {
		while(true) { 
		  temp |= onebit[SUNIT - destbit];
		  if (--destbit <= 0) {
		    *destunit++ &= ~temp;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex += (Fixed)dx;
		    if (ex <= 0) do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex += (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			needData = sLoc;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) == 0) goto WL4;
		  BL4: {}
		  }
		}
	      else {
		while(true) { 
		  if (--destbit <= 0) {
		    *destunit++ &= ~temp;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex += (Fixed)dx;
		    if (ex <= 0) do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex += (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			needData = sLoc;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) != 0) goto BL4;
		  WL4: {}
		  }
		}
	      *destunit &= ~temp;
	      break;
	      }
	    case gray1Up: {
	      SCANTYPE graypat = *grayPtr;
	      if ((sData & sBit) != 0) {
		while(true) { 
		  temp |= onebit[SUNIT - destbit];
		  if (--destbit <= 0) {
		    *destunit = (*destunit & ~temp) | (graypat & temp);
		    destunit++;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex -= (Fixed)dx;
		    if (ex <= 0) do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
			}
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex -= (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
		        needData = sLoc;
			}
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) == 0) goto WL5;
		  BL5: {}
		  }
		}
	      else {
		while(true) { 
		  if (--destbit <= 0) {
		    *destunit = (*destunit & ~temp) | (graypat & temp);
		    destunit++;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex -= (Fixed)dx;
		    if (ex <= 0) do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
			}
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex -= (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
		        needData = sLoc;
			}
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) != 0) goto BL5;
		  WL5: {}
		  }
		}
	      if (temp)
		*destunit = (*destunit & ~temp) | (graypat & temp);
	      break;
	      }
	    case gray1Down: { /* dx <= 0 */
	      SCANTYPE graypat = *grayPtr;
	      if ((sData & sBit) != 0) {
		while(true) { 
		  temp |= onebit[SUNIT - destbit];
		  if (--destbit <= 0) {
		    *destunit = (*destunit & ~temp) | (graypat & temp);
		    destunit++;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex += (Fixed)dx;
		    if (ex <= 0) do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex += (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			needData = sLoc;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) == 0) goto WL6;
		  BL6: {}
		  }
		}
	      else {
		while(true) { 
		  if (--destbit <= 0) {
		    *destunit = (*destunit & ~temp) | (graypat & temp);
		    destunit++;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex += (Fixed)dx;
		    if (ex <= 0) do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex += (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			needData = sLoc;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) != 0) goto BL6;
		  WL6: {}
		  }
		}
	      if (temp)
		*destunit = (*destunit & ~temp) | (graypat & temp);
	      break;
	      }
	    case grayNUp: {
	      if ((sData & sBit) != 0) {
		while(true) { 
		  temp |= onebit[SUNIT - destbit];
		  if (--destbit <= 0) {
		    *destunit = (*destunit & ~temp) | (*(grayPtr++) & temp);
		    destunit++;
		    if (grayPtr >= foreRowEnd) grayPtr -= forePatData.width;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex -= (Fixed)dx;
		    if (ex <= 0) do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
			}
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex -= (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
			needData = sLoc;
			}
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) == 0) goto WL7;
		  BL7: {}
		  }
		}
	      else {
		while(true) { 
		  if (--destbit <= 0) {
		    *destunit = (*destunit & ~temp) | (*(grayPtr++) & temp);
		    destunit++;
		    if (grayPtr >= foreRowEnd) grayPtr -= forePatData.width;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex -= (Fixed)dx;
		    if (ex <= 0) do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
			}
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex -= (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      sBit >>= 1;
		      if (sBit == 0) {
			sBit = 0x80; sLoc++;
			needData = sLoc;
			}
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) != 0) goto BL7;
		  WL7: {}
		  }
		}
	      if (temp)
	        *destunit = (*destunit & ~temp) | (*grayPtr & temp);
	      break;
	      }
	    case grayNDown: { /* dx <= 0 */
	      if ((sData & sBit) != 0) {
		while(true) { 
		  temp |= onebit[SUNIT - destbit];
		  if (--destbit <= 0) {
		    *destunit = (*destunit & ~temp) | (*(grayPtr++) & temp);
		    destunit++;
		    if (grayPtr >= foreRowEnd) grayPtr -= forePatData.width;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex += (Fixed)dx;
		    if (ex <= 0) do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex += (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			needData = sLoc;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) == 0) goto WL8;
		  BL8: {}
		  }
		}
	      else {
		while(true) { 
		  if (--destbit <= 0) {
		    *destunit = (*destunit & ~temp) | (*(grayPtr++) & temp);
		    destunit++;
		    if (grayPtr >= foreRowEnd) grayPtr -= forePatData.width;
		    temp = 0; destbit = SUNIT;
		    }
		  ey -= (Fixed)dy;
		  if (ey <= 0) {
		    do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		    ex += (Fixed)dx;
		    if (ex <= 0) do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    needData = sLoc;
		    }
		  else {
		    ex += (Fixed)dx;
		    if (ex > 0) {
		      if (--cnt < 0) break;
		      continue;
		      }
		    do {
		      if (sBit == 0x80) {
			sBit = 1; sLoc--;
			needData = sLoc;
			}
		      else sBit <<= 1;
		      } while ((ex += ONE) <= 0);
		    }
		  if (--cnt < 0)
		    break;
		  if (needData) {
		    sData = *sLoc ^ sInvert;  needData = NIL;
		    };
		  if ((sData & sBit) != 0) goto BL8;
		  WL8: {}
		  }
		}
	      if (temp)
	        *destunit = (*destunit & ~temp) | (*grayPtr & temp);
	      break;
	      }
	    }
          }
         }
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
      if (!forePatData.constant) {
        if ((foreRowStart = foreRowEnd) >= forePatData.end)
          foreRowStart = forePatData.start;
        foreRowEnd = foreRowStart + forePatData.width;
        }
      }
    if (t != NULL) t++;
    }
  (*args->procs->End)(args->data);
  }
