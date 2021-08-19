/*
				patternimpl.c

Copyright (c) 1983, '84, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE: All  information  contained herein is the  property of  Adobe Systems
Incorporated.  Many of  the  intellectual  and  technical concepts  contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees  for their internal use.  Any  reproduction
or dissemination of this software is strictly forbidden  unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version:
Edit History:
Scott Byer: Wed May 17 17:14:37 1989
Jim Sandman: Fri May 12 11:07:40 1989
Paul Rovner: Fri Dec 29 11:41:31 1989
Joe Pasqua: Tue Feb 28 13:00:11 1989
Steve Schiller: Mon Dec 11 14:43:45 1989
Ed Taft: Sun Jan  7 16:42:14 1990
End Edit History.
*/

#include PACKAGE_SPECS
#include DEVICETYPES
#include DEVPATTERN
#include EXCEPT
#include PSLIB
#include FILESYSTEM
#include STREAM
#include FOREGROUND

#include "patternpriv.h"

#ifndef DISKSL
#define DISKSL 0
#endif

public integer maxPatternSize, maxTotalPatternSize;
public PSCANTYPE grayPatternBase;
private integer totalAlloc;
private integer lastFree;
public integer patTimeStamp, patID;
public PPatCacheInfo *patterns;
private Pool screenPool, halftonePool;

#if DISKSL
/* Gloabl vars for managing threshold arrays (TA's) in RAM/DISK: */
private readonly char *SLName = "DB/ScreenList";
private Stm slstm;		/* Screen list stream. */
private integer stmBytesUsed;	/* Bytes used in ScreenList file. */
private boolean haveFiles;	/* True if we could create the file. */
private integer maxTASize;	/* Largest alwoable single TA. */
private integer curTAUsage;	/* Current mem alloc'ed for all TA's. */
private integer maxTotalTASize;	/* Limit on mem to be alloc'ed for all AT's */ 
private Card32 taTimeStamp;
DevScreen **screens;		/* List of all screens having TA's in mem. */
private integer largestChIndex;	/* Index of screen holding largest TA chunk. */
private unsigned char *largestChunk; /* pointer to largest chunk. */
private integer largestChSize;	/* Size of that largest chunk. */
#endif  /* DISKSL */
#define NULLSLOT -32678		/* Null index into screens array. */
#define MAXSP 2
private ScreenPrivate srceenPrivates[MAXSP];
integer spCount;
#define DEFAULTSCREEN -1

private procedure FreePattern(index) integer index; {
  PPatCacheInfo info;
  integer i, nUnits;
  PCard8 indexArray;
  PCachedHalftone pcv; /* pattern cache vector */

  info = patterns[index];
  patterns[index] = NIL;
  pcv = ((PScreenPrivate)(info->screen->priv))->ch;
  indexArray = pcv[info->dgColor];
  for (i = info->minGray; i < info->maxGray; i++) indexArray[i] = 0;
  if (!info->data.constant) {
    nUnits = info->data.end - info->data.start;
    os_free((char *)info->data.start);
    totalAlloc -= nUnits * sizeof(SCANTYPE);
    }
  os_free((char *)info); totalAlloc -= sizeof(PatCacheInfo);
  }

private integer FreeOldestPattern() {
  integer i, oldest, oldestAge;
  PPatCacheInfo info;
  oldestAge = patTimeStamp+1;
  for (i = 1; i < MAXPAT; i++) {
    info = patterns[i];
    if (info != NIL && info->lastUsed < oldestAge) {
      oldest = i;
      oldestAge = info->lastUsed;
      }
    }
  Assert(oldestAge != patTimeStamp+1);
  FreePattern(oldest);
  return oldest;
  }

private PCard8 PatAlloc(nBytes) integer nBytes; {
  PCard8 p;
  while (nBytes + totalAlloc > maxTotalPatternSize)
    FreeOldestPattern();
  while (true) {
    p = (PCard8)os_calloc(nBytes, 1);
    if (p == NIL) {
      FreeOldestPattern();
      continue;
      }
    break;
    }
  totalAlloc += nBytes;
  return p;
  }

public PCard8 AllocInfoVector() {
  PCard8 p;
  p = (PCard8)PatAlloc((integer)((MAXCOLOR+1) * sizeof(Card8)));
  return p;
  }

private procedure FreeInfoVector(p) PCard8 p; {
  if (p == NULL) return;
  os_free((char *)p);
  totalAlloc -= ((MAXCOLOR+1) * sizeof(Card8));
  }

/* The only time the ScreenPrivate record is allocated separately
   from the DevScreen is for the two default screens. In this
   case we allocate them from global storage squirreled away
   in this module. */
private PScreenPrivate AllocScreenPrivate()
  {
  PScreenPrivate psp;
  Assert(spCount < MAXSP); /* If this fails just make MAXSP one bigger. */
  psp = srceenPrivates + spCount++;
  os_bzero((char *) psp, (long int)sizeof(ScreenPrivate));
  psp->chunkSize = DEFAULTSCREEN;
  return psp;
  }

public PSCANTYPE AllocPatternStorage(nUnits) integer nUnits;{
  return (PSCANTYPE)PatAlloc((integer)(nUnits * sizeof(SCANTYPE)));
  }

