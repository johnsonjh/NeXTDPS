/*
  constcolorpattern.c

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
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
Jim Sandman: Wed Aug  2 13:50:02 1989
Paul Rovner: Thu Aug 31 14:45:06 1989
Ed Taft: Wed Dec 13 18:16:24 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include DEVICETYPES
#include DEVPATTERN
#include EXCEPT
#include PSLIB

#include "patternpriv.h"

public procedure ConstColorSetup(h, markInfo, data)
  PatternHandle h; DevMarkInfo *markInfo; PatternData *data; {
  ColorPatHandle pat = (ColorPatHandle)h;
  PatternData redData, greenData, blueData;
  SCANTYPE value;
  DevColorVal color;
  color = *((DevColorVal *)&markInfo->color);
  if (color.red == color.green &&
      color.red == color.blue && pat->gray.n != 0) 
    value = ConstSetup(
      pat->gray, (integer)color.red, pat->bitsPerPixel);
  else
    value = 
      ConstSetup(
        pat->red, (integer)color.red, pat->bitsPerPixel) +
      ConstSetup(
        pat->green, (integer)color.green, pat->bitsPerPixel) +
      ConstSetup(
        pat->blue, (integer)color.blue, pat->bitsPerPixel);
  data->start = &data->value;
  data->end = data->start + 1;
  data->width = 1;
  data->constant = true;
  data->value = value + pat->firstColor;
  data->id = 0;
  return;
  } /* end ConstColorSetup */


public PatternHandle ConstRGBPattern(
  red, green, blue, gray, firstColor, bitsPerPixel)
  DevColorData red, green, blue, gray; integer firstColor, bitsPerPixel; {
  ColorPatHandle pat;
  integer log2BPP;
  if (red.n < 16 || green.n < 16 || blue.n < 16)
    RAISE(ecLimitCheck, (char *)NIL);
  if (gray.n != 0 && gray.n < 16)
    RAISE(ecLimitCheck, (char *)NIL);
  switch (bitsPerPixel) {
    case 4: {log2BPP = 2; break; }
    case 8: {log2BPP = 3; break; }
    case 16: {log2BPP = 4; break; }
    case 32: {log2BPP = 5; break; }
    default: RAISE(ecLimitCheck, (char *)NIL);
    }
  SetupDeepOnes(bitsPerPixel);
  pat = (ColorPatHandle) os_sureMalloc((long int)sizeof(ColorPatRec));
  pat->procs.setupPattern = ConstColorSetup;
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
  
