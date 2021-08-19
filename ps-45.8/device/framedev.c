/* PostScript generic frame buffer device module

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

Edit History:
04Apr90 Jack	timeslicing images added to Mark

*/

#include PACKAGE_SPECS
#include DEVICE
#include DEVCREATE
#include DEVIMAGE
#include DEVPATTERN
#include EXCEPT
#include POSTSCRIPT
#include PSLIB

#include "devcommon.h"
#include "framedev.h"
#include "framemaskdev.h"

#if (OS == os_mpw)
#include DPSMACSANITIZE
#endif (OS == os_mpw)

#ifndef	DISABLEDPS
#define	DISABLEDPS 0
#endif	DISABLEDPS

public DevProcs *fmProcs;
public PMarkProcs fmMarkProcs;
public PImageProcs fmImageProcs;

public PSCANTYPE framebase;
public integer framebytewidth;
#if (MULTICHROME == 1)
public Card8 framelog2BD;
#endif

#if (MULTICHROME == 1)
private DevShort DepthToLog2BD(depth) DevShort depth; {
  switch (depth) {
    case 1: return 0;
    case 2: return 1;
    case 4: return 2;
    case 8: return 3;
    case 16: return 4;
    case 32: return 5;
    default:
      CantHappen();
    }
}
#endif

public procedure SetFmDeviceMetrics(
  fd, base, scanlinewidth, height, pixelArgs)
  PFmStuff fd; PSCANTYPE base;
  integer scanlinewidth, height;
  PDCPixelArgs pixelArgs;
  {
  boolean invert = pixelArgs->invert;
  PatternHandle pattern;
#if (MULTICHROME == 1)
  integer depth, colors, firstColor, nReds, nGreens, nBlues, nGrays;
  depth = pixelArgs->depth;
  colors = pixelArgs->colors;
  firstColor = pixelArgs->firstColor;
  nReds = pixelArgs->nReds;
  nGreens = pixelArgs->nGreens;
  nBlues = pixelArgs->nBlues;
  nGrays = pixelArgs->nGrays;

  if (colors == 4) {
    DevColorData dmy;
    pattern = (PatternHandle)ConstCMYKPattern(dmy, dmy, dmy, dmy, 0, 32);
    }
  else if (colors == 3) {
    DevColorData red, green, blue, gray;
    red.n = nReds; 
    red.delta = nBlues * nGreens;
    green.n = nGreens;
    green.delta = nBlues;
    blue.n = nBlues;
    gray.n = nGrays;
    if (nReds == nGreens && nReds == nBlues && nReds == nGrays) {
      gray.first = firstColor;
      gray.delta = 1 + nBlues + nBlues * nGreens;
      }
    else {
      gray.first = firstColor + nReds * nGreens * nBlues;
      gray.delta = 1;
      }
    if (!invert) {
      red.first = green.first = blue.first = 0;
      blue.delta = 1;
      }
    else {
      red.first = (red.n-1)* red.delta;
      green.first = (green.n-1)* green.delta;
      blue.first = (blue.n-1);
      gray.first += (gray.n-1) * gray.delta;
      red.delta = -red.delta;
      green.delta = -green.delta;
      blue.delta = -1;
      gray.delta = -gray.delta;
      }
    pattern = RGBPattern(red, green, blue, gray, firstColor, depth);
    }
  else {
    if (!invert && firstColor == 0 && nGrays == 4 && depth == 2) {
      extern PatternHandle GryPat4Of4();
      pattern = GryPat4Of4();
      }
    else if (nGrays != 2) {
      DevColorData cData;
      if (invert) {
	cData.first = firstColor + nGrays - 1;
	cData.n = nGrays;
	cData.delta = -1;
        }
      else {
        cData.first = firstColor;
	cData.n = nGrays;
	cData.delta = 1;
        }
      pattern = GrayPattern(cData, depth);
      }
    else 
      pattern = MonochromePattern(invert);
    }
  fd->log2BD = DepthToLog2BD(depth);
  fd->gen.colors = colors;
#else
  pattern = MonochromePattern(invert);
#endif
  fd->base = base;
  fd->bytewidth = scanlinewidth; /* bytes: width of the bitmap */
  fd->height = height;	/* scanlines: height of the visible region */
  fd->pattern = pattern;
  } /* SetFmDeviceMetrics */

