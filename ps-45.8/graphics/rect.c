/*
				   rect.c

   Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE: All information  contained  herein is the  property of Adobe  Systems
Incorporated.  Many   of the  intellectual and technical   concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal  use.   Any reproduction
or dissemination of this software is strictly forbidden  unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Doug Brotz, May 11, 1983
Edit History:
Scott Byer: Thu May 18 10:15:22 1989
Doug Brotz: Thu Dec 11 17:23:20 1986
Chuck Geschke: Thu Oct 31 15:22:56 1985
Ed Taft: Thu Jul 28 16:42:41 1988
John Gaffney: Tue Feb 12 10:46:11 1985
Ken Lent: Wed Mar 12 15:53:25 1986
Bill Paxton: Wed Aug 31 11:34:20 1988
Don Andrews: Wed Sep 17 15:28:32 1986
Mike Schuster: Wed Jun 17 11:39:01 1987
Ivor Durham: Fri Sep 23 16:01:27 1988
Jim Sandman: Mon Jul 10 12:08:14 1989
Linda Gass: Tue Dec  8 12:14:11 1987
Paul Rovner: Thursday, October 8, 1987 9:40:40 PM
Joe Pasqua: Tue Jan 17 13:41:41 1989
Jack 09Nov87 new ReduceQuadPath, FillRect, use MakeBounds to fix BBox probs
Jack 20Nov87 big reorg of AddTrap code
Terry 06Sep90 new ReduceRect, FastRectFill, FloatRectFill
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
#include "userpath.h" 

#if (DPSXA == 0)

public ReduceRect(real *rect, PMtx m, DevPrim *dp)
{
    DevTrap *t;
    float minx, miny, maxx, maxy, w, h;
    
    DebugAssert(dp->type == trapType && dp->items < dp->maxItems);
    DebugAssert(RealEq0(m->b) && RealEq0(m->c));

    /* If width or height is 0, nudge rect so it overscans something */
    minx = maxx = rect[0] * m->a + m->tx;
    w = rect[2] * m->a;
    if (w > 0.0)	maxx += w;
    else if (w < 0.0)	minx += w;
    else		maxx += (float)0.00001;
    
    miny = maxy = rect[1] * m->d + m->ty;
    h = rect[3] * m->d;
    if (h > 0.0)	maxy += h;
    else if (h < 0.0)	miny += h;
    else		maxy += (float)0.00001;
    
    if (minx < -fp16k) minx = -fp16k;
    if (miny < -fp16k) miny = -fp16k;
    if (maxx >  fp16k) maxx =  fp16k;
    if (maxy >  fp16k) maxy =  fp16k;

    t = dp->value.trap + dp->items++;
    t->l.xg = t->l.xl	= (int) minx;
    if (dp->bounds.x.l > t->l.xl) dp->bounds.x.l = t->l.xl;
    t->g.xg = t->g.xl	= (int) (maxx + (float)0.999999);
    if (dp->bounds.x.g < t->g.xl) dp->bounds.x.g = t->g.xl;
    t->y.l 		= (int) miny;
    if (dp->bounds.y.l > t->y.l) dp->bounds.y.l = t->y.l;
    t->y.g 		= (int) (maxy + (float)0.999999);
    if (dp->bounds.y.g < t->y.g) dp->bounds.y.g = t->y.g;
}

private procedure FastRectFill(Object obj)
{
    real rect[4];
    int count, i;
    NumStrm ns;
    procedure (*getreal)();
    DevPrim *tp = ms->trapsDP;
    extern BBoxCompareResult BoundsCompare();
    DevPrim *clip = GetDevClipPrim();

    SetupNumStrm(&obj, &ns);
    getreal = ns.GetReal;
    count = ns.len;
    if (count&3) TypeCheck();
    count >>= 2;
    do {
	StdInitMark(ms, true);
	if ((i = count) > tp->maxItems) i = tp->maxItems;
	count -= i;
	while (i-- > 0) {
	    (*getreal)(&ns, &rect[0]);
	    (*getreal)(&ns, &rect[1]);
	    (*getreal)(&ns, &rect[2]);
	    (*getreal)(&ns, &rect[3]);
	    ReduceRect(rect, &gs->matrix, tp);
	}
	ms->bbCompMark = BoundsCompare(&tp->bounds, &clip->bounds);
	StdTermMark(ms);
    } while (count > 0);
}

