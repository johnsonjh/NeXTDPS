/* mark.c

Copyright (c) 1983, '84, '85, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include DEVICE
#include DEVPATTERN
#include DEVIMAGE
#include EXCEPT
#include PSLIB

#include "devcommon.h"
#include "devmark.h"

#define MAXTRAPS (15)

#if DPSXA
DevCd devXAOffset;
#endif /* DPSXA */

typedef struct trptrp {
  DevShort cnt;
  procedure (*proc)();
  unsigned char *args;
  DevTrap *traps;
  } TrapTrapInfo;

private procedure AddTrap(t, tt)
  DevTrap *t; register TrapTrapInfo *tt; {
  if (tt->cnt == MAXTRAPS) {
    (*tt->proc)(tt->traps, tt->cnt, tt->args);
    tt->cnt = 0;
    }
  tt->traps[tt->cnt++] = *t;
  }

#if DPSXA
public procedure TrapTrapDispatch(
    clipTrap, clipItems, traps, items, bounds, trapsproc, args, translate)
  register DevTrap *clipTrap; DevTrap *traps;
  integer clipItems, items;
  DevBounds *bounds;
  procedure (*trapsproc)();
  MarkArgs *args;
  DevCd translate;
#else DPSXA
public procedure TrapTrapDispatch(
    clipTrap, clipItems, traps, items, bounds, trapsproc, args)
  register DevTrap *clipTrap; DevTrap *traps;
  integer clipItems, items;
  DevBounds *bounds;
  procedure (*trapsproc)();
  unsigned char *args;
#endif DPSXA
  {
  DevTrap trapList[MAXTRAPS];
  TrapTrapInfo ttinfo;
  DevInterval inner, outer;
  register DevTrap *t, *insideTrap;
  register integer i;
  ttinfo.cnt = 0;
  ttinfo.proc = trapsproc;
  ttinfo.args = args;
  ttinfo.traps = trapList;
  for (; clipItems--; clipTrap++) {
    switch (BoxTrapCompare(bounds, clipTrap, &inner, &outer, (DevTrap *)NULL)) {
      case outside: continue;
      case inside:
        (*trapsproc)(traps, items, args);
        return;
      }
    t = traps; i = items; insideTrap = NULL;
    for (; i--; t++) {
      if (t->y.l >= clipTrap->y.l && t->y.g <= clipTrap->y.g &&
          MIN(t->l.xl,t->l.xg) >= inner.l &&
	  MAX(t->g.xl,t->g.xg) <= inner.g) { /* trap t is inside clipTrap */
        if (insideTrap == NULL) insideTrap = t;
        }
      else {
        if (insideTrap != NULL) {
          (*trapsproc)(insideTrap, t - insideTrap, args);
	  insideTrap = NULL;
	  }
#if DPSXA
		XATrapTrapInt(t, clipTrap,
                    (DevInterval *)NULL, AddTrap, (char *)&ttinfo, translate);
#else /* DPSXA */
        TrapTrapInt(t, clipTrap,
                    (DevInterval *)NULL, AddTrap, (char *)&ttinfo);
#endif /* DPSXA */
        }
      }
    if (insideTrap != NULL)
      (*trapsproc)(insideTrap, t - insideTrap, args);
    }
  if (ttinfo.cnt > 0)
    (*trapsproc)(trapList, ttinfo.cnt, args);
  } /* TrapTrapDispatch */
  
public procedure ClipTrapsRunDispatch(
  clipTrap, clipItems, run, runproc, args)
  DevTrap *clipTrap;
  integer clipItems;
  DevRun *run;
  procedure (*runproc)();
  unsigned char *args;
  {
  DevInterval inner, outer;
  for (; clipItems--; clipTrap++) {
    switch (BoxTrapCompare(
      &run->bounds, clipTrap, &inner, &outer, (DevTrap *)NULL)) {
      case outside: break;
      case inside:
        (*runproc)(run, args);
        return;
      default:
        QIntersectTrp(run, clipTrap, runproc, args);
      }
    }
  }

