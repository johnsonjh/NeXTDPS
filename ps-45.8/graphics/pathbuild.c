/*
				 pathbuild.c

   Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE: All information contained herein  is  the property  of  Adobe Systems
Incorporated.   Many  of the  intellectual  and technical  concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for  their internal use.   Any reproduction
or dissemination of this software is strictly forbidden  unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Edit History:
Scott Byer: Tue May 23 12:57:24 1989
Ed Taft: Thu Jul 28 16:40:06 1988
Bill Paxton: Wed Sep 14 11:20:48 1988
Ivor Durham: Fri Sep 23 15:59:34 1988
Paul Rovner: Wednesday, November 30, 1988 6:40:10 PM
Joe Pasqua: Wed Mar  1 16:32:21 1989
Jim Sandman: Wed Oct  4 15:02:58 1989
Terry 30May90 AddToPath bbox check: if (tail == lp->head && !path->setbbox)
Jack  31Aug90 remove ConvertToListPath in HasCurrentPoint, per Bilodeau.

End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include DEVICE
#include ENVIRONMENT
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

#if (OS != os_mpw)
#define extended longreal
#endif

private Card16 pathLengthLimit;
public PPthElt pathFree;

public Pool lpStorage; /* pool for ListPath records */
public Pool qpStorage; /* pool for QuadPath records */
public Pool ipStorage; /* pool for IntersectPath records */
public Pool spStorage; /* pool for StrkPath records */
public Pool rpStorage; /* pool for ReducedPath records */

public boolean HasCurrentPoint(path) register PPath path;
{
if (path == NULL) return false;
if ((PathType)path->type != listPth)
	return true;
if (path->ptr.lp == NULL || path->ptr.lp->head == 0 || path->ptr.lp->tail == 0) 
	return false;
if (path == &gs->path)
	gs->cp = path->ptr.lp->tail->coord; /* keep cp in sync with path */
return true;
}

#define CheckCurPt(path) \
  if ((PathType)(path)->type == listPth && \
      ((path)->ptr.lp == NULL || (path)->ptr.lp->head == 0)) NoCurrentPoint()

public procedure CheckForCurrentPoint(path) PPath path;
{
if (!HasCurrentPoint(path)) NoCurrentPoint();
}

public procedure NoOp() {}

public procedure InitPath(path)  register PPath path;
{
  path->type = (BitField)listPth;
  path->ptr.lp = NULL;
  path->rp = NULL;
  path->bbox.bl.x = path->bbox.bl.y =
    path->bbox.tr.x = path->bbox.tr.y = fpZero;
  path->secret = path->eoReduced = false;
  path->isRect = path->checkedForRect = false;
  path->setbbox = false;
  path->length = 0;
  } /* end of InitPath */


public ListPath *AllocListPathRec(path) PPath path; {
  register ListPath *lp;
  Assert(path != NULL && (PathType)path->type == listPth);
  path->ptr.lp = lp = (ListPath *)os_newelement(lpStorage);
  lp->head = lp->tail = lp->start = 0;
  lp->refcnt = 1;
  return lp;
  }

private procedure FrLstPth(lp) ListPath *lp; {
  if (lp->head != NULL) {
    lp->tail->next = pathFree;
    pathFree = lp->head;
    }
  os_freeelement(lpStorage, (char *)lp);
  }

public procedure RemReducedRef(path) PPath path; {
  register ReducedPath *rp = path->rp;
  if (rp == NULL) return;
  rp->refcnt--;
  if (rp->refcnt == 0) {
    TermClipDevPrim((DevPrim *)rp->devprim);
      /* might have been used as clip */
    DisposeDevPrim((DevPrim *)rp->devprim);
    os_freeelement(rpStorage, (char *)rp);
    }
  path->rp = NULL;
  }

