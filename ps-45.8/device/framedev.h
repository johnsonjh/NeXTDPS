/* PostScript frame buffer device definitions. A frame device is a direct
   descendent of a generic device, and serves as the ancestor of other
   devices, for example mask devices.

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

#ifndef FRAMEDEVICE_H
#define FRAMEDEVICE_H

#include DEVICE
#include DEVCREATE
#include DEVPATTERN

#include "devmark.h"
#include "genericdev.h"


typedef struct _t_FmStuff{ /* concrete "Device" type for a frame device */
  GenStuff  gen;		   /* must be first */
  PSCANTYPE base;	       /* points to base of frame */
  integer bytewidth;	   /* bytes per scan line */
  integer height;	       /* scanlines */
#if (MULTICHROME == 1)
  integer log2BD;	       /* log2 of fd.bitsPerPixel */
#endif
  PatternHandle pattern;
  PPixelBuffer (*frameProc)(/*
    PPixelBuffer frameBuffer; unsigned char *procHook;*/);
  unsigned char *procHook;
  DevTrackingProcs trackingProcs;
} FmStuff, *PFmStuff;

extern DevProcs *fmProcs;
extern PMarkProcs fmMarkProcs;
extern PImageProcs fmImageProcs;

extern integer framebytewidth;
#if (MULTICHROME == 1)
extern Card8 framelog2BD;
#endif
extern PSCANTYPE framebase;

extern procedure IniFmDevImpl();

extern procedure SetFmDeviceMetrics(/*
  PFmStuff fd; PSCANTYPE base;
  integer scanlinewidth, height;
  PDCPixelArgs pixelArgs; */);

extern procedure AdjustDevMatrix(/* PMtx m; integer width, height; */);

/* The following procs implement fill and stroke marking for frame devices */

extern procedure BlackMasksMark
  (/* DevMask *masks; integer items; MarkArgs *args; */);
extern procedure WhiteMasksMark
  (/* DevMask *masks; integer items; MarkArgs *args; */);
extern procedure GrayMasksMark
  (/* DevMask *masks; integer items; MarkArgs *args; */);
extern procedure ConstantMasksMark
  (/* DevMask *masks; integer items; MarkArgs *args; */);
extern procedure ClippedMasksMark
  (/* DevTrap *t; DevRun *run; DevMask *masks; integer items; MarkArgs *args; */);
extern procedure BlackTrapsMark
  (/* DevTrap *t; integer items; MarkArgs *args; */);
extern procedure WhiteTrapsMark
  (/* DevTrap *t; integer items; MarkArgs *args; */);
extern procedure GrayTrapsMark
  (/* DevTrap *t; integer items; MarkArgs *args; */);
extern procedure ConstantTrapsMark
  (/* DevTrap *t; integer items; MarkArgs *args; */);
extern procedure BlackRunMark
  (/* DevRun *run; integer items; MarkArgs *args; */);
extern procedure WhiteRunMark
  (/* DevRun *run; integer items; MarkArgs *args; */);
extern procedure GrayRunMark
  (/* DevRun *run; integer items; MarkArgs *args; */);
extern procedure ConstantRunMark
  (/* DevRun *run; integer items; MarkArgs *args; */);

#if DPSXA
extern DevCd devXAOffset;
#endif /* DPSXA */
#endif FRAMEDEVICE_H
