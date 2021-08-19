/*
  pathreduce.c

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
Doug Brotz: Thu Dec 11 17:23:20 1986
Chuck Geschke: Thu Oct 31 15:22:56 1985
Ed Taft: Sat Jul 11 11:31:22 1987
John Gaffney: Tue Feb 12 10:46:11 1985
Ken Lent: Wed Mar 12 15:53:25 1986
Bill Paxton: Mon Sep 12 15:00:20 1988
Don Andrews: Wed Sep 17 15:28:32 1986
Mike Schuster: Wed Jun 17 11:39:01 1987
Ivor Durham: Thu Sep 29 11:27:34 1988
Jim Sandman: Wed Dec 13 09:40:36 1989
Linda Gass: Tue Dec  8 12:14:11 1987
Paul Rovner: Thursday, October 8, 1987 9:40:40 PM
Joe Pasqua: Tue Jan 17 13:41:08 1989
Jack 09Nov87 new ReduceQuadPath, FillRect, use MakeBounds to fix BBox probs
Jack 20Nov87 big reorg of AddTrap code
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

#include "graphdata.h"
#include "path.h"
#include "reducer.h"
#include "stroke.h"
#include "graphicspriv.h" 

extern procedure NoOp();
extern procedure QFNewPoint();
extern procedure AddTrap(), AddRunMark();
extern Fixed DevToRdc();
extern procedure ClNewPt();
extern procedure RdcClose();
extern procedure QNewPoint(), QRdcClose(), QReduce(), QResetReducer();
extern procedure StdInitMark();

private Cd rclff, rclfl, rctpf, rctpl, rcrtf, rcrtl, rcbtf, rcbtl;
private boolean rclffirst, rctpfirst, rcrtfirst, rcbtfirst;
private boolean rclffin, rclflin, rctpfin, rctplin, rcrtfin, rcrtlin,
                rcbtfin, rcbtlin;
private real rccliplf, rccliptp, rccliprt, rcclipbt;
private procedure (*rcclnewpt)();
private procedure (*rcrdcclose)();
private DevPrim *rdcStrkMasks;
private PPath addPath;

private DevCd pathOffset;

private procedure AddTrapezoidToPath(yt, yb, xtl, xtr, xbl, xbr, path)
    Component yt, yb, xtl, xtr, xbl, xbr; PPath path;
{
Cd c;
c.x = xbl; c.y = yb;  MoveTo(c, path);
c.x = xbr;            LineTo(c, path);
c.x = xtr; c.y = yt;  LineTo(c, path);
c.x = xtl;            LineTo(c, path);
ClosePath(path);
} /* end of AddTrapezoidToPath */

private procedure CallAddTrapToPath(yt, yb, xtl, xtr, xbl, xbr)
  Fixed yt, yb, xtl, xtr, xbl, xbr; {
#if DPSXA
  BBoxRec trapBBox;
  	yt = RdcToDev(yt);
  	yb = RdcToDev(yb);
  	xtl = RdcToDev(xtl);
  	xtr = RdcToDev(xtr);
  	xbl = RdcToDev(xbl);
  	xbr = RdcToDev(xbr);
    fixtopflt(MIN(xtl,xbl), &trapBBox.bl.x);
    fixtopflt(yb,&trapBBox.bl.y);
    fixtopflt(MAX(xtr,xbr), &trapBBox.tr.x);
    fixtopflt(yt,&trapBBox.tr.y);
/*
  Test bounding boxes to see if trap is outside of chunk area (degenerate trap
  not caught by the rectangular clipper) - overlapping traps are still true
  representations of the path.
*/
    if(BBCompare(&trapBBox, &chunkBBox) == outside)
    	return;	
  AddTrapezoidToPath(
    fixtodbl(yt)+(double)pathOffset.y, fixtodbl(yb)+(double)pathOffset.y,
    fixtodbl(xtl)+(double)pathOffset.x, fixtodbl(xtr)+(double)pathOffset.x,
    fixtodbl(xbl)+(double)pathOffset.x, fixtodbl(xbr)+(double)pathOffset.x,
    addPath);
#else /* DPSXA */
  AddTrapezoidToPath(
    fixtodbl(RdcToDev(yt)), fixtodbl(RdcToDev(yb)), fixtodbl(RdcToDev(xtl)),
    fixtodbl(RdcToDev(xtr)), fixtodbl(RdcToDev(xbl)), fixtodbl(RdcToDev(xbr)),
    addPath);
#endif /* DPSXA */
  }  /* end of CallAddTrapToPath */

private procedure GetDevBBox(bbox) register BBox bbox; {
  DevLBounds bounds;
  (*gs->device->procs->DefaultBounds)(gs->device, &bounds);
  bbox->bl.x = bounds.x.l;
  bbox->bl.y = bounds.y.l;
  bbox->tr.x = bounds.x.g;
  bbox->tr.y = bounds.y.g;
  }

private procedure Get16KBBox(bbox) register BBox bbox; {
  bbox->bl.x = -fp16k;
  bbox->bl.y = -fp16k;
  bbox->tr.x =  fp16k;
  bbox->tr.y =  fp16k;
  }

public procedure SetUpForRectClip(clnewpt, rdcclose, bbox)
  procedure (*clnewpt)();
  procedure (*rdcclose)();
  BBox bbox;
  {
  rcclnewpt = clnewpt;
  rcrdcclose = rdcclose;
  rclffirst = rctpfirst = rcrtfirst = rcbtfirst = true;
  rccliplf = bbox->bl.x - fp8;
  rccliptp = bbox->tr.y + fp8;
  rccliprt = bbox->tr.x + fp8;
  rcclipbt = bbox->bl.y - fp8;
  } /* end of SetUpForRectClip */

