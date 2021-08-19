/*
  maketrap.c

Copyright (c) 1987, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version:
Edit History:
Scott Byer: Thu Jun  1 13:16:00 1989
Ivor Durham: Thu May 19 17:56:52 1988
Bill Paxton: Tue Oct 25 13:37:44 1988
Jim Sandman: Wed Nov  9 15:20:18 1988
Joe Pasqua: Tue Jan 10 14:26:37 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include DEVICE
#include ERROR
#include EXCEPT
#include FP
#include GRAPHICS

#include "graphicspriv.h"

#ifndef DPSXA
#define DPSXA 0
#endif
#if DPSXA
extern DevCd xaOffset;
extern boolean inBuildChar;
#endif /* DPSXA */

private procedure
IntInterval(iNew, iresult)        /* result = intersect(iNew, iresult) */
  register DevInterval *iNew, *iresult;
{
  if (iNew->l > iresult->l)
    iresult->l = iNew->l;
  if (iNew->g < iresult->g)
    iresult->g = iNew->g;
} /* IntInterval */

private procedure
CopyTrimmedEdge(eOld, eNew, yOld, yNew)
  register DevTrapEdge *eOld, *eNew;
  register DevInterval *yOld, *yNew;
{
  register int dy;

  Assert
    ((yNew->l >= yOld->l) && (yNew->g <= yOld->g) && (yNew->g > yNew->l));
  if (eOld->xl == eOld->xg) {	/* it's a vertical edge */
    eNew->xl = eOld->xl;
    eNew->xg = eOld->xg;
    return;
  }
  eNew->ix = eOld->ix;
  eNew->dx = eOld->dx;
  dy = yNew->l - yOld->l;
  if (dy == 0)	   /* use old first line */
    eNew->xl = eOld->xl;
  else if (yOld->g - yNew->l == 1)	/* use old last line */
    eNew->xl = eOld->xg;
  else {	   /* bring l up to yNew->l */
    eNew->ix += dy * eNew->dx;
    eNew->xl = FTrunc(eNew->ix - eNew->dx);
  }
  if (yNew->g - yNew->l == 1)
    return;	   /* 1 line result */
  if (yNew->g < yOld->g) {	/* trim down to yNew->g */
    dy = yNew->g - yNew->l - 2;
    Assert(dy >= 0);
    eNew->xg = FTrunc(eNew->ix + dy * eNew->dx);
  } else
    eNew->xg = eOld->xg;
}

private integer
YDelta(edgeA, edgeB, yRange)
 /* when edges don't cross return -1 if edgeA<=edgeB else 0 if edgeB<edgeA,
 else return +ive dy to y-value beyond which they are equal or reversed */
  register DevTrapEdge *edgeA, *edgeB;
  register int yRange;
{
  register int dy;

  if (yRange == 1)
    return((edgeA->xl <= edgeB->xl) ? -1 : 0);
  if (edgeA->xl < edgeB->xl) {
    if (edgeA->xg > edgeB->xg)
      goto FindDelta;
    return(-1);
  }
  if (edgeA->xl > edgeB->xl) {
    if (edgeA->xg < edgeB->xg)
      goto FindDelta;
    return(0);
  }
  return((edgeA->xg <= edgeB->xg) ? -1 : 0);
FindDelta:  /* here we know they cross */
  if (yRange == 2)
    return(1);
  if ((yRange == 3) || (edgeB->dx == edgeA->dx))
    if ((edgeA->xl <= edgeB->xl) ?
        (edgeA->ix <= edgeB->ix) : (edgeA->ix > edgeB->ix))
      dy = yRange - 1; /* ix's have same ordering as xl's */
    else
      dy = 1;
  if (yRange < 4)
    return (dy);
  if (edgeA->xl == edgeA->xg)
    dy = (FixInt(edgeA->xg) - edgeB->ix) / edgeB->dx;
  else if (edgeB->xl == edgeB->xg)
    dy = (FixInt(edgeB->xg) - edgeA->ix) / edgeA->dx;
  else if (edgeB->dx == edgeA->dx)
    return (dy); /* calculated above */
  else
    dy = (edgeA->ix - edgeB->ix) / (edgeB->dx - edgeA->dx);
  dy += 1;
  if (dy <= 0)
    return(1);
  if (dy >= yRange)
    return(yRange - 1);
  return (dy);
} /* YDelta */