private procedure NoClipMark(device, graphic, args, simpleProcs)
  PDevice device; DevPrim *graphic; MarkArgs *args;
  PSimpleMarkProcs simpleProcs;{
  DevPrim *graphic0 = graphic;
  integer items;
  PMarkProcs markProcs = args->procs;
  for (graphic = graphic0; graphic; graphic = graphic->next) {
    items = graphic->items;
    switch (graphic->type) {
      case trapType:
        (*simpleProcs->TrapsMark)(graphic->value.trap, items, args);
        break;
      case runType: {
        DevRun *run = graphic->value.run;
        for (; items--; run++)
          (*simpleProcs->RunMark)(run, args);
        break;
        }
      case maskType:
        (*simpleProcs->MasksMark)(graphic->value.string, items, args);
        break;
      case imageType: {
	DevImage *image;
	ImageArgs imageArgs;
        (*markProcs->SetupImageArgs)(device, &imageArgs);
	imageArgs.markInfo = args->markInfo;
        image = graphic->value.image;
        for (; items--; image++) {
	  imageArgs.image = image;
          (*markProcs->ImageTraps)(
            image->info.trap, image->info.trapcnt, &imageArgs);
          }
        break;
        }
      default:
        CantHappen();
        break;
      }
    }
  }

public procedure MasksMark(masks, items, args)
  DevMask *masks; integer items; MarkArgs *args; {
  procedure (*maskmark)();
  PMarkProcs procs = args->procs;
  if (args->patData.constant)
    switch (args->patData.value) {
      case LASTSCANVAL: maskmark = procs->black.MasksMark; break;
      case 0: maskmark = procs->white.MasksMark; break;
      default: maskmark = procs->constant.MasksMark;
      }
  else maskmark = procs->gray.MasksMark;
  (*maskmark)(masks, items, args);
  }

private procedure ClipRunMasksDispatch(
  masks, items, bounds, clipRun, args)
  DevMask *masks;
  integer items;
  DevBounds *bounds;
  DevRun *clipRun;
  MarkArgs *args;
  {
  if (items > 4) switch (QCompareBounds(clipRun, bounds)) {
    case outside: return;
    case inside:
      MasksMark(masks, items, args);
      return;
    }
  (*args->procs->ClippedMasksMark) (NULL, clipRun, masks, items, args);
  }

public procedure ClipRunTrapsDispatch(
  t, items, bounds, clipRun, runproc, trapsproc, args)
  DevTrap *t;
  integer items;
  DevBounds *bounds;
  DevRun *clipRun;
  procedure (*runproc)(), (*trapsproc)();
  unsigned char *args;
  {
  if (items > 4) switch (QCompareBounds(clipRun, bounds)) {
    case outside: return;
    case inside:
      (*trapsproc)(t, items, args);
      return;
    }
  for (; items--; t++) {
    QIntersectTrp(clipRun, t, runproc, args);
    }
  }

