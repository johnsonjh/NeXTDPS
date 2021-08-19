/*  devimage.h

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

Original version:
Edit History:
Jim Sandman: Thu Jan 19 09:02:16 1989
Joe Pasqua: Tue Feb 28 10:58:19 1989
Paul Rovner: Wed Aug 23 11:48:56 1989
End Edit History.

*/

#ifndef IMAGEPRIV_H
#define IMAGEPRIV_H

#define LowMask (0xFFFFL)
#define uchar unsigned char

#if SWAPBITS == DEVICE_CONSISTENT
#define SHIFTPIXEL(g,b) ((g) << (SCANUNIT - BPP - (b)))
#else
#define SHIFTPIXEL(g,b) ((g) << (b))
#endif

extern PSCANTYPE pixelVals, pixelThresholds;
extern PSCANTYPE redValues, redThresholds, greenValues, greenThresholds;
#define blueValues pixelValues
#define blueThresholds pixelThresholds

extern PSCANTYPE GetPixBuffers();
extern PSCANTYPE GetRedPixBuffers();
extern PSCANTYPE GetGreenPixBuffers();
extern PSCANTYPE GetWhitePixBuffers();
#define GetBluePixBuffers GetPixBuffers

extern procedure Set1BitValues(/*
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals, thresholds; 
  DevImSampleDecode *decode; */);
extern procedure Set2BitValues(/*
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals, thresholds;
  DevImSampleDecode *decode; */);
extern procedure Set4BitValues(/*
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals, thresholds; 
  DevImSampleDecode *decode; */);
extern procedure Set8BitValues(/*
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals, thresholds; 
  DevImSampleDecode *decode; */);

extern procedure SetRGBtoCMYKValues(/*
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals, thresholds;
  DevImSampleDecode *decode; */);
extern procedure Set1BitCMYKValues(/*
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals;
  DevImSampleDecode *decode; */);
extern procedure Set2BitCMYKValues(/*
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals;
  DevImSampleDecode *decode; */);
extern procedure Set4BitCMYKValues(/*
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals;
  DevImSampleDecode *decode; */);
extern procedure Set8BitCMYKValues(/*
  DevColorData cData; PCard8 tfrFcn; PSCANTYPE vals;
  DevImSampleDecode *decode; */);

#endif IMAGEPRIV_H