public procedure
OKTrap(t, strict)
  register DevTrap *t;
  register boolean strict;
{
  register int yRange = t->y.g - t->y.l;
  register int failure = 0;

  if (yRange < 3) {
    if (yRange <= 0)
      CantHappen(); /* NOT willing to OK empty ones! */
    return;
  }
  return;
}		   /* OKTrap */

#if DPSXA
public integer XATrapTrapInt(t0, t1, yptr, callback, callbackarg, translation)
  DevTrap *t0, *t1;
  DevInterval *yptr; /* optional range restriction */
  void (*callback) ();/* takes a (DevTrap *), and (char *) */
  char *callbackarg;
  DevCd translation;
{ 
  if(translation.x == 0 && translation.y == 0) 
  	TrapTrapInt(t0, t1, (DevInterval *)NULL, callback, callbackarg);
  else {
  	DevTrap T0;
  	DevShort dx,dy;
  	register Fixed fdx;
  	dx = translation.x; dy = translation.y;
  	fdx = FixInt(dx);
  	T0.y.l = t0->y.l + dy;
  	T0.y.g = t0->y.g + dy;
  	T0.l.xl = t0->l.xl + dx;
  	T0.l.xg = t0->l.xg + dx;
  	T0.l.ix = t0->l.ix + fdx;
  	T0.l.dx = t0->l.dx;
  	T0.g.xl = t0->g.xl + dx;
  	T0.g.xg = t0->g.xg + dx;
  	T0.g.ix = t0->g.ix + fdx;
  	T0.g.dx = t0->g.dx;
  	TrapTrapInt(&T0, t1, (DevInterval *)NULL, callback, callbackarg);
  }
}
#endif /* DPSXA */

public integer TrapTrapInt(t0, t1, yptr, callback, callbackarg)
  DevTrap *t0, *t1;
  DevInterval *yptr; /* optional range restriction */
  void (*callback) ();/* takes a (DevTrap *), and (char *) */
  char *callbackarg;
{
  register int yRange, dy, ll;
  DevTrap t[2];
  DevInterval y, yl;

  /* find the common y range */
  if (yptr) {
    y = *yptr;
    IntInterval(&(t0->y), &y);
  } else 
    y = t0->y;
  IntInterval(&(t1->y), &y);
  yRange = y.g - y.l;
  if (yRange <= 0)
    return;           /* disjoint in y */

  /* copy edges to our storage, trim them to common y range */
  CopyTrimmedEdge(&(t0->l), &t[0].l, &(t0->y), &y);
  CopyTrimmedEdge(&(t0->g), &t[0].g, &(t0->y), &y);
  CopyTrimmedEdge(&(t1->l), &t[1].l, &(t1->y), &y);
  CopyTrimmedEdge(&(t1->g), &t[1].g, &(t1->y), &y);
  /* define indices of inner edges at y.l */
  dy = YDelta(&t[1].g, &t[0].l, yRange);
  switch (dy) {
  case -1:
    return;
  case 0:
    break;
  default:
    goto CrossingFound;
  }
  dy = YDelta(&t[0].g, &t[1].l, yRange);
  switch (dy) {
  case -1:
    return;
  case 0:
    break;
  default:
    goto CrossingFound;
  }
  dy = YDelta(&t[0].l, &t[1].l, yRange);
  if (dy>0)
    goto CrossingFound;
  ll = -dy; /* remember index of innermost lesser edge */
  dy = YDelta(&t[1].g, &t[0].g, yRange);
  if (dy>0)
    goto CrossingFound;
  /* else single trap defined by inner edges */
  if (ll != -dy)
    t[ll].g = t[-dy].g;
  t[ll].y = y;
#if (STAGE == DEVELOP)
  OKTrap(&t[ll], false);
#endif
  (*callback)(&t[ll], callbackarg);
  return;
CrossingFound:
  /* though might avoid if (yRange>2); divide at dy and recurse */
  t[0].y = t[1].y = y;
  yl.l = y.l;
  yl.g = y.l + dy;
  TrapTrapInt(&t[0], &t[1], &yl, callback, callbackarg);
  y.l = yl.g;
  TrapTrapInt(&t[0], &t[1], &y, callback, callbackarg);
} /* TrapTrapInt */

