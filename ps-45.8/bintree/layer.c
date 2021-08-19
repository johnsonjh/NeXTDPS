/*****************************************************************************
    
    layer.c
    Definition of layer class for bintree window system.

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 23Apr86 Leo

    Modified:

    19Nov87  Jack  Convert setBackPat & compositeColorTo to use Halftones
    16Dec87  Leo   Removed old change log
    16Dec87  Leo   Converted to use procedure-based CauseSet and WindowList
    08Jan88  Leo   Straight C
    29Jan88  Leo   Device-independent
    29Jan88  Leo   screenBytes is special for Nextdevice
    08Feb88  Leo   Preserve instancing, autoFill, sendRepaint, dirty rect in
    		   placeAt
    10Feb88  Leo   Make tempCauses dynamically allocated
    07Mar88  Leo   l++ bug in LMoveTo fixed
    09Mar88  Leo   Bugs in LCompositeBitsFrom fixed
    13Mar88  Leo   ShieldCursor in LMark only if window is in window list
    24Mar88  Leo   Use new A_DATAONE alphaState in LCompositeColorTo
                   Make LMark have generic mark calling sequence
                   Make LCompositeFrom have generic composite call
    25Mar88  Leo   Use layer->instanceSet to see if instancing rect is there
    26Mar88  Leo   CompositeOperation structure
    27Mar88  Leo   No more statics for CopyOffscreen!
    29Mar88  Jack  Make Image opaque in LMark
    04Apr88  Jack  Invert alpha in LCompositeFrom
    11Apr88  Leo   Instancing now up in graphics state
    16Apr88  Leo   LGetDevices
    11May88  Leo   LPlaceAt fix: transfer setautoFill, etc., earlier
    22May88  Leo   Error check in Highlight case of LCompositeFrom
    24May88  Leo   Reworked piece cache logic in LMark for instancing
    01Jun88  Jack  Add defaultHalftone and new arg to PNewHalftone
    20Jul88  Jack  Remove ALPHA(gs) == 0 check in LMark
    05Aug88  Jack  Look out for NULL screen in LSetExposureColor
    30Aug88  Jack  Clear the window in LInitPage
    05Oct88  Leo   Error checking in LOrder
    18Jan89  Jack  Reversed the sense of ALPHA, new convention for alphaimage
    25Jan89  Jack  Changed LCompositeBitsFrom to LCopyBitsFrom
    08Feb89  Jack  Use LOG2BD for framelog2BD
    16Feb89  Jack  Add premult conversion to alphaimage in LMark
    01Mar89  Ted   Intense hackery. Merged changes from v006/v001
    16May89  Ted   LFlushBits in LPlaceAt due to wot's menu bug.
    20Mar89  Jack  Improve "checkerboard" premult & fix ExposureColor
                   screenphase
    16Jun89  Ted   Merged V007 into code.
    29Jun89  Jack  Fix LMark to neutralize gs->tfrFcn for alpha in alphaimage
    07Jul89  Ted   New method using "Bags" for bitmaps.
    30Oct89  Ted   Added Layer->exposure field for true color exposures
    06Nov89  Terry CustomOps conversion
    29Nov89  Terry IBM compatibility modifications
    05Dec89  Ted   Integratathon!
    22Jan90  Terry Allow interleaved alphaimages to pass through LMark
    23Jan90  Terry Invert alpha during copying instead of image call in LMark
    31Jan90  Terry Modified LSetExposureColor to not use defaultHalftone
    08Feb90  Ted   Leo's LSubListReveal(layer, offSubList()) fix to LOrder.
    08Feb90  Terry Patch up LCopyBitsFrom for the new bitmap conversion scheme
    28Feb90  Terry Modify LPlaceAt to copy layer's baglist + not free old tree
    08Mar90  Terry In LMark, only shield cursor when drawing onscreen
    07Jun90  Terry Changed windowList field to a boolean
    10Jun90  Ted   Fixed bug 6024 in LOffScreenTest
    10Jun90  Ted   Removed LOffScreenTest, LShouldConvert, LNewDummyCause
    24Jun90  Ted   Added LSetDepthLimit.
    03Jul90  Ted   Added promotion code to LMark. Added LPromoteLayer
    03Jul90  Ted   Optimized LSetExposureColor: don't always PFree and PNew.
    03Jul90  Ted   Found leak in LFree: layer's exposure color wasn't freed!
    03Jul90  Ted   Added promotion code to LCompositeFrom
    26Jul90  Ted   Added LPreCopyBitsFrom to map logical to actual depths.
    21Aug90  Terry Made sure a new CompOp is created from scratch in LFill 
    28Aug90  Ted   Changed LPromoteLayer to blindly promote to a given depth.
		   Added promoteDepths[] and promoteParms[] to find depths.
    20Sep90  Ted   Renamed LRenderInBounds to NXRenderInBounds.
    20Sep90  Ted   Added termwindowflag global.
    24Sep90  Ted   Fixed bug 7884: added screen-changed logic to LPlaceAt.

******************************************************************************/

#import PACKAGE_SPECS
#import BINTREE
#import "bintreetypes.h"
#import WINDOWDEVICE
#import DEVPATTERN		/* for DevColorVal */
#import DEVICETYPES		/* for DevImageSource */
#import "bitmap.h"		/* for LCopyBitsFrom */
#import "../mp/mp12.h"

/*****************************************************************************
    Bintree Globals
******************************************************************************/
short remapY;			/* Maps screen to device coordinate space */
Bounds wsBounds;		/* Enormous bound of all screens */
int driverCount;		/* Number of active drivers */
int deviceCount;		/* Number of active devices */
int initialDepthLimit;		/* Maximum visible depth among framebuffers */
int asyncDriversExist;          /* True if >0 drivers can Ping */
NXDevice *deviceList;		/* Head of linked-list of device entries */
NXDriver *driverList;		/* Head of linked-list of driver entries */
NXDevice *deviceCause;		/* Device of current cause */
SubList dummySubList;		/* SubList of on&offscreen layers */
SubList offSubList;		/* SubList of offscreen layers */
SubList extSubList;		/* SubList of extent layers */
NXDevice *holeDevice;		/* Device to control holes in space */
Pattern *whitepattern;
Pattern *blackpattern;
int termwindowflag;		/* Singals BAGFree of terminating window */
NXHookData hookData;		/* Temp storage for hook calls (to avoid
				 * mallocs).
				 */
				 
/*****************************************************************************
    Forward Declarations For Static Layer Procedures
******************************************************************************/
void CopyOffscreen(Bounds *, Piece);
void CopyOnscreen(Bounds *, Piece);
void ExpandTempCauses();
void LAddToInstance(Layer *, Bounds *);
void LHideInstanceIn(Layer *, Bounds);
void LInitialize();
Layer *LNew();
void LObscureInside(Layer *, Bounds *, int);
void LObscureSubList(Layer *, SubList);
void LPromoteLayer(Layer *, int);
void LRemoveFromInstance(Layer *, Bounds *);
void LRevealInside(Layer *, Bounds *, int);
void LRevealSubList(Layer *, SubList);
void LSetAlphaBits(Layer *);
void LSetAlphaOpaque(Layer *layer);
void LSubListObscure(Layer *, SubList);
void LSubListReveal(Layer *, SubList);
int UniqueCause();


/*****************************************************************************
    Defines and Statics
******************************************************************************/
#define TEMPCAUSEEXPAND 32	/* Number of causes to expand list by */
static int *tempCauses;		/* Temporary unique causes */
static char *layerPool;		/* Blind pointer to storage pool for Layers */
static int numTempCauses;	/* Number of causes currently in list */
static const unsigned int needAlphaBits[2] = { /* [layer->alphaState] */
    1<<CLEAR|1<<COPY|1<<SIN|1<<SOUT|1<<DIN|1<<DOUT|1<<DATOP|1<<XOR|1<<DISSOLVE,
    1<<CLEAR|1<<SOUT|1<<DOUT|1<<DATOP|1<<XOR
};
/* bmClassFromDepth[] is used only by LCopyBitsFrom. When other bm classes are
 * included for 2.0, fill them in here. logToPhysDepth[] maps logical depth
 * to actual depth returned by readimage.  Used by LPreCopyBitsFrom. These two
 * arrays MUST coincide with each other (that is, mp12 maps to NX_TWOBITGRAY
 * and bm38 maps to NX_TWENTYFOURBITRGB). bm34 currently isn't included because
 * readimage doesn't output 16bit values yet.  Certainly something that needs
 * to be added for 2.0.
 */
static BMClass *bmClassFromDepth[] = {mp12, mp12, bm38, bm38, bm38};
static int logToPhysDepth[] = {
    NX_TWOBITGRAY, NX_TWOBITGRAY, NX_TWENTYFOURBITRGB,
    NX_TWENTYFOURBITRGB, NX_TWENTYFOURBITRGB
};

