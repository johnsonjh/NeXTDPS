/*
  stroke.c

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

Original version: Doug Brotz, May 11, 1983
Edit History:
Larry Baer: Tue Nov 14 15:10:33 1989
Scott Byer: Thu Jun  1 15:08:20 1989
Doug Brotz: Mon Oct  6 14:57:20 1986
Chuck Geschke: Thu Oct 10 06:30:11 1985
Ed Taft: Thu Jul 28 13:25:29 1988
John Gaffney: Fri Feb  1 13:03:18 1985
Bill Paxton: Tue Oct 25 13:37:44 1988
Don Andrews: Wed Sep 17 14:55:53 1986
Ken Lent: Mon Apr 20 18:08:45 1987
Mike Schuster: Sun Jun 14 12:55:44 1987
Ivor Durham: Fri Aug 26 11:11:37 1988
Linda Gass: Thu Dec  3 17:35:37 1987
Matt Foley: Tue 24 Nov 87
Jim Sandman: Wed Dec 13 12:57:45 1989
Perry Caro: Wed Mar 29 11:20:55 1989
Joe Pasqua: Wed Dec 14 15:55:34 1988
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
#include VM

#include "graphicspriv.h"
#include "path.h"
#include "stroke.h"
#include "graphicsnames.h"

extern DevPrim * DoRdcPth();
extern procedure FeedPathToReducer();
extern BBoxCompareResult BBoxVsClipBBox();
extern procedure AddTrap();
extern procedure NoOp();
extern DevBBox GetDevClipDevBBox();
extern PCIItem FindInCache();
#if (OS == os_vms)
globalref PMarkStateProcs stdMarkProcs;
#else (OS == os_vms)
extern PMarkStateProcs stdMarkProcs;
#endif (OS == os_vms)

#define FracOne 0x40000000
#define FixOne 0x10000

/* The following global variables hold point and vector information
   for the stroke algorithm from segment to segment.  The stroke model
   provides a uniform stroke width in user coordinates in the current
   user space.  If the current transformation matrix is anamorphic scale
   or skew, then the stroke width is not uniform in device space.  So,
   in the worst case, all device coordinates input to the stroke
   algorithm must be inverse transformed back to user space where the
   vector calculations are performed, and the results must be transformed
   back to device space.  If the transformation is benign, we can remain
   in device space for all calculations.  The global variables indicate
   whether they are device coordinates by the "d" prefix or uniform
   coordinates (user space in worst case or device space in best case)
   by the "u" prefix. */

public procedure (*StrkTrp)();
public real strkFoo;
public CircleRec *CircleCache;
public QdCorner *qc;

#define sizeCircleMasks (32)
#define MAXCIRCLEJOINDEPTH (4)

private real cosTable[] = { /* cosTable[i] == cos(i*2) for 0 <= i <= 45 */
1.0,		0.999391,	0.997564,
0.994522,	0.990268,	0.984808,
0.978148,	0.970296,	0.961262,
0.951057,	0.939693,	0.927184,
0.913545,	0.898794,	0.882948,
0.866025,	0.848048,	0.829038,
0.809017,	0.788011,	0.766044,
0.743145,	0.71934,	0.694658,
0.669131,	0.642788,	0.615662,
0.587785,	0.559193,	0.529919,
0.5,		0.469472,	0.438371,
0.406737,	0.374607,	0.34202,
0.309017,	0.275637,	0.241922,
0.207912,	0.173648,	0.139173,
0.104528,	0.0697565,	0.0348995,
0.0
};

#if (OS != os_mpw)

/*-- BEGIN GLOBALS --*/
private  Cd      uP1St,
          dP1St,
          uV2St,
          dLfSt,
          dRtSt,
          uP1,
          dP1,
          uV1,
          dLf,
          dRt;
private boolean needTfm,
          allSegmentsIn,
          needClip,
          doingVectors,
          needMaxBevelChord,
          circleTraps,
          haveBounds,
          dashed,
          filledDash,
          ffldDsh,
          incurve,
          atcurve,
          mitposs,
          isStrkPth,
          normalize,
	  oddXwidth,
	  oddYwidth;
private real    uhalfwidth,
          maxBevelChord,
          prevRadiusForMaxBevelChord,
          xLowCirCenter,
          xHiCirCenter,
          yLowCirCenter,
          yHiCirCenter,
          throwThreshold,
          MAXStrokeThrow;
private Mtx     curIMtx;
private real    dashLength[DASHLIMIT],
          crDshLen,
          fDshLen;
private integer dashLim,
          crDash,
          fcrDash;
private DictObj circleFont;
private Path    strokePath;
private BBoxRec gPathBBox;
private BBox    clipBBox;
private DevCd   f_dRt,
          f_dLf,
          f_uV1,
	  f_dP1,
	  strkll,
	  strkur,
          f_dP1St,
          f_dRtSt,
          f_dLfSt,
          f_uV2St;
private Fixed   f_throwThreshold,
          f_halfwidth,
          f_maxBevelChord;
private Fixed   FracSqrt2;
private real    circ_uhalfwidth,
          circ_a,
          circ_b,
          circ_c,
          circ_d;
private boolean circ_cannot,
  	  strkTstRct,
	  needVec;
private Card16  circ_size,
          circ_maxsize;
private integer circ_maskID;
private PVoidProc StrokeSemaphore;
private DevMask *circleMasks,
         *scip,
         *endCircleMasks;
private Fixed   circ_llx,
          circ_lly,
          circ_urx,
          circ_ury;
/*-- END GLOBALS --*/

#else (OS != os_mpw)

typedef struct {
  Cd      _uP1St,
          _dP1St,
          _uV2St,
          _dLfSt,
          _dRtSt,
          _uP1,
          _dP1,
          _uV1,
          _dLf,
          _dRt;
  boolean _needTfm,
          _allSegmentsIn,
          _needClip,
          _doingVectors,
          _needMaxBevelChord,
          _circleTraps,
          _haveBounds,
          _dashed,
          _filledDash,
          _ffldDsh,
          _incurve,
          _atcurve,
          _mitposs,
          _isStrkPth,
          _normalize,
	  _oddXwidth,
	  _oddYwidth;
  real    _uhalfwidth,
          _maxBevelChord,
          _prevRadiusForMaxBevelChord,
          _xLowCirCenter,
          _xHiCirCenter,
          _yLowCirCenter,
          _yHiCirCenter,
          _throwThreshold,
          _MAXStrokeThrow;
  Mtx     _curIMtx;
  real    _dashLength[DASHLIMIT],
          _crDshLen,
          _fDshLen;
  integer _dashLim,
          _crDash,
          _fcrDash;
  DictObj _circleFont;
  Path    _strokePath;
  BBoxRec _gPathBBox;
  BBox    _clipBBox;
  DevCd   _f_dRt,
          _f_dLf,
          _f_uV1,
	  _f_dP1,
	  _strkll,
	  _strkur,
          _f_dP1St,
          _f_dRtSt,
          _f_dLfSt,
          _f_uV2St;
  Fixed   _f_throwThreshold,
          _f_halfwidth,
          _f_maxBevelChord;
  Fixed   _FracSqrt2;
  real    _circ_uhalfwidth,
          _circ_a,
          _circ_b,
          _circ_c,
          _circ_d;
  boolean _circ_cannot,
  	  _strkTstRct,
	  _needVec;
  Card16  _circ_size,
          _circ_maxsize;
  integer _circ_maskID;
  PVoidProc _StrokeSemaphore;
  DevMask *_circleMasks,
         *_scip,
         *_endCircleMasks;
  Fixed   _circ_llx,
          _circ_lly,
          _circ_urx,
          _circ_ury;
  } GlobalsRec, *Globals;
  
private Globals strokeGlobals;


#define	uP1St		 (strokeGlobals->_uP1St)
#define dP1St		 (strokeGlobals->_dP1St)
#define uV2St		 (strokeGlobals->_uV2St)
#define	dLfSt		 (strokeGlobals->_dLfSt)
#define dRtSt		 (strokeGlobals->_dRtSt)
#define uP1		 (strokeGlobals->_uP1)
#define dP1		 (strokeGlobals->_dP1)
#define uV1		 (strokeGlobals->_uV1)
#define dLf		 (strokeGlobals->_dLf)
#define dRt		 (strokeGlobals->_dRt)
#define needTfm		 (strokeGlobals->_needTfm)
#define allSegmentsIn	 (strokeGlobals->_allSegmentsIn)
#define needClip	 (strokeGlobals->_needClip)
#define doingVectors	 (strokeGlobals->_doingVectors)
#define needMaxBevelChord  (strokeGlobals->_needMaxBevelChord)
#define circleTraps	 (strokeGlobals->_circleTraps)
#define haveBounds	 (strokeGlobals->_haveBounds)
#define dashed		 (strokeGlobals->_dashed)
#define filledDash	 (strokeGlobals->_filledDash)
#define ffldDsh		 (strokeGlobals->_ffldDsh)
#define incurve		 (strokeGlobals->_incurve)
#define atcurve		 (strokeGlobals->_atcurve)
#define mitposs		 (strokeGlobals->_mitposs)
#define isStrkPth	 (strokeGlobals->_isStrkPth)
#define normalize	 (strokeGlobals->_normalize)
#define oddXwidth	 (strokeGlobals->_oddXwidth)
#define oddYwidth	 (strokeGlobals->_oddYwidth)
#define uhalfwidth	 (strokeGlobals->_uhalfwidth)
#define maxBevelChord	 (strokeGlobals->_maxBevelChord)
#define prevRadiusForMaxBevelChord  (strokeGlobals->_prevRadiusForMaxBevelChord)
#define xLowCirCenter	 (strokeGlobals->_xLowCirCenter)
#define xHiCirCenter	 (strokeGlobals->_xHiCirCenter)
#define yLowCirCenter	 (strokeGlobals->_yLowCirCenter)
#define yHiCirCenter	 (strokeGlobals->_yHiCirCenter)
#define throwThreshold	 (strokeGlobals->_throwThreshold)
#define MAXStrokeThrow	 (strokeGlobals->_MAXStrokeThrow)
#define curIMtx		 (strokeGlobals->_curIMtx)
#define dashLength	 (strokeGlobals->_dashLength)
#define crDshLen	 (strokeGlobals->_crDshLen)
#define fDshLen		 (strokeGlobals->_fDshLen)
#define dashLim		 (strokeGlobals->_dashLim)
#define crDash		 (strokeGlobals->_crDash)
#define fcrDash		 (strokeGlobals->_fcrDash)
#define circleFont	 (strokeGlobals->_circleFont)
#define strokePath	 (strokeGlobals->_strokePath)
#define gPathBBox	 (strokeGlobals->_gPathBBox)
#define clipBBox	 (strokeGlobals->_clipBBox)
#define f_dRt		 (strokeGlobals->_f_dRt)
#define f_dLf		 (strokeGlobals->_f_dLf)
#define f_dP1		 (strokeGlobals->_f_dP1)
#define f_uV1		 (strokeGlobals->_f_uV1)
#define f_dP1St		 (strokeGlobals->_f_dP1St)
#define f_dRtSt		 (strokeGlobals->_f_dRtSt)
#define f_dLfSt		 (strokeGlobals->_f_dLfSt)
#define f_uV2St		 (strokeGlobals->_f_uV2St)
#define strkll		 (strokeGlobals->_strkll)
#define strkur		 (strokeGlobals->_strkur)
#define f_throwThreshold  (strokeGlobals->_f_throwThreshold)
#define f_halfwidth	 (strokeGlobals->_f_halfwidth)
#define f_maxBevelChord	 (strokeGlobals->_f_maxBevelChord)
#define FracSqrt2	 (strokeGlobals->_FracSqrt2)
#define circ_uhalfwidth	 (strokeGlobals->_circ_uhalfwidth)
#define circ_a		 (strokeGlobals->_circ_a)
#define circ_b		 (strokeGlobals->_circ_b)
#define circ_c		 (strokeGlobals->_circ_c)
#define circ_d		 (strokeGlobals->_circ_d)
#define circ_cannot	 (strokeGlobals->_circ_cannot)
#define strkTstRct	 (strokeGlobals->_strkTstRct)
#define needVec		 (strokeGlobals->_needVec)
#define circ_size	 (strokeGlobals->_circ_size)
#define circ_maxsize	 (strokeGlobals->_circ_maxsize)
#define circ_maskID	 (strokeGlobals->_circ_maskID)
#define StrokeSemaphore	 (strokeGlobals->_StrokeSemaphore)
#define circleMasks	 (strokeGlobals->_circleMasks)
#define scip		 (strokeGlobals->_scip)
#define endCircleMasks	 (strokeGlobals->_endCircleMasks)
#define circ_llx	 (strokeGlobals->_circ_llx)
#define circ_lly	 (strokeGlobals->_circ_lly)
#define circ_urx	 (strokeGlobals->_circ_urx)
#define circ_ury	 (strokeGlobals->_circ_ury)

