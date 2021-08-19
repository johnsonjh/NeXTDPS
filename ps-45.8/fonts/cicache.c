/*
  cicache.c

Copyright (c) 1984, '85, '86, '87, '88, '90 Adobe Systems Incorporated.
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
Ed Taft: Fri Jul 29 09:36:07 1988
Ivor Durham: Sun Aug 14 14:00:13 1988
Jim Sandman: Wed Apr  4 09:50:48 1990
Perry Caro: Wed Nov  9 14:16:42 1988
Joe Pasqua: Tue Feb 28 13:43:07 1989
Bill Bilodeau: Fri Jan 20 16:45:41 PST 1989
Rick Saenz: Thu Mar 22 12:00:00 PST 1990
End Edit History.
*/

#include PACKAGE_SPECS

#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include DEVICE
#include GC
#include GRAPHICS
#include LANGUAGE
#include ORPHANS
#include PSLIB
#include STREAM
#include VM

#include "fontcache.h"
#include "fontshow.h"
#include "fontdata.h"
#include "fontsnames.h"


/* globals */


public integer CISize;
public PCIItem CI, CIEnd;
public CIOffset CIFreeHead, CISortedList, CISortedEnd;
public PNameEntry *CN;
public integer charSortInterval;

private boolean TrimCI();

#if STAGE==DEVELOP
#define IsConnected() \
  (((cip->ageflink == CINULL) || (CI[cip->ageflink].ageblink == cio)) && \
   ((cip->ageblink == CINULL) || (CI[cip->ageblink].ageflink == cio)))
#define IsFreeCI() \
  (((cip->type & Vmem) == 0) && (CN[cio] == NIL) && \
    (cip->ageflink == CINULL) && (cip->ageblink == CIUNLINKED) && \
     (cip->mid == MIDNULL) && (cip->cilink != CIUNLINKED))
#define IsAllocatedCI() \
  (((cip->type & Vmem) == 0) && (CN[cio] == NIL) && \
    (cip->mid == MIDNULL) && (cip->cilink != CIUNLINKED) && IsConnected())
#define IsReleasedCI() \
  ((cip->cilink == CIUNLINKED) && ((cip->type & Vmem) != 0) && \
   (CN[cio] == NIL) && (cip->mid != MIDNULL) && IsConnected ())
#define IsNormalCI() \
  ((cip->cilink != CIUNLINKED) && ((cip->type & Vmem) != 0) && \
   (CN[cio] != NIL) && (cip->mid != MIDNULL) && IsConnected ())

public CheckCI()
{
  register CIOffset cio, ncio;
  register PCIItem cip;
  register integer i, *cptr;
  extern integer nUsedMasks;
  integer nCi;
  if (!fcCheck) return;
  cio = 0;
  forallCI(cip)
  {
    cio++;
    if ((cip->type & Vmem) == 0)
      { if (cip->ageblink == CIUNLINKED)
          { Assert(IsFreeCI());}
        else
          { Assert(IsAllocatedCI());}
      }
    else if (cip->cilink == CIUNLINKED)
      { Assert(IsReleasedCI()); }
    else
      { Assert(IsNormalCI());
        ncio = CN[cio]->ncilink;
        while(true)
        {
          if (ncio == cio) break;
          Assert (ncio != CINULL);
          ncio = CI[ncio].cilink;
        }
      }
  }
  cio = CISortedList;
  nCi = 0;
  while (cio != CINULL)
  {
    nCi++;
    cip = &CI[cio];
    MIDCount[cip->mid] += 65536;
    cio = cip->ageflink;
  }
  if (nCi != nUsedMasks)
    nCi = 0;
  cptr = &MIDCount[1];
  for (i = 1; i < MTSize; i++)
  {
    Assert((*cptr/65536) == (*cptr%65536));
    *cptr++ %= 65536;
  }
/*  CheckSL();*/
}


#endif STAGE==DEVELOP

public CIOffset CIAlloc() {
  CIOffset cio; register PCIItem cip;
  if (CIFreeHead == CINULL){
    if (!TrimCI()) return CINULL;}
  cio = CIFreeHead; cip = &CI[cio];
  Assert(cip->type == freemem);
  cip->type = allocmem;
  if (CISortedEnd != CINULL)
    CI[CISortedEnd].ageflink = cio;
  else
    CISortedList = cio;
  cip->ageblink = CISortedEnd;
  CISortedEnd = cio;
  CIFreeHead = cip->cilink;
  if (CIFreeHead != CINULL) Assert(CI[CIFreeHead].type == freemem);
  fcData.CIcnt++;
  return cio;
}

