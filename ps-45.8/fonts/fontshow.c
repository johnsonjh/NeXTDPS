 /*
  fontshow.c

Copyright (c) 1984-1990 Adobe Systems Incorporated.
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
Ed Taft: Tue Jan 16 12:40:22 1990
Jim Sandman: Thu Mar 22 13:35:35 1990
Bill Paxton: Tue Apr  4 10:33:17 1989
Ivor Durham: Fri Sep 23 15:40:09 1988
Paul Rovner: Tuesday, October 25, 1988 1:04:28 PM
Perry Caro: Wed Nov  9 14:23:21 1988
Joe Pasqua: Thu Jan 19 15:24:10 1989
Rick Saenz: Thu Mar 22 12:00:00 1990
Chuck Jordan: Fri Mar 30 09:41:16 1990
pgraff@next 4/5/90 	in GetFontAndCharInfo() make mtx translation-free when
		    	ValidMID() fails.
pgraff@next 10/16/90	hacked out assert to bypass atof() bug in assembler
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include DEVICE
#include ERROR
#include EXCEPT
#include FP
#include GRAPHICS
#include LANGUAGE
#include ORPHANS
#include PSLIB
#include STREAM
#include VM

#include "diskcache.h"
#include "fontshow.h"
#include "fontdata.h"
#include "fontcache.h"
#include "fontsnames.h"
#include "fontbuild.h"

#define PccCount (fontCtx->fontBuild._PccCount)


/* The following procedures are extern'd here since	*/
/* they're not exported by the language/grfx package.	*/
extern procedure StrToName(), NameToPString();
extern procedure NullDevice(), CheckForCurrentPoint();

private procedure ShowInternal(), SlowShow(), CleanShowState();

public boolean ReValidateMID()
{
  register PMTItem mp;
  MID mid;

  mp = &MT[gs->matrixID.rep.mid];
  if (mp->umid.stamp != gs->matrixID.stamp ||
      mp->maskID != gs->device->maskID)
    { /* Non-current MID, or MIDNULL; must recompute MID */
    mid = MakeMID(gs->fontDict, &gs->matrix, gs->device->maskID);
    if (mid == MIDNULL)
      {
      gs->matrixID.stamp = MIDNULL;
      curMT = NIL;
      return false;
      }
    mp = &MT[mid];
    DebugAssert(mid == mp->umid.rep.mid && ! mp->mfmid);
    gs->matrixID = mp->umid;
    }
  curMT = mp;
  return true;
}


private boolean MFMcomp(amtx, bmtx)
  register PMtx amtx, bmtx;
{
  return ((amtx->a == bmtx->a) && (amtx->b == bmtx->b) &&
      (amtx->c == bmtx->c) && (amtx->d == bmtx->d) &&
      (amtx->tx == bmtx->tx) && (amtx->ty == bmtx->ty));
}


public MID MakeFontSupp(old,mtx,new,ar,pnewmid)
  PMtx mtx; DictObj old; PDictObj new; Object ar; boolean *pnewmid;
{
  AryObj cob, fm, newfm, newsm;
  DictObj mdict;
  MID mid;
  Mtx cmtx, fmtx;
  FontObj fo;
  PVMRoot root;

  root = (old.shared ? rootShared : rootPrivate);
  DictGetP(old, fontsNames[nm_FID], &fo);
  if (fo.type != fontObj) InvlFont();
  DictGetP(old, fontsNames[nm_FontMatrix], &fm);
  PAryToMtx(&fm, &fmtx);
  MtxCnct(&fmtx, mtx, &fmtx);
  if (!DictTestP(old, fontsNames[nm_OrigFont], &mdict, true))
    mdict = old;
  if (DictTestP(old, fontsNames[nm_ScaleMatrix], &cob, true))
    {
    PAryToMtx(&cob, &cmtx);
    MtxCnct(&cmtx, mtx, &cmtx);
    }
  else cmtx = *mtx;
  mid = InsertMID(
    (FID)fo.val.fontval, &cmtx, mdict, (DevShort) 0, MFMcomp, true);
  if (mid == MIDNULL ||
      MT[mid].values.mf.mfdict.type != dictObj) /* i.e., newly allocated */
    {
    if (ar.type == nullObj || ar.shared != old.shared) {
      boolean origShared = CurrentShared();
      SetShared(old.shared);
      DURING
        AllocCopyDict(
	  old, (integer)((mdict.val.dictval == old.val.dictval)? 2 : 0), new);
	  /* new may be larger than old */
        AllocPArray(fm.length, &newfm);
        ConditionalInvalidateRecycler(&newfm); /* Preclude move by ForcePut */
        AllocPArray(fm.length, &newsm);
        ConditionalInvalidateRecycler(&newsm); /* Preclude move by ForcePut */
      HANDLER
	SetShared(origShared); RERAISE;
	/* This might leave a partially filled-in MID lying around, but
	   nobody will ever hit it because its spaceID isn't set. */
      END_HANDLER;
      SetShared(origShared);
      }
    else {
      *new = VMGetElem (ar, 1);
      ReallocDict (old, new);
      newfm = VMGetElem (ar, 2);
      newsm = VMGetElem (ar, 3);
      }
    new->val.dictval->isfont = true;
    MtxToPAry(&fmtx, &newfm);
    ForcePut(*new, fontsNames[nm_FontMatrix], newfm);
    MtxToPAry(&cmtx, &newsm);
    ForcePut(*new, fontsNames[nm_ScaleMatrix], newsm);
    ForcePut(*new, fontsNames[nm_OrigFont], mdict);
    if (mid != MIDNULL)
      {
      MT[mid].values.mf.origdict = mdict;
      MT[mid].values.mf.mfdict = *new;
      MT[mid].values.mf.dictSpace = root->spaceID;
      }
    *pnewmid = true;
    }
  else {
    *new = MT[mid].values.mf.mfdict;
    *pnewmid = false;
    }
  return mid;
}  /* end of MakeFontSupp */


private procedure MakeCompFont();

public MID MakeFont(old, mtx, new)
  DictObj old; PMtx mtx; PDictObj new; {
  boolean newmid;
  IntObj fntype;

  if (TrickyDict (&old) || ! old.val.dictval->isfont) InvlFont();
  DictGetP(old, fontsNames[nm_FontType], &fntype);
  if (fntype.val.ival == COMPOSEDtype) {
    MakeCompFont(old, mtx, new);
    return MIDNULL;
    }
  return MakeFontSupp (old, mtx, new, NOLL, &newmid);
  }  /* end of MakeFont */