#endif (OS != os_mpw)


public procedure RgstStrokeSemaphoreProc (proc)
  PVoidProc proc;
{
  StrokeSemaphore = proc;
}

public procedure PSSetStrokeAdjust() {
  gs->strokeAdjust = PopBoolean();
  gs->devhlw = fpZero; /* need to recompute this */
  }

public procedure PSCurrentStrokeAdjust() {
  PushBoolean(gs->strokeAdjust); }

private procedure TransIfNeed(c, pc) Cd c;  PCd pc;
{if (needTfm)  TfmPCd(c, &gs->matrix, pc);  else *pc = c;}

private procedure DTransIfNeed(c, pc) Cd c;  PCd pc;
{if (needTfm)  DTfmPCd(c, &gs->matrix, pc);  else *pc = c;}

private procedure ITransIfNeed(c, pc) Cd c;  PCd pc;
{if (needTfm)  TfmPCd(c, &curIMtx, pc);  else *pc = c;}

#define left 1
#define right 2
#define top 4
#define bottom 8
#ifdef XA
#define loBnd XA_MIN
#define hiBnd XA_MAX
#else XA
#define loBnd (-fp16k)
#define hiBnd (fp16k)
#endif XA

private Card16 CodePoint(p) PCd p; {
  register Card16 c;
  register real r;
  c = 0;
  r = p->x;
  if (r < loBnd) c = left;
  else if (r > hiBnd) c = right;
  r = p->y;
  if (r < loBnd) c += bottom;
  else if (r > hiBnd) c += top;
  return c;
  }

private boolean ClipVect(p0, p1) register PCd p0, p1; {
  /* *p0 is first point of vector */
  /* *p1 is other point of vector */
  /* returns false if vector does not intersect the rectangle */
  /* else returns true with *p0 and *p1 == endpoints of intersection */
  register Card16 c;
  Card16 c0, c1;
  register real x, y;
  c0 = CodePoint(p0);
  c1 = CodePoint(p1);
  while (true) {
    if ((c0 | c1) == 0) return true;
    if ((c0 & c1) != 0) return false;
    c = c0;
    if (c == 0) c = c1;
    if (c & left) { /* vector crosses left edge */
      y = p0->y + (p1->y - p0->y) * ((loBnd - p0->x) / (p1->x - p0->x));
      x = loBnd;
      }
    else if (c & right) { /* vector crosses right edge */
      y = p0->y + (p1->y - p0->y) * ((hiBnd - p0->x) / (p1->x - p0->x));
      x = hiBnd;
      }
    else if (c & bottom) { /* vector crosses bottom edge */
      x = p0->x + (p1->x - p0->x) * ((loBnd - p0->y) / (p1->y - p0->y));
      y = loBnd;
      }
    else if (c & top) { /* vector crosses top edge */
      x = p0->x + (p1->x - p0->x) * ((hiBnd - p0->y) / (p1->y - p0->y));
      y = hiBnd;
      }
    if (c == c0) {
      p0->x = x; p0->y = y; c0 = CodePoint(p0);
      }
    else {
      p1->x = x; p1->y = y; c1 = CodePoint(p1);
      }
    }
  }

#undef left
#undef right
#undef top
#undef bottom
#undef loBnd
#undef hiBnd

private procedure Reinitms (bbOriginal)
  BBoxCompareResult bbOriginal;
{
  (*ms->procs->initMark) (ms, needClip);

  if (haveBounds)
    SetTrapBounds (&gPathBBox);

  ms->bbCompMark = bbOriginal;
}

private procedure FillQuad(c0, c1, c2, c3)  Cd c0, c1, c2, c3;
{
Path path;
register PCd pcd;
DevCd dc0, dc1, dc2, dc3;
if (isStrkPth)
  {
  register integer i;
  pcd = &c0;
  MoveTo(*(pcd++), &strokePath);
  for (i = 0; i < 3; i++) LineTo(*(pcd++), &strokePath);
  ClosePath(&strokePath);
  return;
  }
if (!allSegmentsIn)
  {
  register real bl, tr, c;
  register integer i;
#ifdef XA
  bl = XA_MIN;
  tr = XA_MAX;
#else XA
  bl = -fp16k;
  tr = fp16k;
#endif XA
  pcd = &c0;
  for (i = 0; i < 4; i++) {
    c = pcd->x;
    if (c > tr || c < bl) goto SlowFillQuad;
    pcd++;
    }
  pcd = &c0;
  for (i = 0; i < 4; i++) {
    c = pcd->y;
    if (c > tr || c < bl) goto SlowFillQuad;
    pcd++;
    }
  }

/* call FastFillQuad */ {
FixCd(c0, &dc0);
FixCd(c1, &dc1);
FixCd(c2, &dc2);
FixCd(c3, &dc3);
FastFillQuad(dc0, dc1, dc2, dc3);
return; }

SlowFillQuad: {
  register real bl, tr, c;
  register integer i;
  pcd = &c1;
  bl = tr = c0.x;
  for (i = 0; i < 3; i++) {
    c = pcd->x; if (c > tr) tr = c; if (c < bl) bl = c; pcd++; }
  if (bl > clipBBox->tr.x || tr < clipBBox->bl.x) return;
  pcd = &c1;
  bl = tr = c0.y;
  for (i = 0; i < 3; i++) {
    c = pcd->y; if (c > tr) tr = c; if (c < bl) bl = c; pcd++; }
  if (bl > clipBBox->tr.y || tr < clipBBox->bl.y) return;
  if (doingVectors) {
    if (!ClipVect(&c0, &c2)) return;
    FixCd(c0, &dc0);
    FixCd(c2, &dc2);
    if (dc0.y < dc2.y)
      (*StrkTrp)(dc2.y, dc0.y, dc2.x, dc2.x, dc0.x, dc0.x);
    else
      (*StrkTrp)(dc0.y, dc2.y, dc0.x, dc0.x, dc2.x, dc2.x);
    return;
    }
  InitPath(&path);
  DURING {
    BBoxCompareResult bbTemp = ms->bbCompMark;
    pcd = &c0;
    MoveTo(*(pcd++), &path);
    for (i = 0 ; i < 3; i++) LineTo(*(pcd++), &path);
    ClosePath(&path);
    (*ms->procs->termMark)(ms);
    Fill(&path, false);
    Reinitms (bbTemp);
    }
  HANDLER {FrPth(&path); RERAISE;} END_HANDLER;
  FrPth(&path);
  }
} /* end of FillQuad */


private procedure FastFillBevel(dc0, dc1, dc2, simpleBevel)
  DevCd dc0, dc1, dc2; boolean simpleBevel; {
Fixed yt, yb, xtl, xtr, xbl, xbr, tfixed;

if (simpleBevel) { /* check for unnecessary bevel */
  register integer p0x, p1x, p0y, p1y, p2, diff;
  p0x = FTrunc(dc0.x); p1x = FTrunc(dc1.x);
  p0y = FTrunc(dc0.y); p1y = FTrunc(dc1.y);
  if (p0x == p1x && p0y == p1y &&
      ((FracPart(dc0.x) && FracPart(dc0.y)) ||
       (FracPart(dc1.x) && FracPart(dc1.y)))) return;
  if (FracPart(dc0.x) && FracPart(dc1.x) && FracPart(dc2.x)) {
    p2 = FTrunc(dc2.x);
    if (p2 == p0x && p2 == p1x) return;
    if (p0y == p1y && (p2 == p0x || p2 == p1x)) {
      if ((diff=p0x-p1x) < 0) diff = -diff;
      if (diff == 1) return;
      }
    }
  if (FracPart(dc0.y) && FracPart(dc1.y) && FracPart(dc2.y)) {
    p2 = FTrunc(dc2.y);
    if (p2 == p0y && p2 == p1y) return;
    if (p0x == p1x && (p2 == p0y || p2 == p1y)) {
      if ((diff=p0y-p1y) < 0) diff = -diff;
      if (diff == 1) return;
      }
    }
  }

/* sort so that dc0.y <= dc1.y <= dc2.y */
  { register Fixed fcx, fcy;
  if (dc0.y > dc1.y) { fcx=dc0.x; fcy=dc0.y; dc0=dc1; dc1.x=fcx; dc1.y=fcy; }
  if (dc0.y > dc2.y) { fcx=dc0.x; fcy=dc0.y; dc0=dc2; dc2.x=fcx; dc2.y=fcy; }
  if (dc1.y > dc2.y) { fcx=dc1.x; fcy=dc1.y; dc1=dc2; dc2.x=fcx; dc2.y=fcy; }
  }

if (FTrunc(dc0.y) == FTrunc(dc2.y)) { /* one liner */
  register Fixed f, xl, xr;
  xl = xr = dc0.x;
  f = dc1.x; if (f < xl) xl = f; if (f > xr) xr = f;
  f = dc2.x; if (f < xl) xl = f; if (f > xr) xr = f;
  (*StrkTrp)(dc2.y, dc0.y, xl, xr, xl, xr);
  }
else if (dc0.y == dc1.y) { /* horizontal bottom */
  register Fixed xbl, xbr, tfixed;
  xbl = dc0.x; xbr = dc1.x;
  if (xbl > xbr) { tfixed = xbl; xbl = xbr; xbr = tfixed; }
  (*StrkTrp)(dc2.y, dc0.y, dc2.x, dc2.x, xbl, xbr);
  }
else if (dc1.y == dc2.y) { /* horizontal top */
  register Fixed xtl, xtr, tfixed;
  xtl = dc1.x; xtr = dc2.x;
  if (xtl > xtr) { tfixed = xtl; xtl = xtr; xtr = tfixed; }
  (*StrkTrp)(dc2.y, dc0.y, xtl, xtr, dc0.x, dc0.x);
  }
else { /* split into two triangles */
  register Fixed xtl, xtr, tfixed;
  register boolean needTop, needBot;
  xtl = dc0.x +
    fxfrmul(dc2.x - dc0.x,
      fixratio(dc1.y - dc0.y, dc2.y - dc0.y));
  xtr = dc1.x;
  if (xtl > xtr) { tfixed = xtl; xtl = xtr; xtr = tfixed; }
  needTop = true; needBot = true;
  if (FTrunc(dc1.y) == FTrunc(dc0.y) &&
      FTrunc(xtl) <= FTrunc(dc0.x) &&
      FTrunc(dc0.x) <= FTrunc(xtr)) 
    /* can discard the lower triangle */ needBot = false;
  else if (FTrunc(dc2.y) == FTrunc(dc1.y) &&
           FTrunc(xtl) <= FTrunc(dc2.x) &&
	   FTrunc(dc2.x) <= FTrunc(xtr))
    /* can discard the upper triangle */ needTop = false;
  if (needBot) (*StrkTrp)(dc1.y, dc0.y, xtl, xtr, dc0.x, dc0.x);
  if (needTop) (*StrkTrp)(dc2.y, dc1.y, dc2.x, dc2.x, xtl, xtr);
  }
}  /* end of FastFillBevel */

public Fixed F_Dist(a,b) register Fixed a, b; {
  register Fixed t;
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  if (a < b) { t = a; a = b; b = t; }
  /* now a >= b >= 0 */
  /* compute sqrt(a*a + b*b) by a * sqrt(1 + b/a * b/a) */
  if (a == b || ((t=fixratio(b,a)) == FracOne))
    return fxfrmul(a, FracSqrt2);
  return fxfrmul(a, fracsqrt(FracOne + fracmul(t, t)));
  }

private real LengthTfm(u, maxFlg) Component u; boolean maxFlg; {
  Cd v;
  real d0, d1;
  v.x = u;  v.y = fpZero; DTfmP(v, &v); d0 = Dist(v);
  v.x = fpZero;  v.y = u; DTfmP(v, &v); d1 = Dist(v);
  if (maxFlg) { if (d1 > d0) d0 = d1; }
  else { if (d1 < d0) d0 = d1; }
  v.x = v.y = u * 0.707107; DTfmP(v, &v); d1 = Dist(v);
  if (maxFlg) { if (d1 > d0) d0 = d1; }
  else { if (d1 < d0) d0 = d1; }
  return d0;
  }