private procedure EdgeInit(e, x, dx, dy, yBump, lines)
  register DevTrapEdge *e;
  Fixed x, dy;
  register Fixed dx, yBump;
  integer lines;
{
  e->dx = 0; /* show it's undefined */
  if (dx == 0) {
    e->ix = x;
    return;
  }
  if (dy > ONE) {
    e->dx = dx = tfixdiv(dx, dy); /* truncate instead of round */
    dx = fixmul(yBump, dx);
  } else
    dx = muldiv(yBump, dx, dy, false); /* == fixdiv(fixmul(yBump, dx), dy) */
  e->ix = x + dx;
  if (minTrapPrecision && lines > 3) {
    /* round ix and dx so that the fractions have trailing 0s */
    integer mask;
    Fixed endx, err, altend, olddx;
    endx = e->ix + (lines-3) * e->dx; /* the ideal endpoint */
    for (mask = 255; mask != 0; mask >>= 1) {
      x = e->ix & ~mask; /* truncate x so FTrunc(x) == FTrunc(ideal ix) */
      /* round off bottom bits of dx */
      olddx = dx;
      dx = (e->dx + ((mask+1)>>1)) & ~mask;
      if (mask < 255 && dx == olddx) continue;
      /* test size of error introduced */
      altend = x + (lines-3)*dx;
      if (FTrunc(altend) != FTrunc(endx)) continue;
      err = altend - endx;
      if (os_labs(err) > 0x4000L) continue; /* must be within .25 pixel */
      e->ix = x;
      e->dx = dx;
      return;
      }
    }
} /* EdgeInit */

#define lowpart(f) ((f) & 0xFFFFL)

public procedure
BresenhamMT(x, y, dx, dy) /* always gives Closed result; never a nil trap */
  register Fixed x, y, dx, dy;
{
  register DevTrap *t;
  register int lines;
  register DevPrim * trapsDP = ms->trapsDP;

  if (trapsDP->items == trapsDP->maxItems)
    (*ms->procs->trapsFilled)(ms);
  t = trapsDP->value.trap + trapsDP->items;
  if (dy < 0) { x += dx; dx = -dx; y += dy; dy = -dy; }
  if (dx < 0) { /* only trim ends if slope is negative */
    if (dy > 0 && lowpart(y+dy) == 0) dy--;
    if (lowpart(x) == 0) { x--; dx++; }
    }
  lines = (t->y.g = FTrunc(y + dy) + 1) - (t->y.l = FTrunc(y));
  {
    boolean steep = os_labs(dx) < dy;
    Fixed yBump;
    integer x0, x1;
    
    x0 = FTrunc(x);
    x1 = FTrunc(x + dx);
    if (x0 == x1) { /* vertical */
      t->l.xl = t->l.xg = x0;
      t->g.xl = t->g.xg = x0 + 1;
      goto DoBounds;
    }

    if (dx < 0) {
      t->g.xl = x0 + 1;
      if (lines == 1) { t->l.xl = x1; goto DoBounds; }
      t->l.xg = x1;
    } else {
      t->l.xl = x0;
      if (lines == 1) { t->g.xl = x1 + 1; goto DoBounds; }
      t->g.xg = x1 + 1;
    }

    if (steep)
      y -= HALF; /* solve for ix @ mid-y in 2nd row of pixels */
    else
      x += HALF; /* solve for closest-x @ y between rows 1 & 2 of pixels */

    yBump = FixInt(t->y.l + 1) - y;
    Assert((0 < yBump) && (yBump <= (ONE + HALF)));
    EdgeInit(&(t->l), x, dx, dy, yBump, lines);

    if (steep)
      t->g.ix = t->l.ix + ONE;
    else if (dx < 0) { 
      t->g.ix = t->l.ix;
      t->l.ix += t->l.dx;
    } else
      t->g.ix = t->l.ix + t->l.dx;

    if (dx < 0) {
      t->l.xl = FTrunc(t->g.ix);
      if (x0 < t->l.xl) t->l.xl = x0;
      t->g.xg = FTrunc(t->g.ix + t->l.dx * (lines - 2));
      if (x1 + 1 > t->g.xg) t->g.xg = x1 + 1;
    } else {
      t->g.xl = FTrunc(t->l.ix);
      if (x0 + 1 > t->g.xl) t->g.xl = x0 + 1;
      t->l.xg = FTrunc(t->l.ix + t->l.dx * (lines - 2));
      if (x1 < t->l.xg) t->l.xg = x1;
    }

    t->g.dx = t->l.dx;
  }
DoBounds:
  {
    register int min, max;

    if (lines == 1) {
      min = t->l.xl;
      max = t->g.xl;
    } else if (dx < 0) {
      min = t->l.xg;
      max = t->g.xl;
    } else {
      min = t->l.xl;
      max = t->g.xg;
    }
    if (min >= max)
      return;	   /* another nil trap */
#if (STAGE == DEVELOP)
    OKTrap(t, false);
#endif
    trapsDP->items++;
#if DPSXA
    trapsDP->xaOffset = xaOffset;
#endif /* DPSXA */
    if (ms->haveTrapBounds)
      return;
    if (min < trapsDP->bounds.x.l)
      trapsDP->bounds.x.l = min;
    if (max > trapsDP->bounds.x.g)
      trapsDP->bounds.x.g = max;
    if ((min = t->y.l) < trapsDP->bounds.y.l)
      trapsDP->bounds.y.l = min;
    if ((max = t->y.g) > trapsDP->bounds.y.g)
      trapsDP->bounds.y.g = max;
  }
}		   /* BresenhamMT */