private procedure FloatRectFill(Object obj, int flip)
{
    DevTrap *t;
    float *fp, x, y, w, h, tx, ty;
    int minx, miny, maxx, maxy;
    int count, i;
    DevPrim *tp = ms->trapsDP;
    extern BBoxCompareResult BoundsCompare();
    DevPrim *clip = GetDevClipPrim();
  
    if (obj.length < 4) TypeCheck();
    count = *(int *)(obj.val.strval);
    if (!(obj.access & rAccess)) InvlAccess();
    count &= 0xffff;
    if (count&3) TypeCheck();
    count >>= 2;
    fp = (float *)obj.val.strval + 1;
    tx = gs->matrix.tx;
    ty = gs->matrix.ty;
    
    do {
	StdInitMark(ms, true);
	if ((i = count) > tp->maxItems) i = tp->maxItems;
	count -= i;
	tp->items = 0;
	t = tp->value.trap;

	while (i-- > 0) {
	    x = *fp++ + tx;
	    y = *fp++;
    
	    w = *fp++;
	    if (w > 0)		{ minx = x;	maxx = x+w+(float)0.9999999; }
	    else if (w < 0)	{ minx = x+w;	maxx = x+  (float)0.9999999; }
	    else		{ minx = x;	maxx = minx+1; }
    
	    h = *fp++;
	    if (flip) { y = -y; h = -h; }
	    y += ty;
	    if (h > 0)		{ miny = y;	maxy = y+h+(float)0.9999999; }
	    else if (h < 0)	{ miny = y+h;	maxy = y+  (float)0.9999999; }
	    else		{ miny = y;	maxy = miny+1; }
    
            if (minx < -fp16k) { if (maxx<-fp16k) continue; else minx=-fp16k; }
	    if (miny < -fp16k) { if (maxy<-fp16k) continue; else miny=-fp16k; }
	    if (maxx >  fp16k) { if (minx> fp16k) continue; else maxx= fp16k; }
	    if (maxy >  fp16k) { if (miny> fp16k) continue; else maxy= fp16k; }

	    if (tp->bounds.x.l > (t->l.xg=t->l.xl=minx)) tp->bounds.x.l = minx;
	    if (tp->bounds.x.g < (t->g.xg=t->g.xl=maxx)) tp->bounds.x.g = maxx;
	    if (tp->bounds.y.l > (t->y.l = miny)) tp->bounds.y.l = miny;
	    if (tp->bounds.y.g < (t->y.g = maxy)) tp->bounds.y.g = maxy;
	    t++;
	    tp->items++;
	}
	ms->bbCompMark = BoundsCompare(&tp->bounds, &clip->bounds);
	StdTermMark(ms);
    } while (count > 0);
}

private procedure MultiRectFill(Object obj, DevPrim *dp)
{
  Cd userPt;
  real userw, userh;
  QuadPath qp;
  BBoxRec bbox;
  integer count;
  NumStrm ns;
  procedure (*getreal)();
  DevPrim *tp = ms->trapsDP;
  extern BBoxCompareResult BoundsCompare();
  DevPrim *clip = GetDevClipPrim();

  SetupNumStrm(&obj, &ns);
  getreal = ns.GetReal;
  if (ns.len & 3 != 0) TypeCheck();
  count = ns.len >> 2;
  StdInitMark(ms, true);
  while (count > 0) {
    (*getreal)(&ns, &userPt.x);
    (*getreal)(&ns, &userPt.y);
    (*getreal)(&ns, &userw);
    (*getreal)(&ns, &userh);
    dp->items = 0;
    (void)ReduceQuadPath(
      userPt, userw, userh, &gs->matrix, dp, &qp, &bbox, (Path *)NULL);
    if (tp->items + dp->items > tp->maxItems) {
      ms->bbCompMark = BoundsCompare(&tp->bounds, &clip->bounds);
      (*ms->procs->trapsFilled)(ms);
      }
    os_bcopy((char *)dp->value.trap, (char *)(tp->value.trap + tp->items),
      (long int)(dp->items * sizeof(DevTrap)));
    tp->items += dp->items;
    MergeDevBounds(&tp->bounds, &tp->bounds, &dp->bounds);
    count--;
    }
  ms->bbCompMark = BoundsCompare(&tp->bounds, &clip->bounds);
  StdTermMark(ms);
  } /* MultiRectFill */


