/*
			       colorpattern.c

Copyright (c) 1983, '84, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE: All information contained  herein is  the property of  Adobe  Systems
Incorporated.   Many  of the  intellectual and  technical  concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to  Adobe licensees for their  internal use.  Any reproduction
or dissemination of this software  is strictly forbidden unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version:
Edit History:
Scott Byer: Wed May 17 17:12:50 1989
Jim Sandman: Wed Aug  2 12:12:42 1989
Paul Rovner: Tue Nov 28 10:07:37 1989
Ed Taft: Wed Dec 13 18:15:49 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include DEVICETYPES
#include DEVPATTERN
#include EXCEPT
#include PSLIB
#include FOREGROUND

#include "patternpriv.h"

typedef struct _t_CachedColor {
  struct _t_CachedColor *flink, *blink;
  integer redID, greenID, blueID, offset;
  PatternData data;
  } CachedColor, *PCachedColor; 

PCachedColor ccHead;

PCachedColor ccLast;
PCard8 ccEnd;

private procedure InitCCCache() {
  ccHead = (PCachedColor)grayPatternBase;
  ccEnd = (PCard8)grayPatternBase + maxPatternSize;
  ccHead->flink = ccHead;
  ccHead->blink = ccHead;
  ccLast = ccHead +1;
  }

private PCachedColor GetCachePlace(n) integer n; {
  PCachedColor cc;
  while (((PCard8)ccLast + n*sizeof(SCANTYPE) + sizeof(CachedColor)) > ccEnd) {
    PCachedColor old = ccHead->blink;
    PCachedColor next = (PCachedColor)(old->data.end);
    DebugAssert(old != ccHead);
    old->flink->blink = old->blink;
    old->blink->flink = old->flink;
    if ((old->data.end - old->data.start) == n) {
      cc = old;
      goto gotit;
      }
    while (true) {
      integer nUnits = next->data.end - next->data.start;
      integer ct = 0;
      char *svN, *svO;
      PCachedColor t = (PCachedColor)(next->data.end);
      DebugAssert(next > ccHead && next <= ccLast);
      DebugAssert(t > ccHead && t <= ccLast);
      if (next == ccLast) {
        ccLast = old;
        break;
        }
      os_bcopy(
	(char *)next,
	(char *)old,
	(long int)(nUnits*sizeof(SCANTYPE)+sizeof(CachedColor)));
      old->flink->blink = old;
      old->blink->flink = old;
      old->data.start = (PSCANTYPE)((PCard8)old + sizeof(CachedColor));
      old->data.end = old->data.start + nUnits;
      old = (PCachedColor)old->data.end;
      next = t;
      }
    }
  cc = ccLast;
  ccLast++;
  ccLast = (PCachedColor)((PSCANTYPE)ccLast + n);
gotit:
  Assert(cc != NIL);
  cc->flink = ccHead->flink;
  cc->flink->blink = cc;
  cc->blink = ccHead;
  ccHead->flink = cc;
  return cc;
  }


private integer LCM(u, v) integer u, v; {return (u * v) / GCD(u, v);}
private integer LCM3(u, v, w) integer u, v, w; {return LCM(LCM(u, v), w); }

private procedure ComposeColors(
  data, redData, greenData, blueData, firstColor, offset)
  PatternData *data, *redData, *greenData, *blueData; integer firstColor, offset; {
  register PSCANTYPE rp, erp, gp, egp, bp, ebp;
  integer width, height;
  PSCANTYPE start, end, wall, srp, sgp, sbp;
  PCachedColor cc;
  if (ccHead == NULL)
    InitCCCache();
  for (cc = ccHead->flink; cc != ccHead; cc = cc->flink) {
    if (cc->redID == redData->id && cc->greenID == greenData->id &&
      cc->blueID == blueData->id && cc->offset == offset) {
      if (ccHead->flink != cc) {
        cc->flink->blink = cc->blink;
        cc->blink->flink = cc->flink;
	cc->flink = ccHead->flink;
	cc->flink->blink = cc;
	cc->blink = ccHead;
	ccHead->flink = cc;
        }
      *data = cc->data;
      return;
      }
    }
  rp = redData->start;
  gp = greenData->start;
  bp = blueData->start;
  if (greenData->width == redData->width &&
      blueData->width == redData->width &&
      (greenData->end - gp) == (redData->end - rp) &&
      (blueData->end - bp) == (redData->end - rp)
      ) {
    cc = GetCachePlace(redData->end - rp);
    start = (PSCANTYPE)((PCard8)cc + sizeof(CachedColor));
    width = redData->width;
    end = start;
    do {
      *end++ = *rp++ + *gp++ + *bp++ + firstColor;
      } while (rp < redData->end);
    
    }
  else {
    width = LCM3(redData->width, greenData->width, blueData->width);
    height = LCM3(
      (redData->end - rp)/redData->width,
      (greenData->end - gp)/greenData->width,
      (blueData->end - bp)/blueData->width);
    cc = GetCachePlace(height * width);
    start = (PSCANTYPE)((PCard8)cc + sizeof(CachedColor));
    end = start;
    wall = start + (height * width);
    srp = rp; erp = rp + redData->width;
    sgp = gp; egp = gp + greenData->width;
    sbp = bp; ebp = bp + blueData->width;
    do {
      *end++ = *rp++ + *gp++ + *bp++ + firstColor;
      if (rp == erp) {
        rp = srp;
        if (gp == egp) {
          gp = sgp;
	  if (bp == ebp) { /* move together to next row */
	    rp = srp += redData->width; erp = srp + redData->width;
	    gp = sgp += greenData->width; egp = sgp + greenData->width;
	    bp = sbp += blueData->width; ebp = sbp + blueData->width;
	    if (srp == redData->end) {
	      rp = srp = redData->start; erp = srp + redData->width;
	      if (sgp == greenData->end) {
	        gp = sgp = greenData->start; egp = sgp + greenData->width;
	        if (sbp == blueData->end) break;
	        }
	      else if (sbp == blueData->end) {
	        bp = sbp = blueData->start; ebp = sbp + blueData->width;
		}
	      }
	    else if (sgp == greenData->end) {
	      gp = sgp = greenData->start; egp = sgp + greenData->width;
	      if (sbp == blueData->end) {
	        bp = sbp = blueData->start; ebp = sbp + blueData->width;
		}
	      }
	    else if (sbp == blueData->end) {
	      bp = sbp = blueData->start; ebp = sbp + blueData->width;
	      }
	    }
          }
        else if (bp == ebp) bp = sbp;
        }
      else if (gp == egp) {
        gp = sgp;
        if (bp == ebp) bp = sbp;
        }
      else if (bp == ebp) bp = sbp;
      }
      while (end < wall);
    } /* end else arm */
  cc->data.start = start;
  cc->data.end = end;
  cc->data.width = width;
  cc->data.constant = false;
  cc->data.id = ++patID;
  cc->redID = redData->id;
  cc->greenID = greenData->id;
  cc->blueID = blueData->id;
  cc->offset = offset;
  *data = cc->data;
  }


