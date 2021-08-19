 /*
  fastshow.c

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
Ed Taft: Fri Jul 29 09:06:59 1988
Jim Sandman: Mon Oct 16 10:55:57 1989
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
#include "fontcache.h"

/* The following procedures are extern'd here since	*/
/* they're not exported by the language/grfx package.	*/
extern procedure StringMark();

public DevMask *showCache;

#define FSCacheSz (4)	/* id: Was 8, but reduced to save 2K malloc space */
private MID FSCacheMID[FSCacheSz];
private PAryObj FSCacheEnc[FSCacheSz];
private CIOffset *FSCacheCIO;
private integer FSCacheAge[FSCacheSz], fsAge, fsRecent;

public procedure PurgeFSCache(mid) register MID mid; {
  /* called by fontcache */
  register integer i;
  register MID *mp;
  mp = FSCacheMID;
  for (i = 0; i < FSCacheSz; i++) {
    if (*mp == mid || mid == MIDNULL) { *mp = MIDNULL; }
    mp++;
    }
  }

FasterShow(CIOffset *fscachecio, unsigned char *cp, int lengthActual,
	   int dcpX, int dcpY, int *ip)
{
    CIOffset *fsc;
    Fixed boundary, fixhalf;
    PCIItem cip;
    CIOffset cio;
    CharMetrics *cmet;
    DevMask  *scip;
    DevCd *fdcp;
    int i, sixteen = 16;

    fixhalf = 1 << (FRACTION-1);
    i = lengthActual;
    scip = showCache;
    fsc = fscachecio;
    if (ip)
	do {
	    cio = fsc[*cp++];
	    if (cio == 0) return -1;
	    cip = (CIItem *)((int)CI + cio * (short)sizeof(CIItem));
	    SetCharAge(cip);
	    DebugAssert(cip->cmp->maskID == gs->device->maskID);
	    cmet = &(cip->metrics[0]);
	    if (cip->cmp->width != 0) {
		scip->mask = cip->cmp;
		scip->dc.x = (dcpX + cmet->offset.x + fixhalf) >> sixteen;
		scip->dc.y = (dcpY + cmet->offset.y + fixhalf) >> sixteen;
		scip++;
	    }
	    {
		int incx;
		if ((incx = *ip++) < 0) return -1;
		dcpX += incx << sixteen;
	    }
	} while (--i != 0);
    else
	do {
	    cio = fsc[*cp++];
	    if (cio == 0) return -1;
	    cip = (CIItem *)((int)CI + cio * (short)sizeof(CIItem));
	    SetCharAge(cip);
	    DebugAssert(cip->cmp->maskID == gs->device->maskID);
	    cmet = &(cip->metrics[0]);
	    if (cip->cmp->width != 0) {
		scip->mask = cip->cmp;
		scip->dc.x = (dcpX + cmet->offset.x + fixhalf) >> sixteen;
		scip->dc.y = (dcpY + cmet->offset.y + fixhalf) >> sixteen;
		scip++;
	    }
	    dcpX += cmet->incr.x;
	    dcpY += cmet->incr.y;
	} while (--i != 0);

    fdcp = &ss->fdcp;
    lengthActual = (unsigned short)((int)scip - (int)showCache)/sizeof(*scip);
    if (lengthActual) {
	register PMTItem mp;
	register integer llx, lly, urx, ury;

	/* grow bbox to protect against rounding errors */
	mp = curMT;
	llx = fdcp->x + mp->values.sf.bb.ll.x - 0x10000;
	lly = fdcp->y + mp->values.sf.bb.ll.y - 0x10000;
	urx = dcpX    + mp->values.sf.bb.ur.x + 0x10000;
	ury = dcpY    + mp->values.sf.bb.ur.y + 0x10000;

	/* check for cp entering giggle weeds */
	boundary = FIXINT(UPPERSHOWBOUND); 
	if (llx > urx || lly > ury || urx > boundary || ury > boundary)
	    return -1;

	StringMark(showCache, lengthActual, llx, lly, urx, ury);
    }
    fdcp->x = dcpX;
    fdcp->y = dcpY;
    FixedMoveTo(*fdcp);
    return 0;
}

extern Fixed HF4F();