public procedure CIFree(c)
	CIOffset c;
{
  register PCIItem cip = &CI[c];
#if STAGE==DEVELOP
  Assert(CN[c] == NIL);
#endif STAGE==DEVELOP
  cip->type = freemem;
  cip->mid = MIDNULL;
  cip->cilink = CIFreeHead;
  cip->cmp = NIL;
  CIFreeHead = c;
  if (cip->ageflink != CINULL)
    CI[cip->ageflink].ageblink = cip->ageblink;
  else
    CISortedEnd = cip->ageblink;
  if (cip->ageblink != CINULL)
    CI[cip->ageblink].ageflink = cip->ageflink;
  else
    CISortedList = cip->ageflink;
  cip->ageflink = CINULL;
  cip->ageblink = CIUNLINKED;
  fcData.CIcnt--;
}


#define NextAge(X) CIReg [X].ageflink
#define PrevAge(X) CIReg [X].ageblink
#define IsNull(X) X == CINULL
#define IsNonNull(X) X != CINULL
#define CondAssign(X,Y,Z) \
  {if (IsNonNull (Y)) NextAge (Y) = Z; else X = Z; \
   if (IsNonNull (Z)) PrevAge (Z) = Y;}
#define CondAssignF(X,Y,Z) {if (IsNonNull (Y)) NextAge (Y) = Z; else X = Z;}
#define IsMarked(X) (CIReg [X].touched)
#define ClearMark(X) CIReg [X].touched = false

public procedure SortCharAges ()
{
  register CIOffset P, C, N, R, S;
  register PCIItem CIReg = CI;
#if STAGE==DEVELOP
  CheckBM ();
  CheckCI ();
#endif STAGE==DEVELOP
  R = S = P = CINULL;
  C = CISortedList;
  while (IsNonNull (C) && (IsNull (P) || IsNull (S)))
  {
    N = NextAge (C);
    if (IsMarked (C))
    {
      CondAssignF (CISortedList, P, N);
      CondAssignF (R, S, C);
      S = C;
    }
    else
      P = C;
    ClearMark (C);
    C = N;
  }
  while (IsNonNull (C))
  {
    N = NextAge (C);
    if (IsMarked (C))
    {
      NextAge (P) = N;
      NextAge (S) = C;
      S = C;
    }
    else
      P = C;
    ClearMark (C);
    C = N;
  }
  if (IsNonNull (S))
  {
    NextAge (S) = CINULL;
    CISortedEnd = S;
  }
  else
    CISortedEnd = P;
  CondAssignF (CISortedList, P, R);
  C = CISortedList;
  P = CINULL;
  while (IsNonNull (C))
  {
    PrevAge (C) = P;
    P = C;
    C = NextAge (C);
  }
  charSortInterval = 0;
#if STAGE==DEVELOP
  CheckBM ();
  CheckCI ();
#endif STAGE==DEVELOP
}


private procedure UnlinkCI(oldcio)
  register CIOffset oldcio; {
  register PCIOffset pcio;
  register CIOffset cio; register PCIItem cip;
  if (CI[oldcio].cilink != CIUNLINKED) {
    pcio = &CN[oldcio]->ncilink;
    CN[oldcio] = NIL;
    cio = *pcio;
    while (cio != CINULL){
      cip = CI + cio;
      if (cio == oldcio){
	*pcio = cip->cilink; cip->cilink = CIUNLINKED; return;}
      pcio = &(cip->cilink);
      cio = cip->cilink;}
    CantHappen();
    }
  }

private procedure RelinkCIItem(cio) CIOffset cio; {
  register PCIItem cip = &CI[cio];
  if (CISortedList == CISortedEnd) return;
  if (cip->ageflink != CINULL)
    CI[cip->ageflink].ageblink = cip->ageblink;
  else
    CISortedEnd = cip->ageblink;
  if (cip->ageblink != CINULL)
    CI[cip->ageblink].ageflink = cip->ageflink;
  else
    CISortedList = cip->ageflink;
  CI[CISortedEnd].ageflink = cio;
  cip->ageflink = CINULL;
  cip->ageblink = CISortedEnd;
  CISortedEnd = cio;
}

#define MaxMCount 20

private procedure GetFlushArgs(cio, args)
  CIOffset cio; DevFlushMaskArgs *args; {
  PCIItem cip = &CI[cio];
  PMTItem mp = &MT[cip->mid];
  args->font = mp->fid & prebuiltFIDMask;
  args->matrix = &mp->mtx;
  args->name = (char *) CN[cio]->str;
  args->nameLength = CN[cio]->strLen;
  args->horizMetrics = &cip->metrics[0];
  args->vertMetrics = &cip->metrics[1];
  }

private integer ReleaseCI(cio) CIOffset cio; {
  integer freed;
  PCIItem cip = &CI[cio];
  if(!(cip->circle && FlushCircle(cip->cmp))) {
    DevFlushMaskArgs args;
    GetFlushArgs(cio, &args);
    freed = DevFlushMask (cip->cmp, &args);
    }
  PurgeFSCache(cip->mid);
  UnlinkCI(cio);
  CIFree(cio);
  return freed;
  };