public procedure QBresenhamMT(bounds, x, y, dx, dy)
  DevBounds bounds;
  Fixed x, y, dx, dy;
{
  DevTrap *t;
  Fixed steep;
  int lines;
  DevPrim *trapsDP = ms->trapsDP;

  if (trapsDP->items == trapsDP->maxItems) {
    (*ms->procs->trapsFilled)(ms);
    trapsDP->bounds = bounds;
  }
  t = trapsDP->value.trap + trapsDP->items;
  trapsDP->items++;
#if DPSXA
  trapsDP->xaOffset = xaOffset;
#endif /* DPSXA */

  if (dy < 0) { x += dx; dx = -dx; y += dy; dy = -dy; }
  steep = dx;
  if (dx < 0) { /* only trim ends if slope is negative (Why?) */
    if (dy > 0 && lowpart(y+dy) == 0) dy--;
    if (lowpart(x) == 0) { x--; dx++; }
    steep = -steep; /* steep = absval(dx) */
    }
  lines = (t->y.g = FTrunc(y + dy) + 1) - (t->y.l = FTrunc(y));
  {
    Fixed yBump, tmp, eix, edx;
    integer x0, x1;
    
    x0 = FTrunc(x);
    x1 = FTrunc(x + dx);
    if (x0 == x1) { /* vertical */
      t->l.xl = t->l.xg = x0;
      t->g.xl = t->g.xg = x0 + 1;
      return;
    }
    if (lines == 1) { /* horizontal */
      if (dx < 0) {
        t->g.xl = x0 + 1;
        t->l.xl = x1;
      } else {
        t->g.xl = x1 + 1;
        t->l.xl = x0;
      }
      return;
    }

    if (steep < dy) {
      steep = 1;
      y -= HALF; /* steep:   Solve for ix @ mid-y in 2nd pixel row */
    } else {
      steep = 0;
      x += HALF; /* shallow: Solve for closest-x @ y between pixel rows 1+2 */
    }
    yBump = FixInt(t->y.l + 1) - y;

    /* dx is guaranteed to be != 0, and minTrapPrecision is false */
    if (dy > ONE) {
      edx = tmp = tfixdiv(dx, dy); /* truncate instead of round */
      tmp = fixmul(yBump, tmp);
    } else {
      tmp = muldiv(yBump, dx, dy, false); /* == fixdiv(fixmul(yBump,dx),dy) */
      edx = 0; /* show it's undefined */
    }
    eix = x + tmp;

    if (dx < 0) {
      t->g.xl = x0 + 1;
      t->l.xg = x1;
    } else {
      t->l.xl = x0;
      t->g.xg = x1 + 1;
    }

    if (steep) {		/* steep */
      t->g.ix = eix + ONE; t->l.ix = eix;
      if (dx < 0) { 
	t->l.xl = x0;
	t->g.xg = x1 + 1;
      } else {
	t->g.xl = x0 + 1;
	t->l.xg = x1;
      }
    }
    else {			/* shallow */
      tmp = FTrunc(eix + edx * (lines - 2));
      if (dx < 0) { 
        t->l.ix = eix + edx;
        t->g.ix = eix;
	t->l.xl = FTrunc(eix);
	t->g.xg = tmp;
      } else {
	t->l.ix = eix;
        t->g.ix = eix + edx;
	t->l.xg = tmp;
	t->g.xl = FTrunc(eix);
      }
    }
    t->g.dx = t->l.dx = edx;
  }
}

