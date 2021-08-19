/*
  viewclip.c

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
Joe Pasqua: Tue Jan 17 13:48:28 1989
Ivor Durham: Sat May  6 14:59:25 1989
Bill Paxton: Thu Sep 15 09:37:37 1988
Jim Sandman: Mon Nov  7 15:09:48 1988
Ed Taft: Thu Jul 28 16:47:18 1988
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

#include "viewclip.h"
#include "graphdata.h"
#include "gray.h"
#include "graphicspriv.h"
#include "path.h"

extern BBoxCompareResult BBCompare( /* BBox figbb, clipbb; */ );

/* 
  viewclip declarations

  No data handler necessary.  Initial NIL values guaranteeed on creation.
 */

#define	viewClips	(graphicsStatics->viewClip._viewClips)
#define	curVC		(graphicsStatics->viewClip._curVC)

/* the viewclip cache is shared among all PS contexts */
#define viewclipcacheMax (20)

typedef struct _viewclipce {
  struct _viewclipce *next;
  DevPrim *viewclip;	/* the reduction of the viewclip path */
  DevPrim *clip;	/* the reduction of the clip path */
  DevPrim *intersection; /* the intersection of the viewclip and the clip */
  boolean isRect:8;	/* true iff the intersection is a rectangle */
  boolean isEasy:8;	/* true iff the intersection == clip or viewclip */
  BBoxRec bbox;		/* the bbox for the intersection */
  DevBBoxRec devbbox;	/* the bbox in fixed point representation */
  } ViewClipCacheEntry, *PViewClipCacheEntry;

/* private global variables. make change in both arms if changing variables */

#if (OS != os_mpw)

/*-- BEGIN GLOBALS --*/
private Pool vcStorage;	/* Blind ptr to strg pool for ViewClips	*/
private Pool vcCacheStorage;	/* pool for view clip cache entries */
private PViewClipCacheEntry viewclipCache;
private Card16 viewclipcachelength;
/*-- END GLOBALS --*/

#else (OS != os_mpw)

typedef struct {
  Pool g_vcStorage;	/* Blind ptr to strg pool for ViewClips	*/
  Pool g_vcCacheStorage;	/* pool for view clip cache entries */
  PViewClipCacheEntry g_viewclipCache;
  Card16 g_viewclipcachelength;
  } GlobalsRec, *Globals;
  
private Globals globals;

#define vcStorage globals->g_vcStorage
#define vcCacheStorage globals->g_vcCacheStorage
#define viewclipCache globals->g_viewclipCache
#define viewclipcachelength globals->g_viewclipcachelength

#endif (OS != os_mpw)


private procedure TermViewClipIntersection(ce)
PViewClipCacheEntry ce;
{
  Assert(ce->intersection != ce->clip &&
         ce->intersection != ce->viewclip);
  DevFlushClip(ce->intersection);
  DisposeDevPrim(ce->intersection);
} /* TermViewClipIntersection */

private procedure TermViewClip(vc, freevc)
PViewClip vc; boolean freevc;
{
  register PViewClipCacheEntry ce, next, prev;
  DevPrim *vcprim;
  if (vc == NULL) return;
  /* flush cache of entries using this viewclip */
  vcprim = (DevPrim *)(vc->path.rp->devprim);
  DevFlushClip(vcprim);
  ce = viewclipCache; prev = NULL;
  while (ce != NULL) {
    next = ce->next;
    if (ce->viewclip != vcprim) prev = ce;
    else {
      if (prev == NULL) viewclipCache = next;
      else prev->next = next;
      if (!ce->isEasy) TermViewClipIntersection(ce);
      os_freeelement(vcCacheStorage, (char *)ce);
      viewclipcachelength--;
      }
    ce = next;
    }
  RemPathRef(&vc->path);
  if (freevc) os_freeelement(vcStorage, (char *)vc);
  if (vc == curVC) curVC = NULL;
} /* TermViewClip */

public procedure TermClipDevPrim(clip)
register DevPrim *clip;
{
  register PViewClipCacheEntry ce, next, prev;
  /* called by grestore, clip, initclip, and cliprect */
  if (clip == NULL) return;
  DevFlushClip(clip);
  ce = viewclipCache; prev = NULL;
  while (ce != NULL) {
    next = ce->next;
    if (ce->clip != clip) prev = ce;
    else {
      if (prev == NULL) viewclipCache = next;
      else prev->next = next;
      if (!ce->isEasy) TermViewClipIntersection(ce);
      os_freeelement(vcCacheStorage, (char *)ce);
      viewclipcachelength--;
      }
    ce = next;
    }
} /* TermClipDevPrim */

