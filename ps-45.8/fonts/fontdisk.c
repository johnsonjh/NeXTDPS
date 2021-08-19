/*
  fontdisk.c

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

Original version: Chuck Geschke: July 14, 1983
Edit History:
Chuck Geschke: Wed Jan  8 21:20:20 1986
Doug Brotz: Mon Jun  2 17:15:43 1986
Ed Taft: Sun Dec 17 17:52:39 1989
Bill Paxton: Mon Jan 14 13:09:47 1985
Don Andrews: Thu Jul 10 10:07:12 1986
Ivor Durham: Sat Aug 27 14:08:23 1988
Jim Sandman: Tue Dec 12 14:28:05 1989
Bill Bilodeau: Fri Jan 20 16:45:41 PST 1989
Joe Pasqua: Tue Feb 28 13:44:39 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include LANGUAGE
#include ORPHANS
#include SIZES
#include STREAM
#include VM

#include "diskcache.h"
#include "fontcache.h"
#include "fontdata.h"

/* The following procedures should be in some interface	*/
/* local to the fonts package.				*/
/* The following procedures are extern'd here since	*/
/* they're not exported by the postscript package.	*/
extern procedure DisableCC(), EnableCC();

/* default parameters for font cache */

public integer defaultCompThreshold, defaultCacheThreshold;

public boolean cchInited, dlFC;

public FCDataRec fcData;

private procedure InitFontCache();

#if STAGE==DEVELOP
private procedure PSInitFontCache()
{
  integer mtSize, ctSize;
  ctSize = PopInteger()+1;
  mtSize = PopInteger()+1;
  if (mtSize > MAXMID) LimitCheck();
  if (ctSize > MAXCISize) LimitCheck();
  InitFontCache(mtSize, ctSize);
}
#endif STAGE==DEVELOP

public procedure StartCache()
{
  /* the formula below provides the following partitioning of the
     area reserved for the font cache (MID & CI).  We want x to be
     the total bytes and there to be 12 times as many CI as MID. */
  integer x, nMID, nCI;
  integer bytesPerMID =
    sizeof(MTItem) + (2 * sizeof(Card16)) + (2 * sizeof(Card32));
  integer bytesPerCI = sizeof(CIItem) + sizeof(PNameEntry);
  x = ps_getsize(SIZE_FONT_CACHE, 120000);
  nMID = MAX(x/(bytesPerMID + 12 * bytesPerCI),5);
  nCI = MAX((x-(nMID*bytesPerMID))/bytesPerCI,10);
  /* sizes interface override */
  nCI = ps_getsize(SIZE_MASKS,nCI);
  nMID = ps_getsize(SIZE_MIDS,nMID);
  InitFontCache(nMID, nCI);
}

private procedure InitFontCache(mtSize, ctSize) integer mtSize, ctSize; {
  register integer i; Object ob;
  register PNameEntry *pno; register PNameEntry pne;
  ForAllNames(pno, pne, i) pne->ncilink = CINULL;
  MMSize = MAX(mtSize/3,3); MMSize = MIN(MMSize, MAXMSize);
  if (MM == NULL) MM = (MID *)(NEW(MMSize,sizeof(MID)));
  MMEnd = &MM[MMSize];
  {register MID *m; forallMM(m){*m = MIDNULL;}}
  MSSize = MMSize;
  if (MS == NULL) MS = (MID *)(NEW(MSSize,sizeof(MID)));
  MSEnd = &MS[MSSize];
  {register MID *m; forallMS(m){*m = MIDNULL;}}
  MTSize = MIN(mtSize,MAXMTSize);
  if (MT == NULL) MT = (PMTItem)(NEW(MTSize,sizeof(MTItem)));
  MTEnd = &MT[MTSize];
  if (MA == NULL) MA = (PCard32)(NEW(MTSize,sizeof(Card32)));
  if (MIDCount == NULL) MIDCount = (integer *)(NEW(MTSize,sizeof(integer)));
  if (IndMArray == NULL) IndMArray = (PCard16)(NEW(MTSize,sizeof(Card16)));
  if (IndSArray == NULL) IndSArray = (PCard16)(NEW(MTSize,sizeof(Card16)));
  MA[0] = PeterPan;
  CurrentMIDAge = Methuselah;
  InitSortMID ();
  MTFreeHead = MIDNULL;
    {
    register PMTItem m = MT;
    m->type = allocmem;  /* MT[0] permanently allocated for MIDNULL */
    m->umid.rep.generation = 1; /* ensure MIDNULL will not match */
    for(i=1; i<MTSize; i++)
      {
      m++;
      m->type = freemem;
      m->umid.rep.mid = i;
      m->mlink = MTFreeHead;
      MTFreeHead = i;
      }
    }
  CISize = MIN(ctSize,MAXMasks);
  if (CI == NULL) CI = (PCIItem)(NEW(CISize,sizeof(CIItem)));
  CIEnd = &CI[CISize];
  CIFreeHead = CINULL;
  {register PCIItem c = CI;
    c->type = allocmem;
    for(i=1; i<CISize; i++){
      c++; c->type = freemem; c->cilink = CIFreeHead; CIFreeHead = i;}}
  if (CN == NULL) CN = (PNameEntry *)(NEW(CISize,sizeof(PNameEntry)));
  fcData.MIDcnt = 0; fcData.CIcnt = 0;
  cchInited = true;
}


