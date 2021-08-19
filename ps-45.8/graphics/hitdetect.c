/*
  hitdetect.c

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Bill Paxton: Sat Apr  9 08:34:21 1988
Edit History:
Bill Paxton: Sat Apr 30 13:24:52 1988
Ed Taft: Thu Jul 28 16:26:30 1988
Jim Sandman: Tue Apr 11 14:23:44 1989
Ivor Durham: Fri Sep 23 15:55:23 1988
Paul Rovner: Wed Nov 15 13:30:53 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include EXCEPT
#include FP
#include GRAPHICS
#include DEVICE
#include LANGUAGE
#include VM

#include "graphicspriv.h"
#include "userpath.h"

extern BBoxCompareResult QCompareBounds();
extern DevPrim *UCGetDevPrim();
extern DevPrim *DoRdcPth();
extern DevPrim * StrkPthProc();
extern boolean UsrPthQRdcOk(), QEnumOk();
extern procedure FillUserPathEnumerate(), QFillUserPathEnumerate();
extern boolean PointInTraps();
#if DPSXA
extern DevPrim *XARdc(), *XADoRdcPth();
#endif /* DPSXA */


private procedure HitTrap(t, hit) DevTrap *t; boolean *hit; {
  *hit = true;
  }

private procedure HitRun(r, hit) DevRun *r; boolean *hit; {
  *hit = true;
  }

  
private boolean IntersectDP (dp1, dp2) DevPrim *dp1, *dp2; {
  boolean hit = false;
  DevBounds b1, b2;
  DevPrim *dp;
#if (DPSXA == 0)
  FullBounds(dp1, &b1);
  FullBounds(dp2, &b2);
  if (BoundsCompare(&b1, &b2) == outside)
    return false;
#endif /* DPSXA */
  while (dp1 != NULL && !hit) {
    switch (dp1->type) {
      case noneType:
        break;
      case trapType: {
        integer items1 = dp1->items;
        DevTrap *traps1 = dp1->value.trap;
	for (dp = dp2; dp && !hit; dp = dp->next) {
	  switch (dp->type) {
	    case noneType:
	      break;
	    case trapType: {
	      integer i;
	      DevTrap *t;
	      DevInterval inner, outer;
#if DPSXA
		if((dp->xaOffset.x == dp1->xaOffset.x) && (dp->xaOffset.y == dp1->xaOffset.y)){
	    	for (;items1--; traps1++) {
				for (i = dp->items, t = dp->value.trap; i--; t++) 	{
		  			TrapTrapInt(
		    			t, traps1, (DevInterval *)NULL, HitTrap, (char *)&hit);
		  			if (hit) return true;
		  		}
			}
		}
#else /* DPSXA */
	      for (;items1--; traps1++) {
		switch (BoxTrapCompare(
		  &dp->bounds, traps1, &inner, &outer, (DevTrap *)NULL)) {
		  case outside: continue;
		  case inside:
		    return true;
		  }
		for (i = dp->items, t = dp->value.trap; i--; t++) {
		  TrapTrapInt(
		    t, traps1, (DevInterval *)NULL, HitTrap, (char *)&hit);
		  if (hit) return true;
		  }
		}
#endif /* DPSXA */
	      break;
	      }
	    case runType: {
	      integer i;
	      DevRun *r;
	      DevInterval inner, outer;
	      for (;items1--; traps1++) {
		for (i = dp->items, r = dp->value.run; i--; r++) {
		  switch (BoxTrapCompare(
		    &r->bounds, traps1, &inner, &outer, (DevTrap *)NULL)) {
		    case outside: continue;
		    case inside:
		      return true;
		    }
		  QIntersectTrp(r, traps1, HitRun, (char *)&hit);
		  if (hit) return true;
		  }
		}
	      break;
	      }
	    default:
	      CantHappen();
	      break;
	    };
	  }
        break;
        }
      case runType: {
        integer items1 = dp1->items;
        DevRun *runs1 = dp1->value.run;
	for (dp = dp2; dp && !hit; dp = dp->next) {
	  switch (dp->type) {
	    case noneType:
	      break;
	    case trapType: {
	      integer i;
	      DevTrap *t;
	      DevInterval inner, outer;
	      for (;items1--; runs1++) {
		for (i = dp->items, t = dp->value.trap; i--; t++) {
		  switch (BoxTrapCompare(
		    &runs1->bounds, t, &inner, &outer, (DevTrap *)NULL)) {
		    case outside: continue;
		    case inside:
		      return true;
		    }
		  QIntersectTrp(runs1, t, HitRun, (char *)&hit);
		  if (hit) return true;
		  }
		}
	      break;
	      }
	    case runType: {
	      integer i;
	      DevRun *r;
	      for (;items1--; runs1++) {
		for (i = dp->items, r = dp->value.run; i--; r++) {
		  QIntersect(r, runs1, HitRun, (char *)&hit);
		  if (hit) return true;
		  }
		}
	      break;
	      }
	    default:
	      CantHappen();
	      break;
	    };
	  }
        break;
        }
      default:
        CantHappen();
        break;
      }
    dp1 = dp1->next;
    };
  return false;
  }