private PViewClipCacheEntry FindCurrentViewInCache()
{
  register PViewClipCacheEntry ce, prev;
  register DevPrim *clip, *vclip;
  PPath vcpath;
  BBoxCompareResult bbcomp;
  DevBounds bounds;
  
  Assert(curVC != NULL);
  clip = (DevPrim *)(gs->clip.rp->devprim);
  vclip = (DevPrim *)(curVC->path.rp->devprim);
  ce = viewclipCache; prev = NULL;
  while (ce != NULL) {
    if (ce->viewclip == vclip && ce->clip == clip) {
      if (prev != NULL) { /* move ce to front of list */
        prev->next = ce->next;
	ce->next = viewclipCache;
	viewclipCache = ce;
        }
      return ce;
      }
    prev = ce;
    ce = ce->next;
    }
  /* put it in the cache */
  if (viewclipcachelength == viewclipcacheMax) { /* free oldest */
    ce = viewclipCache;
    while (ce->next != NULL) {prev = ce; ce = ce->next;}
    if (!ce->isEasy && prev->isEasy) *prev = *ce;
    else {
      prev->next = NULL;
      if (!ce->isEasy) TermViewClipIntersection(ce);
      }
    }
  else {
    viewclipcachelength++;
    ce = (PViewClipCacheEntry)os_newelement(vcCacheStorage);
    }
  ce->viewclip = vclip;
  ce->clip = clip;
  ce->next = viewclipCache;
  viewclipCache = ce;
  vcpath = &curVC->path;
  bbcomp = BBCompare(&vcpath->bbox, &gs->clip.bbox);
  if (bbcomp == outside) { /* null intersection */
    ce->intersection = InitDevPrim(NewDevPrim(), (DevPrim *)NULL);
    ce->bbox.bl.x = ce->bbox.bl.y = fpZero;
    ce->bbox.tr.x = ce->bbox.tr.y = fpZero;
    ce->isRect = true;
    ce->isEasy = true;
    }
  else if (bbcomp == inside && PathIsRect(&gs->clip)) {
    /* viewclip is inside regular clip */
    if (vcpath->rp == NULL)
      ReducePath(vcpath, curVC->evenOdd);
    ce->intersection = (DevPrim *)(vcpath->rp->devprim);
    ce->bbox = vcpath->bbox;
    ce->isRect = PathIsRect(vcpath);
    ce->isEasy = true;
    }
  else if (bbcomp == overlap && PathIsRect(vcpath) &&
      BBCompare(&gs->clip.bbox, &vcpath->bbox) == inside) {
    /* regular clip is inside viewclip */
    ce->intersection = (DevPrim *)(gs->clip.rp->devprim);
    ce->bbox = gs->clip.bbox;
    ce->isRect = PathIsRect(&gs->clip);
    ce->isEasy = true;
    }
  else {
    if (vcpath->rp == NULL)
      ReducePath(vcpath, curVC->evenOdd);
    ce->intersection = ClipDevPrim(
      (DevPrim *)vcpath->rp->devprim,
      (DevPrim *)gs->clip.rp->devprim);
    FullBounds(ce->intersection, &bounds);
    GetBBoxFromDevBounds(&ce->bbox, &bounds);
    ce->isRect = ce->intersection->next == NULL &&
                 DevPrimIsRect(ce->intersection);
    ce->isEasy = false;
    }
  AddRunIndexes(ce->intersection);
  FixCd(ce->bbox.bl, &ce->devbbox.bl);
  FixCd(ce->bbox.tr, &ce->devbbox.tr);
  return ce;
} /* FindCurrentViewInCache */

#define CurIsFirst() \
  viewclipCache != NULL && \
  viewclipCache->viewclip == (DevPrim *)(curVC->path.rp->devprim) && \
  viewclipCache->clip == (DevPrim *)(gs->clip.rp->devprim)

public DevPrim *GetDevClipPrim()
{
  /* returns the DevPrim for the intersection of current clip & viewclip */
  /* if we are inside a mask device (indicated by gs->noColorAllowed true)
     then viewclips are ignored */
  PViewClipCacheEntry ce;
  if (!curVC || gs->noColorAllowed)
	return (DevPrim *)gs->clip.rp->devprim;
  if (CurIsFirst()) return viewclipCache->intersection;
  ce = FindCurrentViewInCache();
  return ce->intersection;
} /* GetDevClipPrim */

public BBox GetDevClipBBox()
{
  /* returns the bbox for the intersection of the current clip & viewclip */
  /* if we are inside a mask device (indicated by gs->noColorAllowed true)
     then viewclips are ignored */
  PViewClipCacheEntry ce;
  if (!curVC || gs->noColorAllowed)
  	return(&gs->clip.bbox);
  if (CurIsFirst()) return &viewclipCache->bbox;
  ce = FindCurrentViewInCache();
  return &ce->bbox;
  } /* GetDevClipBBox */

