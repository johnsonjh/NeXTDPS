/* PostScript frame mask device implementation

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

#include DEVIMAGE
#include EXCEPT
#include PSLIB
#include FOREGROUND

#include "devcommon.h"
#include "framemaskdev.h"
#include "maskcache.h"

public DevProcs *maskProcs;
public PMarkProcs maskMarkProcs;
public PatternHandle gMaskPattern;

#if (MULTICHROME == 1)
public SCANTYPE *source8bits, *source4bits;
public PCard16 source2bits;
#endif

private integer BytesForMask (width, height, maskID)
  integer width, height, maskID; {
  return ((width + SCANUNIT - 1) / SCANUNIT) * sizeof(SCANTYPE) * height;
  }

public PDevice FmMakeMaskDevice (device, args) 
  PDevice device; MakeMaskDevArgs *args; {
  integer maskID = device->maskID;
  register PMaskStuff stuff;
  register PFmStuff fmStuff;
  integer nBytes;
  PMask mask;
  char *storage;
  Assert((maskID & 0x7) == 0);

  if (args->width == 0) {
    mask = MCGetMask();
    if (mask == NIL)
      return NIL;
    os_bzero((char *)mask, (long int)sizeof(MaskRec));
    mask->maskID = maskID;
    mask->cached = true;
    *args->mask = mask;
    return NIL;
    }
  nBytes = BytesForMask(args->width, args->height, maskID);
  if (nBytes > args->cacheThreshold)
    return NIL;
  storage = MCGetTempBytes(nBytes);
  if (storage == NIL)
    return NIL;
  mask = MCGetMask();
  if (mask == NIL) {
    MCFreeBytes(storage);
    return NIL;
    }
  stuff = (PMaskStuff)os_calloc(
    (long int)sizeof(MaskStuff), (long int)1);
  if (stuff == NIL) {
    MCFreeBytes(storage);
    MCFreeMask(mask);
    return NIL;
    }
  fmStuff = &stuff->fm;
  fmStuff->base = (PSCANTYPE) storage;
  mask->data = (unsigned char *)storage;
  fmStuff->bytewidth =
    ((args->width) + SCANUNIT - 1)/SCANUNIT * sizeof(SCANTYPE);
  fmStuff->height = args->height;
#if (MULTICHROME == 1)
  fmStuff->log2BD = 0;
  fmStuff->gen.colors = 1;
#endif
  /* fmStuff->gen.bbox.x.l = 0; calloc : see BdMaskSetupMark if changed */
  fmStuff->gen.bbox.x.g = mask->width = args->width;
  /* fmStuff->gen.bbox.y.l = 0; calloc : see BdMaskSetupMark if changed */
  fmStuff->gen.bbox.y.g = mask->height = args->height;
  fmStuff->gen.matrix = *args->mtx;
  fmStuff->pattern = gMaskPattern;
  fmStuff->gen.d.procs = maskProcs;
  /* fmStuff->fd.priv = NIL; from calloc */
  /* fmStuff->fd.ref = 0; from calloc */
  fmStuff->gen.d.maskID = maskID;
  stuff->mask = mask;
  *args->mask = mask;
  stuff->offset = args->offset;
  os_bzero((char *) fmStuff->base, fmStuff->height * fmStuff->bytewidth);
  return &fmStuff->gen.d;
  } /* FmMakeMaskDevice */
  

private procedure MaskInitPage (device, c) PDevice device; DevColor c; {
  PMaskStuff stuff = (PMaskStuff)device;
  Assert(device->priv == NIL);
  os_bzero((char *) stuff->fm.base, stuff->fm.height * stuff->fm.bytewidth);
  }

private integer MaskSetupMark (device, clip, args)
  PDevice device; DevPrim **clip; MarkArgs *args; {
  PMaskStuff stuff = (PMaskStuff) device;

  args->procs = maskMarkProcs;
  args->pattern = stuff->fm.pattern;
  if (args->markInfo->halftone == NIL)
    args->markInfo->halftone = defaultHalftone;
  SetupPattern(args->pattern, args->markInfo, &args->patData);
/* redundant ... see FmMakeMaskDev
  (*device->procs->WinToDevTranslation)(device, &translation);
  args->markInfo->offset.x -= translation.x;
  args->markInfo->offset.y -= translation.y;
*/
  return 1;
  }