private procedure FmGoAway (device) PDevice device; {
  PFmStuff fmStuff = (PFmStuff) device;
  if (fmStuff->trackingProcs && fmStuff->trackingProcs->goAway)
    (*fmStuff->trackingProcs->goAway)(fmStuff->procHook);
  DestroyPattern(fmStuff->pattern);
  os_free((char *)fmStuff);
  }

private procedure FmWakeup (device) PDevice device; {
  PFmStuff fmStuff = (PFmStuff) device;
  framebase = fmStuff->base;
  framebytewidth = fmStuff->bytewidth;
#if (MULTICHROME == 1)
  framelog2BD = fmStuff->log2BD;
#endif
  CurrentDevice = device;
  if (fmStuff->trackingProcs && fmStuff->trackingProcs->wakeup)
    (*fmStuff->trackingProcs->wakeup)(fmStuff->procHook);
  }

private procedure FmSleep (device) PDevice device; {
  PFmStuff fmStuff = (PFmStuff) device;
  if (fmStuff->trackingProcs && fmStuff->trackingProcs->sleep)
    (*fmStuff->trackingProcs->sleep)(fmStuff->procHook);
  }

#if (!DISABLEDPS)
private boolean FmPreBuiltChar(device, args)
  PDevice device; PreBuiltArgs *args; {
  return GetPreBuiltChar(1, device->maskID, args);
  }
#endif (!DISABLEDPS)

private procedure FmDeviceInfo(device, args)
  PDevice device; DeviceInfoArgs *args; {
#if (MULTICHROME == 1)
  DevColorData red, green, blue, gray;
  integer firstColor, colors;
  PFmStuff fm = (PFmStuff)device;
  (void)PatternInfo(fm->pattern, &red, &green, &blue, &gray, &firstColor);
  if (red.n == 0) {
    args->dictSize = 3;
    colors = 1;
    }
  else {
    if (gray.n != 0) {
      args->dictSize = 6; colors = 4; }
    else {
      args->dictSize = 5; colors = 3; }
    (*args->AddIntEntry)("RedValues", red.n, args);
    (*args->AddIntEntry)("GreenValues", green.n, args);
    (*args->AddIntEntry)("BlueValues", blue.n, args);
    }
  if (colors != 3)
    (*args->AddIntEntry)("GrayValues", gray.n, args);
  (*args->AddIntEntry)("Colors", colors, args);
  (*args->AddIntEntry)("FirstColor", firstColor, args);
#else
  (*args->AddIntEntry)("GrayValues", 2, args);
  (*args->AddIntEntry)("Colors", 1, args);
  (*args->AddIntEntry)("FirstColor", 0, args);
#endif
  }
  
private integer FmSetupMark (device, clip, args)
  PDevice device; DevPrim **clip; MarkArgs *args; {
  PFmStuff fmStuff = (PFmStuff) device;
  DevPoint translation;

  args->procs = fmMarkProcs;
  args->pattern = fmStuff->pattern;
  if (args->markInfo->halftone == NIL)
    args->markInfo->halftone = defaultHalftone;
  SetupPattern(args->pattern, args->markInfo, &args->patData);
  
  (*device->procs->WinToDevTranslation)(device, &translation);
  args->markInfo->offset.x -= translation.x;
  args->markInfo->offset.y -= translation.y;
  return 1;
  }

private PSCANTYPE FmGetWriteScanline(data, y, xl, xg)
  char *data; integer y, xl, xg; {
#if DPSXA
  return ((PSCANTYPE) ((integer) framebase
    	+ ((devXAOffset.x << framelog2BD) >> 3) 
		+ (y + devXAOffset.y) * framebytewidth));
#else /* DPSXA */
  return (PSCANTYPE)(((PCard8)framebase) + y * framebytewidth);
#endif /* DPSXA */
  }

