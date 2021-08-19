/* imsupport.c

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
Ed Taft: Wed Dec 13 18:18:30 1989
Ivor Durham: Wed Mar 23 13:11:32 1988
Bill Paxton: Tue Mar 29 09:25:41 1988
Paul Rovner: Wed Aug 23 13:43:58 1989
Jim Sandman: Tue Oct 17 14:42:38 1989
Joe Pasqua: Tue Feb 28 11:39:29 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include DEVICETYPES
#include DEVIMAGE
#include DEVPATTERN
#include EXCEPT
#include PSLIB

#include "imagepriv.h"

public PSCANTYPE pixelVals, pixelThresholds;

private GetBuffers(v, t) PSCANTYPE *v, *t; {
  if (*v == NULL) {
    *v = (PSCANTYPE) os_calloc((long int)256, (long int)sizeof(SCANTYPE));
    *t = (PSCANTYPE) os_calloc((long int)256, (long int)sizeof(SCANTYPE));
    if (*v == NULL || *t == NULL)
      RAISE(ecLimitCheck, (char *)NIL);
    }
  }


public PSCANTYPE GetPixBuffers() {
  GetBuffers(&pixelVals, &pixelThresholds);
  return pixelVals;
  }

public PSCANTYPE redValues, redThresholds, greenValues, greenThresholds,
  whiteValues, whiteThresholds;

public PSCANTYPE GetRedPixBuffers() {
  GetBuffers(&redValues, &redThresholds);
  return redValues;
  }

public PSCANTYPE GetGreenPixBuffers() {
  GetBuffers(&greenValues, &greenThresholds);
  return greenValues;
  }

public PSCANTYPE GetWhitePixBuffers() {
  GetBuffers(&whiteValues, &whiteThresholds);
  return whiteValues;
  }

public procedure Set1BitValues(cData, tfrFcn, vals, thresholds, decode)
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals, thresholds;
  DevImSampleDecode *decode; {
  register integer i, g, val; 
  integer nColorsM1 = cData.n - 1;
  integer lastPixVal = cData.first + nColorsM1 * cData.delta;
  real range, min; 
  if (decode != NULL) {
    min = decode->min;
    range = decode->max - decode->min;
    }
  else {
    min = 0.0; range = 1.0;
    }
  for (i = 0; i < 2; i++) { 
    val = (integer)(255.0 * (min + i * range));
    if (val > 255) val = 255;
    else if (val < 0) val = 0;
    if (tfrFcn != NULL) val = tfrFcn[val];
    val *= nColorsM1;
    g = lastPixVal - (val/255) * cData.delta;
    vals[i] = g;
    thresholds[i] = val % 255;
    }
  }

public procedure Set2BitValues(cData, tfrFcn, vals, thresholds, decode)
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals, thresholds;
  DevImSampleDecode *decode; {
  register integer i, g, j, m, val;  
  integer nColorsM1 = cData.n - 1;
  integer lastPixVal = cData.first + nColorsM1 * cData.delta;
  real range, min;
  if (decode != NULL) {
    min = 3 * decode->min;
    range = decode->max - decode->min;
    }
  else {
    min = 0.0; range = 1.0;
    }
  for (i = 0; i < 4; i++) { 
    val = (integer)(85.0 * (min + i * range));
    if (val > 255) val = 255;
    else if (val < 0) val = 0;
    if (tfrFcn != NULL) val = tfrFcn[val];
    val *= nColorsM1;
    g = lastPixVal - (val/255) * cData.delta;
    for (j = 0; j <= 6; j += 2) { /* j is the left shift amount */
      m = i << j; /* byte value after mask */
      vals[m] = g;
      thresholds[m] = val % 255;
      }
    }
  }

public procedure Set4BitValues(cData, tfrFcn, vals, thresholds, decode)
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals, thresholds;
  DevImSampleDecode *decode; {
  register integer i, g, j, m, val;  
  integer nColorsM1 = cData.n - 1;
  integer lastPixVal = cData.first + nColorsM1 * cData.delta;
  real range, min;
  if (decode != NULL) {
    min = 15 * decode->min;
    range = decode->max - decode->min;
    }
  else {
    min = 0.0; range = 1.0;
    }
  for (i = 0; i < 16; i++) { 
    val = (integer)(17.0 * (min + i * range));
    if (val > 255) val = 255;
    else if (val < 0) val = 0;
    if (tfrFcn != NULL) val = tfrFcn[val];
    val *= nColorsM1;
    g = lastPixVal - (val/255) * cData.delta;
    for (j = 0; j <= 4; j += 4) { /* j is the left shift amount */
      m = i << j; /* byte value after mask */
      vals[m] = g;
      thresholds[m] = val % 255;
      }
    }
  }