#define CHORDTHRESHOLD (0.9) /* greater than 0.25 and less than 1.0 */

private procedure GetMaxBevelChord() {
  register real costheta, *c, radius, chordthreshold;
  radius = (needTfm) ? LengthTfm(uhalfwidth, false) : uhalfwidth;
  if (radius < fpOne) radius = fpOne;
  if (radius >= 5.0) chordthreshold = CHORDTHRESHOLD;
  else chordthreshold = 0.25 + (CHORDTHRESHOLD - 0.25) * (radius - fpOne) / 4.0;
  /* if the bevel chord length is less than maxBevelChord, then
       the max error vs. a circle will be less than chordthreshold */
  if (prevRadiusForMaxBevelChord == radius) goto done;
  costheta = fpOne - chordthreshold / radius;
  Assert(costheta >= fpZero && costheta <= fpOne);
  if (cosTable[22] < costheta) c = &cosTable[22];
  else c = &cosTable[45];
  while (*--c < costheta) {}
  maxBevelChord = fpTwo * radius * cosTable[45 - (c - cosTable)];
  prevRadiusForMaxBevelChord = radius;
  if (maxBevelChord < fp16k) f_maxBevelChord = pflttofix(&maxBevelChord);
  else f_maxBevelChord = FixInt(16000);
 done:
  needMaxBevelChord = false;
  }

private boolean GetCurveMiddle(c0, c1, c2, len, pcd)
  Cd c0, c1, c2, *pcd; real *len; {
  register real dx, dy, dist, r;
  real dist0, dist1;
  Cd dv, dc;
  dx = (c0.x + c1.x) * fpHalf - c2.x;
  dy = (c0.y + c1.y) * fpHalf - c2.y;
  dv.x = dx; dv.y = dy; dist = Dist(dv);
  if (dist == 0) return false;
  r = *len / dist;
  pcd->x = dx * r + c2.x;
  pcd->y = dy * r + c2.y;
  return true;
  }

private procedure FillCurveJoin();	/* forward, but private */

private procedure FastFillCurveJoin(dc0, dc1, dc2, depth, len0, len1)
  DevCd dc0, dc1, dc2; integer depth; Fixed len0, len1; {
  /* see if need to subdivide bevel to approximate a circle */
  /* if depth > 0 then len0 is Dist from c0 to c2 and len1 is Dist c1 to c2 */
  DevCd dc3;
  register Fixed dx, dy, dist, r;
  if (depth >= MAXCIRCLEJOINDEPTH) goto Bevel;
  dx = dc0.x - dc1.x; if (dx < 0) dx = -dx;
  dy = dc0.y - dc1.y; if (dy < 0) dy = -dy;
  if (dx < dy) dist = dy + (dx >> 1);
  else dist = dx + (dy >> 1);
  if (dist == 0) return;
  /* maxdelta + .5 * mindelta >= actual length */
  if (needMaxBevelChord) GetMaxBevelChord();
  if (dist > f_maxBevelChord) { /* subdivide */
    Fixed len3;
    if (needTfm) {
      private procedure FillCurveJoin();
      Cd u0, u1, u2, u3;
      Assert(depth == 0);
      UnFixCd(dc0, &u0);
      UnFixCd(dc1, &u1);
      UnFixCd(dc2, &u2);
      FillCurveJoin(u0, u1, u2, 0, fpZero, fpZero);
      return;
      }
    if (depth == 0) { /* compute len0 and len1 */
      if (!normalize) len0 = len1 = f_halfwidth;
      else {
        dx = dc0.x - dc2.x; dy = dc0.y - dc2.y; len0 = F_Dist(dx, dy);
        dx = dc1.x - dc2.x; dy = dc1.y - dc2.y; len1 = F_Dist(dx, dy);
        }
      }
    len3 = (len0 + len1) >> 1;
    dx = ((dc0.x + dc1.x) >> 1) - dc2.x;
    dy = ((dc0.y + dc1.y) >> 1) - dc2.y;
    dist = F_Dist(dx, dy);
    if (dist == 0) return;
    r = fixdiv(len3, dist);
    dc3.x = fixmul(dx, r) + dc2.x;
    dc3.y = fixmul(dy, r) + dc2.y;
    FastFillCurveJoin(dc0, dc3, dc2, depth+1, len0, len3);
    FastFillCurveJoin(dc3, dc1, dc2, depth+1, len3, len1);
    return;
    }
 Bevel:
  FastFillBevel(dc0, dc1, dc2, depth == 0);
  }

private procedure FillBevel(c0, c1, c2, simpleBevel)
  Cd c0, c1, c2; boolean simpleBevel; {
Path path;
integer i;
if (isStrkPth)
  {
  MoveTo(c0, &strokePath);
  LineTo(c1, &strokePath);
  LineTo(c2, &strokePath);
  ClosePath(&strokePath);
  return;
  }
if (!allSegmentsIn)
  {
  register real bl, tr, c;
#ifdef XA
  bl = XA_MIN;
  tr = XA_MAX;
#else XA
  bl = -fp16k;
  tr = fp16k;
#endif XA
  c = c0.x; if (c > tr || c < bl) goto SlowFillBevel;
  c = c1.x; if (c > tr || c < bl) goto SlowFillBevel;
  c = c2.x; if (c > tr || c < bl) goto SlowFillBevel;
  c = c0.y; if (c > tr || c < bl) goto SlowFillBevel;
  c = c1.y; if (c > tr || c < bl) goto SlowFillBevel;
  c = c2.y; if (c > tr || c < bl) goto SlowFillBevel;
  }
/* call FastFillBevel */ {
DevCd dc0, dc1, dc2;
FixCd(c0, &dc0);
FixCd(c1, &dc1);
FixCd(c2, &dc2);
FastFillBevel(dc0, dc1, dc2, simpleBevel);
return; }

SlowFillBevel:
  {
  register real bl, tr, c;
  bl = tr = c0.x;
  c = c1.x; if (c > tr) tr = c; if (c < bl) bl = c;
  c = c2.x; if (c > tr) tr = c; if (c < bl) bl = c;
  if (bl > clipBBox->tr.x || tr < clipBBox->bl.x) return;
  bl = tr = c0.y;
  c = c1.y; if (c > tr) tr = c; if (c < bl) bl = c;
  c = c2.y; if (c > tr) tr = c; if (c < bl) bl = c;
  if (bl > clipBBox->tr.y || tr < clipBBox->bl.y) return; }
  InitPath(&path);
  DURING {
    BBoxCompareResult bbTemp = ms->bbCompMark;
    MoveTo(c0, &path);
    LineTo(c1, &path);
    LineTo(c2, &path);
    ClosePath(&path);
    (*ms->procs->termMark)(ms);
    Fill(&path, false);
    Reinitms (bbTemp);
    }
  HANDLER {FrPth(&path); RERAISE;} END_HANDLER;
  FrPth(&path);
} /* end of FillBevel */

private procedure FillCurveJoin(c0, c1, c2, depth, len0, len1)
  Cd c0, c1, c2; integer depth; real len0, len1; {
  /* see if need to subdivide bevel to approximate a circle */
  /* if depth > 0 then len0 is Dist from c0 to c2 and len1 is Dist c1 to c2 */
  /* if needTfm then len0 and len1 are user space distances */
  Cd c3, diff;
  register real dx, dy, dist;
  real distNoReg;
  if (depth >= MAXCIRCLEJOINDEPTH) goto Bevel;
  dx = c0.x - c1.x; if (dx < 0) dx = -dx;
  dy = c0.y - c1.y; if (dy < 0) dy = -dy;
  if (dx < dy) dist = dy + dx * fpHalf;
  else dist = dx + dy * fpHalf;
  distNoReg = dist;	/* Because RealEq0 uses & on a register otherwise */
  if (RealEq0(distNoReg)) return;
  /* maxdelta + .5 * mindelta >= actual length */
  if (needMaxBevelChord) GetMaxBevelChord();
  if (dist > maxBevelChord) { /* subdivide */
    real len3;
    if (!needTfm) {
      if (depth == 0) {
        if (!normalize) len0 = len1 = uhalfwidth;
        else {
          diff.x = c0.x - c2.x; diff.y = c0.y - c2.y; len0 = Dist(diff);
	  diff.x = c1.x - c2.x; diff.y = c1.y - c2.y; len1 = Dist(diff);
          }
        }
      len3 = (len0 + len1) * fpHalf;
      if (!GetCurveMiddle(c0, c1, c2, &len3, &c3)) return;
      }
    else { /* go back to userspace to do the calculation */
      Cd u0, u1, u2, u3;
      real ulen3;
      TfmPCd(c0, &curIMtx, &u0);
      TfmPCd(c1, &curIMtx, &u1);
      TfmPCd(c2, &curIMtx, &u2);
      if (depth == 0) {
        if (!normalize) len0 = len1 = uhalfwidth;
        else {
          diff.x = u0.x - u2.x; diff.y = u0.y - u2.y; len0 = Dist(diff);
	  diff.x = u1.x - u2.x; diff.y = u1.y - u2.y; len1 = Dist(diff);
          }
        }
      len3 = (len0 + len1) * fpHalf;
      if (!GetCurveMiddle(u0, u1, u2, &len3, &u3)) return;
      TfmPCd(u3, &gs->matrix, &c3);
      }
    FillCurveJoin(c0, c3, c2, depth+1, len0, len3);
    FillCurveJoin(c3, c1, c2, depth+1, len3, len1);
    return;
    }
 Bevel:
  FillBevel(c0, c1, c2, depth == 0);
  }

private procedure AddCirclePath(dp, i, path)
  Cd dp; register integer i; PPath path; {
  /* dp is center in device coordinates */
  Cd c0, v1, v2, v3, t1, t2, t3, p;
  register real t;
  switch (i) {
    case 0: break;
    case 1: dp.x += fpHalf; break;
    case 2: dp.y += fpHalf; break;
    case 3: dp.x += fpHalf; dp.y += fpHalf; break;
    default: CantHappen();
    }
  ITransIfNeed(dp, &p); /* convert to uniform coordinates */
  c0.x = p.x + uhalfwidth; c0.y = p.y;
  TransIfNeed(c0, &c0);
  MoveTo(c0, path);
  v1.y = v2.x = fpp552 * (v1.x = v2.y = v3.y = uhalfwidth);
  v3.x = 0; i = 0;
  do {
    VecAdd(p, v1, &t1);  VecAdd(p, v2, &t2);  VecAdd(p, v3, &t3);
    TransIfNeed(t1, &t1); TransIfNeed(t2, &t2); TransIfNeed(t3, &t3);
    CurveTo(t1, t2, t3, path);
    if (++i == 4) break;
    t = -v1.y;  v1.y = v1.x;  v1.x = t;
    t = -v2.y;  v2.y = v2.x;  v2.x = t;
    t = -v3.y;  v3.y = v3.x;  v3.x = t;
    } while (true);
  ClosePath(path);
  }

private boolean FalseProc() { return false; }

private boolean EnterCircle(dp, i) Cd dp; integer i; {
  /* dp is center in device coordinates */
  Path path;
  Card16 size;
  DevPrim *devprim = NULL;
  MarkState oldms;
  DevPrim *newTrapDevPrim = NULL;
  boolean result, failed;
  if (circ_cannot) return false;
  InitPath(&path);
  oldms = *ms;
  failed = false;
  DURING
  AddCirclePath(dp, i, &path);
  if (path.bbox.bl.x < -fp16k || path.bbox.bl.y < -fp16k ||
      path.bbox.tr.x >  fp16k || path.bbox.tr.y >  fp16k)
    failed = true;
  else {
    if (ms->trapsDP->items > 0) {
      newTrapDevPrim = InitDevPrim(NewDevPrim(), NULL);
      newTrapDevPrim->type = trapType;
      newTrapDevPrim->maxItems = ATRAPLENGTH;
      newTrapDevPrim->value.trap =
               (DevTrap *)NEW(ATRAPLENGTH, sizeof(DevTrap));
      ms->trapsDP = newTrapDevPrim;
      }
    devprim = DoRdcPth(false, &path, &path.bbox, FalseProc,
                     FeedPathToReducer, NULL, NULL);
    }
  HANDLER { goto failure; } END_HANDLER;
  if (failed) goto failure;
  Assert(devprim && devprim->type == trapType);
  size = DevPrimBytes(devprim);
  if (size + circ_size > circ_maxsize) goto failure;
  CircleCache[i].devprim = devprim;
  CircleCache[i].center = dp;
  circ_size += size;
  result = true;
  goto done;
 failure:
  if (devprim) DisposeDevPrim(devprim);
  circ_cannot = true;
  result = false;
 done:
  FrPth(&path);
  *ms = oldms;
  if (newTrapDevPrim) DisposeDevPrim(newTrapDevPrim);
  return result;
  }

