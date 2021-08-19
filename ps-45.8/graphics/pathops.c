/*
  pathops.c

Copyright (c) 1984-1990 Adobe Systems Incorporated.
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
Larry Baer: Thu Nov 30 14:43:24 1989
Scott Byer: Thu Jun  1 13:24:00 1989
Ed Taft: Thu Jul 28 16:41:22 1988
Bill Paxton: Fri Nov  4 15:08:28 1988
Ivor Durham: Tue Aug 16 10:42:47 1988
Jim Sandman: Wed Dec 13 09:28:00 1989
Joe Pasqua: Tue Feb 28 13:18:55 1989
Bill Bilodeau: Fri Oct 13 11:20:35 PDT 1989
Ross Thompson: Tue Feb 13 14:05:58 1990
Peter Graffagnino @ next: 4/3/90:  pathforall on fonts now allowed (yeah!)
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

#if DPSXA
DevCd xaOffset;
boolean strokeOp;
extern Cd UOffset;
#endif /* DPSXA */


extern DevPrim * GetDevClipPrim();
extern PPthElt pathFree;


extern boolean QRdcOk();
extern procedure NoOp();

private Path chrPth;
private boolean chrPthStrk;  /* if false, stroked chrPths return spine*/

extern ListPath *MakeOwnListPath(), *AllocListPathRec();

private procedure AppendPath(p, q) PPath p, q; { /* append path q to path p */
  register PPthElt pe;
  boolean secret;
  register ListPath *pp;
  ListPath *qq;
  if ((PathType)q->type != listPth) ConvertToListPath(q);
  qq = q->ptr.lp;
  if (qq == NULL || qq->head == 0) goto done;
  if (qq->refcnt > 1) qq = MakeOwnListPath(q);
  RemReducedRef(q);
  if ((PathType)p->type != listPth) ConvertToListPath(p);
  if ((pp = p->ptr.lp) == NULL) pp = AllocListPathRec(p);
  if (pp->refcnt > 1) pp = MakeOwnListPath(p);
  RemReducedRef(p);
  if (pp->tail != NULL && pp->tail->tag == pathstart)
    {
    p->length--;
    if (pp->head == pp->tail)  {
      pp->head->next = pathFree;
      pathFree = pp->head;
      pp->head = pp->tail = 0;}
    else
      {
      pe = pp->head;
      while (pe->next != pp->tail) pe = pe->next;
      pp->tail->next = pathFree;
      pathFree = pp->tail;
      pp->tail = pe;
      }
    }
  if (pp->tail == NULL) { *p = *q; qq = pp; }
  else
    {
    pp->tail->next = qq->head;
    pp->tail = qq->tail;
    pp->start = qq->start;
    if (p->bbox.bl.x > q->bbox.bl.x) p->bbox.bl.x = q->bbox.bl.x;
    if (p->bbox.bl.y > q->bbox.bl.y) p->bbox.bl.y = q->bbox.bl.y;
    if (p->bbox.tr.x < q->bbox.tr.x) p->bbox.tr.x = q->bbox.tr.x;
    if (p->bbox.tr.y < q->bbox.tr.y) p->bbox.tr.y = q->bbox.tr.y;
    }
  p->secret |= q->secret;
  p->length += q->length;
 done:
  if (qq != NULL) {
    os_freeelement(lpStorage, (char *)qq);
    }
  InitPath(q);
  }  /* end of AppendPath */


public procedure TlatPath(p, delta) register PPath p; Cd delta; {
  register PPthElt pe;
  ListPath *pp;
  register real dx, dy;
  if (RealEq0(delta.x) && RealEq0(delta.y)) return;
  if ((PathType)p->type != listPth) ConvertToListPath(p);
  if ((pp = p->ptr.lp) == NULL) pp = AllocListPathRec(p);
  if (pp->refcnt > 1) pp = MakeOwnListPath(p);
  if (pp == NULL || pp->head == 0) return;
  dx = delta.x; dy = delta.y;
  for (pe = pp->head; pe != 0; pe = pe->next)
    {pe->coord.x += dx; pe->coord.y += dy;}
  p->bbox.tr.x += dx;  p->bbox.tr.y += dy;
  p->bbox.bl.x += dx;  p->bbox.bl.y += dy;
  }  /* end of TlatPath */