private procedure MakeCompFont(old, mtx, new)
  DictObj old; PMtx mtx; PDictObj new; {
  IntObj fntype;
  boolean origShared = CurrentShared();
  integer i;
  PCard8 newMTPE, oldMTPE;
  AryObj fm, newfm;
  MID mid;
  Mtx fmtx;
  FontObj fo;
  StrObj oldstring, midVecStr, curMIDStr;
  AryObj oldarray, newarray;
  UniqueMID *NPtr;
  PVMRoot root;

  root = (old.shared ? rootShared : rootPrivate);
  DictGetP(old, fontsNames[nm_FID], &fo);
  if (fo.type != fontObj) InvlFont();
  mid = InsertMID(
    (FID)fo.val.fontval, mtx, old, (DevShort) 0, MFMcomp, true);
  if (mid == MIDNULL ||
      MT[mid].values.mf.mfdict.type != dictObj) /* i.e., newly allocated */
    {
    DictGetP (old, fontsNames[nm_FontMatrix], &fm);
    SetShared(old.shared);
    DURING
      AllocCopyDict(old, 0, new);
	/* new may be larger than old */
      new->val.dictval->isfont = true;
      DictGetP (old, fontsNames[nm_FDepVector], &oldarray);
      if (oldarray.length == 0)
        newarray = oldarray;
      else {
        AllocPArray (oldarray.length, &newarray);
        ConditionalInvalidateRecycler(&newarray);
        }
      DictGetP (old, fontsNames[nm_MIDVector], &oldstring);
      if (oldstring.length == 0)
        midVecStr = oldstring;
      else {
        LStrObj(midVecStr, oldstring.length, AllocAligned (oldstring.length));
        ConditionalInvalidateRecycler(&midVecStr);
        }
      LStrObj(curMIDStr, sizeof(UniqueMID), AllocAligned (sizeof(UniqueMID)));
      ConditionalInvalidateRecycler(&curMIDStr);
      AllocPArray (fm.length, &newfm);
      ConditionalInvalidateRecycler(&newfm);
    HANDLER
      SetShared(origShared); RERAISE;
      /* This might leave a partially filled-in MID lying around, but
	 nobody will ever hit it because its spaceID isn't set. */
    END_HANDLER;
    SetShared(origShared);
    oldMTPE =
      (PCard8)oldstring.val.strval + (oldarray.length * sizeof(UniqueMID));
    newMTPE =
      (PCard8)midVecStr.val.strval + (oldarray.length * sizeof(UniqueMID));
    os_bcopy(oldMTPE, newMTPE, (long int)BytesForMTPE(oldarray.length));
    ForcePut (*new, fontsNames[nm_FDepVector], newarray);
    midVecStr.access = nAccess;
    ForcePut (*new, fontsNames[nm_MIDVector], midVecStr);
    curMIDStr.access = nAccess;
    ((UniqueMID*) curMIDStr.val.strval)->stamp = MIDNULL;
    ForcePut (*new, fontsNames[nm_CurMID], curMIDStr);
    PAryToMtx (&fm, &fmtx);
    MtxCnct (&fmtx, mtx, &fmtx);
    MtxToPAry (&fmtx, &newfm);
    ForcePut (*new, fontsNames[nm_FontMatrix], newfm);
    if (mid != MIDNULL)
      {
      MT[mid].values.mf.origdict = old;
      MT[mid].values.mf.mfdict = *new;
      MT[mid].values.mf.dictSpace = root->spaceID;
      }
    NPtr = (UniqueMID *) midVecStr.val.strval;
    for (i = 0; i < oldarray.length; i++) {
      Object elem;
      NPtr->stamp = MIDNULL;
      NPtr++;
      elem = VMGetElem (oldarray, i);
      DictGetP (elem, fontsNames[nm_FontType], &fntype);
      if (fntype.val.ival == COMPOSEDtype) MakeFont (elem, mtx, &elem);
      VMPutElem (newarray, i, elem);
      }
    }
  else
    *new = MT[mid].values.mf.mfdict;
  return;
  }


public procedure PSMakeFont()
{
  Mtx mtx;
  DictObj fdict;
  PopMtx(&mtx);
  PopPDict(&fdict);
  (void) MakeFont(fdict, &mtx, &fdict);
  PushP(&fdict);
}  /* end of PSMakeFont */


public procedure PSScaleFont()
{
  Mtx mtx;
  DictObj fdict;
  PopPReal(&mtx.a);
  PopPDict(&fdict);
  mtx.d = mtx.a;
  mtx.b = mtx.c = mtx.tx = mtx.ty = fpZero;
  (void) MakeFont(fdict, &mtx, &fdict);
  PushP(&fdict);
}  /* end of PSScaleFont */


public procedure PSSelectFont()
{
  Object key, tfm;
  DictObj dict, fontDir;
  Mtx mtx;
  boolean isScaleFont, shared, insert;
  MID mid;

  PopP(&tfm);
  PopP(&key);
  switch (tfm.type)
    {
    case intObj:
      mtx.a = tfm.val.ival;
      isScaleFont = true;
      break;

    case realObj:
      mtx.a = tfm.val.rval;
      isScaleFont = true;
      break;

    case arrayObj:
    case pkdaryObj:
      PAryToMtx(&tfm, &mtx);
      isScaleFont = false;
      break;

    default:
      TypeCheck();
    }
  if (key.type == strObj) StrToName(key, &key);

  if (! SearchSelectFont(key, &mtx, isScaleFont, &dict))
    {
    /* Look first in the private FD, then in shared FD (but do not use
       a font from private FD if we are in shared VM mode).
       Set shared to reflect the directory in which key was found.
       Set insert to true if key was found in private FD or if
       it was found in shared FD and not also in private FD.
       If not found in either place, execute "findfont" and do not
       cache the result since we don't know which FD it was defined in.
     */
    insert = true;  /* assume we will cache it */
    shared = false;  /* assume it is not shared */
    GetPrivFontDirectory(&fontDir);
    if ((Known(fontDir, key) && (insert = ! vmCurrent->shared))
#if (LANGUAGE_LEVEL >= level_2)
     || (vLANGUAGE_LEVEL >= level_2 &&
       (fontDir = rootShared->vm.Shared.sharedFontDirectory,
        shared = Known(fontDir, key)))
#endif
      )
      DictGetP(fontDir, key, &dict);
    else
      {
      insert = false;
      PushP(&key);
      if (psExecute(fontsNames[nm_findfont])) {
        PushP(&tfm);
        return;
        }
      PopPDict(&dict);
      }
    if (isScaleFont) {mtx.d = mtx.a; mtx.b = mtx.c = mtx.tx = mtx.ty = 0;}
    mid = MakeFont(dict, &mtx, &dict);
    if (insert && (mid != MIDNULL)) InsertSelectFont(key, &mtx, shared, mid);
    }

  SetFont(dict);
}

