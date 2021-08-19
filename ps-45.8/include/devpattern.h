/*  PostScript device pattern interface

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

Original version: Jim Sandman: Thu Aug  3 09:24:06 1989
Edit History:
Jim Sandman: Tue May 17 10:25:29 1988
Ivor Durham: Wed Jul 13 09:48:37 1988
Perry Caro: Tue Nov 15 16:54:52 1988
End Edit History.
*/

#ifndef    DEVPATTERN_H
#define    DEVPATTERN_H

#include PUBLICTYPES
#include ENVIRONMENT

#ifndef SCANUNIT
#define SCANUNIT 32
#endif

#if (SCANUNIT!=8) && (SCANUNIT!=16) && (SCANUNIT!=32)
#define SCANUNIT 32
#endif (SCANUNIT!=8) && (SCANUNIT!=16) && (SCANUNIT!=32)

#if SCANUNIT==8
#define SCANSHIFT 3
#define SCANMASK 07
#define SCANTYPE Card8
#define LASTSCANVAL 0xFF
#endif SCANUNIT==8

#if SCANUNIT==16
#define SCANSHIFT 4
#define SCANMASK 017
#define SCANTYPE Card16
#define LASTSCANVAL 0xFFFF
#endif SCANUNIT==16

#if SCANUNIT==32
#define SCANSHIFT 5
#define SCANMASK 037
#define LASTSCANVAL 0xFFFFFFFF
#define SCANTYPE Card32
#endif SCANUNIT==32

typedef SCANTYPE *PSCANTYPE;

/*
  DEVICE_CONSISTENT indicates whether or not the bit order for the display
  controller is consistent with that of the ISP.  For configurations in
  which the bit orders are different, DEVICE_CONSISTENT must be defined in
  CFLAGS as 0 because the default value is 1.
 */

#ifndef	DEVICE_CONSISTENT
#define	DEVICE_CONSISTENT 1
#endif	DEVICE_CONSISTENT

#if	(!DEVICE_CONSISTENT) && (SCANUNIT != 8)
ConfigurationError ("SCANUNIT must be 8 for this configuration");
#endif	(!DEVICE_CONSISTENT) && (SCANUNIT != 8)

#if (SWAPBITS == DEVICE_CONSISTENT)
#define LSHIFT >>
#define RSHIFT <<
#define LSHIFTEQ >>=
#define RSHIFTEQ <<=
#else (SWAPBITS == DEVICE_CONSISTENT)
#define LSHIFT <<
#define RSHIFT >>
#define LSHIFTEQ <<=
#define RSHIFTEQ >>=
#endif (SWAPBITS == DEVICE_CONSISTENT)

typedef struct _t_PatternData {
  PSCANTYPE start, end;
  integer width;
  boolean constant;
  SCANTYPE value;
  integer id;
  } PatternData;
  /* Defines results of SetupPattern routine.
  The field start points to the first SCANTYPE in the pattern.
  The field end points to the first SCANTYPE after the pattern.
  The pattern is width SCANTYPEs wide. If the pattern fits in a single
  SCANTYPE, constant is true and value is the pattern value.
  Otherwise, constant is false. */

typedef struct _t_DevColorVal {
  Card8 red, green, blue, white;
  } DevColorVal;
  /* Actual structure of value of DevColor in DevMarkInfo. Assumes the
  structure fits into a pointer variable. */
  
typedef struct _t_PatternProcsRec {
  procedure (*setupPattern)(/*
    PatternHandle h; DevMarkInfo *markinfo; PatternData *data; */);
    /* Fills in the values of *data for pattern defined by markinfo and y. 
    markinfo->halftone specifies the threshold values used, and in
    conjunction with markinfo->color determines the pattern values.
    The origin of the pattern is defined by markinfo->origin and
    markinfo->screenphase. data->patternRow is set to the row of
    he pattern that corresponds to scanline y on the physical device. */
  procedure (*destroyPattern)(/* PatternHandle h; */);
    /* Destroys the pattern object and all resources it maintains. The
    pattern handle is no longer valid after it is destroyed. */
  integer (*patternInfo)(/*
    PatternHandle h; DevColorData *red, *green, *blue, *gray; 
    integer *firstColor;*/);
    /* Fills in red, green, blue, gray and firstColor with information
    about the colors that the pattern object uses.  Returns
    bitsPerPixel of pattern object. */
  } PatternProcsRec, *PatternHandle;
  