private boolean TransCircle(dp, i) Cd dp; integer i; {
  Cd cd;
  PCircle circle = &CircleCache[i];
  cd.x = dp.x - circle->center.x;
  cd.y = dp.y - circle->center.y;
  if (!TransDevPrim(circle->devprim, cd)) return false;
  circle->center.x = dp.x;
  circle->center.y = dp.y;
  return true;
  }

private boolean AddCircleMask(i, dp) integer i; Cd dp; {
  PCircle circle = &CircleCache[i];
  PMask mask;
  register integer x, y, z;
  if (os_fabs(dp.x) > fp16k || os_fabs(dp.y) > fp16k) return false;
  if (scip == endCircleMasks) {
    (*ms->procs->strokeMasksFilled)(circleMasks, scip - circleMasks,
                     circ_llx, circ_lly, circ_urx, circ_ury);
    scip = circleMasks;
    }
  if (scip == circleMasks) {
    circ_llx = circ_lly = 16000; circ_urx = circ_ury = -16000; }
  scip->mask = mask = circle->mask;
  scip->dc.x = x = (pflttofix(&dp.x) + circle->offset.x + 0x8000L) >> 16;
  scip->dc.y = y = (pflttofix(&dp.y) + circle->offset.y + 0x8000L) >> 16;
  scip++;
  if (x < circ_llx) circ_llx = x;
  if (y < circ_lly) circ_lly = y;
  if ((z = x + mask->width) > circ_urx) circ_urx = z;
  if ((z = y + mask->height) > circ_ury) circ_ury = z;
  return true;
  }

private procedure MkCirc(c) Cd c; {
  integer i;
  { register real u, t;
    u = os_floor(c.x);  t = c.x - u;  c.x = u;  i = 0;
    if (t >= xLowCirCenter) { if (t < xHiCirCenter) i = 1; else c.x++; }
    u = os_floor(c.y);  t = c.y - u;  c.y = u;
    if (t >= yLowCirCenter) { if (t < yHiCirCenter) i += 2; else c.y++; }
    }
  if (CircleCache[i].devprim == NULL)
    EnterCircle(c, i);
  }

#define fxdeps (64)

private procedure F_NormPair(v, d2) register DevCd *v, *d2; {
  DevCd diff;
  Fixed r;
  diff.x = diff.y = 0;
  if (os_labs(v->x) < fxdeps) { /* vertical */
    r = FracPart(d2->x);
    if (oddXwidth) { /* move odd widths off of .5 */
      r -= 0x8000L;
      if (os_labs(r) < fxdeps) {
        diff.x = (r >= 0) ? fxdeps : -fxdeps; goto mvStrk; }
      }
    else { /* move even widths off of .0 */
      if (r < fxdeps) { diff.x = fxdeps; goto mvStrk; }
      else if (r > (FixOne - fxdeps)) { diff.x = -fxdeps; goto mvStrk; }
      }
    }
  if (os_labs(v->y) < fxdeps) { /* horizontal */
    r = FracPart(d2->y);
    if (oddYwidth) { /* move odd widths off of .5 */
      r -= 0x8000L;
      if (os_labs(r) < fxdeps) {
        diff.y = (r >= 0) ? fxdeps : -fxdeps; goto mvStrk; }
      }
    else { /* move even widths off of .0 */
      if (r < fxdeps) { diff.y = fxdeps; goto mvStrk; }
      else if (r > (FixOne - fxdeps)) { diff.y = -fxdeps; goto mvStrk; }
      }
    }
  r = os_labs(v->x) - os_labs(v->y);
  if (os_labs(r) < fxdeps) { /* move 45 degree diagonals off of pixel corners */
    Fixed s, t;
    s = FracPart(d2->x); t = FracPart(d2->y);
    r = s - t; s += t - FixOne;
    if (os_labs(r) < fxdeps || os_labs(s) < fxdeps) {
      if (r < 0x8000L) diff.x = (r < 0x4000L) ? fxdeps : -fxdeps;
      else diff.x = (r < (0x8000L+0x4000L)) ? fxdeps : -fxdeps;
      goto mvStrk; }
    }
  return;
  mvStrk:
    f_dP1.x += diff.x; f_dP1.y += diff.y;
    d2->x += diff.x; d2->y += diff.y;
  }

#define DistAboveFloor(r) ((r) - os_floor(r))

private boolean NormalizePoint(p, v, del) Cd p, v, *del; {
  real eps, r, s, t;
  Cd diff;
  fixtopflt(fxdeps, &eps);
  diff.x = diff.y = fpZero;
  if (os_fabs(v.x) < eps) { /* vertical */
    r = DistAboveFloor(p.x);
    if (oddXwidth) { /* move odd widths off of .5 */
      r -= fpHalf;
      if (os_fabs(r) < eps) { diff.x = RealGe0(r) ? eps : -eps; goto mvStrk; }
      }
    else { /* move even widths off of .0 */
      if (r < eps) { diff.x = eps; goto mvStrk; }
      else if (r > (fpOne - eps)) { diff.x = -eps; goto mvStrk; }
      }
    }
  if (os_fabs(v.y) < eps) { /* horizontal */
    r = DistAboveFloor(p.y);
    if (oddYwidth) { /* move odd widths off of .5 */
      r -= fpHalf;
      if (os_fabs(r) < eps) { diff.y = RealGe0(r) ? eps : -eps; goto mvStrk; }
      }
    else { /* move even widths off of .0 */
      if (r < eps) { diff.y = eps; goto mvStrk; }
      else if (r > (fpOne - eps)) { diff.y = -eps; goto mvStrk; }
      }
    }
  r = os_fabs(v.x) - os_fabs(v.y);
  if (os_fabs(r) < eps) { /* move 45 degree diagonals off of pixel corners */
    s = DistAboveFloor(p.x); t = DistAboveFloor(p.y);
    r = s - t; s += t - fpOne;
    if (os_fabs(r) < eps || os_fabs(s) < eps) {
      if (r < .5) diff.x = (r < .25) ? eps : -eps;
      else diff.x = (r < .75) ? eps : -eps;
      goto mvStrk; }
    }
  return false;
mvStrk:
  *del = diff;
  return true;
  }

#undef fxdeps
#undef DistAboveFloor

private procedure FillCircle(p, dp)  Cd p, dp; {
  /* Fill in a circle centered at p (uniform coordinates) with
     radius uhalfwidth.  dp is p in device coordinates. */
  register integer i;
  Path path;
  StrObj strobj;
  char s[3];
  Mtx circMtx;
  Cd c;
  PCIItem cip;
  boolean done;
  real r;

  if (RealEq0(uhalfwidth)) return;
  if (normalize) {
    Cd diff;
    c.x = fpOne; c.y = fpZero;
    if (NormalizePoint(dp, c, &diff)) { dp.x += diff.x; dp.y += diff.y; }
    c.x = fpZero; c.y = fpOne;
    if (NormalizePoint(dp, c, &diff)) { dp.x += diff.x; dp.y += diff.y; } 
    }
  if (isStrkPth) { AddCirclePath(dp, 0, &strokePath); return; }
  { register real u, t;
    u = os_floor(dp.x);  t = dp.x - u;  dp.x = u;  i = 0;
    if (t >= xLowCirCenter) { if (t < xHiCirCenter) i = 1; else dp.x++; }
    u = os_floor(dp.y);  t = dp.y - u;  dp.y = u;
    if (t >= yLowCirCenter) { if (t < yHiCirCenter) i += 2; else dp.y++; }
    }
  if (circleTraps) { /* output traps for circle */
    if ((CircleCache[i].devprim || EnterCircle(dp, i)) &&
        TransCircle(dp, i)) { /* use traps from the cache */
      AppendTraps(CircleCache[i].devprim);
      return;
      }
    /* cannot use cache -- do it the hard way */
    InitPath(&path);
    DURING {
      BBoxCompareResult bbTemp = ms->bbCompMark;
      AddCirclePath(dp, i, &path);
      (*ms->procs->termMark)(ms);
      Fill(&path, false);
      Reinitms (bbTemp);
      }
    HANDLER { FrPth(&path); RERAISE; } END_HANDLER;
    FrPth(&path);
    return;
    }
  /* else use circleFont */
  if (CircleCache[i].mask && AddCircleMask(i, dp)) return;
  /* try to cache the circle mask */
  GSave();
  done = false;
  DURING
  circMtx.a = circMtx.d = gs->strokeWidth / fpTwo;
  circMtx.b = circMtx.c = circMtx.tx = circMtx.ty = fpZero;
  MtxCnct(&circMtx, &gs->matrix, &circMtx);
  SetMtx(&circMtx);
  if (circleFont.type != dictObj)
    DictGetP(rootShared->vm.Shared.internalDict,
             graphicsNames[nm_CircleFont], &circleFont);
  SetFont(circleFont);
  cip = FindInCache(i+'A');
  if (cip) {
    cip->circle = true;
    CircleCache[i].offset.x = cip->metrics[0].offset.x;
    CircleCache[i].offset.y = cip->metrics[0].offset.y;
    CircleCache[i].mask = cip->cmp;
    if (AddCircleMask(i, dp)) done = true;
    }
  if (!done) {
    BBoxCompareResult bbTemp = ms->bbCompMark;
    (*ms->procs->termMark)(ms);
    NewPath();
    MoveTo(dp, &gs->path);
    s[0] = i+'A'; s[1] = ' '; s[2] = 0;
    VMObjForPString(s, &strobj);
    if (StrokeSemaphore != NIL)
      (*StrokeSemaphore) (1);	/* Prevent context switch */
    gs->circleAdjust = false;
    DURING {
      SimpleShow(strobj);
    } HANDLER {
      if (StrokeSemaphore != NIL)
        (*StrokeSemaphore) (-1);	/* Possibly reenable context switch */
      RERAISE;
    } END_HANDLER;
    if (StrokeSemaphore != NIL)
      (*StrokeSemaphore) (-1);	/* Possibly reenable context switch */
    (*ms->procs->initMark)(ms, needClip);
    ms->bbCompMark = bbTemp;
    }
  HANDLER {
    if (haveBounds) SetTrapBounds(&gPathBBox);
    GRstr();
    RERAISE;
    }
  END_HANDLER;
  if (!done && haveBounds) SetTrapBounds(&gPathBBox);
 doneWithCircle:
  GRstr();
  }

private Fixed F_AdjstHW(v) DevCd v; {
  /* v is unit vector normal to stroke line segment */
  /* adjust halfwidth to eliminate "lumps" from line */
  /* i.e., make sure steep lines turn on constant number of pixels per row */
  /* and shallow lines turn on constant number of pixels per column */
  Fixed absdx, absdy, delta, otherdelta, step, w, ww, len, dh, dlen;
  if (!normalize || f_halfwidth >= 0xA0000L) return f_halfwidth;
  absdx = os_labs(v.x); absdy = os_labs(v.y);
  if (absdx > absdy) { delta = absdx; otherdelta = absdy; }
  else { delta = absdy; otherdelta = absdx; }
  if (otherdelta == 0) return f_halfwidth; /* horizontal or vertical */
  w = fixdiv((f_halfwidth << 1), delta); /* standard spacing */
  ww = (w + 0x8000L) & 0xFFFF0000L; /* round the spacing */
  step = fixdiv(otherdelta, delta); /* step row to row or column to column */
  ww -= step; /* desired spacing */
  dh = (ww - w) >> 1; /* add this to each end of w to get ww */
  dlen = fixmul(dh, delta); /* add this to len to get new len */
  len = f_halfwidth + dlen;
  len++; /* compensate for half open interval scan conversion */
  return len;
  }

