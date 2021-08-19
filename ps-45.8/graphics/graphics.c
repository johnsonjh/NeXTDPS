/*
  graphics.c

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.
Copyright (c) 1988 NeXT, Inc. as an unpublished work.
All Rights Reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Edit History:
Larry Baer: Tue Nov 14 14:41:37 1989
Scott Byer: Thu Jun  1 12:51:47 1989
Ed Taft: Sun Dec 17 17:31:35 1989
Ivor Durham: Fri Sep 23 15:45:14 1988
Bill Paxton: Wed Aug 31 13:54:43 1988
Leo Hourvitz: Thu Jul 07 1988
Jim Sandman: Thu Dec 14 09:47:44 1989
Paul Rovner: Thu Dec 28 17:05:53 1989
Perry Caro: Thu Nov 10 15:44:17 1988
Joe Pasqua: Wed Feb 22 16:31:34 1989
Bill Bilodeau: Fri Oct 13 11:20:35 PDT 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include EXCEPT
#include FP
#include DEVICE
#include GRAPHICS
#include LANGUAGE
#include ORPHANS
#include PSLIB
#include VM

#include "graphdata.h"
#include "path.h"
#include "reducer.h"
#include "graphicspriv.h"
#include "gray.h"
#include "gstack.h"
#include "graphicsnames.h"

#define maxSaveLevel 16
#define RDCBIAS 10
#define FRDCBIAS 655360

#if DPSXA
QuadPath *chunkqp;
BBoxRec chunkBBox;
unsigned long int maxXChunk, maxYChunk;
long int xChunkOffset, yChunkOffset;
#endif DPSXA

public procedure InitGraphics(); /* forward declaration */

private integer rdcbias, frdcbias;

extern Mtx IdentMtx;
public PNameObj graphicsNames;
public boolean minTrapPrecision;
public Fixed edgeminx, edgeminy, edgemaxx, edgemaxy;
private FltnRec fr;

/*	----- Context for the graphics package -----	*/

public PGraphicsData graphicsStatics;

/* general declarations */


public procedure CrMtx(m) PMtx m; {*m = gs->matrix;}


public procedure PSCrMtx()
{
AryObj ao;
PopPArray(&ao);
PushPMtx(&ao, &gs->matrix);
}  /* end of PSCrMtx */



public procedure DfMtx(m)  PMtx m;  {
  (*gs->device->procs->DefaultMtx)(gs->device, m);}


public procedure PSDfMtx()
{
AryObj a;
Mtx m;
PopPArray(&a);
DfMtx(&m);
PushPMtx(&a, &m);
} /* end of PSDfMtx */

public procedure SetMtx(pmtx) PMtx pmtx;
{
  gs->matrix = *pmtx;
  gs->matrixID.stamp = MIDNULL;
  gs->devhlw = fpZero;
}

public procedure PSSetMtx()
{Mtx mtx;  PopMtx(&mtx);  SetMtx(&mtx);}


public procedure Cnct(m)  PMtx m;
{
MtxCnct(m, &gs->matrix, &gs->matrix);
gs->matrixID.stamp = MIDNULL;
gs->devhlw = fpZero;
} /* end of Cnct */


public procedure PSCnct()
{
  Mtx     m;
  PopMtx (&m);
  Cnct (&m);
}


public procedure Tlat(c)  Cd c;
{
Mtx t;
UniqueMID matrixID;
real devhlw = gs->devhlw;
matrixID = gs->matrixID;
TlatMtx(&c.x, &c.y, &t);
Cnct(&t);
gs->matrixID = matrixID;
gs->devhlw = devhlw;
} /* end of Tlat */


public procedure Scal(c)  Cd c;
{
    Mtx s;
    ScalMtx(&c.x, &c.y, &s);
    Cnct(&s);
}


public procedure Rtat(ang)  Preal ang;
{
Mtx r;
real devhlw = gs->devhlw;
RtatMtx(ang, &r);
Cnct(&r);
gs->devhlw = devhlw;
} /* end of Rtat */


public procedure RTfmPCd(c, m, cur, rc)  Cd c; PMtx m; Cd cur; PCd rc;
{DTfmPCd(c, m, rc);  VecAdd(*rc, cur, rc);}