private integer MinEncodingLength(fdict) DictObj fdict; {
  IntObj fntype;
  AryObj fvec;
  integer i, min, temp;
  min = 256;
  DictGetP (fdict, fontsNames[nm_FontType], &fntype);
  if (fntype.val.ival == COMPOSEDtype) {
    DictGetP (fdict, fontsNames[nm_FDepVector], &fvec);
    for (i = 0; i < fvec.length; i++) {
      temp = MinEncodingLength (VMGetElem (fvec, i));
      if (temp < min) min = temp;
      }
    }
  else {
    DictGetP (fdict, fontsNames[nm_Encoding], &fvec);
    return fvec.length;
    }
  return min;
  }

public procedure SetFont(fdict)
  DictObj fdict;
/* This procedure is exported to the graphics interface */
{
  IntObj io;
  StrObj s;
  if (fdict.val.dictval == gs->fontDict.val.dictval) return;
  if (TrickyDict (&fdict) || ! fdict.val.dictval->isfont) InvlFont();
  gs->matrixID.stamp = MIDNULL;
  gs->fontDict = fdict;
  DictGetP(gs->fontDict, fontsNames[nm_Encoding], &gs->encAry);
  gs->pfontDict = fdict;
  if (DictTestP(fdict, fontsNames[nm_WMode], &io, true))
    gs->writingmode = io.val.ival;
  else
    gs->writingmode = 0;
  DictGetP (fdict, fontsNames[nm_FontType], &io);
  if (io.val.ival == COMPOSEDtype)
    {
    gs->escapechar = 256;
    DictGetP (fdict, fontsNames[nm_FMapType], &io);
    gs->fmap = io.val.ival;
    switch (io.val.ival)
      {
      case FMap17:
        break;
      case FMapGen:
        DictGetP (fdict, fontsNames[nm_SubsVector], &gs->svary);
        break;
      case FMap88:
        break;
      case FMap97:
        break;
      case FMapEscape:
        DictGetP (fdict, fontsNames[nm_EscChar], &io);
        gs->escapechar = io.val.ival;
        break;
      default: CantHappen ();
      }
      DictGetP (fdict, fontsNames[nm_FDepVector], &gs->fdary);
      DictGetP (fdict, fontsNames[nm_MIDVector], &s);
      gs->miary = (UniqueMID *) s.val.strval;
      DictGetP (fdict, fontsNames[nm_PrefEnc], &gs->peary);
      DictGetP (fdict, fontsNames[nm_CurMID], &s);
#if 0  /* until gs.compfontMID type changed to UniqueMID* */
      gs->compfontMID = ((UniqueMID*) s.val.strval);
#else
      gs->compfontMID.stamp = ((Card32) s.val.strval);
#endif
      gs->minEncodingLen = MinEncodingLength(fdict);
    }
  else {
    gs->fmap = FMapNone;
    gs->minEncodingLen = gs->encAry.length;
    }
}


public procedure PSSetFont()
{
  DictObj dict;
  PopPDict(&dict);
  SetFont(dict);
}


public procedure PSCrFont()
{
  PushP(&gs->fontDict);
}

public procedure PSRootFont() {PushP(&gs->pfontDict);}

private procedure GetFontAndCharInfo(pcn, name, nameLength, font, mtx)
  Object *pcn; char **name; integer *nameLength, *font; PMtx mtx; {
  if (ValidMID())
    {
    *font = curMT->fid;
    *mtx = curMT->mtx;
    }
  else {
    Object ob;
    Mtx cmtx;
    DictGetP(gs->fontDict, fontsNames[nm_FID], &ob);
    *font = ob.val.fontval;
    cmtx = gs->matrix;
    cmtx.tx = cmtx.ty = 0; /* make translation-free (pgraff@NeXT 5/4/90) */
    if (DictTestP(gs->fontDict, fontsNames[nm_ScaleMatrix], &ob, true))
      {
      Mtx smtx;
      PAryToMtx(&ob, &smtx);
      MtxCnct(&smtx, &cmtx, &cmtx);
      }
    *mtx = cmtx;
    }
  if (pcn->type != nameObj) {
    *name = (char *) NIL;
    *nameLength = 0;
    }
  else {
    *name = (char *) pcn->val.nmval->str;
    *nameLength = pcn->val.nmval->strLen;
    }
  }




/*
  ShowState management

  Macros for ShowState setup and save/restore. When no "show" is in progress,
  showLevel == 0 and ss is either NIL or has a ShowState left over from
  some past "show"; BEGINSHOW allocates one if necessary.

  However, when BuildChar or psExecute is called during a "show",
  showLevel is incremented. If "show" is then invoked recursively (i.e.,
  while showLevel > 0), a new ShowState must be assigned for the duration
  of the "show" and the old one reinstated at the end.

  If an exception occurs during a "show" at the top level, it is
  not caught; the allocated ShowState is simply left lying around
  for use by the next "show". If an exception occurs during a recursive
  "show", the caller of BuildChar or psExecute gets control,
  decrements showLevel, frees the nested ShowState if there is one,
  and reraises the exception.

  The purpose of this arrangement is to minimize the setup overhead
  required for a top-level "show"; in particular, a DURING...HANDLER
  is not required, and a free ShowState is usually available.

  BEGINSHOW begins a new scope containing a register variable
  PShowState ssr, which is a copy of the newly allocated ss.
*/

#define BEGINSHOW { \
  register PShowState ssr; \
  if (showLevel > 0) ssr = PushShowState(); \
  else { \
    if (ss != NIL) TrimShowStates(); \
    if ((ssr = ssFree) != NIL) {ssFree = ssr->link; ssr->link = NIL;} \
    else ssr = NewShowState(); \
    ss = ssr;}

#define ENDSHOW \
  DebugAssert(ssr == ss && IsCleanShowState(ssr)); \
  if (showLevel > 0) PopShowState(); \
  else { \
    DebugAssert(ssr->level == 0 && ssr->link == NIL); \
    ss = NIL; \
    if (ssr->temporary) FreeShowState(ssr); \
    else { \
      os_bzero((char *) ssr, (integer) sizeof(ShowState)); \
      ssr->link = ssFree; ssFree = ssr;}} \
  }  /* this ends scope begun by BEGINSHOW */

