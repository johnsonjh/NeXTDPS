/*
				grypat4of4.c

   Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE:  All information contained  herein is  the  property of Adobe Systems
Incorporated.    Many of the  intellectual   and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees  for their  internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless  prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version:
Edit History:
Scott Byer: Wed May 17 17:13:46 1989
Jim Sandman: Wed Aug  2 13:50:28 1989
Paul Rovner: Fri Dec 29 11:03:42 1989
Joe Pasqua: Tue Feb 28 11:24:53 1989
Steve Schiller: Mon Dec 11 18:54:37 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include DEVICETYPES
#include DEVPATTERN
#include EXCEPT
#include PSLIB
#include FOREGROUND

#include "patternpriv.h"

private SCANTYPE pixVals[4] = {0xFFFFFFFF, 0xAAAAAAAA, 0x55555555, 0};

private procedure BuildPattern(
  grayLevel, screen, data, minGray, maxGray)
  integer grayLevel; DevScreen *screen; PatternData *data;
  integer *minGray, *maxGray;
  {
  SCANTYPE *gray, *grayEnd;
  register Card8 *scrnElt, *eScrnElt;
  register SCANTYPE *ob, *obEnd;
  register integer i;
  register SCANTYPE pattern, oldpat;
  boolean beenHere;
  register integer gLev;
  integer grayN, nUnits;
  integer j, gx, inc, cuminc, grayS, graySB;
  integer grayInc, curMinGray, curMaxGray, log2BPP;
  SCANTYPE gd;

  gLev = grayLevel *3 ;
  gx = gLev / 255;
  gLev = gLev % 255;

    /* grayS == # pixels between distinct rotations of the screen in
       the final pattern */
  grayS = GCD((integer)screen->width, (integer)(SCANUNIT/2));
  graySB = grayS*2;
    /* grayN == # distinct units == width in units of the final pattern */
  grayN = screen->width / grayS;
  
    /* compute grayInc = the delta in the gray pattern to the unit that
       begins at the pixel in the screen to the right of the rightmost
       pixel in the current unit (phew!) */
  grayInc = 1;
  if (grayN > 1)
    {
    inc = (SCANUNIT/2 % screen->width) / grayS;
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
  gd = pixVals[gx];
     /* the darkest gray value for this gx */
  /* ob should get a vector of scanunits, to be indexed by pixel index
     within a scanunit. ob[i] == a scanunit with a 1 for the pixel 
     value of the ith pixel */

  for (j = 0; j < screen->height; j++) { /* foreach row */
    ob = deepPixOneVals[1];  obEnd = ob + SCANUNIT/2;  pattern = 0;
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
      *gray = gd - pattern;
      if ((gray += grayInc) >= grayEnd)
        gray -= grayN; /* hop to the next unit to fill */
      }; /* end foreach unit */
    gray = grayEnd; /* goto start of the next row in the gray pattern */
    }; /* end foreach row */
  *minGray = (gx * 255 + curMinGray + 3 - 1) / 3;
  *maxGray = (gx * 255 + curMaxGray + 3 - 1) / 3;
  }; /* end of BuildPattern */

public procedure Gry4Of4Setup(h, markInfo, data)
  PatternHandle h; DevMarkInfo *markInfo; PatternData *data; {
  register PPatCacheInfo info;
  PCachedHalftone pcv;
  DevScreen *screen = markInfo->halftone->white;
  Card8 index;
  integer i, k, m, x, curMinGray, curMaxGray;
  PCard8 gryArray;
  DevColorVal color;
  integer gray;

  FGEnterMonitor();
  DURING
    color = *((DevColorVal *)&markInfo->color);
    gray = color.white;
  
    if (!ValidateTA(screen)) CantHappen(); /* Get thresholds in memory. */
    pcv = ((PScreenPrivate) (screen->priv))->ch;
    gryArray = pcv[dgGray];
    if (gryArray == NIL) {
      gryArray = pcv[dgGray] = AllocInfoVector();
      if (gryArray == NIL) RAISE(ecLimitCheck, (char *)NIL);
      }
    index = gryArray[gray];
    info = patterns[index];
    if ((index != 0) &&
	(info->screen == screen) &&
	(info->minGray <= gray) &&
	(info->maxGray > gray) &&
	(info->h == h)) {
      *data = info->data;
      }
    else {
      /* nothing cached for this gray value */
      if (ConstantColor((integer)4,screen,&gray,&curMinGray,&curMaxGray)) {
	integer gx = (gray * 3)/255;
	data->start = &data->value;
	data->end = data->start + 1;
	data->constant = true;
	data->width = 1;
	data->value = pixVals[gx];
	}
      else
	BuildPattern(gray, screen, data, &curMinGray, &curMaxGray);
      info = SetPatInfo(screen, data, dgGray, curMinGray, curMaxGray, h);
      }
    x = markInfo->offset.x + markInfo->screenphase.x;
    if ((k = info->lastX - x) != 0) {
      m = screen->width;
      k = k - (k/m)*m;
      if (k < 0) k += m;
      if (k) {
	RollPattern(k*2, data);
	info->lastX = x;
	info->data.id = data->id;
	}
      }
    info->lastUsed = ++patTimeStamp;
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();
  } /* end Gry4Of4Setup */



private integer PatInfo (h, red, green, blue, gray, firstColor)
  PatternHandle h; DevColorData *red, *green, *blue, *gray; 
  integer *firstColor; {
  GrayPatHandle pat = (GrayPatHandle)h;
  if (red) red->n = 0;
  if (green) green->n = 0;
  if (blue) blue->n = 0;
  if (gray) *gray = pat->data;
  if (firstColor) *firstColor = 0;
  return 2;
  }

public PatternHandle GryPat4Of4() {
  GrayPatHandle pat;
  SetupDeepOnes((integer)2);
  pat = (GrayPatHandle) os_sureMalloc((long int)sizeof(GrayPatRec));
  pat->procs.setupPattern = Gry4Of4Setup;
  pat->procs.destroyPattern = DestroyPat;
  pat->procs.patternInfo = PatInfo;
  pat->data.first = 0;
  pat->data.n = 4;
  pat->data.delta = 1;
  pat->bitsPerPixel = 2;
  pat->log2BPP = 1;
  return (&pat->procs);
  }
  
