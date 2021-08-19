/*
				 pathpriv.c

   Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE: All  information contained herein is  the  property of Adobe  Systems
Incorporated.  Many  of  the  intellectual  and technical concepts  contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for  their  internal use.  Any reproduction
or dissemination of this software is  strictly forbidden unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Doug Brotz, May 11, 1983
Edit History:
Scott Byer: Thu May 18 12:57:19 1989
Doug Brotz: Thu Dec 11 17:23:20 1986
Chuck Geschke: Thu Oct 31 15:22:56 1985
Ed Taft: Fri Feb  9 13:32:00 1990
John Gaffney: Tue Feb 12 10:46:11 1985
Ken Lent: Wed Mar 12 15:53:25 1986
Bill Paxton: Tue Feb  6 12:14:39 1990
Don Andrews: Wed Sep 17 15:28:32 1986
Ivor Durham: Sun Aug 14 12:24:09 1988
Joe Pasqua: Mon Mar  6 14:02:14 1989
Linda Gass: Thu Dec  3 18:17:54 1987
Jim Sandman: Tue Mar 27 09:44:05 1990
Paul Rovner: Wednesday, July 6, 1988 2:57:50 PM
Perry Caro: Wed Mar 29 09:55:37 1989
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
#include PSLIB
#include VM

#include "graphdata.h"
#include "path.h"
#include "reducer.h"
#include "stroke.h"
#include "graphicspriv.h"

extern procedure QFNewPoint();

#define BROTZOFILL 0

#define FIXTOFRAC(f) ((f) << 14)

private Fixed FracSqrt2;

/* private global variables. make change in both arms if changing variables */

#if (OS != os_mpw)

/*-- BEGIN GLOBALS --*/
  private FCd ofpSt, markDelta;
  private FCd old_p, old_a, ofaSt, ofLfSt, old_tail;
  private Fixed offsetwidth, erodeConst, ofwSt, old_w;
  private boolean needVec, doOffsetting;
  private procedure (*gProcNewPoint)(), (*gProcRdcClose)();
#if STAGE==DEVELOP
  private Object of_init, of_np, of_cp, of_done;
  private boolean traceOffsetFill;
#endif STAGE==DEVELOP
/*-- END GLOBALS --*/


#else (OS != os_mpw)

typedef struct {
  FCd g_ofpSt;
  FCd g_old_p, g_old_a, g_ofaSt, g_ofLfSt, g_old_tail;
  Fixed g_offsetwidth, g_erodeConst, g_ofwSt, g_old_w;
  boolean g_needVec, g_doOffsetting;
  procedure (*g_newPoint)(), (*g_rdcClose)();
#if STAGE==DEVELOP
  Object g_of_init, g_of_np, g_of_cp, g_of_done;
  boolean g_traceOffsetFill;
#endif STAGE==DEVELOP
  } GlobalsRec, *Globals;

#define ofpSt globals->g_ofpSt
#define old_p globals->g_old_p
#define old_a globals->g_old_a
#define ofaSt globals->g_ofaSt
#define ofLfSt globals->g_ofLfSt
#define old_tail globals->g_old_tail
#define offsetwidth globals->g_offsetwidth
#define erodeConst globals->g_erodeConst
#define ofwSt globals->g_ofwSt
#define old_w globals->g_old_w
#define needVec globals->g_needVec
#define doOffsetting globals->g_doOffsetting
#define gProcNewPoint globals->g_newPoint
#define gProcRdcClose globals->g_rdcClose

#if STAGE==DEVELOP
#define of_init globals->g_of_init
#define of_np globals->g_of_np
#define of_cp globals->g_of_cp
#define of_done globals->g_of_done
#define traceOffsetFill globals->g_traceOffsetFill
#endif STAGE==DEVELOP

private Globals globals;

#endif (OS != os_mpw)


#define FMilli 0x40
#define FCenti 650
#define FracMilli 0x100000
#define FracOne 0x40000000
#define FracHalf 0x20000000
#define FixedOneFive 0x18000
#define FixedTwo 0x20000
#define FracToFixed(f) ((f) >> 14)

private Fixed CDistFixed(a)  FCd a;
{
Fixed max, min, absx, absy;
absx = os_labs(a.x);  absy = os_labs(a.y);
if (absx > absy) {max = absx;  min = absy;} else {max = absy;  min = absx;}
return max + min / 2;
}  /* end of CDistFixed */


#if STAGE==DEVELOP
private procedure psTraceOffsetFill() {
  if (PopBoolean()) {
    PopP(&of_done); PopP(&of_cp); PopP(&of_np); PopP(&of_init);
    traceOffsetFill = true;
    }
  else traceOffsetFill = false;
  }

