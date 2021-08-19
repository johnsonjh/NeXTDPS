/* Clipping utilities

		Copyright 1986, 1987, 1988 -- Adobe Systems, Inc.
	    PostScript is a trademark of Adobe Systems, Inc.
NOTICE:  All information contained herein or attendant hereto is, and
remains, the property of Adobe Systems, Inc.  Many of the intellectual
and technical concepts contained herein are proprietary to Adobe Systems,
Inc. and may be covered by U.S. and Foreign Patents or Patents Pending or
are protected as trade secrets.  Any dissemination of this information or
reproduction of this material are strictly forbidden unless prior written
permission is obtained from Adobe Systems, Inc.

Original version: Bill Paxton, April 16, 1986
Edit History:
Scott Byer: Thu Jun  1 15:23:36 1989
Bill Paxton: Fri Aug 19 13:13:24 1988
Doug Brotz: Thu Jul 24 16:58:57 1986
Ed Taft: Sat Jul 11 11:31:39 1987
Ivor Durham: Fri Sep 23 16:00:47 1988
Mike Schuster: Fri Jun 12 13:16:09 1987
Jim Sandman: Fri Sep 22 13:49:23 1989
Paul Rovner: Fri Nov 24 10:50:44 1989
Jack Newlin 17May90 move clipBuffData into procs for reentrancy
Terry Donahue 10Aug90 move clipBuffSz check out of n1 > 0 check in QIntersect[Trp]
End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include FP
#include DEVICE
#include EXCEPT
#include PSLIB
#include FOREGROUND

#define MaxInt16 (0x7FFF)
#define MinInt16 (0x8000)

#define clipBuffSz (1000)

private procedure DumpBuff(
    buffStart, callBack, ymin, ymax, xmin, xmax, d, args)
  procedure (*callBack)();
  Int16 ymin, ymax, xmin, xmax, *d, *buffStart; char *args; {
  DevRun run;
  run.bounds.y.l = ymin; run.bounds.y.g = ymax + 1;
  run.bounds.x.l = xmin; run.bounds.x.g = xmax + 1;
  if ((run.datalen = d - buffStart) == 0) return;
  if (run.datalen < 0) CantHappen();
  run.data = buffStart;
  run.indx = NULL;
  (*callBack)(&run, args);
  }


#define INDXSHFT (5)
#define INDXMASK ((1 << INDXSHFT)-1)

public integer BytesForRunIndex(r) DevRun *r; {
  return
    ((r->bounds.y.g - r->bounds.y.l + INDXMASK) >> INDXSHFT)*sizeof(DevShort);
  }

public procedure BuildRunIndex(r) DevRun *r; {
  register PInt16 p, strtP, indx;
  register int h, height, msk;
  if (r->indx == NULL)
    r->indx = (DevShort *)os_calloc(1, BytesForRunIndex(r));
  indx = r->indx;
  if (indx == NULL) return;
  p = r->data; strtP = p; h = 0;
  height = r->bounds.y.g - r->bounds.y.l;
  msk = INDXMASK;
  while (h < height) {
    if ((h & msk) == 0) *(indx++) = p - strtP;
    p += ((*p) << 1) + 1;
    h++;
    }
  }

public PInt16 RunArrayRow(r, yr)
  register DevRun *r; register int yr; {
  /* return pointer to data for row yr in run array r. */
  /* returns nil if yr is not part of the run array. */
  register int cnt, y;
  register Int16 *p;
  y = r->bounds.y.l;
  if (y > yr || yr >= r->bounds.y.g) return NIL;
  p = r->data;
  if (y == yr) return p;
  cnt = yr - y;
  if (r->indx != NIL) {
    p += r->indx[cnt >> INDXSHFT]; cnt &= INDXMASK;
    }
  if (cnt) {
    cnt--;
    do { p += ((*p) << 1) + 1; } while (--cnt >= 0);
    }
  return p;
  }