private PSCANTYPE FmGetReadWriteScanline(data, y, xl, xg)
  char *data; integer y, xl, xg; {
#if DPSXA
  return ((PSCANTYPE) ((integer) framebase
    	+ ((devXAOffset.x << framelog2BD) >> 3) 
		+ (y + devXAOffset.y) * framebytewidth));
#else /* DPSXA */
  return (PSCANTYPE)(((PCard8)framebase) + y * framebytewidth);
#endif /* DPSXA */
  }

private procedure FmSetupImageArgs(device, args)
  PDevice device; ImageArgs *args; {
  PFmStuff fm = (PFmStuff)device;

  args->pattern = fm->pattern;
#if (MULTICHROME == 1)
  args->bitsPerPixel = 1 << fm->log2BD;
#else
  args->bitsPerPixel = 1;
#endif

  (void)PatternInfo(
    args->pattern, &args->red, &args->green, &args->blue, &args->gray,
    &args->firstColor);
  args->procs = fmImageProcs;
  args->data = NIL;
  args->device = device;
  }

public procedure AdjustDevMatrix(m, width, height)
  PMtx m;
  integer width, height;
  {
  real w = ((real)(width)/2.0);
  real h = ((real)(height)+1.0)/2.0;
  if (m->a < 0.0 || m->c < 0.0)
    m->tx = w - m->tx;
  else
    m->tx -= w;
  if (m->b < 0.0 || m->d < 0.0)
    m->ty = h - m->ty;
  else
    m->ty -= h;
  }

 public PDevice FrameDevice(
  frameProc, procHook,
  base,
  pmtx, byteWidth, pixelWidth, height,
  pixelArgs, trackingProcs)
  PPixelBuffer (*frameProc)(/*
    PPixelBuffer frameBuffer; unsigned char *procHook;
    boolean clear; unsigned char *pageHook; */);
  unsigned char *procHook;
  PPixelBuffer base;
  PMtx pmtx;
  integer byteWidth, pixelWidth, height;
  PDCPixelArgs pixelArgs;
  DevTrackingProcs trackingProcs;
  {
  PFmStuff fd;
#if (MULTICHROME == 1)
  if (pixelArgs->colors == 4 && pixelArgs->depth != 32)
    return NIL;
#endif
  fd = (PFmStuff) os_calloc((long int)1, (long int)sizeof(FmStuff));
  if (fd == NIL) return NIL;
  
  SetFmDeviceMetrics(fd, (PSCANTYPE) base, byteWidth, height, pixelArgs);

  fd->frameProc = frameProc;
  fd->trackingProcs = trackingProcs;
  fd->procHook = procHook;
  fd->gen.d.procs = fmProcs;
  fd->gen.d.maskID = 0;

  /* coord system origin is upper left, y grows down */
  fd->gen.bbox.x.l = -(pixelWidth/2);
  fd->gen.bbox.x.g = (pixelWidth + 1)/2;
  fd->gen.bbox.y.l = -(height/2);
  fd->gen.bbox.y.g = (height+1)/2;

  fd->gen.matrix = *pmtx;
  AdjustDevMatrix(&fd->gen.matrix, pixelWidth, height);

  return ((PDevice)fd);
  } /* end of FrameDevice */

private boolean FmShowPage (device, clear, nCopies, pageHook)
  PDevice device; boolean clear; integer nCopies; unsigned char *pageHook;
  {
  PFmStuff fmStuff = (PFmStuff) device;
  if (fmStuff->frameProc) {
    PSCANTYPE newBase;
#if (OS == os_mpw)
    newBase =
      (PSCANTYPE) DPSCall5Sanitized(
        fmStuff->frameProc, (PPixelBuffer) fmStuff->base, fmStuff->procHook,
        clear, pageHook, nCopies);
#else (OS == os_mpw)
    newBase =
      (PSCANTYPE) (*fmStuff->frameProc)(
        (PPixelBuffer) fmStuff->base, fmStuff->procHook, clear,
        pageHook, nCopies);
#endif (OS == os_mpw)
    fmStuff->base = framebase = newBase;
    }
  return false;
  }

