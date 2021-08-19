 /*
  cachedev.c

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
Ed Taft: Fri Jul 29 09:06:59 1988
Jim Sandman: Mon Apr 16 10:38:27 1990
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

#include "fontshow.h"
#include "fontdata.h"
#include "fontsnames.h"

#define fntmtx (fontCtx->fontBuild._fntmtx)
#define fMetrics (fontCtx->fontBuild._fMetrics)

#if DPSXA
extern long int xChunkOffset, yChunkOffset;
#endif DPSXA
extern SetBBCompMark();

public boolean CvtToFixed(pfcd,cd,lower,upper)
  register FCd *pfcd; Cd cd; integer lower,upper;
{
  pfcd->x = pflttofix(&cd.x);
  pfcd->y = pflttofix(&cd.y);
  return OkFixed(pfcd,lower,upper);
}


private procedure SetCharWidth(width)  Cd width; {
  register ShowState* ssr = ss;
  Mtx crmtx;
  FCd fdinc;
  CrMtx(&crmtx);
  DTfmPCd(width, &crmtx, &ssr->rdinc);
  if (CvtToFixed(&fdinc, ssr->rdinc, LOWERSHOWBOUND, UPPERSHOWBOUND)) {
    /* width will be cached as a Fixed, so make sure will give same result */
    fixtopflt(fdinc.x, &ssr->rdinc.x);
    fixtopflt(fdinc.y, &ssr->rdinc.y);}
  }  /* end of SetCharWidth */


public boolean FSetCharWidth(w0, w1, v)  FCd w0, w1, v; {
  register ShowState* ssr = ss;
  FCd width;
  if (gs->writingmode == 0) {
    width = w0;
    }
  else {
    width = w1;
    }
  fixtopflt(width.x, &ssr->rdinc.x);
  fixtopflt(width.y, &ssr->rdinc.y);
  ssr->fdinc.x = width.x;
  ssr->fdinc.y = width.y;
  ssr->hMetrics.incr.x = w0.x;
  ssr->hMetrics.incr.y = w0.y;
  ssr->vMetrics.incr.x = w1.x;
  ssr->vMetrics.incr.y = w1.y;
  ssr->vMetrics.offset.x = v.x;
  ssr->vMetrics.offset.y = v.y;
  }  /* end of FSetCharWidth */


private procedure GetCPDelta(ssr, delta) register ShowState* ssr; PCd delta; {
  if (!ssr->useReal) UNFIXCD(ssr->fdcp, &ssr->rdcp);
  delta->x = os_floor(ssr->rdcp.x + fpHalf);
  delta->y = os_floor(ssr->rdcp.y + fpHalf);
  }


private integer SetupMaskDev(ll, ur, saveScale) FCd ll, ur; boolean saveScale; {
  register ShowState* ssr = ss;
  PDevice device;
  integer scale = gs->scale;
  MakeMaskDevArgs args;
  if ((ll.x == ur.x) && (ll.y == ur.y))  {    /* empty bitmap */
    args.height = args.width = 0;
    }
  else
    {
    args.height = FCeil(ur.y - ll.y);
    args.width = FCeil(ur.x - ll.x);
    }
  ssr->cio = CIAlloc(false);
  args.cacheThreshold = ctxCacheThreshold;
  args.compThreshold = ctxCompThreshold;
  args.mtx = &gs->matrix;
  args.mask = &ssr->mask;
  args.offset = &ssr->hMetrics.offset;
  device = (*gs->device->procs->MakeMaskDevice)(gs->device, &args);
  if (args.width == 0 && ssr->mask != NULL) {
    Assert(gs->device->maskID == ssr->mask->maskID);
    return 0;
    }
  if (device == NULL) {
    return -1;
    }
  Assert(gs->device->maskID == device->maskID);
    /* The mask device must have the same maskID as the original device */
  SetMaskDevice(device);
  if (saveScale)
    gs->scale = scale;
  return 1;
  }

private procedure SetDeltaAndTlate(delta, rcd) RCd *delta, rcd; {
  *delta = rcd;
  IDTfmP(rcd, &rcd);
  Tlat(rcd);
  RRoundP(&gs->matrix.tx, &gs->matrix.tx);
  RRoundP(&gs->matrix.ty, &gs->matrix.ty);
  }