private procedure ClipTrapsStringDispatch(
    clipTrap, clipItems, chars, items, bounds, args)
  DevTrap *clipTrap; /* list of clip trapezoids */
  integer clipItems, items;
  DevMask *chars; /* string of characters */
  DevBounds *bounds; /* bounding box for entire string */
  MarkArgs *args; {
  /* divide the string into maximal substrings such that all chars in
     the substring are either inside the inner rect of the clip trap,
     outside the outer rect of the clip trap,
     or may overlap the clip trap. */
  integer len;
  register integer i;
  integer iStr; /* length of substring that is all inside */
  integer oStr; /* length of substring that overlaps */
  register DevMask *m;
  register PMask mm;
  DevInterval inner, outer;
  DevTrap rc; /* clipTrap reduced to minimum trap that includes bounds */
  register int mbxl, mbxg, mbyl, mbyg;
  PMarkProcs markProcs = args->procs;
  PSimpleMarkProcs simpleProcs;
  if (args->patData.constant)
    switch (args->patData.value) {
      case LASTSCANVAL: simpleProcs = &markProcs->black; break;
      case 0: simpleProcs = &markProcs->white; break;
      default: simpleProcs = &markProcs->constant;
      }
  else simpleProcs = &markProcs->gray;
  for (; clipItems--; clipTrap++) {
    len = items; m = chars;
    switch (
      BoxTrapCompare(bounds, clipTrap, &inner, &outer, &rc)) {
      case outside: continue;
      case inside: break;
      default:  /* check each char */
        iStr = oStr = 0;
        for (i = 0; i < len; i++, m++) {
          mbxl = m->dc.x;
          mbyl = m->dc.y;
          mm = m->mask;
          mbxg = mbxl + mm->width;
          mbyg = mbyl + mm->height;
          if ((mbxl >= outer.g) || (mbyl >= rc.y.g) ||
              (mbxg <= outer.l) || (mbyg <= rc.y.l)) {
              /* mask is outside outer bounds */
            if (iStr > 0) {
              (*simpleProcs->MasksMark)(m - iStr, iStr, args);
              iStr = 0;
              }
            else if (oStr > 0) {
              (*markProcs->ClippedMasksMark)(&rc, NULL, m - oStr, oStr, args);
              oStr = 0;
              }
            }
          else if ((mbxl >= inner.l) && (mbxg <= inner.g) &&
                   (mbyl >= rc.y.l)  && (mbyg <= rc.y.g)) {
              /* mask is inside inner bounds */
            if (oStr > 0) {
              (*markProcs->ClippedMasksMark)(&rc, NULL, m - oStr, oStr, args);
              oStr = 0; iStr = 1;
              }
            else iStr++;
            }
          else { /* mask may overlap */
            if (iStr > 0) {
              (*simpleProcs->MasksMark)(m - iStr, iStr, args);
              iStr = 0; oStr = 1;
              }
            else oStr++;
            }
          }
        if (oStr > 0) {
          (*markProcs->ClippedMasksMark)(&rc, NULL, m - oStr, oStr, args);
          len = 0;
          }
        else { m -= iStr; len = iStr; }
      }
    if (len > 0) { /* process the final substring that is all inside */
      (*simpleProcs->MasksMark)(m, len, args);
      if (len == items) return; /* all the string was in a single trap */
      items -= len; /* remove these from further consideration */
      }
    }
  } /* ClipTrapsStringDispatch */
            