public procedure ConvertToListPath(path) register PPath path; {
  DevCd nullCD;
  if (path == NULL || (PathType)path->type == listPth) return;
  if ((PathType)path->type == intersectPth) {
    IntersectPath *ip;
    boolean evenOdd;
    BBoxRec rdcBBox;
    integer maxval, minval, x, y;
    ip = path->ptr.ip;
    evenOdd = ip->evenOdd;
    ConvertToListPath(&ip->path);
    ConvertToListPath(&ip->clip);
#if DPSXA
    {
	Cd xydelta, delta;
	short i,j,first;
	BBoxCompareResult pathComp, clipComp;
	boolean clipping;
	first = true;
	rdcBBox = chunkBBox;
	pathOffset.x = pathOffset.y = 0;
	xydelta.x = -xChunkOffset;
	xydelta.y = -yChunkOffset;
	delta.x = xydelta.x;
	delta.y = 0;
	for(i=0; i<maxYChunk; i++) {
		for(j=0; j<=maxXChunk; j++) {
			if(((pathComp = BBCompare(&ip->path.bbox, &chunkBBox)) != outside)
			  && ((clipComp = BBCompare(&ip->clip.bbox, &chunkBBox)) != outside)) {
    			x = MAX(ip->path.bbox.tr.x, ip->clip.bbox.tr.x);
    			x = MIN(x, rdcBBox.tr.x);
    			y = MAX(ip->path.bbox.tr.y, ip->clip.bbox.tr.y);
    			y = MIN(y, rdcBBox.tr.y);
    			maxval = MAX(x, y);
    			x = MIN(ip->path.bbox.bl.x, ip->clip.bbox.bl.x);
    			x = MAX(x, rdcBBox.bl.x);
    			y = MIN(ip->path.bbox.bl.y, ip->clip.bbox.bl.y);
    			y = MAX(y, rdcBBox.bl.y);
    			minval = MIN(x, y);
    			SetRdcScal(maxval, minval);
    			ResetReducer();
    			RdcClip(false);
				clipping = false;
      			SetUpForRectClip(ClNewPt, RdcClose, &rdcBBox);
      			FeedPathToReducer(&ip->path, RCNextPt, RCLastPt,
                        false, false, nullCD, nullCD);
    			RdcClip(true);
				clipping = true;
    			FeedPathToReducer(&ip->clip, ClNewPt, RdcClose,
                      false, false, nullCD, nullCD);
		        if(first) {
    				FrPth(path);
					first = false;
				}
    			addPath = path;
    			Reduce(CallAddTrapToPath, clipping, evenOdd);
    		}
			pathOffset.x += xChunkOffset;
			TlatPath(&ip->path,delta);
			TlatPath(&ip->clip,delta);
		}
		pathOffset.x = 0;
		pathOffset.y += yChunkOffset;
		delta.x = (maxXChunk+1) * -xydelta.x;
		delta.y = xydelta.y;
		TlatPath(&ip->path,delta);
		TlatPath(&ip->clip,delta);
		delta.x = xydelta.x;
		delta.y = 0;
	}
	delta.x = 0;
	delta.y = maxYChunk * -xydelta.y;
	TlatPath(&ip->path,delta);
	TlatPath(&ip->clip,delta);
  	/* reset offset */
	pathOffset.x = 0;
	pathOffset.y = 0;
   }
#else /* DPSXA */
    Get16KBBox(&rdcBBox);
    x = MAX(ip->path.bbox.tr.x, ip->clip.bbox.tr.x);
    x = MIN(x, rdcBBox.tr.x);
    y = MAX(ip->path.bbox.tr.y, ip->clip.bbox.tr.y);
    y = MIN(y, rdcBBox.tr.y);
    maxval = MAX(x, y);
    x = MIN(ip->path.bbox.bl.x, ip->clip.bbox.bl.x);
    x = MAX(x, rdcBBox.bl.x);
    y = MIN(ip->path.bbox.bl.y, ip->clip.bbox.bl.y);
    y = MAX(y, rdcBBox.bl.y);
    minval = MIN(x, y);
    SetRdcScal(maxval, minval);
    ResetReducer();
    RdcClip(false);
    if (BBCompare(&ip->path.bbox, &rdcBBox) == inside)
      FeedPathToReducer(&ip->path, ClNewPt, RdcClose,
                        false, false, nullCD, nullCD);
    else {
      SetUpForRectClip(ClNewPt, RdcClose, &rdcBBox);
      FeedPathToReducer(&ip->path, RCNextPt, RCLastPt,
                        false, false, nullCD, nullCD);
      }
    RdcClip(true);
    FeedPathToReducer(&ip->clip, ClNewPt, RdcClose,
                      false, false, nullCD, nullCD);
    FrPth(path);
    addPath = path;
    Reduce(CallAddTrapToPath, true, evenOdd);
#endif /* DPSXA */
    }
  else if ((PathType)path->type == quadPth) {
    QuadPath *qp;
    Cd c0, c1, c2, c3;
    boolean checkedForRect = path->checkedForRect;
    boolean isRect = path->isRect;
    qp = path->ptr.qp;
    c0 = qp->c0; c1 = qp->c1; c2 = qp->c2; c3 = qp->c3;
    FrPth(path);
    MoveTo(c0, path);
    LineTo(c1, path);
    LineTo(c2, path);
    LineTo(c3, path);
    ClosePath(path);
    path->checkedForRect = checkedForRect;
    path->isRect = isRect;
    }
  else if ((PathType)path->type == strokePth) {
    register StrkPath *spp;
    register PGState g;
    Path sp;
    spp = path->ptr.sp;
    GSave();
    g = gs;
    g->matrix = spp->matrix;
    g->lineWidth = spp->lineWidth;
    g->miterlimit = spp->miterlimit;
    g->devhlw = spp->devhlw;
    g->dashArray = spp->dashArray;
    g->dashOffset = spp->dashOffset;
    g->flatEps = spp->flatEps;
    g->lineCap = spp->lineCap;
    g->lineJoin = spp->lineJoin;
    g->strokeAdjust = spp->strokeAdjust;
    g->circleAdjust = spp->circleAdjust;
    g->strokeWidth = spp->strokeWidth;
    sp = StrkPth(&spp->path);
    GRstr();
    FrPth(path);
    *path = sp;
    }
  else CantHappen();
  }

