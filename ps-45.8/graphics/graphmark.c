/*
  graphmark.c

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
Bill Paxton: Thu Aug 18 16:28:24 1988
Don Andrews: Wed Sep 17 15:28:32 1986
Mike Schuster: Wed Jun 17 11:39:01 1987
Ivor Durham: Wed Aug 17 17:07:30 1988
Jim Sandman: Wed Dec 13 09:08:28 1989
Linda Gass: Tue Dec  8 12:14:11 1987
Paul Rovner: Fri Aug 25 23:27:04 1989
Joe Pasqua: Thu Jan 12 11:16:08 1989
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

extern procedure AddTrap();
extern DevPrim *UnlinkDP();
extern procedure LinkDP();

#if (OS == os_vms)
globaldef PMarkState ms;
globaldef PMarkStateProcs stdMarkProcs;
#else (OS == os_vms)
public PMarkState ms;
public PMarkStateProcs stdMarkProcs;
#endif (OS == os_vms)

public procedure SetTrapBounds(bbox)
  register BBox bbox; {
  register Fixed xl, yl, xg, yg;
  xl = pflttofix(&bbox->bl.x);
  xg = pflttofix(&bbox->tr.x);
  yl = pflttofix(&bbox->bl.y);
  yg = pflttofix(&bbox->tr.y);
  MakeBounds(&ms->trapsDP->bounds, yl, yg, xl, xg);
  ms->haveTrapBounds = true;
  }
  
#if DPSXA
private boolean OverlapChunk(dp0, dp1)
DevPrim *dp0, *dp1;
{
BBoxRec b0, b1;
	b0.bl.x = dp0->xaOffset.x;
	b0.bl.y = dp0->xaOffset.y;
	b0.tr.x = dp0->xaOffset.x + XA_MAX;
	b0.tr.y = dp0->xaOffset.y + XA_MAX;
	b1.bl.x = dp1->xaOffset.x;
	b1.bl.y = dp1->xaOffset.y;
	b1.tr.x = dp1->xaOffset.x + XA_MAX;
	b1.tr.y = dp1->xaOffset.y + XA_MAX;
	return (BBCompare(&b0, &b1) == overlap);
}
#endif /* DPSXA */

public procedure MarkDevPrim(dp, clip) DevPrim *dp, *clip; {
  DevMarkInfo info;
  register PGState g = gs;
  info.color = g->color->color;
  info.halftone = (g->screen == NULL) ? NULL : g->screen->halftone;
#if DPSXA
  info.screenphase.x = g->screenphase.x - (DevLong)dp->xaOffset.x;
  info.screenphase.y = g->screenphase.y - (DevLong)dp->xaOffset.y;
#else /* DPSXA */
  info.screenphase = g->screenphase;
#endif /* DPSXA */
  info.screenphase = g->screenphase;
  info.offset.x = 0;
  info.offset.y = 0;
  info.priv = (DevPrivate *)&g->extension;
  if (clip && clip->type == noneType || dp->type == noneType)
    return;
#if DPSXA
  if(clip == NULL) { /* no clipping */
  	(*g->device->procs->Mark)(g->device, dp, clip, &info);
		return;
	} else {
		DevPrim *pdp, *cdp, *tmp_pdp, *tmp_cdp;
	/* since the path can be a clip path, we need to check all the DevPrims */		
		pdp = dp;	
		while(pdp != NULL) {
  		cdp = clip;
			while(cdp != NULL) {
				if(((pdp->xaOffset.y == cdp->xaOffset.y) 
				  && (pdp->xaOffset.x == cdp->xaOffset.x)) || (OverlapChunk(pdp,cdp)))
						break;
				cdp = cdp->next;
			}
			if((cdp != NULL) && cdp->type != noneType && pdp->type != noneType) {
				/* update info */
  				info.color = g->color;
  				info.halftone = (g->screen == NULL) ? NULL : g->screen->halftone;
  				info.screenphase.x = g->screenphase.x - (DevLong)pdp->xaOffset.x;
  				info.screenphase.y = g->screenphase.y - (DevLong)pdp->xaOffset.y;
  				info.offset.x = 0;
  				info.offset.y = 0;
  				info.priv = (DevPrivate *)&g->extension;
				tmp_pdp = UnlinkDP(pdp);
				tmp_cdp = UnlinkDP(cdp);
  			    (*g->device->procs->Mark)(g->device, pdp, cdp, &info);
				LinkDP(pdp,tmp_pdp);
				LinkDP(cdp,tmp_cdp);
				g = gs;	
			}
			pdp = pdp->next;
		}
	}	
#else /* DPSXA */
  if (clip == dp) clip = NULL;
  (*g->device->procs->Mark)(g->device, dp, clip, &info);
#endif /* DPSXA */
  }