public procedure RemPathRef(path) register PPath path; {
  if (path->rp != NULL) RemReducedRef(path);
  switch ((PathType)path->type) {
    case listPth: {
      register ListPath *lp = path->ptr.lp;
      if (lp != NULL) {
        lp->refcnt--;
	if (lp->refcnt == 0) FrLstPth(lp);
        path->ptr.lp = NULL;
        }
      break;
      }
    case quadPth: {
      register QuadPath *qp = path->ptr.qp;
      if (qp != NULL) {
        qp->refcnt--;
	if (qp->refcnt == 0) {
          os_freeelement(qpStorage, (char *)qp);
          }
	path->ptr.qp = NULL;
        }
      break;
      }
    case intersectPth: {
      register IntersectPath *ip = path->ptr.ip;
      if (ip != NULL) {
        ip->refcnt--;
	if (ip->refcnt == 0) {
          RemPathRef(&ip->path);
	  RemPathRef(&ip->clip);
	  os_freeelement(ipStorage, (char *)ip);
          }
        path->ptr.ip = NULL;
        }
      break;
      }
    case strokePth: {
      register StrkPath *sp = path->ptr.sp;
      if (sp != NULL) {
        sp->refcnt--;
	if (sp->refcnt == 0) {
          RemPathRef(&sp->path);
	  os_freeelement(spStorage, (char *)sp);
          }
        path->ptr.sp = NULL;
        }
      break;
      }
    default: CantHappen();
    }
  }

public procedure AddPathRef(path) PPath path; {
  { register ReducedPath *rp = path->rp;
    if (rp != NULL) rp->refcnt++; }
  switch ((PathType)path->type) {
    case listPth: {
      register ListPath *lp = path->ptr.lp;
      if (lp != NULL) lp->refcnt++;
      break;
      }
    case quadPth: {
      register QuadPath *qp = path->ptr.qp;
      if (qp != NULL) qp->refcnt++;
      break;
      }
    case intersectPth: {
      register IntersectPath *ip = path->ptr.ip;
      if (ip != NULL) ip->refcnt++;
      break;
      }
    case strokePth: {
      register StrkPath *sp = path->ptr.sp;
      if (sp != NULL) sp->refcnt++;
      break;
      }
    default: CantHappen();
    } 
  }

public ListPath *MakeOwnListPath(path) PPath path; {
  ListPath *lp;
  RemReducedRef(path);
  Assert((PathType)path->type == listPth);
  lp = path->ptr.lp;
  Assert(lp != NULL && lp->refcnt > 1);
  lp->refcnt--;
  path->length = 0;
  path->ptr.lp = NULL;
  AppendCopyToPath(lp->head, path);
  return path->ptr.lp;
  }

public procedure CopyPath(to, from) PPath to, from; {
  Assert(to != from);
  *to = *from;
  AddPathRef(to);
  }

private procedure BuildPathFreeList() {
  register integer i = 100;
  /* add this many path elements to free list at a time */
  /* nothing magic about this number; ok to modify it */
  register PPthElt pe, prv;
  pe = (PPthElt)NEW(i, sizeof(PthElt));
  prv = NULL;
  while (i-- > 0)  {
    pe->next = prv; prv = pe; pe++; }
  pathFree = prv;
  }

public boolean IsPathEmpty(path) register PPath path; {
  register ListPath *lp;
  if (path->length == 0) return true;
  if (path->length > 1 || (PathType)path->type != listPth) return false;
  if ((lp = path->ptr.lp) == NULL) return true;
  if (lp->tail == NULL || lp->tail->tag == pathstart) return true;
  return false;
  }

private procedure AddToBBox(path, coord) PPath path; Cd coord;{
  register ListPath *lp = path->ptr.lp;
  if (path->setbbox) {
    register real r;
    r = coord.x;
    if (r < path->bbox.bl.x || r > path->bbox.tr.x)
      RangeCheck();
    r = coord.y;
    if (r < path->bbox.bl.y || r > path->bbox.tr.y)
      RangeCheck();
    }
  else {
    register real r;
    r = coord.x;
    if (path->bbox.bl.x > r) path->bbox.bl.x = r;
    else if (path->bbox.tr.x < r) path->bbox.tr.x = r;
    r = coord.y;
    if (path->bbox.bl.y > r) path->bbox.bl.y = r;
    else if (path->bbox.tr.y < r) path->bbox.tr.y = r;
    }
  }