private boolean NoRoom(delta, v) RCd *delta, v; {
  register ShowState* ssr = ss;
  RCd rcd;
  if (ssr->noShow) {delta->x = delta->y = fpZero; return false;}
  else
    {
    if (gs->writingmode == 0) 
      GetCPDelta(ssr, &rcd);
    else {
      if (!ssr->useReal) UNFIXCD(ssr->fdcp, &ssr->rdcp);
      rcd.x = os_floor (ssr->rdcp.x - v.x + fpHalf);
      rcd.y = os_floor (ssr->rdcp.y - v.y + fpHalf);
      }
    SetDeltaAndTlate(delta, rcd);
    }
  return true;
  }

private boolean SetCch(ll, ur, delta) FCd ll, ur; PRCd delta; {
  FCd devorigin;
  Cd rcd;
  register ShowState* ssr = ss;

#if DPSXA
  if((ssr->cmr.height >= os_abs(yChunkOffset)) || 
  	     (ssr->cmr.width >= os_abs(xChunkOffset))) goto noroom;
#endif /* DPSXA */
  if (gs->isCharPath || curMT == NIL) goto noroom;
  devorigin.x = FRoundF(-ll.x); 
  devorigin.y = FRoundF(-ll.y);
  fixtopflt(devorigin.x, &rcd.x);
  fixtopflt(devorigin.y, &rcd.y);
  SetDeltaAndTlate(delta, rcd);
  ssr->hMetrics.offset.x = -devorigin.x;
  ssr->hMetrics.offset.y = -devorigin.y;
  switch (SetupMaskDev(ll, ur, false)) {
    case -1: {
      IDTfmP(rcd, &rcd);
      rcd.x = -rcd.x;
      rcd.y = -rcd.y;
      Tlat(rcd);
      goto noroom;
      };
    case 0: {
      return true;
      }
    default: {
      break;
      }
    }
  gs->cp.x = gs->cp.y = fpZero;
  TfmP(gs->cp, &gs->cp);
  return true;
noroom:
  fixtopflt(ssr->vMetrics.offset.x, &rcd.x);
  fixtopflt(ssr->vMetrics.offset.y, &rcd.y);
  return NoRoom(delta, rcd);
  }  /* end of SetCch */


private boolean DTfmToFixed(fcd, rcd) PFCd fcd; RCd rcd; {
  Mtx mtx;
  CrMtx(&mtx);
  DTfmPCd(rcd, &mtx, &rcd);
  fcd->x = pflttofix(&rcd.x);
  fcd->y = pflttofix(&rcd.y);
  return OkFixed(fcd,LOWERSHOWBOUND,UPPERSHOWBOUND);
  }

private boolean TfmToBB(fll, fur, rll, rur) PFCd fll, fur; RCd rll, rur; {
  real r1, r2;
  Cd tll, tur, tlr, tul;
  Mtx mtx;
  CrMtx(&mtx);
  TfmPCd(rll, &mtx, &tll);
  TfmPCd(rur, &mtx, &tur);
  tlr.x = rur.x; tlr.y = rll.y;
  tul.x = rll.x; tul.y = rur.y;
  TfmPCd(tlr, &mtx, &tlr);
  TfmPCd(tul, &mtx, &tul);
  r1 = MAX(tll.x,tur.x);
  r2 = MAX(tlr.x,tul.x);
  r1 = MAX(r1, r2);
  fur->x = pflttofix(&r1);
  r1 = MAX(tll.y,tur.y);
  r2 = MAX(tlr.y,tul.y);
  r1 = MAX(r1, r2);
  fur->y = pflttofix(&r1);
  r1 = MIN(tll.x,tur.x);
  r2 = MIN(tlr.x,tul.x);
  r1 = MIN(r1, r2);
  fll->x = pflttofix(&r1);
  r1 = MIN(tll.y,tur.y);
  r2 = MIN(tlr.y,tul.y);
  r1 = MIN(r1, r2);
  fll->y = pflttofix(&r1);
  return (
    OkFixed(fll,LOWERSHOWBOUND,UPPERSHOWBOUND) &&
    OkFixed(fur,LOWERSHOWBOUND,UPPERSHOWBOUND));
  }

