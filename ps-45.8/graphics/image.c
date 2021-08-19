/*
				   image.c

   Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE:  All information  contained herein  is the property  of Adobe Systems
Incorporated.  Many  of  the intellectual   and technical concepts  contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for  their internal  use.  Any reproduction
or dissemination of this software  is strictly forbidden unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Edit History:
Scott Byer: Fri May 19 10:45:13 1989
Ed Taft: Thu Jul 28 16:37:44 1988
Bill Paxton: Fri Jul  8 13:45:03 1988
Ivor Durham: Fri Sep 23 15:56:33 1988
Joe Pasqua: Thu Jan 12 12:48:12 1989
Jim Sandman: Wed Dec 13 09:17:52 1989
Perry Caro: Tue Nov 15 16:25:53 1988
Paul Rovner: Tue Aug 22 10:45:50 1989
Jack Newlin 7May90 make ImageInternal public (now called from fontbuild.c)
End Edit History.
*/

#include PACKAGE_SPECS
#include COPYRIGHT
#include BASICTYPES
#include DEVICE
#include ERROR
#include EXCEPT
#include GRAPHICS
#include LANGUAGE
#include PSLIB
#include SIZES
#include STREAM
#include VM

#include "graphdata.h"
#include "gray.h"
#include "graphicspriv.h"
#include "graphicsnames.h"
#include "image.h"

#define PROC_SOURCE 0
#define STRING_SOURCE 1
#define STREAM_SOURCE 2

#ifndef DPSONLY
#define DPSONLY 0
#endif /* DPSONLY */
#ifndef DPSXA
#define DPSXA 0
#endif /* DPSXA */

/* Changed to public from private for product/windowimage.c */
public integer imageID;

typedef struct {
  boolean prematureEnd;
  integer rowsLeftToRead, nProcs, length, sourceType;
  Object pixelsProc[4];
  unsigned char *pixChar[4];
  } PixInfo;

private procedure GetStringSource(pixInfo) register PixInfo *pixInfo; {
  StrObj pixStr;
  integer i, nStrings = 0;
  Card32 minLength = MAXCard32;

  for (i = 0; i < pixInfo->nProcs; i++) {
    pixStr = pixInfo->pixelsProc[i];
    if (minLength == MAXCard32)
      minLength = pixStr.length;
    else if (pixStr.length != minLength) {
      RangeCheck();
      }
    if (pixStr.length == 0) {
      pixInfo->prematureEnd = true; minLength = 0; continue;
      }
    pixInfo->pixChar[i] = pixStr.val.strval;
    }
  pixInfo->length = minLength;
  }


private procedure GetProcSource(pixInfo) register PixInfo *pixInfo; {
  StrObj pixStr, ob;
  integer i, nStrings = 0;
  Card32 minLength = MAXCard32;

  for (i = 0; i < pixInfo->nProcs; i++) {
    if (psExecute(pixInfo->pixelsProc[i])) {
      pixInfo->prematureEnd = true; minLength = 0; continue;
      }
    PopPString(&pixStr);
    if (minLength == MAXCard32)
      minLength = pixStr.length;
    else if (pixStr.length != minLength) {
      for (i = 0; i < nStrings; i++)
        EPopP(&ob);
      RangeCheck();
      }
    if (pixStr.length == 0) {
      pixInfo->prematureEnd = true; minLength = 0; continue;
      }
    EPushP(&pixStr);
    pixInfo->pixChar[i] = pixStr.val.strval;
    IPopSimple(refStk, &ob);
    nStrings++;
    }
  if (pixInfo->prematureEnd) {
    for (i = 0; i < nStrings; i++)
      EPopP(&ob);
    }
  pixInfo->length = minLength;
  }


private procedure GetStreamSource(destbytes, bytesRemaining, pixInfo) 
  PCard8 *destbytes; integer bytesRemaining; PixInfo *pixInfo; {
  Stm stm;
  integer i, nRead;
  Card32 minLength = bytesRemaining;

  for (i = 0; i < pixInfo->nProcs; i++) {
    stm = GetStream(pixInfo->pixelsProc[i]);
    nRead = fread(destbytes[i], 1, bytesRemaining, stm);
    if (nRead != bytesRemaining) {
      pixInfo->prematureEnd = true; minLength = 0; continue;
      }
    }
  pixInfo->length = minLength;
  }


