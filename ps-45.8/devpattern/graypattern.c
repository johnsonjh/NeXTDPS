/*
				graypattern.c

   Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE:   All information contained herein is   the property of Adobe Systems
Incorporated.    Many of  the  intellectual and  technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees  for their internal use.   Any reproduction
or dissemination of this software is  strictly forbidden unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version:
Edit History:
Scott Byer: Wed May 17 17:13:10 1989
Jim Sandman: Wed Aug  2 12:13:20 1989
Paul Rovner: Fri Dec 29 11:04:53 1989
Joe Pasqua: Tue Feb 28 11:23:13 1989
Steve Schiller: Mon Dec 11 18:54:14 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include DEVICETYPES
#include DEVPATTERN
#include EXCEPT
#include PSLIB
#include FOREGROUND

#include "patternpriv.h"

private SCANTYPE MakeConstValue(pixelValue, bitsPerPixel)
  SCANTYPE pixelValue, bitsPerPixel; {
  SCANTYPE value;
  value = pixelValue;
  for (; bitsPerPixel < SCANUNIT; bitsPerPixel *= 2) {
    value <<= bitsPerPixel;
    value |= pixelValue;
    pixelValue = value;
    }
  return value;
  };
  

private procedure BuildGrayPattern(
  grayLevel, screen, data, cData, log2BPP, minGray, maxGray)
  integer grayLevel; DevScreen *screen; PatternData *data; DevColorData cData;
  integer log2BPP, *minGray, *maxGray;
  {
  SCANTYPE *gray, *grayEnd;
  register Card8 *scrnElt, *eScrnElt;
  register SCANTYPE *ob, *obEnd;
  register integer i;
  register SCANTYPE pattern, oldpat;
  boolean beenHere;
  register integer gLev;
  integer grayN, nUnits;
  integer j, gx, inc, cuminc, grayS, graySB, nUnitPixels;
  integer grayInc, curMinGray, curMaxGray;
  integer nGraysM1 = cData.n - 1;
  SCANTYPE gd;

  gLev = grayLevel;
  gx = (gLev * nGraysM1) / 255;
  gLev = (gLev * nGraysM1) % 255;
  nUnitPixels = SCANUNIT >> log2BPP;

    /* grayS == # pixels between distinct rotations of the screen in
       the final pattern */
  grayS = GCD((integer)screen->width, nUnitPixels);
  graySB = grayS << log2BPP;
    /* grayN == # distinct units == width in units of the final pattern */
  grayN = screen->width / grayS;
  
    /* compute grayInc = the delta in the gray pattern to the unit that
       begins at the pixel in the screen to the right of the rightmost
       pixel in the current unit (phew!) */
  grayInc = 1;
  if (grayN > 1)
    {
    inc = (nUnitPixels % screen->width) / grayS;
    cuminc = inc;
    while (cuminc != 1)
      {grayInc++; if ((cuminc += inc) >= grayN) cuminc -= grayN;}
    }

  if (screen->height * grayN * sizeof(SCANTYPE) > maxPatternSize) CantHappen();
  curMinGray = 0;  curMaxGray = MAXCOLOR+1;
  nUnits = screen->height * grayN;
  if (nUnits == 1) {
    data->constant = true;
    data->start = &data->value;
    }
  else {
    data->constant = false;
    data->start = AllocPatternStorage(nUnits);
    }
  data->end = data->start + nUnits;
  data->width = grayN;
  gray = data->start;
  gd =
    MakeConstValue(
      (SCANTYPE)cData.first + (nGraysM1-gx) * cData.delta, 
      (SCANTYPE) (1L << log2BPP));
     /* the darkest gray value for this gx */

  /* ob should get a vector of scanunits, to be indexed by pixel index
     within a scanunit. ob[i] == a scanunit with a 1 for the pixel 
     value of the ith pixel */

  for (j = 0; j < screen->height; j++) { /* foreach row */
    ob = deepPixOneVals[log2BPP];  obEnd = ob + nUnitPixels;  pattern = 0;
    scrnElt = screen->thresholds + j * screen->width;
    eScrnElt = scrnElt + screen->width;
    grayEnd = gray + grayN;  beenHere = false;
    for (i = 0; i < grayN; i++) {  /* foreach unit on the row */
      if (beenHere) {pattern = oldpat LSHIFT graySB; ob = obEnd - grayS;}
      beenHere = true;
      do /* foreach pixel, not including ones shifted from the previous unit */
        {
	if (gLev < *scrnElt) {
	  if (*scrnElt < curMaxGray) curMaxGray = *scrnElt;
	  }
	else {
	  pattern |= *ob;
	  if (*scrnElt > curMinGray) curMinGray = *scrnElt;;
	  }
	if (++scrnElt >= eScrnElt)
	  scrnElt -= screen->width;
        } while (++ob < obEnd);
      oldpat = pattern;
      *gray = gd - (pattern * cData.delta);
      if ((gray += grayInc) >= grayEnd)
        gray -= grayN; /* hop to the next unit to fill */
      }; /* end foreach unit */
    gray = grayEnd; /* goto start of the next row in the gray pattern */
    }; /* end foreach row */
  *minGray = (gx * 255 + curMinGray + nGraysM1 - 1) / nGraysM1;
  *maxGray = (gx * 255 + curMaxGray + nGraysM1 - 1) / nGraysM1;
  }; /* end of BuildGrayPattern */