private procedure AddToPath(path, coord, tag)
  register PPath path; Cd coord; PathTag tag; {
  register PPthElt pe, tail;
  register ListPath *lp;
  if (path->length >= pathLengthLimit) LimitCheck();
  if ((PathType)path->type != listPth) ConvertToListPath(path);
  if ((lp = path->ptr.lp) == NULL) lp = AllocListPathRec(path);
  if (lp->refcnt > 1) lp = MakeOwnListPath(path);
  tail = lp->tail;
  if (tag != pathstart) {
    if (tail != NULL && tail->tag == pathstart) {
      if (tail == lp->head && !path->setbbox)
	path->bbox.bl = path->bbox.tr = tail->coord;
      else
	AddToBBox(path, tail->coord);
    }
    AddToBBox(path, coord);
  }
  if (tag == pathstart && tail != NULL && tail->tag == pathstart)
    pe = tail;
  else
    {
    if (pathFree == NULL) BuildPathFreeList();
    pe = pathFree;  pathFree = pe->next;
    path->length++;
    }
  pe->coord = coord;
  if (path == &gs->path) gs->cp = coord;
  pe->tag = tag;
  pe->next = NULL;
  if (tail == NULL) {
    lp->head = pe;
    if (!path->setbbox)
      path->bbox.bl = path->bbox.tr = coord;
    }
  else {
    if (tail != pe) tail->next = pe;
    }
  lp->tail = pe;
  if (tag == pathstart) lp->start = pe;
  } /* end of AddToPath */

public procedure AppendCopyToPath(pe, newPath)
  register PPthElt pe; PPath newPath; {
  while (pe != 0)
    {
    AddToPath(newPath, pe->coord, pe->tag);
    pe = pe->next;
    }
  } /* end of AppendCopyToPath */
 
public procedure FrPth(path) 
  PPath path; 
  {
  RemPathRef(path);
  InitPath(path);
  }

public procedure MoveTo(c, p) 
  Cd c; 
  PPath p;  
  {
   AddToPath(p, c, pathstart);
  }

public procedure PSMoveTo() 
  {
   Cd cd;
   PopPCd(&cd);
   TfmPCd(cd, &gs->matrix, &cd);
   AddToPath(&gs->path, cd, pathstart);
  }				/* end of PSMoveTo */

public procedure PSRMoveTo() 
  {
   Cd cd;
   CheckCurPt(&gs->path);
   PopPCd(&cd);
   RTfmPCd(cd, &gs->matrix, gs->cp, &cd);
   AddToPath(&gs->path, cd, pathstart);
  }				/* end of PSRMoveTo */

public procedure LineTo(c, p) 
  Cd c; 
  PPath p; 
  {
   CheckCurPt(p);
   AddToPath(p, c, pathlineto);
  }				/* end of LineTo */

public procedure PSLineTo() 
  {
   Cd cd;
   PopPCd(&cd);
   TfmPCd(cd, &gs->matrix, &cd);
   LineTo(cd, &gs->path);
  }				/* end of PSLineTo */

public procedure PSRLineTo() 
  {
   Cd cd;
   PopPCd(&cd);
   RTfmPCd(cd, &gs->matrix, gs->cp, &cd);
   LineTo(cd, &gs->path);
  }				/* end of PSRLineTo */

public procedure CurveTo(c1, c2, c3, p) Cd c1, c2, c3; PPath p; {
  CheckCurPt(p);
  AddToPath(p, c1, pathcurveto);
  AddToPath(p, c2, pathcurveto);
  AddToPath(p, c3, pathcurveto);
  }  /* end of CurveTo */

public procedure PSCurveTo() {
  Cd c1, c2, c3;
  PopPCd(&c3); PopPCd(&c2); PopPCd(&c1);
  TfmPCd(c1, &gs->matrix, &c1);
  TfmPCd(c2, &gs->matrix, &c2);
  TfmPCd(c3, &gs->matrix, &c3);
  CurveTo(c1, c2, c3, &gs->path);
  } /* end of PSCurveTo */

public procedure PSRCurveTo() {
  Cd c1, c2, c3, cur;
  cur = gs->cp;
  PopPCd(&c3);  PopPCd(&c2);  PopPCd(&c1);
  RTfmPCd(c1, &gs->matrix, cur, &c1);
  RTfmPCd(c2, &gs->matrix, cur, &c2);
  RTfmPCd(c3, &gs->matrix, cur, &c3);
  CurveTo(c1, c2, c3, &gs->path);
  } /* end of PSRCurveTo */