public procedure TfmP(c, ct)  Cd c; PCd ct;
{
    TfmPCd(c, &gs->matrix, ct);
} /* TfmP */


public procedure DTfmP(c, rc) Cd c; PCd rc;
{DTfmPCd(c, &gs->matrix, rc);}


public procedure ITfmP(c, rc) Cd c; PCd rc;
{ITfmPCd(c, &gs->matrix, rc);}


public procedure IDTfmP(c, rc)  Cd c; PCd rc;
{IDTfmPCd(c, &gs->matrix, rc);}


public procedure PSCrPoint()
{
Cd cd;
CheckForCurrentPoint(&gs->path);
ITfmP(gs->cp, &cd);
PushPCd(&cd);
}  /* end of PSCrPoint */

public procedure InitMtx()
{
  DfMtx (&gs->matrix);
  gs->matrixID.stamp = MIDNULL;
  gs->devhlw = fpZero;
}


#if DPSXA

private procedure InitChunk()
{
register DevPrim *clipPrim;
  if(chunkqp == NULL) {
  	DURING
		chunkqp = (QuadPath *)os_newelement(qpStorage);
  	HANDLER {
    	if (chunkqp) os_freeelement(qpStorage, (char *)chunkqp);
    	RERAISE;
    }
  	END_HANDLER;
   }
    chunkqp->c0.x = chunkqp->c3.x = chunkBBox.bl.x = 
    								xChunkOffset < 0 ? xChunkOffset : 0;
    chunkqp->c0.y = chunkqp->c1.y = chunkBBox.bl.y = 
    								yChunkOffset < 0 ? yChunkOffset : 0;
    chunkqp->c1.x = chunkqp->c2.x = chunkBBox.tr.x = 
    								xChunkOffset < 0 ? 0 : xChunkOffset;
    chunkqp->c2.y = chunkqp->c3.y = chunkBBox.tr.y = 
    								yChunkOffset < 0 ? 0 : yChunkOffset;
}

public procedure SetXABounds(pBounds)
DevLBounds *pBounds;
{
DevLong x,y;
long int maxX, maxY;


	x = pBounds->x.g - pBounds->x.l;
	y = pBounds->y.g - pBounds->y.l;
	maxX = os_labs((x/CHUNKSIZE)) - 1;
	maxY = os_labs(y/CHUNKSIZE);
	if((x % CHUNKSIZE) && (x > CHUNKSIZE)) maxX++;
	if((y % CHUNKSIZE) && (x > CHUNKSIZE)) maxY++;
	if(maxX <= 0) maxXChunk = 0;
	else maxXChunk = maxX;
	if(maxY <= 0) maxYChunk = 1;
	else maxYChunk = maxY;
	
	/* set chunk offset according to device quadrant */
    if(pBounds->x.g == 0)
		xChunkOffset = -CHUNKSIZE;
	else
		xChunkOffset = CHUNKSIZE;
    if(pBounds->y.g == 0)
		yChunkOffset = -CHUNKSIZE;
	else
		yChunkOffset = CHUNKSIZE;
		
	InitChunk();
}
	