public boolean CmptMaskBBMin(cmb, wunits, height, ll, ur)
  SCANTYPE *cmb; integer wunits, height; DevPoint *ll, *ur;
{
  register integer x, y;
  register SCANTYPE *line; register SCANTYPE temp, *ob = deepPixOnes[0];

  line = cmb;
  for (y = 0; y < height; y++) { /* scan up from the bottom */
    for (x = wunits; --x >= 0; ) {  /* for each unit on this scanline */
      if (*(line++) != 0) {ll->y = y; goto findury;}}}

  /* If we get here, the mask has no pixels turned on */
  return false;

  /* If we get here, the mask has some pixels turned on. Note that the
     remaining loops require no end tests, since they are guaranteed to find
     pixels turned on and to terminate. */
findury:
  line = cmb + (height * wunits);
  for (y = height-1; ; y--) {	/* scan down from the top */
    for (x = wunits; --x >= 0; ) {	/* scan left from the far right */
      if (*(--line) != 0) {ur->y = y; goto findllx;}}}

  /* The following loops may test more scan lines than necessary; this
     simplifies setup and typically costs little. */
findllx:
  temp = 0;
  for (x = 0; ; x++){	/* scan right by whole units */
    line = cmb + x;
    for (y = height; --y >= 0; ) {	/* scan top to bottom, ORing units at this x coord */
      temp |= *line; line += wunits;}
    if (temp != 0){	/* got the x unit coord with the first black pixel */
      line = &ob[0];
      for (x = x * SCANUNIT; ; x++) { /* find which pixel within */
        if (temp & *line++) {ll->x = x; goto findurx;}}}}

findurx:
  temp = 0;
  for (x = wunits-1; ; x--){	/* scan left by whole units */
    line = cmb + x;
    for (y = height; --y >= 0; ) {
      temp |= *line; line += wunits;}
    if (temp != 0){
      line = &ob[SCANUNIT];
      for (x = (x * SCANUNIT) + SCANUNIT - 1; ; x--) {
	if (temp & *--line) {ur->x = x; goto done;}}}}

done:
  return true;
} /* CmptMaskBBMin */


private procedure MaskGoAway(device) PDevice device; {
  MaskStuff *stuff = (MaskStuff *) device;
  PMask mask = stuff->mask;
  PCard8 oldData = mask->data;

  DevPoint ll, ur;
  integer wunits, height, width;

  MaskRec mr;
  PMask m;
  DevMask devMask;
  DevMarkInfo info;
  MarkArgs args;

  ll.x = ll.y = ur.x = ur.y = 0;
  mask->maskID = device->maskID;
  if (mask->width == 0
      || !CmptMaskBBMin(
           (SCANTYPE *)(mask->data), MaskUnitWidth(mask),
	    mask->height, &ll, &ur))
    { /* empty bitmap */
    mask->height = mask->width = 0;
    mask->cached = true;
    mask->unused = 0;
    stuff->offset->x = stuff->offset->y = 0;
    MCFreeBytes(mask->data);
    mask->data = NIL;
    os_free((char *)stuff);
    return;
    }


  /* here to copy the mask data into the cache if there's space */

  mr = *mask; /* mr will define the bitblt source */

  /* trim off surrounding white space */
  mr.height = height = ur.y - ll.y + 1;
  mr.data += ll.y * MaskUnitWidth(mask) * sizeof(SCANTYPE);

  width = ur.x - ll.x + 1;
  wunits = ((width + SCANUNIT - 1) / SCANUNIT);
  MCGetCacheBytes(mask, height * wunits * sizeof(SCANTYPE));
  if (mask->data == NULL) {
    /* no cache space; leave it untrimmed and uncached */
    mask->data = oldData;
    mask->cached = false;
    mask->unused = 0;
    os_free((char *)stuff);
    return;
    }

  /* inform the kernel re adjustment due to trimming of the mask origin */
  stuff->offset->x += Fix(ll.x);
  stuff->offset->y += Fix(ll.y);


  /* setup and bitblt the mask data from the maskdevice into the cache */

  framebase = (PSCANTYPE) ((PCard8)mask->data);
  framebytewidth = wunits * sizeof(SCANTYPE);
#if (MULTICHROME == 1)
  framelog2BD = 0;
#endif

  devMask.dc.x = -ll.x; devMask.dc.y = 0;
  devMask.mask = &mr;
  info.offset.x = 0;
  info.offset.y = 0;
  args.markInfo = &info;
  args.device = device;
  os_bzero((char *)framebase, (long int)(height * framebytewidth));
  (*fmMarkProcs->black.MasksMark)(&devMask, 1, &args); /* want bitblt */

  MCFreeBytes(oldData); /* recycle the maskdevice storage */

  mask->height = height;
  mask->width = width;
  mask->cached = true;
  mask->unused = 0;
  os_free((char *)stuff);
  return;
  } /* MaskGoAway */