/* The following promotion table depends on a layer's currentDepth(depth),
 * depthLimit(cap), whether color exists(color), whether color is
 * precise(pcolor) and whether gray is precise(pgray). The elements are depths
 * to be promoted to. If zero, this indicates no promotion should take place.
 *
 * NOTE: depth==2 && cap==2 is an nop and doesn't exist in the table.
 *	 depth==24 && cap==24 is also a nop.
 * Therefore:
 * 1.	The input [depth] should be between 0..2 which corresponds to
 *	NX_TWOBITGRAY..NX_TWELVEBITRGB.
 * 2.	The input [ca] should be between 0..2 which corresponds to
 *	NX_EIGHTBITGRAY..NX_TWENTYFOURBITRGB.
 *
 *	[depth][cap][color][pcolor][pgray]
 */
static char promoteParms[3][3][2][2][2] = {
      /* depth :: NX_TWOBITGRAY */
    { /* cap 8  */ {{{0,2},{0,2}}, {{2,2},{2,2}}},
      /* cap 12 */ {{{0,3},{0,3}}, {{3,3},{3,3}}},
      /* cap 24 */ {{{0,2},{0,2}}, {{3,4},{4,4}}}
    },
      /* depth :: NX_EIGHTBITSGRAY */
    { /* cap 8  */ {{{0,0},{0,0}}, {{0,0},{0,0}}},
      /* cap 12 */ {{{0,0},{0,0}}, {{0,0},{0,0}}}, /* illegal state */
      /* cap 24 */ {{{0,0},{0,0}}, {{4,4},{4,4}}}
    },
      /* depth :: NX_TWELVEBITSRGB */
    { /* cap 8  */ {{{0,0},{0,0}}, {{0,0},{0,0}}}, /* illegal state */
      /* cap 12 */ {{{0,0},{0,0}}, {{0,0},{0,0}}},
      /* cap 24 */ {{{0,4},{0,4}}, {{0,4},{4,4}}}
    }
};

/* Use the following array when you have a layer you may need to promote
 * because you're trnasfering information to it from another layer of some
 * depth.  As usual, zero elements represent nops.
 * Pass for array indices: [cap-2][currentdepth-1][srcdepth-1]
 */