/* 
	Reduces the clip for each chunk in the device space, producing
	a list of DevPrims with different xaOffsets
*/
public procedure XAReducePath(RdcProc,path,evenOdd)
void (*RdcProc)();
PPath path;
boolean evenOdd;
{
DevPrim *path_dp, *dp, *list = NULL;
register int	x,y;
Cd xydelta, delta;
DevCd offset;

	/* some routines initialize the rp in a way that can't be used by extended
	   addressing. Therefore, the rp must be removed and deallocated.
	*/
	if (path->rp != NULL) { 
    	TermClipDevPrim((DevPrim *)path->rp->devprim);
      	/* might have been used as clip */
    	DisposeDevPrim((DevPrim *)path->rp->devprim);
    	os_freeelement(rpStorage, (char *)path->rp);
		path->rp = NULL;
    }
	offset.x = 0; offset.y = 0;
	xydelta.x = -xChunkOffset;
	xydelta.y = -yChunkOffset;
	delta.x = xydelta.x;
	delta.y = 0;
	for(y=0; y<maxYChunk; y++) {
		for(x=0; x<=maxXChunk; x++) {
			if(BBCompare(&path->bbox, &chunkBBox) != outside) {
				RdcProc(path, evenOdd);
				if(path->rp != NULL) {
					dp = path_dp = (DevPrim *)path->rp->devprim;
					if((dp != NULL) && ((dp->type != noneType) || (list == NULL))) {
						dp->xaOffset = offset;
						while(dp->next != NULL) {
							dp = dp->next;
							dp->xaOffset = offset;
						}
						/* add DevPrim to list */
						dp->next = list; 						
						list = path_dp;
					}
    				os_freeelement(rpStorage, (char *)path->rp);
					path->rp = NULL;
				}
			}
			offset.x += xChunkOffset;
			TlatPath(path,delta);
		}
		offset.x = 0;
		offset.y += yChunkOffset;
		delta.x = (maxXChunk+1) * -xydelta.x;
		delta.y = xydelta.y;
		TlatPath(path,delta);
		delta.x = xydelta.x;
		delta.y = 0;
	}
	delta.x = 0;
	delta.y = maxYChunk * -xydelta.y;
	TlatPath(path,delta);
	if(list == NULL) {
		/* create a placeholder for a null clip region */
			DevPrim *tmpDP;
  			DURING
  			tmpDP = (DevPrim *)NewDevPrim();
  			HANDLER {
    			if (tmpDP) DisposeDevPrim(tmpDP);
    		RERAISE;
    		}
  			END_HANDLER;
			InitDevPrim(tmpDP, (DevPrim *)NULL);
			list = tmpDP;
	}
    PutRdc(path,list,evenOdd);
} /* end of XAReducePath */

public procedure InitClipPath(path) register PPath path; {
  register QuadPath *qp = NULL;
  DevLBounds bounds;
  FrPth(path);
  DURING
  qp = (QuadPath *)os_newelement(qpStorage);
  path->rp = (ReducedPath *)os_newelement(rpStorage);
  HANDLER {
    if (qp) os_freeelement(qpStorage, (char *)qp);
    RERAISE;
    }
  END_HANDLER;
  path->rp->refcnt = 1;
  path->rp->devprim = NULL;
  path->ptr.qp = qp;
  qp->refcnt = 1;
  (*gs->device->procs->DefaultBounds)(gs->device, &bounds);
  path->bbox.bl.y = qp->c0.y = qp->c1.y = bounds.y.l;
  path->bbox.tr.y = qp->c2.y = qp->c3.y = bounds.y.g;
  path->bbox.bl.x = qp->c0.x = qp->c3.x = bounds.x.l;
  path->bbox.tr.x = qp->c1.x = qp->c2.x = bounds.x.g;
  path->type = (BitField)quadPth;
  path->secret = path->eoReduced = false;
  path->checkedForRect = true;
  path->isRect = true;
  path->length = 5;
  SetXABounds(&bounds);
  XAReducePath(ReducePath,path,false);
  } /* end of InitClipPath */
  
#else DPSXA


