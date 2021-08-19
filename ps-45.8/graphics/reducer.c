/*
  reducer.c

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

Original version: Doug Brotz, April 11, 1983
Edit History:
Doug Brotz: Wed Nov 19 21:45:27 1986
Chuck Geschke: Thu Oct 10 07:04:43 1985
Ed Taft: Sat Jul 11 11:32:11 1987
John Gaffney: Wed Jan 23 14:41:43 1985
Bill Paxton: Sat Mar 12 11:17:43 1988
Ivor Durham: Sun Aug 14 12:24:12 1988
Jim Sandman: Wed Dec 13 09:43:26 1989
Paul Rovner: Thursday, October 8, 1987 9:36:21 PM
Joe Pasqua: Tue Oct 27 17:16:18 1987
Bill Bilodeau: Fri Oct 13 11:20:35 PDT 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include COPYRIGHT
#include BASICTYPES
#include DEVICE
#include ERROR
#include FP
#include GRAPHICS
#include PSLIB

#include "graphdata.h"
#include "reducer.h"
#include "graphicspriv.h"

public GrowableBuffer gbuf[4];

#ifdef XA
typedef Card32 PtOffset;
typedef Card32 LnOffset;
typedef Card32 RgOffset;
typedef Card32 PqOffset;
#else XA
typedef Card16 PtOffset;
typedef Card16 LnOffset;
typedef Card16 RgOffset;
typedef Card16 PqOffset;
#endif XA

typedef struct
  {
  Int32 whole;
  Card32 num;
  Card32 den;
  } Rational;

typedef struct
  {
  short int sign;
  Card16 place[4]; /* low order bits in place[0] */
  } FourPlace;

  typedef struct
    {
    Rational x;
    Rational y;
    cardinal serial;
    LnOffset line1;
    LnOffset line2;
    }
Point, *PPoint;

  typedef struct
    {
    PtOffset pt1;   /* pt from which line now emanates */
    PtOffset pt2;   /* pt at which line now terminates */
    PtOffset pthi;  /* high pt of line from which this line came */
    PtOffset ptlow; /* low pt of line from which this line came */
    RgOffset reglf;
    RgOffset regrt;
    boolean isUpLine:8;
    boolean isClipLine:8;
    }
Line, *PLine;

  typedef struct
    {
    union {LnOffset lnlf; RgOffset free;} u;
    LnOffset lnrt;
    Fixed yb, xbl, xbr;  /* values for current bottom of trap. */
    short int clipW;     /* clip region winding number */
    short int figW;      /* figure region winding number */
    boolean output:8;    /* if true, current bottom of trap has been output */
    }
Region, *PRegion;

  typedef struct
    {
    PtOffset pt;
    PqOffset next;
    PqOffset prev;
    boolean intersect:8;
    }
PointQ, *PPointQ;


#ifdef XA

#ifndef maxPoints
#define maxPoints 3000
#endif maxPoints

#ifndef maxLines
#define maxLines 3000
#endif maxLines

#ifndef maxPntQ
#define maxPntQ 3000
#endif maxPntQ

#ifndef maxRegions
#define maxRegions 400
#endif maxRegions

#else XA

#ifndef maxPoints
#define maxPoints 1500
#endif maxPoints

#ifndef maxLines
#define maxLines 1500
#endif maxLines

#ifndef maxPntQ
#define maxPntQ 1500
#endif maxPntQ

#ifndef maxRegions
#define maxRegions 200
#endif maxRegions

#endif XA

private PPoint points;
private PtOffset curPoint, firstPoint, lastPoint, endPoint,
  c_firstPoint, c_lastPoint;
private cardinal ptserial, c_ptserial;

private PLine lines;
private LnOffset curLine, endLine;

private PRegion regions;
private RgOffset rgFree, rgHead, exitRegion;
private integer rgsOut;

private PPointQ pointQs;
private PqOffset curPq, pqHead, pqRecent, endPntQ,
  c_pqHead, c_pqRecent;

#define PPt(i) ((PPoint)(((charptr)points) + (i)))
#define PLn(i) ((PLine)(((charptr)lines) + (i)))
#define PRg(i) ((PRegion)(((charptr)regions) + (i)))
#define PPq(i) ((PPointQ)(((charptr)pointQs) + (i)))


private boolean curIsClipLine, interiorClipMode, eoRule, trapShipped;
private Fixed outyt, outxl, outxr;
private integer yMax;
private integer clipxmin, clipxmax, c_xmin, c_xmax;
private Fixed fclpxmin, fclpxmax;

#ifdef DEBUG
private boolean debugOn;
#endif


private procedure LinkRegionFreeList()
{
RgOffset i;
PRegion rg;
rgFree = i = (maxRegions - 1) * sizeof(Region);
while (i != 0)  {rg = PRg(i);  i -= sizeof(Region);  rg->u.free = i;}
rgsOut = 0;
} /* end of LinkRegionFreeList */


private procedure InitReducer()
{
firstPoint = lastPoint = 0;
ptserial = 0;
curPoint = sizeof(Point);  /* PtOffset for points[1] */

curLine = sizeof(Line);  /* LnOffset for lines[1] */
curIsClipLine = false;

curPq = sizeof(PointQ);  /* PqOffset for pointQs[1] */
pqHead = pqRecent = 0;

if (rgsOut != 0)  LinkRegionFreeList();
rgHead = 0;
trapShipped = false;
}  /* end of InitReducer */

public procedure ResetReducer()		/* clear point buffer & initialize */
{
os_bzero((char *)points,curPoint);		/* clear the point buffer */
InitReducer();				/* now go init pointers/offsets */
yMax = 0;
clipxmin = MAXInt32;
clipxmax = 0;
} /* end of ResetReducer */


#ifdef DEBUG
private procedure psResetReducer()
{
ResetReducer();
} /* end of psResetReducer */
#endif


private short int Un4Comp(a, b)
  FourPlace *a, *b;
{/* returns -1 if |a| < |b|, 0 if |a| = |b|, 1 if |a| > |b| */
short int ans;
register Card16 *aplace, *bplace;
register short int i;
ans = 0;
aplace = &a->place[3];  bplace = &b->place[3];
for (i = 3; i >= 0; i--, aplace--, bplace--)
  {
  if (*aplace == *bplace) continue;
  ans = (*aplace < *bplace) ? -1 : 1;
  break;
  }
return ans;
} /* end of Un4Comp */


private procedure AddUn4(a, b, sum)  FourPlace *a, *b, *sum;
{
register short int i;
register Int32 t, carry;
register Card16 *aplace, *bplace, *sumplace;
carry = 0;
aplace = a->place;  bplace = b->place;  sumplace = sum->place;
for (i = 0; i < 4; i++, aplace++, bplace++, sumplace++)
  {
  t = (Int32)(*aplace) + (Int32)(*bplace) + carry;
  *sumplace = (Card16)t;
  carry = (t > 0177777);
  }
sum->sign = 1;
} /* end of AddUn4 */


private procedure SubUn4(a, b, diff)  FourPlace *a, *b, *diff;
{ /* return difference of positive a - positive b, with a >= b */
register short int i;
register Int32 t, borrow;
register Card16 *aplace, *bplace, *diffplace;
borrow = 0;
aplace = a->place;  bplace = b->place;  diffplace = diff->place;
for (i = 0; i < 4; i++, aplace++, bplace++, diffplace++)
  {
  t = (Int32)(*aplace) - (Int32)(*bplace) - borrow;
  *diffplace = (Card16)t;
  borrow = (t < 0);
  }
diff->sign = 1;
}  /* end of SubUn4 */


private procedure Add4(a, b, sum)  FourPlace *a, *b, *sum;
{
FourPlace *lesser, *greater;
if (a->sign == b->sign)
  {AddUn4(a, b, sum); sum->sign = a->sign; return;}
switch (Un4Comp(a, b))
  {
  case -1: lesser = a; greater = b; break;
  case 0:
    sum->sign = 1;
    sum->place[0] = sum->place[1] = sum->place[2] = sum->place[3] = 0;
    return;
  case 1: lesser = b; greater = a; break;
  }
SubUn4(greater, lesser, sum);
if ((a->sign < 0 && greater == a) || (a->sign > 0 && lesser == a))
  sum->sign = -1;
} /* end of Add4 */


