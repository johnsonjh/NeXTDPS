/* PostScript Graphics Reducer using Bresenham edges for path

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
Jim Sandman: Thu Oct 19 11:18:04 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include FP
#include GRAPHICS
#include DEVICE
#include EXCEPT
#include VM

#include "reducer.h"
#include "graphicspriv.h"
#include "path.h"

#define MaxInt16 (0x7FFF)
#define MinInt16 (0x8000)

typedef struct {
  /* augmented for easy splitting and use in scan-conversion */
  Int16 xb, xt;	/* trapL & trapR yield bounds */
  /* fields below not defined if (xt=xb) */ 
  Fixed x; /* truncate yield bound for y if((y=yb+1)<yt) */
  Fixed dx; /* then add this to x if(++y<yt) */
} TrpEdge;

typedef struct {
  /* augmented for easy splitting and use in scan-conversion */
  Int16 yb, yt; /* with trapL & trapR yield bounds */
  TrpEdge l, r;
} Trp, *PTrp;

typedef struct edgerec {
    struct edgerec *next; /* for list of edges with same y value */
    Int16 y0; /* min y value */
    Int16 x, xend; /* min and max location in scanline */
    Int16 xlast, ylast; /* x y location of last pixel */
    Fixed dx, dy, g;
    boolean right:1; /* true iff dx >= 0 */
    boolean up:1;    /* up edge or down edge */
    boolean atVertEdge:1; /* exactly vertical and at pixel boundary */
    boolean endsAtHorzEdge:1;
    boolean startsAtHorzEdge:1;
    boolean unused:11;
  } EdgeRec, *Edge;

typedef struct scanrec {
  Int16 y; /* y location for this list */
  Edge edge; /* list of edges for this y value */
  struct scanrec *next, *prev;
  } ScanRec, *ScanList;

#define HighMask 0xFFFF0000

private Edge edgeArray, freeEdge, endEdgeArray;
private ScanList scanArray, freeScan, endScanArray;
#define clipBuffSz (1000)
private Int16 *buffData, *endBuffData, *clipBuffData, *endClipBuffData;
private ScanList scanList, scn;
private boolean noCurPt;
private Fixed px0, py0;
private Fixed pxNP, pyNP;

private Edge SortEdges(edge) Edge edge;
{
/* simple bubble sort is fine since list is almost sorted to start with */
/* and list is almost always very short */
register Edge e, ep, elast, enxt, ex;
elast = NIL; /* everything is in proper order after elast */
while (true) { /* go through this loop for each bubble pass over list */
  e = edge; /* edge is the start of the list */
  ep = NIL; /* ep is the entry before e */
  ex = NIL; /* ex is where the final exchange took place */
  while (e != NIL)
    {
    enxt = e->next;
    if (enxt == NIL) break;
    if (e->x <= enxt->x)
      { /* order is ok */ if (e == elast) break; ep = e; e = enxt; }
    else
      { /* exchange e and e->next */
      e->next = enxt->next; enxt->next = e;
      if (ep == NIL) edge = enxt;
      else ep->next = enxt;
      ep = ex = enxt;
      }
    }
  if (ex == NIL) break; /* no exchanges on last cycle */
  elast = ex;
  }
return edge;
}  /* end of SortEdges */

private Edge MergeEdges(e1, e2) register Edge e1, e2;
{
register Edge edge, e;
if (e1 == NIL) return e2;
if (e2 == NIL) return e1;
if (e1->x < e2->x)
  { /* start from e1 */
  edge = e1; e1 = e1->next;  if (e1 == NIL) {edge->next = e2; return edge;}
  }
else
  { /* start from e2 */
  edge = e2; e2 = e2->next;  if (e2 == NIL) {edge->next = e1; return edge;}
  }
e = edge; /* e is always the end of the merged list */
while (true)
  {
  if (e1->x < e2->x)
    { /* take next from e1 */
    e = e->next = e1;
    e1 = e1->next;
    if (e1 == NIL) {e->next = e2; break;}
    }
  else
    { /* take next from e2 */
    e = e->next = e2; 
    e2 = e2->next; 
    if (e2 == NIL) {e->next = e1; break;} 
    }
  }
return edge;
}  /* end of MergeEdges */

