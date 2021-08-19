/*
  strokeops.c

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
Larry Baer: Tue Nov 14 14:52:04 1989
Scott Byer: Thu Jun  1 15:13:38 1989
Ed Taft: Thu Jul 28 13:25:29 1988
Bill Paxton: Wed Aug 31 14:14:34 1988
Don Andrews: Wed Sep 17 14:55:53 1986
Ivor Durham: Sun May 14 09:08:47 1989
Joe Pasqua: Wed Dec 14 15:57:27 1988
Jim Sandman: Wed Dec 13 12:59:43 1989
Bill Bilodeau: Fri Oct 13 11:20:35 PDT 1989
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
#include RECYCLER
#include VM

#include "graphicspriv.h"
#include "path.h"
#include "stroke.h"

public boolean CheckForAnamorphicMatrix(m) register PMtx m; {
  /* returns true if matrix is anamorphic */
  register real q;
  q = m->a - m->d;
  if (os_abs(q) < fpp001) {
    q = m->b + m->c;
    return (os_abs(q) > fpp001);
    }
  q = m->a + m->d;
  if (os_abs(q) < fpp001) {
    q = m->b - m->c;
    return (os_abs(q) > fpp001);
    }
  return true;
  }

public procedure StrkInternal(p, isSP) PPath p; boolean isSP; {
  register PPthElt pe;
  boolean rectangle;
  Cd c1, c2;
  ListPath *pp;
  if ((PathType)(p->type) != listPth) ConvertToListPath(p);
  pp = p->ptr.lp;
  if (pp == NULL || pp->head == NULL) return;
  pe = pp->head->next;
  if (pe == NULL || pe->tag != pathlineto || pe->next != NULL)
     rectangle = false;
  else { rectangle = true; c1 = pp->head->coord; c2 = pe->coord; }
  if (DoStroke(isSP, p, &p->bbox, rectangle, c1, c2, DoPath, NULL, NULL,
      false, isSP? NULL : GetDevClipBBox(),isSP? false : DevClipIsRect(), NULL)) {
     if (!isSP) {
       (*ms->procs->termMark)(ms);
       FinStroke();
     }
  }
} /* end of StrkInternal */


public procedure PSStroke()
{Stroke(&gs->path);  NewPath();}


public procedure DoStrkPth(getDP, ctx) DevPrim *(*getDP)(); char *ctx;
{
Path sp;
DevPrim *dp;
DevBounds bounds;
register StrkPath *spp;
register PGState g;
spp = (StrkPath *)os_newelement(spStorage);
spp->refcnt = 1;
InitPath(&spp->path);
g = gs;
spp->matrix = g->matrix;
spp->lineWidth = g->lineWidth;
spp->miterlimit = g->miterlimit;
spp->devhlw = g->devhlw;
spp->dashArray = g->dashArray;
spp->dashOffset = g->dashOffset;
spp->flatEps = g->flatEps;
spp->lineCap = g->lineCap;
spp->lineJoin = g->lineJoin;
spp->strokeAdjust = g->strokeAdjust;
spp->strokeWidth = g->strokeWidth;
InitPath(&sp);
sp.type = (BitField)strokePth;
sp.ptr.sp = spp;
sp.rp = NULL;
#if (DPSXA == 0)
DURING
sp.rp = (ReducedPath *)os_newelement(rpStorage);
dp = (*getDP)(ctx);
HANDLER {
  os_freeelement(spStorage, spp);
  if (sp.rp) os_freeelement(rpStorage, sp.rp);
  RERAISE;
  }
END_HANDLER;
sp.rp->refcnt = 1;
sp.rp->devprim = (char *)dp;
sp.eoReduced = false;
FullBounds(dp, &bounds);
GetBBoxFromDevBounds(&sp.bbox, &bounds);
#endif /* DPSXA */
CopyPath(&spp->path, &gs->path);
FrPth(&gs->path);
gs->path = sp;
}

public DevPrim* StrkPthProc(path) PPath path; {
  DevCd nullCD;
  return (DevPrim *)DoRdcStroke(path, &path->bbox,
                  false, nullCD, nullCD, DoPath, NULL, NULL, true);
  }

public procedure PSStrkPth()
{ DoStrkPth(StrkPthProc, &gs->path); } /* end of PSStrkPth */


public procedure SetLineWidth(pr)  Preal pr;
{
gs->lineWidth = (RealLt0(*pr)) ? -(*pr) : *pr;
gs->devhlw = fpZero;
}  /* end of SetLineWidth */


public procedure PSSetLineWidth()
{
real r;
PopPReal(&r);
SetLineWidth(&r);
} /* end of PSSetWidth */


public procedure PSCrLineWidth()  {PushPReal(&gs->lineWidth);}


public procedure PSSetLineCap()
{
cardinal cap;
cap = PopCardinal();
if (cap > tenonCap) RangeCheck();
gs->lineCap = cap;
}  /* end of PSSetCap */

public procedure PSCrLineCap() {PushCardinal(gs->lineCap);}

public procedure PSSetLineJoin()
{
cardinal join;
join = PopCardinal();
if (join > bevelJoin) RangeCheck();
gs->lineJoin = join;
}  /* end of PSSetJoin */

public procedure PSCrLineJoin() {PushCardinal(gs->lineJoin);}

public procedure PSSetMiterLimit()
{
real lim;
PopPReal(&lim);
if (lim < fpOne)  RangeCheck();
gs->miterlimit = lim;
}  /* end of PSMiterLimit */

public procedure PSCrMiterLimit() {PushPReal(&gs->miterlimit);}

public procedure PSSetDash()
{
AryObj ao, aorig;
Object ob;
real dashOffset, seglen;
boolean allzero;
PopPReal(&dashOffset);
PopPArray(&ao);
ConditionalInvalidateRecycler (&ao);
aorig = ao;	/* Copy after invalidation in case ao moves */
if (aorig.length > DASHLIMIT) LimitCheck();
if (aorig.length != 0)
  {
  allzero = true;
  while (ao.length != 0)
    {
    VMCarCdr(&ao, &ob);  PRealValue(ob, &seglen);
    if (RealLt0(seglen)) RangeCheck();
    if (RealGt0(seglen)) allzero = false;
    }
  if (allzero) RangeCheck();
  }
gs->dashArray = aorig;
gs->dashOffset = dashOffset;
} /* end of PSSetDash */

public procedure PSCrDash()
{PushP(&gs->dashArray);  PushPReal(&gs->dashOffset);}