private boolean PointInRun(r1, dc)
  register DevRun *r1; DevCd dc; {
  register PInt16 d1;
  register int lft, rht, px, py, n1;
  px = dc.x;
  if (px < r1->bounds.x.l) return false; /* left */
  if (px >= r1->bounds.x.g) return false; /* right */
  py = dc.y;
  if (py < r1->bounds.y.l) return false; /* below */
  if (py >= r1->bounds.y.g) return false; /* above */
  d1 = RunArrayRow(r1, py);
  n1 = *(d1++); /* number of pairs for this row */
  while (n1--) {
    lft = *(d1++); rht = *(d1++); /* left and right for pair */
    if (px >= rht) continue;
    if (px >= lft) return true;
    break;
    }
  return false;
  }

public boolean PointInDevPrim(dp, cd) DevPrim *dp; Cd cd; {
  DevBounds bounds;
  integer items;
  DevCd dc;
#if DPSXA
  DevCd tmpdc;
#endif /* DPSXA */
  TfmPCd(cd, &gs->matrix, &cd);
  dc.x = os_floor(cd.x); dc.y = os_floor(cd.y);
#if (DPSXA == 0)
  bounds.x.l = dc.x; bounds.x.g = bounds.x.l + 1;
  bounds.y.l = dc.y; bounds.y.g = bounds.y.l + 1;
#endif /* DPSXA */
  for (; dp; dp = dp->next) {
#if (DPSXA == 0)
    if (BoundsCompare(&bounds, &dp->bounds) == outside) continue;
#endif /* DPSXA */
    items = dp->items;
    switch (dp->type) {
      case noneType: break;
      case trapType:
#if DPSXA
			tmpdc.x = dc.x - dp->xaOffset.x;
			tmpdc.y = dc.y - dp->xaOffset.y;
			if (PointInTraps(dp->value.trap, items, tmpdc)) return true;
#else /* DPSXA */
			if (PointInTraps(dp->value.trap, items, dc)) return true;
#endif /* DPSXA */
        break;
      case runType: {
	DevRun *run = dp->value.run;
	for (; items--; run++)
          if (PointInRun(run, dc)) return true;
        break; }
      default: CantHappen;
      }
    }
  return false;
  }

private DevPrim *GetAperatureDP(ctx, dispose)
  UserPathContext *ctx; boolean *dispose; {
  DevPrim *dp = NULL;
  GetUsrPthAry(ctx);
  *dispose = true;
  if (ctx->ucache) {
    ctx->fill = true; ctx->evenOdd = false;
    dp = UCGetDevPrim(ctx, NULL);
    }
  if (dp == NULL) {
    UsrPthBBox(ctx);
#if DPSXA
	dp = XARdc(XADoRdcPth,ctx);
#else /* DPSXA */
    dp = DoRdcPth(false, ctx, &ctx->bbox,
      UsrPthQRdcOk, FillUserPathEnumerate,
      QEnumOk, QFillUserPathEnumerate);
#endif /* DPSXA */
    }
  else *dispose = ctx->dispose;
  return dp;
  }

public boolean InFill(evenOdd) boolean evenOdd; {
  DevPrim *pathDP;
  Object ob;
  boolean hit;
  TopP(&ob);
#if DPSXA
  XAReducePath(ReducePath, &gs->path, evenOdd);
#else /* DPSXA */
  ReducePath(&gs->path, evenOdd);
#endif /* DPSXA */
  if (gs->path.rp == NULL || gs->path.rp->devprim == NULL)
    return false;
  pathDP = (DevPrim *)(gs->path.rp->devprim);
  if (ob.type == arrayObj || ob.type == pkdaryObj) {
    UserPathContext context;
    boolean dispose;
    DevPrim *aperatureDP;
    aperatureDP = GetAperatureDP(&context, &dispose);
    hit = IntersectDP(pathDP, aperatureDP);
    if (dispose) DisposeDevPrim(aperatureDP);
    }
  else {
    Cd cd;
    PopPCd(&cd);
    hit = PointInDevPrim(pathDP, cd);
    }
  return hit;
  }