public procedure PathBBox(bbox)  BBox bbox;  {*bbox = gs->path.bbox;}

public procedure NewPath() {
  FrPth(&gs->path);
  gs->path.secret = gs->isCharPath;
  gs->cp.x = gs->cp.y = fpZero;
  }  /* end of NewPath */

public procedure GetPathBBoxUserCds(newbl, newtr) PCd newbl, newtr; {
  Cd tl, bl, tr, br;
  register real hi1, hi2, lo1, lo2, t;
  CheckForCurrentPoint(&gs->path);
  tl.x = bl.x = gs->path.bbox.bl.x;
  br.y = bl.y = gs->path.bbox.bl.y;
  br.x = tr.x = gs->path.bbox.tr.x;
  tl.y = tr.y = gs->path.bbox.tr.y;
  ITfmP(tl, &tl);
  ITfmP(tr, &tr);
  ITfmP(bl, &bl);
  ITfmP(br, &br);
  lo1 = tl.x;  hi1 = tr.x;  if (lo1 > hi1) {t = lo1;  lo1 = hi1;  hi1 = t;}
  lo2 = bl.x;  hi2 = br.x;  if (lo2 > hi2) {t = lo2;  lo2 = hi2;  hi2 = t;}
  newbl->x = (lo1 < lo2) ? lo1 : lo2;
  newtr->x = (hi1 > hi2) ? hi1 : hi2;
  lo1 = tl.y;  hi1 = tr.y;  if (lo1 > hi1) {t = lo1;  lo1 = hi1;  hi1 = t;}
  lo2 = bl.y;  hi2 = br.y;  if (lo2 > hi2) {t = lo2;  lo2 = hi2;  hi2 = t;}
  newbl->y = (lo1 < lo2) ? lo1 : lo2;
  newtr->y = (hi1 > hi2) ? hi1 : hi2;
  }  

public procedure PSPathBBox() {
  Cd newbl, newtr;
  GetPathBBoxUserCds(&newbl, &newtr);
  PushPCd(&newbl);  PushPCd(&newtr);
  } /* end of PSPathBBox */

#if DPSXA
public procedure TlatBBox(bbox,delta)
register BBox bbox;
Cd delta;
{
  bbox->tr.x += delta.x;  bbox->tr.y += delta.y;
  bbox->bl.x += delta.x;  bbox->bl.y += delta.y;
}