private real AdjstHW(v)  Cd v;
{
real hw, dw, fulldw, skoche, r;
Cd dv;
if (isStrkPth || RealEq0(strkFoo)) {
  DevCd dc;
  if (needTfm || uhalfwidth >= fp10) return uhalfwidth;
  FixCd(v, &dc);
  fixtopflt(F_AdjstHW(dc), &hw);
  }
else if (needTfm)
  {
  VecMul(v, &uhalfwidth, &dv);
  DTfmPCd(dv, &gs->matrix, &dv);
  fulldw = Dist(dv);
  r = fpOne / fulldw;
  VecMul(dv, &r, &dv);
  skoche = strkFoo * os_fabs(dv.x) * os_fabs(dv.y);
  dw = (fulldw > skoche) ? fulldw - skoche : fpZero;
  hw = (fulldw > fpp001) ? uhalfwidth * dw / fulldw : fpZero;
  }
else
  {
  skoche = strkFoo * os_fabs(v.x) * os_fabs(v.y);
  hw = (uhalfwidth > skoche) ? uhalfwidth - skoche : fpZero;
  }
return hw;
}  /* end of AdjstHW */


private boolean ccw(p0, p1, p2) Cd p0, p1, p2; { /* counter clockwise test */
  real dx1, dx2, dy1, dy2;
  dx1 = p1.x - p0.x; dx2 = p2.x - p0.x;
  dy1 = p1.y - p0.y; dy2 = p2.y - p0.y;
  return (dx1 * dy2 > dy1 * dx2);
  }

private boolean MiterPoint(p1, v1, p2, v2, i, c) Cd p1, v1, p2, v2, *i, c;
{ /* returns in "i" the intersection point of line1, given by point p1
     and unit vector v1, with line2, given by point p2 and unit vector v2.
     If v1 and v2 are nearly opposite, resulting in a large "throw distance",
     MiterPoint returns "false" and "i" is untouched;
     otherwise it returns "true". */
  /* c is the center point for the miter.  Valid miter point must
     be between the lines from p1 to v and p2 to c. */
real len, costheta, xi, yi, slp, dx1dy2, dx2dy1, dx1dx2, dy1dy2;
Cd v, m;
if (RealEq0(v1.x) && RealEq0(v2.y)) {
  if (fpp7 < throwThreshold) return false;
  xi = p1.x; yi = p2.y; goto done; }
if (RealEq0(v2.x) && RealEq0(v1.y)) {
  if (fpp7 < throwThreshold) return false;
  xi = p2.x; yi = p1.y; goto done; } 
VecAdd(v1, v2, &v);
len = Dist(v);
if (len < throwThreshold) return false;
len = fpOne / len;
VecMul(v, &len, &v);
costheta = v.x * v1.x + v.y * v1.y;
if (os_fabs(costheta) < throwThreshold) return false;
if (RealEq0(v1.x)) { /* line1 vertical */
  xi = p1.x; yi = p2.y + ((p1.x - p2.x) * v2.y) / v2.x; goto done; }
if (RealEq0(v2.x)) { /* line2 vertical */
  xi = p2.x; yi = p1.y + ((p2.x - p1.x) * v1.y) / v1.x; goto done; }
if (RealEq0(v1.y)) { /* line1 horizontal */
  yi = p1.y; xi = p2.x + ((p1.y - p2.y) * v2.x) / v2.y; goto done; }
if (RealEq0(v2.y)) { /* line2 horizontal */
  yi = p2.y; xi = p1.x + ((p2.y - p1.y) * v1.x) / v1.y; goto done; }
dx1dy2 = v1.x * v2.y; dx2dy1 = v2.x * v1.y;
slp = fpOne / (dx1dy2 - dx2dy1);
dx1dx2 = v1.x * v2.x; dy1dy2 = v1.y * v2.y;
xi = ((p1.y - p2.y) * dx1dx2 + p2.x * dx1dy2 - p1.x * dx2dy1) * slp;
yi = ((p2.x - p1.x) * dy1dy2 + p1.y * dx1dy2 - p2.y * dx2dy1) * slp;
done:
m.x = xi; m.y = yi;
if (normalize && ccw(c, p1, m) != ccw(c, m, p2)) return false;
i->x = xi; i->y = yi; return true;
}  /* end of MiterPoint */


private procedure HalfRoundCap(lf, rt) Cd lf, rt; {
  /* lf and rt in device space */
  Cd midpt, dp;
  ITransIfNeed(lf, &lf); ITransIfNeed(rt, &rt);
  dp.x = (lf.x + rt.x) * fpHalf;
  dp.y = (lf.y + rt.y) * fpHalf;
  midpt.x = dp.x + (lf.y - dp.y);
  midpt.y = dp.y - (lf.x - dp.x);
  TransIfNeed(lf, &lf); TransIfNeed(rt, &rt); 
  TransIfNeed(dp, &dp); TransIfNeed(midpt, &midpt); 
  FillCurveJoin(lf, midpt, dp, 0, fpZero, fpZero);
  FillCurveJoin(midpt, rt, dp, 0, fpZero, fpZero);
  }

private procedure FillLineSegment(p, p2, v, leftNorm, vlen)
  Cd p, p2, v, leftNorm; real vlen;
{
Cd lf, rt, lf2, rt2, dashv, tenonv, capp, transp;
real r;
boolean circlecaps;
VecAdd(p2, leftNorm, &lf2);  TransIfNeed(lf2, &lf2);
VecSub(p2, leftNorm, &rt2);  TransIfNeed(rt2, &rt2);
if (dashed)
  {
  if (gs->lineCap == tenonCap) {r = AdjstHW(v);  VecMul(v, &r, &tenonv);}
  circlecaps = !normalize || (lf2.x == rt2.x || lf2.y == rt2.y);
  while (vlen >= crDshLen)
    {
    vlen -= crDshLen;
    VecMul(v, &crDshLen, &dashv);
    VecAdd(p, dashv, &p);
    if (gs->lineCap == tenonCap)
      {
      if (filledDash) VecAdd(p, tenonv, &capp);
      else VecSub(p, tenonv, &capp);
      }
    else capp = p;
    VecAdd(capp, leftNorm, &lf);  TransIfNeed(lf, &lf);
    VecSub(capp, leftNorm, &rt);  TransIfNeed(rt, &rt);
    if (filledDash) FillQuad(dLf, dRt, rt, lf);
    dRt = rt; dLf = lf;
    if (gs->lineCap == roundCap && RealNe0(uhalfwidth)) {
      if (circlecaps) {TransIfNeed(p, &transp); FillCircle(p, transp);}
      else if (filledDash) HalfRoundCap(lf, rt);
      else HalfRoundCap(rt, lf);
      }
    if (++crDash == dashLim) crDash = 0;
    crDshLen = dashLength[crDash];
    filledDash = !filledDash;
    }
  crDshLen -= vlen;
  }
if (!dashed || filledDash) FillQuad(dLf, dRt, rt2, lf2);
dRt = rt2; dLf = lf2;
} /* end of FillLineSegment */

private procedure FillJoin(lf, rt, v)  Cd lf, rt, v; {
  integer turn;
  Cd temp;
  register integer tx, ty, px, py;
  if (RealEq0(uhalfwidth)) return;
  if (!mitposs /* this is a join internal to a curve */ ||
      (normalize && gs->lineJoin == roundJoin)) {
    if (mitposs && /* use circle at horizontal-vertical join */
        ((dRt.x == dLf.x && rt.y == lf.y) ||
         (dRt.y == dLf.y && rt.x == lf.x))) {
      FillCircle(uP1, dP1); return; }
    turn = VecTurn(uV1, v);
    if (turn > 0) /* left turn */
      FillCurveJoin(dRt, rt, dP1, 0, fpZero, fpZero);
    else if (turn < 0) /* right turn */
      FillCurveJoin(dLf, lf, dP1, 0, fpZero, fpZero);
    else if (uV1.x != v.x) /* u turn */
      HalfRoundCap(dLf, dRt);
    return;
    }
  if (gs->lineJoin == roundJoin) {
    FillCircle(uP1, dP1); return; }
  turn = VecTurn(uV1, v);
  if (turn != 0 && gs->lineJoin == miterJoin) {
    if (turn > 0) { /* left turn */
      Cd urt, uRt;
      ITransIfNeed(rt, &urt); ITransIfNeed(dRt, &uRt);
      if (!MiterPoint(uRt, uV1, urt, v, &temp, uP1)) goto Bevel;
      TransIfNeed(temp, &temp);
      tx = temp.x; ty = temp.y; px = dRt.x; py = dRt.y; /* truncate */
      if (px==tx && py==ty) goto Bevel;
      px = rt.x; py = rt.y;
      if (px==tx && py==ty) goto Bevel;
      FillQuad(dRt, temp, rt, dP1);
      }
    else { /* right turn */
      Cd ulf, uLf;
      ITransIfNeed(lf, &ulf); ITransIfNeed(dLf, &uLf);
      if (!MiterPoint(uLf, uV1, ulf, v, &temp, uP1)) goto Bevel;
      TransIfNeed(temp, &temp);
      tx = temp.x; ty = temp.y; px = dLf.x; py = dLf.y; /* truncate */
      if (px==tx && py==ty) goto Bevel;
      px = lf.x; py = lf.y;
      if (px==tx && py==ty) goto Bevel;
      FillQuad(dLf, dP1, lf, temp);
      }
    return;
    }
 Bevel:
  if (turn > 0) FillBevel(dRt, rt, dP1, true);
  else FillBevel(dLf, lf, dP1, true);
  }  /* end of FillJoin */


private procedure StrkStrt(dP) Cd dP;
{
dP1St = dP1 = dP;
ITransIfNeed(dP, &uP1);  uP1St = uP1;
needVec = true; atcurve = false;
if (dashed) {crDash = fcrDash; crDshLen = fDshLen; filledDash = ffldDsh;}
}  /* end of StrkStrt */


private procedure StrkLnTo(dP2)  Cd dP2;
{ /* dP2 is in device coordinates. */
real len, r;
Cd v, lf, rt, uP2, leftNorm, diff, dP2init;
dP2init = dP2;
VecSub(dP2, dP1, &v);
if (atcurve && os_fabs(v.x) < 0.01 && os_fabs(v.y) < 0.01) return;
if (normalize && NormalizePoint(dP1, v, &diff)) {
  dP1.x += diff.x; dP1.y += diff.y;
  ITransIfNeed(dP1, &uP1);
  dP2.x += diff.x; dP2.y += diff.y;
  }
ITransIfNeed(dP2, &uP2);
if (doingVectors && !dashed) FillQuad(dP1, dP1, dP2, dP2);
else
  {
  VecSub(uP2, uP1, &v);
  if (RealEq0(v.x))
    {
    if (RealEq0(v.y)) return;
    else if (RealGt0(v.y)) {len = v.y; v.y = fpOne;}
    else {len = -v.y; v.y = -fpOne;}
    }
  else if (RealEq0(v.y))
    {if (RealGt0(v.x)) {len = v.x; v.x = fpOne;} else {len = -v.x; v.x = -fpOne;}}
  else
    {
    len = Dist(v);
    if (RealEq0(len)) return;
    r = fpOne/len;
    VecMul(v, &r, &v);
    }
  leftNorm.x = -v.y;  leftNorm.y = v.x;
  r = AdjstHW(leftNorm);
  VecMul(leftNorm, &r, &leftNorm);
  VecAdd(uP1, leftNorm, &lf);  TransIfNeed(lf, &lf);
  VecSub(uP1, leftNorm, &rt);  TransIfNeed(rt, &rt);
  if (needVec) {dRtSt = rt; dLfSt = lf; uV2St = v;}
  else if (!dashed || filledDash) FillJoin(lf, rt, v);
  dRt = rt;  dLf = lf;
  FillLineSegment(uP1, uP2, v, leftNorm, len);
  }
needVec = false;
dP1 = dP2init; ITransIfNeed(dP1, &uP1); uV1 = v;
if (incurve) mitposs = false;
atcurve = false;
} /* end of StrkLnTo */