/* FmMark is a special wrapper for the lower-level Mark routine which provides
   time-slicing of images.  It works by "choking down" the trapezoids which
   express the device area in which the image is to appear.  Time-slicing at
   this level is only appropriate for a context which is imaging into a frame-
   buffer which can't be altered by any other context, i.e. non-windows.
*/

#define LARGE 100000	/* most image procs do this many dest pixels < 1 sec */
#define WIDEST_DEVICE 10000 /* must be less than LARGE, but not too accurate */

private procedure FmMark(device, graphic, devClip, markInfo)
  PDevice device; DevPrim *graphic; DevPrim *devClip; DevMarkInfo *markInfo; {
  integer dx, dy, yl, yg, ybump, ybprev, alltrapcnt;
  DevTrap *firstTrap, wholeTrap, *splitTrap;
  boolean lslope, gslope;
  DevImage *image;
  boolean imageSplit = (graphic->type == imageType);
  DevBounds *bounds;

  if (imageSplit) {
    bounds = &graphic->bounds;
    yl = bounds->y.l;
    yg = bounds->y.g;
    dx = bounds->x.g - bounds->x.l;
    if (dx * (yg - yl) < LARGE)
      imageSplit = false; /* few enough device pixels to do in single pass */
    else {
      DebugAssert( /* life is simple & bounds == graphic->bounds */
	(graphic->items == 1) && (graphic->next == NULL));
      splitTrap = NIL;
      image = graphic->value.image;
      if (dx > WIDEST_DEVICE) /* it's likely to get clipped, but ensure dy>0 */
        dx = WIDEST_DEVICE;
      dy = LARGE / dx; /* single hunk of scanlines */
      bounds->y.g = yl + dy;
      firstTrap = image->info.trap;
      alltrapcnt = image->info.trapcnt;
    }
  }
  while (1) {
    if (imageSplit) {
      if (!splitTrap) { /* find next to split, take all of any ending early */
	for ( /* find first trap to split */
	  splitTrap = image->info.trap, image->info.trapcnt = 1;
	  splitTrap->y.g < bounds->y.g;
	  splitTrap++, image->info.trapcnt++)
	    DebugAssert(image->info.trapcnt <= alltrapcnt);
	wholeTrap = *splitTrap;  
	lslope = splitTrap->l.xl != splitTrap->l.xg;
	gslope = splitTrap->g.xl != splitTrap->g.xg;
	if (splitTrap->y.l > bounds->y.g) { /* just do earlier whole traps */
	  splitTrap = NIL;
	  image->info.trapcnt--;
	}
      }
      if (splitTrap) {	/* shorten this trap by moving its greater edge */
	splitTrap->y.g = bounds->y.g;
	ybump = splitTrap->y.g - splitTrap->y.l - 2; /* last - 2nd */
	if (ybump >= 0) { /* xg matters, adjust edge's greater limits*/
	  if (lslope) splitTrap->l.xg = FTrunc(
	    splitTrap->l.ix + ybump*splitTrap->l.dx);
	  if (gslope) splitTrap->g.xg = FTrunc(
	    splitTrap->g.ix + ybump*splitTrap->g.dx);
	}
      }
    }
    Mark(device, graphic, devClip, markInfo);		/* in mark.c */
    if (!imageSplit)
      break;
    if (splitTrap && (bounds->y.g == wholeTrap.y.g)) {
      *splitTrap = wholeTrap;		/* done w/ that trap, restore it */
      image->info.trap = splitTrap+1;	/* ready to consider next one */
      splitTrap = NIL;
    }
    if (bounds->y.g == yg) {	/* done with last hunk of scanlines, exit */
      image->info.trap = firstTrap;
      image->info.trapcnt = alltrapcnt;
      bounds->y.l = yl;
      break;
    }
    if (WannaYield())
      PSYield (yield_time_limit, (char *)NIL);
    bounds->y.l = bounds->y.g;
    bounds->y.g = bounds->y.l + dy;
    if (bounds->y.g > yg)
      bounds->y.g = yg;
    if (splitTrap) { /* just keep working on this one, fix up lesser edge */
      image->info.trapcnt = 1;
      image->info.trap = splitTrap;
      ybump = splitTrap->y.g - splitTrap->y.l;
      splitTrap->y.l = splitTrap->y.g;
      if (bounds->y.g > wholeTrap.y.g) /* just finish it this round */
        bounds->y.g = wholeTrap.y.g;
      if (lslope) { /* adjust lesser edge */
	splitTrap->l.xl = splitTrap->l.xg;
	splitTrap->l.ix = splitTrap->l.ix + ybump*splitTrap->l.dx;
        splitTrap->l.xg = FTrunc(splitTrap->l.ix - 2*splitTrap->l.dx);
      }
      if (gslope) { /* adjust greater edge */
	splitTrap->g.xl = splitTrap->g.xg;
	splitTrap->g.ix = splitTrap->g.ix + ybump*splitTrap->g.dx;
        splitTrap->g.xg = FTrunc(splitTrap->g.ix - 2*splitTrap->g.dx);
      }
    }
  }
} /* FmMark */