#define ClipForMark() \
((!m->markClip || (m->bbCompMark==inside && DevClipIsRect()))? \
  NULL : GetDevClipPrim())

private procedure StdTrapsFilled(m) register PMarkState m; {
  MarkDevPrim(m->trapsDP, ClipForMark());
  if (!m->haveTrapBounds)
    EmptyDevBounds(&m->trapsDP->bounds);
  m->trapsDP->items = 0;
  }

public procedure StdInitMark(m, clip)
  register PMarkState m; boolean clip; {
  m->markClip = clip;
  m->haveTrapBounds = false;
  EmptyDevBounds(&m->trapsDP->bounds);
  m->trapsDP->items = 0;
  }

public procedure StdTermMark(m) register PMarkState m; {
  if (m->trapsDP->items > 0)
    (*m->procs->trapsFilled)(m);
  }

public procedure AddRunMark(run) DevRun *run; {
  PMarkState m = ms;
  DevPrim aRunPrim;
  InitDevPrim(&aRunPrim, (DevPrim *)NULL);
  aRunPrim.type = runType;
  aRunPrim.items = 1;
  aRunPrim.maxItems = 1;
  aRunPrim.value.run = run;
  aRunPrim.bounds = run->bounds;
#if DPSXA
  aRunPrim.xaOffset = xaOffset;
#endif /* DPSXA */
  MarkDevPrim(&aRunPrim, ClipForMark());
  }

public procedure ShowMask(mask, dc)
  PMask mask; DevCd dc;
  {
  DevPrim aMaskPrim;
  DevMask devChar;
  BBoxCompareResult bbComp;
#if DPSXA
  BBoxRec figbb;
  DevPrim *clipPrim, *tmp;
  DevCd save_dc;
  DevMarkInfo info;
  register PGState g;
  
  figbb.bl.x = dc.x;
  figbb.bl.y = dc.y;
  figbb.tr.x = dc.x + trueMask->width;
  figbb.tr.y = dc.y + trueMask->height;
  bbComp = BBCompare(&figbb, &gs->clip.bbox);
  if (bbComp == outside) return;
  InitDevPrim(&aMaskPrim, (DevPrim *)NULL);
  aMaskPrim.type = (cached) ? stringType : charType;
  aMaskPrim.items = aMaskPrim.maxItems = 1;
  aMaskPrim.value.mask = &devChar;
  devChar.mask = mask;
  if (PathIsRect(&gs->clip) && bbComp == inside) {
		aMaskPrim.bounds.x.l = dc.x;
		aMaskPrim.bounds.x.g = dc.x + trueMask->width;
		aMaskPrim.bounds.y.l = dc.y;
		aMaskPrim.bounds.y.g = dc.y + trueMask->height;
		aMaskPrim.xaOffset.x = 0;
		aMaskPrim.xaOffset.y = 0;
		devChar.dc.x = dc.x;
		devChar.dc.y = dc.y;
		MarkDevPrim(&aMaskPrim, NULL);
  } else {
		clipPrim = (DevPrim *)gs->clip.rp->devprim;
		save_dc = dc;
		while(clipPrim != NULL) {
			dc.x = save_dc.x - clipPrim->xaOffset.x;
			dc.y = save_dc.y - clipPrim->xaOffset.y;
			aMaskPrim.bounds.x.l = dc.x;
			aMaskPrim.bounds.x.g = dc.x + trueMask->width;
			aMaskPrim.bounds.y.l = dc.y;
			aMaskPrim.bounds.y.g = dc.y + trueMask->height;
			aMaskPrim.xaOffset = clipPrim->xaOffset;
			if(BoundsCompare(&aMaskPrim.bounds,&clipPrim->bounds) != outside) {
				g = gs;
  				info.color = g->color;
  				info.halftone = (g->screen == NULL) ? NULL : g->screen->halftone;
  				info.screenphase = g->screenphase;
  				info.offset.x = 0;
  				info.offset.y = 0;
  				info.priv = (DevPrivate *)&g->extension;
				devChar.dc.x = dc.x;
				devChar.dc.y = dc.y;
				tmp = UnlinkDP(clipPrim);
  				(*g->device->procs->Mark)(g->device, &aMaskPrim, clipPrim, &info);
				LinkDP(clipPrim,tmp);
			}
			clipPrim = clipPrim->next;
		}
  }		
#else /* DPSXA */
  DevBBoxRec figbb;
  figbb.bl.x = FixInt(dc.x);
  figbb.bl.y = FixInt(dc.y);
  figbb.tr.x = FixInt(dc.x + mask->width);
  figbb.tr.y = FixInt(dc.y + mask->height);
  bbComp = DevBBCompare(&figbb, GetDevClipDevBBox());
  if (bbComp == outside) return;
  InitDevPrim(&aMaskPrim, (DevPrim *)NULL);
  aMaskPrim.type = maskType;
  aMaskPrim.items = aMaskPrim.maxItems = 1;
  aMaskPrim.value.mask = &devChar;
  MakeBounds(
    &aMaskPrim.bounds, figbb.bl.y, figbb.tr.y, figbb.bl.x, figbb.tr.x);
  devChar.dc.x = dc.x;
  devChar.dc.y = dc.y;
  devChar.mask = mask;
  MarkDevPrim(&aMaskPrim,
    (bbComp==inside && DevClipIsRect())? NULL : GetDevClipPrim());
#endif /* DPSXA */
  }