private procedure MaskDeviceInfo(device, args)
  PDevice device; DeviceInfoArgs *args; {
  args->dictSize = 3;
  (*args->AddIntEntry)("GrayValues", 2, args);
  (*args->AddIntEntry)("Colors", 1, args);
  (*args->AddIntEntry)("FirstColor", 0, args);
  }
  
private procedure MaskSetupImageArgs(device, args)
  PDevice device; ImageArgs *args; {

  args->pattern = gMaskPattern;
  args->bitsPerPixel = 1;

  (void)PatternInfo(
    args->pattern, &args->red, &args->green, &args->blue, &args->gray,
    &args->firstColor);
  args->procs = fmImageProcs;
  args->data = NIL;
  args->device = device;
}

#if (SWAPBITS == DEVICE_CONSISTENT)
#define	LEFT_BIT	0x01
#define	LEFT4BITS	0x0F
#define	RIGHT4BITS	0xF0
#define	LEFT2BITS	0x03
#define	RIGHT2BITS	0x0C
#else
#define	LEFT_BIT	0x80
#define	LEFT4BITS	0xF0
#define RIGHT4BITS	0x0F
#define	LEFT2BITS	0xC0
#define	RIGHT2BITS	0x30
#endif

public procedure IniMaskDevImpl()
{
  gMaskPattern = MaskPattern(true);

  maskProcs = (DevProcs *) os_sureMalloc (sizeof (DevProcs));
  *maskProcs = *fmProcs;
  maskProcs->PreBuiltChar = DevAlwaysFalse;
  maskProcs->InitPage = MaskInitPage;
  maskProcs->GoAway = MaskGoAway;
  maskProcs->Proc1 = (PIntProc)MaskSetupMark;
  maskProcs->DeviceInfo = MaskDeviceInfo;
  
  maskMarkProcs = (PMarkProcs) os_sureMalloc ((long int)sizeof (MarkProcsRec));
  *maskMarkProcs = *fmMarkProcs;
  maskMarkProcs->SetupImageArgs = MaskSetupImageArgs;

#if (MULTICHROME == 1)
  source8bits = (SCANTYPE *)os_sureMalloc(2048);
  {
	char *sbits = (char *)source8bits;
	unsigned char i = 0, j, imask;
	do {
	  imask = LEFT_BIT;
	  for (j = 0; j < 8; j++) {
		if (i & imask) *sbits++ = 0xFF; else *sbits++ = 0x00;
         imask RSHIFTEQ 1;
	  }
    } while (i++ < 255);
  }
  source4bits = (SCANTYPE *)os_sureMalloc(1024);
  {
    char *sbits = (char *)source4bits;
	unsigned char i = 0, j, imask;
	do {
	  imask = LEFT_BIT;
	  for (j = 0; j < 4; j++) {
		if (i & imask) *sbits = LEFT4BITS; else *sbits = 0;
		imask RSHIFTEQ 1; 
		if (i & imask) *sbits |= RIGHT4BITS;
		imask RSHIFTEQ 1; sbits++;
	  }
    } while (i++ < 255);
  }
  source2bits = (PCard16)os_sureMalloc(512);
  {
    char *sbits = (char *)source2bits;
	unsigned char i = 0, j, imask;
	do {
	  imask = LEFT_BIT;
	  for (j = 0; j < 2; j++) {
		if (i & imask) *sbits = LEFT2BITS; else *sbits = 0;
		imask RSHIFTEQ 1; 
		if (i & imask) *sbits |= RIGHT2BITS;
		imask RSHIFTEQ 1; 
		if (i & imask) *sbits |= (LEFT2BITS RSHIFT 4);
		imask RSHIFTEQ 1; 
		if (i & imask) *sbits |= (RIGHT2BITS RSHIFT 4);
		imask RSHIFTEQ 1; sbits++;
	  }
    } while (i++ < 255);
  }
#endif
} /* end of IniMaskDevImpl */