public procedure InitClipPath(path) register PPath path; {
  register DevPrim *dp = NULL;
  register QuadPath *qp = NULL;
  register DevTrap *t = NULL; /* makes trapezoid directly from bounds */
  DevLBounds bounds;
  FrPth(path);
  DURING
  dp = (DevPrim *)NewDevPrim();
  t = (DevTrap *)NEW(1, sizeof(DevTrap));
  qp = (QuadPath *)os_newelement(qpStorage);
  path->rp = (ReducedPath *)os_newelement(rpStorage);
  HANDLER {
    if (dp) DisposeDevPrim(dp);
    if (t) FREE(t);
    if (qp) os_freeelement(qpStorage, (char *)qp);
    RERAISE;
    }
  END_HANDLER;
  path->rp->refcnt = 1;
  path->rp->devprim = NULL;
  path->rp->devprim = (char *)dp;
  dp->type = trapType;
  dp->next = NULL;
  dp->items = 1;
  dp->maxItems = 1;
  dp->value.trap = NULL;
  dp->value.trap = t;
  path->ptr.qp = qp;
  qp->refcnt = 1;
  (*gs->device->procs->DefaultBounds)(gs->device, &bounds);
  path->bbox.bl.y = qp->c0.y = qp->c1.y = t->y.l = dp->bounds.y.l =
    bounds.y.l;
  path->bbox.tr.y = qp->c2.y = qp->c3.y = t->y.g = dp->bounds.y.g =
    bounds.y.g;
  path->bbox.bl.x = qp->c0.x = qp->c3.x = t->l.xl = t->l.xg = dp->bounds.x.l =
    bounds.x.l;
  path->bbox.tr.x = qp->c1.x = qp->c2.x = t->g.xl = t->g.xg = dp->bounds.x.g =
    bounds.x.g;
  path->type = (BitField)quadPth;
  path->secret = path->eoReduced = false;
  path->checkedForRect = true;
  path->isRect = true;
  path->length = 5;
  } /* end of InitClipPath */

#endif /* DPSXA */

public procedure InitClip() { InitClipPath(&gs->clip); }

public procedure PSErasePage()
{
real gray = fpOne;
GSave();  
SetGray(&gray);
(*gs->device->procs->InitPage)(gs->device, gs->color->color);
GRstr();
}  /* end of ErasePage */

public procedure PSShowPage()
{
real gray = fpOne;
if ((*gs->device->procs->ShowPage)(gs->device, true, 1, NIL))
  return;
SetGray(&gray);
(*gs->device->procs->InitPage)(gs->device, gs->color->color);
InitGraphics();
} /* end of PSShowPage */


public procedure PSCopyPage()
{
if ((*gs->device->procs->ShowPage)(gs->device, false, 1, NIL)) 
  return;
}  /* end of PSCopyPage */

public procedure SetScal()
{
integer maxCd;
DevLBounds bounds;
(*gs->device->procs->DefaultBounds)(gs->device, &bounds);
bounds.x.g = MAX(os_abs(bounds.x.l), os_abs(bounds.x.g));
bounds.y.g = MAX(os_abs(bounds.y.l), os_abs(bounds.y.g));
maxCd = MAX(bounds.x.g, bounds.y.g);
maxCd += RDCBIAS;  maxCd += RDCBIAS;
gs->scale = 0;
if (maxCd == 0) return;
while (maxCd < 16384) {gs->scale++; maxCd *= 2;}
rdcbias = RDCBIAS; frdcbias = FRDCBIAS;
} /* end of SetScal */


public procedure SetRdcScal(maxval, minval) integer maxval, minval;
{
register integer delta, scale, t;
maxval += RDCBIAS;
minval -= RDCBIAS;
delta = maxval - minval;
scale = 0;
if (delta > 0) {
  t = 16384;
  while (delta < t) {scale++; delta <<= 1;}
  }
gs->scale = scale;
rdcbias = -minval;
frdcbias = FixInt(rdcbias);
} /* end of SetRdcScal */


public Fixed DevToRdc(d)  double d;
{return FTrunc((dbltofix(d) + frdcbias) << gs->scale);}


private Fixed FDevToRdc(f)  Fixed f;
{return FTrunc((f + frdcbias) << gs->scale);}


public Fixed RdcToDev(r)  Fixed r;
{return (r >> gs->scale) - frdcbias;}


public procedure ClNewPt(coord)  Cd coord;
{NewPoint(DevToRdc(coord.x), DevToRdc(coord.y));}

public procedure FClNewPt(c) FCd c; {
  if (c.x < edgeminx) edgeminx = c.x;
  if (c.y < edgeminy) edgeminy = c.y;
  if (c.x > edgemaxx) edgemaxx = c.x;
  if (c.y > edgemaxy) edgemaxy = c.y;
  NewPoint(FDevToRdc(c.x), FDevToRdc(c.y));}

public procedure PFClNewPt(c) PFCd c; {FClNewPt(*c);}