public procedure QIntersect(r1, r2, callBack, args)
  DevRun *r1, *r2; procedure (*callBack)();
  char *args; {
  /* callBack with results. */
  /* if the intersection is null, then will not callBack. */
  Int16 clipBuffData[clipBuffSz];
  Int16 strtY, lastY, y, ymax, xmin, xmax, pairs, *p, *strtD;
  register PInt16 d, d1, d2;
  PInt16 lastD;
  if (r1->bounds.x.l >= r2->bounds.x.g) return;
  if (r1->bounds.x.g <= r2->bounds.x.l) return;
  if (r1->bounds.y.l < r2->bounds.y.l) {
    strtY = r2->bounds.y.l;
    d1 = RunArrayRow(r1, strtY);
    if (d1 == NIL) return;
    d2 = r2->data;
    }
  else {
    strtY = r1->bounds.y.l;
    d2 = RunArrayRow(r2, strtY);
    if (d2 == NIL) return;
    d1 = r1->data;
    }
  ymax = MIN(r1->bounds.y.g, r2->bounds.y.g) - 1; y = strtY;

  FGEnterMonitor();
  DURING
  d = clipBuffData; /* data pointer for intersection */
  lastD = strtD = d;
  xmin = MaxInt16; xmax = MinInt16;
  while (y <= ymax) {
    register int lft1, rht1, lft2, rht2, n1, n2;
    pairs = 0; /* keep count of number of pairs for this row */
    p = d; /* save pointer to pairs count */
    d++; /* advance d to place for first pair */
    n1 = *(d1++); /* number of pairs in r1 for this row */
    n2 = *(d2++); /* number of pairs in r2 for this row */
    /* make sure intersection will fit in remaining space in buffer.
     * This must be outside of the following if statement, since otherwise
     * d - strtD may increase past clipBuffSz with a chain of empty scanlines
     */
    if (d - strtD + ((n1 + n2) << 1) >= clipBuffSz) { /* dump buffer */
      if (lastD > strtD)
        DumpBuff(clipBuffData, callBack, strtY, lastY, xmin, xmax-1, lastD, args);
      xmin = MaxInt16; xmax = MinInt16;
      strtY = y; d = clipBuffData; lastD = strtD = d; p = d; d++;
      }
    if (n1 > 0 && n2 > 0) {
      lft1 = *(d1++); rht1 = *(d1++); /* left and right for r1 pair */
      lft2 = *(d2++); rht2 = *(d2++); /* left and right for r2 pair */
      n1--; n2--; /* reduce number of pairs */
      while (true) {
        if (rht1 > lft2 && rht2 > lft1) { /* overlap */
          *(d++) = MAX(lft1, lft2); /* first pixel in intersection */
	  *(d++) = MIN(rht1, rht2); /* first pixel after intersection */
	  pairs++;
	  }
        if (rht1 < rht2) { /* get next pair from r1 */
          if (n1 == 0) break; /* no more pairs in r1 for this row */
	  lft1 = *(d1++); rht1 = *(d1++); n1--;
          }
        else { /* get next pair from r2 */
	  if (n2 == 0) break; /* no more pairs in r2 for this row */
	  lft2 = *(d2++); rht2 = *(d2++); n2--;
          }
        }
      }
    if (n1 > 0) d1 += (n1 << 1); /* advance over trailing pairs */
    if (n2 > 0) d2 += (n2 << 1);
    *p = pairs;
    if (pairs > 0) { /* update xmin and xmax */
      n1 = p[1]; if (n1 < xmin) xmin = n1;
      n1 = d[-1]; if (n1 > xmax) xmax = n1;
      lastY = y; lastD = d;
      }
    else if (y == strtY) { /* skip over leading empty scanlines */
      strtY++; d = strtD; }
    y++;
    }
  if (lastD > strtD)
    DumpBuff(clipBuffData,callBack,strtY,lastY,xmin,xmax-1,lastD,args);
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();
  } /* QIntersect */