public procedure BreakUpPath(pathproc,path,bool,userpath)
procedure (*pathproc)();
PPath path; 
boolean bool,userpath;
{
/*
	Makes iterative calls to the path drawing routine for extended addresses 
	by breaking the path up into pre-determined chunks
*/
Cd xydelta, delta;
short i,j;
BBoxRec graphicBBox;
Cd v;
	
	xaOffset.x = 0;
	xaOffset.y = 0;
	xydelta.x = -xChunkOffset;
	xydelta.y = -yChunkOffset;
	delta.x = xydelta.x;
	delta.y = 0;
  	if(userpath) {
		UOffset.x = 0;
		UOffset.y = 0;
	} else 
    	if ((PathType)path->type != listPth) ConvertToListPath(path);
	
	for(j=0; j<maxYChunk; j++) {
		if(userpath)
			graphicBBox = ((PUserPathContext)path)->bbox;
		else
			graphicBBox = path->bbox;
		if(strokeOp){
			v.x = v.y = gs->lineWidth == 0 ? 1 : gs->lineWidth/fpTwo + 1;
			DTfmPCd(v, &gs->matrix, &v);
			graphicBBox.bl.x -= os_labs(v.x); 
			graphicBBox.bl.y -= os_labs(v.y);
			graphicBBox.tr.x += os_labs(v.x);
			graphicBBox.tr.y += os_labs(v.y);
		}			
		if(BBCompare(&graphicBBox, &chunkBBox) != outside) {
			if(BBCompare(&graphicBBox, &gs->clip.bbox) != outside) {
				(*pathproc)(path,bool);
			}
		}
		for(i=0; i<maxXChunk; i++) {
			xaOffset.x += xChunkOffset;
			if(userpath) {
				TlatBBox(&((PUserPathContext)path)->bbox,delta);
				graphicBBox = ((PUserPathContext)path)->bbox;
				UOffset.x = -xaOffset.x;
			} else {
				TlatPath(path,delta);
				graphicBBox = path->bbox;
			}
			TlatBBox(&gs->clip.bbox,delta);
			if(strokeOp){
				v.x = v.y = gs->lineWidth == 0 ? 1 : gs->lineWidth/fpTwo + 1;
				DTfmPCd(v, &gs->matrix, &v);
				graphicBBox.bl.x -= os_labs(v.x);
				graphicBBox.bl.y -= os_labs(v.y);
				graphicBBox.tr.x += os_labs(v.x);
				graphicBBox.tr.y += os_labs(v.y);
			}			
			if(BBCompare(&graphicBBox, &chunkBBox) == outside) continue;
			if(BBCompare(&graphicBBox, &gs->clip.bbox) == outside) continue;
			(*pathproc)(path,bool);
		}
		xaOffset.x = 0;
		xaOffset.y += yChunkOffset;
		delta.x = maxXChunk * -xydelta.x;
		delta.y = xydelta.y;
		if(userpath) {
			TlatBBox(&((PUserPathContext)path)->bbox,delta);
			UOffset.x = -xaOffset.x;
			UOffset.y = -xaOffset.y;
		} else
			TlatPath(path,delta);
		TlatBBox(&gs->clip.bbox,delta);
		delta.x = xydelta.x;
		delta.y = 0;
	}
	/* restore clip bbox */
	delta.x = 0;
	delta.y = maxYChunk * -xydelta.y;
	TlatBBox(&gs->clip.bbox,delta);
	if(userpath) { /* restore userpath bbox */
		TlatBBox(&((PUserPathContext)path)->bbox,delta);
		graphicBBox = ((PUserPathContext)path)->bbox;
		UOffset.x = UOffset.y = 0;
	}
  /* reset offset */
	xaOffset.x = 0;
	xaOffset.y = 0;
}
#endif /* DPSXA */

#define MidPoint(m,a,b) (m).x=((a).x+(b).x)/fpTwo; (m).y=((a).y+(b).y)/fpTwo

#define BezierDivide(a0, a1, a2, a3, b0, b1, b2, b3) \
  b3 = a3; \
  MidPoint(b2, a2, a3); \
  MidPoint(a3, a1, a2); \
  MidPoint(a1, a0, a1); \
  MidPoint(a2, a1, a3); \
  MidPoint(b1, a3, b2); \
  MidPoint(b0, a2, b1); \
  a3 = b0


public procedure FltnCurve(c0, c1, c2, c3, pfr)
  Cd c0, c1, c2, c3;  PFltnRec pfr;
  /* Recursively reduces the Bezier cubic curve defined by c0, ..., c3
     to straight line segments.  Calls (*pfr->report)() with the successive
     endpoints of these segments (skipping c0, which is assumed to have
     been digested by the caller already).  pfr->reps is a real parameter that
     determines how flat the curve must be before reporting straight line
     segments.  reps is device dependent and in device space; a suitable
     reps for a 200 spot/inch device is 2. */
{
Cd d0, d1, d2, d3;
#define llx d0.x
#define lly d0.y
#define urx d1.x
#define ury d1.y
/*  real llx, lly, urx, ury; */
if (c0.x == c1.x && c0.y == c1.y && c2.x == c3.x && c2.y == c3.y)
  goto ReportC3;
if (pfr->limit <= 0) goto ReportC3;
if (c0.x < c3.x) {llx = c0.x - pfr->reps; urx = c3.x + pfr->reps;}
else {llx = c3.x - pfr->reps;  urx = c0.x + pfr->reps;}
if (c0.y < c3.y) {lly = c0.y - pfr->reps;  ury = c3.y + pfr->reps;}
else {lly = c3.y - pfr->reps;  ury = c0.y + pfr->reps;}
if (   c1.x > llx && c2.x > llx && c1.x < urx && c2.x < urx
    && c1.y > lly && c2.y > lly && c1.y < ury && c2.y < ury)
  {
#define eqa d0.x
#define eqb d0.y
#define eqc d1.x
#define eqd d1.y
#define d d2.x
/*  real eqa, eqb, eqc, eqd, d;  */

  eqa = c3.y - c0.y;
  eqb = c0.x - c3.x;
  eqd = os_fabs(eqa);  d = os_fabs(eqb);
  if (eqd < d) eqd = d;
  if (RealEq0(eqd)) goto ReportC3;
  eqc = -(eqa * c0.x + eqb * c0.y);
  eqd = fpOne / eqd;
  d = (eqa * c1.x + eqb * c1.y + eqc) * eqd;
  if (os_fabs(d) < pfr->reps)
    {
    d = (eqa * c2.x + eqb * c2.y + eqc) * eqd;
    if (os_fabs(d) < pfr->reps) goto ReportC3;
    }
  }
#undef llx
#undef lly
#undef urx
#undef ury
#undef eqa
#undef eqb
#undef eqc
#undef eqd
#undef d

BezierDivide(c0, c1, c2, c3, d0, d1, d2, d3);
pfr->limit--;
FltnCurve(c0, c1, c2, c3, pfr);
FltnCurve(d0, d1, d2, d3, pfr);
pfr->limit++;
return;
ReportC3:
  (*pfr->report)(c3);
} /* end of FltnCurve */