private procedure StrkCap()
{
Cd lf, rt, tenonv, dp, up;
if (RealEq0(uhalfwidth)) return;
switch (gs->lineCap)
  {
  case buttCap: return;
  case roundCap:
    if (!needVec && (!dashed || filledDash)) {
      if (normalize && dLf.x != dRt.x && dLf.y != dRt.y)
        HalfRoundCap(dLf, dRt);
      else {
        if (!normalize) { dp = dP1; up = uP1; }
	else {
          dp.x = (dLf.x + dRt.x) * fpHalf;
	  dp.y = (dLf.y + dRt.y) * fpHalf;
	  ITransIfNeed(dp, &up);
          }
        FillCircle(up, dp);
	}
      }
    if (!dashed || ffldDsh) {
      if (!needVec && normalize && dLfSt.x != dRtSt.x && dLfSt.y != dRtSt.y)
        HalfRoundCap(dRtSt, dLfSt);
      else {
        if (needVec || !normalize) { dp = dP1St; up = uP1St; }
	else {
          dp.x = (dLfSt.x + dRtSt.x) * fpHalf;
	  dp.y = (dLfSt.y + dRtSt.y) * fpHalf;
	  ITransIfNeed(dp, &up);
          }
        FillCircle(up, dp);
	}
      }
    return;
  case tenonCap: {
    real len;
    Cd uLf, uRt, diff;
    if (needVec) return;
    if (!dashed || filledDash) {
      if (!normalize) len = uhalfwidth;
      else {
        ITransIfNeed(dLf, &uLf); ITransIfNeed(dRt, &uRt);
	VecSub(uLf, uRt, &diff); len = Dist(diff) * fpHalf;
        }
      VecMul(uV1, &len, &tenonv);
      DTransIfNeed(tenonv, &tenonv);
      VecAdd(dLf, tenonv, &lf);  VecAdd(dRt, tenonv, &rt);
      FillQuad(dRt, rt, lf, dLf);
      }
    if (!dashed || ffldDsh) {
      if (!normalize) len = uhalfwidth;
      else {
        ITransIfNeed(dLfSt, &uLf); ITransIfNeed(dRtSt, &uRt);
	VecSub(uLf, uRt, &diff); len = Dist(diff) * fpHalf;
        }
      VecMul(uV2St, &len, &tenonv);
      DTransIfNeed(tenonv, &tenonv);
      VecSub(dLfSt, tenonv, &lf);  VecSub(dRtSt, tenonv, &rt);
      FillQuad(dRtSt, dLfSt, lf, rt);
      }
    }
  }
} /* end of StrkCap */


private procedure StrkClose()
{
if (needVec) {if (gs->lineCap == roundCap) FillCircle(uP1St, dP1St); return;}
StrkLnTo(dP1St);
if (!dashed || filledDash) FillJoin(dLfSt, dRtSt, uV2St);
} /* end of StrkClose */


/* Fixed versions */
/* assume !isStrkPth && allSegmentsIn && !needTfm &&
          !dashed && strkFoo==0.0 */

#define F_VecAdd(v1,v2,v3) {(v3).x=(v1).x+(v2).x; (v3).y=(v1).y+(v2).y;}
#define F_VecSub(v1,v2,v3) {(v3).x=(v1).x-(v2).x; (v3).y=(v1).y-(v2).y;}
#define F_VecMul(v,r,v2) {(v2).x=fixmul((v).x,(r)); (v2).y=fixmul((v).y,(r));}

public integer F_VecTurn(v1, v2) DevCd v1, v2;
{ /* v1 & v2 are unit vectors.
     returns 1 if v1 followed by v2 makes a left turn, 0 if straight or
     u-turn, -1 if right turn. */
register Fixed dir;
dir = fixmul(v1.x, v2.y) - fixmul(v2.x, v1.y);
return (dir==0) ? 0 : (dir < 0) ? -1 : 1;
} /* end of F_VecTurn */

private boolean f_ccw(p0, p1, p2) DevCd p0, p1, p2; {
  /* counter clockwise test */
  Fixed dx1, dx2, dy1, dy2;
  dx1 = p1.x - p0.x; dx2 = p2.x - p0.x;
  dy1 = p1.y - p0.y; dy2 = p2.y - p0.y;
  return (fixmul(dx1, dy2) > fixmul(dy1, dx2));
  }

private boolean F_MiterPoint(p1, v1, p2, v2, i, c) DevCd p1, v1, p2, v2, *i, c;
{
register Fixed xi, yi;
Fixed len, costheta, slp, dx1dy2, dx2dy1, dx1dx2, dy1dy2;
DevCd v, m;
if (v1.x == 0 && v2.y == 0) {
  if (fpp7 < throwThreshold) return false;
  xi = p1.x; yi = p2.y; goto done; }
if (v2.x == 0 && v1.y == 0) {
  if (fpp7 < throwThreshold) return false;
  xi = p2.x; yi = p1.y; goto done; } 
F_VecAdd(v1, v2, v)
len = F_Dist(v.x, v.y);
if (len < f_throwThreshold) return false;
len = fixdiv(FixOne, len);
F_VecMul(v, len, v)
costheta = fixmul(v.x, v1.x) + fixmul(v.y, v1.y);
if (os_labs(costheta) < f_throwThreshold) return false;
if (v1.x == 0) { /* line1 vertical */
  xi = p1.x; yi = p2.y + muldiv(p1.x - p2.x, v2.y, v2.x, false); goto done; }
if (v2.x == 0) { /* line2 vertical */
  xi = p2.x; yi = p1.y + muldiv(p2.x - p1.x, v1.y, v1.x, false); goto done; }
if (v1.y == 0) { /* line1 horizontal */
  yi = p1.y; xi = p2.x + muldiv(p1.y - p2.y, v2.x, v2.y, false); goto done; }
if (v2.y == 0) { /* line2 horizontal */
  yi = p2.y; xi = p1.x + muldiv(p2.y - p1.y, v1.x, v1.y, false); goto done; }
dx1dy2 = fixmul(v1.x, v2.y);
dx2dy1 = fixmul(v2.x, v1.y);
dx1dx2 = fixmul(v1.x, v2.x);
dy1dy2 = fixmul(v1.y, v2.y);
slp = fixdiv(FixOne, dx1dy2 - dx2dy1);
xi  = fixmul(p1.y - p2.y, dx1dx2);
xi += fixmul(p2.x, dx1dy2);
xi -= fixmul(p1.x, dx2dy1);
xi  = fixmul(xi, slp);
yi  = fixmul(p2.x - p1.x, dy1dy2);
yi += fixmul(p1.y, dx1dy2);
yi -= fixmul(p2.y, dx2dy1);
yi  = fixmul(yi, slp);
done:
m.x = xi; m.y = yi;
if (normalize && f_ccw(c, p1, m) != f_ccw(c, m, p2)) return false;
i->x = xi; i->y = yi; return true;
}

private procedure F_FillLineSegment(p2, leftNorm, produceLine)
  DevCd p2, leftNorm; boolean produceLine;
{
DevCd lf2, rt2;
F_VecAdd(p2, leftNorm, lf2)
F_VecSub(p2, leftNorm, rt2)
if (produceLine) FastFillQuad(f_dLf, f_dRt, rt2, lf2);
f_dRt = rt2; f_dLf = lf2;
} /* end of F_FillLineSegment */


private procedure F_FillCircle(p) DevCd p; {
  Cd c;
  UnFixCd(p, &c);
  FillCircle(c, c);
  }

private procedure F_HalfRoundCap(lf, rt) DevCd lf, rt; {
  DevCd midpt, dp;
  if (needTfm) {
    Cd l, r;
    UnFixCd(lf, &l); UnFixCd(rt, &r);
    HalfRoundCap(l, r);
    return;
    }
  dp.x = (lf.x + rt.x) >> 1;
  dp.y = (lf.y + rt.y) >> 1;
  midpt.x = dp.x + (lf.y - dp.y);
  midpt.y = dp.y - (lf.x - dp.x);
  FastFillCurveJoin(lf, midpt, dp, 0, 0, 0);
  FastFillCurveJoin(midpt, rt, dp, 0, 0, 0);
  }

private procedure F_FillJoin(lf, rt, v)  DevCd lf, rt, v; {
  integer turn;
  DevCd miterv, temp;
  register integer tx, ty, px, py;
  if (f_halfwidth==0) return;
  if (!mitposs /* this is a join internal to a curve */ ||
      (normalize && gs->lineJoin == roundJoin)) {
    if (mitposs && /* use circle at horizontal-vertical join */
        ((f_dRt.x == f_dLf.x && rt.y == lf.y) ||
         (f_dRt.y == f_dLf.y && rt.x == lf.x))) {
      F_FillCircle(f_dP1); return; }
    turn = F_VecTurn(f_uV1, v);
    if (turn > 0) /* left turn */
      FastFillCurveJoin(f_dRt, rt, f_dP1, 0, 0, 0);
    else if (turn < 0) /* right turn */
      FastFillCurveJoin(f_dLf, lf, f_dP1, 0, 0, 0);
    else if (f_uV1.x != v.x) /* u-turn */
      F_HalfRoundCap(f_dLf, f_dRt);
    return;
    }
  if (gs->lineJoin == roundJoin) {
    F_FillCircle(f_dP1); return; }
  turn = F_VecTurn(f_uV1, v);
  if (turn != 0 && gs->lineJoin == miterJoin) {
    if (turn > 0) { /* left turn */
      if (!F_MiterPoint(f_dRt, f_uV1, rt, v, &temp, f_dP1)) goto Bevel;
      tx = FTrunc(temp.x); ty = FTrunc(temp.y);
      px = FTrunc(f_dRt.x); py = FTrunc(f_dRt.y);
      if (px==tx && py==ty) goto Bevel;
      px = FTrunc(rt.x); py = FTrunc(rt.y);
      if (px==tx && py==ty) goto Bevel;
      FastFillQuad(f_dRt, temp, rt, f_dP1);
      }
    else { /* right turn */
      if (!F_MiterPoint(f_dLf, f_uV1, lf, v, &temp, f_dP1)) goto Bevel;
      tx = FTrunc(temp.x); ty = FTrunc(temp.y);
      px = FTrunc(f_dLf.x); py = FTrunc(f_dLf.y);
      if (px==tx && py==ty) goto Bevel;
      px = FTrunc(lf.x); py = FTrunc(lf.y);
      if (px==tx && py==ty) goto Bevel;
      FastFillQuad(f_dLf, f_dP1, lf, temp);
      }
    return;
    }
 Bevel:
  if (turn > 0) FastFillBevel(f_dRt, rt, f_dP1, true);
  else FastFillBevel(f_dLf, lf, f_dP1, true);
  }  /* end of F_FillJoin */

private procedure FF_StrkStrt(dP) DevCd dP;
{
f_dP1St = f_dP1 = dP;
needVec = true; atcurve = false;
}  /* end of FF_StrkStrt */

private procedure F_StrkStrt(cP) Cd cP;
{
DevCd dP;
dP1St = cP;
FixCd(cP, &dP);
FF_StrkStrt(dP);
}

private procedure VecStrkTrp(yt, yb, xtl, xtr, xbl, xbr)
  register Fixed yt, yb; Fixed xtl, xtr, xbl, xbr; {
  Fixed xt, xb;
  if (xtl != xtr) { xt = xtl; xb = xtr; }
  else { xt = xtl; xb = xbl; }
  BresenhamMT(xt, yt, xb - xt, yb - yt);
  }

public procedure FF_Vector(dP2) DevCd dP2; {
BresenhamMT(f_dP1.x, f_dP1.y, dP2.x - f_dP1.x, dP2.y - f_dP1.y);
needVec = false;  f_dP1 = dP2;
} /* end of FF_Vector */

private procedure F_Vector(cP2) Cd cP2; {
  DevCd dP2;
  FixCd(cP2, &dP2);
  FF_Vector(dP2);
  }

private procedure F_MvStrk(p, p2) register DevCd *p; DevCd *p2; {
  }