public procedure PSRectFill()
{
  DevTrap traps[7];
  DevPrim dp, *graphic, *clip, *g, *c;
  Cd userPt;
  real userw, userh;
  QuadPath qp;
  BBoxRec bbox;
  Object objs[4];
  register PObject args;
  boolean rect, clipisrect;
  extern BBoxCompareResult BoundsCompare();
  DevMarkInfo info;
  int type;
  
  dp.type = trapType;
  dp.next = NULL;
  dp.items = 0;
  dp.maxItems = 7;
  dp.value.trap = traps;
  PopP(&objs[3]);
  type = objs[3].type;
  if (type == pkdaryObj || type == strObj || type == arrayObj) {
    if (RealEq0(gs->matrix.b) && RealEq0(gs->matrix.c) && type == strObj && 
        objs[3].val.strval[0] == 0x95 && objs[3].val.strval[1] == 0x30 &&
	gs->matrix.a == 1.0 && (gs->matrix.d == -1.0 || gs->matrix.d == 1.0))
     	    FloatRectFill(objs[3], (gs->matrix.d == -1.0));
    else if (RealEq0(gs->matrix.b) && RealEq0(gs->matrix.c))
     	    FastRectFill(objs[3]);
    else
    	MultiRectFill(objs[3],&dp);
    return;
  }
  
  PopP(&objs[2]);
  PopP(&objs[1]);
  PopP(&objs[0]);
  args = &objs[0];
  switch (args->type) {
    case realObj: userPt.x = args->val.rval; break;
    case intObj:  userPt.x = (real)(args->val.ival); break;
    default: TypeCheck();
    }
  args++;
  switch (args->type) {
    case realObj: userPt.y = args->val.rval; break;
    case intObj:  userPt.y = (real)(args->val.ival); break;
    default: TypeCheck();
    }
  args++;
  switch (args->type) {
    case realObj: userw = args->val.rval; break;
    case intObj:  userw = (real)(args->val.ival); break;
    default: TypeCheck();
    }
  args++;
  switch (args->type) {
    case realObj: userh = args->val.rval; break;
    case intObj:  userh = (real)(args->val.ival); break;
    default: TypeCheck();
    }
  info.color = gs->color->color;
  info.halftone = (gs->screen == NULL) ? NULL : gs->screen->halftone;
  info.screenphase = gs->screenphase;
  info.priv = (DevPrivate *)&gs->extension;
  info.offset.x = 0;
  info.offset.y = 0;
  clip = GetDevClipPrim();
  clipisrect = DevClipIsRect();
  g = &dp; c = clip;
  rect = ReduceQuadPath(
    userPt, userw, userh, &gs->matrix, &dp, &qp, &bbox, (Path *)NULL);
  if (clipisrect &&
       (BoundsCompare(&dp.bounds, &clip->bounds) == inside))
    c = NULL;
  else if (rect &&
    /* e.g. clippath pathbbox 2 index sub exch 3 index sub exch rectfill */
    (BoundsCompare(&clip->bounds, &dp.bounds) == inside)) {
    g = clip;
    c = NULL;
    }
  if (g->type != noneType && (c == NULL || c->type != noneType))
    (*gs->device->procs->Mark)(gs->device, g, c, &info);
} /* PSRectFill */
#endif /* DPSXA */