public procedure ColorSetup(h, markInfo, data)
  PatternHandle h; DevMarkInfo *markInfo; PatternData *data; {
  ColorPatHandle pat = (ColorPatHandle)h;
  register DevHalftone *halftone = markInfo->halftone;
  PatternData redData, greenData, blueData;
  DevColorVal *color = (DevColorVal *)&markInfo->color;
  
  FGEnterMonitor();
  DURING
    if (color->red == color->green &&
	color->red == color->blue && pat->gray.n != 0) {
      SetupGrayPattern(
	h, markInfo, data, (integer)color->red, pat->gray, 
	pat->log2BPP, markInfo->halftone->white, dgGray);
      }
    else {
      SetupGrayPattern(
	h, markInfo, &redData, (integer)color->red, pat->red,
	pat->log2BPP, markInfo->halftone->red, dgRed);
      SetupGrayPattern(
	h, markInfo, &greenData, (integer)color->green, pat->green,
	pat->log2BPP, markInfo->halftone->green, dgGreen);
      SetupGrayPattern(
	h, markInfo, &blueData, (integer)color->blue, pat->blue,
	pat->log2BPP, markInfo->halftone->blue, dgBlue);
      if (redData.constant && greenData.constant && blueData.constant) {
	data->start = &data->value;
	data->end = data->start + 1;
	data->width = 1;
	data->constant = true;
	data->id = 0;
	data->value =
	  redData.value + greenData.value + blueData.value + pat->firstColor;
	}
      else
	ComposeColors(
	  data, &redData, &greenData, &blueData, pat->firstColor,
	  markInfo->offset.x + markInfo->screenphase.x);
      }
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();
  } /* end ColorSetup */


