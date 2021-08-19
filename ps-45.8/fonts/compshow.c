/*
  compshow.c

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
Larry Baer: Fri Feb  9 09:32:29 1990
Ed Taft: Fri Jul 29 09:06:59 1988
Jim Sandman: Tue Feb 27 10:11:10 1990
Bill Paxton: Tue Apr  4 10:33:17 1989
Ivor Durham: Fri Sep 23 15:40:09 1988
Paul Rovner: Tuesday, October 25, 1988 1:04:28 PM
Perry Caro: Wed Nov  9 14:23:21 1988
Joe Pasqua: Thu Jan 19 15:24:10 1989
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

public procedure CopyDelayedFont() {
  boolean origShared = CurrentShared();
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP
  if (ss->DelayedFont.val.dictval == gs->fontDict.val.dictval) {
    SetShared(gs->fontDict.shared);
    DURING
      AllocCopyDict(gs->fontDict, 0, &gs->fontDict);
    HANDLER
      SetShared(origShared); RERAISE;
    END_HANDLER;
    SetShared(origShared);
    }
  }

private procedure GetDMFItem(shared, dSize, pobj)
  boolean shared; integer dSize; PObject pobj; {
  DictObj d;
  AryObj ar;
  boolean origShared = CurrentShared();
  SetShared(shared);
  DURING
    AllocPArray(4, pobj);
    ConditionalInvalidateRecycler(pobj);
    DictP(dSize, &d);
    VMPutElem(*pobj, 1, d);
    AllocPArray(6, &ar);
    ConditionalInvalidateRecycler(&ar);
    VMPutElem(*pobj, 2, ar);
    AllocPArray(6, &ar);
    ConditionalInvalidateRecycler(&ar);
    VMPutElem(*pobj, 3, ar);
  HANDLER
    SetShared(origShared); RERAISE;
  END_HANDLER;
  SetShared(origShared);
  }

private procedure GetDMFDict(shared, dSize, pobj)
  boolean shared; integer dSize; PObject pobj; {
  boolean origShared = CurrentShared();
  SetShared(shared);
  DURING
    DictP(dSize, pobj);
  HANDLER
    SetShared(origShared); RERAISE;
  END_HANDLER;
  SetShared(origShared);
  }

public procedure DelayedMakeFont ()
{
  AryObj fm, obj, tempobj; Mtx mtx;
  DictObj d; boolean b;
  integer i, idsize, fdsize;
  boolean shared = gs->fontDict.shared;
  PVMRoot root;
  MID mid;
  register ShowState *ssr = ss;
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP
  if (! ssr->havefont) {
    root = (shared ? rootShared : rootPrivate);
    fdsize = gs->fontDict.val.dictval->maxlength + 2;
    obj = ssr->delayedAry;
    if (obj.type != arrayObj) {
      obj = root->trickyDicts.val.arrayval[tdDummy];
      if (obj.type == nullObj)
	GetDMFItem(shared, fdsize+8, &obj);
      else
	VMPutElem(root->trickyDicts, tdDummy, obj.val.arrayval[0]);
      }
    d = obj.val.arrayval[1];
    if (d.val.dictval->maxlength < fdsize) {
      GetDMFDict(shared, fdsize+8, &d);
      VMPutElem(obj, 1, d);
      }
    DictGetP (pfont, fontsNames[nm_FontMatrix], &fm);
    PAryToMtx (&fm, &mtx);
    mid = MakeFontSupp (gs->fontDict, &mtx, &gs->fontDict, obj, &b);
    ssr->delayedAry = obj;
    ssr->NewMID = b;
    ssr->DelayedMID = MT[mid].umid;
    ssr->DelayedFont = gs->fontDict;
    ssr->havefont = true;
    ValidMID();
  }
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP
}

private procedure ZapDelayedDict (d)
  DictObj d;
{
  KeyVal nkv;
  register KeyVal *kvp;
  register integer i;
  PNameEntry pne;
  PDictBody db = d.val.dictval;
  NullObj n;
  
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP
  n = NOLL;
  n.level = d.level;
  nkv.key = n; nkv.value = n;
  kvp = db->begin;
  for (i = db->size; i-- != 0; )
  {
    if (kvp->key.type == nameObj)
    {
      pne = kvp->key.val.nmval;
      pne->kvloc = NULL;
      pne->ts.stamp = 0;
      pne->dict = NULL;       /*  is there a null dictoffset?  */
    }
    *kvp++ = nkv;
  }
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP
}
 