public procedure AppendTraps(dp) register DevPrim *dp; {
  register DevTrap *from;
  register DevPrim *tp = ms->trapsDP;
  register integer items, i;
  Assert (tp->type == trapType);
  while (dp) {
    Assert(dp->type == trapType);
    items = dp->items;
    from = dp->value.trap;
    while (items) {
      if (tp->items == tp->maxItems)
        (*ms->procs->trapsFilled)(ms);
      if (!ms->haveTrapBounds)
        MergeDevBounds(&tp->bounds, &tp->bounds, &dp->bounds);
      i = tp->maxItems - tp->items;
      if (i > items) i = items;
      os_bcopy(from, tp->value.trap + tp->items,
               i * sizeof(DevTrap));
      from += i;
      tp->items += i;
      items -= i;
      }
    dp = dp->next;
    }
  }

public procedure AddTrap(yt, yb, xtl, xtr, xbl, xbr)
  Fixed yt, yb, xtl, xtr, xbl, xbr; {
  register DevTrap *t;
  register Fixed xl = xbl, xg = xbr;
  register Fixed dxl = xtl - xl;
  register Fixed dxg = xtr - xg;
  int lines;
  Fixed yBump;
  boolean dl, dg;
  register DevPrim *trapsDP = ms->trapsDP;

  if (trapsDP->items == trapsDP->maxItems)
    (*ms->procs->trapsFilled)(ms);
  t = trapsDP->value.trap + trapsDP->items;
  {
    register Fixed yg = yt, yl = yb, dy = yg - yl;

    if (yg==yl) yg++;
    if (xg==xl && dxl==0 && dxg==0) xg++;	/* If trap    is 0 width vert
						   rectangle, nudge it.	   */
    xg += 0xFFFFL;
    yg += 0xFFFFL;
    t->l.xl = FTrunc(xl);
    t->g.xl = FTrunc(xg);
    t->y.l = FTrunc(yl);
    t->y.g = FTrunc(yg);
    lines = t->y.g - t->y.l;
    if (lines <= 0)
      return;	   /* no trap */
    if (lines == 1) {	/* 1-liner w/no interpolation */
      if (dxl < 0)
	t->l.xl = FTrunc(xl + dxl);
      if (dxg > 0)
	t->g.xl = FTrunc(xg + dxg);
      goto DoBounds;
    }
    t->l.xg = FTrunc(xl + dxl);
    t->g.xg = FTrunc(xg + dxg);
    dl = (t->l.xg != t->l.xl);
    dg = (t->g.xg != t->g.xl);
    if (!dl && !dg)
      goto DoBounds;	/* a rectangle */
    yBump = FixInt(t->y.l + 1) - yl;
    Assert((0 < yBump) && (yBump <= ONE));
    if (dl) {	   /* slope on lesser edge */
      EdgeInit(&(t->l), xl, dxl, dy, yBump, lines);
      if (dxl < 0) {
	t->l.xl = FTrunc(t->l.ix);
	t->l.ix += t->l.dx;
      } if (dxl > 0)
	t->l.xg = FTrunc(t->l.ix + t->l.dx * (lines - 2));
    }
    if (dg) {	   /* slope on greater edge */
      if ((dxl == dxg) && dl) {	/* parallelogram, copy lesser edge */
	t->g.ix = t->l.ix + xg - xl;
	t->g.dx = t->l.dx;
	if (dxl < 0) /* get ix back to state after EdgeInit */
	  t->g.ix -= t->l.dx;
      } else
	EdgeInit(&(t->g), xg, dxg, dy, yBump, lines);
      if (dxg > 0) {
	t->g.xl = FTrunc(t->g.ix);
	t->g.ix += t->g.dx;
      } if (dxg < 0)
	t->g.xg = FTrunc(t->g.ix + t->g.dx * (lines - 2));
    }
  }
DoBounds:
  {
    register int min, max;

    if (lines == 1) {
      min = t->l.xl;
      max = t->g.xl;
    } else {
      min = (dxl < 0) ? t->l.xg : t->l.xl;
      max = (dxg > 0) ? t->g.xg : t->g.xl;
    }
    if (min >= max)
      return;	   /* another nil trap */
#if (STAGE == DEVELOP)
    OKTrap(t, true);
#endif 
    trapsDP->items++;
    if (ms->haveTrapBounds)
      return;
    if (min < trapsDP->bounds.x.l)
      trapsDP->bounds.x.l = min;
    if (max > trapsDP->bounds.x.g)
      trapsDP->bounds.x.g = max;
    if ((min = t->y.l) < trapsDP->bounds.y.l)
      trapsDP->bounds.y.l = min;
    if ((max = t->y.g) > trapsDP->bounds.y.g)
      trapsDP->bounds.y.g = max;
  }
}		   /* AddTrap */