public procedure Set8BitValues(cData, tfrFcn, vals, thresholds, decode)
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals, thresholds;
  DevImSampleDecode *decode; {
  register integer i, g, val;  
  integer nColorsM1 = cData.n - 1;
  integer lastPixVal = cData.first + nColorsM1 * cData.delta;
  real range, min;
  if (decode != NULL) {
    min = 255 * decode->min;
    range = decode->max - decode->min;
    }
  else {
    min = 0.0; range = 1.0;
    }
  for (i = 0; i < 256; i++) { 
    val = (integer)(min + i * range);
    if (val > 255) val = 255;
    else if (val < 0) val = 0;
    if (tfrFcn != NULL) val = tfrFcn[val];
    val *= nColorsM1;
    g = lastPixVal - (val/255) * cData.delta;
    vals[i] = g;
    thresholds[i] = val % 255;
    }
  }

public procedure Set1BitCMYKValues(cData, tfrFcn, vals, decode)
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals;
  DevImSampleDecode *decode; {
  register integer i, g, val;  
  integer nColorsM1 = cData.n - 1;
  integer lastPixVal = cData.first + nColorsM1 * cData.delta;
  real range, min;
  if (decode != NULL) {
    min = decode->min;
    range = decode->max - decode->min;
    }
  else {
    min = 0.0; range = 1.0;
    }
  for (i = 0; i < 2; i++) { 
    val = (integer)(255.0 * (min + i * range));
    if (val > 255) val = 255;
    else if (val < 0) val = 0;
    if (tfrFcn != NULL) val = 255 - tfrFcn[255 - val];
    val *= nColorsM1;
    g = (val/255) * cData.delta;
    vals[i] = g;
    }
  }

public procedure Set2BitCMYKValues(cData, tfrFcn, vals, decode)
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals;
  DevImSampleDecode *decode; {
  register integer i, g, j, m, val;  
  integer nColorsM1 = cData.n - 1;
  integer lastPixVal = cData.first + nColorsM1 * cData.delta;
  real range, min;
  if (decode != NULL) {
    min = 3 * decode->min;
    range = decode->max - decode->min;
    }
  else {
    min = 0.0; range = 1.0;
    }
  for (i = 0; i < 4; i++) { 
    val = (integer)(85.0 * (min + i * range));
    if (val > 255) val = 255;
    else if (val < 0) val = 0;
    if (tfrFcn != NULL) val = 255 - tfrFcn[255 - val];
    val *= nColorsM1;
    g = (val/255) * cData.delta;
    for (j = 0; j <= 6; j += 2) { /* j is the left shift amount */
      m = i << j; /* byte value after mask */
      vals[m] = g;
      }
    }
  }

public procedure Set4BitCMYKValues(cData, tfrFcn, vals, decode)
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals;
  DevImSampleDecode *decode; {
  register integer i, g, j, m, val;  
  integer nColorsM1 = cData.n - 1;
  integer lastPixVal = cData.first + nColorsM1 * cData.delta;
  real range, min;
  if (decode != NULL) {
    min = 15 * decode->min;
    range = decode->max - decode->min;
    }
  else {
    min = 0.0; range = 1.0;
    }
  for (i = 0; i < 16; i++) { 
    val = (integer)(17.0 * (min + i * range));
    if (val > 255) val = 255;
    else if (val < 0) val = 0;
    if (tfrFcn != NULL) val = 255 - tfrFcn[255 - val];
    val *= nColorsM1;
    g = (val/255) * cData.delta;
    for (j = 0; j <= 4; j += 4) { /* j is the left shift amount */
      m = i << j; /* byte value after mask */
      vals[m] = g;
      }
    }
  }

public procedure Set8BitCMYKValues(cData, tfrFcn, vals, decode)
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals;
  DevImSampleDecode *decode; {
  register integer i, g, val;  
  integer nColorsM1 = cData.n - 1;
  integer lastPixVal = cData.first + nColorsM1 * cData.delta;
  real range, min;
  if (decode != NULL) {
    min = 255 * decode->min;
    range = decode->max - decode->min;
    }
  else {
    min = 0.0; range = 1.0;
    }
  for (i = 0; i < 256; i++) { 
    val = (integer)(min + i * range);
    if (val > 255) val = 255;
    else if (val < 0) val = 0;
    if (tfrFcn != NULL) val = 255 - tfrFcn[255 - val];
    val *= nColorsM1;
    g = (val/255) * cData.delta;
    vals[i] = g;
    }
  }
  

public procedure SetRGBtoCMYKValues(cData, tfrFcn, vals, thresholds, decode)
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals, thresholds;
  DevImSampleDecode *decode; {
  register integer i, g, val;  
  integer nColorsM1 = cData.n - 1;
  integer lastPixVal = cData.first + nColorsM1 * cData.delta;
  real range, min;
  if (decode != NULL) {
    min = 255 * decode->min;
    range = decode->max - decode->min;
    }
  else {
    min = 0.0; range = 1.0;
    }
  for (i = 0; i < 256; i++) { 
    val = (integer)(min + i * range);
    if (val > 255) val = 255;
    else if (val < 0) val = 0;
    if (tfrFcn != NULL) val = 255 - tfrFcn[val];
    val *= nColorsM1;
    g = (val/255) * cData.delta;
    vals[i] = g;
    thresholds[i] = val % 255;
    }
  }