public integer FastShow(showchars) boolean showchars; {
  /* Returns -1 if some character is not in cache or if computations must be
     done in floating point.  If showchars was true and no clipping was
     required, it returns 0.  Otherwise it returns the value of lengthActual.
  */

  register integer dcpX, dcpY; /* private copy of ss->fdcp */
  register DevMask  *scip;
  boolean widthAdjust;
  integer lengthActual;
  integer dcpX0, dcpY0; /* dcpX, dcpY for last character in string */
  character ch;
  boolean xyshow;
  integer llx, lly, urx, ury;
  Fixed xymax, mtx_a, mtx_b, mtx_c, mtx_d;
  MtxType mtxtype;
  PShowState ssr;
  integer curwm = gs->writingmode;
  int *ip = NULL;

  { /* start inner block so registers can be shared with other blocks */
  register integer i;
  register charptr cp;
  PNameEntry pne;
  PNameObj cnptr;
  PAryObj enc;
  PCIOffset fscachecio;
  boolean cacheCIPs = true;
  boolean skipsearch = false;

    { /* inner block for temporary new register declaration */
    register PShowState ssr = ss;
    widthAdjust = ssr->wShow || ssr->aShow;
    dcpX = ssr->fdcp.x; dcpY = ssr->fdcp.y;
    cp = ssr->so.val.strval;
    lengthActual = ssr->so.length;
    enc = gs->encAry.val.arrayval;
    if (!ssr->xShow && !ssr->yShow) xyshow = false;
    else {
      Mtx *mtx = &gs->matrix;
      real mtxmax, mtxabs;
      Fixed fmtxmax;
      xyshow = true;
      llx = urx = dcpX;
      lly = ury = dcpY;
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

  scip = showCache;
  
  {
  register MID *fscachemid;
  register PAryObj *fscacheenc, e;
  register MID cm;
  cm = crMID; e = enc;
  fscacheenc = FSCacheEnc;
  fscachemid = FSCacheMID;
  fscachecio = FSCacheCIO;
  { register integer recent = fsRecent;
    if (fscachemid[recent]==cm && fscacheenc[recent]==e) {
      fscachecio = (PCIOffset)((integer)fscachecio + (recent << 9));
      goto search; }
    }
  for (i = 0; i < FSCacheSz; i++) {
    if (*fscachemid==cm && *fscacheenc==e) {
      FSCacheAge[i] = fsAge; fsRecent = i; goto search; }
    fscachemid++; fscacheenc++; fscachecio += 256;
    }
  fscachemid = FSCacheMID;
  { register integer oldest = 0;
  if (*(fscachemid++) != MIDNULL) {
    integer age = FSCacheAge[0];
    for (i = 1; i < FSCacheSz; i++) {
      if (*(fscachemid++)==MIDNULL) { oldest = i; break; }
      if (FSCacheAge[i] < age) { age = FSCacheAge[i]; oldest = i; }
      }
    }
  FSCacheMID[oldest] = cm;
  FSCacheEnc[oldest] = e;
  FSCacheAge[oldest] = fsAge++;
  fsRecent = oldest;
  fscachecio = (PCIOffset)((integer)FSCacheCIO + (oldest << 9));
  os_bzero((char *)fscachecio, (long int)512);
  skipsearch = true;
  }}

 search:
  /* Special case standard show and xshow with nondecreasing x & y */
   if (!curwm && !widthAdjust && !ss->yShow && showchars &&
      !curMT->xdec && !curMT->ydec && 
      (!ss->xShow || ((mtxtype == identity || mtxtype == flipY) &&
      		      ss->ns.GetFixed == HF4F && ss->ns.scale == 0 &&
#if 0
                      !((int)(ip = (int *)ss->ns.str) & 0x3))) &&
#else
                      (ip = (int *)ss->ns.str))) &&
#endif
      FasterShow(fscachecio, cp, lengthActual, dcpX, dcpY, ip) == 0) return 0;
  {
  register Fixed boundary, fixhalf;
  register PCIItem cip;
  boundary = FIXINT(LOWERSHOWBOUND); fixhalf = 1<<(FRACTION-1);
  i = lengthActual;

  if (lengthActual <= 1 || xyshow || skipsearch) goto hardWay;
  
  do{  /* use FSCache to setup showCache */
    { register CIOffset cio;
      cio = fscachecio[*cp++];
      if (cio == 0) { cp--; goto hardWay; }
      cip = &CI[cio];
      SetCharAge(cip);
      DebugAssert(cip->cmp->maskID == gs->device->maskID);
      }
    { register CharMetrics *cmet = &(cip->metrics[curwm]);
      if (cip->cmp->width != 0) {
        scip->mask = cip->cmp;
        scip->dc.x = (dcpX + cmet->offset.x + fixhalf) >> FRACTION;
        scip->dc.y = (dcpY + cmet->offset.y + fixhalf) >> FRACTION;
        scip++;
        dcpX0 = dcpX; dcpY0 = dcpY;
	}
      else lengthActual--;
      dcpX += cmet->incr.x;
      dcpY += cmet->incr.y;
      }
    if (widthAdjust) {
      register PShowState ssr = ss;
      dcpX += ssr->fdA.x;
      dcpY += ssr->fdA.y;
      if (ssr->wShow && (*(cp-1) == ssr->wChar)){
        dcpX += ssr->fdW.x; dcpY += ssr->fdW.y;}}
    /* check for cp entering giggle weeds */
    if ((dcpX < boundary) || (dcpY < boundary)) 
      goto realslow;
    boundary = -boundary;
    if ((dcpX > boundary) || (dcpY > boundary))
      goto realslow;
    boundary = -boundary;
    } while (--i != 0);
  goto strDone;
  
 hardWay:
  cnptr = gs->encAry.val.arrayval;
  do { /* character loop */
    ch = *cp++;
      { /* inner block for temporary new register declaration */
      register PNameObj pno = &cnptr[ch];
      if (pno->type != nameObj) goto slow;
      pne = pno->val.nmval;
      }
    /* do not bother testing whether the ncilink is CINULL, since the
       CI entry corresponding to CINULL is a valid but vacant entry */
    cip = CI + pne->ncilink; /* i.e. &CI[cio] */
    if (cip->mid == crMID) goto foundchar;
    while (true){
      register PCIItem ocip = cip;
      register CIOffset cio;
      if ((cio = cip->cilink) == CINULL) goto slow;
      cip = CI + cio; /* i.e. &CI[cio] */
      if (cip->mid == crMID) {
        ocip->cilink = cip->cilink;
        cip->cilink = pne->ncilink;
        pne->ncilink = cio;
        break;
	}
      }
    foundchar:
      {
      { register CharMetrics *cmet = &(cip->metrics[curwm]);
        DebugAssert(cip->cmp->maskID == gs->device->maskID);
        SetCharAge(cip);
        if (cacheCIPs) fscachecio[ch] = pne->ncilink;
        if (cip->cmp->width != 0) {
          scip->mask = cip->cmp;
          scip->dc.x = (dcpX + cmet->offset.x + fixhalf) >> FRACTION;
          scip->dc.y = (dcpY + cmet->offset.y + fixhalf) >> FRACTION;
          scip++;
          dcpX0 = dcpX; dcpY0 = dcpY;
	  }
        else lengthActual--;
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
	  if (os_labs(incx) > xymax) goto slow;
	  }
	if (!ssr->yShow) incy = 0;
	else {
          incy = (*ssr->ns.GetFixed)(&ssr->ns);
	  if (os_labs(incy) > xymax) goto slow;
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
        dcpX += incx;
	dcpY += incy;
        if (incx < 0) { if (dcpX < llx) llx = dcpX; }
	else          { if (dcpX > urx) urx = dcpX; }
        if (incy < 0) { if (dcpY < lly) lly = dcpY; }
	else          { if (dcpY > ury) ury = dcpY; }
        }
      if (widthAdjust) {
        register PShowState ssr = ss;
        dcpX += ssr->fdA.x;
        dcpY += ssr->fdA.y;
        if (ssr->wShow && (*(cp-1) == ssr->wChar)){
          dcpX += ssr->fdW.x; dcpY += ssr->fdW.y;}
      }
      /* check for cp entering giggle weeds */
      if (dcpX < boundary || dcpY < boundary) goto realslow;
      boundary = -boundary;
      if (dcpX > boundary || dcpY > boundary) goto realslow;
      boundary = -boundary;
      }
    } while (--i != 0);
  }
  }

 strDone:
    ssr = ss;
    if (showchars && lengthActual) {
	register PMTItem mp;
	DevCd           fll, fur;

	mp = curMT;
	/*
	 * ss->fdcp.x and ss->fdcp.y give the initial character position;
	 * dcpX0 and dcpY0 give the final character position.  This test
	 * assumes that the shape enclosing the bounding boxes of the first
	 * and last characters also encloses the bounding boxes of all the
	 * characters in between.  This assumption is valid since FastShow is
	 * only used with fonts whose characters have width vectors that are
	 * consistent in direction. 
	 */
	if (xyshow) {
	    llx += mp->values.sf.bb.ll.x - 0x10000;
	    lly += mp->values.sf.bb.ll.y - 0x10000;
	    urx += mp->values.sf.bb.ur.x + 0x10000;
	    ury += mp->values.sf.bb.ur.y + 0x10000;
	} else {
	    llx = ssr->fdcp.x + mp->values.sf.bb.ll.x;
	    lly = ssr->fdcp.y + mp->values.sf.bb.ll.y;
	    urx = ssr->fdcp.x + mp->values.sf.bb.ur.x;
	    ury = ssr->fdcp.y + mp->values.sf.bb.ur.y;
	    fll.x = dcpX0 + mp->values.sf.bb.ll.x;
	    fll.y = dcpY0 + mp->values.sf.bb.ll.y;
	    fur.x = dcpX0 + mp->values.sf.bb.ur.x;
	    fur.y = dcpY0 + mp->values.sf.bb.ur.y;
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
	    if (llx > urx || lly > ury)
	        goto realslow;
	}
	ssr->fdcp.x = dcpX;
	ssr->fdcp.y = dcpY;	/* update for FixedMoveTo */
	StringMark(showCache, lengthActual, llx, lly, urx, ury);
	FixedMoveTo(ssr->fdcp);
    }
    else {
	ssr->fdcp.x = dcpX;
	ssr->fdcp.y = dcpY;	/* update for FixedMoveTo */
	if (showchars) FixedMoveTo(ssr->fdcp);
    }
    return 0;

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


public procedure FSInit() {
  showCache = (DevMask *) NEW(sizeShowCache, sizeof(DevMask));
  FSCacheCIO = (CIOffset *) NEW((256 * FSCacheSz), sizeof(CIOffset));
  }