public DevBBox GetDevClipDevBBox()
{
  /* returns the bbox for the intersection of the current clip & viewclip */
  /* if we are inside a mask device (indicated by gs->noColorAllowed true)
     then viewclips are ignored */
  PViewClipCacheEntry ce;
  if (!curVC || gs->noColorAllowed) /* Leo 22Sep87 */
  {
	DevBounds bounds;
	static DevBBoxRec mine;
	FullBounds((DevPrim *)gs->clip.rp->devprim, &bounds);
	mine.bl.x = FixInt(bounds.x.l);
	mine.bl.y = FixInt(bounds.y.l);
	mine.tr.x = FixInt(bounds.x.g);
	mine.tr.y = FixInt(bounds.y.g);
	return(&mine);
  }
  if (CurIsFirst()) return &viewclipCache->devbbox;
  ce = FindCurrentViewInCache();
  return &ce->devbbox;
} /* GetDevClipDevBBox */

public boolean DevClipIsRect()
{
  /* if we are inside a mask device (indicated by gs->noColorAllowed true)
     then viewclips are ignored */
  PViewClipCacheEntry ce;
  if (!curVC || gs->noColorAllowed)
  	return(PathIsRect(&gs->clip));
  if (CurIsFirst()) return viewclipCache->isRect;
  ce = FindCurrentViewInCache();
  return ce->isRect;
} /* DevClipIsRect */

private procedure NewViewClip(path, evenOdd) PPath path; boolean evenOdd; {
  PViewClip new;
  if (gs->noColorAllowed) Undefined(); /* not allowed inside mask device */
  if (curVC != NULL) {
    new = curVC; TermViewClip(curVC, false); }
  else
    new = (PViewClip)os_newelement(vcStorage);
  new->evenOdd = evenOdd;
  new->path = *path;
  if (path == &gs->path) { /* mimic NewPath */
    InitPath(path); gs->cp.x = gs->cp.y = fpZero; }
  path = &new->path;
  DURING
  if (path->rp == NULL || path->eoReduced != evenOdd)
    ReducePath(path, evenOdd);
  AddRunIndexes((DevPrim *)path->rp->devprim);
  HANDLER {RemPathRef(path); os_freeelement(vcStorage, (char *)new); RERAISE;}
  END_HANDLER;
  curVC = new;
} /* NewViewClip */

public procedure PSViewClip() { NewViewClip(&gs->path, false); }
public procedure PSEOViewClip() { NewViewClip(&gs->path, true); } 

public procedure PSRectViewClip()
{
  Object ob;
  Path path;
  TopP(&ob);
  if (ob.type == realObj || ob.type == intObj) {
    NewPath();
    MakeRectPath(&path);
    DURING
    NewViewClip(&path, false);
    HANDLER {RemPathRef(&path); RERAISE;}
    END_HANDLER;
    }
  else {
    BuildMultiRectPath();
    PSViewClip();
    }
} /* PSRectViewClip */

public procedure PSViewClipPath() {
  if (gs->noColorAllowed) Undefined(); /* not allowed inside mask device */
  NewPath();
  if (curVC != NULL)
    CopyPath(&gs->path, &curVC->path);
  else
    InitClipPath(&gs->path);
} /* PSViewClipPath  */

public procedure PSInitViewClip() {
  TermViewClip(curVC, true);
}

private procedure VCSaveProc(level) Level level; {
  /* level is the new level we are entering with this save */
  register PViewClip new;
  if (curVC == NULL) return;
  new = (PViewClip)os_newelement(vcStorage);
  CopyPath(&new->path, &curVC->path);
  new->evenOdd = curVC->evenOdd;
  new->baseSave = level - 1;
  new->next = viewClips;
  viewClips = new;
} /* VCSaveProc */

private procedure VCRestoreProc(level) Level level; {
  /* level is the old level we are going back to with this restore */
  PViewClip vc = viewClips, vc2;
  TermViewClip(curVC);
  while (vc && vc->baseSave > level) {
    vc2 = vc->next; TermViewClip(vc, true); vc = vc2; }
  if (vc && vc->baseSave == level) {
    curVC = vc; vc = vc->next; }
  viewClips = vc;
} /* VCRestoreProc */

public procedure IniViewClip(reason)  InitReason reason;
{
switch (reason)
  {
  case init:
#if (OS == os_mpw)
    globals = (Globals)os_sureCalloc(sizeof(GlobalsRec),1);
#endif (OS == os_mpw)
    vcCacheStorage = os_newpool(sizeof(ViewClipCacheEntry),5,0);
    vcStorage = os_newpool(sizeof(ViewClip),5,0);
    break;
  case romreg:
    RgstSaveProc(VCSaveProc);
    RgstRstrProc(VCRestoreProc);
    break;
  }
}