#define SetupPattern(h, info, data) (*(h)->setupPattern)((h), (info), (data))
#define DestroyPattern(h) (*(h)->destroyPattern)((h))
#define PatternInfo(h, red, green, blue, gray, firstColor) \
  (*(h)->patternInfo)((h), (red), (green), (blue), (gray), (firstColor))

extern PSCANTYPE GetPatternRow(/*
  DevMarkInfo *markinfo; PatternData *data; integer y; */);
  /* Returns the row of the pattern specified by data that corresponds
  to the scanline y on the physical device. */

extern PatternHandle MonochromePattern(/* boolean oneMeansWhite; */);
  /* Returns a handle on a pattern object for single bit-per-pixel
  displays. The boolean parameter specifies whether a one bit is
  black or white. */
  
typedef struct _t_DevColorData {
  integer first, n, delta;
  } DevColorData;
  /* This structure specifies a how to generate pixel information for a 
     color ramp. first is the pixel value of the highest intensity value, n
     is the number of intensity values for the color and delta is the
     difference of adjacent values.  For example a four bit gray color
     map can be specified as {0, 16, 1}.  A color cube can be specified
     by three structures where the pixel value of the color is the sum of the pixel values of the individual components. A 16 entry color map with 2 of each color and
     8 grays can be specified as red = {0, 2, 1}, green = {0, 2, 2},
     blue = {0, 2, 4}, and gray = {8, 8, 1}.  A monochrome  display
     where 0 is white and 1 is black can be specified by {0, 2, 1}. If
     1 is white and 0 is black the specification is {1, 2, -1}.
   */ 
  
extern PatternHandle GrayPattern(/*
  DevColorData gray; integer bitsPerPixel; */);
  /* Returns a handle on a pattern object for a grayscale display
  with bitsPerPixel bits-per-pixel. gray specifies the pixel information. */

#define ColorPattern RGBPattern

extern PatternHandle RGBPattern(/*
  DevColorData red, green, blue, gray; integer firstColor, bitsPerPixel; */);
  /* Returns a handle on a pattern object for a color display
  with bitsPerPixel bits-per-pixel. red, green and blue specify a color
  cube, one of whose corners is the pixel value 0. firstColor can be
  used to offset the color cube in the color map. gray specifies an
  optional gray ramp.
   */

extern PatternHandle ConstGrayPattern(/*
  DevColorData gray; integer bitsPerPixel; */);
  /* Returns a handle on a pattern object for a grayscale display
  with bitsPerPixel bits-per-pixel. gray specifies the pixel information.
  This implementation does no halftoning so the pattern always fits in
  a scanunit. gray.n must be at least 16.
  */

#define ConstColorPattern ConstRGBPattern

extern PatternHandle ConstRGBPattern(/*
  DevColorData red, green, blue, gray; integer firstColor, bitsPerPixel; */);
  /* Returns a handle on a pattern object for a color display
  with bitsPerPixel bits-per-pixel. red, green and blue specify a color
  cube, one of whose corners is the pixel value 0. firstColor can be
  used to offset the color cube in the color map. gray specifies an
  optional gray ramp. This implementation does no halftoning so the
  pattern always fits in a scanunit.  red.n, green.n and blue.n must be
  at least 16.  If gray.n is nonzero, it must be at least 16.  */

extern PatternHandle CMYKPattern(/*
  DevColorData cyan, magenta, yellow, black;
  integer firstColor, bitsPerPixel; */);
  /* Returns a handle on a pattern object for a CMYK device with
  bitsPerPixel bits-per-pixel. cyan, magenta, yellow and black specify
  a color hypercube, one of whose corners is the pixel value 0.
  firstColor can be used to offset the color cube.  */