private procedure FF_StrkLnTo(dP2)  DevCd dP2; {
Fixed len, r;
DevCd v, lf, rt, leftNorm, diff, dP2init;
boolean produceLine;
dP2init = dP2;
F_VecSub(dP2, f_dP1, v)
if (atcurve && os_labs(v.x) < 0x290 && os_labs(v.y) < 0x290) return;
if (normalize) F_NormPair(&v, &dP2);
if (strkTstRct &&
    ((dP2.x > strkur.x && f_dP1.x > strkur.x) ||
     (dP2.y > strkur.y && f_dP1.y > strkur.y) ||
     (dP2.x < strkll.x && f_dP1.x < strkll.x) ||
     (dP2.y < strkll.y && f_dP1.y < strkll.y))) {
  produceLine = false;
  }
else produceLine = true;
if (v.x==0)
  {
  if (v.y==0) return;
  else if (v.y > 0) {len = v.y; v.y = FixOne;}
  else {len = -v.y; v.y = -FixOne;}
  }
else if (v.y==0)
  {if (v.x > 0) {len = v.x; v.x = FixOne;}
   else {len = -v.x; v.x = -FixOne;}}
else
  {
  len = F_Dist(v.x, v.y);
  if (len==0) return;
  r = fixdiv(FixOne,len);
  F_VecMul(v, r, v)
  }
leftNorm.x = -v.y;  leftNorm.y = v.x;
r = F_AdjstHW(leftNorm);
F_VecMul(leftNorm, r, leftNorm)
F_VecAdd(f_dP1, leftNorm, lf)
F_VecSub(f_dP1, leftNorm, rt)
if (needVec) {f_dRtSt = rt; f_dLfSt = lf; f_uV2St = v;}
else if (produceLine) F_FillJoin(lf, rt, v);
f_dRt = rt;  f_dLf = lf;
F_FillLineSegment(dP2, leftNorm, produceLine);
needVec = false;  f_dP1 = dP2init;  f_uV1 = v;
if (incurve) mitposs = false;
atcurve = false;
} /* end of FF_StrkLnTo */

private procedure F_StrkLnTo(cP2)  Cd cP2; {
DevCd dP2;
FixCd(cP2, &dP2);
FF_StrkLnTo(dP2);
}

private procedure F_StrkCap()
{
DevCd lf, rt, dp, tenonv;
if (f_halfwidth==0) return;
switch (gs->lineCap)
  {
  case buttCap: return;
  case roundCap:
    if (!needVec) {
      if (normalize && f_dLf.x != f_dRt.x && f_dLf.y != f_dRt.y)
        F_HalfRoundCap(f_dLf, f_dRt);
      else {
	if (!normalize) dp = f_dP1;
	else {
	  dp.x = (f_dLf.x + f_dRt.x) >> 1;
	  dp.y = (f_dLf.y + f_dRt.y) >> 1;
	  }
	F_FillCircle(dp);
	}
      }
    if (!needVec && normalize &&
        f_dRtSt.x != f_dLfSt.x && f_dRtSt.y != f_dLfSt.y)
      F_HalfRoundCap(f_dRtSt, f_dLfSt);
    else {
      if (needVec || !normalize) dp =  f_dP1St;
      else {
        dp.x = (f_dLfSt.x + f_dRtSt.x) >> 1;
	dp.y = (f_dLfSt.y + f_dRtSt.y) >> 1;
        }
      F_FillCircle(dp);
      }
    return;
  case tenonCap: {
    Fixed len;
    DevCd diff;
    if (needVec) return;
    if (!normalize) len = f_halfwidth;
    else {
      F_VecSub(f_dLf, f_dRt, diff)
      len = F_Dist(diff.x, diff.y) >> 1;
      }
    F_VecMul(f_uV1, len, tenonv)
    F_VecAdd(f_dLf, tenonv, lf)
    F_VecAdd(f_dRt, tenonv, rt)
    FastFillQuad(f_dRt, rt, lf, f_dLf);
    if (!normalize) len = f_halfwidth;
    else {
      F_VecSub(f_dLfSt, f_dRtSt, diff)
      len = F_Dist(diff.x, diff.y) >> 1;
      }
    F_VecMul(f_uV2St, len, tenonv)
    F_VecSub(f_dLfSt, tenonv, lf)
    F_VecSub(f_dRtSt, tenonv, rt)
    FastFillQuad(f_dRtSt, f_dLfSt, lf, rt);
    }
  }
} /* end of F_StrkCap */


private procedure F_VecClose()
{
if (!needVec) FF_Vector(f_dP1St);
}

private procedure F_StrkClose()
{
if (needVec) {
  if (gs->lineCap == roundCap) F_FillCircle(f_dP1St);
  return;}
FF_StrkLnTo(f_dP1St);
F_FillJoin(f_dLfSt, f_dRtSt, f_uV2St);
} /* end of F_StrkClose */


/* end of Fixed versions */

private procedure StrkCurve(start)  boolean start; {
if (gs->flatEps < fpTwo) {incurve = start; mitposs = true; atcurve = !start;}
}

private boolean CheckForRectangle(c1, c2) Cd c1, c2; {
DevCd dc1, dc2, diff;
register Fixed yt, yb, xtl, xtr, xbl, xbr;
Fixed xdif, ydif, fixedhalfwidth;
boolean vert, increase;
FixCd(c1, &dc1);
FixCd(c2, &dc2);
F_VecSub(dc2, dc1, diff)
if (diff.x == 0 && diff.y == 0) return false;
if (os_labs(diff.x) <= 1000) vert = true;
else {if (os_labs(diff.y) <= 1000) vert = false; else return false;}
if (normalize) {f_dP1 = dc1; F_NormPair(&diff, &dc2); dc1 = f_dP1;}
fixedhalfwidth = pflttofix(&uhalfwidth);
if (vert)
  {
  if ((increase = (boolean)(dc2.y > dc1.y)))
       {yt = dc2.y; yb = dc1.y;}
  else {yt = dc1.y; yb = dc2.y;}
  xtl = xbl = dc1.x - fixedhalfwidth;  xtr = xbr = dc1.x + fixedhalfwidth;
  }
else
  {
  if ((increase = (boolean)(dc2.x > dc1.x)))
       {xtr = dc2.x; xtl = dc1.x;}
  else {xtr = dc1.x; xtl = dc2.x;}
  yt = dc1.y + fixedhalfwidth;  yb = dc1.y - fixedhalfwidth;
  xbr = xtr;  xbl = xtl;
  }
if (!dashed)
  {
  (*StrkTrp)(yt, yb, xtl, xtr, xbl, xbr);
  }
else
  {
  integer i;
  Fixed fxdcdl, fxdvlen, fxdlim, fxdprev, fxdnext, fxddshlen[DASHLIMIT];
  for (i = 0; i < dashLim; i++)
    {
    if (dashLength[i] > 32000.0) return false;
    fxddshlen[i] = pflttofix(dashLength + i);
    }
  fxdcdl = pflttofix(&fDshLen);
  if (vert)
    {
    fxdvlen = yt - yb;
    if (increase) {fxdlim = yt;  fxdprev = yb;}
    else {fxdlim = yb;  fxdprev = yt;}
    }
  else
    {
    fxdvlen = xtr - xtl;
    if (increase) {fxdlim = xtr;  fxdprev = xtl;}
    else {fxdlim = xtl;  fxdprev = xtr;}
    }
  crDash = fcrDash;  filledDash = ffldDsh;
  while (fxdvlen >= fxdcdl)
    {
    fxdvlen -= fxdcdl;
    if (increase) fxdnext = fxdprev + fxdcdl;
    else fxdnext = fxdprev - fxdcdl;
    if (filledDash)
      {
      if (vert)
        {
        if (increase) (*StrkTrp)(fxdnext, fxdprev, xtl, xtr, xbl, xbr);
        else (*StrkTrp)(fxdprev, fxdnext, xtl, xtr, xbl, xbr);
        }
      else
        {
        if (increase)
          (*StrkTrp)(yt, yb, fxdprev, fxdnext, fxdprev, fxdnext);
        else (*StrkTrp)(yt, yb, fxdnext, fxdprev, fxdnext, fxdprev);
        }
      }
    fxdprev = fxdnext;
    if (++crDash == dashLim) crDash = 0;
    fxdcdl = fxddshlen[crDash];
    filledDash = !filledDash;
    }
  if (filledDash)
    {
    if (vert)
      {
      if (increase) (*StrkTrp)(fxdlim, fxdprev, xtl, xtr, xbl, xbr);
      else (*StrkTrp)(fxdprev, fxdlim, xtl, xtr, xbl, xbr);
      }
    else
      {
      if (increase) (*StrkTrp)(yt, yb, fxdprev, fxdlim, fxdprev, fxdlim);
      else (*StrkTrp)(yt, yb, fxdlim, fxdprev, fxdlim, fxdprev);
      }
    }
  }
return true;
} /* end of CheckForRectangle */

private procedure SetNormMidPts() {
  real r = uhalfwidth * fpTwo;
  integer wi = r;
  if ((wi & 1) != 0) /* odd width */
    oddXwidth = oddYwidth = true;
  else /* even width */
    oddXwidth = oddYwidth = false;
  }

private procedure NormalizeStrokeWidth(w) Preal w; {
  Cd c;
  real len, r, q;
  c.x = *w; c.y = 0; DTfmP(c, &c); /* c is the device width vector */
  r = len = Dist(c); /* len is device length of the width vector */
  RRoundP(&len, &r);  /* r is rounded device length */
  r -= fpOne; if (RealLt0(r)) r = fpZero;
  q = r / len; /* q is the normalization factor */
  c.x *= q; c.y *= q; IDTfmP(c, &c); gs->strokeWidth = Dist(c);
  uhalfwidth = r / fpTwo;
  SetNormMidPts();
  }

private Cd GetNormStrkWdth(m, w, c)
  PMtx m; Preal w; Cd c; {
  real r, q;
  r = Dist(c); q = os_fabs(*w) / r;
  c.x *= q; c.y *= q; /* c is now width in user space */
  DTfmP(c, &c); /* c is the device width vector */
  return c;
  }

private boolean OddWidth(w, q) Preal w, q; {
  integer wi;
  real r;
  r = (*w) * (*q);
  RRoundP(&r, &r);
  wi = r;
  return (wi & 1) != 0; /* return true if the width is odd */
  }

private procedure AnamorphicNormStrkWdth(w) Preal w; {
  Cd c;
  Mtx m;
  real r, q, horizw, vertw, wmax;
  integer wi;
  CrMtx(&m);
  /* find width of line that is horizontal in device space */
  c.x = m.b; c.y = m.d; /* user space normal */
  c = GetNormStrkWdth(&m, w, c);
  horizw = os_fabs(c.y);
  /* find width of line that is vertical in device space */
  c.x = m.a; c.y = m.c; /* user space normal */
  c = GetNormStrkWdth(&m, w, c);
  vertw = os_fabs(c.x);
  wmax = (horizw > vertw) ? horizw : vertw;
  RRoundP(&wmax, &r);
  r -= fpOne; if (RealLt0(r)) r = fpZero;
  q = r / wmax;
  gs->strokeWidth = (*w) * q;
  uhalfwidth = r / fpTwo;
  oddYwidth = OddWidth(&horizw, &q);
  oddXwidth = OddWidth(&vertw, &q);
  }

private procedure SetHalfWidth()
{ /* sets needTfm and uhalfwidth.  */
Mtx matrix;
real dif;
if (RealNe0(gs->devhlw))
  {
  needTfm = false;
  uhalfwidth = gs->devhlw;
  if (gs->strokeAdjust) SetNormMidPts();
  return;
  }
if (RealEq0(gs->lineWidth) && !dashed) {
  needTfm = false; uhalfwidth = fpZero; gs->strokeWidth = fpZero;
  return;}
CrMtx(&matrix);
needTfm = true;
dif = matrix.b + matrix.c;
if (os_fabs(dif) <= fpp001)
  {dif = matrix.a - matrix.d; needTfm = (boolean)(os_fabs(dif) > fpp001);}
if (needTfm)
  {
  dif = matrix.b - matrix.c;
  if (os_fabs(dif) <= fpp001)
    {dif = matrix.a + matrix.d; needTfm = (boolean)(os_fabs(dif) >fpp001);}
  }
if (gs->strokeAdjust) {
  if (CheckForAnamorphicMatrix(&gs->matrix))
    AnamorphicNormStrkWdth(&gs->lineWidth);
  else
    NormalizeStrokeWidth(&gs->lineWidth);
  if (needTfm) uhalfwidth = gs->strokeWidth / fpTwo;
  else gs->devhlw = uhalfwidth;
  }
else {
  gs->strokeWidth = gs->lineWidth;
  uhalfwidth = gs->strokeWidth / fpTwo;
  if (!needTfm) gs->devhlw = uhalfwidth = LengthTfm(uhalfwidth, true);
  }
if (needTfm) MtxInvert(&gs->matrix, &curIMtx); 
}  /* end of SetHalfWidth */

