/*****************************************************************************

    bag.c

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 29Jun89 Ted Cohn

    Modified:

    08Jan90 Ted   Removed alphabits from bag structure and where used before.
    08Jan90 Ted   Removed invisbits from bag structure also.
    09Jan90 Ted   Added BAGAllocAlpha routine. Removed "register" directives.
    06Feb90 Terry Replaced BMNewMemAt calls with BMNew macro calls.
    26Jun90 Ted   Removed BAGAllocAlpha; moved functionality to LSetAlphaBits.
    13Jul90 Ted   Removed BAGDecRef. Changed BAGIncRef to BAGDup.
    13Jul90 Ted   Created BAGDelete macro in bintree.h.
    16Jul90 Ted   Added NXSetHookMask routine for driver support.
    06Aug90 Ted   Removed BAGNewDummy() which is now obsolete.
    
******************************************************************************/

#import PACKAGE_SPECS
#import "bintree.h"
#import "windowdevice.h"
#import "bintreetypes.h"

static char *bagPool;  /* Blind pointer to storage pool for Bags */

/*****************************************************************************
    BAGInitialize
    Initialize bag manager.
******************************************************************************/
void BAGInitialize()
{
    bagPool = (char *) os_newpool(sizeof(NXBag), 0, 0);
}


/*****************************************************************************
    BAGNew
    Create a new bag for a window that will live on the given device.
    The "layer" provides the preferred depth, bounds and layerType.
    The "device" refers to the device this bag should be owned by.
******************************************************************************/
NXBag *BAGNew(Layer *layer, NXDevice *device)
{
    NXBag *bag;

    bag = (NXBag *)os_newelement(bagPool);
    bzero(bag, sizeof(NXBag));
    bag->type = BAG;
    bag->layer = layer;
    bag->next = layer->bags;
    bag->device = device;
    bag->refCount = 1;
    bag->procs = device->driver->procs;
    (*bag->procs->NewWindow)(bag, &layer->bounds,
	layer->layerType, layer->currentDepth, layer->local);
    if (layer->layerType != NONRETAINED && layer->alphaState == A_BITS) {
	(*bag->procs->NewAlpha)(bag);
    }
    return layer->bags = bag;
}


/*****************************************************************************
    BAGCompositeFrom
    Perform the given CompositeOperation.
	    dst must be a bag.
	    src may be either a bag or pattern.
******************************************************************************/
void BAGCompositeFrom(CompositeOperation *cop)
{
    Bounds dstBds;
    struct _any *srcAny;
    int cursorShielded = 1;	/* Assume cursor will be shielded */

#if STAGE==DEVELOP
    DebugAssert(cop->dst.any->type == BAG);
    if (cop->src.any) DebugAssert(cop->src.any->type == BAG ||
    cop->src.any->type == PATTERN);
#endif
    srcAny = cop->src.any;
    dstBds = cop->srcBounds;
    if ((dstBds.minx == dstBds.maxx) || (dstBds.miny == dstBds.maxy))
	return;
    if (cop->delta.i != 0)
	OFFSETBOUNDS(dstBds, cop->delta.cd.x, cop->delta.cd.y);

    /* Check for very easy case */
    if ((cop->mode == COPY) && (cop->dst.bag == cop->src.bag) &&
    (cop->dstCH == cop->srcCH) && (cop->delta.i == 0))
	return;

    /* Should we shield the cursor? */
    if (cop->dstCH == VISCHAN)
	if ((srcAny) && (srcAny->type == BAG) && (cop->srcCH == VISCHAN)) {
	    Bounds shieldBounds;
	    boundBounds(&cop->srcBounds, &dstBds, &shieldBounds);
	    ShieldCursor(&shieldBounds);
	} else ShieldCursor(&dstBds);
    else
	if ((srcAny) && (srcAny->type == BAG) && (cop->srcCH == VISCHAN))
	    ShieldCursor(&cop->srcBounds);
	else
	    cursorShielded = 0;
    (*cop->dst.bag->procs->Composite)(cop, &dstBds);
    if (cursorShielded) UnShieldCursor();
}