public procedure FontPathBBox(ll, ur) PFCd ll, ur; {
  if (edgeminx == MAXinteger) {
    ll->x = ll->y = ur->x = ur->y = 0;
    }
  else {
    ll->x = FTruncF(edgeminx);
    ll->y = FTruncF(edgeminy);
    ur->x = FCeilF(edgemaxx);
    ur->y = FCeilF(edgemaxy);
    }
  }
 
public procedure SetBBCompMark(delta)
PFCd delta;
{
BBoxRec bbox;
	bbox.bl.x = FTrunc(edgeminx) + FTrunc(delta->x);
	bbox.bl.y = FTrunc(edgeminy) + FTrunc(delta->y);
	bbox.tr.x = FTrunc(edgemaxx) + FCeil(delta->x);
	bbox.tr.y = FTrunc(edgemaxy) + FCeil(delta->y);
	ms->bbCompMark = BBCompare(&bbox, GetDevClipBBox());
}

public procedure InitFontFlat(proc) PVoidProc proc; {
  edgeminx = edgeminy = MAXinteger;
  edgemaxx = edgemaxy = MINinteger;
  fr.report = proc;
  fr.reportFixed = true;
  fr.limit = FLATTENLIMIT;
  fr.feps = 0x3000;
  }

public procedure FFCurveTo(c0, c1, c2, c3) PFCd c0, c1, c2, c3; {
  fr.llx = fr.lly = 0;
  FFltnCurve(*c0, *c1, *c2, *c3, &fr, true);
  }

public procedure BBInt(bb1, bb2)
    register BBox bb1;
    register BBox bb2;
    {
    bb1->bl.x = bb1->bl.x > bb2->bl.x ? bb1->bl.x : bb2->bl.x;
    bb1->bl.y = bb1->bl.y > bb2->bl.y ? bb1->bl.y : bb2->bl.y;
    bb1->tr.x = bb1->tr.x < bb2->tr.x ? bb1->tr.x : bb2->tr.x;
    bb1->tr.y = bb1->tr.y < bb2->tr.y ? bb1->tr.y : bb2->tr.y;
    }

public procedure GetBBoxFromDevBounds(bb, bounds)
  register BBox bb; register DevBounds *bounds; {
  bb->bl.x = (real)(bounds->x.l);
  bb->bl.y = (real)(bounds->y.l);
  bb->tr.x = (real)(bounds->x.g) - .00001;
  bb->tr.y = (real)(bounds->y.g) - .00001;
  }

public procedure GetBBoxFromDevLBounds(bb, bounds)
  register BBox bb; register DevLBounds *bounds; {
  bb->bl.x = (real)(bounds->x.l);
  bb->bl.y = (real)(bounds->y.l);
  bb->tr.x = (real)(bounds->x.g) - .00001;
  bb->tr.y = (real)(bounds->y.g) - .00001;
  }

public BBoxCompareResult BBCompare(figbb, clipbb) register BBox figbb, clipbb;
{
if (figbb->bl.y >= clipbb->bl.y && figbb->tr.y <= clipbb->tr.y
   && figbb->bl.x >= clipbb->bl.x && figbb->tr.x <= clipbb->tr.x)
   return inside;
if (figbb->tr.y <= clipbb->bl.y || figbb->bl.y >= clipbb->tr.y
   || figbb->tr.x <= clipbb->bl.x || figbb->bl.x >= clipbb->tr.x)
   return outside;
return overlap;
} /* end of BBCompare */


public BBoxCompareResult DevBBCompare(figbb, clipbb)
  register DevBBox figbb, clipbb; /* fixed point version */
{
if (figbb->bl.y >= clipbb->bl.y && figbb->tr.y <= clipbb->tr.y
   && figbb->bl.x >= clipbb->bl.x && figbb->tr.x <= clipbb->tr.x)
   return inside;
if (figbb->tr.y <= clipbb->bl.y || figbb->bl.y >= clipbb->tr.y
   || figbb->tr.x <= clipbb->bl.x || figbb->bl.x >= clipbb->tr.x)
   return outside;
return overlap;
} /* end of DevBBCompare */