private boolean TrimCI () {
  register CIOffset cio; register PCIItem CIReg, cip;
  CIOffset next;
  integer MCount, amount, nTrimmed = 0;
  boolean result = false;
  amount = CISize / 16;
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP
  CIReg = CI;
Restart:
  MCount = 0;
  cio = CISortedList;
  while (cio != CINULL) {
    cip = &CIReg [cio];
    next = cip->ageflink;
    if (cip->type != Vmem) {
      cio = next; continue;}
    if (cip->touched) {
      MCount++;
      if (MCount > MaxMCount) {
	SortCharAges ();
        MCount = 0;
	goto Restart;
	}
      cip->touched = false;
      RelinkCIItem(cio);
      cio = next;
      continue;
      }
    MIDCount[cip->mid]--;
#if STAGE==DEVELOP
    Assert(MIDCount[cip->mid] >= 0);
    Assert (cip->type != freemem);
#endif STAGE==DEVELOP
    ReleaseCI(cio);
    result = true;
    nTrimmed++;
    if (nTrimmed >= amount)
      break;
    cio = next;
    }
  return result;
  }


public procedure DeleteCIs(mid) MID mid; {
  CIOffset CISLTemp; integer count;
  register CIOffset cio; register PCIItem cip;
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP
  if ((count = MIDCount[mid]) == 0) return;
  MIDCount[mid] = 0;
  CISLTemp = CISortedList;
  while (count != 0) {
    Assert(CISLTemp != CINULL);
    cio = CISLTemp;
    cip = &CI[cio];
    CISLTemp = cip->ageflink;
    if (cip->mid == mid) {
      count--;
      ReleaseCI(cio);
      }
    }
  }


#if 0  /* As per Rick Saenz */
/* XXX this needs to be moved ... */
extern procedure PutMaskInDiskCache();

public boolean FlushMID(mid) MID mid; {
  CIOffset CISLTemp; integer count;
  register CIOffset cio; register PCIItem cip;
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP
  if ((count = MIDCount[mid]) == 0) return(false);
  CISLTemp = CISortedList;
  while (count != 0) {
    Assert(CISLTemp != CINULL);
    cio = CISLTemp;
    cip = &CI[cio];
    CISLTemp = cip->ageflink;
    if (cip->mid == mid) {
      count--;
      if (!cip->cmp->onDisk && cip->cmp->data) {
	DevFlushMaskArgs args;
	GetFlushArgs(cio, &args);
	PutMaskInDiskCache(cip->cmp, &args);
	return(true);
      }
    }
  }
  return(false);
}
#endif

public PurgeCI(pne) PNameEntry pne; {
  register CIOffset dcio, cio; register PCIItem cip;
  cio = pne->ncilink;
  while(cio != CINULL) {
    cip = &CI[cio];
    dcio = cio; cio = cip->cilink;
    if (cip->mid == MIDNULL){
      CIFree(dcio);}
    else {
      MIDCount[cip->mid]--;
      PurgeFSCache(cip->mid);
      ReleaseCI(dcio);
      }
    }
  pne->ncilink = CINULL;
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP
}

public integer PSFlushMasks(needed, maskID) integer needed, maskID; {
  register MID m, next;
  register integer count, freed = 0;
  register CIOffset cio; register PCIItem CIReg, cip;
  CIOffset CISLTemp, CISETemp;
  integer MCount;
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP
  CIReg = CI;
Restart:
  MCount = 0;
  cio = CISortedList;
  while (cio != CINULL && freed < needed) {
    cip = &CIReg [cio];
    next = cip->ageflink;
    if (cip->type != Vmem) {
      cio = next; continue;}
    if (cip->touched) {
      MCount++;
      if (MCount > MaxMCount) {
        SortCharAges ();
        MCount = 0;
        goto Restart;
        }
      cip->touched = false;
      RelinkCIItem(cio);
      cio = next;
      continue;
      }
    if (maskID >= 0 && maskID != cip->cmp->maskID) {
      cio = next;
      continue;
      }
    MIDCount[cip->mid]--;
#if STAGE==DEVELOP
    Assert(MIDCount[cip->mid] >= 0);
    Assert (cip->type != freemem);
#endif STAGE==DEVELOP
    freed += ReleaseCI(cio);
    cio = next;
    }
  return freed;
  }



public procedure PSFlushFontCache() {
  register CIOffset cio = 0;
  register PCIItem cip;
  PurgeFSCache(MIDNULL);
  forallCI(cip)
    {
    cio++;
    if (cip->type != Vmem)
      ReleaseCI(cio);
    }
  }
  
