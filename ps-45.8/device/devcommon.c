/* devcommon.c -- procs and vars shared by different device modules

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

*/

#include PACKAGE_SPECS
#include DEVICE
#include DEVPATTERN

#include "devcommon.h"
#include "genericdev.h"

public PDevice CurrentDevice;

private real colorWr, colorWg, colorWb;

#define fp255 255.0

public procedure DevNoOp (device) PDevice device; {
  }
  
public boolean DevAlwaysFalse (device) PDevice device; {
  return false;
  }

/* This is to get 0.833333 * 255 + fpHalf1 to round to 213 instead of 212 */
#define fpHalf1 0.50001

public DevColor ConvertColorRGB (device, colorSpace, input, tfr, priv, gt, r)
  PDevice device; integer colorSpace; DevInputColor *input;
  DevTfrFcn *tfr; DevPrivate *priv; DevGamutTransfer gt; 
  DevRendering r; {
  integer red, green, blue, gray;
  real v;
  DevColorVal result;
  switch (colorSpace) {
    case DEVGRAY_COLOR_SPACE: {
      red = green = blue = gray =
        (integer)(input->value.gray * fp255 + fpHalf1);
      break;
      }
    case DEVRGB_COLOR_SPACE: {
      red = (integer) (input->value.rgb.red * fp255 + fpHalf1);
      green = (integer) (input->value.rgb.green * fp255 + fpHalf1);
      blue = (integer) (input->value.rgb.blue * fp255 + fpHalf1);
      gray = (integer)(
	colorWr * input->value.rgb.red + colorWg * input->value.rgb.green +
	colorWb * input->value.rgb.blue + fpHalf1);
      break;
      }
    case DEVCMYK_COLOR_SPACE: {
      red = (integer) ((fpOne - input->value.cmyk.cyan) * fp255 + fpHalf1);
      green = (integer) ((fpOne - input->value.cmyk.magenta) * fp255 + fpHalf1);
      blue = (integer) ((fpOne - input->value.cmyk.yellow) * fp255 + fpHalf1);
      gray = (integer) (input->value.cmyk.black * fp255 + fpHalf1);
      red   -= gray;  if (red   < 0) red   = 0;
      green -= gray;  if (green < 0) green = 0;
      blue  -= gray;  if (blue  < 0) blue  = 0;
      v = 
 	fp255 -
 	  (colorWr * input->value.cmyk.cyan +
 	   colorWg * input->value.cmyk.magenta +
 	   colorWb * input->value.cmyk.yellow +
 	   fp255 * input->value.cmyk.black);
      gray = (integer)(v + fpHalf1);
      if (gray < 0) gray = 0;
      break;
      }
    }
  if (tfr)
    {
    if (tfr->red) red = tfr->red[red];
    if (tfr->green) green = tfr->green[green];
    if (tfr->blue) blue = tfr->blue[blue];
    if (tfr->white) gray = tfr->white[gray];
    }
  result.red = red;
  result.green = green;
  result.blue = blue;
  result.white = gray;
  return *((DevColor *)&result);
  }

public DevColor ConvertColorCMYK (device, colorSpace, input, tfr, priv, gt, r)
  PDevice device; integer colorSpace; DevInputColor *input;
  DevTfrFcn *tfr; DevPrivate *priv; DevGamutTransfer gt; 
  DevRendering r; {
  integer red, green, blue, gray;
  DevColorVal result;
  switch (colorSpace) {
    case DEVGRAY_COLOR_SPACE: {
      red = green = blue = gray =
        (integer)(input->value.gray * fp255 + fpHalf1);
      break;
      }
    case DEVRGB_COLOR_SPACE: {
      integer removal, black;
      red = (integer) (input->value.rgb.red * fp255 + fpHalf1);
      green = (integer) (input->value.rgb.green * fp255 + fpHalf1);
      blue = (integer) (input->value.rgb.blue * fp255 + fpHalf1);
      gray = (red > green) ? red : green;
      if (gray < blue) gray = blue;
      removal = black = MAXCOLOR - gray;
      if (tfr && tfr->ucr != NULL)
	removal = tfr->ucr[removal];
      red += removal;  green += removal;  blue += removal;
      if (tfr && tfr->ucr != NULL) {
	if (red > MAXCOLOR) red = MAXCOLOR;
	else if (red < 0) red = 0;
	if (green > MAXCOLOR) green = MAXCOLOR;
	else if (green < 0) green = 0;
	if (blue > MAXCOLOR) blue = MAXCOLOR;
	else if (blue < 0) blue = 0;
	}
      if (tfr && tfr->bg != NULL)
	gray = tfr->bg[black];
      else gray = MAXCOLOR - black;
      break;
      }
    case DEVCMYK_COLOR_SPACE: {
      red = (integer) (input->value.cmyk.cyan * fp255 + fpHalf1);
      green = (integer) (input->value.cmyk.magenta * fp255 + fpHalf1);
      blue = (integer) (input->value.cmyk.yellow * fp255 + fpHalf1);
      gray = (integer) (input->value.cmyk.black * fp255 + fpHalf1);
      break;
      }
    }
  if (tfr)
    {
    if (tfr->red) red = tfr->red[red];
    if (tfr->green) green = tfr->green[green];
    if (tfr->blue) blue = tfr->blue[blue];
    if (tfr->white && colorSpace != DEVRGB_COLOR_SPACE)
      gray = tfr->white[gray];
    }
  result.red = red;
  result.green = green;
  result.blue = blue;
  result.white = gray;
  return *((DevColor *)&result);
  }

public procedure IniDevCommon() {
  colorWr = fpp3 * fp255;
  colorWg = fpp59 * fp255;
  colorWb = fp255 - (colorWr + colorWg);
  }