public procedure IniFmDevImpl() {
  fmProcs = (DevProcs *)  os_sureMalloc ((long int)sizeof (DevProcs));
  *fmProcs = *genProcs;
  fmProcs->Mark = FmMark;
  fmProcs->ShowPage = FmShowPage;
  fmProcs->Sleep = FmSleep;
  fmProcs->Wakeup = FmWakeup;
  fmProcs->GoAway = FmGoAway;
#if (!DISABLEDPS)
  fmProcs->PreBuiltChar = FmPreBuiltChar;
#endif (!DISABLEDPS)
  fmProcs->DeviceInfo = FmDeviceInfo;
  fmProcs->Proc1 = (PIntProc)FmSetupMark;

  fmMarkProcs = (PMarkProcs)  os_sureMalloc ((long int)sizeof (MarkProcsRec));

  fmMarkProcs->black.MasksMark = BlackMasksMark;
  fmMarkProcs->black.TrapsMark = BlackTrapsMark;
  fmMarkProcs->black.RunMark = BlackRunMark;
  fmMarkProcs->white.MasksMark = WhiteMasksMark;
  fmMarkProcs->white.TrapsMark = WhiteTrapsMark;
  fmMarkProcs->white.RunMark = WhiteRunMark;
  fmMarkProcs->constant.MasksMark = ConstantMasksMark;
  fmMarkProcs->constant.TrapsMark = ConstantTrapsMark;
  fmMarkProcs->constant.RunMark = ConstantRunMark;
  fmMarkProcs->gray.MasksMark = GrayMasksMark;
  fmMarkProcs->gray.TrapsMark = GrayTrapsMark;
  fmMarkProcs->gray.RunMark = GrayRunMark;
  fmMarkProcs->ClippedMasksMark = ClippedMasksMark;

  fmMarkProcs->ImageTraps = ImageTraps;
  fmMarkProcs->ImageRun = ImageRun;

  fmMarkProcs->SetupImageArgs = FmSetupImageArgs;

  fmImageProcs = (PImageProcs)os_sureCalloc(
    (long int)sizeof(ImageProcsRec), (long int)1);
  fmImageProcs->GetWriteScanline = FmGetWriteScanline;
  fmImageProcs->GetReadWriteScanline = FmGetReadWriteScanline;
  fmImageProcs->Begin = DevNoOp;
  fmImageProcs->End = DevNoOp;

  } /* end of IniFmDevImpl */