public procedure PathForAll(flg) boolean flg;
{
register PPthElt pe;
Path path;
Object moveto, lineto, curveto, close;
Mtx im;
Cd cd;  register PCd pcd;
pcd = &cd;
PopP(&close);
PopP(&curveto);
PopP(&lineto);
PopP(&moveto);
if (!flg && gs->path.secret) PSError(invlaccess); /* NOT InvlAccess() */
MtxInvert(&gs->matrix, &im);
if ((PathType)gs->path.type != listPth) ConvertToListPath(&gs->path);
if (gs->path.ptr.lp == NULL) return; /* empty path */
CopyPath(&path, &gs->path);
DURING
for (pe = path.ptr.lp->head; pe != 0; pe = pe->next)
  {
  switch (pe->tag)
    {
    case pathstart:
      TfmPCd(pe->coord, &im, pcd);  PushPCd(pcd);
      if (psExecute(moveto)) goto freepath;
      break;
    case pathlineto:
      TfmPCd(pe->coord, &im, pcd);  PushPCd(pcd);
      if (psExecute(lineto)) goto freepath;
      break;
    case pathcurveto:
      TfmPCd(pe->coord, &im, pcd);  PushPCd(pcd);
      pe = pe->next;  
      TfmPCd(pe->coord, &im, pcd);  PushPCd(pcd);
      pe = pe->next;
      TfmPCd(pe->coord, &im, pcd);  PushPCd(pcd);
      if (psExecute(curveto)) goto freepath;
      break;
    case pathclose:
      if (psExecute(close)) goto freepath;
      break;
    default: break;
    }
  };
freepath:
  RemPathRef(&path);
  if (GetAbort() == PS_EXIT) SetAbort((integer)0);
HANDLER 
  {RemPathRef(&path);  RERAISE;}
END_HANDLER;
} /* end of PSPathForAll */

public procedure PSPathForAll() { PathForAll(true); }

public procedure DoPath
    (path, stproc, ltproc, clsproc, endproc, ctproc, noendst, reps,
     reportFixed, testRect, ll, ur)
  PPath path; procedure (*stproc)(), (*ltproc)(), (*clsproc)(), (*endproc)(),
  (*ctproc)();  boolean noendst, reportFixed;  real reps;
  boolean testRect; DevCd ll, ur;