public integer AllocPatternIndex() {
  integer curFree, alloc;
  PPatCacheInfo info;
  PSCANTYPE screen;
  curFree = lastFree;
  while (true) {
    if (++curFree == MAXPAT) curFree = 1;
    if (patterns[curFree] == NIL) break;
    if (curFree == lastFree) { curFree = FreeOldestPattern(); break; }
    }
  lastFree = curFree;
  patterns[curFree] = (PPatCacheInfo)PatAlloc((integer)sizeof(PatCacheInfo));
  return curFree;
  }

public boolean CheckScreenDims (width, height) DevShort width, height; {
  if ((width * height * sizeof(SCANTYPE)) <= maxPatternSize) return(true);
  else return(false);
  }


public boolean IndependentColors () {return(false);}

#if DISKSL

/* Puts the current threshold array at 'slot' onto the disk.
   Returns the a pointer to the memory chunk used for the 
   threshold array (through mem) and the size of that chunk
   (through size). Returns 'false' if it failed. */
private boolean SendTAToDisk(slot, mem, size)
integer slot;
unsigned char **mem;
integer *size;
  {
  register DevScreen *s;
  PScreenPrivate psp;
  integer nbytes, bytesWriten, filePos;
  
  if (!haveFiles) return false;
  s = screens[slot];
  psp = (PScreenPrivate) (s->priv);
  Assert(psp->chunkSize > 0);
  if (fseek(slstm, 0, 2) == EOF) return false;	/* Seek to end of file. */
  if ((filePos=ftell(slstm)) == EOF) return false;
  nbytes = s->width*s->height;
  bytesWriten = fwrite(s->thresholds, 1, nbytes, slstm);
  if (nbytes != bytesWriten || ferror(slstm)) {os_clearerr(slstm); return false;}
  *size = psp->chunkSize;
  psp->chunkSize = 0;
  *mem = s->thresholds;
  s->thresholds = (unsigned char *) filePos;
  screens[slot] = NULL;
  stmBytesUsed += nbytes;
  return true;
  }

/* Sends the oldest threshold array to the disk, and frees the
   memory and slot associated with it. Care is taken not to free
   the threshold array corresponding to 'largestChIndex'.
   Returns false if it failed to free anything. */
private boolean SendOldestToDisk()
  {
  Card32 oldset = 0;
  integer slot = NULLSLOT;
  integer i, size;
  PCard8 mem;
  
  if (!haveFiles) return false;
  for (i=0; i<MAXSCRN; i++) {
    if (i == largestChIndex) continue;
    if (screens[i] != NULL) {
      PScreenPrivate psp = (PScreenPrivate) (screens[i]->priv);
      if (taTimeStamp-psp->age > oldset) {
        oldset = taTimeStamp-psp->age;
        slot = i;
        }
      }
    }
  if (slot == NULLSLOT) return false;
  if (!SendTAToDisk(slot, &mem, &size)) return false;
  os_free(mem);
  curTAUsage -= size;
  return true;
  }

private integer AllocScrnSlot()
  {
  integer slot = NULLSLOT;
  integer i;
  for (i=0; i<MAXSCRN; i++)
    if (screens[i] == NULL) return i;
  if (!SendOldestToDisk()) return NULLSLOT; /* Free up slot for oldest TA, */
  return AllocScrnSlot();			/* And try again... */
  }

private procedure DecStmByteCnt(fileUsage)
integer fileUsage;
  {
  stmBytesUsed -= fileUsage;
  if (stmBytesUsed <= 0) {
    /* If file usage has dropped to 0, truncate file. */
    (void) fseek(slstm, 1L, 0);	/* Using 1 instead of 0, because of fseek problem. */
    (void) FileTruncate(slstm);
    if (ferror(slstm)) os_clearerr(slstm);
    }
  }
#endif  /* DISKSL */

private procedure FreeScreen(s) DevScreen *s; {
  PScreenPrivate psp;
  if (s == NULL) return;
#if DISKSL
  psp = (PScreenPrivate) (s->priv);
  Assert(psp != NULL);
  Assert(psp->chunkSize != DEFAULTSCREEN);
  if (psp->chunkSize > 0) {	/* True for screen TA in memory. */
    if (s->thresholds == largestChunk) {
      /* Don't free largest chunk, put it aside: */
      screens[largestChIndex] = NULL;
      largestChIndex = NULLSLOT;
      }
    else {
      DevScreen **sh;
      if (s->thresholds != NULL) {
        os_free((char *)s->thresholds);
        curTAUsage -= psp->chunkSize;}
      for (sh = screens; sh<screens+MAXSCRN; sh++)
        if (*sh == s) *sh = NULL;
      }
    }
  else {	/* Screen TA is on disk. */
    DecStmByteCnt(s->width*s->height);}
  /* If everything but the largest chunk is freed, then free largest chunk also: */
  if (stmBytesUsed == 0 && curTAUsage == largestChSize && largestChIndex == NULLSLOT) {
    Assert(largestChunk != NULL);
    os_free((char *)largestChunk);
    largestChunk = NULL;
    largestChSize = 0;
    curTAUsage = 0;}
#endif  /* DISKSL */
  os_freeelement(screenPool, (char *)s);
  }