private integer ReadSlices(source, row0, nRows, pixInfo)
  DevImageSource *source; integer row0, nRows; PixInfo *pixInfo; {
  integer bytesRemaining, sliceHeight, bytesAvail, rowsToRead, i;
  integer wbytes = source->wbytes;
  unsigned char *destbyte[4];
  /* row0 is the first row of the next slice */
  /* nRows is the max number of rows for a slice */
  rowsToRead = nRows;
  if (row0) {
    rowsToRead--; /* reserve space to copy last line of previous */
    }
  if (rowsToRead > pixInfo->rowsLeftToRead)
    rowsToRead = pixInfo->rowsLeftToRead;
  if (rowsToRead == 0) return 0; /* nothing more new to read */
  pixInfo->rowsLeftToRead -= rowsToRead;
  sliceHeight = rowsToRead;
  for (i = 0; i < pixInfo->nProcs; i++) {
    destbyte[i] = source->samples[i];
    }
  bytesRemaining = sliceHeight * wbytes;
  if (row0 != 0) { /* copy last scan line of previous slice */
    sliceHeight++;
    for (i = 0; i < pixInfo->nProcs; i++) {
      os_bcopy(
        (char *)(destbyte[i] + (nRows-1) * wbytes), (char *)destbyte[i],
        wbytes);
      destbyte[i] += wbytes;
      }
    }
  if (pixInfo->sourceType == STREAM_SOURCE) {
    GetStreamSource(destbyte, bytesRemaining, pixInfo);
    return sliceHeight;
    }
  while (true) {
    if (bytesRemaining == 0) break;
    if (pixInfo->prematureEnd) {
      for (i = 0; i < pixInfo->nProcs; i++) {
        os_bzero((char *)destbyte[i], bytesRemaining);
        }
      break;
      }
    if (pixInfo->length == 0) {
      if (pixInfo->sourceType == PROC_SOURCE) GetProcSource(pixInfo);
      else GetStringSource(pixInfo);
      }
    if (pixInfo->prematureEnd) {
      for (i = 0; i < pixInfo->nProcs; i++) {
        os_bzero((char *)destbyte[i], bytesRemaining);
        }
      break;
      }
    bytesAvail = pixInfo->length;
    if (bytesAvail > bytesRemaining) bytesAvail = bytesRemaining;
    for (i = 0; i < pixInfo->nProcs; i++) {
      os_bcopy(
        (char *)pixInfo->pixChar[i], (char *)destbyte[i], bytesAvail);
      pixInfo->pixChar[i] += bytesAvail;
      destbyte[i] += bytesAvail;
      }
    bytesRemaining -= bytesAvail;
    pixInfo->length -= bytesAvail;
    if (pixInfo->length == 0 && pixInfo->sourceType == PROC_SOURCE) {
      for (i = 0; i < pixInfo->nProcs; i++)
        {
        Object tmp;
        EPopP(&tmp);
        }
      }
    }
  return sliceHeight;
  }

#define DFLT_IMAGE_BUF_SIZE 0x60000

private charptr MaxNEW(minCount, pMaxCount, size)
integer minCount, *pMaxCount, size;
{
register integer count, middle, high, low0, high0, reserve;
integer maxAlloc = ps_getsize(SIZE_IMAGE_BUFFER, DFLT_IMAGE_BUF_SIZE);
charptr p;
count = *pMaxCount;
high = count * size;
if (high > maxAlloc)
  {
  count = maxAlloc / size;
  if (count < minCount) count = minCount;
  high = count * size;
  }
while (true) {
  p = NEW(high, 1);
  if (p != NULL) break;
  if (count == minCount) LimitCheck();
  count--;
  high = count * size;
  }
*pMaxCount = count;
return p;
}

#define NIMAGETRAPS 7

private procedure ProcSampleProc(sample, result, source)
  integer sample; DevInputColor *result; DevImageSource *source; {
  Color c = (Color) source->procData;
  integer N = 1 << source->bitspersample;
  real min, max;
  min = source->decode[IMG_GRAY_SAMPLES].min;
  max = source->decode[IMG_GRAY_SAMPLES].max;
  if (c->indexed) {
    sample = min + sample * (max - min)/N;
    PushInteger(sample);
    }
  else {
    real tint = min + sample * (max - min)/N;
    PushPReal(&tint);
    }
  if (psExecute(c->cParams.procc))
    RAISE(GetAbort(), (char *)NIL);
  PopColorValues(c->colorSpace, &result);
  }