private procedure ArcCenterShift(pc, pr, pdc) real *pc, *pr, *pdc; {
  real mn, mx, r, c, d1, d2, f;
  r = *pr; /* radius */
  if (RealLt0(r)) r = -r;
  c = *pc; /* center */
  mn = c - r; /* position in min pixel */
  mx = c + r; /* position in max pixel */
  f = os_floor(mn); 
  d1 = mn - f; /* margin on min side */
  f = os_floor(mx);
  if (f == mx) d2 = fpZero;
  else d2 = f + fpOne - mx; /* margin on max side */
  *pdc = (d2 - d1) * fpHalf; /* make the margins equal */
  }


private procedure SmallArc(pa, m, curveTo, ccwise, context, lastPt, c0)
    real * pa; PMtx m; char *context; procedure (*curveTo)();
    boolean ccwise, lastPt; Cd c0; {
  real ang, halfang, v, sina, cosa;
  Cd c1, c2, c3;
  ang = RAD(*pa);
  halfang = ang / fpTwo;
  v = (fp1p3333333 * (fpOne - os_cos(halfang))) / os_sin(halfang);
  sina = os_sin(ang);  cosa = os_cos(ang);
  c1.x = fpOne;  c1.y = (ccwise) ? v : -v;
  c2.x = cosa + v * sina;  c2.y = sina - v * cosa; if (!ccwise) c2.y = -c2.y;
  c3.x = cosa;  c3.y = (ccwise) ? sina : -sina;
  TfmPCd(c1, m, &c1); TfmPCd(c2, m, &c2);
  if (lastPt) c3 = c0;
  else TfmPCd(c3, m, &c3);
  (*curveTo)(c1, c2, c3, context);
  }

public procedure ArcInternal(center, radius, angStart, angEnd, ccwise,
  init, moveTo, lineTo, curveTo, mtx, context)
  Cd center;  extended radius, angStart, angEnd;  boolean ccwise;
  boolean (*init)();
  procedure (*moveTo)();
  procedure (*lineTo)();
  procedure (*curveTo)();
  PMtx mtx;
  char *context;
  {
  Cd c0, c1, c2, c3;
  real ang, firstAng, lastAng, rval, dx, dy;
  Mtx m, trans, rot, scale;
  integer n;
  real qStart, qEnd;
  boolean fullCircle, initPt;
  TlatMtx(&center.x, &center.y, &trans);
  rval = radius;
  ScalMtx(&rval, &rval, &scale);
  rval = angStart;
  RtatMtx(&rval, &rot);
  m = *mtx;
  MtxCnct(&trans, &m, &m);
  MtxCnct(&scale, &m, &m);
  MtxCnct(&rot, &m, &m);
  if (ccwise) {
    while (angStart > angEnd) angEnd += fp360;
    }
  else {
    while (angStart < angEnd) angEnd -= fp360;
    }
  qStart = os_floor(angStart / fp90);
  qEnd = os_floor(angEnd / fp90);
  firstAng = angStart - qStart * fp90;
  lastAng = angEnd - qEnd * fp90;
  if (qStart == qEnd) {
    firstAng = (ccwise) ? angEnd - angStart : angStart - angEnd;
    ang = lastAng = fpZero;
    }
  else if (ccwise) {
    if (RealNe0(firstAng)) {
      firstAng = fp90 - firstAng;
      angStart += firstAng;
      }
    if (RealNe0(lastAng))
      angEnd -= lastAng;
    ang = angEnd - angStart;
    }
  else {
    if (RealNe0(firstAng))
      angStart -= firstAng;
    if (RealNe0(lastAng)) {
      lastAng = fp90 - lastAng;
      angEnd += lastAng;
      }
    ang = angStart - angEnd;
    }
  n = (integer)(ang / fp90);
  initPt = (*init)(context);
  fullCircle = (ang + firstAng + lastAng == fp360);
  if (gs->circleAdjust && fullCircle && initPt) {
    extern boolean CheckForAnamorphicMatrix();
    extern real Dist();
    Cd c;
    real rx, ry;
    if (!CheckForAnamorphicMatrix(mtx)) {
      c.x = radius; c.y = fpZero; DTfmPCd(c, mtx, &c);
      rx = Dist(c); ry = rx;
      }
    else {
      c.x = fpOne; c.y = fpZero; IDTfmPCd(c, mtx, &c);
      rx = radius / Dist(c);
      c.y = fpOne; c.x = fpZero; IDTfmPCd(c, mtx, &c);
      ry = radius / Dist(c);
      }
    TfmPCd(center, mtx, &c);
    ArcCenterShift(&c.x, &rx, &dx);
    ArcCenterShift(&c.y, &ry, &dy);
    m.tx += dx; m.ty += dy;
    }
  c0.x = fpOne;  c0.y = fpZero;
  TfmPCd(c0, &m, &c0);
  if (initPt)
    (*moveTo)(c0, context);
  else
    (*lineTo)(c0, context);
  if (RealNe0(firstAng)) {
    SmallArc(&firstAng, &m, curveTo, ccwise, context, false, c0);
    if (!ccwise) firstAng = -firstAng;
    RtatMtx(&firstAng, &rot);
    MtxCnct(&rot, &m, &m);
    }
  if (n > 0) {
    ang = (ccwise) ? fp90 : -fp90;
    RtatMtx(&ang, &rot);
    }
  while (n-- > 0)
    {
    c1.x = fpOne;   c1.y = (ccwise) ? fpp552 : -fpp552;
    c2.x = fpp552;  c2.y = (ccwise) ? fpOne : -fpOne;
    c3.x = fpZero;  c3.y = (ccwise) ? fpOne : -fpOne;
    TfmPCd(c1, &m, &c1); TfmPCd(c2, &m, &c2);
    if (fullCircle && n == 0 && RealEq0(lastAng)) c3 = c0;
    else TfmPCd(c3, &m, &c3);
    (*curveTo)(c1, c2, c3, context);
    MtxCnct(&rot, &m, &m);
    }
  if (RealNe0(lastAng))
    SmallArc(&lastAng, &m, curveTo, ccwise, context, fullCircle, c0);
  } /* end of ArcInternal */

