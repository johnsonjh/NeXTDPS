/*
  midcache.c

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

Edit History:
Ed Taft: Fri Jul 29 09:36:07 1988
Ivor Durham: Sun Aug 14 14:00:13 1988
Jim Sandman: Mon May  8 16:47:47 1989
Perry Caro: Wed Nov  9 14:16:42 1988
Joe Pasqua: Tue Feb 28 13:43:07 1989
Bill Bilodeau: Fri Jan 20 16:45:41 PST 1989
Paul Rovner: Fri Sep  1 10:11:38 1989
P. Graffagnino: 5/4/90  gave TrimEldestMID() an argument
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
#include "fontdata.h"
#include "fontsnames.h"

/* globals */

#define MaxMCount 20

public PMTItem MT, MTEnd;
public integer MTSize;
public MID *MM, *MMEnd;
public integer MMSize;
public MID *MS, *MSEnd;
public integer MSSize;
public MID MTFreeHead;
public PCard32 MA;
public Card32 CurrentMIDAge;
public PCard16 IndMArray, IndSArray;
public integer *MIDCount;

private boolean IsSortedMID;
private MID LastCurMID;
private PCard16 MMEldestPtr, MSEldestPtr;

private procedure MTUnlink();
private boolean TrimEldestMID();

#if STAGE==DEVELOP
#define IsFreeMT() \
  ((mp->type == freemem) && (MIDCount[m] == 0) && (MA[m] == PeterPan))
#define IsAllocMT() (mp->type != freemem)

public CheckMT()
{
  register integer count;
  register PMTItem mp;
  register MID m;
  count = 0;
  m = 0;
  forallMT(mp)
  {
    m++;
    if (mp->type == freemem)
      { Assert(IsFreeMT()); MIDCount[m] = 1234567890; }
    else
      { Assert(IsAllocMT()); count++; }
  }
  m = MTFreeHead;
  while (m != MIDNULL)
  {
    mp = &MT[m];
    Assert(MIDCount[m] == 1234567890);
    MIDCount[m] = 0;
    Assert(IsFreeMT());
    m = mp->mlink;
  }
  m = 0;
  forallMT(mp)
  {
    m++;
    Assert(MIDCount[m] != 1234567890);
  }
  Assert(count == fcData.MIDcnt);
}

#endif STAGE==DEVELOP



#define Swap(A,B) {Temp = A; A = B; B = Temp;}

private procedure QuickSort (L, U)
  register PCard16 L, U;
{
  register PCard16 I = L;
  register PCard16 J = U;
  register MID Temp;
  register PCard32 MAreg = MA;
  register PCard16 MidIndex = (PCard16) ((((integer) L + (integer) U) >> 2) << 1);    /* isn't there a better way of doing this? */
  register Card32 MidValue = MAreg [*MidIndex];
  do
  {
    while ((MAreg [*I] <= MidValue) && (I < U)) I++;
    while ((MAreg [*J] >= MidValue) && (J > L)) J--;
    if (I < J)
    {
      Swap (*I, *J);
      I++;
      J--;
    }
  }
  while (I < J);
  if (I == J)
  {
    if ((I == L) || (J == U)) Swap (*I, *MidIndex);
    if (MAreg [*I] <= MidValue) I++;
    if (MAreg [*J] >= MidValue) J--;
  }
  if ((J - L) > 1)
    QuickSort (L, J);
  else if (((J - L) == 1) && (MAreg [*L] > MAreg [*J]))
    Swap (*L, *J);
  if ((U - I) > 1)
    QuickSort (I, U);
  else if (((U - I) == 1) && (MAreg [*I] > MAreg [*U]))
    Swap (*I, *U);
}
  
private procedure SortMIDAges ()
{
  register PCard16 IMAPtr, ISAPtr;
  register PCard32 MAReg = MA;
  register integer I;
  register PMTItem mp = &MT [1];
  IMAPtr = &IndMArray [0];
  ISAPtr = &IndSArray [0];
  *IMAPtr++ = 0;
  *ISAPtr++ = 0;
  for (I = 1; I < MTSize; I++)
  {
    if (mp->mfmid) *IMAPtr++ = I; else *ISAPtr++ = I;
    mp++;
  }
  QuickSort (&IndMArray [1], (IMAPtr-1));
  QuickSort (&IndSArray [1], (ISAPtr-1));
  MMEldestPtr = IMAPtr;
  MSEldestPtr = ISAPtr;
  LastCurMID = CurrentMIDAge;
  IsSortedMID = true;
#if STAGE < EXPORT
  {
    integer C = 0;
    for (I = 1; I < MTSize; I++) {if (MAReg [I] != PeterPan) C++;}
    while (MAReg [*--IMAPtr] != PeterPan)
    {
      Assert (MAReg [*IMAPtr] >= MAReg [*(IMAPtr-1)]);
      C--;
    }
    while (MAReg [*--ISAPtr] != PeterPan)
    {
      Assert (MAReg [*ISAPtr] >= MAReg [*(ISAPtr-1)]);
      C--;
    }
    Assert (C == 0);
  }
#endif STAGE < EXPORT
}