private Edge AdvanceEdges(edge, strtY, endY)
  Edge edge; Int16 strtY, endY; {
  register Edge e, ep;
  register int x0, x1, cnt;
  register Fixed g, dy, dx;
  if (strtY == endY) return edge;
  ep = NIL; e = edge;
  Assert(strtY < endY);
  while (e != NIL) {
    if (e->ylast < endY) {
      e = e->next;
      if (ep == NIL) edge = e; else ep->next = e;
      continue; }
    if (e->dx == 0) goto Nxt;
    cnt = endY - MAX(e->y0, strtY);
    g = e->g; dy = e->dy; dx = e->dx;
    if (e->right) {
      x1 = e->xend;
      until (cnt-- == 0) {
        x0 = x1; g += dx;
        while (g >= 0) { x1++; g -= dy; }
	if (x1 > e->xlast) x1 = e->xlast;
	}
      if (e->ylast == endY) x1 = e->xlast;
      }
    else {
      x0 = e->x;
      until (cnt-- == 0) {
        x1 = x0; g -= dx;
	while (g >= 0) { x0--; g -= dy; }
	if (x0 < e->xlast) x0 = e->xlast;
	}
      if (e->ylast == endY) x0 = e->xlast;
      }
    e->g = g; e->x = x0; e->xend = x1;
    Nxt: ep = e; e = e->next;
    }
  return edge;
  }

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

