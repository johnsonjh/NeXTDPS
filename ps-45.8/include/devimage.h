/*  PostScript device image interface

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

Original version: Jim Sandman: Thu Jan 12 15:06:57 1989
Edit History:
Jim Sandman: Thu Jul 14 15:20:23 1988
Ivor Durham: Fri Jul 15 10:44:51 1988
Joe Pasqua: Wed Dec 14 14:46:09 1988
Paul Rovner: Mon Aug 21 10:49:26 1989
Ed Taft: Wed Dec 13 18:02:23 1989
End Edit History.
*/

#ifndef    DEVIMAGE_H
#define    DEVIMAGE_H

#include PUBLICTYPES
#include ENVIRONMENT
#include DEVICETYPES
#include DEVPATTERN

typedef struct _t_ImageProcsRec {
  PSCANTYPE (*GetWriteScanline)(/* char *data; integer y, xl, xg */);
    /* Returns a pointer to the start of a scanline. data is the data
    parameter to ImageTraps or ImageRun. y is the scanline number.
    xl is the pixel number of the first pixel written. xg is the pixel
    number of the first pixel after those written. If the device has a
    directly addressable framebuffer, the result should simply be
    framebase + y*framebytewidth. Otherwise the result should be a
    pointer to a scratch space. The caller guarantees to access the
    buffer between units containing xl and xg. The full scanline need
    not be allocated, but the pointer should point to the beginning of
    the scanline. y, xl and xg should be saved for updating the
    framebuffer later. */
  PSCANTYPE (*GetReadWriteScanline)(/* char *data; integer y, xl, xg */);
    /* Same as GetWriteScanline except that the scanline may be read
    from as well as written to. If there is no directly addressable
    ramebuffer, the scratch buffer should be initialized with data
    from the framebuffer. */
  procedure (*Begin)(/* char *data */);
    /* Called before the first call on GetWriteScanline or
    GetReadWriteScanline. data is the data parameter to 
    ImageTraps or ImageRun. */
  procedure (*End)(/* char *data */);
    /* Called after the last call on GetWriteScanline or
    GetReadWriteScanline. data is the data parameter to 
    ImageTraps or ImageRun. */
  } ImageProcsRec, *PImageProcs;
  
typedef struct  _t_ImageArgs {
  DevImage *image;
  PImageProcs procs;
  char *data;
  DevColorData red, green, blue, gray;
  integer firstColor, bitsPerPixel;
  PatternHandle pattern;
  DevMarkInfo *markInfo;
  PDevice device;
  } ImageArgs;
  
/* The following image procedures take the following parameters:
     integer items; the number of traps or runs
     DevTrap *t;    a vector of traps or NIL if run non-NIL
     DevRun *run    a vector of runs or NIL if t non-NIL
     ImageArgs *args  defined above
  
  Their names try to reflect the capability of the routine across various
  dimensions including:
    1, 3 and 4 color image data
    1,2,4,8 bit per sample source data
    1, 3 and 4 color device
    1,2,4,8 bit per pixel device or X capable of all 1,2,4,8,16
  The main idea is to develop a stable of imaging routines that OEMs can
  chose from.
    
*/
extern procedure ImS11D1X(); /* src: 1 clr 1 bit  dev: 1 clr 1,2,4,8 bpp; */
extern procedure ImS12D1X(); /* src: 1 clr 2 bit  dev: 1 clr 1,2,4,8 bpp; */
extern procedure ImS14D1X(); /* src: 1 clr 4 bit  dev: 1 clr 1,2,4,8 bpp; */
extern procedure ImS18D1X(); /* src: 1 clr 8 bit  dev: 1 clr 1,2,4,8 bpp; */
extern procedure ImS11D11(); /* src: 1 clr 1 bit  dev: 1 clr 1 bpp; */
extern procedure ImS12D11(); /* src: 1 clr 2 bit  dev: 1 clr 1 bpp; */
extern procedure ImS14D11(); /* src: 1 clr 4 bit  dev: 1 clr 1 bpp; */
extern procedure ImS18D11(); /* src: 1 clr 8 bit  dev: 1 clr 1 bpp; */
extern procedure ImS11D12(); /* src: 1 clr 1 bit  dev: 1 clr 2 bpp; */
extern procedure ImS12D12(); /* src: 1 clr 2 bit  dev: 1 clr 2 bpp; */
extern procedure ImS12D12NoTfr(); /* src: 1 clr 2 bit  dev: 1 clr 2 bpp; */
extern procedure ImS14D12(); /* src: 1 clr 4 bit  dev: 1 clr 2 bpp; */
extern procedure ImS18D12(); /* src: 1 clr 8 bit  dev: 1 clr 2 bpp; */
extern procedure ImS11D14(); /* src: 1 clr 1 bit  dev: 1 clr 4 bpp; */
extern procedure ImS12D14(); /* src: 1 clr 2 bit  dev: 1 clr 4 bpp; */
extern procedure ImS14D14(); /* src: 1 clr 4 bit  dev: 1 clr 4 bpp; */
extern procedure ImS18D14(); /* src: 1 clr 8 bit  dev: 1 clr 4 bpp; */
extern procedure ImS11D14(); /* src: 1 clr 1 bit  dev: 1 clr 8 bpp; */
extern procedure ImS12D18(); /* src: 1 clr 2 bit  dev: 1 clr 8 bpp; */
extern procedure ImS14D18(); /* src: 1 clr 4 bit  dev: 1 clr 8 bpp; */
extern procedure ImS18D18(); /* src: 1 clr 8 bit  dev: 1 clr 8 bpp; */
extern procedure ImS1XD1X(); /* s: 1 clr 2,4,8 bps  dev: 1 clr 1,2,4,8 bpp; */
extern procedure ImS1XD11(); /* s: 1 clr 2,4,8 bps  dev: 1 clr 1 bpp; */
extern procedure ImS1XD12(); /* s: 1 clr 2,4,8 bps  dev: 1 clr 2 bpp; */
extern procedure ImS1XD14(); /* s: 1 clr 2,4,8 bps  dev: 1 clr 4 bpp; */
extern procedure ImS1XD18(); /* s: 1 clr 2,4,8 bps  dev: 1 clr 8 bpp; */

extern procedure ImSXXD11();
extern procedure ImSXXD12();
extern procedure ImSXXD1X();
extern procedure ImSXXD3X();

extern procedure Im101();
extern procedure Im103();
extern procedure Im104();
extern procedure Im105();

#endif DEVIMAGE_H