private procedure FreeHalftone (h) DevHalftone *h; {
  if (h->white != NULL && h->white == h->red) 
    h->red = h->green = h->blue = NULL;
  FreeScreen(h->white);
  FreeScreen(h->red);
  FreeScreen(h->green);
  FreeScreen(h->blue);
  os_freeelement(halftonePool, h);
  }

#if DISKSL
/* Returns false in case of failure. */
private boolean GetMemForTA(s)
DevScreen *s;
  {
  integer taSize, scrnSlot;
  PScreenPrivate psp;
  taSize = (long int)s->width*s->height;
  if (taSize > maxTASize) return false;
  Assert(taSize > 0);
  psp = (PScreenPrivate) s->priv;
  if (taSize+curTAUsage > maxTotalTASize) {
    /* The addition of the new screen will exceed our memory limit => free stuff up */
    if (taSize+largestChSize <= maxTotalTASize) {
      /* We can get back enough memory by sending screens to disk and freeing
         their threshold arrays: */
      do {if (!SendOldestToDisk()) return false;}
      while(taSize+curTAUsage > maxTotalTASize);}
    else {
      /* We can't get back enough memory by freeing threshold arrays because
         we have to hold onto the largest threshold array for any screen 
         produced so far.  But we can send the largest threshold array to disk
         and re use its memory if the new threshold array is smaller it: */
      if (taSize <= largestChSize) {
        if (largestChIndex == NULLSLOT) {
          if ((scrnSlot = AllocScrnSlot()) == NULLSLOT) return false;
          s->thresholds = largestChunk;
          largestChIndex = scrnSlot;}
        else {
          if (!SendTAToDisk(largestChIndex, &s->thresholds, &psp->chunkSize))
            return false;}
        screens[largestChIndex] = s;
        psp->chunkSize = largestChSize;
        return true;}
      /* It is better to give up on a new request for a threshold array,
         than to jeprodize an existing one: */
      else return false;}
    }
  if ((scrnSlot = AllocScrnSlot()) == NULLSLOT) return false;
  if (taSize <= largestChSize && largestChIndex == NULLSLOT && largestChunk != NULL) {
    /* Reuse largest chunk instead of allocating more memory: */
    s->thresholds = largestChunk;
    largestChIndex = scrnSlot;
    psp->chunkSize = largestChSize;}
  else {
    s->thresholds = (unsigned char *)os_calloc(taSize, 1);
    if (s->thresholds == NULL) return false;
    curTAUsage += taSize;
    psp->chunkSize = taSize;}
  screens[scrnSlot] = s;
  if (taSize > largestChSize) {
    /* Update largest chunk: */
    if (largestChIndex == NULLSLOT && largestChunk != NULL) {
      /* If old largest chunk was floating, then free it: */
      os_free((char *)largestChunk);
      curTAUsage -= largestChSize;}
    largestChIndex = scrnSlot;
    largestChSize = taSize;
    largestChunk = s->thresholds;}
  return true;
  }
#endif  /* DISKSL */

private DevScreen *AllocScreen(w, h) integer w, h; {
  integer i;
  register DevScreen *s;
  PScreenPrivate psp;
  s = (DevScreen *) os_newelement(screenPool);
  if (s== NULL) return NULL; 
  s->width = w;
  s->height = h;
  s->priv = (DevPrivate *) (((PCard8) s) + sizeof(DevScreen));
  psp = (PScreenPrivate) s->priv;
  for (i=0; i<dgNColors; i++) psp->ch[i] = NULL;
#if DISKSL
  psp->age = taTimeStamp++;
  if (!GetMemForTA(s)) {os_freeelement(screenPool, (char *)s); return NULL;} 
#else /* DISKSL */
  s->thresholds = (unsigned char *)os_malloc((long int)w * h);
  if (s->thresholds == NULL) {
    FreeScreen(s); return NULL; }
#endif  /* DISKSL */
  return s;
  }

/* makes sure that the screen's TA is in memory. Returns false
   if it failed. */
public boolean ValidateTA(s)
DevScreen *s;
  {
  PScreenPrivate psp;
  integer filePos, taSize, bytes;
  psp = (PScreenPrivate) s->priv;
  if (psp == NULL) s->priv = (DevPrivate *) (psp = AllocScreenPrivate());
#if DISKSL
  if (psp->chunkSize == DEFAULTSCREEN) return true; 
  psp->age = taTimeStamp++;
  if (psp->chunkSize > 0) return true;
  if (!haveFiles) return false;
  filePos = (integer) s->thresholds;
  if (!GetMemForTA(s)) return false;
  fseek(slstm, filePos, 0);
  taSize = s->width*s->height;
  bytes = fread(s->thresholds, 1, taSize, slstm);
  if (bytes != taSize || ferror(slstm)) {
    os_clearerr(slstm);
    return false;
    }
  DecStmByteCnt(taSize);
#endif  /* DISKSL */
  return true;
  }