public boolean CallArcInit(path) PPath path; {
  return !HasCurrentPoint(path);
  }

public procedure Arc(center, radius, angStart, angEnd, ccwise, path)
  Cd center;
  extended radius, angStart, angEnd;
  boolean ccwise; 
  PPath path; {
  ArcInternal(
    center, radius, angStart, angEnd, ccwise,
    CallArcInit, MoveTo, LineTo, CurveTo, &gs->matrix, (char *)path);
  }

private procedure CallArc(ccwise)  boolean ccwise; {
  Cd center;
  real radius, angStart, angEnd;
  PopPReal(&angEnd);
  PopPReal(&angStart);
  PopPReal(&radius);
  PopPCd(&center);
  ArcInternal(
    center, radius, angStart, angEnd, ccwise, CallArcInit,
    MoveTo, LineTo, CurveTo, &gs->matrix, (char *)&gs->path);
  }  /* end of CallArc */

public procedure PSArc() {CallArc(true);}

public procedure PSArcN() {CallArc(false);}

public real Dist(v) Cd v; {
  real absx, absy;
  absx = os_fabs(v.x);  absy = os_fabs(v.y);
  if (absx <= absy)
    {if (absx <= (absy / fp1024)) return absy;}
  else
    {if (absy <= (absx / fp1024)) return absx;}
  return os_sqrt(v.x * v.x + v.y * v.y);
  }  /* end of Dist */

public real CDist(v)  Cd v; {
  real max, min, absx, absy;
  absx = os_fabs(v.x);  absy = os_fabs(v.y);
  if (absx > absy) {max = absx;  min = absy;} else {max = absy;  min = absx;}
  return max + min / fpTwo;
  }  /* end of CDist */