public MID MMEldest ()
{
  register PCard16 Ptr;
  register MID m;
  register Card32 Temp;
  Ptr = MMEldestPtr;
  while (true)
  {
    m = *--Ptr;
    Temp = MA [m];
    if (Temp == PeterPan) return MIDNULL;
    if (Temp >= LastCurMID)
    {
      MMEldestPtr = Ptr;
      IsSortedMID = false;
      return m;
    }
  }
}

public MID MSEldest ()
{
  register PCard16 Ptr;
  register MID m;
  register Card32 Temp;
repeat:
  Ptr = MSEldestPtr;
  while (true)
  {
    m = *--Ptr;
    Temp = MA [m];
    if (Temp == PeterPan)
    {
      if (IsSortedMID) return MIDNULL;
      SortMIDAges ();
      goto repeat;
    }
    if (Temp >= LastCurMID)
    {
      MSEldestPtr = Ptr;
      IsSortedMID = false;
      return m;
    }
  }
}

private boolean TrimEldestMID(amount)
  register integer amount;
  /* amount is number of items.
     note that the test for MTs is '>' and not '>=' since the
     zero-th item is never allocated but is counted in the Size variable */
{
  register MID next;
  register integer count;

  count = fcData.MIDcnt / 2;
  next = MMEldest ();
  while (next != MIDNULL && count > 0) {
    PurgeMID(next);
      /* this just frees the MID; it doesn't purge any characters */
    if ((MTSize-fcData.MIDcnt) > amount) return true;
    next = MMEldest ();
    count--;
    }
  count = fcData.MIDcnt/2;
  next = MSEldest ();
  while (next != MIDNULL && count > 0) {
    PurgeMID(next);
    if ((MTSize - fcData.MIDcnt) > amount) return true;
    next = MSEldest ();
    count--;
    }
  return false;
}


private MID MTAlloc()
/* Allocates an MID, possibly displacing an existing MID and its
   cached characters. The returned MID has type allocmem and is
   not on any lists; the caller is expected to fill it in, set its
   type appropriately, and put it on hash and aging lists. Returns
   MIDNULL if no MID can be allocated. */
{
  MID m;
  register PMTItem mp;

  if (MTFreeHead == MIDNULL && ! TrimEldestMID(1))
    return MIDNULL;
  m = MTFreeHead; mp = &MT[m];
#if STAGE==DEVELOP
  Assert(IsFreeMT());
#endif STAGE==DEVELOP
  mp->type = allocmem;
  MTFreeHead = mp->mlink;
  if (MTFreeHead != MIDNULL) Assert(MT[MTFreeHead].type == freemem);
  fcData.MIDcnt++;
#if STAGE==DEVELOP
  MA[m] = Methuselah;
  CheckMT();
#endif STAGE==DEVELOP
  return m;
}


private procedure MTFree(m, unlink)
  register MID m; boolean unlink;
/* Deallocates MID, and also unlinks it from its hash chain and aging
   list if unlink is true. */
{
  register PMTItem mp;

  mp = &MT[m];
#if STAGE==DEVELOP
  Assert(IsAllocMT());
#endif STAGE==DEVELOP
  DebugAssert((mp->type != freemem) && (MIDCount[m] == 0));
  MA [m] = PeterPan;
  mp->umid.rep.generation++;  /* invalidate all refs to this MID */
  if (unlink) MTUnlink(m);
  mp->type = freemem;
  mp->mlink = MTFreeHead;
  MTFreeHead = m;
  fcData.MIDcnt--;
#if STAGE==DEVELOP
  CheckMT();
#endif STAGE==DEVELOP
}


private procedure MTUnlink(m)
  register MID m;