private procedure RCBotNew(coord)  Cd coord;
{
boolean cin = (boolean)(coord.y >= rcclipbt);
Cd temp;
if (rcbtfirst) {rcbtf = coord; rcbtfin = cin;}
else if (rcbtlin != cin)
  {
  temp.y = rcclipbt;
  temp.x = rcbtl.x + (coord.x - rcbtl.x) * (rcclipbt - rcbtl.y)
                    / (coord.y - rcbtl.y);
  (*rcclnewpt)(temp);
  }
rcbtl = coord;  rcbtlin = cin;  rcbtfirst = false;
if (cin) (*rcclnewpt)(coord);
} /* end of RCBotNew */


private procedure RCRtNew(coord)  Cd coord;
{
boolean cin = (boolean)(coord.x <= rccliprt);
Cd temp;
if (rcrtfirst) {rcrtf = coord; rcrtfin = cin;}
else if (rcrtlin != cin)
  {
  temp.x = rccliprt;
  temp.y = rcrtl.y + (coord.y - rcrtl.y) * (rccliprt - rcrtl.x)
                    / (coord.x - rcrtl.x);
  RCBotNew(temp);
  }
rcrtl = coord;  rcrtlin = cin; rcrtfirst = false;
if (cin) RCBotNew(coord);
} /* end of RCRtNew */


private procedure RCTopNew(coord)  Cd coord;
{
boolean cin = (boolean)(coord.y <= rccliptp);
Cd temp;
if (rctpfirst) {rctpf = coord; rctpfin = cin;}
else if (rctplin != cin)
  {
  temp.y = rccliptp;
  temp.x = rctpl.x + (coord.x - rctpl.x) * (rccliptp - rctpl.y)
                    / (coord.y - rctpl.y);
  RCRtNew(temp);
  }
rctpl = coord;  rctplin = cin; rctpfirst = false;
if (cin) RCRtNew(coord);
} /* end of RCTopNew */


private procedure FRCNextPt(coord)  DevCd coord; {
  Cd cd;
  fixtopflt(coord.x, &cd.x);
  fixtopflt(coord.y, &cd.y);
  RCNextPt(cd);
  }

public procedure RCNextPt(coord)  Cd coord;
{
boolean cin = (boolean)(coord.x >= rccliplf);
Cd temp;
if (rclffirst) {rclff = coord;  rclffin = cin;}
else if (rclflin != cin)
  {
  temp.x = rccliplf;
  temp.y = rclfl.y + (coord.y - rclfl.y) * (rccliplf - rclfl.x)
                    / (coord.x - rclfl.x);
  RCTopNew(temp);
  }
rclfl = coord;  rclflin = cin;  rclffirst = false;
if (cin) RCTopNew(coord);
} /* end of RCNextPt */


private procedure RCLfClose()
{
Cd temp;
if (rclffin != rclflin)
  {
  temp.x = rccliplf;
  temp.y = rclfl.y + (rclff.y - rclfl.y) * (rccliplf - rclfl.x)
                     / (rclff.x - rclfl.x);
  RCTopNew(temp);
  }
} /* end of RCLfClose */


private procedure RCTopClose()
{
Cd temp;
if (rctpfin != rctplin)
  {
  temp.y = rccliptp;
  temp.x = rctpl.x + (rctpf.x - rctpl.x) * (rccliptp - rctpl.y)
                     / (rctpf.y - rctpl.y);
  RCRtNew(temp);
  }
} /* end of RCTopClose */


private procedure RCRtClose()
{
Cd temp;
if (rcrtfin != rcrtlin)
  {
  temp.x = rccliprt;
  temp.y = rcrtl.y + (rcrtf.y - rcrtl.y) * (rccliprt - rcrtl.x)
                     / (rcrtf.x - rcrtl.x);
  RCBotNew(temp);
  }
} /* end of RCRtClose */


private procedure RCBotClose()
{
Cd temp;
if (rcbtfin != rcbtlin)
  {
  temp.y = rcclipbt;
  temp.x = rcbtl.x + (rcbtf.x - rcbtl.x) * (rcclipbt - rcbtl.y)
                     / (rcbtf.y - rcbtl.y);
  (*rcclnewpt)(temp);
  }
} /* end of RCBotClose */


public procedure RCLastPt()
{
RCLfClose();
RCTopClose();
RCRtClose();
RCBotClose();
rclffirst = rctpfirst = rcrtfirst = rcbtfirst = true;
(*rcrdcclose)();
} /* end of RCLastPt */


public procedure FeedPathToReducer(
  path, NextPt, LastPt, reportFixed, testRect, ll, ur)
  PPath path;  procedure (*NextPt)(), (*LastPt)();
  boolean reportFixed, testRect; DevCd ll, ur;
{DoPath(
  path, NextPt, NextPt, LastPt, LastPt, NoOp, true, gs->flatEps,
  reportFixed, testRect, ll, ur);}

public boolean QRdcOk(path,fill) PPath path; boolean fill;
{
/* the old reducer has a performance advantage for large delta y */
/* the new reducer has a performance advantage for complex paths */
  integer dy, len;
  if ((PathType)path->type != listPth) ConvertToListPath(path); 
#if DPSXA
  if(!gs->noColorAllowed) return false; /* check for mask device */
#endif /* DPSXA */
  len = path->length;
  { /* check the bbox height */
  register BBox bb = &path->bbox;
  register integer th, ll, ur;
  th = 700 + ((len - 4) << 6); /* ad hoc function */
  ll = bb->bl.y; ur = bb->tr.y;
  if ((dy=ur-ll) > th) return false;
  }
  if (!fill) return true; /* offset fill is biased toward new reducer */
  if (len > 16) return true;
  if (dy < 100 && len > 8) return true;
  { /* if the path is all lines use the old reducer */
  register PPthElt pe;
  if (path->ptr.lp == NULL || (pe = path->ptr.lp->head) == 0)
    return true; /* null path */
  while (pe != 0) {
    if (pe->tag==pathcurveto) return true;
    pe = pe->next;
    }
  return false;
  }
}  /* end of QRdcOk */