public procedure QReduce(eofill, callBack, ymin, ymax, args)
  boolean eofill; procedure (*callBack)(); Int16 ymin, ymax;
  char *args;
{
register Edge e;
ScanList sl = scanList;
Edge edge, ep;
boolean inScan, twoEdges;
register Int16 *onoffdata, *onoffend;
Int16 strtX, strtY, *onoffline, xmin, xmax, pairs;
register int x, y, i;
integer cnt;
if (sl == NIL) return;
xmin = MaxInt16; xmax = MinInt16;
onoffline = onoffdata = buffData; onoffdata++;
onoffend = endBuffData - 8;
edge = NIL;
y = sl->y;
strtY = y;
while (true)
  {
  { /* by far the most common case is to sort a list of 2 edges */
    register Edge e1 = edge; /* e1 is the first item on the list */
    if (e1 == NIL) {} /* list is empty */
    else {
      e = e1->next; /* e is the second item on the list */
      if (e == NIL) {} /* single item on list */
      else if (e->next != NIL) edge = SortEdges(e1); /* more than 2 items */
      else if (e1->x <= e->x) {} /* order is ok */
      else { e->next = e1; e1->next = NIL; edge = e; } /* reverse them */
      }
    }
  if (sl != NIL && sl->y == y) {
    /* the most common case is to merge 1 new edge into a list of 2 edges */
    register boolean merged = false;
    if (sl->edge->next == NIL) { /* only one item to merge */
      register Edge e1 = edge; /* e1 is the first item on the list */
      if (e1 == NIL) { edge = sl->edge; merged = true; }
      else {
        e = e1->next; /* e is the second item on the list */
        if (e == NIL) {} /* single item on list; very rare case */
	else if (e->next == NIL) { /* 2 items on list */
          x = sl->edge->x; /* the location of the item to be added */
	  if (x <= e1->x) { /* insert at front */
            e = sl->edge; e->next = e1; edge = e;
            }
          else if (x >= e->x) { /* add to end */
            e->next = sl->edge;
            }
          else { /* put in middle */
            e1->next = sl->edge; e1->next->next = e;
            }
          merged = true;
          }
        }
      }
    if (!merged) edge = MergeEdges(edge, sl->edge);
    sl = sl->next;}
  if (edge == NIL && sl == NIL) goto Done;
  if (y > ymax) goto Done;
  if (y < ymin) {
    strtY = y;
    while (sl != NIL && sl->y <= ymin) { /* add to edge list */
      register Edge e1;
      e = sl->edge; Assert(e != NIL);
      while ((e1=e->next) != NIL) e = e1; /* move e to end of list */
      e->next = edge; edge = sl->edge; sl = sl->next;
      }
    edge = AdvanceEdges(edge, strtY, ymin);
    strtY = y = ymin;
    continue;
    }
  e = edge; ep = NIL; inScan = false; cnt = pairs = 0;
  if (e != NIL) {
    register Edge e1 = e->next;
    twoEdges = (e1 != NIL && e1->next == NIL);
    }
  else twoEdges = false;
  if (e != NIL) {if (e->x < xmin) xmin = e->x;}
  else x = xmax;
  while (e != NIL)
    { /* go across the scanline */
    x = e->x;
    if (!inScan) {inScan = true; strtX = x+1;}
    while ((i=e->x) <= x) {
      if (!(e->atVertEdge) && i < strtX) strtX = i;
      if ((i=e->xend) > x) x = i;
      i = e->ylast; /* get ylast in register */
      if (!twoEdges) {
        register int cntDelta;
        if (e->dy != 0)
          cntDelta = ((e->y0 == y && !(e->startsAtHorzEdge)) ||
	              (i == y && !(e->endsAtHorzEdge))) ? 1 : 2;
        else {
          cntDelta = 0;
          if (e->startsAtHorzEdge) cntDelta++;
	  if (e->endsAtHorzEdge) cntDelta++;
          }
	if (e->up) cnt += cntDelta; else cnt -= cntDelta;
        }
      if (e->dx != 0 && i != y)
        { /* update x and xend */
        if (i-1 == y)
          { /* next will be last for this edge */
          if (e->right) {e->x = e->xend; e->xend = e->xlast;}
          else {e->xend = e->x; e->x = e->xlast;}
          }
        else
          { /* do Bresenham */
          register Fixed d, g;
	  register int xn;
          d = e->dy; g = e->g;
          if (e->right)
            {
            e->x = xn = e->xend; g += e->dx;
            while (g >= 0) {xn++; g -= d;}
            if (xn > e->xlast) xn = e->xlast;
            e->xend = xn;
            }
          else
            {
            e->xend = xn = e->x; g -= e->dx;
            while (g > 0) {xn--; g -= d;}
            if (xn < e->xlast) xn = e->xlast;
            e->x = xn;
            }
          e->g = g;
          }
        }
      if (i == y)
        { /* remove e from edge list */
        register Edge e1 = ep; /* get ep into register */
        if (e1 == NIL) edge = e->next;
        else e1->next = e->next;
        }
      else ep = e;
      e = e->next;
      if (e == NIL) break;
      }
    if (twoEdges) {
      if (e == NIL) {
        *(onoffdata++) = strtX; *(onoffdata++) = x+1; pairs = 1;
        }
      }
    else if (inScan && (eofill ? cnt & 3 : cnt) == 0)
      {
      inScan = false; i = strtX;
      if (onoffdata >= onoffend)
        { /* flush the buffer */
        *(onoffline) = pairs;
	DumpBuff(buffData, callBack, strtY, y, xmin,
          (x > xmax)? x : xmax, onoffdata, args);
        xmin = MaxInt16; xmax = MinInt16; pairs = 0; strtY = y;
        onoffline = onoffdata = buffData; onoffdata++;
        }
      if (i <= onoffdata[-1] && pairs > 0) /* merge with previous */
        onoffdata[-1] = x+1;
      else
        { /* add a new pair */
        *(onoffdata++) = i;
        *(onoffdata++) = x+1;
        pairs++;
        }
      }
    }
  Assert(cnt == 0);
  if (x > xmax) xmax = x;
  *(onoffline) = pairs;
  if (onoffdata >= onoffend)
    { /* dump the buffer */
    DumpBuff(buffData, callBack, strtY, y, xmin, xmax, onoffdata, args);
    xmin = MaxInt16; xmax = MinInt16; strtY = y+1;
    onoffdata = buffData;
    }
  y++; onoffline = onoffdata++;
  }
Done:
if (onoffline > buffData)
  {
  DumpBuff(buffData, callBack, strtY, y-1, xmin, xmax, onoffline, args);
  }
}  /* end of QReduce */