private integer max(a, b) integer a, b; { return (a > b) ? a : b;}

public integer ColorPatInfo (h, red, green, blue, gray, firstColor)
  PatternHandle h; DevColorData *red, *green, *blue, *gray; 
  integer *firstColor; {
  ColorPatHandle pat = (ColorPatHandle)h;
  if (red) *red = pat->red;
  if (green) *green = pat->green;
  if (blue) *blue = pat->blue;
  if (gray) *gray = pat->gray;
  if (firstColor)
    *firstColor = (pat->firstColor & ((1 << (1 << pat->log2BPP))-1));
  return pat->bitsPerPixel;
  }

public PatternHandle RGBPattern(
  red, green, blue, gray, firstColor, bitsPerPixel)
  DevColorData red, green, blue, gray; integer firstColor, bitsPerPixel; {
  ColorPatHandle pat;
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
  pat = (ColorPatHandle) os_sureMalloc((long int)sizeof(ColorPatRec));
  pat->procs.setupPattern = ColorSetup;
  pat->procs.destroyPattern = DestroyPat;
  pat->procs.patternInfo = ColorPatInfo;
  pat->red = red;
  pat->green = green;
  pat->blue = blue;
  pat->gray = gray;
  pat->bitsPerPixel = bitsPerPixel;
  pat->firstColor = firstColor;
  pat->log2BPP = log2BPP;
  for (;bitsPerPixel < SCANUNIT; bitsPerPixel *=2) {
    pat->firstColor <<= bitsPerPixel;
    pat->firstColor |= firstColor;
    firstColor = pat->firstColor;
    }
  return (&pat->procs);
  }
  
private procedure CMYKConstColorSetup(h, markInfo, data)
  PatternHandle h; DevMarkInfo *markInfo; PatternData *data; {
  ColorPatHandle pat = (ColorPatHandle)h;
  PatternData redData, greenData, blueData;
  SCANTYPE value;
  DevColorVal color;
  color = *((DevColorVal *)&markInfo->color);
  value = ((integer)color.red << 24) +
          ((integer)color.green << 16) +
		  ((integer)color.blue << 8) +
		  ((integer)color.white);
  data->start = &data->value;
  data->end = data->start + 1;
  data->width = 1;
  data->constant = true;
  data->value = value + pat->firstColor;
  data->id = 0;
  return;
  } /* end CMYKConstColorSetup */


public PatternHandle ConstCMYKPattern( /* XXX incomplete */
  cyan, magenta, yellow, black, firstColor, bitsPerPixel)
  DevColorData cyan, magenta, yellow, black;
  integer firstColor, bitsPerPixel;
  {
  DevColorData red, green, blue, gray; 
  ColorPatHandle pat;
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

  Assert(bitsPerPixel == 32); /* XXX for now */
  SetupDeepOnes(bitsPerPixel);

  pat = (ColorPatHandle) os_sureMalloc((long int)sizeof(ColorPatRec));
  pat->procs.setupPattern = CMYKConstColorSetup;
  pat->procs.destroyPattern = DestroyPat;
  pat->procs.patternInfo = ColorPatInfo;
  
  red.n = green.n = blue.n = gray.n = 256; 
  red.delta = 1 << 24;
  green.delta = 1 << 16;
  blue.delta = 1 << 8;
  gray.delta = 1;
  red.first = green.first = blue.first = gray.first = 0;

  pat->red = red;
  pat->green = green;
  pat->blue = blue;
  pat->gray = gray;
  pat->bitsPerPixel = bitsPerPixel;
  pat->firstColor = 0;
  pat->log2BPP = log2BPP;
  return (&pat->procs);
  }
  