private procedure Sub4(a, b, diff)  FourPlace *a, *b, *diff;
{
FourPlace *lesser, *greater;
if (a->sign != b->sign)
  {AddUn4(a, b, diff); diff->sign = a->sign; return;}
switch (Un4Comp(a, b))
  {
  case -1: lesser = a; greater = b; break;
  case 0:
    diff->sign = 1;
    diff->place[0] = diff->place[1] = diff->place[2] = diff->place[3] = 0;
    return;
  case 1: lesser = b; greater = a; break;
  }
SubUn4(greater, lesser, diff);
if ((a->sign < 0 && greater == a) || (a->sign > 0 && lesser == a))
  diff->sign = -1;
return;
} /* end of Sub4 */


private procedure MulUn22(a, b, prod) Card32 a, b; FourPlace *prod;
{
register Card16 *prodplace;
register Card32 tp, ts;
register Card16 t16;
register Card16 a0, a1, b0, b1;
prodplace = prod->place;
*(prodplace++) = 0;  *(prodplace++) = 0;  *(prodplace++) = 0; *prodplace = 0;
prod->sign = 1;
if (a == 0 || b == 0) return;
a0 = a;  a1 = a >> 16;
b0 = b;  b1 = b >> 16;

tp = (Card32)a0;
tp *= (Card32)b0;
prodplace = prod->place;
*(prodplace++) = tp;
*prodplace = tp >> 16;

tp = (Card32)a1;
tp *= (Card32)b0;
t16 = tp;
ts = (Card32)(*prodplace) + (Card32)t16;
*prodplace = ts;
*(prodplace+1) = (Card16)(tp >> 16);
*(prodplace+1) += (Card16)(ts >> 16);
if (b1 == 0) return;

tp = (Card32)a0;
tp *= (Card32)b1;
t16 = tp;
ts = (Card32)(*prodplace) + (Card32)t16;
*(prodplace++) = ts;
*prodplace += (Card16)(tp >> 16);
*prodplace += (Card16)(ts >> 16);

tp = (Card32)a1;
tp *= (Card32)b1;
t16 = tp;
ts = (Card32)(*prodplace) + (Card32)t16;
*(prodplace++) = ts;
*prodplace = (Card16)(tp >> 16);
*prodplace += (Card16)(ts >> 16);
} /* end of MulUn22 */


private procedure Mul2By2(a, b, prod)  Int32 a, b; FourPlace *prod;
{/* a and b are both 32 bit integers, with a having up to 31 significant
     bits, and b having up to 31 significant bits.  Returns the answer
     of a * b as a FourPlace. */
MulUn22((Card32)((a < 0) ? -a : a), (Card32)((b < 0) ? -b : b), prod);
if (a != 0 && b != 0 && (a < 0) != (b < 0)) prod->sign = -1;
} /* end of Mul2By2 */


private procedure Div3By2(a, b, r)  FourPlace *a; Int32 b; Rational *r;
{ /* This routine is only used for a final division calculation to obtain
     the coordinates of an intersection point, which, to be valid, must lie
     within a 16 bit range.  While the numerator may have up to 48 bits and
     the denominator may have up to 32 bits, quotients of more than 16 bits
     are out of range for the intersection calculation.  Therefore, this
     function reports "out of range" by returning a Rational with den = 0
     for results that must be out of range.  A returned nonzero den
     indicates an exact rational answer. */
Card16 b0, b1;
Card32 babs, quot, a32, apl2, apl1;
Int8 bits8;	/* Actually "char": beware machines without signed "char"s */
FourPlace rem, prod;
int abits, bbits, scaleshift;
short int asign;
if (b == 0) UndefResult();
babs = (b > 0) ? b : -b;
b0 = babs;
b1 = babs >> 16;
/* determine minimum significant bits for quotient by determining the
   significant bits for both a and b. */
bits8 = a->place[2] >> 8;  if (bits8 != 0) {abits = 48; goto ShiftA;}
bits8 = a->place[2];       if (bits8 != 0) {abits = 40; goto ShiftA;}
bits8 = a->place[1] >> 8;  if (bits8 != 0) {abits = 32; goto ShiftA;}
bits8 = a->place[1];       if (bits8 != 0) {abits = 24; goto ShiftA;}
bits8 = a->place[0] >> 8;  if (bits8 != 0) {abits = 16; goto ShiftA;}
bits8 = a->place[0];       if (bits8 != 0) {abits =  8; goto ShiftA;}
r->whole = r->num = 0;  r->den = 1;  return;
ShiftA:  while ((bits8 & 0x80) == 0) {bits8 <<= 1; abits--;}
bits8 = b1 >> 8;  if (bits8 != 0) {bbits = 32; goto ShiftB;}
bits8 = b1;       if (bits8 != 0) {bbits = 24; goto ShiftB;}
bits8 = b0 >> 8;  if (bits8 != 0) {bbits = 16; goto ShiftB;}
bits8 = b0;       bbits = 8;
ShiftB:  while ((bits8 & 0x80) == 0) {bits8 <<= 1; bbits--;}
if (abits - bbits >= 16) {r->den = 0; return;}
asign = a->sign;
if (bbits <= 16)
  {/* b is at most 16 bits; a is at most 31 bits. Do division directly. */
  a32 = ((Card32)a->place[1] << 16) + a->place[0];
  r->num = a32 - (r->whole = a32 / babs) * babs;
  r->den = babs;
  goto Normalize;
  }

/* must normalize babs (left bit = 1) to reduce error in trial quotient. */
scaleshift = 32 - bbits;
a32 = ((Card32)a->place[2] << 16) + (Card32)(a->place[1]);
a32 <<= scaleshift;  apl2 = a32 & 0xFFFF0000;
a32 = ((Card32)a->place[1] << 16) + (Card32)(a->place[0]);
a32 <<= scaleshift;  apl1 = a32 >> 16;
b1 = babs >> (16 - scaleshift);

/* we know that the high 16 bits of the quotient must be 0, so we only
   need to get the last place of the quotient. */
a32 = apl2 + apl1;
quot = a32 / b1;
/* following test may be true at most twice */
a->sign = 1;
while (true)
  {
  MulUn22(babs, quot, &prod);
  Sub4(a, &prod, &rem);
  if (rem.sign >= 0) break;
  quot--;
  }
r->whole = quot;
r->num = ((Card32)rem.place[1] << 16) + (Card32)(rem.place[0]);
r->den = babs;
Normalize:
  if ((asign > 0) != (b > 0))
    {r->whole = -r->whole - 1; r->num = r->den - r->num;}
  while (r->num >= r->den) {r->whole++; r->num -= r->den;}
  if (r->num == 0) r->den = 1;
} /* end of Div3By2 */


public short int RatComp(a, b)
  Rational *a, *b;
{
FourPlace crossa, crossb;
if (a->whole != b->whole) return (a->whole < b->whole) ? -1 : 1;
/* whole parts are equal, compare cross products */
MulUn22(a->num, b->den, &crossa);
MulUn22(b->num, a->den, &crossb);
return Un4Comp(&crossa, &crossb);
} /* end of RatComp */


#ifdef DEBUG
private procedure PushRational(r)  Rational r;
{
PushInteger(r.whole);
PushInteger(r.num);
PushInteger(r.den);
} /* end of PushRational */
#endif

#ifdef DEBUG
private Rational PopRational()
{Rational r;
r.den = PopInteger();
r.num = PopInteger();
r.whole = PopInteger();
return r;
} /* end of PopRational */
#endif

#ifdef DEBUG
private procedure PushFourPlace(f)
FourPlace f;
{int i;
PushInteger(f.sign);
for (i = 3; i >= 0; i--) PushInteger(f.place[i]);
} /*end of PushFourPlace */
#endif

#ifdef DEBUG
private FourPlace PopFourPlace()
{FourPlace f; int i;
for (i = 0; i < 4; i++) f.place[i] = PopInteger();
f.sign = PopInteger();
return f;
} /* end of PopFourPlace */
#endif