public procedure MinimumClip()
{
PPath clip = &gs->clip;
DevTrap *t=NULL;
DevPrim *dp;
Cd cp;

  FrPth(clip);
  if(HasCurrentPoint(&gs->path)) 
  	cp = gs->cp;
  else
  	cp.x = cp.y = 0;
  MoveTo(cp, clip);
  ClosePath(clip);
  /* clip is expected to have an rp */
  DURING
  dp = (DevPrim *)NewDevPrim();
  t = (DevTrap *)NEW(1, sizeof(DevTrap));
  clip->rp = (ReducedPath *)os_newelement(rpStorage);
  HANDLER {
    if (dp) DisposeDevPrim(dp);
    if (t) FREE(t);
    if(clip->rp) os_freeelement(rpStorage, (char *)clip->rp);
    RERAISE;
  }
  END_HANDLER;
  clip->rp->refcnt = 1;
  clip->rp->devprim = (char *)dp;
  dp->type = trapType;
  dp->bounds.x.l = dp->bounds.x.g = cp.x;
  dp->bounds.y.l = dp->bounds.y.g = cp.y;
  dp->next = NULL;
  dp->items = 1;
  dp->maxItems = 1;
  dp->value.trap = NULL;
  dp->value.trap = t;
  t->y.l = t->y.g = gs->cp.y;
  t->l.xl = t->l.xg = t->g.xl = t->g.xg = gs->cp.x;
  clip->secret = clip->eoReduced = false;
}

#if DPSXA

public DevPrim *UnlinkDP(dp)
DevPrim *dp;
{
DevPrim *prev;
DevCd offset;

	offset = dp->xaOffset;
	while((dp != NULL) 
		&& (dp->xaOffset.y == offset.y) && (dp->xaOffset.x == offset.x)) {
			prev = dp;
			dp = dp->next;
	}
	prev->next = NULL;
	return(dp);
}

public procedure LinkDP (dp, nextDP)
DevPrim *dp, *nextDP;
{
	while (dp->next != NULL) dp = dp->next;
	dp->next = nextDP;
}

public procedure ReducePathClipInt(path, evenOdd)
register PPath path; boolean evenOdd; {
register PPath clip = &gs->clip;
  if (PathIsRect(clip) && BBCompare(&path->bbox,&clip->bbox)==inside) {
    XAReducePath(ReducePath, path, evenOdd);
    RemPathRef(clip);
    CopyPath(clip, path);
  } else {
    DevPrim *clipPrim, *cdp, *pdp, *tmp_cdp, *tmp_pdp;
    register IntersectPath *ip;
    ip = (IntersectPath *)os_newelement(ipStorage);
    ip->refcnt = 1;
    ip->evenOdd = evenOdd;
    ip->clip = *clip;
    InitPath(&ip->path);
    InitPath(clip);
    clip->type = (BitField)intersectPth;
    clip->ptr.ip = ip;
    clip->rp = (ReducedPath *)os_newelement(rpStorage);
    clip->rp->refcnt = 1;
    clip->rp->devprim = NULL;
    CopyPath(&ip->path, path);
    XAReducePath(ReducePath, &ip->path, evenOdd);
    cdp = (DevPrim *)ip->clip.rp->devprim;
    while(cdp != NULL) {
    	pdp = (DevPrim *)ip->path.rp->devprim;
			while(pdp != NULL) {
				if((pdp->xaOffset.y == cdp->xaOffset.y) 
					&& (pdp->xaOffset.x == cdp->xaOffset.x))
						break;
				pdp = pdp->next;
			}
			if(pdp != NULL) {
				tmp_pdp = UnlinkDP(pdp);
				tmp_cdp = UnlinkDP(cdp);
				clipPrim = ClipDevPrim(cdp,pdp);
				LinkDP(pdp,tmp_pdp);
				LinkDP(cdp,tmp_cdp);
				clipPrim->xaOffset = cdp->xaOffset;
				tmp_cdp = clipPrim;
				while(tmp_cdp->next != NULL) tmp_cdp = tmp_cdp->next;
				tmp_cdp->next = (DevPrim *)clip->rp->devprim;
				clip->rp->devprim = (char *)clipPrim;
			}
			cdp = cdp->next;
		}
	    if(clip->rp->devprim == NULL) {
		/* create a placeholder for a null clip region */
			DevPrim *tmpDP=NULL;
  			DURING
  			tmpDP = (DevPrim *)NewDevPrim();
  			HANDLER {
    			if (tmpDP) DisposeDevPrim(tmpDP);
    		RERAISE;
    		}
  			END_HANDLER;
			InitDevPrim(tmpDP, (DevPrim *)NULL);
			clip->rp->devprim = (char *)tmpDP;
			clip->bbox.bl.x = clip->bbox.bl.y = 0;
			clip->bbox.tr.x = clip->bbox.tr.x = 0;
	    } else {
			clip->bbox = ip->clip.bbox;
			BBInt(&clip->bbox, &ip->path.bbox);
		}
	}
}