#define FixOne (1<<16)

private Edge BuildEdge(fx0, fy0, fx1, fy1) Fixed fx0, fy0, fx1, fy1;
{
boolean up;
Fixed x0, y0, x1, y1;
register Fixed g, dx, dy;
register int x, xend;
register Edge edge;
if (fx1 < edgeminx) edgeminx = fx1;
if (fx1 > edgemaxx) edgemaxx = fx1;
if (fy1 < edgeminy) edgeminy = fy1;
if (fy1 > edgemaxy) edgemaxy = fy1;
if (fy0 < fy1) up = true;
else
  {
  register Fixed tmp;
  up = false;
  tmp = fx0; fx0 = fx1; fx1 = tmp;
  tmp = fy0; fy0 = fy1; fy1 = tmp;
  }
if (freeEdge >= endEdgeArray) LimitCheck();
edge = freeEdge++;
#define r dx
#define s dy
s = fy1;
if ((s & 0xFFFF) == 0) { /* upper y is exactly at pixel boundary */
  if (s == fy0) return NIL;
  /* discard lines that are exactly horizontal and on pixel edge */
  fy1--; /* if upper y is exactly at pixel boundary, move it down */
  edge->endsAtHorzEdge = true;
  }
else edge->endsAtHorzEdge = false;
if ((fy0 & 0xFFFF) == 0) edge->startsAtHorzEdge = true;
else edge->startsAtHorzEdge = false;
r = fx0; s = fx1;
if (r != s) { /* if line is not exactly vertical */
  if (r > s) { /* fx0 is right end */
    if ((r & 0xFFFF) == 0) fx0--; /* move it left if ends at pixel edge */
    }
  else { /* fx1 is right end */
    if ((s & 0xFFFF) == 0) fx1--; /* move it left if ends at pixel edge */
    }
  edge->atVertEdge = false;
  }
else { /* line is exactly vertical */
  if ((r & 0xFFFF) == 0) { fx0--; fx1--; edge->atVertEdge = true; }
  else edge->atVertEdge = false;
  }
#undef r
#undef s
  {
  register Fixed tmp = HighMask;
  x = FTrunc(x0 = fx0 & tmp);
  edge->y0 = FTrunc(y0 = fy0 & tmp);
  edge->xlast = FTrunc(x1 = fx1 & tmp);
  edge->ylast = FTrunc(y1 = fy1 & tmp);
  }
edge->up = up;
if (y0 == y1)
  { /* horizontal line */
  edge->dy = 0;
  if (x0 < x1) {edge->x = x; edge->xend = edge->xlast;}
  else {edge->x = edge->xlast; edge->xend = x;}
  }
else if (x0 == x1)
  { /* vertical line */
  edge->x = edge->xend = x;
  edge->dx = 0; edge->dy = edge->g = -1;
  }
else
  {
  register Fixed f;
  dx = (fx1 - fx0); dy = (fy1 - fy0); f = 0x60000000;
  if ((os_labs(dx) & f) != 0 || (dy & f) != 0) { dx >>= 2; dy >>= 2; }
  edge->dx = dx; edge->dy = dy;
  xend = x; f = FixOne;
  if (dx >= 0)
    {
    edge->right = true;
    g = fixmul(fx0-x0-f, dy) - fixmul(fy0-y0-f, dx);
    while (g >= 0) { xend++; g -= dy; }
    }
  else
    {
    edge->right = false;
    g = fixmul(x0-fx0, dy) + fixmul(fy0-y0-f, dx);
    while (g > 0) { x--; g -= dy; }
    }
  edge->x = x; edge->xend = xend; edge->g = g;
  }
return edge;
}  /* end of BuildEdge */