private procedure DMFRestore()
{
  register ShowState *ssr = ss;
  UniqueMID umid = ssr->DelayedMID;
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP
  if (ssr->delayedAry.type == arrayObj) {
    PVMRoot root = (ssr->delayedAry.shared ? rootShared : rootPrivate);
    VMPutElem(ssr->delayedAry, 0, root->trickyDicts.val.arrayval[tdDummy]);
    VMPutElem(root->trickyDicts, tdDummy, ssr->delayedAry);
    ssr->delayedAry = NOLL;
    }
  if (ssr->havefont) {
    if ((umid.rep.mid != MIDNULL) && 
      (MT[umid.rep.mid].umid.stamp == umid.stamp))
      PurgeMID (umid.rep.mid);
    if (ssr->NewMID) ZapDelayedDict (ssr->DelayedFont);
    ssr->havefont = false;
    ssr->DelayedFont = NOLL;
    }
#if STAGE==DEVELOP
  CheckCI();
#endif STAGE==DEVELOP
}


#define FontS 16777216
#define FontT 8388608
#define FontSTMask 8388607
#define FontFCMask 255
#define FontFIMask 65281
#define FontST (FontS + FontT)

#define MatchChar 0
#define NoMatchBackup 1
#define NoMatchFIC 2
#define MatchBackup 6
#define MatchFIC 7

#define SizeWElem 4
#define SizeSElem 11

typedef struct
        {
            DictObj fdT;
            AryObj fdaryT, pearyT, encaryT;
            UniqueMID *miaryT;
            StrObj kvaryT, svaryT;
            integer fmT, mdT, fontno;
	} SSState, *SSStatePtr;

/* In order for FastShowSupp to determine whether the show string
   is clipped, it is essential for the bounding box of the root
   font to enclose the bounding boxes of all the descendent fonts.
   SlowShow only computes the bounding box of the root composite
   font, and it does not compute the bounding boxes of any of the
   interior composite fonts of a multilevel composite font.  However
   ShowInternal, by calling InvalidateCachedMIDs, ensures that the
   bounding box information of interior fonts will be computed if
   they should be extracted from the FDepVector and made the current
   font. */
public procedure InvalidateCachedMIDs (f, miary, fdary)
  DictObj f; UniqueMID *miary; AryObj fdary;
{
  register integer i;
  register UniqueMID *SPtr;
  StrObj S;
  DictObj elem;
  IntObj fntype;
  SPtr = miary;
  for (i = 0; i < fdary.length; i++)
  {
    SPtr->stamp = MIDNULL;
    SPtr++;
    elem = VMGetElem (fdary, i);
    DictGetP (elem, fontsNames[nm_FontType], &fntype);
    if (fntype.val.ival == COMPOSEDtype)
    {
      UniqueMID *m;
      AryObj fd;
      DictGetP (elem, fontsNames[nm_MIDVector], &S);
      m = (UniqueMID *) S.val.strval;
      DictGetP (elem, fontsNames[nm_FDepVector], &fd);
      InvalidateCachedMIDs(elem, m, fd);
    }
  }
  DictGetP (f, fontsNames[nm_CurMID], &S);
  *((UniqueMID*) S.val.strval) = curMT->umid;
}


private Fixed FixedfromString(PA,E)
  PStrObj PA;
  integer E;
{
  register Fixed ck;
  ck = VMGetChar(*PA,(cardinal)E++) << 24;
  ck |= (VMGetChar(*PA,(cardinal)E++) << 16);
  ck |= (VMGetChar(*PA,(cardinal)E++) << 8);
  ck |= VMGetChar(*PA,(cardinal)E++);
  return ck;
}

