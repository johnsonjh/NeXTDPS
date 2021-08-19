/*
  bitmapdevice.c

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

Original version: Doug Brotz: Sept. 13, 1983
Edit History:
Jim Sandman: Mon Jul 31 09:34:44 1989
Ivor Durham: Wed Jun 15 16:44:18 1988
Perry Caro: Thu Nov 10 13:22:02 1988
Paul Rovner: Thu Oct 26 16:53:52 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include BASICTYPES
#include DEVICE
#include GRAPHICS
#include LANGUAGE
#include ORPHANS
#include PSLIB
#include PUBLICTYPES
#include DEVCREATE

/* Cheat declarations to avoid importing internal device interface. */

typedef struct {
/*  Device fd; */
  DevBounds bbox;
  unsigned char *base;
  DevShort wbytes;
  DevShort depth;
  integer dpi, firstcolor, colors, nReds, nGreens, nBlues, nGrays; 
  boolean invert;
} Stuff, *PStuff;


private procedure PSCreateBuffer() { /* dpi width height depth colors firstcolor nReds nGreens nBlues nGrays invert createbuffer => handle */
  integer width, height, scanlinewidth; /* pixels, scanlines, bytes */
  integer firstcolor, colors, nReds, nGreens, nBlues, nGrays; 
  PStuff stuff;
  DevShort depth;
  boolean invert;
  unsigned char *base;
  invert = PopBoolean();
  nGrays = PopInteger();
  nBlues = PopInteger();
  nGreens = PopInteger();
  nReds = PopInteger();
  firstcolor = PopInteger();
  colors = PopInteger();
  depth = PopInteger();
  height = PopInteger();
  width = PopInteger();
  scanlinewidth = (((width * depth) + 31) >> 5) << 2;
  base = (unsigned char *)os_malloc(scanlinewidth * (height+1));
  stuff = (PStuff)os_malloc(sizeof(Stuff));
  stuff->base = base;
  stuff->bbox.x.l = 0;
  stuff->bbox.x.g = width;
  stuff->bbox.y.l = 0;
  stuff->bbox.y.g = height;
  stuff->dpi = PopInteger();
  stuff->wbytes = scanlinewidth;
  stuff->invert = invert;
  stuff->depth = depth;
  stuff->nGrays = nGrays;
  stuff->nBlues = nBlues;
  stuff->nGreens = nGreens;
  stuff->nReds = nReds;
  stuff->firstcolor = firstcolor;
  stuff->colors = colors;
  PushInteger((integer)stuff);
  }


private procedure PSSetBufferDevice() {
  PStuff stuff;
  Mtx m;
  DCPixelArgs pixelArgs;
  
  stuff = (PStuff)PopInteger();

  m.a = ((real)(stuff->dpi))/(72.0);
  m.b = 0.0;
  m.c = 0.0;
  m.d = -m.a;
  m.tx = 0.0;
  m.ty = 0.0;

  pixelArgs.depth = stuff->depth;
  pixelArgs.colors = stuff->colors;
  pixelArgs.firstColor = stuff->firstcolor;
  pixelArgs.nReds = stuff->nReds;
  pixelArgs.nGreens = stuff->nGreens;
  pixelArgs.nBlues = stuff->nBlues;
  pixelArgs.nGrays = stuff->nGrays;
  pixelArgs.invert = stuff->invert;
  
  NewDevice(
    FrameDevice(
      NULL, NULL,
      stuff->base, &m, stuff->wbytes, stuff->bbox.x.g,
      stuff->bbox.y.g,
      &pixelArgs));
  }


private procedure PSDestroyBuffer() {
  PStuff stuff;
  stuff = (PStuff)PopInteger();
  os_free(stuff->base);
  os_free(stuff);
  }

public procedure BitmapDevInit(reason)  InitReason reason;
{
switch (reason)
  {
  case init: break;
  case romreg: break;
  case ramreg: {
    RgstExplicit("createbuffer", PSCreateBuffer);
    RgstExplicit("setbufferdevice", PSSetBufferDevice);
    RgstExplicit("destroybuffer", PSDestroyBuffer);
    break;
    }
  }
}


