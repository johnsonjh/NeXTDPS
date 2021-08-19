/*
  devbounds.c

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

Original version: Jim Sandman: Fri Jul 28 11:09:26 1989

Edit History:
Jim Sandman: Thu Aug 27 11:15:01 PDT 1987
Bill Paxton: Thu Mar 10 15:22:05 1988
End Edit History.
*/

/*
 * DevBounds methods
 */

#include PACKAGE_SPECS
#include PUBLICTYPES
#include DEVICE

static DevBounds emptyDevBounds =
    {maxDevInterval, minDevInterval, maxDevInterval, minDevInterval};

/* Answer empty DevBounds */
public DevBounds *EmptyDevBounds(self)
    register DevBounds *self;
    {
    *self = emptyDevBounds;
    return self;
    }

/* Merge (union) the DevBounds a and b into self */
public DevBounds *MergeDevBounds(self, a, b)
    register DevBounds *self;
    register DevBounds *a;
    register DevBounds *b;
    {
    self->x.l = a->x.l < b->x.l ? a->x.l : b->x.l;
    self->x.g = a->x.g > b->x.g ? a->x.g : b->x.g;
    self->y.l = a->y.l < b->y.l ? a->y.l : b->y.l;
    self->y.g = a->y.g > b->y.g ? a->y.g : b->y.g;
    return self;
    }

/* fill in full bounds for entire list */
public procedure FullBounds(self,bounds) DevPrim *self; DevBounds *bounds; {
  *bounds = self->bounds;
  while ((self = self->next) != NULL) {
    MergeDevBounds(bounds, &self->bounds, bounds);
    }
  }

/* Answer the DevBounds of a DevTrap */
public DevBounds *DevTrapDevBounds(self, trap)
    register DevBounds *self;
    register DevTrap *trap;
    {
    self->x.l = trap->l.xl < trap->l.xg ? trap->l.xl : trap->l.xg;
    self->x.g = trap->g.xl > trap->g.xg ? trap->g.xl : trap->g.xg;
    self->y.l = trap->y.l;
    self->y.g = trap->y.g;
    return self;
    }

/* Answer DevBounds of a DevRun */
public DevBounds *DevRunDevBounds(self, run)
    register DevBounds *self;
    register DevRun *run;
    {
    *self = run->bounds;
    return self;
    }

/* Answer DevBounds of a DevMask */
public DevBounds *DevMaskDevBounds(self, mask)
    register DevBounds *self;
    register DevMask *mask;
    {
    PMask m = mask->mask;
    self->x.l = mask->dc.x;
    self->x.g = mask->dc.x + m->width;
    self->y.l = mask->dc.y;
    self->y.g = mask->dc.y + m->height;
    return self;
    }

public BBoxCompareResult BoundsCompare(figb, clipb)
  register DevBounds *figb, *clipb;
{
if (figb->y.l >= clipb->y.l && figb->y.g <= clipb->y.g
   && figb->x.l >= clipb->x.l && figb->x.g <= clipb->x.g)
   return inside;
if (figb->y.g <= clipb->y.l || figb->y.l >= clipb->y.g
   || figb->x.g <= clipb->x.l || figb->x.l >= clipb->x.g)
   return outside;
return overlap;
}


/* Answer nonzero if and only if DevBounds overlap */
public boolean OverlapDevBounds(self, a)
    register DevBounds *self;
    register DevBounds *a;
    {
    if (self->x.l >= a->x.g || a->x.l >= self->x.g)
        return false;
    if (self->y.l >= a->y.g || a->y.l >= self->y.g)
        return false;
    return true;
    }