#ifdef DEBUG
private procedure psCompRat()
{Rational a, b;
b = PopRational();
a = PopRational();
PushInteger(RatComp(&a, &b));
} /* end of psCompRat */
#endif

#ifdef DEBUG
private procedure psFPMult()
{
Card32 a, b;
FourPlace prod;
b = PopInteger();
a = PopInteger();
MulUn22(a, b, &prod);
PushFourPlace(prod);
} /* end of psFPMult */
#endif

#ifdef DEBUG
private procedure psDiv32()
{
Int32 divisor;
FourPlace dividend;
Rational quotient;
divisor = PopInteger();
dividend = PopFourPlace();
Div3By2(&dividend, divisor, &quotient);
PushRational(quotient);
} /* end of psDiv32 */
#endif

#ifdef DEBUG
private procedure DebugPrintMakePoint(pt)  PtOffset pt;
{
PPoint ppt;
ppt = PPt(pt);
os_printf("Make point: %1d (%1d+%1d/%1d, %1d+%1d/%1d)\n",      /* UNIX */
       ppt->serial, ppt->x.whole, ppt->x.num, ppt->x.den,
       ppt->y.whole, ppt->y.num, ppt->y.den);
fflush(os_stdout);                                                 /* UNIX */
} /* end of DebugPrintMakePoint */
#endif

#ifdef DEBUG
private procedure DebugPrintMakeLine(line)  LnOffset line;
{
PLine pline;
pline = PLn(line);
os_printf("Make line: %1d -> %1d, low: %1d, hi: %1d\n",            /* UNIX */
  PPt(pline->pt1)->serial, PPt(pline->pt2)->serial,
  PPt(pline->ptlow)->serial, PPt(pline->pthi)->serial);
fflush(os_stdout);                                                 /* UNIX */
} /* end of DebugPrintMakeLine */
#endif

#ifdef DEBUG
private procedure DebugPrintNewPoint(x, y)  integer x, y;
{
os_printf("New Point: (%1d, %1d)\n", x, y);
fflush(os_stdout);                                                 /* UNIX */
} /* end of DebugPrintNewPoint */
#endif

#ifdef DEBUG
private procedure DebugPrintRdcClose()
{
os_printf("Close Path\n");
fflush(os_stdout);                                                 /* UNIX */
} /* end of DebugPrintRdcClose */
#endif


private PtOffset MakePoint(x, y)  Rational *x, *y;
{
register PtOffset pt;
register PPoint ppt;
if (curPoint == endPoint) LimitCheck();
pt = curPoint;
curPoint += sizeof(Point);
ppt = PPt(pt);
ppt->x = *x;
ppt->y = *y;
ppt->serial = ptserial++;
ppt->line1 = ppt->line2 = 0;
#ifdef DEBUG
if (debugOn) DebugPrintMakePoint(pt);
#endif
return pt;
} /* end of MakePoint */


private boolean PtLT(pt1, pt2)  PtOffset pt1, pt2;
{ /* Returns true iff pt1 occurs earlier in the y,x ordering */
register PPoint ppt1, ppt2;
register Int32 w1, w2;
ppt1 = PPt(pt1);  ppt2 = PPt(pt2);
if ((w1=ppt1->y.whole) != (w2=ppt2->y.whole))
  return (boolean)(w1 < w2);
switch (RatComp(&ppt1->y, &ppt2->y))
  {
  case -1: return true;
  case  0:
    if ((w1=ppt1->x.whole) != (w2=ppt2->x.whole))
      return (boolean)(w1 < w2);
    switch (RatComp(&ppt1->x, &ppt2->x))
      {
      case -1: return true;
      case  0: return (boolean)(ppt1->serial < ppt2->serial);
      case  1: return false;
      }
  case  1: return false;
  } /*NOTREACHED*/
} /* end of PtLT */


private procedure MakeLine(pt1, pt2, oldln1, oldln2)
  PtOffset pt1, pt2; LnOffset oldln1, oldln2;
{
PLine pline, poldline;
LnOffset line, oldline;
PPoint ppt1, ppt2;
if (curLine == endLine) LimitCheck();
line = curLine;
pline = PLn(line);
curLine += sizeof(Line);
pline->pt1 = pt1;
pline->pt2 = pt2;
oldline = (oldln1 == 0) ? oldln2 : oldln1;
if (oldline == NIL)
  {
  pline->isClipLine = curIsClipLine;
  if (pline->isUpLine = PtLT(pt1, pt2))
    {pline->ptlow = pt1; pline->pthi = pt2;}
  else {pline->ptlow = pt2; pline->pthi = pt1;}
  }
else
  {
  poldline = PLn(oldline);
  pline->isClipLine = poldline->isClipLine;
  pline->isUpLine = poldline->isUpLine;
  pline->ptlow = poldline->ptlow;
  pline->pthi = poldline->pthi;
  }
ppt1 = PPt(pt1);  ppt2 = PPt(pt2);
if (ppt1->line1 == oldln1) ppt1->line1 = line; else ppt1->line2 = line;
if (ppt2->line1 == oldln2) ppt2->line1 = line; else ppt2->line2 = line;
pline->reglf = pline->regrt = 0;
#ifdef DEBUG
if (debugOn) DebugPrintMakeLine(line);
#endif
} /* end of MakeLine */


private RgOffset MakeRegion(linelf, linert)  LnOffset linelf, linert;
{
RgOffset region;
PRegion pregion;
if (rgFree == 0) LimitCheck();
rgsOut++;
region = rgFree;
rgFree = PRg(rgFree)->u.free;
pregion = PRg(region);
pregion->u.lnlf = linelf;
pregion->lnrt = linert;
if (linelf != 0) PLn(linelf)->regrt = region;
if (linert != 0) PLn(linert)->reglf = region;
pregion->clipW = pregion->figW = 0;
pregion->output = false;
return region;
} /* end of MakeRegion */


private procedure FreeRegion(region)  RgOffset region;
{
PRg(region)->u.free = rgFree;
rgFree = region;
rgsOut--;
} /* end of FreeRegion */


private procedure PQInsert(pt, intersect)  PtOffset pt; boolean intersect;
{
register PPointQ ppq, ppqRecent;
register PqOffset pq, xp;
if (curPq == endPntQ) LimitCheck();
pq = curPq;
ppq = PPq(pq);
curPq += sizeof(PointQ);
ppq->pt = pt;
ppq->intersect = intersect;
if (pqHead == 0) {ppq->next = ppq->prev = 0; pqHead = pq;}
else
  {
  if (pqRecent == 0) pqRecent = pqHead;
  ppqRecent = PPq(pqRecent);
  if (PtLT(pt, ppqRecent->pt))
    { /* search backwards */
    while ((xp=ppqRecent->prev) != 0 && PtLT(pt, PPq(xp)->pt))
      {pqRecent = xp;  ppqRecent = PPq(xp);}
    ppq->next = pqRecent;  ppq->prev = ppqRecent->prev;  ppqRecent->prev = pq;
    if (ppq->prev == 0) pqHead = pq;  else PPq(ppq->prev)->next = pq;
    }
  else
    { /* search forwards */
    while ((xp=ppqRecent->next) != 0 && PtLT(PPq(xp)->pt, pt))
      {pqRecent = xp;  ppqRecent = PPq(xp);}
    ppq->prev = pqRecent;  ppq->next = ppqRecent->next;  ppqRecent->next = pq;
    if (ppq->next != 0) PPq(ppq->next)->prev = pq;
    }
  }
pqRecent = pq;
}  /* end of PQInsert */


private PqOffset PopPQ()
{
register PqOffset pq, pqh;
pq = pqHead;
if (pq == pqRecent) pqRecent = 0;
if (pq != 0) {
  pqh = PPq(pq)->next;
  pqHead = pqh;
  if (pqh != 0) PPq(pqh)->prev = 0;
  }
return pq;
} /* end of PopPQ */


private Fixed RatToFixed(x)  register Rational *x;
{return (Fixed)((x->whole << 16) + ufixratio(x->num, x->den));}