procedure GetInfoSupp (SPtr)
  register SSStatePtr SPtr;
{
  Object ob;
  switch (SPtr->fmT)
  {
  case FMapNone:
    break;
  case FMapGen:
    DictGetP(SPtr->fdT,fontsNames[nm_SubsVector],&SPtr->svaryT);
  case FMap17:
  case FMap88:
  case FMap97:
  case FMapEscape:
    DictGetP(SPtr->fdT,fontsNames[nm_PrefEnc],&SPtr->pearyT);
    DictGetP(SPtr->fdT,fontsNames[nm_FDepVector],&SPtr->fdaryT);
    DictGetP(SPtr->fdT,fontsNames[nm_MIDVector],&ob);
    SPtr->miaryT = (UniqueMID *) ob.val.strval;
    break;
  }
}

private procedure SSRestoreState ()
{
  register ShowState *ssr = ss;
  if ((ssr->fontDict).val.dictval != (gs->fontDict).val.dictval)
  {
    gs->fontDict = ssr->fontDict;
    gs->encAry = ssr->encAry;
    gs->fmap = ssr->fmap;
  }
}

#define BytefromString(A,E) VMGetChar(A,(cardinal)E)

private integer Xxxxx(p, i) PCard8 p; integer i; {
  return GetMTPE(p, i);
  }
  
#define GetInfo(x) { \
  register SSStatePtr OldSPtr = SPtr; \
  register integer TempMdT, TempFmT; \
  Object ob; \
  integer mtpeVal; \
  PCard8 mtpe; \
  SPtr++; \
  if (x >= OldSPtr->encaryT.length) RangeCheck (); \
  ob = OldSPtr->encaryT.val.arrayval[x]; \
  SPtr->mdT = TempMdT = ob.val.ival; \
  SPtr->fdT = OldSPtr->fdaryT.val.arrayval[(cardinal) TempMdT]; \
  mtmid = (OldSPtr->miaryT + TempMdT); \
  mtpe = (PCard8)(OldSPtr->miaryT + OldSPtr->fdaryT.length); \
  mtpeVal = Xxxxx(mtpe, TempMdT); \
  SPtr->fmT = TempFmT = mtpeVal & MTMASK; \
  if (mtpeVal & PEMASK) \
    SPtr->encaryT = OldSPtr->pearyT; \
  else \
    DictGetP (SPtr->fdT, fontsNames[nm_Encoding], &SPtr->encaryT); \
  if (TempFmT != 0) GetInfoSupp (SPtr); /* 0==FMapNone */ \
  }

#define SSGetChar(d) \
  { \
    if (i >= lenshow) RangeCheck (); \
    d = VMGetChar(ssr->so,(cardinal)i++); \
  }


#define CSGetChar(d) d = *cp++;
#define CSGCh() (*cp++)
#define CSTestChar() if (cp >= cplimit) RangeCheck ();