public procedure
MakeBounds(b, yl, yg, xl, xg)
  register DevBounds *b;
  register Fixed yl, yg;
  register Fixed xl, xg;
{
  Assert((yg >= yl) && (xg >= xl));
  if (yg==yl) yg++;	/* Just in case on pixel crack.			   */
  if (xg==xl) xg++;
  xg += 0xFFFFL;
  yg += 0xFFFFL;
  b->x.l = FTrunc(xl);
  b->x.g = FTrunc(xg);
  b->y.l = FTrunc(yl);
  b->y.g = FTrunc(yg);
}		   /* MakeBounds */

public BBoxCompareResult BoxTrapCompare(figbb, clipt, inner, outer, rt)
  register DevBounds *figbb;
  register DevInterval *inner, *outer;
  register DevTrap *clipt, *rt;
/* BoxTrapCompare returns the result of comparing the figure bounding
   box with the clipping trapezoid.  Returns "inside" iff "figbb" is
   completely inside "clipt"; returns "outside" iff they are disjoint;
   and returns "overlap" iff "figbb" is partially inside "clipt" and
   partially outside "clipt". If (rt) interpolation will be made
   on diagonal edges of the clipt, storing the result in *rt, where
   necessary to refine the accuracy of the result. In the case that
   "overlap" is returned, a complete reduced trapezoid will be stored.
   A reduced trapezoid is the smallest possible trapezoid whose
   intersection with figbb is the same as clipt's.  (In other cases,
   rt may be altered but meaningless.) */
{
  register BBoxCompareResult result;

  if ((clipt->y.l >= clipt->y.g) || /* null trap (causes problems later!) */
   (clipt->y.l >= figbb->y.g) || (clipt->y.g <= figbb->y.l))
    return (outside); /* disjoint in y */
  if (clipt->l.xl < clipt->l.xg) {
    inner->l = clipt->l.xg;
    outer->l = clipt->l.xl;
  } else {
    inner->l = clipt->l.xl;
    outer->l = clipt->l.xg;
  }
  if (outer->l >= figbb->x.g)
    return (outside);
  if (clipt->g.xl < clipt->g.xg) {
    inner->g = clipt->g.xl;
    outer->g = clipt->g.xg;
  } else {
    inner->g = clipt->g.xg;
    outer->g = clipt->g.xl;
  }
  if (outer->g <= figbb->x.l)
    return (outside);
  /* figbb is not trivially outside clipt */
  if (!rt) /* no reduction is desired */
    return ((
      (inner->l <= figbb->x.l) &&
      (inner->g >= figbb->x.g) &&
      (clipt->y.l <= figbb->y.l) &&
      (clipt->y.g >= figbb->y.g)) ? inside : overlap);
  /* else get the final y result */
  if (clipt->y.l <= figbb->y.l) {
    result = inside;
    rt->y.l = figbb->y.l;
  } else {
    result = overlap;
    rt->y.l = clipt->y.l;
  }
  if (clipt->y.g >= figbb->y.g)
    rt->y.g = figbb->y.g;
  else {
    result = overlap;
    rt->y.g = clipt->y.g;
  }
  if ((result == inside) &&
    (inner->l <= figbb->x.l) && (inner->g >= figbb->x.g))
      return (inside); /* y & x both inside without reduction */
  CopyTrimmedEdge(&clipt->l, &rt->l, &clipt->y, &rt->y);
  CopyTrimmedEdge(&clipt->g, &rt->g, &clipt->y, &rt->y);
  if ((rt->y.l != clipt->y.l) || (rt->y.g != clipt->y.g)) {
  /* y reduction occurred, recompute inner, outer & x-outside check */
    if (rt->l.xl < rt->l.xg) {
      inner->l = rt->l.xg;
      outer->l = rt->l.xl;
    } else {
      inner->l = rt->l.xl;
      outer->l = rt->l.xg;
    }
    if (outer->l >= figbb->x.g)
      return (outside);
    if (rt->g.xl < rt->g.xg) {
      inner->g = rt->g.xl;
      outer->g = rt->g.xg;
    } else {
      inner->g = rt->g.xg;
      outer->g = rt->g.xl;
    }
    if (outer->g <= figbb->x.l)
      return (outside);
    /* now reduced x either overlaps or is inside */
    if ((result == inside) && /* in y, see if x is also */
      (inner->l <= figbb->x.l) && (inner->g >= figbb->x.g))
        return (inside);
  }
  /* now we have irreducible overlap, but may be able to reduce x to figbb */
  if (inner->l <= figbb->x.l)
    rt->l.xl = rt->l.xg = figbb->x.l;
  if (inner->g >= figbb->x.g)
    rt->g.xl = rt->g.xg = figbb->x.g;
  return (overlap);
} /* BoxTrapCompare */