private procedure Trace_OFInit() {
  if (psExecute(of_init)) RAISE((int)GetAbort(), (char *)NIL);
  }

private procedure Trace_OFNP(cd) Cd cd; {
  PushPCd(&cd);
  if (psExecute(of_np)) RAISE((int)GetAbort(), (char *)NIL);
  }

private procedure Trace_OFCP() {
  if (psExecute(of_cp)) RAISE((int)GetAbort(), (char *)NIL);
  }

private procedure Trace_OFDone() { 
  if (psExecute(of_done)) RAISE((int)GetAbort(), (char *)NIL);
  } 

#endif STAGE==DEVELOP

private procedure OFIntersect(a, p, w, r)  FCd a, p, *r;  Fixed w;
{
/* "a" is a unit vector composed of two fracs along the new line segment.
 * "p" is the new line segment's end point translated to the left by "w",
 *      expressed as two Fixed's.
 * "w" is the frac inset width.
 *  Compute the new point "c" by the formula
 *    c = p + (w old_a - old_w a) / (old_a dot lfNorm_a).
 */
register Fixed x, y, d;
FCd c, c2, a2;
FCd cd, v, old_v;
boolean left_turn;
extern integer F_VecTurn();
a2 = a;
x = -a.y;  y = a.x;
d = fracmul(old_a.x, x) + fracmul(old_a.y, y);
if (os_labs(d) < FracMilli)
  {
  a.x += old_a.x;  a.y += old_a.y;
  if (CDistFixed(a) > FracHalf) *r = old_p;
  *r = old_tail;
  return;
  }
a.x = fracratio(fracmul(w, old_a.x) - fracmul(old_w, a.x), d);
a.y = fracratio(fracmul(w, old_a.y) - fracmul(old_w, a.y), d);
d = CDistFixed(a);
if (d <= FixedOneFive)  /* common case */
  {
  r->x = old_p.x + a.x;  r->y = old_p.y + a.y;
  return;
  }
left_turn = (F_VecTurn(old_a, a2)==1) ? true : false;
if (offsetwidth < 0) /* mirror transform */
  left_turn = left_turn ? false : true;
a2 = a;
a.x += (a.x >> 1);  a.y += (a.y >> 1); /* extend by 1/2 current length */
a.x = fixdiv(a.x, d); a.y = fixdiv(a.y, d); /* length is now 1.5 pixels */
c2.x = old_p.x + a.x;  c2.y = old_p.y + a.y;
if (left_turn) /* erode inside of sharp angle */
  {
  p.x = old_p.x + a2.x;  p.y = old_p.y + a2.y;
  }
else /* right turn  --  erode outside of sharp angle */
  {
  p = old_tail;
  }
(*gProcNewPoint)(p);
#if STAGE==DEVELOP
if (traceOffsetFill) Trace_OFNP(p);
#endif STAGE==DEVELOP
(*gProcNewPoint)(c2);
#if STAGE==DEVELOP
if (traceOffsetFill) Trace_OFNP(c2);
#endif STAGE==DEVELOP
*r = p;
return;
}  /* end of OFIntersect */


public procedure OFMoveToP(c) PFCd c; {
  ofpSt = old_p = *c;
  needVec = true;
  if (!doOffsetting)
    (*gProcNewPoint)(*c);
  }

private procedure OFLineTo(p)  FCd p;
{
Fixed absvx, absvy, w, d;
FCd v, wLfNorm, lf, a;
if (!doOffsetting) {
  (*gProcNewPoint)(p);
  needVec = false;
  return;
  }

v.x = p.x - old_p.x;  v.y = p.y - old_p.y;
absvx = os_labs(v.x);  absvy = os_labs(v.y);
if (absvx < FMilli)  /* FMilli == real .001;  FCenti == real .01 */
  {
  if (absvy < FMilli) return;
  else if (absvy > FCenti) {a.y = (v.y > 0) ? FracOne : -FracOne; a.x = 0;}
  else goto usesqrt;
  }
else if (absvy < FMilli && absvx > FCenti)
    {a.x = (v.x > 0) ? FracOne : -FracOne;  a.y = 0;}
else
usesqrt:
  {
  if (absvx > absvy) {d = absvx; absvx = absvy; absvy = d;}
    /*  Now absvy >= absvx.
     *  Compute sqrt(x*x + y*y) by |y| * sqrt( 1 + x/y * x/y) */
  d = fixratio(absvx, absvy);
  if (d == FracOne){
    /* set d = absvy * sqrt(2) */
    d = fxfrmul(absvy, FracSqrt2);}
  else{
    d = fxfrmul(absvy, fracsqrt((UFrac)(fracmul(d, d) + FracOne)));}
  a.x = fixratio(v.x, d);  a.y = fixratio(v.y, d);
  }
w = offsetwidth + fracmul(erodeConst, fracmul(os_labs(a.x), os_labs(a.y)));
wLfNorm.x = FracToFixed(fracmul(-w, a.y));
wLfNorm.y = FracToFixed(fracmul(w, a.x));
lf.x = old_p.x + wLfNorm.x;  lf.y = old_p.y + wLfNorm.y;
if (needVec) {ofaSt = a;  ofLfSt = lf;  ofwSt = w;}
else {
  FCd p2;
  OFIntersect(a, lf, w, &p2);
  (*gProcNewPoint)(p2);
#if STAGE==DEVELOP
  if (traceOffsetFill) Trace_OFNP(p2);
#endif STAGE==DEVELOP
  }

old_tail.x = p.x + wLfNorm.x;  old_tail.y = p.y + wLfNorm.y;
old_p = p;  old_a = a;  old_w = w;
needVec = false;
} /* end of OFLineTo */