public BBoxCompareResult BBoxVsClipBBox(b) BBox b; {
  BBoxCompareResult bbcomp;
  Cd coord;
  bbcomp = BBCompare(b, GetDevClipBBox());
  if (bbcomp != inside || DevClipIsRect()) return bbcomp;
  return overlap;
  }

public boolean PathIsRect(path)  register PPath path; {
if (path->checkedForRect)
  return path->isRect;
path->checkedForRect = true;
if (path->rp != NULL) {
  /* intersectPth and strokePth always come through here */
  register DevPrim *p = (DevPrim *)(path->rp->devprim);
  return path->isRect = p != NULL && p->next == NULL && DevPrimIsRect(p);
  }
if ((PathType)path->type == quadPth) {
  register QuadPath *qp;
  qp = path->ptr.qp;
  if (qp->c0.x == qp->c1.x) 
    return path->isRect = (qp->c2.x == qp->c3.x &&
            qp->c1.y == qp->c2.y &&
	    qp->c0.y == qp->c3.y);
  if (qp->c0.y == qp->c1.y)
    return path->isRect = (qp->c2.y == qp->c3.y &&
            qp->c1.x == qp->c2.x &&
	    qp->c0.x == qp->c3.x);
  return path->isRect = false;
  }
if ((PathType)path->type == listPth) {
  boolean xChangedFirst;
  Cd c1, c2, c3, c4;
  register PPthElt pe;
  ListPath *pp;
  path->isRect = false;
  pp = path->ptr.lp;
  if (pp == NULL || (pe = pp->head) == 0) return false;
  c1 = pe->coord;
  if ((pe = pe->next) == NULL) return false;
  if (pe->tag != pathlineto) return false;
  c2 = pe->coord;
  if (c1.x == c2.x) xChangedFirst = false;
  else if (c1.y == c2.y) xChangedFirst = true;
  else return false;
  if ((pe = pe->next) == NULL) return false;
  if (pe->tag != pathlineto) return false;
  c3 = pe->coord;
  if ((xChangedFirst && c3.x != c2.x) || (!xChangedFirst && c3.y != c2.y))
    return false;
  if ((pe = pe->next) == NULL) return false;
  if (pe->tag != pathlineto) return false;
  c4 = pe->coord;
  if ((xChangedFirst && (c4.y != c3.y || c4.x != c1.x))
     || (!xChangedFirst && (c4.x != c3.x || c4.y != c1.y))) return false;
  if ((pe = pe->next) != NULL)
    { PathTag tag;
    tag = pe->tag;
    if ((tag != pathclose && tag != pathlineto)
        || (tag == pathclose && pe->next != NULL)) return false;
    if (tag == pathlineto)
      {
      if (pe->coord.x != c1.x || pe->coord.y != c1.y) return false;
      if ((pe = pe->next) != NULL)
        {
        if (pe->tag != pathclose || pe->next != NULL) return false;
        }
      }
    }
  return path->isRect = true;
  }
CantHappen();
}  /* end of PathIsRect */

public procedure AddRdcTrap(yt, yb, xtl, xtr, xbl, xbr)
    Fixed yt, yb, xtl, xtr, xbl, xbr;
    {
    AddTrap(
        RdcToDev(yt), RdcToDev(yb), 
        RdcToDev(xtl), RdcToDev(xtr), 
        RdcToDev(xbl), RdcToDev(xbr));
    }

private procedure ReducePathRun(run) char *run; {
 *(ms->rdcDevPrim) = (DevPrim *)AddRunDevPrim(
   *(ms->rdcDevPrim), (DevRun *)run);
 }

private procedure ReducePathTrapsFilled(m) register PMarkState m; {
  DevPrim *dp = CopyDevPrim(m->trapsDP);
  dp->next = *(ms->rdcDevPrim);
  *(ms->rdcDevPrim) = dp;
  if (!m->haveTrapBounds)
    EmptyDevBounds(&m->trapsDP->bounds);
  m->trapsDP->items = 0;
  }

private procedure SetScalFromBBoxes(bb1, bb2) BBox bb1, bb2; {
  integer maxval, minval, m0, m1;
  m0 = MIN(bb1->tr.x, bb2->tr.x);
  m1 = MIN(bb1->tr.y, bb2->tr.y);
  maxval = MAX(m0, m1);
  m0 = MAX(bb1->bl.x, bb2->bl.x);
  m1 = MAX(bb1->bl.y, bb2->bl.y);
  minval = MIN(m0, m1);
  SetRdcScal(maxval, minval);
  }

private DevPrim * ReverseDevPrimList(dp) register DevPrim *dp; {
  register DevPrim * nxt, * prv;
  prv = NULL;
  while (dp != NULL) {
    nxt = dp->next; dp->next = prv; prv = dp; dp = nxt;
    }
  return prv;
  }