#define IsCleanShowState(ssr) \
  (ssr->cio == CINULL)


private PShowState ssFree;  /* global free list */
#if STAGE==DEVELOP
private integer ssCount;  /* total number of ShowStates in existence */
#endif STAGE==DEVELOP

#define NSHOWSTATES 4  /* pool of permanently allocated ShowStates */


private PShowState NewShowState()
{
  register PShowState ssr;
  ssr = (PShowState) NEW(1, (integer) sizeof(ShowState));
#if STAGE==DEVELOP
  ssCount++;
#endif STAGE==DEVELOP
  ssr->temporary = true;
  return ssr;
}


private procedure FreeShowState(ssr)
  register PShowState ssr;
{
  DebugAssert(IsCleanShowState(ssr));
  if (ssr->delayedAry.type == arrayObj) {
    PVMRoot root = ssr->delayedAry.shared ? rootShared : rootPrivate;
    VMPutElem(ssr->delayedAry, 0, root->trickyDicts.val.arrayval[tdDummy]);
    VMPutElem(root->trickyDicts, tdDummy, ssr->delayedAry);
    ssr->delayedAry = NOLL;
    }
  if (ssr->temporary)
    {
    os_free((char *) ssr);
#if STAGE==DEVELOP
    ssCount--;
#endif STAGE==DEVELOP
    }
  else
    {
    os_bzero((char *) ssr, (integer) sizeof(ShowState));
    ssr->link = ssFree; ssFree = ssr;
    }
}


private procedure TrimShowStates()
/* Discards ShowStates down to and including the current showLevel */
{
  register PShowState ssr;
  while ((ssr = ss) != NIL && ssr->level >= showLevel)
    {
    ss = ssr->link;
    FreeShowState(ssr);
    }
}


private PShowState PushShowState()
{
  register PShowState ssr;
  TrimShowStates();
  if ((ssr = ssFree) != NIL) ssFree = ssr->link;
  else ssr = NewShowState();
  ssr->link = ss;
  ssr->level = showLevel;
  ss = ssr;
  return ssr;
}


private procedure PopShowState()
{
  register PShowState ssr = ss;
  DebugAssert(ssr != NIL && showLevel == ssr->level);
  ss = ssr->link;
  FreeShowState(ssr);
}


private procedure CleanShowState(ssr) register PShowState ssr;
/* Frees resources (CI) held by the current ShowState */
{
  if (ssr->cio != CINULL)
    {
    CIFree(ssr->cio);
    ssr->cio = CINULL;
    ssr->mask = NULL;
    }
}


public procedure SimpleShow(so)
	StrObj so;
{
  BEGINSHOW;
  ssr->so = so;
  ShowInternal();
  ENDSHOW;
}


public boolean OkFixed(pfcd,lower,upper)
      register DevCd *pfcd; integer lower,upper;{
  register Fixed f;
  f = FIXINT(lower+1);
  if ((pfcd->x < f) || (pfcd->y < f)) return false;
  f = FIXINT(upper-1);
  if ((pfcd->x > f) || (pfcd->y > f)) return false;
  return true;
}


private procedure ShowInternal()
{
register PShowState ssr = ss; integer len;
register PGState gsr = gs;
ssr->rdcp = gsr->cp;
if ((ssr->so.access & rAccess) == 0) InvlAccess();
if (!ssr->noShow) CheckForCurrentPoint(&gsr->path);
if ((len = ssr->so.length) == 0) return;
if (ValidMID()) DecrSetMIDAge(crMID);
ssr->encAry = gsr->encAry;
ssr->fontDict = gsr->fontDict;
ssr->fmap = gsr->fmap;
switch (ssr->fmap)
{
case FMapNone:
  if (gsr->minEncodingLen != 256) {
    register integer minLen = gsr->minEncodingLen;
    register integer i;
    register PCard8 p = ssr->so.val.strval;
    for (i = len;i; i--) {
      if (*p++ >= minLen )
        PSRangeCheck();
      }
    }
  if (ssr->xShow) {
    if ((ssr->yShow && ssr->ns.len < len * 2) || ssr->ns.len < len)
      PSRangeCheck();
    }
  else if (ssr->yShow && ssr->ns.len < len)
    PSRangeCheck();
  break;
case FMapGen:
  ssr->svary = gsr->svary;
case FMap17:
case FMap88:
case FMap97:
case FMapEscape:
  ssr->fdary = gsr->fdary;
  ssr->miary = gsr->miary;
  ssr->peary = gsr->peary;
#if 0  /* until gs.compfontMID type changed to UniqueMID* XXX */
  if (curMT != NIL && curMT->umid.stamp != gsr->compfontMID->stamp)
#else
  if (curMT != NIL && curMT->umid.stamp != ((UniqueMID*)gsr->compfontMID.stamp)->stamp)
#endif
    InvalidateCachedMIDs (gsr->fontDict, gsr->miary, gsr->fdary);
  if (gsr->minEncodingLen != 256 || ssr->xShow || ssr->yShow) {
    len = ScanCompString();
    if (ssr->xShow) {
      if ((ssr->yShow && ssr->ns.len < len * 2) || ssr->ns.len < len)
	PSRangeCheck();
      }
    else if (ssr->yShow && ssr->ns.len < len)
      PSRangeCheck();
    }
  break;
}
if (!CvtToXFixed(&(ssr->fdcp), ssr->rdcp, LOWERSHOWBOUND, UPPERSHOWBOUND))
  goto realslow;
/* make sure fdcp and rdcp agree */
FIXTOPFLT(ssr->fdcp.x, &ssr->rdcp.x);
FIXTOPFLT(ssr->fdcp.y, &ssr->rdcp.y);
if (ssr->aShow) {
  if (!CvtToXFixed(&(ssr->fdA), ssr->rdA, LOWERSHOWBOUND, UPPERSHOWBOUND))
    goto realslow;
  FIXTOPFLT(ssr->fdA.x, &ssr->rdA.x);
  FIXTOPFLT(ssr->fdA.y, &ssr->rdA.y);
  }
if (ssr->wShow) {
  if (!CvtToXFixed(&(ssr->fdW), ssr->rdW, LOWERSHOWBOUND, UPPERSHOWBOUND))
    goto realslow;
  FIXTOPFLT(ssr->fdW.x, &ssr->rdW.x);
  FIXTOPFLT(ssr->fdW.y, &ssr->rdW.y);
  }
/* we have at least a shot at doing all the computations in fixed point */
if (ssr->kShow) goto realslow;
if (curMT == NIL) goto doslow;
if (gsr->isCharPath) goto doslow;
if (len > sizeShowCache) goto doslow;
if ((gsr->writingmode == 0) ? !curMT->monotonic : !curMT->monotonic1)
  goto doslow;
if (ssr->cShow) goto doslow;
if (PccCount != 0) goto doslow;
#if (DPSXA == 0)
if (ssr->fmap == FMapNone) {
  if (FastShow((boolean)!ssr->noShow) < 0)
    goto doslow;		/* missed cache or need reals */
  }
else {
  if (ssr->wShow) goto doslow;
  if (CompositeShow((boolean)!ssr->noShow) < 0)
    goto doslow;		/* missed cache or need reals */
  }
if (ssr->noShow)
  UNFIXCD(ssr->fdcp, &ssr->rdcp); /* result for stringwidth */
goto done;
realslow:
  ssr->useReal = true;
#endif /* DPSXA */
doslow:
  if (ssr->fmap == FMapNone) SlowShow(); /* always updates ss->rdcp */
  else CompSlowShow();
done: ;
}  /* end of ShowInternal */