public procedure OFLineToP(c) PFCd c; {OFLineTo(*c);}

public procedure OFClose()
{
FCd cd;
if (needVec) return;
if (doOffsetting) {
  OFLineTo(ofpSt);
  OFIntersect(ofaSt, ofLfSt, ofwSt, &cd);
  }
else {
  cd = ofpSt;
  }
(*gProcNewPoint)(cd);
(*gProcRdcClose)();
#if STAGE==DEVELOP
if (traceOffsetFill) { Trace_OFNP(cd); Trace_OFCP(); }
#endif STAGE==DEVELOP
needVec = true;
} /* end of OFClose */


#define MaxInt16 ((short int) 0x7FFF)
#define MinInt16 ((short int) 0x8000)

public procedure InitOFill(qrdc, offset, min, max, ow, varcoeff)
  boolean qrdc, offset; integer min, max; Fixed ow, varcoeff; {
  real r;
  doOffsetting = offset;
  if (offset) {
    fixtopflt(ow, &r);
    offsetwidth = FIXTOFRAC(ow);
    fixtopflt(varcoeff, &r);
    erodeConst = pflttofrac(&r);
    r = gs->matrix.c * gs->matrix.b - gs->matrix.a * gs->matrix.d;
    if (RealGt0(r))  {
      offsetwidth = -offsetwidth;  erodeConst = -erodeConst;
      }
    }
  if (qrdc) {
    QResetReducer();
    gProcNewPoint = QFNewPoint;
    gProcRdcClose = QRdcClose;
    }
  else {
    SetRdcScal(max, min);
    ResetReducer();
    RdcClip(false);
    gProcNewPoint = FClNewPt;
    gProcRdcClose = RdcClose;
    }
  InitFontFlat(OFLineTo);
  ms->bbCompMark = overlap; /* always clip since erosion can throw spikes */
  (*ms->procs->initMark)(ms, true);
  }
  
private procedure OFAddRunMark(run) DevRun *run; {
  TransDevRun(run, FRound(markDelta.x), FRound(markDelta.y));
  AddRunMark(run);
  }

private procedure OFAddRdcTrap(yt, yb, xtl, xtr, xbl, xbr)
    Fixed yt, yb, xtl, xtr, xbl, xbr;
    {
    AddTrap(
        RdcToDev(yt)+markDelta.y, RdcToDev(yb)+markDelta.y, 
        RdcToDev(xtl)+markDelta.x, RdcToDev(xtr)+markDelta.x, 
        RdcToDev(xbl)+markDelta.x, RdcToDev(xbr)+markDelta.x);
    }

public procedure OFMark(qrdc, delta) boolean qrdc; FCd delta; {
  markDelta = delta;
  if (qrdc) {
    QReduce(false, OFAddRunMark, MinInt16, MaxInt16, (char *)NIL);
    }
  else {
    Reduce(OFAddRdcTrap, false, false);
    }
  (*ms->procs->termMark)(ms);
  }
  