public procedure NewPoint(xi, yi)  integer xi, yi;
{
PtOffset pt;
Rational x, y;
x.whole = xi;
y.whole = yi;
if (curIsClipLine)
  {
  if (xi < clipxmin) clipxmin = xi;
  if (xi > clipxmax) clipxmax = xi;
  }
else if (yi > yMax) yMax = yi;
x.num = y.num = 0;  x.den = y.den = 1;
#ifdef DEBUG
if (debugOn) DebugPrintNewPoint(x.whole, y.whole);
#endif
if (lastPoint != 0 && PPt(lastPoint)->x.whole == x.whole
    && PPt(lastPoint)->y.whole == y.whole) return;
pt = MakePoint(&x, &y);
if (lastPoint != 0)
  {MakeLine(lastPoint, pt, 0, 0);  PQInsert(lastPoint, false);}
else firstPoint = pt;
lastPoint = pt;
} /* end of NewPoint */

#ifdef DEBUG
private procedure psNewPoint()
{
Cd c;
PopPCd(&c);
RRoundP(&c.x, &c.x);  RRoundP(&c.y, &c.y);
NewPoint(c.x, c.y);
} /* end of psNewPoint */
#endif


public procedure RdcClose()
{
PPoint plastpt, pfirstpt;
if (lastPoint == 0) return;
#ifdef DEBUG
if (debugOn) DebugPrintRdcClose();
#endif
plastpt = PPt(lastPoint);  pfirstpt = PPt(firstPoint);
if (firstPoint == lastPoint)
  {
  lastPoint = MakePoint(&pfirstpt->x, &pfirstpt->y);
  MakeLine(lastPoint, firstPoint, 0, 0);
  MakeLine(firstPoint, lastPoint, 0, 0);
  PQInsert(firstPoint, false);
  PQInsert(lastPoint, false);
  }
else
  {
  if (pfirstpt->x.whole == plastpt->x.whole
      && pfirstpt->y.whole == plastpt->y.whole)
    {lastPoint = PLn(plastpt->line1)->pt1;  PPt(lastPoint)->line2 = 0;}
  else PQInsert(lastPoint, false);
  MakeLine(lastPoint, firstPoint, 0, 0);
  }
lastPoint = firstPoint = 0;
} /* end of RdcClose */

#ifdef DEBUG
private procedure psRdcClose()  {RdcClose();}
#endif

#ifdef DEBUG
private procedure DebugPrintRegions()
{
PRegion prg;
prg = PRg(rgHead);
os_printf("Regions: ");
fflush(os_stdout);                                                   /* UNIX */
os_printf("  CW: %1d FW: %1d\n", prg->clipW, prg->figW);             /* UNIX */
fflush(os_stdout);                                                   /* UNIX */
while (prg->lnrt != 0)
  {
  os_printf(" %1d -> %1d ",                                          /* UNIX */
    PPt(PLn(prg->lnrt)->pt1)->serial, PPt(PLn(prg->lnrt)->pt2)->serial);
  fflush(os_stdout);                                                 /* UNIX */
  prg = PRg(PLn(prg->lnrt)->regrt);
  os_printf("CW: %1d FW: %1d B: %f L: %f R: %f\n",                   /* UNIX */
          prg->clipW, prg->figW, fixtodbl(prg->yb),
          fixtodbl(prg->xbl), fixtodbl(prg->xbr));
  fflush(os_stdout);                                                 /* UNIX */
  };
fflush(os_stdout);                                                   /* UNIX */
} /* end of DebugPrintRegions */
#endif

#ifdef DEBUG
private procedure DebugPrintIntersect(lnlf, lnrt, x, y)
  LnOffset lnlf, lnrt; Rational x, y;
{
PLine plnlf, plnrt;
plnlf = PLn(lnlf);  plnrt = PLn(lnrt);
os_printf                                                           /* UNIX */
 ("Intersect %1d -> %1d & %1d -> %1d at (%1d+%1d/%1d, %1d+%1d/%1d)\n",
   PPt(plnlf->pt1)->serial, PPt(plnlf->pt2)->serial,
   PPt(plnrt->pt1)->serial, PPt(plnrt->pt2)->serial,
   x.whole, x.num, x.den, y.whole, y.num, y.den);
fflush(os_stdout);                                                   /* UNIX */
} /* end of DebugPrintIntersect */
#endif

#ifdef DEBUG
private procedure DebugPrintEvent(s, pt)  string s; PtOffset pt;
{
PPoint ppt;
PLine pline1, pline2;
ppt = PPt(pt);
pline1 = PLn(ppt->line1);  pline2 = PLn(ppt->line2);
os_printf("%4sevent: ", s);
if (pline1->pt2 == pt)
  os_printf("%1d ->", PPt(pline1->pt1)->serial);
else os_printf("%1d <-", PPt(pline1->pt2)->serial);
os_printf(" %1d ", ppt->serial);
if (pline2->pt1 == pt)
  os_printf("-> %1d\n", PPt(pline2->pt2)->serial);
else os_printf("<- %1d\n", PPt(pline2->pt1)->serial);
fflush(os_stdout);                                                   /* UNIX */
} /* end of DebugPrintEvent */
#endif


public Rational XatY(line, y, chooseLeft)
  LnOffset line; integer y; boolean chooseLeft;
{
Rational r;
register Int32 num, lowX, hiX, lowY, hiY;
register PLine pline;
register PPoint pptlow, ppthi;
pline = PLn(line);  pptlow = PPt(pline->ptlow);  ppthi = PPt(pline->pthi);
lowX = pptlow->x.whole;
lowY = pptlow->y.whole;
hiX = ppthi->x.whole;
hiY = ppthi->y.whole;

if (lowY == hiY) return
  ((pline->isUpLine == chooseLeft) ? PPt(pline->pt1)->x : PPt(pline->pt2)->x);

num = (y - lowY) * (hiX - lowX);
r.den = hiY - lowY;
r.num = (num > 0) ? num : -num;
r.whole = r.num / r.den;
r.num = r.num - r.whole * r.den;
if (num < 0) {r.whole = -r.whole - 1; r.num = r.den - r.num;}
if (r.num == r.den) {r.whole++; r.num = 0;}
if (r.num == 0) r.den = 1;
r.whole += lowX;
return r;
} /* end of XatY */


private Fixed FixedXatY(line, y, chooseLeft)
  LnOffset line; Fixed y; boolean chooseLeft;
{
register Int32 lowX, hiX, lowY, hiY;
PLine pline;
PPoint pptlow, ppthi;
pline = PLn(line);  pptlow = PPt(pline->ptlow);  ppthi = PPt(pline->pthi);
lowX = pptlow->x.whole;
lowY = pptlow->y.whole;
hiX = ppthi->x.whole;
hiY = ppthi->y.whole;

if (lowY == hiY) return RatToFixed(
 (pline->isUpLine == chooseLeft) ? &PPt(pline->pt1)->x : &PPt(pline->pt2)->x);

return FixInt(lowX) + fixmul(y - FixInt(lowY), fixdiv(hiX - lowX, hiY -lowY));
} /* end of FixedXatY */


private Rational YatX(lowX, dx, lowY, dy, x)
  Int32 lowX, dx, lowY, dy, x;
{ /* NOT GENERAL PURPOSE: assumes dx != 0 */
Rational r;
Int32 num;

num = (x - lowX) * dy;
r.den = (dx > 0) ? dx : -dx;
r.num = (num >= 0) ? num : -num;
r.whole = r.num / r.den;
r.num = r.num - r.whole * r.den;
if (num != 0 && (num < 0) != (dx < 0))
  {r.whole = -r.whole - 1; r.num = r.den - r.num;}
if (r.num == r.den) {r.whole++; r.num = 0;}
if (r.num == 0) r.den = 1;
r.whole += lowY;
return r;
} /* end of YatX */