private procedure GetDevComponent(table, i, r)
  unsigned char *table; integer i; Preal r; {
  Card8 b;
  b = table[i];
  *r = b/255.0;
  }

private procedure GetLComponent(table, i, r)
  unsigned char *table; integer i; Preal r; {
  Card8 b;
  b = table[i];
  *r = b/2.55;
  }

private procedure GetABComponent(table, i, r)
  char *table; integer i; Preal r; {
  integer s;
  s = table[i];
  if (s > 127)  /* watch out for unsigned chars */
    *r = 128 - s;
  else
   * r = s;
  }

private procedure TableSampleProc(sample, result, source)
  integer sample; DevInputColor *result; DevImageSource *source; {
  Color c = (Color) source->procData;
  integer N = 1 << source->bitspersample;
  real min, max;
  unsigned char *table = c->cParams.table.val.strval;
  min = source->decode[IMG_GRAY_SAMPLES].min;
  max = source->decode[IMG_GRAY_SAMPLES].max;
  sample = min + sample * (max - min)/N;
  if (sample < 0)
    sample = 0;
  else if (sample > c->cParams.maxIndex)
    sample = c->cParams.maxIndex;
  if (c->colorSpace > 0) {
    GetDevComponent(table, sample, &result->value.tint);
    }
  else if (c->colorSpace == DEVGRAY_COLOR_SPACE) {
    GetDevComponent(table, sample, &result->value.gray);
    }
  else if (c->colorSpace == CIELIGHTNESS_COLOR_SPACE) {
    GetLComponent(table, sample, &result->value.lightness);
    }
  else if (c->colorSpace == DEVRGB_COLOR_SPACE) {
    integer i = sample * 3;
    GetDevComponent(table, i+2, &result->value.rgb.blue);
    GetDevComponent(table, i+1, &result->value.rgb.green);
    GetDevComponent(table, i, &result->value.rgb.red);
    }
  else if (c->colorSpace == DEVCMYK_COLOR_SPACE) {
    integer i = sample * 4;
    GetDevComponent(table, i+3, &result->value.cmyk.black);
    GetDevComponent(table, i+2, &result->value.cmyk.yellow);
    GetDevComponent(table, i+1, &result->value.cmyk.magenta);
    GetDevComponent(table, i, &result->value.cmyk.cyan);
    }
  else if (c->colorSpace == CIELIGHTNESS_COLOR_SPACE) {
    integer i = sample * 3;
    GetLComponent(table, i, &result->value.lab.l);
    GetABComponent(table, i+1, &result->value.lab.a);
    GetABComponent(table, i+2, &result->value.lab.b);
    }
  }