public procedure OffsetFill(p, ow, varcoeff)
  PPath p; Fixed ow, varcoeff;
{
PPthElt pe;
Cd c0, c1, c2, c3;
FCd startCd, d0, d1, d2, d3;
/*Fixed feps; ...not used */
real mirr;
real reps;
extern boolean QRdcOk();
boolean qRdcr;
FltnRec fr;
integer count;
boolean working;

if (gs->isCharPath
    || (ow == 0 && varcoeff == 0)
    || (p->bbox.bl.x < -fp16k)
    || (p->bbox.tr.x > fp16k) 
    || (p->bbox.bl.y < -fp16k)
    || (p->bbox.tr.y > fp16k))
  {Fill(p, false); return;}
Assert((PathType)p->type == listPth);
if (p->ptr.lp == NULL || p->ptr.lp->head == 0) return;
doOffsetting = true;
fr.report = OFLineTo;
offsetwidth = FIXTOFRAC(ow);
fixtopflt(varcoeff, &reps);
erodeConst = pflttofrac(&reps);
mirr = gs->matrix.c * gs->matrix.b - gs->matrix.a * gs->matrix.d;
if (RealGt0(mirr))  {
  offsetwidth = -offsetwidth;  erodeConst = -erodeConst;
  }
fr.reps = reps;
fr.reportFixed = true;
ms->bbCompMark = overlap; /* always clip since erosion can throw spikes */
if (ow < 0) ow = -ow;
fixtopflt(ow, &reps);
reps = gs->flatEps / (fpOne + fpTwo * reps);
fr.feps = pflttofix(&reps);
#if STAGE==DEVELOP
if (traceOffsetFill) Trace_OFInit();
#endif STAGE==DEVELOP
working = true;
count = 0;
while (working)
{
DURING
working = false;
qRdcr = QRdcOk(p,false);
(*ms->procs->initMark)(ms, true);
if (qRdcr) {
  QResetReducer();
  gProcNewPoint = QFNewPoint;
  gProcRdcClose = QRdcClose;
  }
else {
  integer maxval, minval;
  maxval = MAX(p->bbox.tr.x, p->bbox.tr.y);
  minval = MIN(p->bbox.bl.x, p->bbox.bl.y);
  SetRdcScal(maxval, minval);
  ResetReducer();
  RdcClip(false);
  gProcNewPoint = FClNewPt;
  gProcRdcClose = RdcClose;
  }
for (pe = p->ptr.lp->head; pe != 0; pe = pe->next)
  {
  c1 = pe->coord;
  switch (pe->tag)
    {
    case pathstart:
      if (pe->next == NULL) break;
      FixCd(c1, &startCd);
      OFMoveToP(&startCd);
      break;
    case pathlineto:
      FixCd(c1, &d1);
      OFLineTo(d1);
      if (pe->next == NULL || pe->next->tag == pathstart)
        OFClose();
      break;
    case pathcurveto:
      pe = pe->next;  c2 = pe->coord;
      pe = pe->next;  c3 = pe->coord;
      FixCd(c0, &d0);  FixCd(c1, &d1);  FixCd(c2, &d2);  FixCd(c3, &d3);
      fr.limit = FLATTENLIMIT;
      FFltnCurve(d0, d1, d2, d3, &fr, true);
      if (pe->next == NULL || pe->next->tag == pathstart)
        OFClose();
      break;
    case pathclose:
      OFClose();
      if (pe->next != NULL && pe->next->tag != pathstart)
        OFMoveToP(&startCd);
      break;
    }
  c0 = pe->coord;
  }
d0.x = d0.y = 0;
OFMark(qRdcr, d0);
#if STAGE==DEVELOP
if (traceOffsetFill) Trace_OFDone();
#endif STAGE==DEVELOP
HANDLER
{ if (count++ < 3)
  {
    working = true;
    fr.feps = fr.feps*2;
  }
  else
    RERAISE;
}
END_HANDLER;
}

}

#if STAGE==DEVELOP
private procedure PSOffsetFill()
{
real ow, varcoeff;
PopPReal(&varcoeff);
PopPReal(&ow);
OffsetFill(&gs->path, pflttofix(&ow), pflttofix(&varcoeff));
NewPath();
} /* end of PSOffsetFill */
#endif STAGE==DEVELOP


#if STAGE==DEVELOP 
private procedure PSDPathForAll() { PathForAll(true); }
#endif STAGE==DEVELOP

public procedure IniPathPriv(reason)  InitReason reason;
{
switch (reason)
  {
  case init: {
#if (OS == os_mpw)
      globals = (Globals)os_sureCalloc(sizeof(GlobalsRec),1);
#endif (OS == os_mpw)
    FracSqrt2 = fracsqrt((UFrac)(FracOne + (FracOne-1)));
    break;
    }
  case romreg:
#if STAGE==DEVELOP
    traceOffsetFill = false;
    if (vSTAGE==DEVELOP) {
      RgstExplicit("offsetfill", PSOffsetFill);
      RgstExplicit("traceoffsetfill", psTraceOffsetFill);
      RgstExplicit("dpathforall", PSDPathForAll);
      }
#endif STAGE==DEVELOP
    break;
  }
}