private short int PointOnLine(x, y, line)  Rational *x, *y; LnOffset line;
{/* Determines if (x, y) is contained within the closed-open line segment,
    line, which is closed at its low end and open at its upper end.
    The value returned indicates:
    0: (x, y) is not in the close-open line segment.
    1: (x, y) is the low point of the line segment.
    2: (x, y) is contained in the open-open line segment.
  */
register Rational *r, *lowr, *hir;
register PLine pline;
pline = PLn(line);
if (PPt(pline->ptlow)->y.whole == PPt(pline->pthi)->y.whole)
  {r = x;
  lowr = (pline->isUpLine) ? &PPt(pline->pt1)->x : &PPt(pline->pt2)->x;
  hir  = (pline->isUpLine) ? &PPt(pline->pt2)->x : &PPt(pline->pt1)->x;}
else {r = y;
     lowr = (pline->isUpLine) ? &PPt(pline->pt1)->y : &PPt(pline->pt2)->y;
     hir  = (pline->isUpLine) ? &PPt(pline->pt2)->y : &PPt(pline->pt1)->y;}
switch (RatComp(r, lowr))
  {
  case -1: return 0;
  case  0: return 1;
  case  1: return (RatComp(r, hir) == -1) ? 2 : 0;
  } /*NOTREACHED*/
} /* end of PointOnLine */


public short int Intersect(lf, rt, x, y)
  LnOffset lf, rt; Rational *x, *y;
{/* This function performs computations to determine if the lines lf and
    rt intersect each other.  lf is originally thought to be to the left
    of rt.  For a point to be considered on a line, it must lie in the
    closed-open line segment that is closed at the segment's lower endpoint
    and open at its upper endpoint.
    The return value indicates several cases.

    0: The line segments don't intersect.  The segments may be parallel,
       one or both might be degenerate, the point at which their extended
       lines intersect is not on both segments, or they intersect at one 
       segment's low points, but the segments are already in proper
       orientation to each other.
    1: The segments intersect at the low point of one of the segments, such
       that lf is actually to the right of rt.
    2: The segments intersect properly, with the point of intersection lying
       in the open-open portions of both segments.

    Only in cases 1 and 2 are the actual Rational coordinates of the
    point of intersection returned in x and y. */

Int32 lflowx, lflowy, lfhix, lfhiy, rtlowx, rtlowy, rthix, rthiy;
Int32 lfdx, lfdy, rtdx, rtdy;
Int32 rtdxlfdy, lfdxrtdy;
short int onlf, onrt;
register PLine plf, prt;
register PPoint pptlow, ppthi;
plf = PLn(lf);  prt = PLn(rt);
pptlow = PPt(plf->ptlow);  ppthi = PPt(plf->pthi);
lflowx = pptlow->x.whole;  lfhix  = ppthi->x.whole;
lflowy = pptlow->y.whole;  lfhiy  = ppthi->y.whole;
pptlow = PPt(prt->ptlow);  ppthi = PPt(prt->pthi);
rtlowx = pptlow->x.whole;  rthix  = ppthi->x.whole;
rtlowy = pptlow->y.whole;  rthiy  = ppthi->y.whole;
lfdx = lfhix - lflowx;
lfdy = lfhiy - lflowy;
rtdx = rthix - rtlowx;
rtdy = rthiy - rtlowy;
lfdxrtdy = lfdx * rtdy;
rtdxlfdy = rtdx * lfdy;
/* perform inverse slope cross-product test.  Both dy's are >= 0.
   The test says that either the two lines have equal slopes, one or both
   are degenerate, lf is left of rt at the time of the test and they
   point away from each other, or that they intersect at a low point and
   they are already in correct relation to each other. */
if (lfdxrtdy <= rtdxlfdy) return 0;
/* compute intersection pt and determine if it is on both lines. */
if (lfdx == 0)
  {*x = PPt(plf->ptlow)->x; *y = YatX(rtlowx, rtdx, rtlowy, rtdy, lflowx);}
else if (rtdx == 0)
  {*x = pptlow->x; *y = YatX(lflowx, lfdx, lflowy, lfdy, rtlowx);}
else if (lfdy == 0)
  {*y = PPt(plf->ptlow)->y; *x = XatY(rt, lflowy, false);}
else if (rtdy == 0)
  {*y = pptlow->y; *x = XatY(lf, rtlowy, false);}
else
  {
  FourPlace add1, mult1, mult2, mult3, sub1;
/*  
  *x = Div3By2(
         Add4(
           Mul2By2(lfdx * rtdx, lflowy - rtlowy),
           Sub4(Mul2By2(lfdxrtdy, rtlowx), Mul2By2(rtdxlfdy, lflowx))),
         lfdxrtdy - rtdxlfdy);
*/
  Mul2By2(lfdxrtdy, rtlowx, &mult2);
  Mul2By2(rtdxlfdy, lflowx, &mult3);
  Sub4(&mult2, &mult3, &sub1);
  Mul2By2(lfdx * rtdx, lflowy - rtlowy, &mult1);
  Add4(&mult1, &sub1, &add1);
  Div3By2(&add1, lfdxrtdy - rtdxlfdy, x);
  if (x->den == 0) return 0; /* intersection is out of bounds */
/*  *y = Div3By2(
         Add4(
           Mul2By2(lfdy * rtdy, lflowx - rtlowx),
           Sub4(Mul2By2(rtdxlfdy, rtlowy), Mul2By2(lfdxrtdy, lflowy))),
         rtdxlfdy - lfdxrtdy);
*/
  Mul2By2(rtdxlfdy, rtlowy, &mult2);
  Mul2By2(lfdxrtdy, lflowy, &mult3);
  Sub4(&mult2, &mult3, &sub1);
  Mul2By2(lfdy * rtdy, lflowx - rtlowx, &mult1);
  Add4(&mult1, &sub1, &add1);
  Div3By2(&add1, rtdxlfdy - lfdxrtdy, y);
  if (y->den == 0) return 0; /* intersection is out of bounds */
  };
/* Check if (x, y) is contained within both closed-open line segments. */
onlf = PointOnLine(x, y, lf);
if (onlf == 0) return 0;
onrt = PointOnLine(x, y, rt);
return ((onrt == 2) ? onlf : onrt);
}  /* end of Intersect */


private procedure ShipTrapezoid(callBack, region, yt, xtl, xtr, isExit)
  procedure (*callBack)(/* yt, yb, xtl, xtr, xbl, xbr */);
  RgOffset region; Fixed yt, xtl, xtr; boolean isExit;
{
PRegion prg;
prg = PRg(region);
#ifdef DEBUG
if (debugOn)
  {
  os_printf("Call Trapezoid: region: %1d -> %1d, %1d -> %1d\n",  /* UNIX */
       PPt(PLn(prg->u.lnlf)->pt1)->serial, PPt(PLn(prg->u.lnlf)->pt2)->serial,
       PPt(PLn(prg->lnrt)->pt1)->serial, PPt(PLn(prg->lnrt)->pt2)->serial);
  os_printf("    yt: %f xtl: %f xtr: %f", fixtodbl(yt), fixtodbl(xtl),
          fixtodbl(xtr));
  if (isExit) os_printf(" Exit");
  putchar('\n');
  }
#endif
if (eoRule && ((prg->figW & 01) == 0)) return;
if (interiorClipMode)
  {if (xtl < fclpxmin) xtl = fclpxmin;  if (xtr > fclpxmax) xtr = fclpxmax;}
if (interiorClipMode == (prg->clipW != 0))
/* if ((interiorClipMode && (prg->clipW != 0))
    || (!interiorClipMode && (prg->clipW == 0))) */
  {
  if ((yt != prg->yb)
       || ((!prg->output)
            && (isExit || (xtl > prg->xbl) || (xtr < prg->xbr))))
    {
#ifdef DEBUG
    if (debugOn)
      os_printf("Trapezoid: %f, %f, %f, %f, %f, %f\n",          /* UNIX */
               fixtodbl(yt), fixtodbl(prg->yb), fixtodbl(xtl), fixtodbl(xtr),
               fixtodbl(prg->xbl), fixtodbl(prg->xbr));
#endif
    if ((yt != prg->yb) || !trapShipped || (yt != outyt)
        || (MIN(xtl, prg->xbl) < outxl) || (MAX(xtr, prg->xbr) > outxr))
      {
      (*callBack)(yt, prg->yb, xtl, xtr, prg->xbl, prg->xbr);
      trapShipped = true;  outyt = yt;  outxl = xtl;  outxr = xtr;
      }
    prg->output = true;
    }
  else if ((xtl < prg->xbl) || (xtr > prg->xbr)) prg->output = false;
  }
else prg->output = false;
prg->yb = yt;
prg->xbl = xtl;
prg->xbr = xtr;
} /* end of ShipTrapezoid */