/* Responsible for enumerating and flattening a path.  "stproc", which takes a
   single Cd argument, is called for all pathstart's (except a trailing
   pathstart when "noendst" is true) and for all implicit pathstart's
   (between a pathclose and a pathlineto or pathcurveto.  "ltproc", which
   takes a single Cd argument, is called for a pathlineto's and for all
   flattened sections of pathcurveto's.  "clsproc", which takes no arguments,
   is called for all pathclose's.  "endproc", which takes no arguments, is
   called after any pathlineto or pathcurveto that is either the last path
   element or is followed by a pathstart.  "ctproc" is called immediately
   before and immediately after flattening any pathcurveto's.  Its boolean
   argument indicates before (true) or after (false) the flattening.
   "reps" is a real flatness epsilon, to be used while flattening.  */
{
/* Assert: cannot be two consecutive pathStarts or two consecutive
   pathCloses. */
register PPthElt pe;
Cd startCd, c0, c1, c2, c3;
DevCd d0, d1, d2, d3;
boolean needUseFixed, useFixed;
FltnRec fr;
fr.report = ltproc;
fr.reps = reps;
fr.ll = ll;
fr.ur = ur;
fr.reportFixed = reportFixed;
if (reportFixed) {
  needUseFixed = false; useFixed = true;
  fr.feps = dbltofix(reps);
  }
else needUseFixed = true;
if ((PathType)path->type != listPth) ConvertToListPath(path);
if (path->ptr.lp == NULL) return;
for (pe = path->ptr.lp->head; pe != 0; pe = pe->next)
  {
  c1 = pe->coord;
  switch (pe->tag)
    {
    case pathstart:
      if (noendst && pe->next == 0) break;
      startCd = c1;
      if (reportFixed) { FixCd(c1, &d0); (*stproc)(d0); }
      else (*stproc)(c1);
      break;
    case pathlineto:
      if (reportFixed) { FixCd(c1, &d0); (*ltproc)(d0); }
      else (*ltproc)(c1);
      if (pe->next == NULL || pe->next->tag == pathstart)
        (*endproc)();
      break;
    case pathcurveto:
      if (needUseFixed)
        {
        useFixed = (boolean)(
                  path->bbox.bl.x > -fp16k && path->bbox.bl.y > -fp16k &&
                  path->bbox.tr.x <  fp16k && path->bbox.tr.y <  fp16k);
        if (useFixed) fr.feps = dbltofix(reps);
        needUseFixed = false;
        }
      pe = pe->next;  c2 = pe->coord;
      pe = pe->next;  c3 = pe->coord;
      (*ctproc)(true);
      fr.limit = FLATTENLIMIT;
      if (useFixed)
        {
        FixCd(c0, &d0);  FixCd(c1, &d1);  FixCd(c2, &d2);  FixCd(c3, &d3);
	FFltnCurve(d0, d1, d2, d3, &fr, (boolean)!testRect);
        }
      else FltnCurve(c0, c1, c2, c3, &fr);
      (*ctproc)(false);
      if (pe->next == NULL || pe->next->tag == pathstart)
        (*endproc)();
      break;
    case pathclose:
      (*clsproc)();
      if (pe->next != NULL && pe->next->tag != pathstart) {
        if (reportFixed) { FixCd(startCd, &d0); (*stproc)(d0); }
        else (*stproc)(startCd);
	}
      break;
    }
  c0 = pe->coord;
  }
} /* end of DoPath */

#if DPSXA
private procedure XAFillPath(path,evenOdd)	PPath path; boolean evenOdd;
{
	FillPath(evenOdd, (char *)path, &path->bbox, QRdcOk,
            FeedPathToReducer, (procedure (*)())NULL, (procedure (*)())NULL);
}
#endif /* DPSXA */

public procedure Fill(path, evenOdd)  PPath path; boolean evenOdd;
{
    ListPath *pp;
    switch ((PathType)path->type) {
      case listPth:
        if (gs->isCharPath ||
            path->rp == NULL ||
	    path->eoReduced != evenOdd) {
          pp = path->ptr.lp;
          if (pp == NULL || pp->head == 0) return;
          if (gs->isCharPath) {
            if (pp->tail->tag != pathclose)
              ClosePath(path);
            AppendPath(&chrPth, path);
            return;
            }
#if DPSXA
	 if(!strokeOp)
		  BreakUpPath(XAFillPath,path,evenOdd,false);
	 else
#endif /* DPSXA */
          FillPath(
            evenOdd, (char *)path, &path->bbox, QRdcOk,
            FeedPathToReducer, (procedure (*)())NULL, (procedure (*)())NULL);
	  return;
	  }
	break;
      case quadPth: case strokePth:
#if DPSXA
	 if(!strokeOp)
		  BreakUpPath(XAFillPath,path,evenOdd,false);
	 else
#endif /* DPSXA */
        ReducePath(path, evenOdd);
	break;
      case intersectPth: break;
      default: CantHappen();
      }
    if (path->rp && path->rp->devprim)
      MarkDevPrim((DevPrim *)path->rp->devprim, GetDevClipPrim());
} /* end of Fill */