private procedure SetupForCircles() {
  real xw, yw;
  integer i;
  register PGState g = gs;
  if (uhalfwidth == circ_uhalfwidth &&
      g->matrix.a == circ_a && g->matrix.b == circ_b &&
      g->matrix.c == circ_c && g->matrix.d == circ_d) return;
  circ_uhalfwidth = uhalfwidth;
  circ_a = g->matrix.a; circ_b = g->matrix.b;
  circ_c = g->matrix.c; circ_d = g->matrix.d;
  circ_cannot = false; circ_size = 0;
  if (RealNe0(g->devhlw)) xw = yw = g->devhlw;
  else {
    xw = uhalfwidth * os_sqrt(circ_a * circ_a + circ_c * circ_c);
    yw = uhalfwidth * os_sqrt(circ_b * circ_b + circ_d * circ_d);
    }
  xLowCirCenter = xHiCirCenter = xw - os_floor(xw);
  if (xLowCirCenter < fpHalf) xHiCirCenter = fpOne - xLowCirCenter;
  else xLowCirCenter = fpOne - xHiCirCenter;
  yLowCirCenter = yHiCirCenter = yw - os_floor(yw);
  if (yLowCirCenter < fpHalf) yHiCirCenter = fpOne - yLowCirCenter;
  else yLowCirCenter = fpOne - yHiCirCenter;
  for (i = 0; i < 4; i++) {
    if (CircleCache[i].devprim) {
      DisposeDevPrim(CircleCache[i].devprim);
      CircleCache[i].devprim = NULL;
      }
    }
  circ_maskID = -1; /* force discard */
  }

private procedure PreStroke() {
  register PGState g = gs;
  dashed = (boolean)(g->dashArray.length != 0); /* Must precede SetHalfWidth */
  doingVectors = RealEq0(g->lineWidth);
  if (!doingVectors) {
    SetHalfWidth();
    if (RealEq0(uhalfwidth)) doingVectors = true;
    }
  normalize = !doingVectors && g->strokeAdjust;
  }

public boolean FlushStrokeCircle(mask) PMask mask; {
  int i;
  for (i = 0; i < 4; i++) {
    if (CircleCache[i].mask == mask) {
      CircleCache[i].flushed = true;
      return true;
      }
    }
  return false;
}

public procedure PreCacheTrapCircles() {
  Cd c;
  PreStroke();
  if (doingVectors) return;
  SetupForCircles();
  c.x = c.y = fpZero; MkCirc(c);
  c.x = fpHalf; MkCirc(c);
  c.y = fpHalf; MkCirc(c);
  c.x = fpZero; MkCirc(c);
  }

public boolean DoStroke(
  isSP, context, bbox, rectangle, c1, c2, enumerate, qEnumOk, qenumerate,
  circletraps, devClipBBox, devClipIsRect, devClipDevBBox)
char *context; /* either a path or a userpath context */
BBox bbox, devClipBBox;
DevBBox devClipDevBBox;
boolean isSP, rectangle, devClipIsRect, (*qEnumOk)();
Cd c1, c2;
procedure (*enumerate)(), (*qenumerate)(); {
real xClipDelta, yClipDelta;
boolean doCircle, solidVectors;
integer i;
register PGState g = gs;

isStrkPth = isSP;
circleTraps = circletraps;
PreStroke();
solidVectors = doingVectors && !dashed;
gPathBBox = *bbox;
if (doingVectors) {
  doCircle = needTfm = false;
  uhalfwidth = fpZero; xClipDelta = yClipDelta = fpp001;
  }
else {
  if (MAXStrokeThrow != g->miterlimit)
    {MAXStrokeThrow = g->miterlimit;  throwThreshold = fpOne / MAXStrokeThrow;}
  doCircle = (boolean)((g->lineJoin == roundJoin || g->lineCap == roundCap)
             && RealNe0(uhalfwidth) && !isSP);
  if (needTfm)
    {
    Cd v;  real dist;
    v.x = fpOne; v.y = fpZero;  DTfmPCd(v, &curIMtx, &v);
    xClipDelta = MAXStrokeThrow * uhalfwidth;  xClipDelta /= Dist(v);
    v.x = fpZero; v.y = fpOne;  DTfmPCd(v, &curIMtx, &v);
    yClipDelta = MAXStrokeThrow * uhalfwidth;  yClipDelta /= Dist(v);
    }
  else xClipDelta = yClipDelta = MAXStrokeThrow * uhalfwidth;
  if (normalize) { xClipDelta += fpOne; yClipDelta += fpOne; }
  gPathBBox.bl.x -= xClipDelta;
  gPathBBox.bl.y -= yClipDelta;
  needMaxBevelChord = true;
  }
gPathBBox.tr.x += xClipDelta;
gPathBBox.tr.y += yClipDelta;
 if (isStrkPth) {
   needClip = false;
 }
 else {
   clipBBox = devClipBBox;
   ms->bbCompMark = BBCompare(&gPathBBox, clipBBox);
   switch (ms->bbCompMark) {
     case outside: return false;
     case inside: if (devClipIsRect) { needClip = false; break; }
     case overlap: needClip = true;
     }
   (*ms->procs->initMark)(ms, needClip);
 }
#ifdef XA
allSegmentsIn = (boolean)
  (gPathBBox.bl.x > XA_MIN && gPathBBox.bl.y > XA_MIN &&
   gPathBBox.tr.x < XA_MAX && gPathBBox.tr.y < XA_MAX);
#else XA
allSegmentsIn = (boolean)
  (gPathBBox.bl.x > -fp16k && gPathBBox.bl.y > -fp16k &&
   gPathBBox.tr.x < fp16k && gPathBBox.tr.y < fp16k);
#endif XA
haveBounds = !circletraps &&
             allSegmentsIn && 
             xClipDelta < fp50 &&
	     yClipDelta < fp50;
if (doCircle) {
  SetupForCircles();
  if (circ_maskID != g->device->maskID) {
    for (i = 0; i < 4; i++) {
      if (CircleCache[i].mask) {
        FreeStrokeCircle(CircleCache[i].mask, CircleCache[i].flushed);
        CircleCache[i].mask = NULL;
        CircleCache[i].flushed = false;
        }
      }
    circ_maskID = g->device->maskID;
    }
  }
scip = circleMasks;
if (dashed) {
  real unitlength, dlen;
  boolean allzero;
  AryObj dao;  Object ob;
  allzero = true;  dao = g->dashArray;
  dashLim = dao.length;
  if (!needTfm) {
    Cd v; v.x = fpOne; v.y = fpZero; DTfmP(v, &v); unitlength = Dist(v); }
  for (i = 0; i < dashLim; i++)
    {
    VMCarCdr(&dao, &ob);  PRealValue(ob, &dlen);
    if (RealLt0(dlen)) RangeCheck();
    if (RealGt0(dlen)) allzero = false;
    dashLength[i] = (needTfm) ? dlen : dlen * unitlength;
    }
  if (allzero) RangeCheck();
  if (!needTfm) fDshLen = g->dashOffset * unitlength;
  else fDshLen = g->dashOffset;
  if (RealGe0(fDshLen))
    {
    fcrDash = 0;  ffldDsh = true;
    while (true)
      {
      fDshLen -= dashLength[fcrDash];
      if (RealLe0(fDshLen)) break;
      ffldDsh = !ffldDsh;
      if (++fcrDash == dashLim) fcrDash = 0;
      }
    fDshLen = -fDshLen;
    }
  else
    {
    fcrDash = dashLim - 1;  ffldDsh = false;
    while (true)
      {
      fDshLen += dashLength[fcrDash];
      if (RealGe0(fDshLen)) break;
      ffldDsh = !ffldDsh;
      if (--fcrDash < 0) fcrDash = dashLim - 1;
      }
    fDshLen = dashLength[fcrDash] - fDshLen;
    }
  }
incurve = false; mitposs = true;
if (doingVectors) StrkTrp = VecStrkTrp;
else StrkTrp = AddTrap;
if (haveBounds && !isStrkPth) SetTrapBounds(&gPathBBox);
if (!solidVectors && rectangle &&
    !needTfm && allSegmentsIn && !isStrkPth &&
    g->lineCap == buttCap && CheckForRectangle(c1, c2)) 
    goto doneWithStroke;
if (dashed || ms->bbCompMark != overlap || !allSegmentsIn || isStrkPth)
  strkTstRct = false;
else { /* avoid flattening curves that cannot overlap clipbb */
  Fixed xmrgn, ymrgn;
  if (devClipDevBBox == NULL) devClipDevBBox = GetDevClipDevBBox();
  strkTstRct = true;
  if (doingVectors) xmrgn = ymrgn = FixOne;
  else {
    xmrgn = pflttofix(&xClipDelta) + FixOne;
    ymrgn = pflttofix(&yClipDelta) + FixOne;
    }
  strkll.x = devClipDevBBox->bl.x - xmrgn;
  strkll.y = devClipDevBBox->bl.y - ymrgn;
  strkur.x = devClipDevBBox->tr.x + xmrgn;
  strkur.y = devClipDevBBox->tr.y + ymrgn;
  }
f_halfwidth = pflttofix(&uhalfwidth);
if (!allSegmentsIn || needTfm || dashed || isStrkPth ||
    (!doingVectors && !RealEq0(strkFoo)))
  (*enumerate)(context, StrkStrt, StrkLnTo, StrkClose, StrkCap,
          StrkCurve, true, g->flatEps, false, strkTstRct, strkll, strkur);
else if (doingVectors) {
  if (qEnumOk != NULL && (*qEnumOk)(context))
    (*qenumerate)(context, FF_StrkStrt, FF_Vector, F_VecClose, NoOp,
            NoOp, true, g->flatEps, strkTstRct, strkll, strkur);
  else
    (*enumerate)(context, FF_StrkStrt, FF_Vector, F_VecClose, NoOp,
            NoOp, true, g->flatEps, true, strkTstRct, strkll, strkur);
  }
else {
  f_throwThreshold = pflttofix(&throwThreshold);
  if (qEnumOk != NULL && (*qEnumOk)(context))
    (*qenumerate)(context, FF_StrkStrt, FF_StrkLnTo, F_StrkClose, F_StrkCap,
            StrkCurve, true, g->flatEps, strkTstRct, strkll, strkur);
  else
    (*enumerate)(context, FF_StrkStrt, FF_StrkLnTo, F_StrkClose, F_StrkCap,
            StrkCurve, true, g->flatEps, true, strkTstRct, strkll, strkur);
  }
doneWithStroke:
return true;
}

public procedure FinStroke() {
  if (scip > circleMasks)
    (*ms->procs->strokeMasksFilled)(circleMasks, scip - circleMasks,
                   circ_llx, circ_lly, circ_urx, circ_ury);
  }
  /* this is called after TermMark so that traps are marked before circles */
  /* strictly for appearance -- do not want dots before lines */

public Path StrkPth(p) PPath p;
{
InitPath(&strokePath);
strokePath.secret = p->secret;
DURING
StrkInternal(p, true);
HANDLER
  {FrPth(&strokePath);  RERAISE;}
END_HANDLER;
return strokePath;
}  /* end of StrkPth */


public procedure IniStroke(reason)  InitReason reason;
{
integer i;
switch (reason)
  {
  case init:
#if (OS == os_mpw)
    strokeGlobals = (Globals)os_sureCalloc(sizeof(GlobalsRec),1);
#endif (OS == os_mpw)
    MAXStrokeThrow = fpZero;
    strkFoo = fpZero;
    CircleCache = (CircleRec *)os_calloc(4, sizeof(CircleRec));
    circleMasks = (DevMask *)os_calloc(sizeCircleMasks, sizeof(DevMask));
    endCircleMasks = circleMasks + sizeCircleMasks;
    qc = (QdCorner *)os_malloc(4*sizeof(QdCorner));
    qc[0].ptr1 = qc[2].ptr2 = &qc[1];
    qc[1].ptr1 = qc[3].ptr2 = &qc[2];
    qc[2].ptr1 = qc[0].ptr2 = &qc[3];
    qc[3].ptr1 = qc[1].ptr2 = &qc[0];
    FracSqrt2 = fracsqrt(FracOne + (FracOne - 1));
    prevRadiusForMaxBevelChord = -fpOne;
    for (i = 0; i < 4; i++) {
      CircleCache[i].devprim = NULL;
      CircleCache[i].mask = NULL;
      }
    circ_uhalfwidth = -fpOne; /* bogus value to force initial miss */
    circ_maxsize = (800 * sizeof(DevTrap));
    break;
  case romreg:
    break;
  }
} /* end of IniStroke */


