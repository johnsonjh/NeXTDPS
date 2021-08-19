/*
				monopattern.c

   Copyright (c) 1984, '85, '86, '87, '88' 89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE:  All information contained herein  is the  property  of Adobe Systems
Incorporated.   Many  of  the intellectual  and technical  concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to  Adobe licensees for their  internal use.  Any reproduction
or dissemination of this software is strictly  forbidden unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version:
Edit History:
Scott Byer: Wed May 17 17:14:08 1989
Jim Sandman: Wed Aug  2 12:20:50 1989
Paul Rovner: Fri Dec 29 11:05:51 1989
Ivor Durham: Tue Jul 12 12:08:59 1988
Joe Pasqua: Tue Feb 28 12:59:46 1989
Steve Schiller: Fri Dec  8 14:58:56 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include DEVICETYPES
#include DEVPATTERN
#include EXCEPT
#include PSLIB
#include FOREGROUND

#include "patternpriv.h"

private procedure SetupMonoPattern(
  mono, grayLevel, screen, data, minGray, maxGray)
  MonoPatHandle mono; integer grayLevel; DevScreen *screen; PatternData *data;
  integer *minGray, *maxGray;
  {
  /* 
   Constructs a gray pattern for the given grayLevel and fills in data with
   the appropriate values. The globals curMinGray and curMaxGray are set.
   
   This pattern has one row for each row of the screen. It is an integral
   number of SCANUNITs wide and one bit deep. The pattern is wide enough
   to include all rotations of rows of the screen that could occur at unit
   boundaries in a device framebuffer that has been tiled with the screen.
   
   The range of gray values that map to the pattern thus constructed is
   specified by the values stored in curMinGray and curMaxGray.
   curMinGray has the largest gray value <= grayLevel. curMaxGray has
   the smallest gray value > grayLevel. So this gray pattern is applicable
   if (curMinGray <= grayLevel < curMaxGray).
*/
  SCANTYPE *gray, *grayEnd;
  register Card8 *scrnElt, *eScrnElt;
  register SCANTYPE *ob, *obEnd;
  register integer i;
  register SCANTYPE pattern, oldpat;
  register boolean beenHere;
  register integer gLev;
  integer grayN, nUnits;
  integer j, gx, inc, cuminc, grayS, grayInc, curMinGray, curMaxGray;
  
  gLev = grayLevel;
  gx = gLev / 255;
  gLev = gLev % 255;

  /* grayS == # pixels between distinct rotations of the screen in
     the final pattern */
  grayS = GCD((integer)screen->width, (integer)SCANUNIT);
  /* grayN == # distinct units == width in units of the final pattern */
  grayN = screen->width / grayS;
    /* compute grayInc = the delta in the gray pattern to the unit that
       begins at the pixel in the screen to the right of the rightmost
       pixel in the current unit (phew!) */
  grayInc = 1;
  if (grayN > 1)
    {
    inc = (SCANUNIT % screen->width) / grayS;
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

  /* ob should get a vector of scanunits, to be indexed by pixel index
     within a scanunit. ob[i] == a scanunit with a 1 for the pixel 
     value of the ith pixel */

  for (j = 0; j < screen->height; j++) { /* foreach row */
    ob = deepPixOneVals[0];  obEnd = ob + SCANUNIT;  pattern = 0;
    scrnElt = screen->thresholds + j * screen->width;
    eScrnElt = scrnElt + screen->width;
    grayEnd = gray + grayN;  beenHere = false;
    for (i = 0; i < grayN; i++) {  /* foreach unit on the row */
      if (beenHere) {pattern = oldpat LSHIFT grayS; ob = obEnd - grayS;}
      beenHere = true;
      do /* foreach pixel, not including ones shifted from the previous unit */
        {
	if (gLev < *scrnElt) {
	  if (*scrnElt < curMaxGray) curMaxGray = *scrnElt;
	  }
	else {
	  pattern |= *ob;
	  if (*scrnElt > curMinGray) curMinGray = *scrnElt;
	  }
	if (++scrnElt >= eScrnElt)
	  scrnElt -= screen->width;
        } while (++ob < obEnd);
      oldpat = pattern;
      *gray = mono->oneMeansWhite ? pattern : LASTSCANVAL - pattern;
      if ((gray += grayInc) >= grayEnd)
        gray -= grayN; /* hop to the next unit to fill */
      }; /* end foreach unit */
    gray = grayEnd; /* goto start of the next row in the gray pattern */
    }; /* end foreach row */
  *minGray = curMinGray;
  *maxGray = curMaxGray;
  } /* end of SetupMonoPattern */


public procedure MonoSetup(h, markInfo, data)
  PatternHandle h; DevMarkInfo *markInfo; PatternData *data; {
  MonoPatHandle mono = (MonoPatHandle)h;
  integer gray;
  register DevScreen *screen;
  register PPatCacheInfo info;
  PCachedHalftone pcv;
  Card8 index;
  integer i, k, m, x, curMinGray, curMaxGray;
  PCard8 gryArray;
  DevColorVal color;
  color = *((DevColorVal *)&markInfo->color);

  gray = color.white;
  screen = markInfo->halftone->white;

  if (gray == 0 || gray == MAXCOLOR) {
    data->start = &data->value;
    data->end = data->start + 1;
    data->constant = true;
    data->width = 1;
    data->value = (gray == 0 ? LASTSCANVAL : 0);
    if (mono->oneMeansWhite) data->value = ~data->value;
    return;
    }

  FGEnterMonitor();
  DURING
    if (!ValidateTA(screen)) CantHappen();
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
      register integer n;
      if (ConstantColor(
	(integer)2, screen, &gray, &curMinGray, &curMaxGray)) {
	data->start = &data->value;
	data->end = data->start + 1;
	data->width = 1;
	data->constant = true;
	data->value = (gray == 0 ? LASTSCANVAL : 0);
	if (mono->oneMeansWhite) data->value = ~data->value;
	}
      else
	SetupMonoPattern(
	  mono, gray, screen, data, &curMinGray, &curMaxGray);
      info = SetPatInfo(screen, data, dgGray, curMinGray, curMaxGray, h);
      }
    x = markInfo->offset.x + markInfo->screenphase.x;
    if ((k = info->lastX - x) != 0) {
      m = screen->width;
      k = k - (k/m)*m; /* Broken-out implementation of % */
      if (k < 0) k += m;
      if (k) {
	RollPattern(k, data);
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
  } /* end MonoSetup */


public integer MonoPatInfo (h, red, green, blue, gray, firstColor)
  PatternHandle h; DevColorData *red, *green, *blue, *gray; 
  integer *firstColor; {
  MonoPatHandle pat = (MonoPatHandle)h;
  if (red) red->n = 0;
  if (green) green->n = 0;
  if (blue) blue->n = 0;
  if (firstColor) *firstColor = 0;
  if (gray) {
    gray->n = 2;
    if (pat->oneMeansWhite) {
      gray->delta = -1;
      gray->first = 1;
      }
    else {
      gray->delta = 1;
      gray->first = 0;
      }
    }
  return 1;
  }

public PatternHandle MonochromePattern(oneMeansWhite) boolean oneMeansWhite; {
  MonoPatHandle pat = (MonoPatHandle) os_sureMalloc((long int)sizeof(MonoPatRec));
  pat->procs.setupPattern = MonoSetup;
  pat->procs.destroyPattern = DestroyPat;
  pat->procs.patternInfo = MonoPatInfo;
  pat->oneMeansWhite = oneMeansWhite;
  return (&pat->procs);
  }
  