public DevHalftone * AllocHalftone(
  wWhite, hWhite, wRed, hRed, wGreen, hGreen, wBlue, hBlue)
  DevShort wWhite, hWhite, wRed, hRed, wGreen, hGreen, wBlue, hBlue; {
  DevHalftone *h = (DevHalftone *) os_newelement(halftonePool);
  DevScreen *s;
  if (h == NULL) return NULL;
  h->priv = NULL;
  h->red = h->green = h->blue = h->white = NULL;
  s = AllocScreen(wWhite, hWhite);
  if (s== NULL) {
    FreeHalftone(h); return NULL; }
  h->white = s;
  if (wRed == 0) {
    h->red = h->green = h->blue = s;
    return h;
    }
  s = AllocScreen(wRed, hRed);
  if (s== NULL) {
    FreeHalftone(h); return NULL; }
  h->red = s;
  s = AllocScreen(wGreen, hGreen);
  if (s== NULL) {
    FreeHalftone(h); return NULL; }
  h->green = s;
  s = AllocScreen(wBlue, hBlue);
  if (s== NULL) {
    FreeHalftone(h); return NULL; }
  h->blue = s;
  return h;
  }

private procedure FlushPatCache(screen) DevScreen *screen; {
  integer i;
  PCachedHalftone pcv; /* pattern cache vector */
  if (screen == NIL) return;
  if (screen->priv == NIL) return;
  pcv = ((PScreenPrivate)(screen->priv))->ch;
  for (i = 1; i < MAXPAT; i++) {
    PPatCacheInfo info = patterns[i];
    if (info != NIL && info->screen == screen) 
      FreePattern(i);
    }
  for (i = 0; i < dgNColors; i++) FreeInfoVector(pcv[i]);
}

public procedure FlushHalftone(halftone) DevHalftone *halftone; {
  if (halftone == NIL) return;
  if (halftone->red != halftone->white) {
    FlushPatCache(halftone->red);
    FlushPatCache(halftone->green);
    FlushPatCache(halftone->blue);
    }
  FlushPatCache(halftone->white);
  FreeHalftone(halftone);
  }



public procedure RollPattern (leftRoll, data)
  register integer leftRoll; PatternData *data; {
  register PSCANTYPE unit, sGry, eGry;
  register integer rightRoll; /* # bits */
  register SCANTYPE temp1, temp2;
  SCANTYPE temp3; integer gnd;
  Assert(leftRoll > 0);
  gnd = data->width;
  while (leftRoll > SCANUNIT) { /* first roll whole scanunits */
    sGry = data->start;
    while (sGry < data->end) { /* foreach row */
      eGry = sGry + gnd; /* first address beyond this row */
      unit = sGry + 1; /* adr of 2nd SCANUNIT-sized guy on this row */
      temp1 = *sGry;
      while (unit < eGry) *(sGry++) = *(unit++);
      *(sGry++) = temp1;
      }
    leftRoll -= SCANUNIT;
    }
  if (leftRoll) { /* finally roll partial scanunits */
    rightRoll = SCANUNIT - leftRoll;
    sGry = data->start;
    while (sGry < data->end) { /* foreach row */
      eGry = sGry + gnd; /* first address beyond this row */
      unit = sGry + 1; /* adr of 2nd SCANUNIT-sized guy on this row */
      temp3 = temp1 = *sGry;
      while (unit < eGry) {
        temp2 = *(unit++);
        *(sGry++) = (temp1 LSHIFT leftRoll) | (temp2 RSHIFT rightRoll);
        temp1 = temp2;
        }
      *(sGry++) = (temp1 LSHIFT leftRoll) | (temp3 RSHIFT rightRoll);
      }
    }
  data->id == ++patID;
  }; /* end RollPattern */


public PSCANTYPE GetPatternRow (markinfo, data, y)
  DevMarkInfo *markinfo; PatternData *data; integer y; {
  integer k, m;
  integer origY = markinfo->offset.y + markinfo->screenphase.y;
  if ((k = y - origY) == 0) return data->start;
  else {
    m = (data->end - data->start)/data->width;
    k = k - (k/m)*m; /* Broken-out implementation of % */
    if (k < 0) k += m;
    return data->start + k * data->width;
    }
  }; /* end GetPatternRow */

public procedure DestroyPat (h) PatternHandle h; {
  integer i;
  FGEnterMonitor();
  for (i = 1; i < MAXPAT; i++) {
    PPatCacheInfo info = patterns[i];
    if (info != NIL && info->h == h) 
      FreePattern(i);
    }
  FGExitMonitor();
  os_free((char *)h);
  }; /* end DestroyPat */


public boolean ConstantColor(nGrays, screen, gray, minGray, maxGray) 
  integer nGrays; DevScreen *screen; integer *gray, *minGray, *maxGray; {
  integer gx, gLev, nGraysM1;
  register integer minThreshold, maxThreshold, val;
  register PCard8 threshold, end;
  boolean constant;
  nGraysM1 = nGrays-1;
  gx = (*gray * nGraysM1) / 255;
  gLev = (*gray * nGraysM1) % 255;
  maxThreshold = 0;
  minThreshold = MAXinteger;
  threshold = screen->thresholds;
  end = threshold + screen->width * screen->height;
  while (true) {
    val = *threshold;
    if (val < minThreshold) minThreshold = val;
    if (val > maxThreshold) maxThreshold = val;
    threshold++;
    if (threshold >= end) break;
    }
  if (gLev >= maxThreshold)
    gx++; /* constant but next creamy gray value */
  else if (gLev > minThreshold) return false;
  *gray = (gx * 255 + nGraysM1 - 1) / nGraysM1;
  constant = true;
  *minGray = ((gx-1) * 255 + maxThreshold + nGraysM1 - 1) / nGraysM1;
  if (*minGray < 0) *minGray = 0;
  *maxGray = (gx * 255 + minThreshold + nGraysM1 - 1) / nGraysM1;
  if (*maxGray > 256) *maxGray = 256;
  return true;
  }
  