private procedure ShipLeftAnchoredTrap
    (callBack, region, y, lfx, isxt, line, chsLft)
  procedure (*callBack)(); RgOffset region; Fixed y, lfx; boolean isxt;
  LnOffset line; boolean chsLft;
{
Fixed rtx;
rtx = FixedXatY(line, y, chsLft);
if (rtx < lfx) rtx = lfx;
ShipTrapezoid(callBack, region, y, lfx, rtx, isxt);
} /* end of ShipLeftAnchoredTrap */


private procedure ShipRightAnchoredTrap
    (callBack, region, y, rtx, isxt, line, chsLft)
  procedure (*callBack)(); RgOffset region; Fixed y, rtx; boolean isxt;
  LnOffset line; boolean chsLft;
{
Fixed lfx;
lfx = FixedXatY(line, y, chsLft);
if (lfx > rtx) lfx = rtx;
ShipTrapezoid(callBack, region, y, lfx, rtx, isxt);
} /* end of ShipRightAnchoredTrap */


private procedure ShortenLine(line, x, y)  LnOffset line; Rational x, y;
{
PLine pline;
PtOffset newpt;
PPoint plowpt;
pline = PLn(line);
plowpt = PPt((pline->isUpLine) ? pline->pt1 : pline->pt2);
if (RatComp(&plowpt->x, &x) == 0
    && RatComp(&plowpt->y, &y) == 0) return;
newpt = MakePoint(&x, &y);
if (pline->isUpLine) pline->pt1 = newpt; else pline->pt2 = newpt;
}  /* end of ShortenLine */


private procedure Interchange(lf, rt, x, y, callBack)
  LnOffset lf, rt; Rational x, y; procedure (*callBack)();
{
RgOffset rglf, rgrt, rgmid;
PRegion prglf, prgrt, prgmid;
PLine plf, prt;
integer delta;
Fixed fixedx, fixedy, ftemp;
#ifdef DEBUG
if (debugOn) os_printf("Flip!\n");
#endif
plf = PLn(lf);  prt = PLn(rt);
rglf = plf->reglf;  prglf = PRg(rglf);
rgmid = plf->regrt;  prgmid = PRg(rgmid);
rgrt = prt->regrt;  prgrt = PRg(rgrt);
fixedx = RatToFixed(&x);
fixedy = RatToFixed(&y);
if (prgmid->figW != 0)
  ShipTrapezoid(callBack, rgmid, fixedy, fixedx, fixedx, true);
ShortenLine(lf, x, y);
ShortenLine(rt, x, y);
delta = (plf->isUpLine) ? 1 : -1;
if (plf->isClipLine) prgmid->clipW += delta;
else prgmid->figW += delta;
delta = (prt->isUpLine) ? -1 : 1;
if (prt->isClipLine) prgmid->clipW += delta;
else prgmid->figW += delta;
prgmid->output = false;
prgmid->yb = fixedy;
prgmid->xbl = prgmid->xbr = fixedx;
prglf->lnrt = prgmid->u.lnlf = rt;
prgrt->u.lnlf = prgmid->lnrt = lf;
plf->reglf = prt->regrt = rgmid;
plf->regrt = rgrt;
prt->reglf = rglf;
if (prglf->u.lnlf != 0 && prglf->figW != 0)
  {
  ftemp = FixedXatY(prglf->u.lnlf, fixedy, false);
  if (ftemp > fixedx) ftemp = fixedx;
  ShipTrapezoid(callBack, rglf, fixedy, ftemp, fixedx, false);
  }
if (prgrt->lnrt != 0 && prgrt->figW != 0)
  {
  ftemp = FixedXatY(prgrt->lnrt, fixedy, true);
  if (ftemp < fixedx) ftemp = fixedx;
  ShipTrapezoid(callBack, rgrt, fixedy, fixedx, ftemp, false);
  }
} /* end of Interchange */


private procedure CheckIntersection(lf, rt, callBack)
  LnOffset lf, rt; procedure (*callBack)();
{ /* lf and rt are two lines in the current region list, with
     lf to the left of rt.
     Lines that are joined to each other at a point do not intersect.
     Lines with equal slopes never intersect.  A line never intersects
     another at its high endpoint.
     If the lines intersect properly, (between, not including, endpoints)
     then create two new points at this intersection and two new lines,
     linking these points, old lines and new lines to form two bends or
     an entry and exit according to the directions of the original lines.
     If the lines intersect at either's low endpoint, check for which line
     is left of the other.  If they must switch positions, rearrange
     them in the region list, (no trapezoid output is necessary), and repeat
     recursively for the new right line with its new neighbor to the right
     and for the new left line with its new neighbor to the left. */
Rational x, y;
register PLine plf, prt;
if ((lf == 0) || (rt == 0)) return;
plf = PLn(lf);  prt = PLn(rt);
 { register PtOffset lf1, rt1, lf2, rt2;
   lf1 = plf->pt1; rt1 = prt->pt1;
   lf2 = plf->pt2; rt2 = prt->pt2;
   if (lf1 == rt1 || lf1 == rt2 || lf2 == rt1 || lf2 == rt2) return;
   }
 { register Int32 lflow, lfhi, rtlow, rthi;
   lflow = PPt(plf->ptlow)->x.whole; 
   lfhi = PPt(plf->pthi)->x.whole;
   rtlow = PPt(prt->ptlow)->x.whole;
   rthi = PPt(prt->pthi)->x.whole;
   if (MAX(lflow, lfhi) < MIN(rtlow, rthi)) return;
   }
switch (Intersect(lf, rt, &x, &y))
  {
  case 0: return; /* segments don't intersect after all */
  case 1: /* intersection at low point.  Flip the lines. */
#ifdef DEBUG
    if (debugOn) DebugPrintIntersect(lf, rt, x, y);
#endif
    Interchange(lf, rt, x, y, callBack);
    /* N.B. now lf is on the right, and rt is on the left! */
    CheckIntersection(PRg(prt->reglf)->u.lnlf, rt, callBack);
    CheckIntersection(lf, PRg(plf->regrt)->lnrt, callBack);
    return;
  }
/* Both segments are intersected properly. */
#ifdef DEBUG
if (debugOn) DebugPrintIntersect(lf, rt, x, y);
#endif
  {
register PtOffset pta, ptb;
register LnOffset LF, RT;
LF = lf; RT = rt;
pta = MakePoint(&x, &y);
ptb = MakePoint(&x, &y);
if (plf->isUpLine)
  {
  if (prt->isUpLine)
    { /* up-up case */
    MakeLine(ptb, prt->pt2, 0, RT);
    MakeLine(ptb, plf->pt2, 0, LF);
    plf->pt2 = prt->pt2 = pta;
    }
  else
    { /* up-down case */
    MakeLine(prt->pt1, ptb, RT, 0);
    MakeLine(ptb, plf->pt2, 0, LF);
    plf->pt2 = prt->pt1 = pta;
    }
  }
else if (prt->isUpLine)
  { /* down-up case */
  MakeLine(ptb, prt->pt2, 0, RT);
  MakeLine(plf->pt1, ptb, LF, 0);
  plf->pt1 = prt->pt2 = pta;
  }
else
  { /* down-down case */
  MakeLine(prt->pt1, ptb, RT, 0);
  MakeLine(plf->pt1, ptb, LF, 0);
  plf->pt1 = prt->pt1 = pta;
  };
PPt(pta)->line1 = LF;
PPt(pta)->line2 = RT;
PQInsert(pta, true);
PQInsert(ptb, true);
}
} /* end of CheckIntersection */