public procedure ImageInternal(
  imageCols, imageRows, bitsPerPixel, pixelsProc,
  imageMtx, isMask, invbits, nColors, colorSpace, nProcs, alpha, decode)
  integer imageRows, imageCols, bitsPerPixel;
  Object *pixelsProc;
  PMtx imageMtx;
  boolean isMask, invbits;
  integer nColors, colorSpace, nProcs;
  boolean alpha;
  DevImSampleDecode *decode;
{ /* ImageInternal */
DevImageSource source;
DevImage image;
PixInfo pixInfo;
DevTrap traps[NIMAGETRAPS];
DevPrim dp;
Mtx imageTfm, imageInverseTfm;
integer nRows, row0, wbits, wbytes, i;
unsigned char *pixels, *sampleStorage, *samples[5];
boolean singleSlice;
Color c;

if (imageCols < 0 || imageRows < 0) RangeCheck();
if (imageCols == 0 || imageRows == 0 || gs->isCharPath) return;
dp.type = trapType;
dp.next = NULL;
dp.maxItems = NIMAGETRAPS;
dp.value.trap = traps;
MtxInvert(imageMtx, &imageTfm);
MtxCnct(&imageTfm, &gs->matrix, &imageTfm);
MtxInvert(&imageTfm, &imageInverseTfm);
image.info.mtx = &imageInverseTfm;
wbits = imageCols;
/* if (nProcs == 1) wbits *= nColors; (jkn) account for interleaved alpha */
if (nProcs == 1) wbits *= (alpha ? nColors + 1 : nColors);
switch (bitsPerPixel) {
  case 1: break;
  case 2: wbits <<= 1; break;
  case 4: wbits <<= 2; break;
  case 8: wbits <<= 3; break;
  default: RangeCheck();
  }
wbytes = (wbits + 7) >> 3;
if (wbytes > 32000) LimitCheck();
pixInfo.nProcs = nProcs;
pixInfo.prematureEnd = false;
pixInfo.rowsLeftToRead = imageRows;
pixInfo.length = 0;
for (i = 0; i < nProcs; i++) {
  pixInfo.pixelsProc[i] = pixelsProc[i];
  pixInfo.pixChar[i] = NULL;
  }
image.imagemask = isMask;
image.invert = invbits;
image.unused = alpha;
image.source = &source;
if (gs->tfrFcn != NULL) {
  if (!gs->tfrFcn->active) ActivateTfr(gs->tfrFcn);
  image.transfer = gs->tfrFcn->devTfr;
  }
else image.transfer = NULL;
source.sourceID = (++imageID == 0 ? ++imageID : imageID);
source.decode = decode;
if (decode != NULL && (c->emulatedSeparation || c->indexed)) {
  if (c->indexed && c->cParams.table.type == strObj)
    source.sampleProc = TableSampleProc;
  else
    source.sampleProc = ProcSampleProc;
  source.procData = (char *)c;
  }
else {
  source.sampleProc = (PVoidProc)NULL;
  source.procData = NULL;
  }
source.interleaved = (nProcs == 1 && (alpha || nColors != 1));
source.bitspersample = bitsPerPixel;
source.wbytes = wbytes;
source.colorSpace = colorSpace;
source.nComponents = nColors;
source.samples = samples;
nRows = imageRows;
row0 = 0;
switch (pixelsProc[0].type) {
  case strObj: {
    GetStringSource(&pixInfo);
    pixInfo.sourceType = STRING_SOURCE;
    break;
    }
  case arrayObj:
  case pkdaryObj: {
    GetProcSource(&pixInfo);
    pixInfo.sourceType = PROC_SOURCE;
    break;
    }
  case stmObj: {
    pixInfo.sourceType = STREAM_SOURCE;
    break;
    }
  default: TypeCheck();
  }
/* if (alpha || (pixInfo.length < imageRows * wbytes)) { (jkn) why alpha? */
if (pixInfo.length < imageRows * wbytes) {
  sampleStorage = pixels = MaxNEW((integer)3, &nRows, wbytes*nProcs);
  switch (nProcs) {
    case 5: /* (jkn) may have 5 samples with interleaved alpha */
      samples[4] = sampleStorage;
      sampleStorage += wbytes * nRows;
    case 4:
      samples[3] = sampleStorage;
      sampleStorage += wbytes * nRows;
    case 3:
      samples[2] = sampleStorage;
      sampleStorage += wbytes * nRows;
    case 2:
      samples[1] = sampleStorage;
      sampleStorage += wbytes * nRows;
    case 1:
      samples[0] = sampleStorage;
      break;
    }
  singleSlice = false;
  source.height = ReadSlices(&source, row0, nRows, &pixInfo);
  }
else {
  pixels = NULL;
  for (i = 0; i < nProcs; i++)
    samples[i] = pixInfo.pixChar[i];
  pixInfo.rowsLeftToRead = 0;
  singleSlice = true;
  source.height = imageRows;
  }
image.info.sourcebounds.x.l = 0;
image.info.sourcebounds.x.g = imageCols;
image.info.sourceorigin.x = 0;
DURING
while (true) {
  Cd cd;
  real w, h;
  QuadPath qp;
  BBoxRec bbox;
  image.info.sourcebounds.y.l = row0;
  image.info.sourcebounds.y.g = source.height + row0;
  image.info.sourceorigin.y = row0;
  row0 += source.height - 1;
  /* -1 because slices overlap by one scanline */
  /* slices must overlap to compensate for floating point fuzz */
  /* inverse tfm is not exact; can cause bit drop out */
  cd.x = image.info.sourceorigin.x;
  cd.y = image.info.sourceorigin.y;
  w = (image.info.sourcebounds.x.g - image.info.sourcebounds.x.l);
  h = (image.info.sourcebounds.y.g - image.info.sourcebounds.y.l);
#if DPSXA
  {
short i,j;
Cd xydelta, delta;
Mtx saveMtx, tlatMtx, saveImageMtx;

	/* 
		Images are handled slightly different. Since it isn't practical to translate
		an image, like a path, the image matrix will include an extra translation
		for the chunk translation.
	*/
	xaOffset.x = 0;
	xaOffset.y = 0;
	xydelta.x = -xChunkOffset;
	xydelta.y = -yChunkOffset;
	delta.x = xydelta.x;
	delta.y = 0;
	saveMtx = gs->matrix;
	saveImageMtx = imageTfm;
	tlatMtx.a = 1.0;
	tlatMtx.b = 0.0;
	tlatMtx.c = 0.0;
	tlatMtx.d = 1.0;
	tlatMtx.tx = (float) -xChunkOffset;
	tlatMtx.ty = 0.0;
	for(j=0; j<maxYChunk; j++) {
  		dp.items = 0;
  		ReduceQuadPath(c, w, h, &imageTfm, &dp, &qp, &bbox, (Path *)NULL);
  		if (dp.items > 0) {
			image.info.trapcnt = dp.items;
			image.info.trap = dp.value.trap;
			DoImageMark(&image, &dp.bounds);
    		}
		for(i=0; i<maxXChunk; i++) {
			xaOffset.x += xChunkOffset;
			TlatBBox(&gs->clip.bbox,delta);
			MtxCnct(&gs->matrix,&tlatMtx,&gs->matrix);
			MtxInvert(imageMtx, &imageTfm);
			MtxCnct(&imageTfm, &gs->matrix, &imageTfm);
			MtxInvert(&imageTfm, &imageInverseTfm);
			image.info.mtx = &imageInverseTfm;
  			dp.items = 0;
  			ReduceQuadPath(c, w, h, &imageTfm, &dp, &qp, &bbox, (Path *)NULL);
  			if (dp.items > 0) {
				image.info.trapcnt = dp.items;
				image.info.trap = dp.value.trap;
				DoImageMark(&image, &dp.bounds);
			}
		}
		xaOffset.x = 0;
		xaOffset.y += yChunkOffset;
		delta.x = maxXChunk * -xydelta.x;
		delta.y = xydelta.y;
		TlatBBox(&gs->clip.bbox,delta);
		delta.x = xydelta.x;
		delta.y = 0;
		tlatMtx.tx = (float) (maxXChunk * xChunkOffset);
		tlatMtx.ty = (float)-yChunkOffset;
		MtxCnct(&gs->matrix,&tlatMtx,&gs->matrix);
		MtxInvert(imageMtx, &imageTfm);
		MtxCnct(&imageTfm, &gs->matrix, &imageTfm);
		MtxInvert(&imageTfm, &imageInverseTfm);
		image.info.mtx = &imageInverseTfm;
		tlatMtx.tx = (float)-xChunkOffset;
		tlatMtx.ty = 0;
	}
	/* restore clip */
	delta.x = 0;
	delta.y = maxYChunk * -xydelta.y;
	TlatBBox(&gs->clip.bbox,delta);	
	/* reset offset */
	xaOffset.x = 0;
	xaOffset.y = 0;	
	/* restore matricies */
	gs->matrix = saveMtx;
	imageTfm = saveImageMtx;
	MtxInvert(&imageTfm, &imageInverseTfm);
	image.info.mtx = &imageInverseTfm;
  }
#else /* DPSXA */
  dp.items = 0;
  ReduceQuadPath(cd, w, h, &imageTfm, &dp, &qp, &bbox, (Path *)NULL);
  if (dp.items > 0) {
    image.info.trapcnt = dp.items;
    image.info.trap = dp.value.trap;
    DoImageMark(&image, &dp.bounds);
    }
#endif /* DPSXA */
  if (singleSlice || (pixInfo.rowsLeftToRead == 0))
    break;
  source.height = ReadSlices(&source, row0, nRows, &pixInfo);
  }
HANDLER {
  if (pixels != NULL)
    FREE(pixels);
  if (pixInfo.sourceType == PROC_SOURCE && 
      pixInfo.length != 0 && !pixInfo.prematureEnd) 
    for (i = 0; i < pixInfo.nProcs; i++) {
      Object tmp;
      EPopP(&tmp);
      }
  RERAISE;
  }
END_HANDLER

if (pixels != NULL)
  FREE(pixels);
if (pixInfo.sourceType == PROC_SOURCE && 
    pixInfo.length != 0 && !pixInfo.prematureEnd) 
  for (i = 0; i < pixInfo.nProcs; i++) {
    Object tmp;
    EPopP(&tmp);
    }
}