/* XXX */ /* integer showTime, tshowTime;     ...removed */

public procedure PSShow()
{
/*tshowTime = os_clock();                     ...removed */
  BEGINSHOW;
  PopPString(&ssr->so);
  ShowInternal();
  ENDSHOW;
/*showTime += os_clock() - tshowTime;         ...removed */
}  /* end of PSShow */

public procedure XYShow(x, y) boolean x, y;
{
  Object ob;
  BEGINSHOW;
  PopP(&ob);
  PopPString(&ssr->so);
  SetupNumStrm(&ob, &ssr->initns);
  ssr->ns = ssr->initns;
  ssr->xShow = x;
  ssr->yShow = y;
  ShowInternal();
  ENDSHOW;
}  /* end of PSShow */

public procedure PSXYShow() { XYShow(true, true); }
public procedure PSYShow()  { XYShow(false, true); }
public procedure PSXShow()  { XYShow(true, false); }

public procedure PSStrWidth()
{
  Cd cd;
  BEGINSHOW;
  PopPString(&ssr->so);
  ssr->noShow = true;
  ShowInternal();
  VecSub(ssr->rdcp, gs->cp, &cd);
  IDTfmP(cd, &cd);
  PushPCd(&cd);
  ENDSHOW;
}  /* end of PSStrWidth */

public procedure PSWidthShow()
{
  integer wC;  Cd cd;
  BEGINSHOW;
  PopPString(&ssr->so);
  wC = PopInteger();
  if ((gs->fmap == FMapNone) && ((wC < 0) || (wC > 255)))
    RangeCheck();
  ssr->wChar = wC;
  PopPCd(&cd);
  DTfmP(cd, &ssr->rdW);
  ssr->wShow = true;
  ShowInternal();
  ENDSHOW;
}  /* end of PSWidthShow */

public procedure PSAShow()
{
  Cd cd;
  BEGINSHOW;
  PopPString(&ssr->so);
  PopPCd(&cd);
  DTfmP(cd, &ssr->rdA);
  ssr->aShow = true;
  ShowInternal();
  ENDSHOW;
}  /* end of PSAShow */

public procedure PSKShow()
{
  BEGINSHOW;
  if (gs->fmap != FMapNone) InvlFont ();
  PopPString(&ssr->so);
  PopPArray(&ssr->ko);
  ssr->kShow = true;
  ShowInternal();
  ENDSHOW;
}  /* end of PSKShow */

public procedure PSCShow()
{
  BEGINSHOW;
  PopPString(&ss->so);
  PopPArray(&ss->ko);
  ss->noShow = true;
  ss->cShow = true;
  ShowInternal();
  ENDSHOW;
}  /* end of PSCShow */

public procedure PSAWidthShow()
{
  integer wC;  Cd cd;
  BEGINSHOW;
  PopPString(&ssr->so);
  PopPCd(&cd);
  DTfmP(cd, &ssr->rdA);
  wC = PopInteger();
  if ((gs->fmap == FMapNone) && ((wC < 0) || (wC > 255)))
    RangeCheck();
  ssr->wChar = wC;
  PopPCd(&cd);
  DTfmP(cd, &ssr->rdW);
  ssr->aShow = true;
  ssr->wShow = true;
  ShowInternal();
  ENDSHOW;
}  /* end of PSAWidthShow */

public PCIItem FindInCache(c)
	integer /*character*/ c;
{
  register CIOffset cio;
  register PCIItem cip;
  register MID mid;
  PNameObj pno;

  if (!ValidMID ())
    return NIL;
  DecrSetMIDAge(crMID);
  mid = crMID;
  pno = (PNameObj) (gs->encAry.val.arrayval + c);
  if (pno->type != nameObj)
    return NIL;
  for (cio = pno->val.nmval->ncilink; cio != CINULL; cio = cip->cilink) {
    cip = &CI[cio];
    if (cip->mid == mid) {
      DebugAssert ((cip->type & Vmem) != 0 &&
		   cip->cmp->maskID == gs->device->maskID);
      return cip;
      }
    }
  return NIL;
  }

private procedure SafeShowMask(mask, dc, args)
  PMask mask; DevCd dc; DevFlushMaskArgs *args; {
  DURING
    ShowMask(mask, dc);
  HANDLER
    DevFlushMask(mask, args);
    CleanShowState(ss);
    RERAISE;
  END_HANDLER;
  DevFlushMask(mask, args);
  }

private boolean usePrebuilts = true;

