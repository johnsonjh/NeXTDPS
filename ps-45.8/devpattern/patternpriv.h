/*
				patternpriv.h

	     Copyright (c) 1988, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE:  All information  contained herein is  the property  of Adobe Systems
Incorporated.  Many  of   the intellectual and technical  concepts  contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only  to Adobe licensees  for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden  unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Edit History:
Scott Byer: Wed May 17 17:14:59 1989
Jim Sandman: Fri May 12 10:51:16 1989
Paul Rovner: Fri Dec 29 11:00:08 1989
End Edit History.
*/

#ifndef    PATTERNPRIV_H
#define    PATTERNPRIV_H

#include PUBLICTYPES
#include DEVICETYPES


extern integer maxPatternSize, maxTotalPatternSize;
extern PSCANTYPE grayPatternBase;

extern procedure DestroyPat(/* PatternHandle h; */);
  
/* MAXPAT must be no larger than 256 */
#define MAXPAT (256)
#define MAXCOLOR 255

#define MAXSCRN 32

typedef Card8 DGColor;
#define dgGray 3
#define dgRed 2
#define dgGreen 1
#define dgBlue 0
#define dgNColors 4

typedef struct {
  DevScreen *screen;
  Card16 maxGray; /* This pattern is usable if minGray <= gray < maxGray */
  Card8 minGray;
  DGColor dgColor;
  PatternHandle h;
  integer lastX;
  integer lastUsed;
  PatternData data;
  integer priv;
  } PatCacheInfo, *PPatCacheInfo;

typedef PCard8 CachedHalftone[dgNColors], *PCachedHalftone;

/* Private data associated with a DevScreen:
   The chunkSize below is 0 if the threshold array is out on the disk.
   Otherwise it is the actual size of allocated memory. */
typedef struct {
  Card32 age;
  Card32 chunkSize;
  CachedHalftone ch;
  } ScreenPrivate, *PScreenPrivate;

extern PPatCacheInfo *patterns;
extern integer patTimeStamp;
extern integer patID;

extern SetupCacheRec(/* DevHalftone *halftone; */);

extern integer GCD(/* integer u, v */);

extern PSCANTYPE AllocPatternStorage(/* integer nUnits */);
extern PScreenPrivate AllocScreenPrivate();
extern PCard8 AllocInfoVector();
extern procedure RollPattern (/* integer leftRollPatternData *data; */);

extern PPatCacheInfo SetupGrayPattern(/* 
  PatternHandle h; DevMarkInfo *markinfo; PatternData *data;
  DevColorData cData; integer log2BPP, color; */);

extern boolean ConstantColor(/*
  integer nGrays; DevScreen *screen; PCard8 gray; integer *minGray, *maxGray;*/);

extern SCANTYPE ConstSetup(/*
  DevColorData cData; integer color, bitsPerPixel; */);

extern procedure SetupDeepOnes(/* integer bitsPerPixel; */);

extern PPatCacheInfo SetPatInfo(
  /* DevScreen *screen; PatternData *data;
  integer minGray, maxGray; PatternHandle h; */);

extern integer ColorPatInfo(
  /* PatternHandle h; DevColorData red, green, blue, gray; */);

extern integer GrayPatInfo(
  /* PatternHandle h; DevColorData red, green, blue, gray; */);

extern boolean ValidateTA(/* DevScreen *s */);
 
typedef struct {
  PatternProcsRec procs;
  boolean positive;
  } MaskPatRec, *MaskPatHandle;
  
typedef struct {
  PatternProcsRec procs;
  boolean oneMeansWhite;
  } MonoPatRec, *MonoPatHandle;
  
typedef struct {
  PatternProcsRec procs;
  DevColorData data;
  integer bitsPerPixel, log2BPP;
  } GrayPatRec, *GrayPatHandle;
  
typedef struct {
  PatternProcsRec procs;
  DevColorData red, green, blue, gray;
  integer firstColor, bitsPerPixel, log2BPP;
  } ColorPatRec, *ColorPatHandle;
  
#endif PATTERNPRIV_H