/* Unlinks MID from its hash chain */
{
  register PMTItem mp = &MT[m];
  if (mp->mfmid){
    Assert(MM[mp->hash] != MIDNULL);
    if (MM[mp->hash] == m)
      {
      MM[mp->hash] = mp->mlink; return;}
    else {mp = &MT[MM[mp->hash]];}}
  else{
    Assert(MS[mp->hash] != MIDNULL);
    if (MS[mp->hash] == m)
      {MS[mp->hash] = mp->mlink; return;}
    else {mp = &MT[MS[mp->hash]];}}
  while (mp->mlink != MIDNULL){
    if (mp->mlink == m){
      mp->mlink = MT[m].mlink; return;}
    mp = &MT[mp->mlink];}
  CantHappen();
}


public Card16 HashMID(fid, mtx, maskID, tblSize)
  FID fid; register PMtx mtx; DevShort maskID; Card16 tblSize;
/* Note: MIDToFile uses this procedure to hash a data structure on
   the disk; thus, this algorithm is effectively cast in concrete. */
{
  register Card32 h;
  real r;
  h = fid;
  r = mtx->a * fp1024; RRoundP(&r, &r); h ^= ((integer)r)<<16;
  r = mtx->b * fp1024; RRoundP(&r, &r); h ^= ((integer)r)<<12;
  r = mtx->c * fp1024; RRoundP(&r, &r); h ^= ((integer)r)<<8;
  r = mtx->d * fp1024; RRoundP(&r, &r); h ^= ((integer)r)<<4;
  h = h * 0x41c64e6d + maskID;
  h = (Card16) ((h >> 16) ^ h) * tblSize;
  return (Card16) (h >> 16);
}


public MID InsertMID(fid, cmtx, d, maskID, mcomp, mf)
  FID fid; PMtx cmtx; DictObj d; DevShort maskID;
  boolean (*mcomp)();
  register boolean mf;
{
  register MID m;
  register PMTItem mp;
  Card16 hash;
  PVMRoot root = (d.shared ? rootShared : rootPrivate);

  hash = HashMID(fid, cmtx, maskID, (Card16) MMSize);
    /* Note that MMSize = MSSize */
  m = (mf)? MM[hash] : MS[hash];
  while (m != MIDNULL)
    {
    mp = &MT[m];
    if (mp->fid == fid &&
        (*mcomp)(&(mp->mtx), cmtx) &&
        (mf ?
	 ( /* compare remaining parts of makefont key */
          mp->values.mf.origdict.val.dictval == d.val.dictval &&
          mp->values.mf.dictSpace.stamp == root->spaceID.stamp) :
	 ( /* compare remaining parts of setfont key */
	  mp->maskID == maskID)))
      {
      DecrSetMIDAge(m);
      return m;
      }
    m = mp->mlink;
    }

  /* no hit; create new MID */
  m = MTAlloc();
  if (m == MIDNULL) return MIDNULL;
  mp = &MT[m];
  mp->hash = hash;
  mp->fid = fid;
  mp->mtx = *cmtx;
  mp->type = Vmem;
  mp->mfmid = mf;
  if (mf)
    {
    mp->mlink = MM[hash]; MM[hash] = m;
    /* Initialize to null values. This MID won't be found in any space
       until caller assigns real dicts and sets dictSpace. */
    mp->values.mf.dictSpace.stamp = 0;
    mp->values.mf.mfdict = NOLL;
    mp->values.mf.origdict = NOLL;
    }
  else
    {
    mp->mlink = MS[hash]; MS[hash] = m;
    mp->maskID = maskID;
    mp->values.sf.bb.ll.x = mp->values.sf.bb.ur.x = 0;
    mp->values.sf.bb.ll.y = mp->values.sf.bb.ur.y = 0;
    mp->values.sf.bb1.ll.x = mp->values.sf.bb1.ur.x = 0;
    mp->values.sf.bb1.ll.y = mp->values.sf.bb1.ur.y = 0;
    mp->monotonic = true;
    mp->xinc = false; mp->yinc = false;
    mp->xdec = false; mp->ydec = false;
    mp->monotonic1 = true;
    mp->xinc1 = false; mp->yinc1 = false;
    mp->xdec1 = false; mp->ydec1 = false;
    mp->shownchar = false;
    }
  SetMIDAge (m); /* don't link until mfmid is set */
  return m;
}  /* end of InsertMID */


