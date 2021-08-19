/*
  devprim.c -- routines for creating, managing, and manipulating devprims.
  
  Unless otherwise noted at its point of definition, each public item defined
  herein is exported to device.h

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

*/

#include PACKAGE_SPECS
#include DEVICE
#include EXCEPT
#include PSLIB
#include FOREGROUND

#include "devprim.h"

#define DEVPRIMVALUELENGTH (100*(sizeof(DevTrap)))

private PVoidProc FlushClipProc;
private Pool devPrimStorage;
private DevPrim *clipCallBack;

public PVoidProc SetFlushClipProc (p) PVoidProc p; { /* exported to devprim.h */
  PVoidProc old = FlushClipProc;
  FlushClipProc = p;
  return old;
  }

public procedure DevFlushClip (clip) DevPrim *clip; {
  FGEnterMonitor();
  DURING
    if (FlushClipProc != NULL) (*FlushClipProc)(clip);
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();
  }

/* Answer a new, uninitialized DevPrim */
public DevPrim *NewDevPrim()
  {
  DevPrim *ans;
  FGEnterMonitor();
  DURING
    if (devPrimStorage == NIL)
      devPrimStorage = os_newpool (sizeof(DevPrim), 5, 0);
    ans = (DevPrim *)os_newelement(devPrimStorage);
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();
  return ans;
  }

/* Answer an initialized DevPrim */
public DevPrim *InitDevPrim(self, next) DevPrim *self, *next; {
  self->type = noneType;
  EmptyDevBounds(&self->bounds);
  self->next = next;
  self->items = 0;
  self->maxItems = 0;
  self->priv = NULL;
  self->value.value = NULL;
  return self;
  }
    
/* Size of a DevPrim (runType or trapType) */
/* Dispose a DevPrim */
public procedure DisposeDevPrim(self) register DevPrim *self; {
  DevPrim *next;
  FGEnterMonitor();
  DURING
    for (; self; self = next) {
      switch (self->type) {
        case runType: {
          int items = self->items;
          DevRun *run = self->value.run;
          if (!run) break;
          for (; items--; run++) {
            if (run->data) os_free((char *)run->data);
            if (run->indx) os_free((char *)run->indx);
            }
          break; }
        default: break;
        }
      if (self->value.value != NULL) os_free((char *)self->value.value);
      next = self->next;
      os_freeelement(devPrimStorage,(char *)self);
      }
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();
  }
    
/* Size of a DevPrim */
public boolean DevPrimIsRect(p) register DevPrim *p; {
  register DevTrap *t;
  /* this tests the particular DevPrim p only */
  /* it ignores p->next */
  if (p == NULL || p->type != trapType || p->items != 1)
    return false;
  t = p->value.trap;
  return (t->l.xl == t->l.xg && t->g.xl == t->g.xg);
  }

public integer DevPrimBytes(dp) register DevPrim *dp; {
  /* not valid for imageType */
  register integer size = 0, i;
  while (dp) {
    size += sizeof(DevPrim);
    i = dp->maxItems;
    switch (dp->type) {
      case runType: {
        register DevRun *run;
        size += i * sizeof(DevRun);
	run = dp->value.run;
	for (i = dp->items; i--; run++)
          size += run->datalen * sizeof(DevShort);
        break; }
      case trapType:
        size += i * sizeof(DevTrap);
	break;
      case maskType:
        size += i * sizeof(DevMask);
	break;
      case noneType:
        break;
      default: CantHappen();
      }
    dp = dp->next;
    }
  return size;
  }

/* Copy a DevPrim */
private procedure CopyRun(from, to) DevRun *from, *to;
  {
  *to = *from;
  to->data = to->indx = NULL;
  to->data = (DevShort *)os_sureCalloc(
    (long int)sizeof(DevShort), (long int)from->datalen);
  os_bcopy(
    (char *)from->data, (char *)to->data,
    (long int)(from->datalen * sizeof(DevShort)));
  }