private boolean PreBuiltChar(pcn) Object *pcn;
{
  Object ob;
  PreBuiltArgs args;
  boolean hasChar;
  Mtx cmtx;
  integer transform;
  if (!usePrebuilts)
    return false;
  args.matrix = &cmtx;
  GetFontAndCharInfo(
    pcn, &args.name, &args.nameLength, &args.font, args.matrix);
  if (TypeOfFID(args.font) == ndcFID || pcn->type != nameObj) return false;
  transform = TransformFID(args.font);
  if (transform == PREBUILT_OUTLINE) return false;
  args.transformedChar = transform / transformedCharFIDFactor;
  transform -= args.transformedChar * transformedCharFIDFactor;
  args.inBetweenSize = transform / inBetweenSizeFIDFactor;
  args.exactSize = transform - args.inBetweenSize * inBetweenSizeFIDFactor;
  args.bitmapWidths = BitmapWidthsFID(args.font) != 0;
  args.font &= prebuiltFIDMask;
  args.cacheThreshold = ctxCacheThreshold;
  args.compThreshold = ctxCompThreshold;
  args.horizMetrics = &ss->hMetrics;
  args.vertMetrics = &ss->vMetrics;
  if ((*gs->device->procs->PreBuiltChar)(gs->device, &args)) {
    if (AlreadyValidMID()) {
      if (charSortInterval++ > MaxCharSortInterval)
	SortCharAges();
      ss->cio = CIAlloc(false);
      }
    ss->mask = args.mask;
    return true;
    }
  return false;
}


public procedure MoveToAfterShow(ssr)
  register PShowState ssr;
{
  if (!ssr->useReal) UNFIXCD(ssr->fdcp, &ssr->rdcp);
  if (!ssr->noShow) MoveTo(ssr->rdcp, &gs->path);
}


private procedure GetShowFlushArgs(pcn, args)
  Object *pcn; DevFlushMaskArgs *args; {
  GetFontAndCharInfo(
    pcn, &args->name, &args->nameLength, &args->font, &args->matrix);
  args->horizMetrics = &ss->hMetrics;
  args->vertMetrics = &ss->vMetrics;
  }