public procedure QIntersectTrp(r1, trp, callBack, args)
  DevRun *r1;
  register DevTrap *trp; procedure (*callBack)(); char *args; {
  /* callBack with results. */
  /* if the intersection is null, then will not callBack. */
  Int16 clipBuffData[clipBuffSz];
  Int16 tlast, strtY, lastY, y, ymax, xmin, xmax, *p, *strtD;
  register int lft1, rht1, lft2, rht2, n1, pairs;
  register PInt16 d, d1;
  Fixed lx, rx;
  PInt16 lastD;
  boolean leftSlope, rightSlope;
  xmin = MIN(trp->l.xl, trp->l.xg);
  if (xmin >= r1->bounds.x.g) return;
  xmax = MAX(trp->g.xl, trp->g.xg);
  if (xmax <= r1->bounds.x.l) return;
  lx = trp->l.ix; rx = trp->g.ix;
  lft2 = trp->l.xl; rht2 = trp->g.xl;
  leftSlope = (trp->l.xg != lft2);
  rightSlope = (trp->g.xg != rht2);
  if (r1->bounds.y.l <= trp->y.l) {
    strtY = trp->y.l;
    d1 = RunArrayRow(r1, strtY);
    if (d1 == NIL) return;
    }
  else {
    strtY = r1->bounds.y.l;
    d1 = r1->data;
    if (strtY > trp->y.g) return;
    if (strtY == trp->y.g) { lft2 = trp->l.xg; rht2 = trp->g.xg; }
    else {
      integer delta;
      delta = strtY - trp->y.l - 1;
      if (leftSlope) {
        if (delta > 0) lx += (trp->l.dx) * delta;
	lft2 = FTrunc(lx); lx += trp->l.dx;
	}
      if (rightSlope) {
        if (delta > 0) rx += (trp->g.dx) * delta;
	rht2 = FTrunc(rx); rx += trp->g.dx;
	}
      }
    }
  tlast = trp->y.g - 1;
  ymax = MIN(r1->bounds.y.g - 1, tlast); y = strtY;

  FGEnterMonitor();
  DURING
  d = clipBuffData; /* data pointer for intersection */
  lastD = strtD = d;
  xmin = MaxInt16; xmax = MinInt16;
  while (y <= ymax) {
    pairs = 0; /* keep count of number of pairs for this row */
    p = d; /* save pointer to pairs count */
    d++; /* advance d to place for first pair */
    n1 = *(d1++); /* number of pairs in r1 for this row */

    /* make sure intersection will fit in remaining space in buffer.
     * This must be outside of the following if statement, since otherwise
     * d - strtD may increase past clipBuffSz with a chain of empty scanlines
     */
    if (d - strtD + (n1 << 1) >= clipBuffSz) { /* dump buffer */
      if (lastD > strtD)
        DumpBuff(clipBuffData, callBack, strtY, lastY, xmin, xmax-1, lastD, args);
      xmin = MaxInt16; xmax = MinInt16;
      strtY = y; d = clipBuffData; lastD = strtD = d; p = d; d++;
      }
    if (n1 > 0) {
      lft1 = *(d1++); rht1 = *(d1++); /* left and right for r1 pair */
      n1--; /* reduce number of pairs */
      while (true) {
        if (rht1 > lft2 && rht2 > lft1) { /* overlap */
          *(d++) = MAX(lft1, lft2); /* first pixel in intersection */
	  *(d++) = MIN(rht1, rht2); /* first pixel after intersection */
	  pairs++;
	  }
        if (rht1 < rht2) { /* get next pair from r1 */
          if (n1 == 0) break; /* no more pairs in r1 for this row */
	  lft1 = *(d1++); rht1 = *(d1++); n1--;
          }
        else break;
        }
      }
    if (n1 > 0) d1 += (n1 << 1); /* advance over trailing pairs */
    *p = pairs;
    if (pairs > 0) { /* update xmin and xmax */
      n1 = p[1]; if (n1 < xmin) xmin = n1;
      n1 = d[-1]; if (n1 > xmax) xmax = n1;
      lastY = y; lastD = d;
      }
    else if (y == strtY) { /* skip over leading empty scanlines */
      strtY++; d = strtD; }
    y++;
    if (y == tlast) { lft2 = trp->l.xg; rht2 = trp->g.xg; }
    else {
      if (leftSlope) { lft2 = FTrunc(lx); lx += trp->l.dx; }
      if (rightSlope) { rht2 = FTrunc(rx); rx += trp->g.dx; }
      }
    }
  if (lastD > strtD)
    DumpBuff(clipBuffData,callBack, strtY, lastY, xmin, xmax-1, lastD, args);
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();
  } /* QIntersectTrp */

public procedure QIntersectBounds(r1, b, callBack, args)
  DevRun *r1; DevBounds *b; procedure (*callBack)();
  char *args; {
  /* callBack with results. */
  /* if the intersection is null, then will not callBack. */
  DevTrap trp;
  trp.y = b->y;
  trp.l.xl = trp.l.xg = b->x.l;
  trp.g.xl = trp.g.xg = b->x.g;
  QIntersectTrp(r1, &trp, callBack, args);
  }

public BBoxCompareResult QCompareBounds(r1, b)
  register DevRun *r1; register DevBounds *b; {
  /* returns inside, outside, or overlap (like BBCompare) */
  register PInt16 d1;
  boolean partIn, partOut;
  register int lft1, lft2, rht1, rht2, n1, cnt;
  Int16 y;
  if (r1->bounds.y.l >= b->y.g || r1->bounds.y.g <= b->y.l) return outside;
  if (r1->bounds.x.l >= b->x.g || r1->bounds.x.g <= b->x.l) return outside;
  partIn = false;
  if (b->x.l < r1->bounds.x.l || b->x.g > r1->bounds.x.g ||
      b->y.l < r1->bounds.y.l || b->y.g > r1->bounds.y.g) partOut = true;
  else partOut = false;
  if (r1->bounds.y.l < b->y.l) { y = b->y.l; d1 = RunArrayRow(r1, y); }
  else { y = r1->bounds.y.l; d1 = r1->data; }
  cnt = MIN(r1->bounds.y.g, b->y.g) - y;
  lft2 = b->x.l; rht2 = b->x.g;
  while (cnt > 0 && !(partIn && partOut)) {
    n1 = *(d1++); /* number of pairs for this row */
    if (n1 == 0) partOut = true;
    else {
      lft1 = *(d1++); rht1 = *(d1++); /* left and right for pair */
      n1--; /* reduce number of pairs */
      while (true) {
        if (rht1 > lft2 && rht2 > lft1) { /* overlap */
          partIn = true;
          if (lft2 < lft1 || rht2 > rht1) partOut = true;
          break;
	  }
        if (n1 == 0 || lft1 > rht2) { partOut = true; break; }
        lft1 = *(d1++); rht1 = *(d1++); n1--;
        }
      if (n1 > 0) d1 += (n1 << 1); /* advance over trailing pairs */
      }
    cnt--;
    }
  if (partIn && partOut) return overlap;
  if (!partOut) return inside;
  return outside;
  }