private ScanList AllocScanList()
{
if (freeScan >= endScanArray) LimitCheck();
return freeScan++;
}  /* end of AllocScanList */

private procedure InsertEdge(edge) Edge edge;
{
register int x, y, sy;
register ScanList s, sl;
register Edge e, ep;
if (edge == NIL) return;
y = edge->y0;
s = scn; edge->next = NIL;
if (s == NIL)
  { /* first time */
  scn = scanList = s = AllocScanList();
  s->y = y; s->edge = edge;
  s->next = s->prev = NIL;
  return;
  }
/* find correct place according to y */
sy = s->y;
if (sy != y)
  {
  if (sy > y)
    { /* search down list from s */
    while (true)
      {
      if ((sl = s->prev) == NIL || (sy = sl->y) < y) break;
      s = sl;
      if (sy == y) goto EqualY;
      }
    sl = AllocScanList();
    sl->prev = s->prev; sl->next = s; s->prev = sl;
    if (sl->prev == NIL) scanList = sl; else sl->prev->next = sl;
    }
  else
    { /* search up list from s */
    while (true)
      {
      if ((sl = s->next) == NIL || (sy = sl->y) > y) break;
      s = sl;
      if (sy == y) goto EqualY;
      }
    sl = AllocScanList();
    sl->prev = s;  sl->next = s->next;  s->next = sl;
    if (sl->next != NIL) sl->next->prev = sl;
    }
  sl->y = y; sl->edge = edge;
  scn = sl;
  return;
  }
/* insert into edge list for existing scanlist */
EqualY:
scn = s; e = s->edge; ep = NIL; x = edge->x;
while (true)
  {
  if (e == NIL || e->x >= x)
    { /* insert edge in front of e */
    edge->next = e;
    if (ep == NIL) scn->edge = edge;
    else ep->next = edge;
    break;
    }
  ep = e; e = e->next;
  }
}  /* end of InsertEdge */ 


public procedure QNewPoint(c) Cd c;
{
Fixed px1, py1;
px1 = pflttofix(&c.x);
py1 = pflttofix(&c.y);
if (noCurPt) {pxNP = px1; pyNP = py1; noCurPt = false;}
else InsertEdge(BuildEdge(px0, py0, px1, py1));
px0 = px1; py0 = py1;
} /* end of QNewPoint */

public procedure QFNewPoint(c) FCd c;
{
if (noCurPt) {pxNP = c.x; pyNP = c.y; noCurPt = false;}
else InsertEdge(BuildEdge(px0, py0, c.x, c.y));
px0 = c.x; py0 = c.y;
} /* end of QNewPoint */

public procedure QResetReducer()
{
scanList = scn = NIL;
noCurPt = true;
freeEdge = edgeArray;
freeScan = scanArray;
InitFontFlat(QFNewPoint);
}  /* end of QResetReducer */

public procedure QPFNewPoint(c) PFCd c; {QFNewPoint(*c);}

public procedure QRdcClose()
{
if (!noCurPt) InsertEdge(BuildEdge(px0, py0, pxNP, pyNP));
noCurPt = true;
}  /* end of QRdcClose */

public procedure IniQReducer(b1, b2, b3) PGrowableBuffer b1, b2, b3; {
  edgeArray = (Edge)b1->ptr;
  endEdgeArray = edgeArray + b1->len/sizeof(EdgeRec);
  buffData = (Int16 *)b2->ptr;
  endBuffData = buffData + b2->len/sizeof(Int16);
  scanArray = (ScanList)b3->ptr;
  endScanArray = scanArray + b3->len/sizeof(ScanRec);
  }	/* end of IniQReducer		   */