public procedure DoImage(nColors, colorSpace, nProcs, mask, alpha)
  integer nColors, colorSpace, nProcs; boolean mask, alpha; {
  Object procs[5];
  integer w, h, d, i, sourceType;
  boolean invert;
  Mtx imageMtx;
  for (i = nProcs - 1;  i >= 0;  i--) PopP(&(procs[i]));
  sourceType = procs[0].type;
  for (i = 1; i < nProcs; i++)
    if (procs[i].type != sourceType) TypeCheck();
  PopMtx(&imageMtx);
  if (mask) {
    invert = PopBoolean();
    d = 1;
    }
  else {
    invert = false;
    d = PopInteger();
    if (gs->noColorAllowed) Undefined();
    }
  h = PopInteger();
  w = PopInteger();
  ImageInternal(
    w, h, d, procs, &imageMtx, mask, invert, nColors, colorSpace, nProcs,
    alpha, NIL);
  }
  
#if (DPSONLY == 0)
private integer GetInt(d, nm) DictObj d; integer nm; {
  Object ob;
  if (DictTestP(d, graphicsNames[nm], &ob, true) && ob.type == intObj)
      return ob.val.ival;
  TypeCheck();
  return 0;
  }

private procedure ImageDict() {
  DictObj d;
  Object ob;
  Object source;
  integer width, height, bps, nProcs, nColors, sourceType, i;
  boolean multipleSources;
  Mtx imageMtx;
  DevImSampleDecode decode[4];
  Color c = gs->color;
  PopP(&d);
  if (GetInt(d, nm_ImageType) != 1) 
    RangeCheck();
  width = GetInt(d, nm_Width);
  height = GetInt(d, nm_Height);
  bps = GetInt(d, nm_BitsPerSample);
  if (DictTestP(d, graphicsNames[nm_ImageMatrix], &ob, true))
    PAryToMtx(&ob, &imageMtx);
  else
    TypeCheck();
  if (DictTestP(d, graphicsNames[nm_MultipleDataSources], &ob, true))
    switch (ob.type) {
      case boolObj: multipleSources = ob.val.bval; break;
      default: TypeCheck(); return;
      }
  else multipleSources = false;
  if (!DictTestP(d, graphicsNames[nm_DataSource], &source, true))
    TypeCheck();
  if (c->emulatedSeparation || c->indexed)
    nColors = 1;
  else 
    switch (c->colorSpace) {
      case DEVRGB_COLOR_SPACE: nColors = 3; break;
      case CIELAB_COLOR_SPACE: nColors = 3; break;
      case DEVCMYK_COLOR_SPACE: nColors = 4; break;
      default: nColors = 1; break;
      }
  if (multipleSources) {
    if (source.type != arrayObj)
      TypeCheck();
    if (source.length != nProcs) 
      RangeCheck();
    nProcs = nColors;
    sourceType = source.val.arrayval[0].type;
    for (i = 1; i < nProcs; i++)
      if (source.val.arrayval[i].type != sourceType) TypeCheck();
    }
  else
    nProcs = 1;
  if (!DictTestP(d, graphicsNames[nm_Decode], &ob, true) ||
      ob.type != arrayObj)
    TypeCheck();
  if (ob.length != nColors * 2) 
    RangeCheck();
  for (i = 0; i < nColors; i++) {
    Object n;
    VMCarCdr(&ob, &n); PRealValue(n, &decode[i].min);
    VMCarCdr(&ob, &n); PRealValue(n, &decode[i].max);
    }
  ImageInternal(
    width, height, bps, (multipleSources ? source.val.arrayval : &source),
    &imageMtx, false, false, nColors, c->colorSpace, nProcs, false, decode);
  }
#endif /* DPSONLY */


public procedure PSImage() {
  Object ob;
#if (DPSONLY == 0)
  TopP(&ob);
  if (ob.type == dictObj) 
    ImageDict();
  else
#endif /* DPSONLY */
    DoImage(1L, (integer)DEVGRAY_COLOR_SPACE, 1L, false, false);
  }

public procedure PSImageMask() {
  DoImage(1L, (integer)DEVGRAY_COLOR_SPACE, 1L, true, false);
  }

public procedure PSColorImage() {
  integer nProcs, colorSpace;
  integer nColors = PopInteger();
  switch (nColors) {
    case 1: colorSpace = DEVGRAY_COLOR_SPACE; break;
    case 3: colorSpace = DEVRGB_COLOR_SPACE; break;
    case 4: colorSpace = DEVCMYK_COLOR_SPACE; break;
    default: RangeCheck(); break;
    }
  nProcs = (PopBoolean()) ? nColors : 1;
  DoImage(nColors, colorSpace, nProcs, false, false);
  } /* end of PSColorImage */