public PPatCacheInfo SetPatInfo(screen, data, color, minGray, maxGray, h)
  DevScreen *screen; PatternData *data; integer color;
  integer minGray, maxGray; PatternHandle h; {

  Card8 index;
  PCachedHalftone pcv = ((PScreenPrivate) screen->priv)->ch;
  PCard8 gryArray = pcv[color];
  integer i;
  register PPatCacheInfo info;
  index = AllocPatternIndex();
  info = patterns[index];
  info->screen = screen;
  info->minGray = minGray;
  info->maxGray = maxGray;
  info->dgColor = color;
  info->data = *data;
  if (data->constant) {
    info->data.start = &info->data.value;
    info->data.end = info->data.start + 1;
    }
  info->data.id = data->id = ++patID;
  info->lastX = 0;
  info->lastUsed = 0;
  info->h = h;
  for (i = minGray; i < maxGray; i++)
    gryArray[i] = index;
  return info;
  }

private procedure InitSLFiles()
  /* Initialize screen threshold array file: */
  {
   integer initialSize;
#if DISKSL
  DiskAttributes dattr; Disk disk;
  DURING {
    disk = DiskGetNext((Disk) NIL);
    if (disk == NIL) RAISE(ecNotInitialized,"");
    DiskGetAttributes(disk, &dattr);
    slstm = FileDirStmOpen(disk, SLName, accUpdate, MIN(4000, dattr.size/5));
    stmBytesUsed = 0;
    haveFiles = true;
    DecStmByteCnt(0L);
    }
  HANDLER{
    haveFiles = false;}
  END_HANDLER;
  haveFiles = false;
#endif
  }

private procedure IniDeepOnes(); /*forward */

public procedure InitPatternImpl(
  maxPatSize, maxTotPatSize, maxThrArrSize, maxTotThrArrSize)
  integer maxPatSize, maxTotPatSize; {
  patTimeStamp = 0;
  patID = 0;
  lastFree = 1;
  maxPatternSize = maxPatSize;
  maxTotalPatternSize = maxTotPatSize;
  patterns = (PatCacheInfo **)
    os_sureMalloc((long int)(MAXPAT * sizeof(PatCacheInfo *)));
  os_bzero((char *)patterns, (long int)(MAXPAT * sizeof(PatCacheInfo *)));
  grayPatternBase = (PSCANTYPE) os_sureMalloc((long int)maxPatSize);
  IniDeepOnes();
  totalAlloc = maxPatSize + MAXPAT * sizeof(PatCacheInfo *);
  Assert (maxTotPatSize > totalAlloc + 4 * MAXPAT + sizeof(ScreenPrivate));
  halftonePool = os_newpool(sizeof(DevHalftone),4,0);
  screenPool = os_newpool(sizeof(DevScreen)+sizeof(ScreenPrivate),8,0);

#if DISKSL
  maxTASize = maxThrArrSize;
  maxTotalTASize = maxTotThrArrSize;
  Assert(maxTASize<=maxTotalTASize);
  InitSLFiles();
  curTAUsage = 0;
  taTimeStamp = 0;
  screens = (DevScreen **)
    os_sureMalloc((long int)(MAXSCRN * sizeof(DevScreen *)));
  os_bzero((char *)screens, (long int)(MAXSCRN * sizeof(DevScreen *)));
  largestChIndex = NULLSLOT;
  largestChunk = NULL;
  largestChSize = 0;
#endif  /* DISKSL */
  spCount = 0;
  }


public PSCANTYPE *deepOnes;
public PSCANTYPE *deepPixOneVals;
public PSCANTYPE *deepPixOnes;
public PSCANTYPE *swapPixOnes;
public PSCANTYPE onebit;

#if SWAPBITS == DEVICE_CONSISTENT

#if SCANUNIT==8
public readonly SCANTYPE leftBitArray[SCANUNIT] =
  {0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80};

public readonly SCANTYPE rightBitArray[SCANUNIT] =
  {0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f};

/*
  public PSCANTYPE onebit;
    {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
*/
#endif SCANUNIT==8

#if SCANUNIT==16
public readonly SCANTYPE leftBitArray[SCANUNIT] =
  {0xffff, 0xfffe, 0xfffc, 0xfff8, 0xfff0, 0xffe0, 0xffc0, 0xff80,
   0xff00, 0xfe00, 0xfc00, 0xf800, 0xf000, 0xe000, 0xc000, 0x8000};

public readonly SCANTYPE rightBitArray[SCANUNIT] =
  {0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f,
   0xff, 0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff, 0x3fff, 0x7fff};

/*
  public PSCANTYPE onebit;
  {0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
   0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000};*/
#endif SCANUNIT==16