procedure ArcToInternal(c0, c1, c2, radius, pct1, pct2,
   init, moveTo, lineTo, curveTo, mtx, context)
  Cd c0, c1, c2;
  real radius;
  Cd *pct1, *pct2;
  boolean (*init)();
  procedure (*moveTo)();
  procedure (*lineTo)();
  procedure (*curveTo)();
  PMtx mtx;
  char *context; {
  Cd c, ct1, ct2, v0, v1, v2;
  real v1Len, v2Len, cos2theta, tantheta, a, v;
  VecSub(c0, c1, &v1);
  VecSub(c2, c1, &v2);
  v1Len = Dist(v1);
  v2Len = Dist(v2);
  if (RealEq0(v1Len) || RealEq0(v2Len)) UndefResult();
  v1Len = fpOne / v1Len;  v2Len = fpOne / v2Len;
  VecMul(v1, &v1Len, &v1);
  VecMul(v2, &v2Len, &v2);
  cos2theta = -(v1.x * v2.x + v1.y * v2.y);
  if (cos2theta == -fpOne
      || (tantheta = os_sqrt((fpOne - cos2theta) / (fpOne + cos2theta)),
          RealEq0(tantheta)))
    {ct1 = ct2 = c1;  TfmPCd(c1, mtx, &c1);  (*lineTo)(c1, context);}
  else
    {
    a = radius * tantheta;
    v = a - fp1p3333333 * radius * (os_sqrt(tantheta*tantheta +fpOne) -fpOne) / tantheta;
    ct1.x = c1.x + v1.x * a;  ct1.y = c1.y + v1.y * a;
    ct2.x = c1.x + v2.x * a;  ct2.y = c1.y + v2.y * a;
    TfmPCd(ct1, mtx, &c);
    TfmPCd(c0, mtx, &c0);
    VecSub(c0, c, &v0);
    if (os_fabs(v0.x) > fpHalf || os_fabs(v0.y) > fpHalf)
      (*lineTo)(c, context); /* filter out short initial lineto */
      /* protects against miter spikes caused by small floating point errors
          introducted by ITfmP in DoArcTo */
    c.x = c1.x + v1.x * v;  c.y = c1.y + v1.y * v;
    c2.x = c1.x + v2.x * v;  c2.y = c1.y + v2.y * v;
    TfmPCd(c, mtx, &c);  TfmPCd(c2, mtx, &c1);  TfmPCd(ct2, mtx, &c2);
    (*curveTo)(c, c1, c2, context);
    }
  *pct1 = ct1;
  *pct2 = ct2;
  } /* end of ArcToInternal */

private procedure DoArcTo(pushflg) boolean pushflg; {
  Cd c0, c1, c2, ct1, ct2;
  real radius;
  PopPReal(&radius);
  PopPCd(&c2);  PopPCd(&c1);
  CheckForCurrentPoint(&gs->path);
  ITfmP(gs->cp, &c0);
  ArcToInternal(
    c0, c1, c2, radius, &ct1, &ct2, CallArcInit, MoveTo,
    LineTo, CurveTo, &gs->matrix, (char *)&gs->path);
  if (pushflg) {
    PushPCd(&ct1);
    PushPCd(&ct2);
    }
  } /* end of DoArcTo */

public procedure PSArcTo() { DoArcTo(true); }
public procedure PSArcT()  { DoArcTo(false); }

public procedure ClosePath(p) PPath p; {
  register ListPath *pp;
  PPthElt pe;
  if ((PathType)p->type != listPth) ConvertToListPath(p);
  pp = p->ptr.lp;
  if (pp == NULL || pp->tail == NULL || pp->tail->tag == pathclose)
    return;
  pe = pp->start; /* Lisa compiler bug: double indirection actual parameter. */
  AddToPath(p, pe->coord, pathclose);
  pp = p->ptr.lp; /* AddToPath may have copied */
  pp->start = pp->tail;
  } /* end of ClosePath */

public procedure PSClosePath()  {ClosePath(&gs->path);}

public procedure PathBuildInit(reason)  InitReason reason;
{
switch (reason)
  {
  case init:
    lpStorage = os_newpool(sizeof(ListPath),10,0);
    qpStorage = os_newpool(sizeof(QuadPath),5,0);
    ipStorage = os_newpool(sizeof(IntersectPath),5,0);
    spStorage = os_newpool(sizeof(StrkPath),5,0);
    rpStorage = os_newpool(sizeof(ReducedPath),10,0);
    pathLengthLimit = 3000; /* max number of elements */
    BuildPathFreeList();
    break;
  case romreg:
    break;
  case ramreg:
    break;
  }
}