public procedure DoImageMark(image, bnds)
  DevImage *image; DevBounds *bnds; {
  DevPrim aImagePrim;
  BBoxCompareResult bbComp;
#if DPSXA
  BBoxRec imbb;
  imbb.bl.x = bnds->x.l;
  imbb.bl.y = bnds->y.l;
  imbb.tr.x = bnds->x.g;
  imbb.tr.y = bnds->y.g;
  bbComp = BBCompare(&imbb, &gs->clip.bbox);
#else /* DPSXA */
  DevBBoxRec imbb;
  imbb.bl.x = FixInt(bnds->x.l);
  imbb.bl.y = FixInt(bnds->y.l);
  imbb.tr.x = FixInt(bnds->x.g);
  imbb.tr.y = FixInt(bnds->y.g);
  bbComp = DevBBCompare(&imbb, GetDevClipDevBBox());
#endif /* DPSXA */
  if (bbComp == outside) return;
  InitDevPrim(&aImagePrim, (DevPrim *)NULL);
  aImagePrim.type = imageType;
  aImagePrim.items = 1;
  aImagePrim.bounds = *bnds;
  aImagePrim.value.image = image;
  MarkDevPrim(&aImagePrim, 
    (bbComp==inside && DevClipIsRect())? NULL : GetDevClipPrim());
  }

public procedure StringMark(scip, len, llx, lly, urx, ury)
  DevMask *scip; integer len; Fixed llx, lly, urx, ury; {
  DevPrim aStringPrim;
  DevBBoxRec strbb;
  BBoxCompareResult bbComp;
  strbb.bl.x = llx; strbb.bl.y = lly;
  strbb.tr.x = urx; strbb.tr.y = ury;
  bbComp = DevBBCompare(&strbb, GetDevClipDevBBox());
  if (bbComp == outside) return;
  aStringPrim.type = maskType;
  aStringPrim.next = (DevPrim *)NULL;
  aStringPrim.items = len;
  aStringPrim.maxItems = len;
  aStringPrim.value.string = scip;
  aStringPrim.priv = NULL;
  MakeBounds(&aStringPrim.bounds, lly, ury, llx, urx);
#if DPSXA
  aStringPrim.xaOffset = xaOffset;
#endif /* DPSXA */
  MarkDevPrim(&aStringPrim,
    (bbComp==inside && DevClipIsRect())? NULL : GetDevClipPrim());
  }

private procedure MarkStrokeMasks(devmask, len, llx, lly, urx, ury)
  DevMask* devmask; integer len, llx, lly, urx, ury; {
  DevPrim aStringPrim;
  InitDevPrim(&aStringPrim, (DevPrim *)NULL);
  aStringPrim.type = maskType;
  aStringPrim.items = len;
  aStringPrim.maxItems = len;
  aStringPrim.value.string = devmask;
  MakeBounds(&aStringPrim.bounds,
             FixInt(lly), FixInt(ury),
             FixInt(llx), FixInt(urx));
#if DPSXA
  aStringPrim.xaOffset = xaOffset;
#endif /* DPSXA */
  MarkDevPrim(&aStringPrim, !ms->markClip ? NULL : GetDevClipPrim());
  }

public procedure MarkInit(reason)  InitReason reason;
{
DevPrim *aTrapDevPrim;
switch (reason)
  {
  case init:
    ms = (PMarkState)NEW(1, sizeof(MarkState));
    aTrapDevPrim = InitDevPrim(NewDevPrim(), (DevPrim *)NULL);
    aTrapDevPrim->type = trapType;
    aTrapDevPrim->maxItems = ATRAPLENGTH;
    aTrapDevPrim->value.trap =
               (DevTrap *)NEW(ATRAPLENGTH, sizeof(DevTrap));
    ms->trapsDP = aTrapDevPrim;
    stdMarkProcs = (PMarkStateProcs)NEW(1, sizeof(MarkStateProcs));
    stdMarkProcs->initMark = StdInitMark;
    stdMarkProcs->termMark = StdTermMark;
    stdMarkProcs->trapsFilled = StdTrapsFilled;
    stdMarkProcs->strokeMasksFilled = MarkStrokeMasks;
    ms->procs = stdMarkProcs;
    break;
  case romreg:
    break;
  case ramreg:
    break;
  }
}