public procedure Mark(device, graphic, devClip, markInfo)
  PDevice device; DevPrim *graphic; DevPrim *devClip; DevMarkInfo *markInfo; {
  DevPrim *graphic0 = graphic;
  DevPrim *clip;
  integer items, clipItems, traps;
  DevTrap *clipTrap;
  DevRun *run, *clipRun;
  DevImage *image;
  DevBounds bounds;
  BBoxCompareResult bbcomp;
  MarkArgs markArgs;
  PMarkProcs markProcs;
  PSimpleMarkProcs simpleProcs;
#if DPSXA
  DevCd translate;
  
  if(devClip != NULL) {
  	devXAOffset = devClip->xaOffset;
	translate.x = graphic->xaOffset.x - devClip->xaOffset.x;
	translate.y = graphic->xaOffset.y - devClip->xaOffset.y;
  } else {
  	devXAOffset = graphic->xaOffset;
	translate.x = translate.y = 0;
  }
#endif DPSXA

  markArgs.markInfo = markInfo;
  markArgs.device = device;
  (void)(*device->procs->Proc1)(device, &devClip, &markArgs); /* SetupMark */
  markProcs = markArgs.procs;
  FullBounds(graphic, &bounds);

  if (markArgs.patData.constant)
    switch (markArgs.patData.value) {
      case LASTSCANVAL: simpleProcs = &markProcs->black; break;
      case 0: simpleProcs = &markProcs->white; break;
      default: simpleProcs = &markProcs->constant;
      }
  else simpleProcs = &markProcs->gray;
  if (!devClip) {
    NoClipMark(device, graphic, &markArgs, simpleProcs);
    if (device->procs->Proc2 != NULL)
      (*device->procs->Proc2)(device);
    return;
    }
    
  for (clip = devClip; clip; clip = clip->next) {
    switch (clip->type) {
      case noneType: break;
      case trapType:
        bbcomp = BoundsCompare(&bounds,&clip->bounds);
        if (bbcomp == outside) continue;
        if (bbcomp == inside && DevPrimIsRect(clip)) {
          NoClipMark(device, graphic, &markArgs, simpleProcs);
          if (device->procs->Proc2 != NULL)
            (*device->procs->Proc2)(device);
          return; /* entire graphic inside this rectangle */
          }
        for (graphic = graphic0; graphic; graphic = graphic->next) {
          clipItems = clip->items;
          clipTrap = clip->value.trap;
          switch (graphic->type) {
            case trapType:
#if DPSXA
              TrapTrapDispatch(
                clipTrap, clipItems,
                graphic->value.trap, (integer)graphic->items,
                &graphic->bounds, simpleProcs->TrapsMark, &markArgs, translate);
#else /* DPSXA */
              TrapTrapDispatch(
                clipTrap, clipItems,
                graphic->value.trap, (integer)graphic->items,
                &graphic->bounds, simpleProcs->TrapsMark, &markArgs);
#endif /* DPSXA */
              break;
            case runType:
              run = graphic->value.run;
              items = graphic->items;
              for (; items--; run++) {
                ClipTrapsRunDispatch(
                  clipTrap, clipItems, run, simpleProcs->RunMark, &markArgs);
                }
              break;
            case maskType:
              ClipTrapsStringDispatch(
                clipTrap, clipItems, graphic->value.string,
                (integer)graphic->items, &graphic->bounds,
                &markArgs);
              break;
            case imageType: {
	      ImageArgs imageArgs;
              (*markProcs->SetupImageArgs)(device, &imageArgs);
              imageArgs.markInfo = markInfo;
              image = graphic->value.image;
              items = graphic->items;
              for (; items--; image++) {
                imageArgs.image = image;
#if DPSXA
                TrapTrapDispatch(
                  clipTrap, clipItems, image->info.trap,
                  (integer)image->info.trapcnt, &graphic->bounds,
                  markArgs.procs->ImageTraps, (MarkArgs *)&imageArgs,translate);
#else /* DPSXA */
                TrapTrapDispatch(
                  clipTrap, clipItems, image->info.trap,
                  (integer)image->info.trapcnt, &graphic->bounds,
                  markProcs->ImageTraps, &imageArgs);
#endif /* DPSXA */
                }
              break;
              }
            default:
              CantHappen();
              break;
            }
          }
        break;
      case runType:
        bbcomp = BoundsCompare(&bounds,&clip->bounds);
	if (bbcomp == outside) continue;
        for (graphic = graphic0; graphic; graphic = graphic->next) {
          clipItems = clip->items;
          clipRun = clip->value.run;
          switch (graphic->type) {
            case trapType:
              for (; clipItems--; clipRun++) {
                ClipRunTrapsDispatch(
                  graphic->value.trap, (integer)graphic->items,
                  &graphic->bounds, clipRun, simpleProcs->RunMark,
                  simpleProcs->TrapsMark, &markArgs);
                }
              break;
            case runType:
              for (; clipItems--; clipRun++) {
                run = graphic->value.run;
                items = graphic->items;
                for (; items--; run++) {
                  QIntersect(
                    clipRun, run, simpleProcs->RunMark, (char *)&markArgs);
                  }
                }
              break;
            case maskType:
              for (; clipItems--; clipRun++) {
                ClipRunMasksDispatch(
                  graphic->value.string, (integer)graphic->items,
                  &graphic->bounds, clipRun, &markArgs);
                }
              break;
            case imageType:
              for (; clipItems--; clipRun++) {
		ImageArgs imageArgs;
                (*markProcs->SetupImageArgs)(device, &imageArgs);
                imageArgs.markInfo = markInfo;
                image = graphic->value.image;
                items = graphic->items;
                for (; items--; image++) {
		  imageArgs.image = image;
                  ClipRunTrapsDispatch(
                    image->info.trap, (integer)image->info.trapcnt,
                    &graphic->bounds, clipRun, markProcs->ImageRun,
                    markProcs->ImageTraps, &imageArgs);
                  }
                }
              break;
            default:
              CantHappen();
              break;
            }
          }
        break;
      default:
        CantHappen();
        break;
      }
    }
  if (device->procs->Proc2 != NULL)
    (*device->procs->Proc2)(device);
  } /* Mark */