public integer ScanCompString () {
  SSState SSStack [MaxSSS];
  SSStatePtr SPtrSave;
  charptr cp, cplimit;
  integer EscapeChar, ufi, ufc;    
  UniqueMID *mtmid;
  integer nChars = 0;

  SPtrSave = &SSStack [0];
  {
    register ShowState *ssr = ss;
    register SSStatePtr SPtr = SPtrSave;
    cp = ssr->so.val.strval;
    cplimit = &cp [ssr->so.length];
    SPtr->fdT = ssr->fontDict;
    SPtr->encaryT = ssr->encAry;
    SPtr->kvaryT = ssr->kvary;
    SPtr->svaryT = ssr->svary;
    SPtr->fdaryT = ssr->fdary;
    SPtr->miaryT = ssr->miary;
    SPtr->pearyT = ssr->peary;
    SPtr->fmT = ssr->fmap;
  }

  EscapeChar = gs->escapechar;

  while (cp < cplimit) {
    {
      register SSStatePtr SPtr;
      SPtr = SPtrSave;
    Rep1:
      CSGetChar (ufc);
      if (ufc == EscapeChar) {
      Rep0:
        if (SPtr->fmT == FMapEscape) {
          CSTestChar ();
          CSGetChar (ufc);
          if (ufc != EscapeChar) {
            SPtrSave++;
            GetInfo (ufc);
            CSTestChar ();
            goto Rep1;
            }
          }
        if ((SPtr != &SSStack [0]) && ((SPtr-1)->fmT == FMapEscape)) {
          SPtrSave--;
          SPtr = SPtrSave;
          CSTestChar ();
          goto Rep0;
          }
        }
      goto Lab0;
    Lab9:
      GetInfo (ufi);
    Lab0:
      switch (SPtr->fmT) {
	case FMapNone:
	  break;
	case FMap17:
	  ufi = ufc >> 7;
	  ufc &= 0x7f;
	  goto Lab9;
	case FMap88:
	  ufi = ufc;
	  CSTestChar ();
	  CSGetChar (ufc);
	  goto Lab9;
	case FMap97:
	  ufi = ufc << 1;
	  CSTestChar ();
	  CSGetChar (ufc);
	  ufi |= (ufc >> 7);
	  ufc &= 0x7f;
	  goto Lab9;
	case FMapEscape:
	  SPtrSave++;
	  ufi = 0;
	  goto Lab9;
	case FMapGen: {
	    register unsigned char *svptr = SPtr->svaryT.val.strval;
	    register integer subst;
	    integer j, k, l;
	    if ((ufi = *svptr++) == 0) {
	      ufi = SPtr->svaryT.length;
	      while (--ufi != 0) {
		if (ufc < (subst = *svptr++)) break;
		ufc -= subst;
	        }
	      ufi -= SPtr->svaryT.length;
	      ufi = ~ufi;
	      }
	    else {
	      j = ufi+1;    /* j == number of bytes per int */;
	      do {
	        CSTestChar (); ufc = (ufc<<8) + CSGCh ();}
	        while (--ufi != 0);
	      l = ufc;
	      k = (integer)svptr + SPtr->svaryT.length - 1;
	      while ((integer)svptr != k) {
		ufc = j; subst = *svptr++;
		while (--ufc != 0) subst = (subst<<8) + *svptr++;
		if (l < subst) break;
		l -= subst; ufi++;
	        }
	      ufc = l;
	      }
	    }
	  goto Lab9;
	}
      nChars++;
      if (ufc >= SPtr->encaryT.length)
        PSRangeCheck();
    }
  }
  return nChars;
}