extern PatternHandle ConstCMYKPattern(/*
  DevColorData red, green, blue, gray; integer firstColor, bitsPerPixel; */);
  /* Returns a handle on a pattern object for a CMYK with bitsPerPixel
  bits-per-pixel. cyan, magenta, yellow and black specify a color
  hypercube, one of whose corners is the pixel value 0.  firstColor can
  be used to offset the color cube. This implementation does no
  halftoning so the pattern always fits in a scanunit.  cyan.n,
  magenta.n, yellow.n and black.n must be at least 16.  */

extern PatternHandle MaskPattern(/* boolean positive */);
  /* Returns a handle on a pattern object for building masks.
  The pattern data is always a constant value. If positive is true,
  the value is all ones (0xFFFFFFFF) otherwise the value is zero. */


extern procedure InitPatternImpl(
  /* integer maxPatternSize, maxTotalPatternSize,
     maxThrArrSize, maxTotThrArrSize; */);
  /* This routine initializes the pattern implementation. The argument
  maxPatternSize is the maximum number of bytes a single pattern and
  the argument maxTotalPatternSize is the maximum number of bytes of
  patterns that are cached by the implementation. 
  The argument maxThrArrSize is the maximum number of bytes a single
  threshold array may be and the argument maxTotThrArrSize is the
  maximum number of bytes for all in-memory threshold arrays. Other
  threshold arrays may spill out onto the disk. */
  
extern boolean CheckScreenDims(/* integer width, height; */);
  /* Provides an implementation of the DevCheckScreenDims routine 
  for device implementations that use the pattern implementation. It limits
  the screen dimensions so that a single pattern will not exceed the
  maxPatternSize argument to InitPatternImpl. */

extern DevHalftone *AllocHalftone(/*
  DevShort wWhite, hWhite, wRed, hRed, wGreen, hGreen, wBlue, hBlue */);
  /* Provides an implementation of the DevAllocHalftone routine for
  device implementations that use the pattern implementation.  */

extern procedure FlushHalftone(/* DevHalftone *halftone; */);
  /* Provides an implementation of the DevFlushHalftone routine for
  device implementations that use the pattern implementation.  */

extern boolean IndependentColors();
  /* Provides an implementation of the IndependentColors routine for device
  implementations that use the pattern implementation. It returns false. */

extern readonly SCANTYPE leftBitArray[SCANUNIT];
extern readonly SCANTYPE rightBitArray[SCANUNIT];
extern PSCANTYPE onebit;

extern PSCANTYPE *deepOnes;
  /* Indexed by log2BD. Element is an array indexed by bit position within a unit.
     Subarray element is the mask for the pixel beginning at the bit position.
     Only aligned bit indices are given.
  */

extern PSCANTYPE *deepPixOneVals;
  /* Indexed by log2BD. Element is an array indexed by pixel position within a unit.
     Subarray element is a unit with the value 1 for the pixel.
  */

extern PSCANTYPE *deepPixOnes;
  /* Indexed by log2BD. Element is an array indexed by pixel position within a unit.
     Subarray element is the mask for the pixel.
  */

extern PSCANTYPE *swapPixOnes;
  /* Indexed by log2BD. Element is an array indexed by
     ((SCANUNIT >> framelog2BD) - pixel index - 1) within a unit.
     Subarray element is the mask for the pixel.
  */


#endif DEVPATTERN_H
/* v002 sandman Fri Jun 3 10:21:32 PDT 1988 */
/* v002 sandman Thu Aug 11 11:42:16 PDT 1988 */
/* v003 caro Tue Nov 8 14:24:07 PST 1988 */
/* v004 pasqua Wed Dec 14 17:50:59 PST 1988 */
/* v005 sandman Thu Jan 12 14:52:13 PST 1989 */
/* v005 pasqua Mon Feb 6 13:20:09 PST 1989 */
/* v006 sandman Mon Mar 6 10:59:55 PST 1989 */
/* v006 bilodeau Fri Apr 21 12:53:24 PDT 1989 */
/* v007 byer Mon May 15 16:49:06 PDT 1989 */
/* v009 sandman Mon Oct 16 15:18:40 PDT 1989 */
/* v011 taft Fri Jan 5 15:34:16 PST 1990 */