public DevPrim *CopyDevPrim(from) DevPrim *from;
  { /* not for imageType */
  register DevPrim *to;
  DevPrim *next, *result, *prev;
  register integer i;
  result = prev = NULL;
  DURING
  while (from != NULL) {
    to = NewDevPrim();
    if (result == NULL) result = to;
    if (prev != NULL) prev->next = to;
    *to = *from; to->next = NULL;
    switch (to->type) {
      case runType: {
        register DevRun *toRun, *fromRun;
        to->value.run = NULL;
        to->value.run = toRun = (DevRun *)os_sureCalloc(
          (long int)to->items, (long int)sizeof(DevRun));
	for (i = to->items; i--; toRun++)
          toRun->data = toRun->indx = NULL;
	toRun = to->value.run; fromRun = from->value.run;
	for (i = to->items; i--; toRun++) {
          CopyRun(fromRun, toRun); fromRun++; }
	break; }
      case trapType:
        to->value.trap = NULL;
        to->value.trap = (DevTrap *)os_sureCalloc(
          (long int)to->items, (long int)sizeof(DevTrap));
	os_bcopy(
	  (char *)from->value.trap, (char *)to->value.trap,
          (long int)(to->items * sizeof(DevTrap)));
	break;
      case maskType:
        to->value.mask = NULL;
        to->value.mask = (DevMask *)os_sureCalloc(
          (long int)to->items, (long int)sizeof(DevMask));
	os_bcopy(
	  (char *)from->value.mask, (char *)to->value.mask,
          (long int)(to->items * sizeof(DevMask)));
	break;
      case noneType:
        break;
      default: CantHappen();
      }
    prev = to;
    from = from->next;
    }
  HANDLER {
    if (result) DisposeDevPrim(result);
    RERAISE;
    }
  END_HANDLER;
  return result;
  }

/* Add a new DevPrimValue when old is full, answer a DevPrim */
private DevPrim *AddDevPrimValue(self, type, length)
    DevPrim *self;
    DevPrimType type;
    int length;
    {
    if (self->type != noneType)
	  self = InitDevPrim(NewDevPrim(), self);
    self->type = type;
    self->maxItems = DEVPRIMVALUELENGTH / length;
    self->value.value = (DevPrivate *) os_sureCalloc(
      (long int)1, (long int)DEVPRIMVALUELENGTH);
    return self;
    }

/* Add an item to a DevPrim, answer a DevPrim */
public DevPrim *AddDevPrim(self, type, value, length, bounds)
    DevPrim *self;
    DevPrimType type;
    DevPrivate *value;
    int length;
    DevBounds *bounds;
    {
    if (self->type != type || self->items == self->maxItems)
      self = AddDevPrimValue(self, type, length);
    MergeDevBounds(&self->bounds, &self->bounds, bounds);
    os_bcopy(
      (char *)value, (char *)self->value.value + (length * self->items),
      (long int) length);
    self->items++;
    return self;
    }

/* Add a box to a DevPrim as a DevTrap, answer a DevPrim */
public DevPrim *AddBoxDevPrim(self, b)
    DevPrim *self;
    register DevBounds *b;
    {
    DevTrap trap;
    trap.y.g = b->y.g;
    trap.y.l = b->y.l;
    trap.l.xl = trap.l.xg = b->x.l;
    trap.g.xl = trap.g.xg = b->x.g;
    return AddDevPrim(
      self, trapType, (DevPrivate *)&trap, sizeof(DevTrap), b);
    }
    
/* Add a DevRun to a DevPrim, answer a DevPrim */
public DevPrim *AddRunDevPrim(self, run)
    DevPrim *self;
    DevRun *run;
    {
    DevRun newRun;
    newRun = *run;
    newRun.data = (DevShort *) os_sureCalloc(
      (long int)newRun.datalen, (long int)sizeof(DevShort));
 /* FIX THIS: storage leak possible if AddDevPrim fails */
    os_bcopy(
      (char *)run->data, (char *)newRun.data, 
      (long int)(newRun.datalen*sizeof(DevShort)));
    newRun.indx = NULL;
    return AddDevPrim(
      self, runType, (DevPrivate *)&newRun, sizeof(DevRun), &newRun.bounds);
    }

private procedure RunRunClipCallBack(run)
    DevRun *run;
    {
    clipCallBack = AddRunDevPrim(clipCallBack, run);
    BuildRunIndex(run);
    }

private procedure TrapTrapClipCallBack(trap)
    DevTrap *trap;
    {
    DevBounds bounds;
    DevTrapDevBounds(&bounds, trap);
    clipCallBack = AddDevPrim(
      clipCallBack, trapType, (DevPrivate *)trap, sizeof(DevTrap), &bounds);
    }

private DevPrim *RunRunClip(run1, run2, clip)
    DevRun *run1;
    DevRun *run2;
    DevPrim *clip;
    {
    clipCallBack = clip;
    QIntersect(run1, run2, RunRunClipCallBack, (char *)NIL);
    return clipCallBack;
    }

private DevPrim *RunTrapClip(run, trap, clip)
    DevRun *run;
    DevTrap *trap;
    DevPrim *clip;
    {
    clipCallBack = clip;
    QIntersectTrp(run, trap, RunRunClipCallBack, (char *)NIL);
    return clipCallBack;
    }