#else DPSXA

public procedure ReducePathClipInt(path, evenOdd)
  register PPath path; boolean evenOdd; {
  register PPath clip = &gs->clip;
  if (PathIsRect(clip) && BBCompare(&path->bbox,&clip->bbox)==inside) {
    RemPathRef(clip);
  	if(!HasCurrentPoint(path)) MinimumClip(); /* clip from empty path */
	else {
	  CopyPath(clip, path);
	  ReducePath(clip, evenOdd);
	  AddRunIndexes((DevPrim *)clip->rp->devprim);
	}
  }
  else {
    DevPrim *clipPrim;
    DevTrap *t;
    DevBounds bounds;
    register IntersectPath *ip;
    ip = (IntersectPath *)os_newelement(ipStorage);
    ip->refcnt = 1;
    ip->evenOdd = evenOdd;
    ip->clip = *clip;
    InitPath(&ip->path);
    InitPath(clip);
    clip->type = (BitField)intersectPth;
    clip->ptr.ip = ip;
    clip->rp = (ReducedPath *)os_newelement(rpStorage);
    clip->rp->refcnt = 1;
    clip->rp->devprim = NULL;
    CopyPath(&ip->path, path);
    ReducePath(&ip->path, evenOdd);
    clipPrim = ClipDevPrim(
      (DevPrim *)ip->clip.rp->devprim, (DevPrim *)ip->path.rp->devprim);
    if(clipPrim->items == 0) {
    	DisposeDevPrim(clipPrim);
    	MinimumClip();
	} else {
    	clip->rp->devprim = (char *)clipPrim;
		FullBounds(clipPrim, &bounds);
		GetBBoxFromDevBounds(&clip->bbox, &bounds);
	}
   }
  }

#endif /* DPSXA */

public procedure Clip(evenOdd)  boolean evenOdd;
{
integer bbcomp;
boolean secret;
secret = gs->clip.secret;
ReducePathClipInt(&gs->path, evenOdd);
gs->clip.secret = secret || gs->path.secret;
} /* end of Clip */


public procedure PSClip() {Clip(false);}

public procedure PSEOClip() {Clip(true);}

public procedure MakeRectPath(path) PPath path; {
  Cd userPt;
  real userw, userh;
  QuadPath *qp = NULL;
  DevPrim *dp = NULL;
  DevTrap *t = NULL;
  DURING
  qp = (QuadPath *)os_newelement(qpStorage);
  dp = (DevPrim *)NewDevPrim();
  t = (DevTrap *)NEW(7, sizeof(DevTrap));
  HANDLER {
    if (qp) os_freeelement(qpStorage, (char *)qp);
    if (dp) DisposeDevPrim(dp);
    if (t) FREE(t);
    RERAISE;
    }
  END_HANDLER;
  dp->type = trapType;
  dp->next = NULL;
  dp->items = 0;
  dp->maxItems = 7;
  dp->value.trap = t;
  PopPReal(&userh);
  PopPReal(&userw);
  PopPReal(&userPt.y);
  PopPReal(&userPt.x);
  ReduceQuadPath(userPt, userw, userh, &gs->matrix, dp, qp, (BBox)NULL, path);
  }


private procedure RdBytesCopy(p, nbytes, arg)
  PCard8 p; integer nbytes; char *arg;
{
os_bcopy((char *)p, *(char **)arg, nbytes);
*(PCard8 *)arg += nbytes;
}