private procedure SetCacheLimit(cacheThreshold) integer cacheThreshold; {
  integer used, size;
  DevMaskCacheInfo(&used, &size);
  if (cacheThreshold < 0) RangeCheck();
  if (5*cacheThreshold > size) LimitCheck();
  WriteContextParam (
    (char *)&ctxCacheThreshold, (char *)&cacheThreshold,
    (integer)sizeof(cacheThreshold), (PVoidProc)NIL);
  }

public procedure PSSetCacheLimit() { SetCacheLimit(PopInteger()); }

public procedure PSCacheStatus() {
  integer used, size;
  DevMaskCacheInfo(&used, &size);
  PushInteger(used);
  PushInteger(size);
  PushInteger(fcData.MIDcnt);
  PushInteger(MTSize-1);
  PushInteger(fcData.CIcnt);
  PushInteger(CISize-1);
  PushInteger (ctxCacheThreshold);
  }  /* end of PSCacheStatus */

public procedure PSStCParams() {
  integer n, v, size, limit, defer;
  IntObj ob;
  defer = 0;
  n = CountToMark(opStk);
  if (n > 0) {
    limit = PopInteger();
    n--;
  } else {
    limit = defaultCacheThreshold;
  }
  if (n > 0) {
    v = PopInteger();
    WriteContextParam(
      (char *)&ctxCompThreshold, (char *)&v,
      (integer)sizeof(v), (PVoidProc)NIL);
    n--;
  }
  if (n > 0) {
  	/* size can be no less than 5 * CacheThreshold */
    DevSetMaskCacheSize(PopInteger(),limit*5);
    n--;
  }
  SetCacheLimit(limit); 	
  while (n-- >= 0) PopP(&ob);
}  /* end of PSStCParams */

public procedure PSCrCParams() {
  MarkObj mark;  Object ob;
  integer used, size;
  DevMaskCacheInfo(&used, &size);
  LMarkObj(mark);
  PushP(&mark);
  PushInteger (size);
  PushInteger (ctxCompThreshold);
  PushInteger (ctxCacheThreshold);
  }  /* end of PSCrCParams */

  
public RstrFC(fcStream) Stm fcStream; {
  if (!cchInited) StartCache();
  }

public procedure ReInitFontCache() {}

public procedure FontDiskInit(reason)
	InitReason reason;
{
  switch (reason){
    case init: {
      cchInited = false;
      defaultCacheThreshold = dfCacheThreshold;
      defaultCompThreshold = dfCompThreshold;
      MT = NULL; MM = NULL; MS = NULL; CI = NULL; CN = NULL;
#if VMINIT
      /* verify conditions essential to correct calls of AssembleROM */
      Assert(sizeof(MTItem) % 4 == 0);
#endif VMINIT
      break;}
    case romreg: {
#if STAGE==DEVELOP
      if (vSTAGE==DEVELOP) RgstExplicit("initfontcache",PSInitFontCache);
#endif STAGE==DEVELOP
      if (!cchInited) {StartCache();}
      break;}
    endswitch}
}