private procedure RdcEntry(pt, callBack, intersect)
  PtOffset pt; procedure (*callBack)(/* yt, yb, xtl, xtr, xbl, xbr */);
  boolean intersect;
{
register PPoint ppt;
RgOffset rgrt, rgmid;
register RgOffset rglf;
register PRegion prglf;
short int delta;
LnOffset lf, rt;
PLine plf, prt;
Fixed fxdptx, fxdpty, ftemplx, ftemprx;
Rational xaty;
#ifdef DEBUG
if (debugOn) DebugPrintEvent("Entry ", pt);
#endif
ppt = PPt(pt);
fxdptx = RatToFixed(&ppt->x);
fxdpty = RatToFixed(&ppt->y);
/* search for existing region in which this entry point falls */
if (intersect) {rglf = exitRegion;  prglf = PRg(rglf);}
else
  {
  rglf = rgHead;  prglf = PRg(rglf);
  while (prglf->lnrt != 0)
    {
    if (RatComp(&ppt->x, &PPt(PLn(prglf->lnrt)->pt1)->x) >= 0)
      {
      if (RatComp(&ppt->x, &PPt(PLn(prglf->lnrt)->pt2)->x) >= 0)
        {rglf = PLn(prglf->lnrt)->regrt;  prglf = PRg(rglf);  continue;}
      }
    else if (RatComp(&ppt->x, &PPt(PLn(prglf->lnrt)->pt2)->x) < 0) break;
    xaty = XatY(prglf->lnrt, ppt->y.whole, true);
    if (RatComp(&ppt->x, &xaty) < 0) break;
    rglf = PLn(prglf->lnrt)->regrt;  prglf = PRg(rglf);
    }
  }
/* output trapezoid of the split region */
if ((!intersect) && prglf->figW != 0)
  {
  ftemplx = FixedXatY(prglf->u.lnlf, fxdpty, true);
  ftemprx = FixedXatY(prglf->lnrt, fxdpty, false);
  if (ftemplx > ftemprx) ftemplx = ftemprx;
  ShipTrapezoid(callBack, rglf, fxdpty, ftemplx, ftemprx, false);
  }
/* split existing region into 2 regions, and insert a new region.
   check slopes of pt's lines to determine which is left and which is right.*/
  { register PLine pl1, pl2;
    pl1 = PLn(ppt->line1); pl2 = PLn(ppt->line2);
    if (  (PPt(pl1->pthi)->x.whole - PPt(pl1->ptlow)->x.whole)
         * (PPt(pl2->pthi)->y.whole - PPt(pl2->ptlow)->y.whole)
       <= (PPt(pl2->pthi)->x.whole - PPt(pl2->ptlow)->x.whole)
         * (PPt(pl1->pthi)->y.whole - PPt(pl1->ptlow)->y.whole))
      {lf = ppt->line1; rt = ppt->line2;}
    else {lf = ppt->line2; rt = ppt->line1;};
  }
plf = PLn(lf);  prt = PLn(rt);
  { register PRegion prgrt, prgmid;
    rgmid = MakeRegion(lf, rt);  prgmid = PRg(rgmid);
    prgmid->clipW = prglf->clipW;
    prgmid->figW = prglf->figW;
    delta = (plf->isUpLine) ? -1 : 1;
    if (plf->isClipLine) prgmid->clipW += delta;
    else prgmid->figW += delta;
    rgrt = MakeRegion(rt, prglf->lnrt);  prgrt = PRg(rgrt);
    prgrt->clipW = prgmid->clipW;
    prgrt->figW = prgmid->figW;
    delta = (prt->isUpLine) ? -1 : 1;
    if (prt->isClipLine) prgrt->clipW += delta;
    else prgrt->figW += delta;
    prgrt->xbr = prglf->xbr;
    prgrt->output = prglf->output;
    prgrt->yb = prgmid->yb = fxdpty;
    prgrt->xbl = prgmid->xbl = prgmid->xbr = prglf->xbr = fxdptx;
    prglf->lnrt = lf;
    plf->reglf = rglf;
    /* check for intersections of adjacent lines */
    CheckIntersection(prglf->u.lnlf, lf, callBack);
    CheckIntersection(rt, PRg(prt->regrt)->lnrt, callBack);
    }
} /* end of RdcEntry */


private procedure RdcBend(pt, callBack)
  PtOffset pt; procedure (*callBack)(/* yt, yb, xtl, xtr, xbl, xbr */);
{
register PPoint ppt;
register LnOffset oldln, newln;
register PLine poldln, pnewln;
register RgOffset rglf, rgrt;
PRegion prglf;
register PRegion prgrt;
register Fixed fxdptx, fxdpty;
#ifdef DEBUG
if (debugOn) DebugPrintEvent("Bend ", pt);
#endif
ppt = PPt(pt);
if (PLn(ppt->line1)->regrt == 0) {oldln = ppt->line2; newln = ppt->line1;}
else {oldln = ppt->line1; newln = ppt->line2;};
poldln = PLn(oldln);  pnewln = PLn(newln);
/* replace existing line in the region list by the new line */
rglf = pnewln->reglf = poldln->reglf;  prglf = PRg(rglf);
rgrt = pnewln->regrt = poldln->regrt;  prgrt = PRg(rgrt);
prglf->lnrt = prgrt->u.lnlf = newln;
poldln->reglf = poldln->regrt = 0;
fxdptx = RatToFixed(&ppt->x);
fxdpty = RatToFixed(&ppt->y);
/* output trapezoids in the two adjacent regions */
if (prglf->figW != 0)
  ShipRightAnchoredTrap
        (callBack, rglf, fxdpty, fxdptx, false, prglf->u.lnlf, true);
if (prgrt->figW != 0 && prgrt->lnrt != 0)
  ShipLeftAnchoredTrap
        (callBack, rgrt, fxdpty, fxdptx, false, prgrt->lnrt, false);
/* check for intersections with adjacent lines */
CheckIntersection(PRg(pnewln->reglf)->u.lnlf, newln, callBack);
CheckIntersection(newln, PRg(pnewln->regrt)->lnrt, callBack);
} /* end of RdcBend */