public boolean ShowByName(c, pcn, fno, CFmp)
  integer c; Object *pcn; integer fno; PMTItem CFmp;
{
  register PCIItem cip;
  register CharMetrics *pcm;
  register PShowState ssr = ss;
  PMask cmptr;
  DevCd rectFcp;
  Cd cd, rincr;
  boolean fixedinc, doNotCache, haveXYincr;
  PNameEntry pne;
  Mtx mtx;
  DevFlushMaskArgs args;
  args.matrix = &mtx;

  DebugAssert(ssr->cio == CINULL && ssr->mask == NULL);
  fixedinc = true; /* ss->rdinc is not valid if char is cached */
  doNotCache = gs->isCharPath || ! ValidMID();
  haveXYincr = false;
  cip = NIL;
  pcm = NIL;
  ssr->pcn = pcn;

  if (! doNotCache)
    {
    MID mid = crMID;
    if (pcn->type != nameObj) {doNotCache = true; goto buildit;}
    pne = pcn->val.nmval;
    if (PccCount != 0) {doNotCache = true; goto buildit;}
    /* do not bother testing whether the ncilink is CINULL, since the
       CI entry corresponding to CINULL is a valid but vacant entry */
    cip = &CI[pne->ncilink];
    if (cip->mid == mid)
      {
      DebugAssert((cip->type & Vmem) != 0 &&
		  cip->cmp->maskID == gs->device->maskID);
      goto showchar;
      }
    while (cip->cilink != CINULL)
      {
      register PCIItem ocip = cip;
      cip = &CI[cip->cilink];
      if (cip->mid == mid)
	{
	CIOffset cio = ocip->cilink;
        DebugAssert((cip->type & Vmem) != 0 &&
		    cip->cmp->maskID == gs->device->maskID);
	ocip->cilink = cip->cilink;
	cip->cilink = pne->ncilink;
	pne->ncilink = cio;
	goto showchar;
	}
      }

    cip = NIL;
    }

buildit:
  Assert(cip==NIL);
  if (gs->isCharPath || ! PreBuiltChar(pcn))
    {
    boolean bcresult;
    if (charSortInterval++ > MaxCharSortInterval)
      SortCharAges();
    DelayedMakeFont();
    showLevel++;
    ssr->inBuildChar = true;
    bcresult = BuildChar(c, pcn); /* if true, build encountered exception */
    ssr->inBuildChar = false;
    showLevel--;
    if (bcresult)
      {
      if (ssr->mask != NULL) {
        GetShowFlushArgs(pcn, &args);
        DevFlushMask(ssr->mask, &args);
        }
      CleanShowState(ssr);
      return true; /* caller should trim show states */
      }
    fixedinc = false;  /* ss->rdinc is valid */
    if (ssr->mask == NIL)
      goto charshown; /* character was output directly */
    }

  if (! doNotCache &&
      ValidMID() &&
      ssr->cio != CINULL &&
      (curMT->shownchar || ssr->so.length > 1) && ssr->mask->cached)
    {
    /* character has been put in cache successfully */
    cip = CI + ssr->cio; /* i.e. &CI[ssr->cio] */
    cip->type = Vmem;
    }

link:
    ssr->vMetrics.offset.x = ssr->hMetrics.offset.x - ssr->vMetrics.offset.x;
    ssr->vMetrics.offset.y = ssr->hMetrics.offset.y - ssr->vMetrics.offset.y;
  if (cip != NIL)
    {
    register PMTItem mp;
    Fixed f;
    if (! ValidMID())
      { /* MID fell out of cache and can't be put back in */
      CIFree(ssr->cio);
      ssr->cio = CINULL;
      cip = NIL;
      goto showchar;
      }
    mp = curMT;
    cip->mid = crMID;
    MIDCount[cip->mid]++;
    cip->cmp = cmptr = ssr->mask;
    cip->metrics[0] = ssr->hMetrics;
    cip->metrics[1] = ssr->vMetrics;
    Assert(cmptr->maskID == mp->maskID);
    DebugAssert(pcn->type == nameObj && pne == pcn->val.nmval);
    cip->cilink = pne->ncilink;
    pne->ncilink = ssr->cio;
    CN[ssr->cio] = pcn->val.nmval;
    ssr->cio = CINULL;  /* CI item now owned by font cache */
    ssr->mask = NULL;
    /* update font bbox for this MID */
    pcm = &cip->metrics[0];
    if (pcm->offset.x < mp->values.sf.bb.ll.x)
      mp->values.sf.bb.ll.x = pcm->offset.x;
    if (pcm->offset.y < mp->values.sf.bb.ll.y)
      mp->values.sf.bb.ll.y = pcm->offset.y;
    if ((f = pcm->offset.x + FIXINT(cmptr->width)) > mp->values.sf.bb.ur.x)
      mp->values.sf.bb.ur.x = f;
    if ((f = pcm->offset.y + FIXINT(cmptr->height)) > mp->values.sf.bb.ur.y)
      mp->values.sf.bb.ur.y = f;
    if (mp->monotonic)
      {
      if (pcm->incr.x > 0) mp->xinc = true;
      if (pcm->incr.x < 0) mp->xdec = true;
      if (pcm->incr.y > 0) mp->yinc = true;
      if (pcm->incr.y < 0) mp->ydec = true;
      if ((mp->xinc && mp->xdec) || (mp->yinc && mp->ydec))
        mp->monotonic = false;
      }
    pcm = &cip->metrics[1];
    if (pcm->offset.x < mp->values.sf.bb1.ll.x)
      mp->values.sf.bb1.ll.x = pcm->offset.x;
    if (pcm->offset.y < mp->values.sf.bb1.ll.y)
      mp->values.sf.bb1.ll.y = pcm->offset.y;
    if ((f = pcm->offset.x + FIXINT(cmptr->width)) > mp->values.sf.bb1.ur.x)
      mp->values.sf.bb1.ur.x = f;
    if ((f = pcm->offset.y + FIXINT(cmptr->height)) > mp->values.sf.bb1.ur.y)
      mp->values.sf.bb1.ur.y = f;
    if (mp->monotonic1)
      {
      if (pcm->incr.x > 0) mp->xinc1 = true;
      if (pcm->incr.x < 0) mp->xdec1 = true;
      if (pcm->incr.y > 0) mp->yinc1 = true;
      if (pcm->incr.y < 0) mp->ydec1 = true;
      if ((mp->xinc1 && mp->xdec1) || (mp->yinc1 && mp->ydec1))
        mp->monotonic1 = false;
      }
    if (CFmp != NIL) {
      if (mp->values.sf.bb.ll.x < CFmp->values.sf.bb.ll.x)
        CFmp->values.sf.bb.ll.x = mp->values.sf.bb.ll.x;
      if (mp->values.sf.bb.ll.y < CFmp->values.sf.bb.ll.y)
        CFmp->values.sf.bb.ll.y = mp->values.sf.bb.ll.y;
      if (mp->values.sf.bb.ur.x > CFmp->values.sf.bb.ur.x)
        CFmp->values.sf.bb.ur.x = mp->values.sf.bb.ur.x;
      if (mp->values.sf.bb.ur.y > CFmp->values.sf.bb.ur.y)
        CFmp->values.sf.bb.ur.y = mp->values.sf.bb.ur.y;
      if (mp->values.sf.bb1.ll.x < CFmp->values.sf.bb1.ll.x)
        CFmp->values.sf.bb1.ll.x = mp->values.sf.bb1.ll.x;
      if (mp->values.sf.bb1.ll.y < CFmp->values.sf.bb1.ll.y)
        CFmp->values.sf.bb1.ll.y = mp->values.sf.bb1.ll.y;
      if (mp->values.sf.bb1.ur.x > CFmp->values.sf.bb1.ur.x)
        CFmp->values.sf.bb1.ur.x = mp->values.sf.bb1.ur.x;
      if (mp->values.sf.bb1.ur.y > CFmp->values.sf.bb1.ur.y)
        CFmp->values.sf.bb1.ur.y = mp->values.sf.bb1.ur.y;
      if (!mp->monotonic) CFmp->monotonic = false;
      if (!mp->monotonic1) CFmp->monotonic1 = false;
      }
    }
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP

showchar:
  if (cip != NIL) {
    SetCharAge (cip);
    cmptr = cip->cmp;
    pcm = &cip->metrics[gs->writingmode];
    }
  else {
    cmptr = ssr->mask;
    pcm = (gs->writingmode == 0) ? &ssr->hMetrics : &ssr->vMetrics;
    }
  if (fixedinc)
    {
    FIXTOPFLT(pcm->incr.x, &ssr->rdinc.x);
    FIXTOPFLT(pcm->incr.y, &ssr->rdinc.y);
    }
  if ((!ssr->noShow) && (cmptr->width != 0))
    {
    if (!ssr->useReal)
      {
      rectFcp.x = FROUND(ssr->fdcp.x + pcm->offset.x);
      rectFcp.y = FROUND(ssr->fdcp.y + pcm->offset.y);
      }
    else
      {
      FIXTOPFLT(pcm->offset.x, &cd.x);
      FIXTOPFLT(pcm->offset.y, &cd.y);
      VecAdd(ssr->rdcp, cd, &cd);
      RRoundP(&cd.x, &cd.x); rectFcp.x = cd.x;
      RRoundP(&cd.y, &cd.y); rectFcp.y = cd.y;
      }
    if ( /* make sure entire bbox fits in Fixed point range */
#if (DPSXA != 0)
        true ||
#endif /* (DPSXA != 0) */
        rectFcp.x > (integer)(-(1<<15)) &&
        rectFcp.y > (integer)(-(1<<15)) &&
        (integer)(rectFcp.x + cmptr->width) < (integer)(1<<15) &&
        (integer)(rectFcp.y + cmptr->height) < (integer)(1<<15))
      {
      /* If it is being shown from the scratch area, we must protect
         the call to ShowMask so that the resources held in the
	 ShowState will be reclaimed if ShowMask raises an exception.
	 If it is being shown from the cache, this is unnecessary;
	 the resources are held by the cache, not by the ShowState. */
      if (cip == NIL)
	{
	if (ssr->mask != NULL) {
	  GetShowFlushArgs(pcn, &args);
	  SafeShowMask(cmptr, rectFcp, &args); /* will call DevFlushMask */
	  }
	}
      else 
        ShowMask(cip->cmp, rectFcp);
      }
    else if (cip == NIL) {
      GetShowFlushArgs(pcn, &args);
      DevFlushMask(cmptr, &args);
      }
    }
  else if (cip == NIL) {
    GetShowFlushArgs(pcn, &args);
    DevFlushMask(cmptr, &args);
    }

charshown:
  if (ssr->mask != NULL || ssr->cio != CINULL) CleanShowState(ssr);
  if (AlreadyValidMID()) curMT->shownchar = true;

  if (!ssr->useReal)
    {
    DevCd incr;
    rectFcp.x = ssr->fdcp.x;
    rectFcp.y = ssr->fdcp.y;
    if (ssr->xShow || ssr->yShow)
      {
      Fixed x;
      if (!ssr->xShow) incr.x = 0;
      else incr.x = (*ssr->ns.GetFixed)(&ssr->ns);
      if (!ssr->yShow) incr.y = 0;
      else incr.y = (*ssr->ns.GetFixed)(&ssr->ns);
      haveXYincr = true; UNFIXCD(incr, &rincr);
      if (os_labs(incr.x) > ssr->xymax || os_labs(incr.y) > ssr->xymax)
        {
	UNFIXCD(rectFcp, &ssr->rdcp);
	goto usereal;
	}
      x = incr.x;
      incr.x = fixmul(x, ssr->mtx_a) + fixmul(incr.y, ssr->mtx_c);
      incr.y = fixmul(x, ssr->mtx_b) + fixmul(incr.y, ssr->mtx_d);
      }
    else if (pcm)
      {
      incr.x = pcm->incr.x;
      incr.y = pcm->incr.y;
      }
    else FIXCD(ssr->rdinc, &incr);
    ssr->fdcp.x += incr.x + ssr->fdA.x;
    ssr->fdcp.y += incr.y + ssr->fdA.y;
    if (ssr->wShow && ((c | fno) == ssr->wChar))
      {
      ssr->fdcp.x += ssr->fdW.x;
      ssr->fdcp.y += ssr->fdW.y;
      }
    if (!OkFixed(&ssr->fdcp, LOWERSHOWBOUND, UPPERSHOWBOUND))
      {
      UNFIXCD(rectFcp, &ssr->rdcp);
      goto usereal;
      }
    }
  else
    {
usereal:
    ssr->useReal = true;  /* once real, always real */
    if (ssr->xShow || ssr->yShow)
      {
      if (!haveXYincr)
        {
	if (!ssr->xShow) rincr.x = fpZero;
	else (*ssr->ns.GetReal)(&ssr->ns, &rincr.x);
	if (!ssr->yShow) rincr.y = fpZero;
	else (*ssr->ns.GetReal)(&ssr->ns, &rincr.y);
	}
      DTfmPCd(rincr, &gs->matrix, &rincr);
      VecAdd(rincr, ssr->rdA, &cd);
      }
    else VecAdd(ssr->rdinc, ssr->rdA, &cd);
    VecAdd(ssr->rdcp, cd, &ssr->rdcp);
    if (ssr->wShow && ((c | fno) == ssr->wChar))
      VecAdd(ssr->rdcp, ssr->rdW, &ssr->rdcp);
    }
  return false;
  }