public procedure BuildMultiRectPath()
{
  Cd userPt, cd;
  real userw, userh, initx;
  Object objs[4];
  register PObject args;
  integer count;
  NumStrm ns;
  boolean simpleArgs;
  procedure (*getreal)();
  PPath path = &gs->path;
  PMtx mtx = &gs->matrix;
  
  PopP(&objs[3]);
  if (objs[3].type == arrayObj ||
      objs[3].type == strObj ||
      objs[3].type == pkdaryObj) {
    SetupNumStrm(&objs[3], &ns);
    getreal = ns.GetReal;
    if (ns.len & 3 != 0) TypeCheck();
    count = ns.len >> 2;
    simpleArgs = false;
    }
  else {
    count = 1;
    PopP(&objs[2]);
    PopP(&objs[1]);
    PopP(&objs[0]);
    args = &objs[0];
    simpleArgs = true;
    }
  NewPath();
  while (--count >= 0) {
    if (simpleArgs) {
      switch (args->type) {
        case realObj: userPt.x = args->val.rval; break;
        case intObj:  userPt.x = (real)(args->val.ival); break;
        default: TypeCheck();
        }
      args++;
      switch (args->type) {
        case realObj: userPt.y = args->val.rval; break;
        case intObj:  userPt.y = (real)(args->val.ival); break;
        default: TypeCheck();
        }
      args++;
      switch (args->type) {
        case realObj: userw = args->val.rval; break;
        case intObj:  userw = (real)(args->val.ival); break;
        default: TypeCheck();
        }
      args++;
      switch (args->type) {
        case realObj: userh = args->val.rval; break;
        case intObj:  userh = (real)(args->val.ival); break;
        default: TypeCheck();
        }
      }
    else {
      (*getreal)(&ns, &userPt.x);
      (*getreal)(&ns, &userPt.y);
      (*getreal)(&ns, &userw);
      (*getreal)(&ns, &userh);
      }
    if (RealLt0(userw)) { userw = -userw; userPt.x -= userw; }
    if (RealLt0(userh)) { userh = -userh; userPt.y -= userh; }
      /* make all rects counter clockwise in userspace */
      /* so can ignore winding number issues */
    initx = userPt.x;
    TfmPCd(userPt, mtx, &cd);
    MoveTo(cd, path);
    userPt.x += userw;
    TfmPCd(userPt, mtx, &cd);
    LineTo(cd, path);
    userPt.y += userh;
    TfmPCd(userPt, mtx, &cd);
    LineTo(cd, path);
    userPt.x = initx; /* cannot just sub userw because of float roundoff */
    TfmPCd(userPt, mtx, &cd);
    LineTo(cd, path);
    ClosePath(path);
    }
} /* BuildMultiRectPath */

#if DPSXA
public procedure PSRectFill() {
  GSave();
  DURING
    BuildMultiRectPath();
    Fill(&gs->path);
    NewPath();
  HANDLER {GRstr(); RERAISE;}
  END_HANDLER;
  GRstr();
  }
#endif /* DPSXA */

public procedure PSRectStroke() {
  boolean ismtx = CheckForMtx();
  Mtx sMtx;
  if (ismtx) PopMtx(&sMtx);
  GSave();
  DURING
    BuildMultiRectPath();
    if (ismtx) 
      Cnct(&sMtx);
    Stroke(&gs->path);
    NewPath();
  HANDLER {GRstr(); RERAISE;}
  END_HANDLER;
  GRstr();
  }

public procedure PSRectClip() {
  Object ob;
  Path path;
  TopP(&ob);
  if (ob.type == realObj || ob.type == intObj) {
    MakeRectPath(&path);
    DURING
    ReducePathClipInt(&path, false);
    HANDLER {FrPth(&path); RERAISE;}
    END_HANDLER;
    FrPth(&path);
    }
  else { /* multiple rectangles */
    BuildMultiRectPath();
    Clip(false);
    }
  NewPath();
  }