private procedure RdcExit(pt, callBack)
  PtOffset pt; procedure (*callBack)(/* yt, yb, xtl, xtr, xbl, xbr */);
{
PPoint ppt;
RgOffset rglf, rgmid, rgrt, rglfrt;
PRegion prglf, prgmid, prgrt, prglfrt;
LnOffset lnlf, lnrt, lnlfrt;
PLine plnlf, plnrt, plnlfrt;
Fixed fxdptx, fxdpty, rglfxl, rgrtxr;
short int delta;
#ifdef DEBUG
if (debugOn) DebugPrintEvent("Exit ", pt);
#endif
ppt = PPt(pt);
if (PRg(PLn(ppt->line1)->regrt)->lnrt == ppt->line2)
  {lnlf = ppt->line1; lnrt = ppt->line2;}
else {lnlf = ppt->line2; lnrt = ppt->line1;};
plnlf = PLn(lnlf);  plnrt = PLn(lnrt);
fxdptx = RatToFixed(&ppt->x);
fxdpty = RatToFixed(&ppt->y);
if (plnlf->regrt != plnrt->reglf)
  {/* complicated case--lines bound more than one region*/
   /* for now, find lnlf and lnrt by searching to the right of each
      of line1 and line2 until we find the other line. */
  while (true)
    {
    lnlf = PRg(plnlf->regrt)->lnrt;
    if (lnlf == 0) {lnlf = lnrt; lnrt = ppt->line2; break;}
    if (lnlf == lnrt) {lnlf = ppt->line2; break;}
    plnlf = PLn(lnlf);
    };
  };
plnlf = PLn(lnlf);  plnrt = PLn(lnrt);
rglf = plnlf->reglf;  prglf = PRg(rglf);
rgmid = plnrt->reglf;  prgmid = PRg(rgmid);
rgrt = plnrt->regrt;  prgrt = PRg(rgrt);
if (prglf->figW != 0)
  {
  rglfxl = FixedXatY(prglf->u.lnlf, fxdpty, true);
  if (rglfxl > fxdptx) rglfxl = fxdptx;
  }
while (plnlf->regrt != rgmid)
  {/* output trapezoid of left region and fix up winding number of left inner
      region until only one region separates the lines. */
  if (prglf->figW != 0)
    ShipTrapezoid(callBack, rglf, fxdpty, rglfxl, fxdptx, false);
  rglfrt = plnlf->regrt;  prglfrt = PRg(rglfrt);
  if (prglfrt->figW != 0)
    ShipTrapezoid(callBack, rglfrt, fxdpty, fxdptx, fxdptx, false);
  prglfrt->yb = fxdpty;
  prglfrt->xbl = prglfrt->xbr = fxdptx;
  lnlfrt = prglfrt->lnrt;  plnlfrt = PLn(lnlfrt);
  PRg(plnlfrt->regrt)->u.lnlf = lnlf;
  plnlf->regrt = plnlfrt->regrt;
  plnlfrt->reglf = rglf;
  plnlfrt->regrt = plnlf->reglf = rglfrt;
  ShortenLine(lnlfrt, ppt->x, ppt->y);
  prglfrt->lnrt = lnlf;
  prglfrt->u.lnlf = prglf->lnrt = lnlfrt;
  delta = (plnlf->isUpLine) ? 1 : -1;
  if (plnlf->isClipLine) prglfrt->clipW += delta;
  else prglfrt->figW += delta;
  delta = (plnlfrt->isUpLine) ? -1 : 1;
  if (plnlfrt->isClipLine) prglfrt->clipW += delta;
  else prglfrt->figW += delta;
  CheckIntersection(prglf->u.lnlf, lnlfrt, callBack);
  rglf = plnlf->reglf;  prglf = PRg(rglf);
  rglfxl = fxdptx;
  }
/* output trapezoids for bounded region and its neighbors. */
if (prglf->figW != 0)
  ShipTrapezoid(callBack, rglf, fxdpty, rglfxl, fxdptx, false);
if (prgmid->figW != 0)
  ShipTrapezoid(callBack, rgmid, fxdpty, fxdptx, fxdptx, true);
if (prgrt->figW != 0)
  {
  rgrtxr = FixedXatY(prgrt->lnrt, fxdpty, false);
  if (rgrtxr < fxdptx) rgrtxr = fxdptx;
  ShipTrapezoid(callBack, rgrt, fxdpty, fxdptx, rgrtxr, false);
  }
/* delete the bounded region and join its neighbors together. */
plnrt->reglf = plnrt->regrt = plnlf->reglf = plnlf->regrt = 0;
prglf->lnrt = prgrt->lnrt;
if (prgrt->lnrt != 0) PLn(prgrt->lnrt)->reglf = rglf;
prglf->yb = fxdpty;
if (prglf->lnrt != 0) prglf->xbr = prgrt->xbr;
prglf->output = (boolean)(prglf->output && prgrt->output);
exitRegion = rglf;
FreeRegion(rgmid);
FreeRegion(rgrt);
CheckIntersection(prglf->u.lnlf, prglf->lnrt, callBack);
} /* end of RdcExit */


public procedure Reduce(callBack, clipInterior, eo)
  procedure (*callBack)(/* yt, yb, xtl, xtr, xbl, xbr */);
  boolean clipInterior, eo;
{
rgHead = MakeRegion(0, 0);
if (lastPoint != 0) RdcClose();
interiorClipMode = clipInterior;
eoRule = eo;
if (clipInterior) {fclpxmin = FixInt(clipxmin);  fclpxmax = FixInt(clipxmax);}
  { register PqOffset pq;
    register PPointQ ppq;
    register PtOffset pt;
    register PPoint ppt;
    while ((pq = PopPQ()) != 0)
      {
      ppq = PPq(pq);
      pt = ppq->pt;  ppt = PPt(pt);
      if (ppt->y.whole > yMax) break;
      switch ((PLn(ppt->line1)->regrt != 0) + (PLn(ppt->line2)->regrt != 0))
        {
        case 0: RdcEntry(pt, callBack, ppq->intersect); break;
        case 1: RdcBend(pt, callBack); break;
        case 2: RdcExit(pt, callBack); break;
        endswitch;
        };
#ifdef DEBUG
      if (debugOn) DebugPrintRegions();
#endif
      };
    }
  { register RgOffset rg;
    register PRegion prg;
    while (rgHead != 0)
      {
      prg = PRg(rgHead);
      if (prg->lnrt == 0) rg = 0; else rg = PLn(prg->lnrt)->regrt;
      FreeRegion(rgHead);
      rgHead = rg;
      }
    }
} /* end of Reduce */


#ifdef DEBUG
private Object psReduceCallBackObj;

private boolean ReducePSCallBack(yt, yb, xtl, xtr, xbl, xbr)
  Fixed yt, yb, xtl, xtr, xbl, xbr;
{
PushInteger(yt);
PushInteger(yb);
PushInteger(xtl);
PushInteger(xtr);
PushInteger(xbl);
PushInteger(xbr);
return psExecute(psReduceCallBackObj);
} /* end of ReducePSCallBack */
#endif

#ifdef DEBUG
private procedure psReduce()
{
PopP(&psReduceCallBackObj);
Reduce(ReducePSCallBack, false, false);
} /* end of psReduce */
#endif

#ifdef DEBUG
private procedure psReduceAndClip()
{
PopP(&psReduceCallBackObj);
Reduce(ReducePSCallBack, true, false);
} /* end of psReduce */
#endif

public procedure RdcClip(b)  boolean b;  {curIsClipLine = b;}


#ifdef DEBUG
private procedure psSetClip()
{
RdcClip(true);
} /* end of psSetClip */
#endif

#ifdef DEBUG
private procedure psSetFig()
{
RdcClip(false);
} /* end of psSetFig */
#endif

#ifdef DEBUG
private procedure psReducerDebug()
{
if (debugOn) debugOn = false; else debugOn = true;
} /* end of psReducerDebug */
#endif


public procedure IniReducer(reason)  InitReason reason; {
  integer len;
  switch (reason)
    {
    case init:
#ifdef DEBUG
      debugOn = false;
#endif
      if (LENBUFF0 < maxPoints * sizeof(Point))
        len = maxPoints * sizeof(Point);
      else len = LENBUFF0;
      gbuf[0].ptr = (char *)NEW(1, len);
      gbuf[0].len = len;
      points = (PPoint)gbuf[0].ptr;
      endPoint = (len / sizeof(Point));
      endPoint *= sizeof(Point);
	  
      if (LENBUFF1 < maxLines * sizeof(Line))
        len = maxLines * sizeof(Line);
      else len = LENBUFF1;
      gbuf[1].ptr = (char *)NEW(1, len);
      gbuf[1].len = len;
      lines = (PLine)gbuf[1].ptr;
      endLine = len / sizeof(Line);
      endLine *= sizeof(Line);
  
      if (LENBUFF2 < maxPntQ * sizeof(PointQ))
        len = maxPntQ * sizeof(PointQ);
      else len = LENBUFF2;
      gbuf[2].ptr = (char *)NEW(1, len);
      gbuf[2].len = len;
      pointQs = (PPointQ) gbuf[2].ptr;
      endPntQ = len / sizeof(PointQ);
      endPntQ *= sizeof(PointQ);
  
      if (LENBUFF3 < maxRegions * sizeof(Region))
        len = maxRegions * sizeof(Region);
      else len = LENBUFF3;
      gbuf[3].ptr = (char *)NEW(1, len);
      gbuf[3].len = len;
      regions = (PRegion) gbuf[3].ptr;
  
      rgsOut = 1;
      InitReducer();
      break;
    case romreg:
#ifdef DEBUG
      RgstExplicit("rreset", psResetReducer);
      RgstExplicit("rpoint", psNewPoint);
      RgstExplicit("rclose", psRdcClose);
      RgstExplicit("reduce", psReduce);
      RgstExplicit("rdebug", psReducerDebug);
      RgstExplicit("reduceclip", psReduceAndClip);
      RgstExplicit("rsetclip", psSetClip);
      RgstExplicit("rsetfig", psSetFig);
      RgstExplicit("rcomp", psCompRat);
      RgstExplicit("fpmult", psFPMult);
      RgstExplicit("div32", psDiv32);
#endif
      break;
    }
  IniQReducer(&gbuf[0], &gbuf[1], &gbuf[2]);
  IniCScan(&gbuf[0], &gbuf[1], &gbuf[2], &gbuf[3]);
  } /* end of IniReducer */