public boolean SetCchDevice(m, delta) RMetrics *m; RCd *delta; {
  FCd fw0, fll, fur, fw1, fv;
  if (!DTfmToFixed(&fw0, m->w0) ||
      !DTfmToFixed(&fv, m->v) ||
      !DTfmToFixed(&fw1, m->w1) ||
      !TfmToBB(&fll, &fur, m->c0, m->c1)) {
    RCd rcd;
    Mtx crmtx;
    SetCharWidth((gs->writingmode == 0) ? m->w0 : m->w1);
    CrMtx(&crmtx);
    DTfmPCd (m->v, &crmtx, &rcd);
    return NoRoom(delta, rcd);
  }
  FSetCharWidth(fw0, fw1, fv);
  return SetCch(fll, fur, delta);
}


public boolean MakeCacheDev(fm) FMetrics *fm; {
  register ShowState* ssr = ss;
  (fntmtx.dtfm)(fm->w0, &fm->w0);
  (fntmtx.dtfm)(fm->w1, &fm->w1);
  (fntmtx.dtfm)(fm->v, &fm->v);
  FSetCharWidth(fm->w0, fm->w1, fm->v);
  FontPathBBox(&fm->c0, &fm->c1);
  if (ssr->useReal) FIXCD(ssr->rdcp, &ssr->fdcp);
  if (OkFixed(&fm->c0, LOWERSHOWBOUND, UPPERSHOWBOUND) &&
      OkFixed(&fm->c1, LOWERSHOWBOUND, UPPERSHOWBOUND) &&
      OkFixed(&ssr->hMetrics.incr, LOWERSHOWBOUND, UPPERSHOWBOUND) &&
      OkFixed(&ssr->vMetrics.incr, LOWERSHOWBOUND, UPPERSHOWBOUND) &&
      OkFixed(&ssr->vMetrics.offset, LOWERSHOWBOUND, UPPERSHOWBOUND)) {
    ssr->hMetrics.offset.x = -FRoundF(-fm->c0.x);
    ssr->hMetrics.offset.y = -FRoundF(-fm->c0.y);
    switch (SetupMaskDev(fm->c0, fm->c1, true)) {
      case -1: {
	if (ssr->noShow) {
	  return false;
	}
	ssr->delta.x = ssr->fdcp.x;
	ssr->delta.y = ssr->fdcp.y;
	if (gs->writingmode != 0) {
	  ssr->delta.x -= fm->v.x;
	  ssr->delta.y -= fm->v.y;
	  }
	SetBBCompMark(&ssr->delta);
	return true;
	};
      case 0: {
	return true; /* char was empty. have mask and are done. */
	}
      default: {
	ssr->delta.x = -ssr->hMetrics.offset.x;
	ssr->delta.y = -ssr->hMetrics.offset.y;
	return true;
	}
      }
    }
  else {
    ssr->delta.x = ssr->fdcp.x;
    ssr->delta.y = ssr->fdcp.y;
    if(!ssr->noShow)
		SetBBCompMark(&ssr->delta);
    return !ssr->noShow;
    };
  }


public procedure CSRun(run) DevRun *run; {
  register ShowState* ssr = ss;
  TransDevRun(run, FRound(ssr->delta.x), FRound(ssr->delta.y));
  AddRunMark(run);
  }


