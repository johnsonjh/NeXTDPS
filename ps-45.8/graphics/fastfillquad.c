/*
  fastfillquad.c

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
Ed Taft: Thu Jul 28 13:25:29 1988
Bill Paxton: Tue Oct 25 13:37:44 1988
Ivor Durham: Fri Aug 26 11:11:37 1988
Jim Sandman: Wed Apr  5 15:51:57 1989
Perry Caro: Wed Mar 29 11:20:55 1989
Joe Pasqua: Wed Dec 14 15:55:34 1988
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
#include "path.h"
#include "stroke.h"


#define Bubble(qcp1,qcp2) if (qcp1->c.y > qcp2->c.y) \
  {t = (integer)qcp2; qcp2 = qcp1; qcp1 = (PQdCorner)t;}

#define CanSkipTrap() \
  !needOut && FTrunc(yt)==FTrunc(yb) && \
  (xbl == xbr || FracPart(yb) != 0) && \
  FTrunc(xtl) < FTrunc(xbr) && \
  FTrunc(xbl) < FTrunc(xtr)

#define QuadTrap(qcp1,qcp2) { \
  xtr = qcp1->c.x; \
  qcptr = (qcp2->ptr1 == qcp1) ? qcp2->ptr2 : qcp2->ptr1; \
  if (FTrunc(qcp2->c.x) == FTrunc(qcptr->c.x)) xtl = qcp2->c.x; \
  else xtl = qcp2->c.x \
   + fxfrmul(qcptr->c.x - qcp2->c.x, \
        fixratio(yt - qcp2->c.y, qcptr->c.y - qcp2->c.y)); \
  if (xtr < xtl) {tfixed = xtr; xtr = xtl; xtl = tfixed;} \
  if (CanSkipTrap()) needOut = true; \
  else { \
    (*StrkTrp)(yt, yb, xtl, xtr, xbl, xbr); \
    needOut = false; \
    } \
  yb = yt;  xbl = xtl;  xbr = xtr; \
  }

extern QdCorner *qc;
extern procedure (*StrkTrp)();

public procedure FastFillQuad(dc0, dc1, dc2, dc3) DevCd dc0, dc1, dc2, dc3; {
/* this mini-reducer is for use only on convex quadrilaterals completely
     contained in the clipping region */
PQdCorner qcptr;
register PQdCorner pq0, pq1, pq2, pq3;
Fixed yt, yb;
boolean needOut;

qc[0].c = dc0; qc[1].c = dc1; qc[2].c = dc2; qc[3].c = dc3;

/* bubble sort pointers to qc[0 - 3] so that lowest (y,x) comes first. */
pq0 = &qc[0]; pq1 = &qc[1]; pq2 = &qc[2]; pq3 = &qc[3];
  { register integer t;
  Bubble(pq0,pq1)
  Bubble(pq1,pq2)
  Bubble(pq2,pq3)
  Bubble(pq1,pq2)
  Bubble(pq0,pq1)
  Bubble(pq1,pq2)
  }

if (FTrunc(pq0->c.y) == FTrunc(pq3->c.y)) { /* one liner */
  register Fixed f, xl, xr;
  xl = xr = pq0->c.x;
  f = pq1->c.x; if (f < xl) xl = f; if (f > xr) xr = f;
  f = pq2->c.x; if (f < xl) xl = f; if (f > xr) xr = f;
  f = pq3->c.x; if (f < xl) xl = f; if (f > xr) xr = f;
  (*StrkTrp)(pq3->c.y, pq0->c.y, xl, xr, xl, xr);
  return;
  }

if (pq1->c.y == pq0->c.y && pq3->c.y == pq2->c.y) {
  /* horizontal top and bottom */
  register Fixed xbl, xbr, xtl, xtr, tfixed;
  xbl = pq0->c.x; xbr = pq1->c.x;
  if (xbl > xbr) { tfixed = xbl; xbl = xbr; xbr = tfixed; }
  xtl = pq2->c.x; xtr = pq3->c.x;
  if (xtl > xtr) { tfixed = xtl; xtl = xtr; xtr = tfixed; }
  (*StrkTrp)(pq3->c.y, pq0->c.y, xtl, xtr, xbl, xbr);
  return;
  }

if (pq1->c.y - pq0->c.y == pq3->c.y - pq2->c.y &&
    pq1->c.x - pq0->c.x == pq3->c.x - pq2->c.x) { /* parallelogram */
  register Fixed xbl, xbr, xtl, xtr, tfixed;
  needOut = false;
  xbl = pq0->c.x +
    fxfrmul(pq2->c.x - pq0->c.x,
      fixratio(pq1->c.y - pq0->c.y, pq2->c.y - pq0->c.y));
  xbr = pq1->c.x;
  if (xbl > xbr) {
    tfixed = xbl; xbl = xbr; xbr = tfixed;
    xtr = pq2->c.x; xtl = xtr - (xbr - xbl);
    }
  else { xtl = pq2->c.x; xtr = xtl + (xbr - xbl); }
  if (FTrunc(pq1->c.y) == FTrunc(pq0->c.y)) needOut = true;
  else
    (*StrkTrp)(pq1->c.y, pq0->c.y, xbl, xbr, pq0->c.x, pq0->c.x);
  if (!needOut && FTrunc(pq2->c.y) == FTrunc(pq1->c.y) &&
      FracPart(pq1->c.y) != 0 &&
      FTrunc(xtl) < FTrunc(xbr) && FTrunc(xbl) < FTrunc(xtr))
    needOut = true;
  else {
    needOut = false;
    (*StrkTrp)(pq2->c.y, pq1->c.y, xtl, xtr, xbl, xbr); 
    }
  if (!needOut && FTrunc(pq3->c.y) == FTrunc(pq2->c.y) &&
      FracPart(pq2->c.y) != 0) {}
  else
    (*StrkTrp)(pq3->c.y, pq2->c.y, pq3->c.x, pq3->c.x, xtl, xtr);
  return;
  }

 { register Fixed xbl, xbr, xtl, xtr, tfixed;
  yb = pq0->c.y;  xbl = xbr = pq0->c.x;  yt = pq1->c.y;
  needOut = false;
  if (yt == yb)  /* horizontal bottom -- no initial trapezoid */
    {
    xbr = pq1->c.x;
    if (xbr < xbl) {tfixed = xbr; xbr = xbl; xbl = tfixed;}
    needOut = true;
    }
  else  QuadTrap(pq1,pq0)
  yt = pq2->c.y;
  if (yt != yb)  QuadTrap(pq2,pq3)
  else
    {
    tfixed = pq2->c.x;
    if (xbl > tfixed) {xbl = tfixed; needOut = true;}
    else if (xbr < tfixed) {xbr = tfixed; needOut = true;}
    }
  yt = pq3->c.y;
  xtr = xtl = pq3->c.x;
  if (CanSkipTrap()) return;
  (*StrkTrp)(yt, yb, xtl, xtr, xbl, xbr);
  }}  /* end of FastFillQuad */