public boolean PointInTraps(t, items, dc)
  register DevTrap *t; integer items; DevCd dc; {
  register integer x, y, z, w;
  x = dc.x; y = dc.y;
  for (; items--; t++) {
    z = t->y.l;
    if (y < z) continue; /* below */
    if (y == z) { /* on first scanline */
      if (x >= t->l.xl && x < t->g.xl) return true;
      continue;
      }
    z = t->y.g;
    if (y >= z) continue; /* above */
    if (y + 1 == z) { /* on last scanline */
      if (x >= t->l.xg && x < t->g.xg) return true;
      continue;
      }
    z = t->l.xl; w = t->l.xg;
    if (x < z && z < w) continue; /* left */
    if (x >= z && x >= w) { /* maybe inside */
      z = t->g.xl; w = t->g.xg;
      if (x < z && x < w) return true; /* inside */
      }
    else z = t->g.xl; w = t->g.xg;
    if (x >= z && x >= w) continue; /* right */
    z = y - t->y.l - 1; /* number of dx steps beyond ix */
    if (t->l.xl == t->l.xg) w = t->l.xl; /* vertical left edge */
    else w = FTrunc(t->l.ix + z * t->l.dx);
    if (x < w) continue; /* left */
    if (t->g.xl == t->g.xg) w = t->g.xl; /* vertical right edge */
    else w = FTrunc(t->g.ix + z * t->g.dx);
    if (x >= w) continue; /* right */
    return true;
    }
  return false;
  }