public integer CompositeShow (showchars) boolean showchars; {
  Object ob;
  Fixed ufw, ufh;
  Mtx mtxtemp;
  UniqueMID *mtmid;
  SSState SSStack [MaxSSS];
  SSStatePtr SPtrSave;
  Cd width;
  DevMask *scipsave;
  integer EscapeChar, curwm, dcpX, dcpY, dcpX0, dcpY0, fmtsave;
  boolean xyshow, xinc, yinc, xdec, ydec;
  Fixed xymax, mtx_a, mtx_b, mtx_c, mtx_d;
  MtxType mtxtype;
  register charptr cp, cplimit;
  register integer ufi, ufc;                   /* desperation */
  register Fixed boundary, fixhalf;
  register PCIItem cip;
  PNameEntry CSpne;
  CIOffset CScio;
  boundary = FIXINT(LOWERSHOWBOUND); fixhalf = 1<<(FRACTION-1);
  SPtrSave = &SSStack [0];

  {
    register ShowState *ssr = ss;
    register SSStatePtr SPtr = SPtrSave;
    dcpX = ssr->fdcp.x; dcpY = ssr->fdcp.y;
    cp = ssr->so.val.strval;
    cplimit = &cp [ssr->so.length];
    SPtr->fdT = ssr->fontDict;
    SPtr->encaryT = ssr->encAry;
    SPtr->kvaryT = ssr->kvary;
    SPtr->svaryT = ssr->svary;
    SPtr->fdaryT = ssr->fdary;
    SPtr->miaryT = ssr->miary;
    SPtr->pearyT = ssr->peary;
    SPtr->fmT = ssr->fmap;
    if (!ssr->xShow && !ssr->yShow) xyshow = false;
    else {
      Mtx *mtx = &gs->matrix;
      real mtxmax, mtxabs;
      Fixed fmtxmax;
      xyshow = true;
      xinc = yinc = xdec = ydec = false;
      mtxmax = os_fabs(mtx->a);
      mtxabs = os_fabs(mtx->b); if (mtxabs > mtxmax) mtxmax = mtxabs;
      mtxabs = os_fabs(mtx->c); if (mtxabs > mtxmax) mtxmax = mtxabs;
      mtxabs = os_fabs(mtx->d); if (mtxabs > mtxmax) mtxmax = mtxabs;
      if (mtxmax > 8000.0) goto realslow;
      PFLTTOFIX(&mtxmax, fmtxmax);
      xymax = fixdiv(FIXINT(8000), fmtxmax);
      PFLTTOFIX(&mtx->a, mtx_a);
      PFLTTOFIX(&mtx->b, mtx_b);
      PFLTTOFIX(&mtx->c, mtx_c);
      PFLTTOFIX(&mtx->d, mtx_d);
      if (mtx_b == 0 && mtx_c == 0) {
        if (mtx_a == FIXINT(1) && mtx_d == FIXINT(1))
          mtxtype = identity;
	else if (mtx_a == FIXINT(1) && mtx_d == FIXINT(-1))
          mtxtype = flipY;
        else mtxtype = bc_zero;
	}
      else if (mtx_a == 0 && mtx_d == 0) mtxtype = ad_zero;
      else mtxtype = general;
      }
  }

  curwm = gs->writingmode;
  EscapeChar = gs->escapechar;
  scipsave = showCache;

  while (cp < cplimit) {
    {
      register SSStatePtr SPtr;
      SPtr = SPtrSave;
    Rep1:
      CSGetChar (ufc);
      if (ufc == EscapeChar) {
      Rep0:
        if (SPtr->fmT == FMapEscape) {
          CSTestChar ();
          CSGetChar (ufc);
          if (ufc != EscapeChar) {
            SPtrSave++;
            GetInfo (ufc);
            CSTestChar ();
            goto Rep1;
            }
          }
        if ((SPtr != &SSStack [0]) && ((SPtr-1)->fmT == FMapEscape)) {
          SPtrSave--;
          SPtr = SPtrSave;
          CSTestChar ();
          goto Rep0;
          }
        }
      goto Lab0;
    Lab9:
      GetInfo (ufi);
    Lab0:
      switch (SPtr->fmT) {
	case FMapNone:
	  break;
	case FMap17:
	  ufi = ufc >> 7;
	  ufc &= 0x7f;
	  goto Lab9;
	case FMap88:
	  ufi = ufc;
	  CSTestChar ();
	  CSGetChar (ufc);
	  goto Lab9;
	case FMap97:
	  ufi = ufc << 1;
	  CSTestChar ();
	  CSGetChar (ufc);
	  ufi |= (ufc >> 7);
	  ufc &= 0x7f;
	  goto Lab9;
	case FMapEscape:
	  SPtrSave++;
	  ufi = 0;
	  goto Lab9;
	case FMapGen: {
	    register unsigned char *svptr = SPtr->svaryT.val.strval;
	    register integer subst;
	    integer j, k, l;
	    if ((ufi = *svptr++) == 0) {
	      ufi = SPtr->svaryT.length;
	      while (--ufi != 0) {
		if (ufc < (subst = *svptr++)) break;
		ufc -= subst;
	        }
	      ufi -= SPtr->svaryT.length;
	      ufi = ~ufi;
	      }
	    else {
	      j = ufi+1;    /* j == number of bytes per int */
	      do {
	        CSTestChar (); ufc = (ufc<<8) + CSGCh ();}
	        while (--ufi != 0);
	      l = ufc;
	      k = (integer)svptr + SPtr->svaryT.length - 1;
	      while ((integer)svptr != k) {
		ufc = j; subst = *svptr++;
		while (--ufc != 0) subst = (subst<<8) + *svptr++;
		if (l < subst) break;
		l -= subst; ufi++;
	        }
	      ufc = l;
	      }
	    }
	  goto Lab9;
	}
      {
        register PNameObj pno;
        pno = &(SPtr->encaryT.val.arrayval [ufc]);
        if (pno->type != nameObj)
          goto slow;
        CSpne = pno->val.nmval;
      }
    }
    {
      register PCIItem cip;
      {
        {
	register MID md = mtmid->rep.mid;
	if (md == MIDNULL)
	  goto slow;
	if (MT[md].umid.stamp != mtmid->stamp)
	  goto slow;
	SetMIDAge (md);
	cip = CI + CSpne->ncilink;
	if (cip->mid == md) goto foundchar;
	while (true) {
	  register PCIItem ocip = cip;
	  if ((CScio = cip->cilink) == CINULL)
	    goto slow;
	  cip = CI + CScio;
	  if (cip->mid == md) {
	    ocip->cilink = cip->cilink;
	    cip->cilink = CSpne->ncilink;
	    CSpne->ncilink = CScio;
	    break;
	    }
	  }
	}
      }
    foundchar:
      {
        {
        register CharMetrics *cmet = &(cip->metrics[curwm]);
        register PMask cmptr = cip->cmp;
        DebugAssert(cmptr->maskID == gs->device->maskID);
        SetCharAge(cip);
        if (cmptr->width != 0) {
          register DevMask *scip = scipsave;
          scip->mask = cip->cmp;
          scip->dc.x = (dcpX + cmet->offset.x + fixhalf) >> FRACTION;
          scip->dc.y = (dcpY + cmet->offset.y + fixhalf) >> FRACTION;
          scip++;
          scipsave = scip;
          dcpX0 = dcpX; dcpY0 = dcpY;
	  }
        if (!xyshow) {
          dcpX += cmet->incr.x;
          dcpY += cmet->incr.y;
	  }
        }
      if (xyshow) {
        register PShowState ssr = ss;
	Fixed x, incx, incy;
	if (!ssr->xShow) incx = 0;
	else {
          incx = (*ssr->ns.GetFixed)(&ssr->ns);
	  if (os_labs(incx) > xymax)
	    goto slow;
	  }
	if (!ssr->yShow) incy = 0;
	else {
          incy = (*ssr->ns.GetFixed)(&ssr->ns);
	  if (os_labs(incy) > xymax)
	    goto slow;
	  }
	/* do fast inline delta transform */
	switch (mtxtype) {
          case identity:
            break;
	  case flipY:
            incy = -incy; break;
	  case bc_zero:
	    incx = fixmul(incx, mtx_a);
	    incy = fixmul(incy, mtx_d);
            break;
          case ad_zero:
            x = incx;
	    incx = fixmul(incy, mtx_c);
	    incy = fixmul(x, mtx_b);
            break;
	  case general:
            x = incx;
	    incx = fixmul(x, mtx_a) + fixmul(incy, mtx_c);
	    incy = fixmul(x, mtx_b) + fixmul(incy, mtx_d);
	  }
        /* check for monotonic */
	if (incx != 0) {
          if (incx < 0 && !xdec) {
            if (xinc) goto slow; xdec = true; }
	  else if (incx > 0 && !xinc) {
            if (xdec) goto slow; xinc = true; }
	  }
        if (incy != 0) {
          if (incy < 0 && !ydec) {
            if (yinc) goto slow; ydec = true; }
	  else if (incy > 0 && !yinc) {
            if (ydec) goto slow; yinc = true; }
          }
        dcpX += incx;
	dcpY += incy;
        }
      if (ss->aShow) {
        register PShowState ssr = ss;
        dcpX += ssr->fdA.x;
        dcpY += ssr->fdA.y;
        }
      /* check for cp entering giggle weeds */
      if ((dcpX < boundary) || (dcpY < boundary))
        goto realslow;
      boundary = -boundary;
      if ((dcpX > boundary) || (dcpY > boundary))
        goto realslow;
      boundary = -boundary;
      }
    }
  }
 strDone:
  if (!showchars || (scipsave == showCache)){
    register PShowState ssr = ss;
    /* update ss->cp for stringwidth */
    ssr->fdcp.x = dcpX; ssr->fdcp.y = dcpY;
    /* if showing chars and scipsave == showCache then do moveto */
    if (showchars) FixedMoveTo(ssr->fdcp);
    return 0;}
{				/* if bbox doesn't fit then call SlowShow() */
	register DevBB *bb;
	register integer llx, lly, urx, ury;
	DevCd           fll, fur;

	bb = (curwm == 0) ? &curMT->values.sf.bb : &curMT->values.sf.bb1;
	/*
	 * ss->fdcp.x and ss->fdcp.y give the initial character position;
	 * dcpX0 and dcpY0 give the final character position.  This test
	 * assumes that the shape enclosing the bounding boxes of the first
	 * and last characters also encloses the bounding boxes of all the
	 * characters in between.  This assumption is valid since FastShow is
	 * only used with fonts whose characters have width vectors that are
	 * consistent in direction. 
	 */
	{
		register PShowState ssr = ss;
		llx = ssr->fdcp.x + bb->ll.x;
		lly = ssr->fdcp.y + bb->ll.y;
		urx = ssr->fdcp.x + bb->ur.x;
		ury = ssr->fdcp.y + bb->ur.y;
		ssr->fdcp.x = dcpX;
		ssr->fdcp.y = dcpY;	/* update for FixedMoveTo */
		fll.x = dcpX0 + bb->ll.x;
		fll.y = dcpY0 + bb->ll.y;
		fur.x = dcpX0 + bb->ur.x;
		fur.y = dcpY0 + bb->ur.y;
		if (llx > fll.x)
		  llx = fll.x;
		if (lly > fll.y)
		  lly = fll.y;
		if (urx < fur.x)
		  urx = fur.x;
		if (ury < fur.y)
		  ury = fur.y;
		/* grow bbox to protect against rounding errors */
		llx -= 0x10000;
		lly -= 0x10000;
		urx += 0x10000;
		ury += 0x10000;
		if (llx > urx || lly > ury)	/* out of order means
						 * overflow */
			goto realslow;
	}
	StringMark(showCache, scipsave-showCache, llx, lly, urx, ury);
	FixedMoveTo(ss->fdcp);
	return 0;
}
realslow:
ss->useReal = true;
slow:
if (xyshow) {
  register PShowState ssr = ss;
  ssr->xymax = xymax;
  ssr->mtx_a = mtx_a; ssr->mtx_b = mtx_b;
  ssr->mtx_c = mtx_c; ssr->mtx_d = mtx_d;
  }
/* build chars and/or compute in reals */
  return -1;
}