#if SCANUNIT==32
public readonly SCANTYPE leftBitArray[SCANUNIT] =
  {0xffffffffL, 0xfffffffeL, 0xfffffffcL, 0xfffffff8L,
    0xfffffff0L, 0xffffffe0L, 0xffffffc0L, 0xffffff80L,
   0xffffff00L, 0xfffffe00L, 0xfffffc00L, 0xfffff800L,
     0xfffff000L, 0xffffe000L, 0xffffc000L, 0xffff8000L,
   0xffff0000L, 0xfffe0000L, 0xfffc0000L, 0xfff80000L,
     0xfff00000L, 0xffe00000L, 0xffc00000L, 0xff800000L,
   0xff000000L, 0xfe000000L, 0xfc000000L, 0xf8000000L,
     0xf0000000L, 0xe0000000L, 0xc0000000L, 0x80000000L};

public readonly SCANTYPE rightBitArray[SCANUNIT] =
  {0x00000000L, 0x00000001L, 0x00000003L, 0x00000007L,
   0x0000000fL, 0x0000001fL, 0x0000003fL, 0x0000007fL,
   0x000000ffL, 0x000001ffL, 0x000003ffL, 0x000007ffL,
   0x00000fffL, 0x00001fffL, 0x00003fffL, 0x00007fffL,
   0x0000ffffL, 0x0001ffffL, 0x0003ffffL, 0x0007ffffL,
   0x000fffffL, 0x001fffffL, 0x003fffffL, 0x007fffffL,
   0x00ffffffL, 0x01ffffffL, 0x03ffffffL, 0x07ffffffL,
   0x0fffffffL, 0x1fffffffL, 0x3fffffffL, 0x7fffffffL};

/*
  public PSCANTYPE onebit;
  {0x00000001, 0x00000002, 0x00000004, 0x00000008,
   0x00000010, 0x00000020, 0x00000040, 0x00000080,
   0x00000100, 0x00000200, 0x00000400, 0x00000800,
   0x00001000, 0x00002000, 0x00004000, 0x00008000,
   0x00010000, 0x00020000, 0x00040000, 0x00080000,
   0x00100000, 0x00200000, 0x00400000, 0x00800000,
   0x01000000, 0x02000000, 0x04000000, 0x08000000,
   0x10000000, 0x20000000, 0x40000000, 0x80000000}; */
#endif SCANUNIT==32

#else SWAPBITS == DEVICE_CONSISTENT

#if SCANUNIT==8
public readonly SCANTYPE leftBitArray[SCANUNIT] =
  {0xff, 0x7f, 0x3f, 0x1f, 0xf, 0x7, 0x3, 0x1};

public readonly SCANTYPE rightBitArray[SCANUNIT] =
  {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};

/*
  public PSCANTYPE onebit;
  {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01}; */
#endif SCANUNIT==8

#if SCANUNIT==16
public readonly SCANTYPE leftBitArray[SCANUNIT] =
  {0xffff, 0x7fff, 0x3fff, 0x1fff, 0xfff, 0x7ff, 0x3ff, 0x1ff,
     0xff,   0x7f,   0x3f,   0x1f,   0xf,   0x7,   0x3,   0x1};

public readonly SCANTYPE rightBitArray[SCANUNIT] =
  {0x0000, 0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xfe00,
   0xff00, 0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8, 0xfffc, 0xfffe};

/*
  public PSCANTYPE onebit;
  {0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0100,
   0x0080, 0x0040, 0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001}; */
#endif SCANUNIT==16

#if SCANUNIT==32
public readonly SCANTYPE leftBitArray[SCANUNIT] =
  {0xffffffffL, 0x7fffffffL, 0x3fffffffL, 0x1fffffffL, 0xfffffffL, 0x7ffffffL,
   0x3ffffffL, 0x1ffffffL, 0xffffffL, 0x7fffffL, 0x3fffffL, 0x1fffffL,
   0xfffffL, 0x7ffffL, 0x3ffffL, 0x1ffffL,
   0xffffL, 0x7fffL, 0x3fffL, 0x1fffL, 0xfffL, 0x7ffL, 0x3ffL, 0x1ffL,
   0xffL, 0x7fL, 0x3fL, 0x1fL, 0xfL, 0x7L, 0x3L, 0x1L};

public readonly SCANTYPE rightBitArray[SCANUNIT] =
  {0x00000000L, 0x80000000L, 0xc0000000L, 0xe0000000L, 0xf0000000L,
   0xf8000000L, 0xfc000000L, 0xfe000000L, 0xff000000L, 0xff800000L,
   0xffc00000L, 0xffe00000L, 0xfff00000L, 0xfff80000L, 0xfffc0000L,
   0xfffe0000L, 0xffff0000L, 0xffff8000L, 0xffffc000L, 0xffffe000L,
   0xfffff000L, 0xfffff800L, 0xfffffc00L, 0xfffffe00L, 0xffffff00L,
   0xffffff80L, 0xffffffc0L, 0xffffffe0L, 0xfffffff0L, 0xfffffff8L,
   0xfffffffcL, 0xfffffffeL};

/*
  public PSCANTYPE onebit;

  {0x80000000L, 0x40000000L, 0x20000000L, 0x10000000L, 0x8000000L, 0x4000000L,
   0x2000000L, 0x1000000L, 0x800000L, 0x400000L, 0x200000L, 0x100000L,
   0x80000L, 0x40000L, 0x20000L, 0x10000L, 0x8000L, 0x4000L, 0x2000L, 0x1000L,
   0x800L, 0x400L, 0x200L, 0x100L, 0x80L, 0x40L, 0x20L, 0x10L, 0x8L, 0x4L,
   0x2L, 0x1L};
*/
#endif SCANUNIT==32

