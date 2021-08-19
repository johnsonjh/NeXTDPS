/* winclip.c - Implements merged window system/DPS clip management

Copyright (c) 1989 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Pasqua: Tue May 30 10:05:53 PDT 1989
Edit History:
Joe Pasqua: Mon Jul 10 15:56:00 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include EXCEPT

#include "winclip.h"

/*	===== BEGIN	CONSTANTS/INLINES =====	*/
#define winclipcacheMax (20) /* must be greater than 1 */
#define IsEasy(ce) \
  ((ce)->devclip == (ce)->intersection || (ce)->winclip == (ce)->intersection)


/*	===== BEGIN	TYPE DECLS =====	*/
typedef struct _winclipce {
  struct _winclipce *next;
  DevPrim *winclip;	/* the window clip region (in device space) */
  DevPrim *devclip;	/* the device clip region */
  DevPrim *intersection; /* the intersection of winclip and devclip */
  } WinClipCacheEntry, *PWinClipCacheEntry;


/*	===== BEGIN	EXTERN PROCEDURES =====	*/


/*	===== BEGIN	PRIVATE Variables =====	*/
private PWinClipCacheEntry winclipCache;	/* initialize to NULL */
private Card16 winclipcachelength;		/* initialize to 0 */


/*	===== BEGIN	PRIVATE Routines =====	*/
private procedure FreeIntersection(ce)
  register PWinClipCacheEntry ce;
  {
  if (ce->intersection != ce->winclip && ce->intersection != ce->devclip)
    DisposeDevPrim(ce->intersection);
  }


/*	===== BEGIN	PUBLIC Routines =====	*/
public DevPrim *FindDevWinClip(devclip, winclip)
  register DevPrim *devclip, *winclip;
  {
  register PWinClipCacheEntry ce, prev;
  BBoxCompareResult bbcomp;
  DevBounds devbounds, winbounds;
  
  ce = winclipCache; prev = NULL;
#if DPSXA
  ce = NULL; /* winclip cacheing not needed for DPSXA */
#endif /* DPSXA */
  while (ce != NULL) {
    if (ce->devclip == devclip && ce->winclip == winclip) {
      if (prev != NULL) { /* move ce to front of list */
        prev->next = ce->next;
	ce->next = winclipCache;
	winclipCache = ce;
        }
      return ce->intersection;
      }
    prev = ce;
    ce = ce->next;
    }
  /* must put it in the cache */
  if (winclipcachelength == winclipcacheMax) { /* free oldest */
    ce = winclipCache;
    while (ce->next != NULL) {prev = ce; ce = ce->next;}
    if (!IsEasy(ce) && IsEasy(prev)) *prev = *ce;
    else {
      prev->next = NULL;
      FreeIntersection(ce);
      }
    }
  else {
    winclipcachelength++;
    ce = (PWinClipCacheEntry)os_malloc((long int)sizeof(WinClipCacheEntry));
    if (ce == NIL) RAISE(ecLimitCheck, (char *)NIL);
    }
  ce->devclip = devclip;
  ce->winclip = winclip;
  ce->next = winclipCache;
  winclipCache = ce;
  FullBounds(devclip, &devbounds);
  FullBounds(winclip, &winbounds);
  bbcomp = BoundsCompare(&devbounds, &winbounds);
  if (bbcomp == outside) /* null intersection */
    ce->intersection = InitDevPrim(NewDevPrim(), (DevPrim *)NULL);
  else if (bbcomp == inside &&
           winclip->next==NULL && DevPrimIsRect(winclip))
    ce->intersection = devclip; /* devclip is inside winclip */
  else if (bbcomp == overlap &&
           devclip->next==NULL && DevPrimIsRect(devclip) &
	   BoundsCompare(&winbounds, &devbounds) == inside)
    ce->intersection = winclip; /* winclip is inside devclip */
  else /* must build the intersection */
    ce->intersection = ClipDevPrim(winclip, devclip);
  AddRunIndexes(ce->intersection);
  return ce->intersection;
  }

public procedure SetWindow(device, origin, winclip, window)
  PDevice device; DevPoint origin; DevPrim *winclip; DevPrivate *window;
  {
  WinInfo *w;
  if (device->priv != NULL) { /* get rid of the old stuff */
    w = (WinInfo *)(device->priv);
    if (w->winclip != NIL) 
      {
      TermDevWinClip(w->winclip);
      DisposeDevPrim(w->winclip);
      }
    }
  else {
    w = (WinInfo *)os_malloc((long int)sizeof(WinInfo));
    if (w == NIL) RAISE(ecLimitCheck, (char *)NIL);
    device->priv = (DevPrivate *)w;
    }
  w->window = window;
  w->origin = origin;
  w->winclip = winclip;
  }

public procedure TermDevWinClip(clip)
  DevPrim *clip;
  {
  register PWinClipCacheEntry ce, next, prev;
  register DevPrim *c = clip;
  ce = winclipCache; prev = NULL;
  while (ce != NULL) {
    next = ce->next;
    if (ce->winclip != c && ce->devclip != c) prev = ce;
    else {
      if (prev == NULL) winclipCache = next;
      else prev->next = next;
      FreeIntersection(ce);
      os_free((char *)ce);
      winclipcachelength--;
      }
    ce = next;
    }
  }

public procedure TermWindow(device)
  PDevice device;
  {
  WinInfo *w;
  if (device->priv == NULL) return;
  w = (WinInfo *)(device->priv);
  if (w->winclip) {
    TermDevWinClip(w->winclip);
    DisposeDevPrim(w->winclip);
    }
  os_free((char *)w);
  device->priv = NULL;
  }

public procedure InitWinClip()
  {
  winclipCache = NULL;
  winclipcachelength = 0;
  }