public boolean MakeCacheDev2(fm) FMetrics *fm; {
  register ShowState* ssr = ss;
  RMetrics m;
  FMetrics mfm; /* modified metrics */
  RCd rcd;
  boolean result;
  Mtx mtx;
  CrMtx(&mtx);
  FontPathBBox(&fm->c0, &fm->c1);
  (*fntmtx.itfm)(fm->c0, &mfm.c0);
  (*fntmtx.itfm)(fm->c1, &mfm.c1);
  PFCdToPRCd(&mfm.c0, &m.c0);
  PFCdToPRCd(&mfm.c1, &m.c1);  
  PFCdToPRCd(&fm->w0, &m.w0);
  PFCdToPRCd(&fm->w1, &m.w1);
  PFCdToPRCd(&fm->v, &m.v);
  ModifyCachingParams(gs->fontDict, ss->pcn, &m);
  if (!DTfmToFixed(&mfm.w0, m.w0) ||
      !DTfmToFixed(&mfm.v, m.v) ||
      !DTfmToFixed(&mfm.w1, m.w1)) {
    SetCharWidth((gs->writingmode == 0) ? m.w0 : m.w1);
    if (ss->noShow) 
      return false;
    else {
      Mtx crmtx;
      CrMtx(&crmtx);
      if (gs->writingmode == 0) 
	       GetCPDelta(ssr, &rcd);
      else {
		if (!ssr->useReal) UNFIXCD(ssr->fdcp, &ssr->rdcp);
		DTfmPCd (m.v, &crmtx, &rcd);
		rcd.x = os_floor (ssr->rdcp.x - rcd.x + 0.5);
		rcd.y = os_floor (ssr->rdcp.y - rcd.y + 0.5);
	  }
      PRCdToPFCd(&rcd, &ssr->delta);
      SetBBCompMark(&ssr->delta);
      return true;
      }
    }
  else {
	TfmPCd(m.c0, &mtx, &rcd);
	mfm.c0.x = pflttofix(&rcd.x);
	mfm.c0.y = pflttofix(&rcd.y);
	TfmPCd(m.c1, &mtx, &rcd);
	mfm.c1.x = pflttofix(&rcd.x);
	mfm.c1.y = pflttofix(&rcd.y);
    FSetCharWidth(mfm.w0, mfm.w1, mfm.v);
    ssr->hMetrics.offset.x = -FRoundF(-mfm.c0.x);
    ssr->hMetrics.offset.y = -FRoundF(-mfm.c0.y);
    switch (SetupMaskDev(fm->c0, fm->c1, true)) {
      case -1: {
		if (ssr->noShow)
		  return false;
		ssr->delta.x = ssr->fdcp.x + mfm.c0.x - fm->c0.x;
		ssr->delta.y = ssr->fdcp.y + mfm.c0.y - fm->c0.y;
		if (gs->writingmode != 0) {
		  ssr->delta.x -= mfm.v.x;
		  ssr->delta.y -= mfm.v.y;
		  }
		SetBBCompMark(&ssr->delta);
		break;
	  }
      case 0: {
		return true; /* char was empty. have mask and are done. */
	  }
      default: {
		ssr->delta.x = FRoundF(-fm->c0.x);
		ssr->delta.y = FRoundF(-fm->c0.y);
		break;
	  }
    }
    return true;
  }
}


private procedure SetCacheDev(m, hasVertMetrics)
  RMetrics *m; boolean hasVertMetrics; {
  RCd delta;
  if (ss == NIL || ! ss->inBuildChar) Undefined();
  PopPCd(&m->c1);
  PopPCd(&m->c0);
  PopPCd(&m->w0);
  if (!hasVertMetrics) {
    m->w1 = m->w0;
    m->v.x = m->v.y = fpZero;
    }
  if (!SetCchDevice(m, &delta))
    NullDevice();
  }

public procedure PSSetCchDevice2() {
  RMetrics m;
  PopPCd(&m.v);
  PopPCd(&m.w1);
  SetCacheDev(&m, true);
  }


public procedure PSSetCchDevice() {
  RMetrics m;
  SetCacheDev(&m, false);
  }


public procedure PSSetCharWidth() {
  Cd cd;
  register ShowState* ssr = ss;
  if (ssr == NIL || ! ssr->inBuildChar) Undefined();
  PopPCd(&cd);
  SetCharWidth(cd);
  if (ssr->noShow) NullDevice();
  else
    {
    if (!ssr->useReal) UNFIXCD(ssr->fdcp, &ssr->rdcp);
    IDTfmP(ssr->rdcp, &cd);
    Tlat(cd);
    }
  }  /* end of PSSetCharWidth */


/*

 TO DO:

   1) The vMetrics.offset contains v. Need to make it contain hMetrics - v in
     ShowByName.


*/ 

