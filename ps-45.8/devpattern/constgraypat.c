/*
  constgraypattern.c

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
Jim Sandman: Wed Aug  2 13:49:25 1989
Paul Rovner: Thu Aug 31 14:44:47 1989
Ed Taft: Wed Dec 13 18:16:36 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include DEVICETYPES
#include DEVPATTERN
#include EXCEPT
#include PSLIB

#include "patternpriv.h"

public SCANTYPE ConstSetup(cData, color, bitsPerPixel)
  DevColorData cData; integer color, bitsPerPixel; {
  integer nColorsM1 = cData.n - 1;
  integer lastPixVal = cData.first + nColorsM1 * cData.delta; 
  integer pix;
  SCANTYPE value;
  color *= nColorsM1;
  pix = lastPixVal - (color/255) * cData.delta;
  if ((color % 255) > 128) pix -= cData.delta;
  value = pix;
  for (; bitsPerPixel < SCANUNIT; bitsPerPixel *= 2) {
    value <<= bitsPerPixel;
    value |= pix;
    pix = value;
    }
  return value;
  } /* end ConstSetup */


private procedure ConstGraySetup(h, markinfo, data)
  PatternHandle h; DevMarkInfo *markinfo; PatternData *data; {
  GrayPatHandle pat = (GrayPatHandle)h;
  DevColorVal color;
  color = *((DevColorVal *)&markinfo->color);
  data->start = &data->value;
  data->end = data->start + 1;
  data->constant = true;
  data->width = 1;
  data->value =
    ConstSetup(pat->data, (integer)color.white, pat->bitsPerPixel);
  data->id = 0;
  } /* end ConstGraySetup */


public PatternHandle ConstGrayPattern(data, bitsPerPixel)
  DevColorData data; integer bitsPerPixel; {
  GrayPatHandle pat;
  integer log2BPP;
  if (data.n < 16) RAISE(ecLimitCheck, (char *)NIL);
  switch (bitsPerPixel) {
    case 4: {log2BPP = 2; break; }
    case 8: {log2BPP = 3; break; }
    case 16: {log2BPP = 4; break; }
    default: RAISE(ecLimitCheck, (char *)NIL);
    }
  SetupDeepOnes(bitsPerPixel);
  pat = (GrayPatHandle) os_sureMalloc((long int)sizeof(GrayPatRec));
  pat->procs.setupPattern = ConstGraySetup;
  pat->procs.destroyPattern = DestroyPat;
  pat->procs.patternInfo = GrayPatInfo;
  pat->data = data;
  pat->bitsPerPixel = bitsPerPixel;
  pat->log2BPP = log2BPP;
  return (&pat->procs);
  }
  