public boolean SSSOutCall() {
  boolean bcresult;
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP
  DelayedMakeFont ();
  showLevel++;
  if ((bcresult = psExecute(ss->ko)))
    TrimShowStates(); /* free nested ShowStates but not current one */
  showLevel--;
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP
  if (bcresult) return true;
  ValidMID();
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP
  return false;
  }

private procedure SlowShow()
{
  register PShowState ssr = ss;
  register integer i, c;
  register PCard8 cp;

  ssr->havefont = true;
  ssr->DelayedFont = NOLL;
  cp = ssr->so.val.strval;
  if (ssr->xShow || ssr->yShow) ssr->ns = ssr->initns;
  for (i = ssr->so.length; --i >= 0; )
    {  /* the main loop */
    if (GetAbort() != 0)
      RAISE((int)GetAbort(), (char *)NIL); /* allow user to interrupt during show */
    c = *cp++;
    if (ShowByName(c, ssr->encAry.val.arrayval + c, 0, NULL)) {
      TrimShowStates();
      RAISE((int)GetAbort(), (char *)NIL);
      }
    if (ssr->kShow && i > 0)
      {
      Assert(ssr->useReal);
      MoveTo(ssr->rdcp, &gs->path);
      PushInteger((integer) c);
      PushInteger((integer) *cp);
      if (SSSOutCall())
        break;  /* just quit; leave exception pending */
      ssr->rdcp = gs->cp;
      }
    if (ssr->cShow) {
      Cd cd;
      PushInteger ((integer) c);
      IDTfmP (ssr->rdinc, &cd);
      PushPCd (&cd);
      if (SSSOutCall())
        break;  /* just quit; leave exception pending */
      }
    }
  MoveToAfterShow(ssr);
}


public procedure SimpleShowByName(c, pcn)
  integer c; Object *pcn;
{
  BEGINSHOW;
  ssr->rdcp = gs->cp;
  if (!CvtToXFixed(&(ssr->fdcp), ssr->rdcp, LOWERSHOWBOUND, UPPERSHOWBOUND))
    ssr->useReal = true;
  else
    { /* make sure fdcp and rdcp agree */
    FIXTOPFLT(ssr->fdcp.x, &ssr->rdcp.x);
    FIXTOPFLT(ssr->fdcp.y, &ssr->rdcp.y);
    }
  if (!ssr->noShow) CheckForCurrentPoint(&gs->path);
  if (ValidMID()) DecrSetMIDAge(crMID);
  ss->encAry = gs->encAry;
  ssr->so.length = 2;  /* subvert single character no-cache kludge */
  ss->havefont = true;
  ss->DelayedFont = NOLL;
  if (ShowByName(c, ssr->encAry.val.arrayval + c, 0, NULL)) {
    TrimShowStates();
    RAISE((int)GetAbort(), (char *)NIL);
    }
  MoveToAfterShow(ssr);
  ENDSHOW;
}


public procedure FontShwDataHandler (code)
  StaticEvent code;
{
  switch (code) {
   case staticsCreate: {
    ctxCompThreshold = defaultCompThreshold;
    ctxCacheThreshold = defaultCacheThreshold;
    break;
   }

   case staticsDestroy: {
    showLevel = 0;
    TrimShowStates();
    break;
   }
  }
}

private procedure PSUsePrebuilts() {usePrebuilts = PopBoolean();}

public procedure FontShowInit(reason)  InitReason reason;
{
integer i;
PShowState ssr;
switch (reason)
  {
  case init:
    ssr = (PShowState) NEW(NSHOWSTATES, (integer) sizeof(ShowState));
    for (i = NSHOWSTATES; --i >= 0; ssr++)
      {ssr->link = ssFree; ssFree = ssr;}
#if STAGE==DEVELOP
    /* These assertions assure that the specified "null" values are in fact
       represented by zero (PushShowState and BEGINSHOW depend on this) */
    Assert(CINULL == 0);
    Assert(MIDNULL == 0);
    /*
    {real f = fpZero; Assert(*(integer *)&f == 0);}
    */
    ssCount = NSHOWSTATES;
#endif STAGE==DEVELOP
    FSInit();
    break;
  case romreg: 
#if STAGE==DEVELOP
    if (vSTAGE == DEVELOP)
      RgstExplicit("useprebuilts", PSUsePrebuilts);
#endif STAGE==DEVELOP
    break;
  endswitch
  }
}  /* FontShowInit */