#endif SWAPBITS == DEVICE_CONSISTENT

#define GETMEM(n) (PSCANTYPE) os_sureMalloc((long int)(n))

public procedure SetupDeepOnes(bpp) integer bpp; {
  integer i;
  switch (bpp) {
    case 1: {
      if (deepOnes[0] != NULL) return;
      deepOnes[0] = GETMEM(SCANUNIT * sizeof(SCANTYPE));
      deepPixOnes[0] = GETMEM(SCANUNIT * sizeof(SCANTYPE));
      swapPixOnes[0] = GETMEM(SCANUNIT * sizeof(SCANTYPE));
      deepPixOneVals[0] = deepPixOnes[0];
#if SWAPBITS == DEVICE_CONSISTENT
      for (i = 0; i < SCANUNIT; i++) deepOnes[0][i] = 0x1 << i;
      for (i = 0; i < SCANUNIT; i++) {
	deepPixOnes[0][i] = 0x1 << i;
	swapPixOnes[0][i] = 0x1 << (SCANUNIT - i - 1);
	}
#else SWAPBITS == DEVICE_CONSISTENT
      for (i = 0; i < SCANUNIT; i++) 
        deepOnes[0][i] = 0x1 << (SCANUNIT - i - 1);
      for (i = 0; i < SCANUNIT; i++) {
	deepPixOnes[0][i] = 0x1 << (SCANUNIT - i - 1);
	swapPixOnes[0][i] = 0x1 << i;
	}
#endif SWAPBITS == DEVICE_CONSISTENT
      break;
      }
    case 2: {
      if (deepOnes[1] != NULL) return;
      deepOnes[1] = GETMEM(SCANUNIT * sizeof(SCANTYPE));
      deepPixOnes[1] = GETMEM(SCANUNIT/2 * sizeof(SCANTYPE));
      swapPixOnes[1] = GETMEM(SCANUNIT/2 * sizeof(SCANTYPE));
      deepPixOneVals[1] =  GETMEM(SCANUNIT/2 * sizeof(SCANTYPE));
#if SWAPBITS == DEVICE_CONSISTENT
      for (i = 0; i < SCANUNIT; i+=2) deepOnes[1][i] = 0x3 << i;
      for (i = 0; i < (SCANUNIT >> 1); i++) {
	deepPixOnes[1][i] = 0x3 << (i << 1);
	swapPixOnes[1][i] = 0x3  << (SCANUNIT - (i * 2) - 2);
        deepPixOneVals[1][i] = 1  << (i << 1);
	}
#else SWAPBITS == DEVICE_CONSISTENT
      for (i = 0; i < SCANUNIT; i+=2)
        deepOnes[1][i] = 0x3  << (SCANUNIT - i - 2);
      for (i = 0; i < (SCANUNIT >> 1); i++) {
	deepPixOnes[1][i] = 0x3  << (SCANUNIT - (i * 2) - 2);
	swapPixOnes[1][i] = 0x3  << (i << 1);
	deepPixOneVals[1][i] = 1  << (SCANUNIT - (i * 2) - 2);
	}
#endif SWAPBITS == DEVICE_CONSISTENT
      break;
      }
    case 4: {
      if (deepOnes[2] != NULL) return;
      deepOnes[2] = GETMEM(SCANUNIT * sizeof(SCANTYPE));
      deepPixOnes[2] = GETMEM(SCANUNIT/4 * sizeof(SCANTYPE));
      swapPixOnes[2] = GETMEM(SCANUNIT/4 * sizeof(SCANTYPE));
      deepPixOneVals[2] = GETMEM(SCANUNIT/4 * sizeof(SCANTYPE));
#if SWAPBITS == DEVICE_CONSISTENT
      for (i = 0; i < SCANUNIT; i+=4) deepOnes[2][i] = 0xf  << i;
      for (i = 0; i < (SCANUNIT >> 2); i++) {
	deepPixOnes[2][i] = 0xf  << (i << 2);
	swapPixOnes[2][i] = 0xf  << (SCANUNIT - (i * 4) - 4);
	deepPixOneVals[2][i] = 1  << (i << 2);
	}
#else SWAPBITS == DEVICE_CONSISTENT
      for (i = 0; i < SCANUNIT; i+=4)
        deepOnes[2][i] = 0xf  << (SCANUNIT - i - 4);
      for (i = 0; i < (SCANUNIT >> 2); i++) {
	deepPixOnes[2][i] = 0xf  << (SCANUNIT - (i * 4) - 4);
	swapPixOnes[2][i] = 0xf  << (i << 2);
	deepPixOneVals[2][i] = 1  << (SCANUNIT - (i * 4) - 4);
	}
#endif SWAPBITS == DEVICE_CONSISTENT
      break;
      }
    case 8: {
      if (deepOnes[3] != NULL) return;
      deepOnes[3] = GETMEM(SCANUNIT * sizeof(SCANTYPE));
      deepPixOnes[3] = GETMEM(SCANUNIT/8 * sizeof(SCANTYPE));
      swapPixOnes[3] = GETMEM(SCANUNIT/8 * sizeof(SCANTYPE));
      deepPixOneVals[3] = GETMEM(SCANUNIT/8 * sizeof(SCANTYPE));
#if SWAPBITS == DEVICE_CONSISTENT
      for (i = 0; i < SCANUNIT; i+=8) deepOnes[3][i] = 0xff << i;
      for (i = 0; i < (SCANUNIT >> 3); i++) {
	deepPixOnes[3][i] = 0xff << (i << 3);
	swapPixOnes[3][i] = 0xff << (SCANUNIT - (i * 8) - 8);
	deepPixOneVals[3][i] = 1 << (i << 3);
	}
#else SWAPBITS == DEVICE_CONSISTENT
      for (i = 0; i < SCANUNIT; i+=8)
        deepOnes[3][i] = 0xff << (SCANUNIT - i - 8);
      for (i = 0; i < (SCANUNIT >> 3); i++) {
	deepPixOnes[3][i] = 0xff << (SCANUNIT - (i * 8) - 8);
	swapPixOnes[3][i] = 0xff << (i << 3);
	deepPixOneVals[3][i] = 1 << (SCANUNIT - (i * 8) - 8);
	}
#endif SWAPBITS == DEVICE_CONSISTENT
      break;
      }
#if SCANUNIT >= 16
    case 16: {
      if (deepOnes[4] != NULL) return;
      deepOnes[4] = GETMEM(SCANUNIT * sizeof(SCANTYPE));
      deepPixOnes[4] = GETMEM(SCANUNIT/16 * sizeof(SCANTYPE));
      swapPixOnes[4] = GETMEM(SCANUNIT/16 * sizeof(SCANTYPE));
      deepPixOneVals[4] = GETMEM(SCANUNIT/16 * sizeof(SCANTYPE));
#if SWAPBITS == DEVICE_CONSISTENT
      for (i = 0; i < SCANUNIT; i+=16) deepOnes[4][i] = 0xffff << i;
      for (i = 0; i < (SCANUNIT >> 4); i++) {
	deepPixOnes[4][i] = 0xffff << (i << 4);
	swapPixOnes[4][i] = 0xffff << (SCANUNIT - (i * 16) - 16);
	deepPixOneVals[4][i] = 1 << (i << 4);
	}
#else SWAPBITS == DEVICE_CONSISTENT
      for (i = 0; i < SCANUNIT; i+=16)
        deepOnes[4][i] = 0xffff << (SCANUNIT - i - 16);
      for (i = 0; i < (SCANUNIT >> 4); i++) {
	deepPixOnes[4][i] = 0xffff << (SCANUNIT - (i * 16) - 16);
	swapPixOnes[4][i] = 0xffff << (i << 4);
	deepPixOneVals[4][i] = 1 << (SCANUNIT - (i * 16) - 16);
	}
#endif SWAPBITS == DEVICE_CONSISTENT
      break;
      }
#endif SCANUNIT >= 16
#if SCANUNIT == 32
    case 32: {
      if (deepOnes[5] != NULL) return;
      deepOnes[5] = GETMEM(SCANUNIT * sizeof(SCANTYPE));
      deepPixOnes[5] = GETMEM(SCANUNIT/32 * sizeof(SCANTYPE));
      swapPixOnes[5] = GETMEM(SCANUNIT/32 * sizeof(SCANTYPE));
      deepPixOneVals[5] = GETMEM(SCANUNIT/32 * sizeof(SCANTYPE));
#if SWAPBITS == DEVICE_CONSISTENT
      for (i = 0; i < SCANUNIT; i+=32) deepOnes[5][i] = 0xffffffff << i;
      for (i = 0; i < (SCANUNIT >> 5); i++) {
	deepPixOnes[5][i] = 0xffffffff << (i << 5);
	swapPixOnes[5][i] = 0xffffffff << (SCANUNIT - (i * 32) - 32);
	deepPixOneVals[5][i] = 1 << (i << 5);
	}
#else SWAPBITS == DEVICE_CONSISTENT
      for (i = 0; i < SCANUNIT; i+=32)
        deepOnes[5][i] = 0xffffffff << (SCANUNIT - i - 32);
      for (i = 0; i < (SCANUNIT >> 5); i++) {
	deepPixOnes[5][i] = 0xffffffff << (SCANUNIT - (i * 32) - 32);
	swapPixOnes[5][i] = 0xffffffff << (i << 5);
	deepPixOneVals[5][i] = 1 << (SCANUNIT - (i * 32) - 32);
	}
#endif SWAPBITS == DEVICE_CONSISTENT
      break;
      }
#endif SCANUNIT == 32
    }
  }

private procedure IniDeepOnes() {
  integer i;
  deepOnes =
    (PSCANTYPE *) os_calloc((long int)6, (long int)sizeof(PSCANTYPE));
  deepPixOnes =
    (PSCANTYPE *) os_calloc((long int)6, (long int)sizeof(PSCANTYPE));
  swapPixOnes =
    (PSCANTYPE *) os_calloc((long int)6, (long int)sizeof(PSCANTYPE));
  deepPixOneVals =
    (PSCANTYPE *) os_calloc((long int)6, (long int)sizeof(PSCANTYPE));
  SetupDeepOnes((integer)1);
  }
  
  