public PPatCacheInfo SetupGrayPattern(
  h, markInfo, data, gray, cData, log2BPP, screen, color)
  PatternHandle h; DevMarkInfo *markInfo; PatternData *data;
  DevColorData cData; integer gray, log2BPP; DevScreen *screen;
  DGColor color; {
  register PPatCacheInfo info;
  PCachedHalftone pcv;
  Card8 index;
  integer i, k, m, x, curMinGray, curMaxGray;
  PCard8 gryArray;

  if (!ValidateTA(screen)) CantHappen();	/* Make sure thresholds in mem */
  pcv = ((PScreenPrivate) (screen->priv))->ch;
  gryArray = pcv[color];
  if (gryArray == NIL) {
    gryArray = pcv[color] = AllocInfoVector();
    if (gryArray == NIL) RAISE(ecLimitCheck, (char *)NIL);
    }
  index = gryArray[gray];
  info = patterns[index];
  if ((index != 0) &&
      (info->screen == screen) &&
      (info->dgColor == color) &&
      (info->minGray <= gray) &&
      (info->maxGray > gray) &&
      (info->h == h)) {
    *data = info->data;
    }
  else {
    /* nothing cached for this gray value */
    if (ConstantColor(cData.n, screen, &gray, &curMinGray, &curMaxGray)) {
      integer gx = (gray * (cData.n-1))/255;
      data->start = &data->value;
      data->end = data->start + 1;
      data->constant = true;
      data->width = 1;
      data->value =
	MakeConstValue(
	  (SCANTYPE)cData.first + (cData.n-gx-1) * cData.delta,
	  (SCANTYPE)(1L << log2BPP));
      }
    else
      BuildGrayPattern(
        gray, screen, data, cData, log2BPP, &curMinGray, &curMaxGray);
    info = SetPatInfo(screen, data, color, curMinGray, curMaxGray, h);
    }
  x = markInfo->offset.x + markInfo->screenphase.x;
  if ((k = info->lastX - x) != 0) {
    m = screen->width;
    k = k - (k/m)*m;
    if (k < 0) k += m;
    if (k) {
      RollPattern(k << log2BPP, data);
      info->lastX = x;
      info->data.id = data->id;
      }
    }
  info->lastUsed = ++patTimeStamp;
  return info;
  } /* end SetupGrayPattern */



public procedure GraySetup(h, markinfo, data)
  PatternHandle h; DevMarkInfo *markinfo; PatternData *data; {
  GrayPatHandle pat = (GrayPatHandle)h;
  DevColorVal *color = (DevColorVal *)&markinfo->color;
  FGEnterMonitor();
  DURING
    (void)SetupGrayPattern(
      h, markinfo, data, (integer)color->white, pat->data,
      pat->log2BPP, markinfo->halftone->white, dgGray);
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();
  } /* end GraySetup */


public integer GrayPatInfo (h, red, green, blue, gray, firstColor)
  PatternHandle h; DevColorData *red, *green, *blue, *gray; 
  integer *firstColor; {
  GrayPatHandle pat = (GrayPatHandle)h;
  if (red) red->n = 0;
  if (green) green->n = 0;
  if (blue) blue->n = 0;
  if (gray) *gray = pat->data;
  if (firstColor) *firstColor = 0;
  return pat->bitsPerPixel;
  }

public PatternHandle GrayPattern(data, bitsPerPixel)
  DevColorData data; integer bitsPerPixel; {
  GrayPatHandle pat;
  integer log2BPP;
  switch (bitsPerPixel) {
    case 1: {log2BPP = 0; break; }
    case 2: {log2BPP = 1; break; }
    case 4: {log2BPP = 2; break; }
    case 8: {log2BPP = 3; break; }
    case 16: {log2BPP = 4; break; }
    case 32: {log2BPP = 5; break; }
    default: RAISE(ecLimitCheck, (char *)NIL);
    }
  SetupDeepOnes(bitsPerPixel);
  pat = (GrayPatHandle) os_sureMalloc((long int)sizeof(GrayPatRec));
  pat->procs.setupPattern = GraySetup;
  pat->procs.destroyPattern = DestroyPat;
  pat->procs.patternInfo = GrayPatInfo;
  pat->data = data;
  pat->bitsPerPixel = bitsPerPixel;
  pat->log2BPP = log2BPP;
  return (&pat->procs);
  }
  