public procedure PSInFill() { PushBoolean(InFill(false)); }
public procedure PSInEOFill() { PushBoolean(InFill(true)); } 

#if DPSXA
private XAStrkPthProc (path, bool)
PPath path;
boolean bool;
{
	PutRdc(path,StrkPthProc(path),bool);
}
#endif /* DPSXA */

public boolean InStroke() {
  DevPrim *pathDP;
  Object ob;
  boolean hit;
  TopP(&ob);
  if (ob.type == arrayObj || ob.type == pkdaryObj) {
    UserPathContext context;
    boolean dispose;
    DevPrim *aperatureDP;
    aperatureDP = GetAperatureDP(&context, &dispose);
#if DPSXA
    XAReducePath(XAStrkPthProc, &gs->path, false);
    pathDP = (DevPrim *)gs->path.rp->devprim;   
#else /* DPSXA */
    pathDP = StrkPthProc(&gs->path);
#endif /* DPSXA */
    hit = IntersectDP(pathDP, aperatureDP);
    if (dispose) DisposeDevPrim(aperatureDP);
    }
  else {
    Cd cd;
    PopPCd(&cd);
#if DPSXA
    XAReducePath(XAStrkPthProc, &gs->path, false);
    pathDP = (DevPrim *)gs->path.rp->devprim;   
#else /* DPSXA */
    pathDP = StrkPthProc(&gs->path);
#endif /* DPSXA */
    hit = PointInDevPrim(pathDP, cd);
    }
  DisposeDevPrim(pathDP);
#if DPSXA
  gs->path.rp->devprim = NULL;
#endif /* DPSXA */
  return hit;
  }

public procedure PSInStroke() { PushBoolean(InStroke()); }

private boolean InUFill(evenOdd) boolean evenOdd; {
  Cd cd;
  Object ob;
  DevPrim *dp = NULL;
  DevPrim *dpAp;
  UserPathContext context, contextAp;
  boolean hit, dispose, userpath, disposeAp;
  GetUsrPthAry(&context);
  TopP(&ob);
  if (ob.type == arrayObj || ob.type == pkdaryObj) {
    dpAp = GetAperatureDP(&contextAp, &disposeAp);
    userpath = true;
    }
  else {
    PopPCd(&cd);
    userpath = false;
    }
  dispose = true;
  if (context.ucache) {
    context.fill = true; context.evenOdd = evenOdd;
    dp = UCGetDevPrim(&context, NULL);
    }
  if (dp == NULL) {
    UsrPthBBox(&context);
#if DPSXA
	dp = XARdc(XADoRdcPth, &context);
#else /* DPSXA */
    dp = DoRdcPth(evenOdd, &context, &context.bbox,
      UsrPthQRdcOk, FillUserPathEnumerate,
      QEnumOk, QFillUserPathEnumerate);
#endif /* DPSXA */
    }
  else dispose = context.dispose;
  if (userpath) {
    hit = IntersectDP(dp, dpAp);
    if (disposeAp) DisposeDevPrim(dpAp);
    }
  else
    hit = PointInDevPrim(dp, cd);
  if (dispose) DisposeDevPrim(dp);
  return hit;
  }

public procedure PSInUFill() { PushBoolean(InUFill(false)); }
public procedure PSInUEOFill() { PushBoolean(InUFill(true)); }  

public procedure PSInUStroke() { /* temporary implementation */
  Cd cd;
  DevPrim *dp = NULL;
  Object ob;
  UserPathContext context;
  Mtx sMtx;
  AryObj a;
  boolean hit;
  boolean ismtx = CheckForMtx();
  if (ismtx) { TopP(&a); PopMtx(&sMtx); }
  GetUsrPthAry(&context);
  if (!context.ucache) goto hardWay;
  TopP(&ob);
  if (ob.type == arrayObj || ob.type == pkdaryObj) goto hardWay;
  PopPCd(&cd);
  context.fill = false;
  context.circletraps = true;
  dp = UCGetDevPrim(&context, ismtx ? &sMtx : NULL);
  if (dp == NULL) { PushPCd(&cd); goto hardWay; }
  hit = PointInDevPrim(dp, cd);
  if (context.dispose) DisposeDevPrim(dp);
  PushBoolean(hit);
  return;
  hardWay:
  PushP(&context.aryObj);
  if (ismtx) PushP(&a);  GSave();
  PSUStrokePath();
  PSInFill();
  GRstr();
  }