static char promoteDepths[3][4][4] = {
    /* cap 8  */ {{0,2,2,2}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
    /* cap 12 */ {{0,3,3,3}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
    /* cap 24 */ {{0,2,3,4}, {0,0,0,4}, {0,0,0,4}, {0,0,0,0}}
};

/*****************************************************************************
    copyCO
    Set the given composite operation to be a straight copy.
******************************************************************************/
void copyCO(CompositeOperation *cop)
{
    bzero(cop, sizeof(CompositeOperation));
    cop->mode = COPY;
}


/*****************************************************************************
    CopyOffscreen
    Callback procedure used in conjuntion with FindDiffBounds and the
    instance-checking code in LAddToInstance.
******************************************************************************/
static void CopyOffscreen(Bounds *bounds, Piece p)
{
    CompositeOperation co;
    
    copyCO(&co);
    co.dst.any = p.any;
    co.srcCH = BPCHAN;
    co.dstCH = BACKCHAN;
    co.srcIsDst = true;
    co.srcBounds = *bounds;
    PieceCompositeFrom(&co);
}

static void CopyOnscreen(Bounds *bounds, Piece p)
{
    CompositeOperation co;
    copyCO(&co);
    co.dst.any = p.any;
    co.srcCH = BACKCHAN;
    co.dstCH = BPCHAN;
    co.srcIsDst = true;
    co.srcBounds = *bounds;
    PieceCompositeFrom(&co);
}

/*****************************************************************************
    ComputeHideBounds
    Returns true if result bounds need to be hidden.
    Result gets instance bounds to be hidden so instance bounds do not
    overlap draw bounds.
******************************************************************************/
static int ComputeHideBounds(Bounds draw, Bounds instance, Bounds *result)
{
    BBoxCompareResult compare;
    int w, h, area, max;

    compare = IntersectAndCompareBounds(&instance, &draw, result);
    if (compare == outside) return false;
    if (compare == inside) return true;

    w = instance.maxx - instance.minx;
    h = instance.maxy - instance.miny;
    max = 0;
    
    /* Find maximal instance area to preserve and return the part to hide */
    if ((area = (draw.minx-instance.minx)*h) > max) { /* area left of draw */
        *result = instance;
	result->minx = draw.minx;
	max = area;
    }
    if ((area = (instance.maxx-draw.maxx)*h) > max) { /* area right of draw */
        *result = instance;
	result->maxx = draw.maxx;
	max = area;
    }
    if ((area = (draw.miny-instance.miny)*w) > max) { /* area below draw */
        *result = instance;
	result->miny = draw.miny;
	max = area;
    }
    if ((area = (instance.maxy-draw.maxy)*w) > max) { /* area above draw */
        *result = instance;
	result->maxy = draw.maxy;
    }
    return true;
}

/*****************************************************************************
    LayerInit (extern)
    Initialize BinTree.
******************************************************************************/
void LayerInit(int reason)
{
    if (reason==1) {
	PInitialize();
	WLInitialize();
	BAGInitialize();
	CSInitialize();
	BPInitialize();
	DPInitialize();
	LInitialize();
    }
}


/*****************************************************************************
    LInitialize
    Initialize the layer manager.
******************************************************************************/
static void LInitialize()
{
    int i;

    layerPool = (char *) os_newpool(sizeof(Layer), 0, 0);

    /* Get a bunch of causes to use whenever necessary */
    tempCauses = NULL;
    numTempCauses = 0;
    ExpandTempCauses();
}


/*****************************************************************************
    UniqueCause
    Generate a unique cause id.
******************************************************************************/
static int UniqueCause()
{
    static int thisCause = 2048;
    
    thisCause++;
    while ((thisCause == ONSCREENREASON) || (thisCause == OFFSCREENREASON) ||    
    (thisCause == NOREASON))
        thisCause++;
    return thisCause & 0x7fffffff;
}


/*****************************************************************************
    ExpandTempCauses
    Expand the temporary list of causes by adding new unique causes.
******************************************************************************/
static void ExpandTempCauses()
{
    int *newTempCauses, *tmpPtr;
    int newNum, i;

    newNum = numTempCauses + TEMPCAUSEEXPAND;
    tmpPtr = newTempCauses = (int *) malloc(newNum * sizeof(int *));
    for (i=0; i<newNum; i++)
        *tmpPtr++ = UniqueCause();
    if (tempCauses)
        free(tempCauses);
    tempCauses = newTempCauses;
    numTempCauses = newNum;
}


/*****************************************************************************
    LNew
    Return a new layer. Called by LNewAt() and LNewDummyAt().
******************************************************************************/
static Layer *LNew()
{
    Layer *layer;

    layer = (Layer *) os_newelement(layerPool);
    bzero(layer, sizeof(Layer));
    layer->type = LAYER;
    return layer;
}


/*****************************************************************************
    LNewAt
    Create a new layer with the given qualities.  StartDepth is the depth that
    the window should first be set to.  DepthLimit is the cap.  Local is a
    boolean that, when set, indicates that windowbitmap needs one bag's backing
    store to be NX_TWOBITGRAY and never be deallocated until the window is
    freed.
******************************************************************************/
Layer *LNewAt(int type, Bounds pane, int *win, int local, int startDepth,
    int depthLimit)
{
    Bounds r;
    Bitmap *bm;
    int i, cause;
    Layer *layer, **l;
    NXDevice *device;
    CompositeOperation co;
    
    if (local) {
	local = 0;
	for (device=deviceList; device; device=device->next)
	    if (strcmp(device->driver->name, "MegaPixel")==0) {
		local = true;
		break;
	    }
	if (!local) return NULL;
    }
    layer = LNew();
    if (layer->local = local) layer->device = device;
    layer->bounds = pane;
    layer->alphaState = A_ONE;
    layer->exposure = whitepattern;
    layer->causeId = UniqueCause();
    layer->psWin = win;	/* Link to PostScript window */
    layer->autoFill = true;
    collapseBounds(&layer->dirty, &pane); /* initialize dirty rect */
    layer->sendRepaint = true;
    layer->layerType = type;
    layer->depthLimit = depthLimit;
    layer->currentDepth = (depthLimit < startDepth) ? depthLimit : startDepth;
    layer->capped = (layer->depthLimit == layer->currentDepth);
    layer->tree.bp = BPNewAt(NULL, NULL, layer->bounds, OFFSCREEN, layer,
	NULL, holeDevice);
    for (i=0, l=(Layer **)extSubList.ptr; i<extSubList.len; i++, l++) {
        r = (*l)->bounds;
        if (sectBounds(&r, &layer->bounds, &r)) {
            cause = MAKECONVERTCAUSE((*l)->causeId);
            deviceCause = (*l)->device;
            layer->tree = PieceObscureInside(layer->tree, r, cause);
            if (layer->tree.any->type == DIVPIECE)
                DPSwapCause(layer->tree.dp, (*l)->causeId, cause);
        }
    }
    /* OK, tree is created, need to allocate bits and bags!!! */
    /* BitPieces' conv field should be set to spacial device coverage */
    PieceApplyProc(layer->tree, BPAllocBag);
    /* Now if we are local, we need to make sure the local bag is never
     * deleted.  We do this by either creating the bag if not yet created
     * or by making an additional reference to it if it already exists.
     */
    if (local) BAGFind(layer, layer->device);
    /* Now clear buffer to exposure color */
    copyCO(&co);
    co.dst.any = layer->tree.any;
    co.src.pat = layer->exposure;
    co.dstCH = BACKCHAN;
    co.srcBounds = layer->bounds;
    PieceCompositeFrom(&co);
    return layer;
}


/*****************************************************************************
    LNewDummyAt
    Create a dummy layer.
******************************************************************************/
Layer *LNewDummyAt(Bounds pane)
{
    Layer *layer;
    
    layer = LNew();
    layer->bounds = pane;
    layer->causeId = UniqueCause();
    layer->layerType = DUMMY;
    collapseBounds(&layer->dirty, &layer->bounds);
    return layer;
}


/*****************************************************************************
    LAddToDirty
    Clips bounds to layer bounds then adds result to the dirty bounds.
******************************************************************************/
void LAddToDirty(Layer *layer, Bounds *bounds)
{
    Bounds ibounds;
    
    if (layer->layerType == BUFFERED) {
        sectBounds(bounds, &layer->bounds, &ibounds);
        boundBounds(&layer->dirty, &ibounds, &layer->dirty);
    }
}


/*****************************************************************************
    LAddToInstance
    Adds the given rectangle to the instanced area of the given layer.
******************************************************************************/
static void LAddToInstance(Layer *layer, Bounds *bounds)
{
    Bounds newInstance;

    DebugAssert(layer->type == LAYER);
    if (layer->instanceSet) {
        if (!boundBounds(&layer->instance, bounds, &newInstance)) {
            if (layer->layerType == RETAINED) {
                /* Move these about-to-be-instanced-in areas offscreen */                
                FindDiffBounds(&layer->instance, &newInstance, CopyOffscreen,
		    layer->tree.any);
                /* CopyOffscreen is callback procedure defined above */
            }
            layer->instance = newInstance;
        }
    } else {
        if (layer->layerType == RETAINED)
	    CopyOffscreen(bounds, layer->tree);
        layer->instanceSet = true;
        layer->instance = *bounds;
    }
}


/*****************************************************************************
    NXRenderInBounds
    Apply a boundary over the given layer and call the given callback
    procedure with the given data for each bitpiece that itersects the bounds.
******************************************************************************/
void NXRenderInBounds(Layer *layer, Bounds *b, void (*proc)(), void *data)
{
    BoundsBundle bundle;
 
    bundle.data = data;
    bundle.proc = proc;
    bundle.chan = (layer->layerType == BUFFERED) ? BACKCHAN : BPCHAN;
    LAddToDirty(layer, b);
    PieceApplyBoundsProc(layer->tree, b, (void *)&bundle, BPRenderInBounds);
}


/*****************************************************************************
    NXApplyBounds
    Apply a boundary over the given layer and call the given callback
    procedure with the given data for each bitpiece that itersects the bounds.
    This is useful for drivers to query the window's visibility across a given
    area.
******************************************************************************/
void NXApplyBounds(Layer *layer, Bounds *b, void (*proc)(), void *data)
{
    BoundsBundle bundle;
 
    bundle.data = data;
    bundle.proc = proc;
    PieceApplyBoundsProc(layer->tree, b, (void *)&bundle, BPApplyBounds);
}


/*****************************************************************************
    Layer2Wd
    Convert id into a WindowDevice pointer.
******************************************************************************/
WindowDevice *Layer2Wd(Layer *layer)
{
    if (!layer) return NULL;
    DebugAssert(layer->type == LAYER);
    return (WindowDevice *) layer->psWin;
}


/*****************************************************************************
    LBackingBounds
    Return the layer's backing bitmap's boundary rectangle.
    Called by windowbitmap.c.
******************************************************************************/
Bounds *LBackingBounds(Layer *layer)
{
    NXBag *bag;
    DebugAssert(layer->type == LAYER && layer->local);
    for (bag = layer->bags; bag; bag = bag->next) {
	if (bag->device == layer->device)
	    return &bag->backbits->bounds;
    }
    return NULL;
}


/*****************************************************************************
    LGetBacking
    Return the layer's backing bits pointer and rowbytes.
    Only works on MP device! Called by WBOpenBitmap() in windowbitmap.c.
******************************************************************************/
int LGetBacking(Layer *layer, unsigned int **bits, int *rowbytes)
{
    NXBag *bag;
    DebugAssert(layer->type == LAYER && layer->local);
    for (bag = layer->bags; bag; bag = bag->next) {
	if (bag->device == layer->device) {
	    *bits = ((LocalBitmap *)bag->backbits)->bits;
	    *rowbytes = ((LocalBitmap *)bag->backbits)->rowBytes;
	    return 0;
	}
    }
    return 1;
}


/*****************************************************************************
    LBoundsAt
    Return the layer's boundary rectangle.
    Called by windowbitmap.c.
******************************************************************************/
Bounds *LBoundsAt(Layer *layer)
{
    DebugAssert(layer->type == LAYER);
    return &layer->bounds;
}


/*****************************************************************************
    LCompositeFrom
    Composite with the given layers and bounds.
******************************************************************************/
void LCompositeFrom(CompositeInfo *ci, Bounds dstBounds)
{
    Pattern pat0;
    CompositeOperation co;
    Layer *srcLayer = ci->swin;
    Layer *dstLayer = ci->dwin;
    Bounds hideBds;
    
    DebugAssert(dstLayer->type == LAYER);
    bzero(&co, sizeof(CompositeOperation));
    if (!(co.dst.any = dstLayer->tree.any))
        return;

    /* Make dstBounds global */
    OFFSETBOUNDS(dstBounds, dstLayer->bounds.minx, dstLayer->bounds.miny);
    co.srcBounds = dstBounds;
    co.mode = ci->op;
    co.instancing = INSTANCING(PSGetGStateExt(NULL));
    co.alpha = ci->alpha;
    if (co.mode != HIGHLIGHT) {
        co.info = ci->markInfo;
	co.info.priv = (DevPrivate *) PSGetGStateExt(NULL);
    }
    if (srcLayer) {
	co.delta.cd.x = dstBounds.minx-(srcLayer->bounds.minx+ci->offset.x);
        co.delta.cd.y = dstBounds.miny-(srcLayer->bounds.miny+ci->offset.y);
        OFFSETBOUNDS(co.srcBounds, -co.delta.cd.x, -co.delta.cd.y);
        /* Clip dstBounds against source and dest layer bounds; */
        /* if it's empty afterwards, we're done. */
        clipBounds(&co.srcBounds, &(srcLayer->bounds), &dstBounds);
	co.srcAS = srcLayer->alphaState;
    } else if (co.mode != HIGHLIGHT) {
        co.srcAS = (co.alpha == OPAQUE) ? A_ONE : A_BITS;
    }
    clipBounds(&dstBounds, &(dstLayer->bounds), &co.srcBounds);
    if (NULLBOUNDS(dstBounds))
        return;
    co.info.offset.x = dstLayer->bounds.minx;
    co.info.offset.y = dstLayer->bounds.miny;
    
    /* If this op will produce some non-one alpha, turn off the hint */
    if ((dstLayer->alphaState == A_ONE) && (co.mode != HIGHLIGHT) &&
        ((1<<co.mode) & needAlphaBits[co.srcAS]))
        LSetAlphaBits(dstLayer);	/* Initialize them bits */
    co.dstAS = dstLayer->alphaState;
    
    /* Add to instancing if instancing, or to dirty rect if buffered
     * or hide instance rect so drawing doesn't overlap if retained.
     */
    if (co.instancing)
        LAddToInstance(dstLayer, &dstBounds);
    else if (dstLayer->layerType == BUFFERED)
        boundBounds(&dstLayer->dirty, &dstBounds, &dstLayer->dirty);
    else if (dstLayer->layerType == RETAINED && dstLayer->instanceSet && 
	    ComputeHideBounds(dstBounds, dstLayer->instance, &hideBds))
        LHideInstanceIn(dstLayer, hideBds);

    /* Draw to backing in buffered windows and retained A_BITS windows */
    if (!co.instancing && (dstLayer->layerType == BUFFERED))
        co.dstCH = BACKCHAN;
    
    /* Set up source, depending on whether srcLayer is real or not */
    if (srcLayer) {
	co.srcAS = srcLayer->alphaState;
        if (co.mode == HIGHLIGHT)
            co.srcAS = A_ONE;
        else if (co.mode == DISSOLVE) {
            co.srcAS = A_BITS;
            if (co.alpha == TRANSPARENT)
                return;			/* NOP: useless to perform dissolve! */
            if (co.alpha == OPAQUE)
                co.mode = COPY;		/* Shortcut is COPY */
        }
        co.src.any = srcLayer->tree.any;
	
	/* Read from backing in buffered windows */
        if (srcLayer->layerType == BUFFERED)
            co.srcCH = BACKCHAN;
    } else if (co.mode != HIGHLIGHT) {	/* virtual source */
	PSetHalftone(&pat0, &co.info);
    	co.src.pat = &pat0;
    }

    /* Check for layer promotion */
    if (!dstLayer->capped && !co.instancing && !dstLayer->local) {
	if (co.src.any)
	    switch(co.src.any->type) {
	    case PATTERN:
		{
		    boolean isColor, pColor, pGray;
		    DevColorVal c = *((DevColorVal *)&co.src.pat->color);
		    unsigned char a = ALPHAVALUE(PSGetGStateExt(NULL));
		    unsigned int uc;
		    uc = (*(unsigned int *)&c) & 0xFFFFFF00|a;
		    isColor = pColor = pGray = false;
		    pGray = ((c.white!=NX_BLACK && c.white!=NX_DKGRAY &&
			c.white!=NX_LTGRAY && c.white!=NX_WHITE) ||
			(a!=0 && a!=0x55 && a!=0xAA && a!=0xFF));
		    if (isColor = !(c.red==c.green && c.green==c.blue))
			pColor = ((uc&0xF0F0F0F0) != ((uc<<4)&0xF0F0F0F0));
		    /* Must not reference the array if currentDepth == 24 or
		     * the depthLimit == 2. But since these represent capped
		     * conditions, we wouldn't be here anyway so it's okay.
		     */
		    if (pGray || isColor) {
			int depth = promoteParms[dstLayer->currentDepth-1]
			    [dstLayer->depthLimit-2][isColor][pColor][pGray];
			if (depth) LPromoteLayer(dstLayer, depth);
		    }
		    break;
		}
	    case DIVPIECE:
	    case BITPIECE:
		{
		    int depth = promoteDepths[dstLayer->depthLimit-2]
			[dstLayer->currentDepth-1][srcLayer->currentDepth-1];
		    if (depth) LPromoteLayer(dstLayer, depth);
		    break;
		}
	    }
    }
    PieceCompositeFrom(&co);
}


/*****************************************************************************
    LCopyBitsFrom
    Returns a local bitmap for a specified region inside a given layer. After
    using the bitmap, use the bm_delete macro to free the bitmap.  Also, the
    bits are left-justified, that is, not aligned to any screen.
    
    INPUT:
    	layer		Layer of interest.
	bounds		Bounds of interest.
	alphaWanted	True if alpha is desired.
    
    RETURN:
    	Ptr to a LocalBitmap (which the caller frees using using bm_delete).

******************************************************************************/
LocalBitmap *LCopyBitsFrom(Layer *layer, Bounds bounds, boolean alphaWanted)
{
    Bitmap *pbm;
    CBStruct cbs;
    BMCompOp bcop;
    BMClass *class;

    DebugAssert(layer->type == LAYER);
    /* Makes bounds global */
    OFFSETBOUNDS(bounds, layer->bounds.minx, layer->bounds.miny);
    /* If bounds are completely outside layer bounds, return null */
    if (!sectBounds(&bounds, &layer->bounds, &bounds))
        return NULL;

    class = bmClassFromDepth[layer->currentDepth];
    bcop.op = COPY;
    bcop.srcType = BM_BITMAPSRC;
    bcop.srcAS = layer->alphaState;
    bcop.dstAS = !alphaWanted;
    PSGetMarkInfo(NULL, &bcop.info);
    /* left-justify bits by forcing bounds.minx to be zero for copy */
    bcop.dstBounds = bounds;
    bcop.dstBounds.minx = 0;
    bcop.dstBounds.maxx = bounds.maxx - bounds.minx;
    cbs.bcop = &bcop;
    cbs.bm = bm_new(class, &bcop.dstBounds, false);
    if (alphaWanted) bm_newAlpha(cbs.bm, true);
    PieceApplyBoundsProc(layer->tree, &bounds, (void *)&cbs, BPCopyBitsFrom);
    pbm = bm_makePublic(cbs.bm, &cbs.bm->bounds, layer->currentDepth);
    bm_delete(cbs.bm);
    /* restore original requested bounds */
    pbm->bounds = bounds;
    return (LocalBitmap *)pbm;
}


/*****************************************************************************
    LCopyContents
    Copy contents of src layer to dst layer 
******************************************************************************/
void LCopyContents(Layer *src, Layer *dst)
{
    CompositeInfo ci;
    Bounds bounds;

    PSGetMarkInfo(NULL, &ci.markInfo);
    ci.op = COPY; ci.swin = src; ci.dwin = dst;
    ci.offset.x = ci.offset.y = 0;
    bounds.minx = bounds.miny = 0;
    bounds.maxx = src->bounds.maxx - src->bounds.minx;
    bounds.maxy = src->bounds.maxy - src->bounds.miny;
    LCompositeFrom(&ci, bounds);
}


/*****************************************************************************
    LCurrentAlphaState
    Return the current global state of the layer's alpha.
******************************************************************************/
int LCurrentAlphaState(Layer *layer)
{
    DebugAssert(layer->type == LAYER);
    return layer->alphaState;
}


/*****************************************************************************
    LCurrentDepth
    Return the current depth of the layer.
******************************************************************************/
int LCurrentDepth(Layer *layer)
{
    DebugAssert(layer->type == LAYER);
    return layer->currentDepth;
}


/*****************************************************************************
    LDepthLimit
    Return the current depth limit of the layer.
******************************************************************************/
int LDepthLimit(Layer *layer)
{
    DebugAssert(layer->type == LAYER);
    return layer->depthLimit;
}


/*****************************************************************************
    LFill
    Fills the layer with the contents of the windows which are "op" of
    the otherLayer.
******************************************************************************/
void LFill(Layer *layer, int op, Layer *otherLayer)
{
    int	i;
    Layer **l;
    CompositeInfo ci;
    SubList belowThere;
    CompositeOperation co;
    Bounds compositeBounds;
    
    ci.op = COPY;
    ci.dwin = layer;
    ci.alpha = 0;
    compositeBounds = layer->bounds;
    OFFSETBOUNDS(compositeBounds, -layer->bounds.minx, -layer->bounds.miny);

    /* Draw to backing in BUFFERED windows */

    belowThere = WLBelowButNotBelow(op, otherLayer, BELOW, NULL);
    for (i=0,l=belowThere.ptr+belowThere.len-1; i<belowThere.len; i++, l--)
	if (TOUCHBOUNDS(layer->bounds, (*l)->bounds)) {
	    if ((*l)->layerType != NONRETAINED) {
		ci.swin = *l;
		ci.offset.x = layer->bounds.minx - (*l)->bounds.minx;
		ci.offset.y = layer->bounds.miny - (*l)->bounds.miny;
		LCompositeFrom(&ci, compositeBounds);
	    } else {
		/* Have to fill in the the other layer's backPat by hand.
		 * Recreate the entire CompositeOp since PieceCompositeFrom
		 * trashes co.dst if it's a bitpiece.
		 */
		copyCO(&co);
		co.dst.any = layer->tree.any;
		if (layer->layerType == BUFFERED)
		    co.dstCH = BACKCHAN;
		co.info.offset.x = (*l)->bounds.minx;
		co.info.offset.y = (*l)->bounds.miny;
		co.src.pat = (*l)->exposure;
		sectBounds(&layer->bounds,&(*l)->bounds, &co.srcBounds);
		PieceCompositeFrom(&co);
		/* Update our layer's dirty rect if BUFFERED */
		if (layer->layerType == BUFFERED)
		    boundBounds(&layer->dirty, &co.srcBounds, &layer->dirty);
	    }
	}
}


/*****************************************************************************
    LFind
    Return the topmost layer in the global window list below the given
    position which contains the given point in upper-lefthand coordinates.
******************************************************************************/
Layer *LFind(int x, int y, int op, Layer *relWin)
{
    SubList sl;
    Layer *l;
    
    sl = WLBelowButNotBelow(op, relWin, BELOW, NULL);
    for (; sl.len; sl.len--, sl.ptr++) {
        l = *sl.ptr;
        if ((x >= l->bounds.minx) && (y >= l->bounds.miny)
        && (x < l->bounds.maxx) && (y < l->bounds.maxy))
            return l;
    }
    return NULL;
}


/*****************************************************************************
    LFindPieceBounds
    Return the address of the bounds of the BitPiece of this layer
    enclosing the given location.
******************************************************************************/
Bounds *LFindPieceBounds(Layer *layer, Point pt)
{
    DebugAssert(layer->type == LAYER);
    if ((pt.x < layer->bounds.minx) || (pt.x >= layer->bounds.maxx) ||
    (pt.y < layer->bounds.miny) || (pt.y >= layer->bounds.maxy)) {
#if STAGE==DEVELOP
        printf(os_stderr,"LFindPieceBounds:Lookout! pt outside bounds!\n");
#endif STAGE==DEVELOP
        return &layer->bounds;
    } else
        return PieceFindPieceBounds(layer->tree, pt);
}


/*****************************************************************************
    LFlushBits
    Flush backing store to screen if the layer is BUFFERED and its dirty
    rectangle is not empty.
******************************************************************************/
void LFlushBits(Layer *layer)
{
    DebugAssert(layer->type == LAYER);
    if ((layer->layerType == BUFFERED) && (!NULLBOUNDS(layer->dirty))) {
        if (layer->windowList)
	    CopyOnscreen(&layer->dirty, layer->tree);
        collapseBounds(&layer->dirty, &layer->bounds);
    }
}


/*****************************************************************************
    LFree
    Free a layer and all structures associated with it.
******************************************************************************/
void LFree(Layer *layer)
{
    NXBag *bag;

    DebugAssert(layer->type == LAYER);
    if (layer->windowList) {
        LRevealSubList(layer, WLBelowButNotBelow(BELOW, layer, BELOW, NULL));
        WLRemove(layer);
    }
    termwindowflag = true;
    if (layer->exposure) PFree(layer->exposure);
    if (layer->tree.dp) PieceFree(layer->tree);
    /* If special WindowBitmap window, be sure to free the last bag
     * if it exists...
     */
    if (layer->local && layer->bags)
	BAGDelete(layer->bags);
    termwindowflag = false;
    /* Mark the layer free and free it */
    layer->type = 0;
    os_freeelement(layerPool, layer);
}


/*****************************************************************************
    LGetDeviceStatus
    Return information about the screen.   KLUDGE
******************************************************************************/
DeviceStatus LGetDeviceStatus(Layer *layer)
{
    NXBag *bag;
    int bpp, color;
    DeviceStatus ds;

    DebugAssert(layer->type == LAYER);
    ds.minbitsperpixel = 9999;
    ds.maxbitsperpixel = 0;
    ds.color = 0;
    for (bag = layer->bags; bag; bag = bag->next) {
	switch (bag->device->visDepthLimit) {
	    case NX_TWOBITGRAY: bpp = 2; color = 0; break;
	    case NX_EIGHTBITGRAY: bpp = 8; color = 0; break;
	    case NX_TWELVEBITRGB: bpp = 12; color = 1; break;
	    case NX_TWENTYFOURBITRGB: bpp = 24; color = 1; break;
	    default: bpp = 0; color = 0; break;
	}
	if (bpp < ds.minbitsperpixel) ds.minbitsperpixel = bpp;
	if (bpp > ds.maxbitsperpixel) ds.maxbitsperpixel = bpp;
	if (color) ds.color = 1;
    }
    return ds;
}


/*****************************************************************************
    LGetSize
    Return the width and height of this layer.
******************************************************************************/
void LGetSize(Layer *layer, short *wp, short *hp)
{
    DebugAssert(layer->type == LAYER);
    *wp = layer->bounds.maxx - layer->bounds.minx;
    *hp = layer->bounds.maxy - layer->bounds.miny;
}


/*****************************************************************************
    LHideInstance
    Hide instancing inside local rectangle (Bounds are local).
******************************************************************************/
void LHideInstance(Layer *layer, Bounds hideBounds)
{
    DebugAssert(layer->type == LAYER);
    if (!layer->instanceSet) return;
    /* Turn hideBounds into global coordinates */
    OFFSETBOUNDS(hideBounds, layer->bounds.minx, layer->bounds.miny);
    LHideInstanceIn(layer, hideBounds);
}


/*****************************************************************************
    LHideInstanceIn
    Hide instancing inside global rectangle (Bounds are global).
******************************************************************************/
static void LHideInstanceIn(Layer *layer, Bounds hideBounds)
{
    CompositeOperation co;
    DebugAssert(layer->type == LAYER);
    copyCO(&co);
    if (sectBounds(&layer->instance, &hideBounds, &co.srcBounds)) {
        co.dst.any = layer->tree.any;
        if (layer->layerType == NONRETAINED)
            co.src.pat = layer->exposure;
        else {
            co.srcCH = BACKCHAN;
	    co.srcIsDst = true;
	}
        PieceCompositeFrom(&co);
        LRemoveFromInstance(layer, &hideBounds);
    }
}


/*****************************************************************************
    LInitPage
    InitPage function for the window device.  Makes sure the alpha state
    is reset to opaque.
******************************************************************************/
void LInitPage(PDevice pdevice)
{
    Layer *l = (Layer *) Wd2Layer(pdevice);
    uchar instancing = INSTANCING(PSGetGStateExt(NULL));
    CompositeOperation co;
    
    /* Add to instancing if instancing, or to dirty rect if not */
    if (instancing)
        LAddToInstance(l, &l->bounds);
    else if (l->layerType == BUFFERED)
        l->dirty = l->bounds;
    /* Determine the piece to draw in */
    copyCO(&co);
    co.dst.any = l->tree.any;
    co.dstCH = (instancing || (l->layerType != BUFFERED)) ? BPCHAN : BACKCHAN;
    co.src.pat = whitepattern;
    co.srcBounds = l->bounds;
    PieceCompositeFrom(&co);
    LSetAlphaOpaque(l);
}


/*****************************************************************************
    LMark
    Fill the intersection of the given graphic and clipper in the
    current layer, updating lastPiece in our instance variables.
    The clip may be NULL, in which case the whole graphic is
    drawn within the limits of the window.  Coordinates are local.
******************************************************************************/
void LMark(PDevice pdevice, DevPrim *graphic, DevPrim *clip, DevMarkInfo *info)
{
    MarkRec mrec;
    Layer *layer;
    Bounds markBds, clipBds, trimBds, hideBds;
    int instancing = INSTANCING(PSGetGStateExt(NULL));
    int usingGrayPatterns = GRAYPATSTATE(PSGetGStateExt(NULL));

    layer = (Layer *)Wd2Layer(pdevice);
    if (layer == NULL) PSInvalidID();
    DebugAssert(layer->type == LAYER);
    if (layer->layerType == NONRETAINED && !layer->windowList) return;
    mrec.device = pdevice;
    mrec.graphic = graphic;
    mrec.clip = clip;
    mrec.info = *info;
    /* Transform from local to global coordinates, init and trim bounds */
    mrec.info.offset.x = layer->bounds.minx;
    mrec.info.offset.y = layer->bounds.miny;
    BoundsFromIPrim(mrec.graphic, &markBds);
    if (mrec.clip) {
        BoundsFromIPrim(mrec.clip, &clipBds);
        if (!sectBounds(&markBds, &clipBds, &markBds)) return;
    }
    OFFSETBOUNDS(markBds, layer->bounds.minx, layer->bounds.miny);

    /* May need to call within to catch Reducer overflow */
    if (!sectBounds(&markBds, &layer->bounds, &trimBds)) return;

    /* Check layer promotion */
    if (!layer->capped && !instancing && !usingGrayPatterns && !layer->local) {
	boolean isColor, pColor, pGray;
	isColor = pColor = pGray = false;
	if (graphic->type == imageType) {
	    DevImageSource *source = graphic->value.image->source;
	    if (!graphic->value.image->unused) {
		if (source->nComponents > 1) {
		    isColor = true;
		    pColor = (source->bitspersample > 4);
		}
	    } else { /* alphaimage case */
		if (source->nComponents > 2) {
		    isColor = true;
		    pColor = (source->bitspersample > 4);
		}
	    }
	    pGray = (source->bitspersample > 2);
	} else {
	    DevColorVal c = *((DevColorVal *)&info->color);
	    unsigned char a = ALPHAVALUE(PSGetGStateExt(NULL));
	    unsigned int uc;
	    uc = (*(unsigned int *)&c)&0xFFFFFF00 | a;
	    pGray = ((c.white!=NX_BLACK && c.white!=NX_DKGRAY &&
		c.white!=NX_LTGRAY && c.white!=NX_WHITE) ||
		(a!=0 && a!=0x55 && a!=0xAA && a!=0xFF));
	    if (isColor = !(c.red==c.green && c.green==c.blue))
		pColor = ((uc&0xF0F0F0F0) != ((uc<<4)&0xF0F0F0F0));
	}
	/* Must not reference the array if currentDepth == 24 or
	 * the depthLimit == 2. But since these represent capped
	 * conditions, we wouldn't be here anyway so it's okay...
	 */
	if (pGray || isColor) {
	    int depth = promoteParms[layer->currentDepth-1]
		[layer->depthLimit-2][isColor][pColor][pGray];
	    if (depth) LPromoteLayer(layer, depth);
	}
    }

    /* If layer doesn't have alpha and the mark does, then the layer will */
    if (layer->alphaState != A_BITS) {
	if (graphic->type == imageType) {
	    DevImage *image = graphic->value.image;
	    if ((image->imagemask && ALPHAVALUE(PSGetGStateExt(NULL))!=OPAQUE)
	    || image->unused)
		LSetAlphaBits(layer); 
	}
	else if (ALPHAVALUE(PSGetGStateExt(NULL)) != OPAQUE)
	    LSetAlphaBits(layer); 
    }

    /* If instancing, add trimmed mark bounds to instance bounds,
     * else if BUFFERED window, add mark bounds to dirtyrect,
     * else if RETAINED window, hide instance rect so drawing doesn't overlap.
     */
    if (instancing)
	LAddToInstance(layer, &trimBds);
    else if (layer->layerType == BUFFERED)
	boundBounds(&layer->dirty, &trimBds, &layer->dirty);
    else if (layer->layerType == RETAINED && layer->instanceSet && 
	    ComputeHideBounds(trimBds, layer->instance, &hideBds))
        LHideInstanceIn(layer, hideBds);

    /* For BUFFERED windows, when not instancing, mark into backing.
     * Otherwise see if mark bounds are within bounds of cached piece.
     * If not, cascade down the bintree to find the piece, use and cache that.
     */
    if (!instancing && layer->layerType == BUFFERED)
        PieceMark(layer->tree, &mrec, &markBds, 0, BACKCHAN);
    else {
	if (layer->lastPiece&&withinBounds(&trimBds,&layer->lastPiece->bounds))
	    BPMark(layer->lastPiece, &mrec, &markBds, instancing, BPCHAN);
	else {
	    layer->lastPiece = PieceMark(layer->tree, &mrec, &markBds,
	        instancing, BPCHAN);
	}
    }
}


/*****************************************************************************
    LMoveTo
    Reposition a window in global coordinates.
******************************************************************************/
void LMoveTo(Layer *layer, Point newOrigin)
{
    NXBag *bag;
    Layer **l;
    short dx, dy;
    Bounds newPane, oldPane, tmpPane;
    SubList aboveList, belowList, dummyList;
    int screenBefore, screenAfter, reasonIndex, newCause, cause, i;

    /* Create destination rectangle */
    DebugAssert(layer->type == LAYER);
    oldPane = layer->bounds;
    newPane.minx = newOrigin.x;
    newPane.maxy = newOrigin.y;
    newPane.maxx = newOrigin.x + layer->bounds.maxx - layer->bounds.minx;
    newPane.miny = newOrigin.y - layer->bounds.maxy + layer->bounds.miny;
    dx = newPane.minx - layer->bounds.minx;
    dy = newPane.miny - layer->bounds.miny;
    if (dx==0 && dy==0)
        return;
    newCause = UniqueCause();
    screenAfter = screenBefore = reasonIndex = 0;
    aboveList.len = belowList.len = dummyList.len = 0;
    if (layer->windowList) {
        aboveList = WLAboveButNotAbove(ABOVE, layer, ABOVE, NULL);
        belowList = WLBelowButNotBelow(BELOW, layer, BELOW, NULL);
	for (i=0, l=(Layer **)offSubList.ptr; i<offSubList.len; i++, l++) {
	    if (!screenBefore && TOUCHBOUNDS(oldPane, (*l)->bounds))
		screenBefore = 1;
	    if (!screenAfter && TOUCHBOUNDS(newPane, (*l)->bounds))
		screenAfter = 1;
	}
	if (screenBefore||screenAfter)
            dummyList = offSubList;
    }
    /* Check for window conversion.  If the window remains within the same
     * extent before and after the move, then no conversion will take place.
     */
    if (extSubList.len > 1) {
        int old, new;
	old = new = -1;
	for (i=0, l=(Layer **)extSubList.ptr; i<extSubList.len; i++, l++) {
	    if (withinBounds(&oldPane, &(*l)->bounds)) old = i;
	    if (withinBounds(&newPane, &(*l)->bounds)) new = i;
	}
	if ( !((new >= 0) && (new == old)) ) {
	    screenBefore = screenAfter = 1;
	    dummyList = (dummyList.len) ? dummySubList : extSubList;
	}
    }

    /* Expand temp causes list if necessary */
    while (numTempCauses < (aboveList.len + dummyList.len))
        ExpandTempCauses();

    /* Obscure the parts of lower windows covered by my new position */
    for (i=0, l=(Layer **)belowList.ptr; i<belowList.len; i++, l++)
        LObscureInside(*l, &newPane, newCause);
    
    /* Obscure the parts of me that will be covered by other windows */
    for (i=0, l=(Layer **)aboveList.ptr; i<aboveList.len; i++, l++) {
        tmpPane = (*l)->bounds;
        OFFSETBOUNDS(tmpPane, -dx, -dy);
        LObscureInside(layer, &tmpPane, tempCauses[reasonIndex++]);
    }
    /* If we'll be partially offscreen afterwards, hide those parts of us */
    if (screenAfter)
        for (i=0, l=(Layer **)dummyList.ptr; i<dummyList.len; i++, l++) {
            tmpPane = (*l)->bounds;
            OFFSETBOUNDS(tmpPane, -dx, -dy);
            if ((*l)->extent) {
                deviceCause = (*l)->device;
                cause = MAKECONVERTCAUSE((*l)->causeId);
            } else
                cause = tempCauses[reasonIndex++];
            LObscureInside(layer, &tmpPane, cause);
        }
    if (layer->processHooks)
	for (bag=layer->bags; bag; bag = bag->next)
	    if (bag->hookMask & (1<<NX_MOVEWINDOW)) {
		hookData.type = NX_MOVEWINDOW;
		hookData.bag = bag;
		hookData.d.move.delta.x = dx;
		hookData.d.move.delta.y = dy;
		hookData.d.move.stage = 0;
		(*bag->procs->Hook)(&hookData);
	    }
    /* Offset tree and move bits on screen */
    PieceAdjust(layer->tree, dx, dy);
    /* Adjust contents of each bag in layer's bag list */
    for (bag=layer->bags; bag; bag = bag->next)
	(*bag->procs->MoveWindow)(bag, dx, dy, &layer->bounds, &newPane);
    if (layer->processHooks)
	for (bag=layer->bags; bag; bag = bag->next)
	    if (bag->hookMask & (1<<NX_MOVEWINDOW)) {
		hookData.bag = bag;
		hookData.d.move.stage = 1;
		(*bag->procs->Hook)(&hookData);
	    }
    layer->bounds = newPane;
    OFFSETBOUNDS(layer->dirty, dx, dy);
    if (layer->instanceSet)
        OFFSETBOUNDS(layer->instance, dx, dy);
    /* Reveal the parts of me that were covered by the old position of
       windows above me */
    reasonIndex = 0;
    for (i=0, l=(Layer **)aboveList.ptr; i<aboveList.len; i++, l++) {
        tmpPane = (*l)->bounds;
        OFFSETBOUNDS(tmpPane, dx, dy);
        LRevealInside(layer, &tmpPane, (*l)->causeId);
        /* Put old cause for this window back, if needed */
        if (layer->tree.any->type == DIVPIECE)
            DPSwapCause(layer->tree.dp, (*l)->causeId,
                tempCauses[reasonIndex++]);
    }
    /* Reveal pcs that were offscreen, give new offscreen pcs real causes */
    for (i=0, l=(Layer **)dummyList.ptr; i<dummyList.len; i++, l++) {
    	int lcauseid = (*l)->causeId;
        if (screenBefore) {
            tmpPane = (*l)->bounds;
            OFFSETBOUNDS(tmpPane, dx, dy);
	    cause = (*l)->extent ? MAKECONVERTCAUSE(lcauseid) : lcauseid;
            LRevealInside(layer, &tmpPane, cause);
        }
        if (screenAfter) {
	    cause = (*l)->extent ? MAKECONVERTCAUSE(lcauseid) :
		tempCauses[reasonIndex++];
            if (layer->tree.any->type == DIVPIECE)
                DPSwapCause(layer->tree.dp, lcauseid, cause);
        }
    }
    /* Reveal the parts of lower windows covered only by my old position */
    for (i=0, l=(Layer **)belowList.ptr; i<belowList.len; i++, l++)
        LRevealInside(*l, &oldPane, layer->causeId);
    layer->causeId = newCause;
}


/*****************************************************************************
    LNewInstance
    Erase all instancing and turn layer instancing off.
******************************************************************************/
void LNewInstance(Layer *layer)
{
    DebugAssert(layer->type == LAYER);
    if (layer->instanceSet) {
        LHideInstanceIn(layer, layer->instance);
        layer->instanceSet = false;
    }
}


/*****************************************************************************
    LObscureInside
    Obscure all pieces inside the given rectangle for the given layer.
    It will subdivide pieces if necessary to make them be either entirely
    inside or outside the given bounds.  Records the cause.
******************************************************************************/
static void LObscureInside(Layer *layer, Bounds *rect, int cause)
{
    Bounds isect;

    DebugAssert(layer->type == LAYER);
    if (sectBounds(rect, &layer->bounds, &isect)) {  /* Find intersection */
        layer->tree = PieceObscureInside(layer->tree, isect, cause);
        layer->lastPiece = NULL;  /* Invalidate graphics cache */
    }
}


/*****************************************************************************
    LObscureSubList
    Obscure all layers in the sublist by the bounds of the given layer.
******************************************************************************/
static void LObscureSubList(Layer *layer, SubList sl)
{
    int i;
    Layer **l;
    
    for (i=0, l=sl.ptr; i<sl.len; i++, l++)
        if (TOUCHBOUNDS(layer->bounds, (*l)->bounds))
            LObscureInside(*l, &(layer->bounds), layer->causeId);
}


/*****************************************************************************
    LOrder
    Change window order in the windowList.
******************************************************************************/
void LOrder(Layer *layer, int op, Layer *otherLayer)
{
    NXBag *bag;
    int curOne, curMinus;

    DebugAssert(layer->type == LAYER);
#if STAGE==DEVELOP
    if (otherLayer) DebugAssert(otherLayer->type == LAYER);
#endif
    if (op == OUT)
        if (!layer->windowList)
            return; /* No op */
        else
            layer->tree = PieceObscureBecause(layer->tree, OFFSCREENREASON);
    else {
        if (layer == otherLayer)
            return; /* Also very easy */
        if (otherLayer && !otherLayer->windowList)
            PSInvalidID(); /* Can't reference somebody not in window list */
    }
    if (!layer->windowList) {
        /* Putting into list, so obscure us by screen */
        curOne = curMinus = OUT;
        LSubListObscure(layer, offSubList);
    } else {
        /* Changing position in list */
        curOne = ABOVE;
	curMinus = BELOW;
    }
    
    /* Obscure the windows below our eventual position but not below our
       current one */
    LObscureSubList(layer, WLBelowButNotBelow(op, otherLayer, curOne, layer));

    /* Obscure ourselves within those windows above our eventual position but
       not above our current position */
    LSubListObscure(layer, WLAboveButNotAbove(op, otherLayer, curMinus,
	layer));

    /* Reveal ourselves inside those windows above our current position but not
       above our eventual position */
    LSubListReveal(layer, WLAboveButNotAbove(curOne, layer, op, otherLayer));

    /* Reveal those windows below our current position but not below our
       eventual position */
    LRevealSubList(layer, WLBelowButNotBelow(curMinus, layer, op, otherLayer));
    
    /* Special processing for list removal Leo 06Feb90, Ted 07Feb90 */
    if (op == OUT)
    	LSubListReveal(layer, offSubList);

    /* Special processing for list insertion */
    if (!layer->windowList)
        layer->tree = PieceRevealBecause(layer->tree, ONSCREENREASON);
    else
        WLRemove(layer);
    /* Now notify any devices that want to know about the ordering */
    if (layer->processHooks)
	for (bag=layer->bags; bag; bag = bag->next)
	    if (bag->hookMask & (1<<NX_ORDERWINDOW)) {
		hookData.type = NX_ORDERWINDOW;
		hookData.bag = bag;
		hookData.d.order.where = op;
		hookData.d.order.inWindowList = layer->windowList;
		(*bag->procs->Hook)(&hookData);
	    }
    if (op != OUT) {
        if (op == BELOW)
            WLPutAfter(layer, otherLayer);
        else
            WLPutBefore(layer, otherLayer);
        layer->windowList = 1;
    } else
       layer->windowList = 0;
}


/*****************************************************************************
    LPlaceAt
    Resize window.  NOTE:  Layer returned is different than layer passed!
******************************************************************************/
Layer *LPlaceAt(Layer *layer, Bounds newBounds)
{
    Bounds sct;
    int changed;
    Layer *newLayer;
    NXBag *bag, *nb, *ob;

    DebugAssert(layer->type == LAYER);
    LFlushBits(layer); /* Make sure we flush any dirty bits first */
    
    /* Create a new Layer, transfer exposure, flags, dirty, instance, device */
    newLayer = LNewAt(layer->layerType, newBounds, layer->psWin, layer->local,
    	layer->currentDepth, layer->depthLimit);
    newLayer->exposure = layer->exposure;
    newLayer->autoFill = layer->autoFill;
    newLayer->processHooks = layer->processHooks;
    newLayer->sendRepaint = layer->sendRepaint;
    sectBounds(&layer->dirty, &newLayer->bounds, &newLayer->dirty);
    if (layer->instanceSet)
        sectBounds(&layer->instance, &newLayer->bounds, &newLayer->instance);
    
    /* Insert in window list just after me, if I'm in the list */
    if (layer->windowList)
        LOrder(newLayer, BELOW, layer);

    /* If new and old layers intersect, copy intersection of backing stores
     * and allocate alpha bits for the new layer if the old layer has alpha bits.
     */
    if (IntersectAndCompareBounds(&newBounds, &layer->bounds, &sct) != outside) {
	if (layer->layerType != NONRETAINED) {
	    CompositeOperation co;
	    if (layer->alphaState == A_BITS)
		LSetAlphaBits(newLayer);
	    copyCO(&co);
	    co.src.any = layer->tree.any;
	    co.dst.any = newLayer->tree.any;
	    co.srcCH = co.dstCH = BACKCHAN;
	    co.srcBounds = sct;
	    PieceCompositeFrom(&co);
	}
    }
    /* Now notify any devices that want to know about the switch */
    if (layer->processHooks) {
	NXBag *newBag;
	hookData.type = NX_PLACEWINDOW;
	hookData.d.place.newBounds = newBounds;
	hookData.d.place.newLayer = newLayer;
  	for (bag=layer->bags; bag; bag = bag->next)
  	    if (bag->hookMask & (1<<NX_PLACEWINDOW)) {
		for (newBag=newLayer->bags; newBag; newBag = newBag->next)
		    if (newBag->device == bag->device) break;
  		hookData.bag = bag;
		hookData.d.place.newBag = newBag;
  		(*bag->procs->Hook)(&hookData);
  	    }
    }

    /* Send a Screen-Changed event if window changes screens */
    changed = 0;
    /* First check for new devices for layer */
    for (nb = newLayer->bags; nb; nb = nb->next) {
	for (ob = layer->bags; ob; ob = ob->next)
	    if (nb->device == ob->device) break;
	if (!ob) { changed = 1; break; }
    }
    if (!changed) /* Okay, now check for extinct layer screens */
	for (ob = layer->bags; ob; ob = ob->next) {
	    for (nb = newLayer->bags; nb; nb = nb->next)
		if (ob->device == nb->device) break;
	    if (!nb) { changed = 1; break; }
	}
    if (changed) {
	LRedraw(newLayer, &newLayer->bounds, REDRAW_CHANGED);
	FlushRedrawRects();
    }
    /* KLUDGE!!! In order to avoid obliterating those (perfectly good) bits
     * on the screen from the new layer's backing store when we destroy the
     * old layer, we'll tell the new layer that its backing store IS the
     * screen for a minute.
     */
    PieceApplyProc(newLayer->tree, BPPointScreen);

    layer->exposure = NULL; /* Don't free the newLayer's copy */
    LFree(layer);
    PieceApplyProc(newLayer->tree, BPReplaceBits);
    return newLayer;
}


/*****************************************************************************
    LPreCopyBitsFrom
    Call this routine before calling LCopyBitsFrom to know what type of bitmap
    it will return.
******************************************************************************/
int LPreCopyBitsFrom(Layer *layer)
{
    return logToPhysDepth[layer->currentDepth];
}


/*****************************************************************************
    LPrintOn
    Print out layer info:   Level	Information
			    0		type, alphaState
			    1		position, cause id
			    2		piece tree
******************************************************************************/
void LPrintOn(Layer *layer, int dumpLevel)
{
    NXBag *bag;
    const char *type;
    short (*proc)();
    int size;
    
    DebugAssert(layer->type == LAYER);
    switch (layer->layerType) {
	case DUMMY: type = "Dummy"; break;
	case RETAINED: type = "Retained"; break;
	case NONRETAINED: type = "Nonretained"; break;
	case BUFFERED: type = "Buffered"; break;
	default: type = "Unknown"; break;
    }
    size = 0;
    for (bag = layer->bags; bag; bag = bag->next)
	size += (*bag->procs->WindowSize)(bag);
    os_fprintf(os_stdout, "%s: (%d,%d),(%d,%d) %d%c bytes (%d)\n",
	       type,
	       layer->bounds.minx, layer->bounds.miny,
	       layer->bounds.maxx, layer->bounds.maxy,
	       size, (layer->alphaState==A_BITS) ? '+' : ' ', size);
    if (dumpLevel > 0) {
        if (layer->tree.any) {
            os_fprintf(os_stdout, "Bintree:\n");
            PiecePrintOn(layer->tree, 1);
        }
    }
    fflush(os_stdout);
}


/*****************************************************************************
    LPromoteLayer
    Blindly promotes a layer to the given depth.  You should be careful that
    the depth is appropriate for the layer, given its currentDepth and
    depthLimit values.  Use the static arrays "promoteParms" or
    "promoteDepths" to determine the appropriate depth.
******************************************************************************/
static void LPromoteLayer(Layer *layer, int depth)
{
    NXBag *bag;
    DevMarkInfo info;
    /* If the layer is RETAINED, make sure the backing store is
     * up-to-date before promoting it.
     */
    if (CopybackRetained(layer))
	PieceApplyProc(layer->tree, BPCopyback);
    PSGetMarkInfo(NULL, &info);
    for (bag = layer->bags; bag; bag = bag->next) {
	(*bag->procs->PromoteWindow)(bag, &layer->bounds, depth,
	    layer->layerType, info.screenphase);
    }
    layer->currentDepth = depth;
    layer->capped = (depth == layer->depthLimit);
}


/*****************************************************************************
    LRedraw
    When a window becomes exposed, respond by issuing a redraw event or 
    a screen-changed event dependind on the reason.
******************************************************************************/
void LRedraw(Layer *layer, Bounds *rp, int why)
{
    int miny, maxy, minx, maxx;
    Bounds r;
	
    maxy = wsBounds.maxy - rp->miny;
    miny = wsBounds.maxy - rp->maxy;
    maxx = rp->maxx;
    minx = rp->minx;
    GlobalToLocal(Layer2Wd(layer), &minx, &miny);
    GlobalToLocal(Layer2Wd(layer), &maxx, &maxy);
    r.minx = minx;
    r.miny = miny;
    r.maxx = maxx;
    r.maxy = maxy;
    if (why == REDRAW_EXPOSED)
        RedrawWindowDevice(Layer2Wd(layer), &r);
    else
        ChangedWindowDevice(Layer2Wd(layer), &r);
}


/*****************************************************************************
    LRemoveFromInstance
    Reduce instance rectangle by hideBounds.
******************************************************************************/
static void LRemoveFromInstance(Layer *layer, Bounds *hideBounds)
{
    Bounds hidInstance;
    BBoxCompareResult compare;

    DebugAssert(layer->type == LAYER);
    if (!layer->instanceSet) return;
    compare = IntersectAndCompareBounds(&layer->instance, hideBounds,
	&hidInstance);
    if (compare != outside) {
        if (compare == inside)
            layer->instanceSet = false;
        else {
            /* Reduce layer->instance to part that's still hid, if possible */
            if ((hidInstance.miny == layer->instance.miny)
            &&  (hidInstance.maxy == layer->instance.maxy)) {
                if (hidInstance.minx == layer->instance.minx)
                    layer->instance.minx = hidInstance.maxx;
                else if (hidInstance.maxx == layer->instance.maxx)
                    layer->instance.maxx = hidInstance.minx;
            } else {
                if ((hidInstance.minx == layer->instance.minx)
                &&  (hidInstance.maxx == layer->instance.maxx)) {
                    if (hidInstance.miny == layer->instance.miny)
                        layer->instance.miny = hidInstance.maxy;
                    else if (hidInstance.maxy == layer->instance.maxy)
                        layer->instance.maxy = hidInstance.miny;
                }
            }
        }
    }
}


/*****************************************************************************
    LRepaintIn
    This is called by low-level window code when it realizes there is a
    rectangle of a non-retained window which needs to be refreshed.
    It automatically fills and/or generates exposed events as necessary.
******************************************************************************/
void LRepaintIn(Layer *layer, Bounds bounds, NXBag *bag)
{
    DebugAssert(layer->type == LAYER);
    if (layer->autoFill) {
        CompositeOperation co;
	copyCO(&co);
        co.info.offset.x = layer->bounds.minx;
        co.info.offset.y = layer->bounds.miny;
        co.dst.bag = bag;
	co.dstCH = VISCHAN;
        co.src.pat = layer->exposure;
        co.srcBounds = bounds;
        BAGCompositeFrom(&co);
    }
    if (bag->hookMask & (1<<NX_REPAINTWINDOW)) {
	hookData.type = NX_REPAINTWINDOW;
	hookData.bag = bag;
	hookData.d.repaint.bounds = bounds;
	(*bag->procs->Hook)(&hookData);
    }

    if (layer->sendRepaint)
        LRedraw(layer, &bounds, REDRAW_EXPOSED);
}


/*****************************************************************************
    LRevealInside
    Reveal all pieces inside the given bounds for the given layer.
    Remove the cause.
******************************************************************************/
static void LRevealInside(Layer *layer, Bounds *rect, int cause)
{
    Bounds isect;

    DebugAssert(layer->type == LAYER);
    /* Find the intersection */
    if (sectBounds(rect, &layer->bounds, &isect) || ISCONVERTCAUSE(cause)) {
        layer->tree = PieceRevealInside(layer->tree, isect, cause);
        layer->lastPiece = NULL;  /* Invalidate graphics cache */
    }
}


/*****************************************************************************
    LRevealSubList
    Reveal all layers in the sublist by the bounds of the given layer.
******************************************************************************/
static void LRevealSubList(Layer *layer, SubList sl)
{
    int i;
    Layer **l;

    for (i=0, l=sl.ptr; i<sl.len; i++, l++)
        if (TOUCHBOUNDS(layer->bounds, (*l)->bounds))
            LRevealInside(*l, &(layer->bounds), layer->causeId);
}


/*****************************************************************************
    LSetAlphaBits
    Allocate alpha backing store for the layer if not already allocated.
    If we already allocated alpha, just initialize to opaque.
******************************************************************************/
static void LSetAlphaBits(Layer *layer)
{
    NXBag *bag;

    DebugAssert(layer->type == LAYER);
    DebugAssert(layer->alphaState != A_BITS);
    /* Nonretained windows have no backing store or storage for alpha.
     * Retained windows with alpha must have valid backing store over
     * the entire window, since all future marks and composites will be
     * done into the backing store and then flushed to the screen.
     */
    if (layer->layerType == NONRETAINED)
	return;
    /* When we add alpha to a Retained window for the first time, we
     * MUST valid the backing store by copying back all visible regions.
     * All work will now be done in the backing store (like with Buffered
     * windows) though immediately flushed (unlike Buffered windows).
     */
    if (CopybackRetained(layer))
	PieceApplyProc(layer->tree, BPCopyback);
    for (bag = layer->bags; bag; bag = bag->next) {
	(*bag->procs->NewAlpha)(bag);
    }
    layer->alphaState = A_BITS;
}


/*****************************************************************************
    LSetAlphaOpaque
    Set a layer's global alpha state.  Once a window has alpha bits allocated,
    they are never deallocated.
******************************************************************************/
static void LSetAlphaOpaque(Layer *layer)
{
    DebugAssert(layer->type == LAYER);
    layer->alphaState = A_ONE;
}


/*****************************************************************************
    LSetAutofill
    Set the layer's autoFill state:  Should I repaint when exposed?
******************************************************************************/
void LSetAutofill(Layer *layer, int flag)
{
    DebugAssert(layer->type == LAYER);
    layer->autoFill = flag;
}


/*****************************************************************************
    LSetDepthLimit
    Set the layer's depth limit.  If the current depth is greater than the
    new depth limit, then the window is dithered down to this new depth.
******************************************************************************/
void LSetDepthLimit(Layer *layer, int cap)
{
    NXBag *bag;
    DevMarkInfo info;
    
    DebugAssert(layer->type == LAYER);
    PSGetMarkInfo(NULL, &info);
    layer->depthLimit = cap;
    if (layer->currentDepth > cap) {
	if (CopybackRetained(layer))
	    PieceApplyProc(layer->tree, BPCopyback);
	layer->currentDepth = cap;
	for (bag = layer->bags; bag; bag = bag->next) {
	    (*bag->procs->PromoteWindow)(bag, &layer->bounds, cap,
		layer->layerType, info.screenphase);
	}
    }
    layer->capped = (cap == layer->currentDepth);
}


/*****************************************************************************
    LSetExposureColor
    Set the layer's background pattern to the current gstate's screen.
******************************************************************************/
void LSetExposureColor(Layer *layer)
{
    DevMarkInfo mi;

    DebugAssert(layer->type == LAYER);
    PSGetMarkInfo(NULL, &mi);
    if (layer->exposure->permanent)
	layer->exposure = PNew();
    PSetHalftone(layer->exposure, &mi); /* May be NULL */
    layer->exposure->alpha = OPAQUE;	/* Exposures are opaque */
}


/*****************************************************************************
    LSetSendRepaint
    Set the layer's sendRepaint state.
******************************************************************************/
void LSetSendRepaint(Layer *layer, int update)
{
    DebugAssert(layer->type == LAYER);
    layer->sendRepaint = update;
}


/*****************************************************************************
    LSetType
    Set the layer's layerType.
******************************************************************************/
int LSetType(Layer *layer, int newtype)
{
    DebugAssert(layer->type == LAYER);
    if (layer->layerType == newtype) return 1;
    if (newtype == BUFFERED && layer->layerType == RETAINED) {
	if (layer->alphaState != A_BITS)
	    PieceApplyProc(layer->tree, BPCopyback);
	collapseBounds(&layer->dirty, &layer->bounds);
	layer->layerType = BUFFERED;
	return 1;
    } else if (layer->layerType == BUFFERED && newtype == RETAINED) {
	LFlushBits(layer);
	layer->layerType = RETAINED;
	return 1;
    }
    return 0;
}

/*****************************************************************************
    LSubListObscure
    Obscure the given layer by all the bounds of the given sublist.
******************************************************************************/
static void LSubListObscure(Layer *layer, SubList sl)
{
    int i;
    Layer **l;

    for (i=0, l=sl.ptr; i<sl.len; i++, l++)
        if (TOUCHBOUNDS(layer->bounds, (*l)->bounds))
            LObscureInside(layer, &((*l)->bounds), (*l)->causeId);
}


/*****************************************************************************
    LSubListReveal
    Reveal the given layer by all the bounds of the given sublist.
******************************************************************************/
static void LSubListReveal(Layer *layer, SubList sl)
{
    int i;
    Layer **l;

    for (i=0, l=sl.ptr; i<sl.len; i++, l++)
        if (TOUCHBOUNDS(layer->bounds, (*l)->bounds))
            LRevealInside(layer, &((*l)->bounds), (*l)->causeId);
}








