public procedure RdBytes(xbyte, ybit, wbytes, hbits, s)
  integer xbyte, ybit, wbytes, hbits;  character *s;
{
character *stmp = s;
integer outcome;
if (wbytes < 0 || hbits < 0 || xbyte < 0 || ybit < 0) RangeCheck();
outcome = (*gs->device->procs->ReadRaster)
  (gs->device, xbyte, ybit, wbytes, hbits, RdBytesCopy, (char *) &stmp);
if (outcome != 0)
  if (outcome == 1) LimitCheck(); else RangeCheck();
}  /* end of RdBytes */


#if STAGE==DEVELOP
private procedure PSRdBytes()
{
integer xbyte, ybit, wbytes, hbits;
StrObj strobj;
PopPString(&strobj);
hbits = PopInteger();
wbytes = PopInteger();
ybit = PopInteger();
xbyte = PopInteger();
if (wbytes * hbits > strobj.length) RangeCheck();
RdBytes(xbyte, ybit, wbytes, hbits, strobj.val.strval);
} /* end of PSRdBytes */
#endif STAGE==DEVELOP


public procedure PSSetFlatThreshold()
{
real flatEps;
PopPReal(&flatEps);
gs->flatEps = (flatEps < fpp2) ? fpp2 : ((flatEps > fp100) ? fp100 : flatEps);
}  /* end of PSSetFlatThreshold */


public procedure PSCrFlatThreshold() {PushPReal(&gs->flatEps);}

public procedure PSWTranslation() {
  DevPoint p;
  p.x = 0; p.y = 0;
  (*gs->device->procs->WinToDevTranslation)(gs->device, &p);
  PushInteger(p.x);
  PushInteger(p.y);
  }

typedef struct {
  DeviceInfoArgs args;
  DictObj dict;
  } RealDIArgs;
  
private procedure AddEntry(key, val, args)
  char *key; Object val; DeviceInfoArgs *args; {
  RealDIArgs *ra = (RealDIArgs *)args;
  NameObj keyObj;
  if (ra->dict.type != dictObj) DictP((cardinal)ra->args.dictSize, &ra->dict);
  FastName(key, os_strlen(key), &keyObj);
  DictPut(ra->dict, keyObj, val);
  }

private procedure AddIntEntry(key, val, args)
  char *key; integer val; DeviceInfoArgs *args; {
  IntObj valObj;
  LIntObj(valObj, val);
  AddEntry(key, valObj, args);
  }

private procedure AddRealEntry(key, val, args)
  char *key; Preal val; DeviceInfoArgs *args; {
  RealObj valObj;
  LRealObj(valObj, *val);
  AddEntry(key, valObj, args);
  }

private procedure AddStringEntry(key, val, args)
  char *key, val; DeviceInfoArgs *args; {
  StrObj valObj;
  valObj = MakeStr(val);
  AddEntry(key, valObj, args);
  }

public procedure PSDeviceInfo() {
  RealDIArgs ra;
  if (gs->noColorAllowed) Undefined(); /* not allowed in mask device */
  ra.args.dictSize = 0;
  ra.args.AddIntEntry = AddIntEntry;
  ra.args.AddRealEntry = AddRealEntry;
  ra.args.AddStringEntry = AddStringEntry;
  LNullObj(ra.dict);
  (*gs->device->procs->DeviceInfo)(gs->device, &ra.args);
  if (ra.dict.type == nullObj)
    DictP(0, &ra.dict);
  SetDictAccess(ra.dict, rAccess);
  PushP(&ra.dict);
  }


public procedure IniGraphics(reason)  InitReason reason;
{
switch (reason)
  {
  case init:
#if DPSXA
	chunkqp = NULL;
#endif /* DPSXA */
    break;
  case romreg:
    graphicsNames =
      RgstPackageNames((integer)PACKAGE_INDEX, (integer)NUM_PACKAGE_NAMES);
#include "ops_graphicsDPS.c"
    minTrapPrecision = DevMinimizeTrapPrecision();
    break;
  case ramreg:
    break;
  }
} /* end of IniGraphics */