/*****************************************************************************
    BAGFind
    Looks for a bag associated with the given device in the layer's bag list.
    If not found, it automatically creates one for the new device and returns
    the new bag.
******************************************************************************/
NXBag *BAGFind(Layer *layer, NXDevice *device)
{
    NXBag *bag;

    for (bag = layer->bags; bag; bag = bag->next)
	if (bag->device == device)
	    return BAGDup(bag);
    return BAGNew(layer, device);
}


/*****************************************************************************
    BAGFree
    Does not necessarily free a bag. First decrements the refCount.
    Really frees the bag when its refCount is zero.  Also removes it from
    the layer's bag list.
******************************************************************************/
void BAGFree(NXBag *bag)
{
    NXBag **ptr = &bag->layer->bags;

    /* Remove bag from layer's bag list if found */
    while (*ptr) {
	if (*ptr == bag) {
	    *ptr = bag->next;
	    break;
	}
	ptr = &((*ptr)->next);
    }
    /* termwindowflag is a boolean that tells if the window is truly being
     * terminated (true) or just moving off of this device.
     */
    (*bag->procs->FreeWindow)(bag, termwindowflag);
    bag->type = 0;
    os_freeelement(bagPool, bag);
}


/*****************************************************************************
    NXSetHookMask
    This utility lets drivers specify hooks for a given bag/layer at any time.
    If the mask is non-zero, the layer's processHook flag is set so that
    bintree will call bags' hook procedures. If the mask is zero,
    then we need check if the other bags' masks are zero before clearing the
    processHooks flag.
******************************************************************************/
void NXSetHookMask(NXBag *bag, int mask)
{
    Layer *layer = bag->layer;
    DebugAssert(layer);
    bag->hookMask = mask;
    if (layer->processHooks == (mask)) return;
    /* If non-zero, just set the layer's processHooks flag */
    if (mask) {
	layer->processHooks = true;
    } else {
	NXBag *b;
	int testmask = 0;
        for (b = bag; b; b = b->next) testmask |= b->hookMask; 
	layer->processHooks = (testmask);
    }
}


/*****************************************************************************
    NXWID2Bag
    Support function for drivers to be able to find a bag associated with
    a given window.  Given a window id, returns the bag of the requested
    device if found, or NULL.
******************************************************************************/
NXBag *NXWID2Bag(short id, NXDevice *device)
{
    NXBag *bag;
    Layer *layer;
    
    if (!(layer = (Layer *)ID2Layer(id)))
	return NULL;
    for (bag = layer->bags; bag; bag = bag->next)
	if (bag->device == device)
	    return bag;
    return NULL;
}

/*****************************************************************************
    NXGetWindowInfo	--- LEAVE THIS IN.  DRIVER SUPPORT.
    Support function for drivers to get information on particular windows by
    their layer pointer.  Bags point to the layer that owns them.  Layers can
    be found by their window ids by calling ID2Layer().
******************************************************************************/
int NXGetWindowInfo(Layer *layer, NXWindowInfo *wi)
{
    PWindowDevice win;
    
    if (!layer || layer->type != LAYER) return -1;
    win = (PWindowDevice)layer->psWin;
    if (!win->exists) return -1;
    wi->id = win->id;
    wi->type = layer->layerType;
    wi->local = layer->local;
    wi->opaque = layer->alphaState;
    wi->autofill = layer->autoFill;
    wi->sendrepaint = layer->sendRepaint;
    wi->instancing = layer->instanceSet;
    wi->currentdepth = layer->currentDepth;
    wi->depthlimit = layer->depthLimit;
    wi->bags = layer->bags;
    wi->context = win->owner;
    wi->bounds = layer->bounds;
    wi->dirty = layer->dirty;
    wi->instance = layer->instance;
    return 0;
}