public procedure PSFill()  {Fill(&gs->path, false);  NewPath();}

public procedure PSEOFill()  {Fill(&gs->path, true);  NewPath();}

public procedure Stroke(p) PPath p;
{
if (gs->isCharPath)
  {
  if (chrPthStrk) {
    Path sp;
    sp = StrkPth(p);
    DURING
    AppendPath(&chrPth, &sp);
    HANDLER { FrPth(&sp); RERAISE; }
    END_HANDLER;
    }
  else AppendPath(&chrPth, p);
  }
else {
#if DPSXA
	strokeOp = true;
	BreakUpPath(StrkInternal,p,false,false);
	strokeOp = false;
#else /* DPSXA */
	StrkInternal(p, false);
#endif /* DPSXA */
  };
}  /* end of Stroke */


private Path flatPth;

private procedure FlPthMvTo(c) Cd c;  {MoveTo(c, &flatPth);}

private procedure FlPthLnTo(c) Cd c;  {LineTo(c, &flatPth);}

private procedure FlPthCls()  {ClosePath(&flatPth);}

public Path FltnPth(p, reps) PPath p; real reps;
{
DevCd dummy;
dummy.x = dummy.y = 0;
InitPath(&flatPth);
flatPth.secret = p->secret;
DURING
DoPath(p, FlPthMvTo, FlPthLnTo, FlPthCls, NoOp, NoOp,
       false, reps, false, false, dummy, dummy);
HANDLER
  {FrPth(&flatPth);  RERAISE;}
END_HANDLER;
return flatPth;
} /* end of FltnPth */


public procedure PSFltnPth()
{
Path flatPath;
flatPath = FltnPth(&gs->path, gs->flatEps);
RemPathRef(&gs->path);
gs->path = flatPath;
} /* end of PSFltnPth */


public procedure ReversePath(path) PPath path;
{
PPthElt pe, nextpe, prevpe, startpe, tailpe;
ListPath *pp;
if ((PathType)path->type != listPth) ConvertToListPath(path);
pp = path->ptr.lp;
if (pp == NULL || pp->head == 0) return;
if (pp->refcnt > 1) pp = MakeOwnListPath(path);
tailpe = 0;			/* stays 0 until after close/start */
startpe = pe = pp->head;
nextpe = pe->next;
while (true)
  {
  if (nextpe == 0)
    {
    pe->tag = pathstart;
    startpe->next = NULL; pp->tail = startpe;
    if (tailpe == 0)
      pp->head = pe;
     else
      tailpe->next = pe;
    goto fin;
    }
  pe->tag = nextpe->tag;
  switch (pe->tag)
    {
    case pathstart:
      if (tailpe == 0)
        pp->head = pe;
      else
        tailpe->next = pe;
      tailpe = startpe;
      startpe = pe = nextpe;
      nextpe = pe->next;
      break;
    case pathlineto:
    case pathcurveto:
      prevpe = pe; pe = nextpe;
      nextpe = pe->next;
      pe->next = prevpe;
      break;
    case pathclose:
      pe->tag = pathstart;
      if (tailpe == 0)
        pp->head = pe;
      else
        tailpe->next = pe;
      tailpe = nextpe;
      startpe->next = nextpe;
      nextpe->coord = pe->coord;
      if (nextpe->next == NULL) {pp->tail = nextpe;  goto fin;}
      startpe = pe = nextpe->next;
      nextpe = pe->next;
    }
  }
fin:
pp->start = pe;
if (path == &gs->path) gs->cp = pp->tail->coord;
}  /* end of ReversePath */


public procedure PSReversePath()  {ReversePath(&gs->path);}


public procedure PSCharPath()
{
StrObj strobj;
chrPthStrk = PopBoolean();
InitPath(&chrPth);
gs->isCharPath = true;
chrPth.secret = true;
DURING
PopPString(&strobj);
SimpleShow(strobj);
gs->isCharPath = false;
AppendPath(&gs->path, &chrPth);
MoveTo(gs->cp, &gs->path);
HANDLER
{gs->isCharPath = false;  FrPth(&chrPth);  RERAISE;}
END_HANDLER;
}  /* end of PSCharPath */


public procedure PSClipPath()
{
NewPath();
CopyPath(&gs->path, &gs->clip);
} /* end of PSClipPath */