private DevPrim *TrapTrapClip(trap1, trap2, clip)
    DevTrap *trap1;
    DevTrap *trap2;
    DevPrim *clip;
    {
    clipCallBack = clip;
    TrapTrapInt(
      trap1, trap2, (DevInterval *)NULL, TrapTrapClipCallBack, (char *)NULL);
    return clipCallBack;
    }
    
public procedure AddRunIndexes(clip) DevPrim *clip; {
  FGEnterMonitor();
  DURING
    for (; clip; clip = clip->next) {
      if (clip->type == runType) {
        DevRun *run = clip->value.run;
        int items = clip->items;
        for (; items--; run++) {
          if (run->indx == NULL) BuildRunIndex(run);
	  }
        }
      }
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();
  }

/* Answer a new DevPrim which describes the intersection of two DevPrims */
public DevPrim *ClipDevPrim(clip1, clip2)
  DevPrim *clip1, *clip2; {
  DevPrim *clip;
  DevPrim *clip20 = clip2;
  
  clip = InitDevPrim(NewDevPrim(), (DevPrim *)NULL);

  FGEnterMonitor();
  DURING
  for (; clip1; clip1 = clip1->next) {
    switch (clip1->type) {
      case noneType: break;
      case trapType: {
        for (clip2 = clip20; clip2; clip2 = clip2->next) {
          if (!OverlapDevBounds(&clip1->bounds, &clip2->bounds))
            continue;
          switch (clip2->type) {
            case noneType: break;
            case trapType: {
              int items1 = clip1->items;
              DevTrap *trap1 = clip1->value.trap;
              for (; items1--; trap1++) {
                int items2 = clip2->items;
                DevTrap *trap2 = clip2->value.trap;
                for (; items2--; trap2++)
                    clip = TrapTrapClip(trap1, trap2, clip);
                }
              break;
              }
            case runType: {
              int items1 = clip1->items;
              DevTrap *trap1 = clip1->value.trap;
              for (; items1--; trap1++) {
                int items2 = clip2->items;
                DevRun *run2 = clip2->value.run;
                for (; items2--; run2++)
                    clip = RunTrapClip(run2, trap1, clip);
                }
              break;
              }
            default: CantHappen(); break;
            }
          }
        break;
        }
      case runType: {
        for (clip2 = clip20; clip2; clip2 = clip2->next) {
          if (!OverlapDevBounds(&clip1->bounds, &clip2->bounds))
            continue;
          switch (clip2->type) {
            case noneType: break;
            case trapType: {
              int items1 = clip1->items;
              DevRun *run1 = clip1->value.run;
              for (; items1--; run1++) {
                int items2 = clip2->items;
                DevTrap *trap2 = clip2->value.trap;
                for (; items2--; trap2++)
                    clip = RunTrapClip(run1, trap2, clip);
                }
              break;
              }
            case runType: {
              int items1 = clip1->items;
              DevRun *run1 = clip1->value.run;
              for (; items1--; run1++) {
                int items2 = clip2->items;
                DevRun *run2 = clip2->value.run;
                for (; items2--; run2++)
                    clip = RunRunClip(run1, run2, clip);
                }
              break;
              }
            default: CantHappen(); break;
            }
          }
        break;
        }
      default:CantHappen(); break;
      }
    }
  HANDLER
    FGExitMonitor();
    RERAISE;
  END_HANDLER;
  FGExitMonitor();
  return clip;
  }

/* Returns true if it can be shown easily that the clip encloses the rect */
/* exported to devprim.h */
public boolean EnclosesRect(dp, r) DevPrim *dp; DevBounds *r; {
  for (; dp; dp = dp->next) {
    switch (dp->type) {
      case noneType: break;
      case trapType: {
        DevTrap *trap1 = dp->value.trap;
        int items1 = dp->items;
        if (!OverlapDevBounds(&dp->bounds, r))
          continue;
        for (; items1--; trap1++) {
	  DevInterval n1, n2;
          if (BoxTrapCompare(r,trap1,&n1,&n2,NULL) == inside)
	    return true;
          }
        break;
        }
      case runType: {
        DevRun *run1 = dp->value.run;
        int items1 = dp->items;
        if (!OverlapDevBounds(&dp->bounds, r))
          continue;
        for (; items1--; run1++) {
          if (QCompareBounds(run1, r) == inside) return true;
          }
        break;
        }
      default: CantHappen();
      }
    }
  return false;
  } /* EnclosesRect */

