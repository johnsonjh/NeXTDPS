/*
  maskpattern.c

Copyright (c) 1988 Adobe Systems Incorporated.
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
Jim Sandman: Thu Apr 13 09:11:07 1989
Paul Rovner: Fri Aug 18 14:23:44 1989
Ed Taft: Wed Dec 13 18:19:03 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include DEVICETYPES
#include DEVPATTERN
#include PSLIB

#include "patternpriv.h"

public procedure MaskSetup(h, markinfo, data)
  PatternHandle h; DevMarkInfo *markinfo; PatternData *data; {
  MaskPatHandle mask = (MaskPatHandle)h;
  data->start = &data->value;
  data->end = data->start + 1;
  data->width = 1;
  data->constant = true;
  data->value = (mask->positive ? LASTSCANVAL : 0);
  data->id = 0;
  return;
  }

public integer MaskPatInfo (h, red, green, blue, gray, firstColor)
  PatternHandle h; DevColorData *red, *green, *blue, *gray; 
  integer *firstColor; {
  MaskPatHandle pat = (MaskPatHandle)h;
  if (red) red->n = 0;
  if (green) green->n = 0;
  if (blue) blue->n = 0;
  if (gray) {
    gray->n = 1;
    gray->delta = 0;
    gray->first = pat->positive ? 1 : 0;
    }
  if (firstColor) *firstColor = 0;
  return 0;
  }

public PatternHandle MaskPattern(positive) boolean positive; {
  MaskPatHandle pat = (MaskPatHandle) os_sureMalloc((long int)sizeof(MaskPatRec));
  pat->procs.setupPattern = MaskSetup;
  pat->procs.destroyPattern = DestroyPat;
  pat->procs.patternInfo = MaskPatInfo;
  pat->positive = positive;
  return (&pat->procs);
  }
  