public DevPrim * DoRdcPth(evenOdd, context, bBox, qReduce, enumerate,
    qEnumOk, qenumerate)
  boolean evenOdd;
  char *context;
  register BBox bBox;
  boolean (*qReduce)(), (*qEnumOk)();
  procedure (*enumerate)(), (*qenumerate)(); {
  boolean rectClip, qenum;
  BBoxRec rdcBBox;
  DevPrim *dp = NULL;
  DevPrim **oldrdcDevPrim = ms->rdcDevPrim;
  DevCd nullCD;
  ms->rdcDevPrim = &dp;
  Get16KBBox(&rdcBBox);
  rectClip = (BBCompare(bBox, &rdcBBox) != inside);
#if DPSXA
  if(false) {
#else /* DPSXA */
  if ((*qReduce)(context, true)) {
#endif /* DPSXA */
    dp = InitDevPrim(NewDevPrim(), (DevPrim *)NULL);
    QResetReducer();
    qenum = (qEnumOk != NULL && (*qEnumOk)(context));
    if (!rectClip) {
      if (qenum)
        (*qenumerate)(context, QFNewPoint, QRdcClose, false, nullCD, nullCD);
      else
        (*enumerate)(context, QFNewPoint, QRdcClose,
                     true, false, nullCD, nullCD);
      }
    else {
      SetUpForRectClip(QNewPoint, QRdcClose, &rdcBBox);
      if (qenum)
        (*qenumerate)(context, FRCNextPt, RCLastPt, false, nullCD, nullCD);
      else
        (*enumerate)(context, RCNextPt, RCLastPt,
                     false, false, nullCD, nullCD);
      }
    DURING
    QReduce(evenOdd, ReducePathRun, -16000, 16000, (char *)NIL);
    HANDLER {
      ms->rdcDevPrim = oldrdcDevPrim;
      DisposeDevPrim(dp);
      RERAISE;
      }
    END_HANDLER;
    }
  else {
    procedure (*oldTrapsFilled)();
#if DPSXA
		integer minval, maxval;
		rdcBBox.bl.x = chunkBBox.bl.x - RDCXTRA;
		rdcBBox.bl.y = chunkBBox.bl.y - RDCXTRA;
		rdcBBox.tr.x = chunkBBox.tr.x + RDCXTRA;
		rdcBBox.tr.y = chunkBBox.tr.y + RDCXTRA;
    SetUpForRectClip(ClNewPt, RdcClose, &rdcBBox);
		minval = MIN(rdcBBox.bl.x, rdcBBox.bl.y);
		maxval = MAX(rdcBBox.tr.x, rdcBBox.tr.y);
		SetRdcScal(maxval, minval);
		ResetReducer();
		RdcClip(true);
		RCNextPt(chunkqp->c0);
		RCNextPt(chunkqp->c1);
		RCNextPt(chunkqp->c2);
		RCNextPt(chunkqp->c3);
		RCLastPt();
		rectClip = true;
#else /* DPSXA */
    SetScalFromBBoxes(bBox, &rdcBBox);
    ResetReducer();
#endif /* DPSXA */
    RdcClip(false);
    
    (*ms->procs->initMark)(ms, true);
    oldTrapsFilled = ms->procs->trapsFilled;
    ms->procs->trapsFilled = ReducePathTrapsFilled;
    DURING
    if (!rectClip)
      (*enumerate)(context, ClNewPt, RdcClose,
                   false, false, nullCD, nullCD);
    else {
      SetUpForRectClip(ClNewPt, RdcClose, &rdcBBox);
      (*enumerate)(context, RCNextPt, RCLastPt,
                   false, false, nullCD, nullCD);
      }
#if DPSXA
		Reduce(AddRdcTrap, true, evenOdd);
#else /* DPSXA */
    Reduce(AddRdcTrap, false, evenOdd);
#endif /* DPSXA */
    (*ms->procs->termMark)(ms);
    HANDLER {
      if (dp) DisposeDevPrim(dp);
      ms->rdcDevPrim = oldrdcDevPrim;
      ms->procs->trapsFilled = oldTrapsFilled;
      RERAISE;
      }
    END_HANDLER;
    ms->procs->trapsFilled = oldTrapsFilled;
    }
  ms->rdcDevPrim = oldrdcDevPrim;
  if (dp == NULL)
    dp = InitDevPrim(NewDevPrim(), (DevPrim *)NULL);
  else dp = ReverseDevPrimList(dp);
    /* reverse so will be shown in same order produced */
  return dp;
  }

public procedure PutRdc(path, dp, evenOdd)
  PPath path; DevPrim *dp; boolean evenOdd; {
  ReducedPath *rdcpth;
  path->rp = rdcpth = (ReducedPath *)os_newelement(rpStorage);
  rdcpth->refcnt = 1;
  rdcpth->devprim = (char *)dp; 
  path->eoReduced = evenOdd;
  }

public procedure ReducePath(path, evenOdd)
  register PPath path; boolean evenOdd; {
  boolean fixedOk;
  DevPrim *dp;
  if ((PathType)path->type == intersectPth) return; /* already done */
  if (path->rp != NULL &&
      path->rp->devprim != NULL &&
      path->eoReduced == evenOdd)
    return; /* already reduced in desired form */
#if DPSXA
	if(path != &gs->clip)
		RemReducedRef(path);
#else /* DPSXA */
  RemReducedRef(path);
#endif /* DPSXA */
  if ((PathType)path->type == strokePth) ConvertToListPath(path);
  switch ((PathType)path->type) {
    case quadPth: {
      QuadPath *qp;
      register real r;
      Cd *rp;
      DevCd dc0, dc1, dc2, dc3;
      integer i;
      qp = path->ptr.qp;
      rp = &qp->c0; fixedOk = true;
#if DPSXA
      for (i = 4; i > 0; i--) {
        r = rp->x;
        if (r > XA_MAX || r < -XA_MAX) { fixedOk = false; break; }
        r = rp->y;
        if (r > XA_MAX || r < -XA_MAX) { fixedOk = false; break; }
				rp++;
			}
#else /* DPSXA */
      for (i = 4; i > 0; i--) {
        r = rp->x;
        if (r > fp16k || r < -fp16k) { fixedOk = false; break; }
        r = rp->y;
        if (r > fp16k || r < -fp16k) { fixedOk = false; break; }
				rp++;
			}
#endif /* DPSXA */
      if (fixedOk) {
        procedure (*oldStrkTrp)();
        extern procedure (*StrkTrp)();
	DevPrim devprim;
	MarkState oldms;
	DevTrap traps[4];
        oldStrkTrp = StrkTrp;
        StrkTrp = AddTrap;
	oldms = *ms;
	ms->haveTrapBounds = false;
	ms->trapsDP = InitDevPrim(&devprim, (DevPrim *)NULL);
	devprim.type = trapType;
	devprim.items = 0;
	devprim.maxItems = 4;
	devprim.value.trap = (DevTrap *)&traps[0];
        FixCd(qp->c0, &dc0);
        FixCd(qp->c1, &dc1);
        FixCd(qp->c2, &dc2);
        FixCd(qp->c3, &dc3);
	DURING
        FastFillQuad(dc0, dc1, dc2, dc3);
	HANDLER { *ms = oldms; RERAISE; }
	END_HANDLER;
	*ms = oldms;
        StrkTrp = oldStrkTrp;
	dp = CopyDevPrim(&devprim);
        break;
	}
      ConvertToListPath(path);
      }
      /* fall through to listPth case */
    case listPth:
      dp = DoRdcPth(
        evenOdd, (char *)path, &path->bbox, QRdcOk, FeedPathToReducer,
        (boolean (*)())NULL, (PVoidProc)NULL);
      break;
    default: CantHappen();
    }
  DURING
  PutRdc(path, dp, evenOdd);
  HANDLER { DisposeDevPrim(dp); RERAISE; }
  END_HANDLER;
  }

public procedure FillPath(evenOdd, context, bBox, qReduce, enumerate,
  qEnumOk, qenumerate)
  boolean evenOdd;
  char *context;
  register BBox bBox;
  boolean (*qReduce)(), (*qEnumOk)();
  procedure (*enumerate)(), (*qenumerate)(); {
  BBox clipBBox;
  BBoxRec rdcBBox;
  extern BBox GetDevClipBBox();
  integer yt, yb;
  boolean doRectClip, testRect, qRdc;
  DevCd ll, ur;
  clipBBox = GetDevClipBBox();
  ms->bbCompMark = BBCompare(bBox, clipBBox);
#if DPSXA
  /* check for mask device */
  bbCompChunk = gs->noColorAllowed ? inside : BBCompare(bBox, &chunkBBox);
  if (bbCompChunk == outside) return;
#endif DPSXA
  if (ms->bbCompMark == outside) return;
  qRdc = (*qReduce)(context, true);
  testRect = false;
  if (ms->bbCompMark == inside)
    doRectClip = false;
  else {
    GetDevBBox(&rdcBBox);
    if (BBCompare(bBox, &rdcBBox) == overlap) doRectClip = true;
    else { /* avoid flattening curves that cannot overlap clip rect */
      extern DevBBox GetDevClipDevBBox();
      DevBBox clipbb;
      doRectClip = false;
      testRect = true;
      clipbb = GetDevClipDevBBox();
      ll.x = clipbb->bl.x - 0x10000;
      ll.y = clipbb->bl.y - 0x10000;
      ur.x = clipbb->tr.x + 0x10000;
      ur.y = clipbb->tr.y + 0x10000;
      }
    }
  (*ms->procs->initMark)(ms, true);
#if DPSXA
  if(bbCompChunk == inside) {
    integer minval, maxval;
    minval = MIN(bBox->bl.x, bBox->bl.y);
    maxval = MAX(bBox->tr.x, bBox->tr.y);
    SetRdcScal(maxval, minval);
	ResetReducer();
	RdcClip(false);
    (*enumerate)(context, ClNewPt, RdcClose, false, testRect, ll, ur);
	Reduce(AddRdcTrap, false, evenOdd);
  } else {
    integer minval, maxval;
	ll.x = rdcBBox.bl.x = chunkBBox.bl.x - RDCXTRA;
	ll.y = rdcBBox.bl.y = chunkBBox.bl.y - RDCXTRA;
	ur.x = rdcBBox.tr.x = chunkBBox.tr.x + RDCXTRA;
	ur.y = rdcBBox.tr.y = chunkBBox.tr.y + RDCXTRA;
    minval = MIN(rdcBBox.bl.x, rdcBBox.bl.y);
    maxval = MAX(rdcBBox.tr.x, rdcBBox.tr.y);
    SetRdcScal(maxval, minval);
	SetUpForRectClip(ClNewPt, RdcClose, &rdcBBox);
	ResetReducer();
	RdcClip(true);
	RCNextPt(chunkqp->c0);
	RCNextPt(chunkqp->c1);
	RCNextPt(chunkqp->c2);
	RCNextPt(chunkqp->c3);
	RCLastPt();
	RdcClip(false);
	(*enumerate)(context, RCNextPt, RCLastPt, false, testRect, ll, ur);
	Reduce(AddRdcTrap, true, evenOdd);
  }
#else /* DPSXA */
  if (qRdc) {
    QResetReducer();
    if (!doRectClip) {
      if (qEnumOk != NULL && (*qEnumOk)(context))
        (*qenumerate)(context, QFNewPoint, QRdcClose, testRect, ll, ur);
      else
        (*enumerate)(context, QFNewPoint, QRdcClose, true, testRect, ll, ur);
      }
    else {
      SetUpForRectClip(QNewPoint, QRdcClose, &rdcBBox);
      if (qEnumOk != NULL && (*qEnumOk)(context))
        (*qenumerate)(context, FRCNextPt, RCLastPt, testRect, ll, ur);
      else
        (*enumerate)(context, RCNextPt, RCLastPt, false, testRect, ll, ur);
      }
    yt = clipBBox->tr.y;
    yb = clipBBox->bl.y;
    QReduce(evenOdd, AddRunMark, (Int16)yb-1, (Int16)yt+1, (char *)NIL);
    (*ms->procs->termMark)(ms);
    return;
    }
  if (!doRectClip) {
    integer minval, maxval;
    minval = MIN(bBox->bl.x, bBox->bl.y);
    maxval = MAX(bBox->tr.x, bBox->tr.y);
    SetRdcScal(maxval, minval);
    }
  else
    SetScalFromBBoxes(bBox, &rdcBBox);
  ResetReducer();
  RdcClip(false);
  if (!doRectClip)
    (*enumerate)(context, ClNewPt, RdcClose, false, testRect, ll, ur);
  else {
    SetUpForRectClip(ClNewPt, RdcClose, &rdcBBox);
    (*enumerate)(context, RCNextPt, RCLastPt, false, testRect, ll, ur);
    }
  Reduce(AddRdcTrap, false, evenOdd);
#endif /* DPSXA */
  (*ms->procs->termMark)(ms);
} /* end of FillPath */

public boolean	   /* return (isRect) */
ReduceQuadPath(userPt, userw, userh, matrix, dp, qp, bbox, path)
  Cd userPt;
  real userw, userh;
  PMtx matrix;
  register DevPrim *dp;
  register QuadPath *qp;
  register BBox bbox;	/* if isRect && !path qp won't be set! */
  Path *path;
{
  boolean fixedOK, isRect = true;
  MarkState oldms;

  if (path)
    bbox = &path->bbox;
  Assert(dp && qp && bbox);
  Assert((dp->type == trapType) && (dp->items == 0) && (dp->maxItems > 6));
  {
    register PMtx m = matrix;
    register real px, py, wx, wy, hx, hy;

    if (RealEq0(m->a) && RealEq0(m->d)) {
      px = userPt.y * m->c + m->tx;
      wx = userh * m->c;
      py = userPt.x * m->b + m->ty;
      hy = userw * m->b;
    } else if (RealEq0(m->b) && RealEq0(m->c)) {
      px = userPt.x * m->a + m->tx;
      wx = userw * m->a;
      py = userPt.y * m->d + m->ty;
      hy = userh * m->d;
    } else {
      isRect = false;
      px = userPt.x * m->a + userPt.y * m->c + m->tx;
      py = userPt.y * m->d + userPt.x * m->b + m->ty;
      wx = userw * m->a;
      wy = userw * m->b;
      if (wy < 0) {
	px -= (wx = -wx);
	py -= (wy = -wy);
      }
      hx = userh * m->c;
      hy = userh * m->d;
      if (hy < 0) {
	px -= (hx = -hx);
	py -= (hy = -hy);
      }
      qp->c0.x = px;
      qp->c1.x = px + hx;
      qp->c2.x = px + hx + wx;
      qp->c3.x = px + wx;
      if (hx < 0)
	if (wx < 0) {
	  bbox->bl.x = px + hx + wx;
	  bbox->tr.x = px;
	} else {
	  bbox->bl.x = px + hx;
	  bbox->tr.x = px + wx;
      } else if (wx < 0) {
	bbox->bl.x = px + wx;
	bbox->tr.x = px + hx;
      } else {
	bbox->bl.x = px;
	bbox->tr.x = px + hx + wx;
      }
      qp->c0.y = py;
      qp->c1.y = py + hy;
      qp->c2.y = py + hy + wy;
      qp->c3.y = py + wy;
      bbox->bl.y = py;
      bbox->tr.y = py + hy + wy;
    }
    if (isRect) {
      if (wx < 0) {
	bbox->bl.x = px + wx;
	bbox->tr.x = px;
      } else {
	bbox->bl.x = px;
	bbox->tr.x = px + wx;
      } if (hy < 0) {
	bbox->bl.y = py + hy;
	bbox->tr.y = py;
      } else {
	bbox->bl.y = py;
	bbox->tr.y = py + hy;
      }
    }
  }
#if DPSXA
  fixedOK = ((bbox->bl.x >= chunkBBox.bl.x) && (bbox->bl.y >= chunkBBox.bl.y) &&
	     (bbox->tr.x <= chunkBBox.tr.x) && (bbox->tr.y <= chunkBBox.tr.y));
#else /* DPSXA */
  fixedOK = ((bbox->bl.x >= -fp16k) && (bbox->bl.y >= -fp16k) &&
	     (bbox->tr.x <= fp16k) && (bbox->tr.y <= fp16k));
#endif /* DPSXA */
  if (isRect && (path || !fixedOK)) {	/* fill qp after all! */
    qp->c0.x = qp->c3.x = bbox->bl.x;
    qp->c0.y = qp->c1.y = bbox->tr.y;
    qp->c1.x = qp->c2.x = bbox->tr.x;
    qp->c2.y = qp->c3.y = bbox->bl.y;
  }
  oldms = *ms;
  ms->haveTrapBounds = false;
  EmptyDevBounds(&dp->bounds);
  ms->trapsDP = dp;
  if (fixedOK)
    if (isRect) {
      register Fixed xl, yl, xg, yg;
      xl = pflttofix(&bbox->bl.x);
      xg = pflttofix(&bbox->tr.x);
      yl = pflttofix(&bbox->bl.y);
      yg = pflttofix(&bbox->tr.y);
      AddTrap(yg, yl, xl, xg, xl, xg);
    } else {
      DevCd dc0, dc1, dc2, dc3;
      Fixed xl, xg;
      procedure(*oldStrkTrp) ();
      extern procedure(*StrkTrp) ();

      oldStrkTrp = StrkTrp;
      StrkTrp = AddTrap;
      FixCd(qp->c0, &dc0);
      FixCd(qp->c1, &dc1);
      FixCd(qp->c2, &dc2);
      FixCd(qp->c3, &dc3);
      xl = (dc1.x < dc0.x) ?
	((dc2.x < dc1.x) ? dc2.x : dc1.x) : ((dc3.x < dc0.x) ? dc3.x : dc0.x);
      xg = (dc1.x > dc0.x) ?
	((dc2.x > dc1.x) ? dc2.x : dc1.x) : ((dc3.x > dc0.x) ? dc3.x : dc0.x);
      FastFillQuad(dc0, dc1, dc2, dc3);
      StrkTrp = oldStrkTrp;
  } else {
    extern procedure ClNewPt();
    extern procedure RdcClose();
    BBoxRec rdcBBox;

#if !(DPSXA)
    Get16KBBox(&rdcBBox);
    SetUpForRectClip(ClNewPt, RdcClose, &rdcBBox);
    SetScalFromBBoxes(bbox, &rdcBBox);
#endif /* DPSXA */
    ResetReducer();
#if DPSXA
	{
	integer minval, maxval;
	rdcBBox.bl.x = chunkBBox.bl.x - RDCXTRA;
 	rdcBBox.bl.y = chunkBBox.bl.y - RDCXTRA;
	rdcBBox.tr.x = chunkBBox.tr.x + RDCXTRA;
	rdcBBox.tr.y = chunkBBox.tr.y + RDCXTRA;
	minval = MIN(rdcBBox.bl.x, rdcBBox.bl.y);
	maxval = MAX(rdcBBox.tr.x, rdcBBox.tr.y);
	SetRdcScal(maxval, minval);
	SetUpForRectClip(ClNewPt, RdcClose, &rdcBBox);
	RdcClip(true);
	RCNextPt(chunkqp->c0);
	RCNextPt(chunkqp->c1);
	RCNextPt(chunkqp->c2);
	RCNextPt(chunkqp->c3);
	RCLastPt();
	}
#endif /* DPSXA */
    RdcClip(false);
    RCNextPt(qp->c0);
    RCNextPt(qp->c1);
    RCNextPt(qp->c2);
    RCNextPt(qp->c3);
    RCLastPt();
#if DPSXA
    Reduce(AddRdcTrap, true, false);
#else /* DPSXA */
    Reduce(AddRdcTrap, false, false);
#endif /* DPSXA */
  }
  *ms = oldms;
done:
  if (!isRect && (dp->items == 1)) {	/* check again */
    register DevTrap *dt = dp->value.trap;

    isRect = (dt->l.xl == dt->l.xg) && (dt->g.xl == dt->g.xg);
  }
  if (path) {
    path->type = (BitField)quadPth;
    path->ptr.qp = qp;
    qp->refcnt = 1;
    path->rp = (ReducedPath *)os_newelement(rpStorage);
    path->rp->refcnt = 1;
    path->rp->devprim = (char *) dp;
    path->secret = path->eoReduced = false;
    path->checkedForRect = true;
    path->isRect = isRect;
    path->length = 5;
  }
  return (isRect);
}		   /* ReduceQuadPath */

private procedure RdcStrkTermMark() {
  /* if stroke calls termMark then it is about to directly mark something */
  /* which means we cannot cache the results */
  /* so do an error to cause an unwind of the stack */
  PSError(limitcheck);
  }

private procedure RdcStrkDumpMasks(devmask, len, llx, lly, urx, ury)
  DevMask* devmask; integer len, llx, lly, urx, ury; {
  register DevPrim *dp = InitDevPrim(NewDevPrim(), (DevPrim *)NULL);
  dp->next = rdcStrkMasks;
  rdcStrkMasks = dp;
  dp->type = maskType;
  dp->items = len;
  dp->maxItems = len;
  dp->value.string = (DevMask *)NEW(len, sizeof(DevMask));
  os_bcopy((char *)devmask, (char *)dp->value.string, len * sizeof(DevMask));
  MakeBounds(&dp->bounds,
             FixInt(lly), FixInt(ury),
             FixInt(llx), FixInt(urx));
  }

public DevPrim * DoRdcStroke(context, bBox, rectangle, c1, c2,
                             enumerate, qEnumOk, qenumerate, circletraps)
  char *context;
  BBox bBox;
  boolean rectangle, circletraps;
  Cd c1, c2;
  boolean (*qEnumOk)();
  procedure (*enumerate)(), (*qenumerate)(); {
  DevPrim *dp = NULL, *result;
  BBoxRec clipbbox;
  MarkStateProcs msprocs;
  PMarkStateProcs oldprocs;
  DevBBoxRec devclipbbox;
  DevPrim **oldRdcDevPrim;

  if (circletraps && (gs->lineJoin == roundJoin || gs->lineCap == roundCap))
    PreCacheTrapCircles(); 
  Get16KBBox(&clipbbox);
  devclipbbox.bl.x = devclipbbox.bl.y = FixInt(-16000);
  devclipbbox.tr.x = devclipbbox.tr.y = FixInt(16000);
  rdcStrkMasks = NULL;
  msprocs.initMark = StdInitMark;
  msprocs.termMark = RdcStrkTermMark;
  msprocs.trapsFilled = ReducePathTrapsFilled;
  msprocs.strokeMasksFilled = RdcStrkDumpMasks;
  oldRdcDevPrim = ms->rdcDevPrim;
  ms->rdcDevPrim = &dp;
  oldprocs = ms->procs;
  ms->procs = &msprocs;
  DURING
  DoStroke(false, context, bBox, rectangle, c1, c2,
           enumerate, qEnumOk, qenumerate, (int)circletraps,
	   &clipbbox, true, &devclipbbox);
  StdTermMark(ms);
  FinStroke();
  HANDLER {
    if (dp) DisposeDevPrim(dp);
    if (rdcStrkMasks) DisposeDevPrim(rdcStrkMasks);
    ms->procs = oldprocs;
    ms->rdcDevPrim = oldRdcDevPrim;
    RERAISE;
    }
  END_HANDLER;
  ms->procs = oldprocs;
  ms->rdcDevPrim = oldRdcDevPrim;
  result = dp;
  if (rdcStrkMasks != NULL) {
    Assert(!circletraps);
    result = rdcStrkMasks;
    while (rdcStrkMasks->next) rdcStrkMasks = rdcStrkMasks->next;
    rdcStrkMasks->next = dp;
    rdcStrkMasks = NULL;
    }
  else result = dp;
  if (result == NULL)
    result = InitDevPrim(NewDevPrim(), (DevPrim *)NULL);
  else result = ReverseDevPrimList(result);
  return result;
  }