public procedure CompSlowShow ()
{
  register integer i, lenshow;
  integer c, ufc, ufi, subst, temp, f;
  Object ob;
  Fixed ufw, ufh;
  Mtx cmtx, mtxtemp;
  Cd width;
  UniqueMID *mtmid, oldMID;
  boolean hnftemp, retval, nomtx;
  register ShowState *ssr = ss;
  SSState SSStack [MaxSSS];
  register SSStatePtr SPtr;
  SSStatePtr SPtrSave;
  PMTItem CFmp;
  NameObj cn;
  unsigned char *svptr;
  integer EscapeChar = gs->escapechar;

  oldMID = gs->matrixID;
  lenshow = ssr->so.length;
  if (lenshow == 0) return;
  if (ssr->xShow || ssr->yShow) ssr->ns = ssr->initns;

  SPtrSave = SPtr = &SSStack [0];

  CrMtx(&cmtx);
  ssr->havefont = false;
  ssr->DelayedFont = NOLL;
  nomtx = true;
  SPtr->fdT = ssr->fontDict;
  SPtr->encaryT = ssr->encAry;
  SPtr->kvaryT = ssr->kvary;
  SPtr->svaryT = ssr->svary;
  SPtr->fdaryT = ssr->fdary;
  SPtr->miaryT = ssr->miary;
  SPtr->pearyT = ssr->peary;
  SPtr->fmT = ssr->fmap;

  CFmp = &MT [(curMT != NIL) ? crMID : MIDNULL];
  i = 0;

  while (i < lenshow)
  {
    if (retval = (GetAbort() != 0)) break;
    SPtr = SPtrSave;
    f = SPtr->fontno;
  Rep1:
    SSGetChar (ufc);
    if (ufc == EscapeChar)
    {
    Rep0:
      if (SPtr->fmT == FMapEscape)
      {
	SSGetChar (ufc);
	if (ufc != EscapeChar)
	{
	  SPtrSave++;
	  GetInfo (ufc);
	  SPtr->fontno = f = ufc << 8;
	  goto Rep1;
	}
      }
      if ((SPtr != &SSStack [0]) && ((SPtr-1)->fmT == FMapEscape))
      {
	SPtrSave--;
	SPtr = SPtrSave;
	f = SPtr->fontno;
	goto Rep0;
      }
    }
    goto Lab0;
  Lab9:
    GetInfo (temp);
  Lab0:
    switch (SPtr->fmT)
    {
    case FMapNone:
      break;
    case FMap17:
      temp = ufc >> 7;
      f = temp << 7;
      ufc &= 0x7f;
      goto Lab9;
    case FMap88:
      temp = ufc;
      f = temp << 8;
      SSGetChar (ufc);
      goto Lab9;
    case FMap97:
      temp = ufc << 1;
      SSGetChar (ufc);
      temp |= (ufc >> 7);
      f = temp << 7;
      ufc &= 0x7f;
      goto Lab9;
    case FMapEscape:
      SPtrSave++;
      SPtr->fontno = 0;
      f = temp = 0;
      goto Lab9;
    case FMapGen:
      svptr = SPtr->svaryT.val.strval;
      f = ufi = *svptr++;
      while (ufi-- != 0) {SSGetChar (c); ufc = (ufc<<8) + c;}
      ufi = (integer)svptr + SPtr->svaryT.length - 1;
      for (temp = 0; (integer)svptr != ufi; temp++)
      {
	c = f; subst = 0;
	while (c-- >= 0) subst = (subst<<8) + *svptr++;
	if (ufc < subst) break;
	ufc -= subst;
      }
      f = temp << 8;
      goto Lab9;
    }
    {
      register UniqueMID *pm = ((SPtr-1)->miaryT + SPtr->mdT);
      pfont = (SPtr-1)->fdT;
      gs->fontDict = SPtr->fdT;
      gs->encAry = SPtr->encaryT;
      gs->fmap = FMapNone;
      gs->matrixID = *pm;
      if ((pm->stamp == MIDNULL) || (MT[pm->rep.mid].umid.stamp != pm->stamp)) {
	DelayedMakeFont ();
	if (curMT != NULL) {
	  register PMTItem mp = curMT;
	  *pm = curMT->umid;
	  CFmp->monotonic &= mp->monotonic;
	  if (mp->values.sf.bb.ll.x < CFmp->values.sf.bb.ll.x)
	    CFmp->values.sf.bb.ll.x = mp->values.sf.bb.ll.x;
	  if (mp->values.sf.bb.ll.y < CFmp->values.sf.bb.ll.y)
	    CFmp->values.sf.bb.ll.y = mp->values.sf.bb.ll.y;
	  if (mp->values.sf.bb.ur.x > CFmp->values.sf.bb.ur.x)
	    CFmp->values.sf.bb.ur.x = mp->values.sf.bb.ur.x;
	  if (mp->values.sf.bb.ur.y > CFmp->values.sf.bb.ur.y)
	    CFmp->values.sf.bb.ur.y = mp->values.sf.bb.ur.y;
	  CFmp->monotonic1 &= mp->monotonic1;
	  if (mp->values.sf.bb1.ll.x < CFmp->values.sf.bb1.ll.x)
	    CFmp->values.sf.bb1.ll.x = mp->values.sf.bb1.ll.x;
	  if (mp->values.sf.bb1.ll.y < CFmp->values.sf.bb1.ll.y)
	    CFmp->values.sf.bb1.ll.y = mp->values.sf.bb1.ll.y;
	  if (mp->values.sf.bb1.ur.x > CFmp->values.sf.bb1.ur.x)
	    CFmp->values.sf.bb1.ur.x = mp->values.sf.bb1.ur.x;
	  if (mp->values.sf.bb1.ur.y > CFmp->values.sf.bb1.ur.y)
	    CFmp->values.sf.bb1.ur.y = mp->values.sf.bb1.ur.y;
	  }
	}
        if (curMT != NULL) SetMIDAge (crMID);
      }
    cn = VMGetElem(SPtr->encaryT,(cardinal)ufc);
    retval = ShowByName (ufc, &cn, f, CFmp);
    if (retval) {
      DMFRestore ();
      break;  /* just quit; leave exception pending */
      }
    if (ssr->cShow) {
      Cd cd;
      PushInteger ((integer) ufc);
      IDTfmP (ssr->rdinc, &cd);
      PushPCd (&cd);
      if (SSSOutCall()) {
	DMFRestore ();
	break;  /* just quit; leave exception pending */
	}
      }
    DMFRestore ();
    }
  SSRestoreState ();
  MoveToAfterShow(ssr);
  gs->matrixID = oldMID;
  }