public MID MakeMID(d, curmtx, maskID)
  DictObj d; PMtx curmtx; DevShort maskID;
{
  FID fid;
  Mtx t0mtx, cmtx;
  AryObj cob;
  DictObj mdict;
  Object ob;
  register PMTItem mp;

  if ((d.type != dictObj) || TrickyDict(&d)) InvlFont();
  if (!d.val.dictval->isfont) InvlFont();
  DictGetP(d, fontsNames[nm_FID], &ob);
  if (ob.type != fontObj) InvlFont();
  fid = ob.val.fontval;
  t0mtx = *curmtx;
  t0mtx.tx = t0mtx.ty = 0;  /* translation-free CTM */
  if (Known(d, fontsNames[nm_ScaleMatrix]))
    {
    DictGetP(d, fontsNames[nm_ScaleMatrix], &cob);
    PAryToMtx(&cob, &cmtx);
    MtxCnct(&cmtx, &t0mtx, &cmtx);
    }
  else cmtx = t0mtx;
  if (Known(d, fontsNames[nm_OrigFont]))
    DictGetP(d, fontsNames[nm_OrigFont], &mdict);
  else mdict = d;
  return InsertMID(fid, &cmtx, mdict, maskID, MtxEqAlmost, false);
}  /* end of MakeMID */


public procedure PurgeMID(mid)
  register MID mid;
{
  register PMTItem mp = &MT[mid];
  register CIOffset cio = 0;
  register PCIItem cip;

  Assert((mp->type & Vmem) != 0);

  /* only setfont MIDs have chars to purge; just free makefont MIDs */
  if (mp->mfmid)
    {
    PurgeSFForMID(mid);
    MTFree(mid, true);
    return;
    }

  PurgeFSCache(mid); /* purge FastShow cache */
  DeleteCIs(mid);
  MTFree(mid, true);
#if STAGE==DEVELOP
  CheckMT();
#endif STAGE==DEVELOP
}

private procedure PurgeMM(root, wantToPurge, arg)
  PVMRoot root;
  boolean (*wantToPurge)(/* PMTItem mp, char *arg */);
  char *arg;
/* Purges all makefont MIDs that refer to fonts in the specified space
   and for which wantToPurge(mp, arg) returns true. */
{
  register MID m, pm, *map;
  register PMTItem mp;
  register Card32 stamp = root->spaceID.stamp;

  forallMM(map)
    {
    m = *map; pm = MIDNULL;
    while (m != MIDNULL)
      {
      mp = &MT[m];
      DebugAssert(mp->mfmid);
      if (mp->values.mf.dictSpace.stamp == stamp &&
          (*wantToPurge)(mp, arg))
	{
	MID fm = m;
	m = mp->mlink;
	PurgeSFForMID(fm);
	if (pm == MIDNULL) *map = m;
	else MT[pm].mlink = m;
	MTFree(fm, false);
	}
      else {pm = m; m = mp->mlink;}
      }
    }
}


private boolean MIDPurgedByRestore(mp, arg)
  PMTItem mp; char *arg;
{
  DebugAssert(mp->values.mf.mfdict.type == dictObj &&
	      mp->values.mf.mfdict.level >= mp->values.mf.origdict.level);
  return mp->values.mf.mfdict.level > *(integer *)arg;
}


public procedure PurgeOnRstr(lev)
  Level lev;
/* Called on a restore to purge font cache of makefont MIDs for font
   dictionaries that are being reclaimed. */
{
  integer level = lev;
  PurgeMM(rootPrivate, MIDPurgedByRestore, (char *) &level);
}


private boolean MIDPurgedByGC(mp, arg)
  PMTItem mp; char *arg;
{
#if STAGE==DEVELOP
  boolean purged;
  Assert(mp->values.mf.mfdict.type == dictObj);
  purged = GC_WasCollected(&mp->values.mf.mfdict, (GC_Info) arg);
  if (! purged)
    /* if mfdict was seen, origdict must have been seen also, since
       mfdict contains a reference to origdict */
    Assert(mp->values.mf.origdict.shared ||
           ! GC_WasCollected(&mp->values.mf.origdict, (GC_Info) arg));
  return purged;
#else STAGE==DEVELOP
  return GC_WasCollected(&mp->values.mf.mfdict, (GC_Info) arg);
#endif STAGE==DEVELOP
}


public procedure PurgeOnGC(clientData, info)
  RefAny clientData; GC_Info info;
/* Called at the end of a collection to purge font cache of makefont MIDs
   for font dictionaries that are being reclaimed. */
{
  PVMRoot root;

  root = GC_GetRoot(info);
  PurgeMM(root, MIDPurgedByGC, (char *) info);
}


public procedure InitSortMID () {
#if STAGE==DEVELOP
  Assert (MA [0] == PeterPan);
#endif STAGE==DEVELOP
  MMEldestPtr = &IndMArray [1];
  MSEldestPtr = &IndSArray [1];
  IndMArray [0] = IndSArray [0] = 0;
  IsSortedMID = false;
  LastCurMID = Methuselah;
  }

