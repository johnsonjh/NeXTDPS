/*****************************************************************************

    extent.c

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 01Apr89 Ted Cohn

    Modified:

    05Apr89 Ted   EXBuildExtents creates screen extents over WorkSpace.
    20Apr89 Ted   Ripped out EXBuildExtents due to new extent method.
    22May89 Ted   Replace EXBuildExtents; lazy-conversion dosn't work.
    08Sep89 Ted   Dynamically configure screen layout from ascii file.
    18Sep89 Ted   Dynamically load device drivers into windowserver.
    26Mar90 Terry EXFindDevice now takes bps, planar, and nColors as args
    28Jun90 Ted   Made workSpaceBounds a constant static variable.
    10Aug90 Ted   Removed EXIsLocalDevice.
    
    PURPOSE:
	This file calculates and assigns "extent" boundaries to each screen.
	These are boundaries of influence, so to speak.  
    
    dummySubList
	This is a list of offscreen layers and extent layers.  This is used in
	hiding windows when they travel offscreen as well as conversion
	between devices.  It is a concatenation of extSubList and offSubList.

    extSubList
	This is a sublist of screen extents (active screens) where windows
	are to be converted between as well as where the cursor can travel.
 
    offSubList
	This sublist specifies the offscreen areas where windows are to be
	hidden.
	
    holeDevice
	This is a ptr to the thinnest device on the system, namely the one
	with the fewest bits/pixel (MegaPixel). It is used when a window moves
	to a hole in extent space.

******************************************************************************/

#import PACKAGE_SPECS
#import "bintreetypes.h"

/* Forward Declarations */
void EXAddToSubList(Layer *, SubList *);
void EXAllocDummies(BitPiece *);
void EXDummyScreens();
void EXBuildExtents();
void EXFindHoles(BitPiece *);

#define SB(d)		d->bounds
#define SE(d)		d->extent
#define HADJ(a,b)	(((a).miny<=(b).maxy)&&((a).maxy>=(b).miny))
#define VADJ(a,b)	(((a).minx<=(b).maxx)&&((a).maxx>=(b).minx))
#define EXABOVE(a,b)    ((a.maxy<=b.miny)&&(a.minx<b.maxx)&&(a.maxx>b.minx))
#define EXBELOW(a,b)    ((a.miny>=b.maxy)&&(a.minx<b.maxx)&&(a.maxx>b.minx))
#define EXLEFT(a,b)	((a.maxx<=b.minx)&&(a.miny<b.maxy)&&(a.maxy>b.miny))
#define EXRIGHT(a,b)    ((a.minx>=b.maxx)&&(a.miny<b.maxy)&&(a.maxy>b.miny))

static const Bounds workSpaceBounds = {-INF, INF, -INF, INF};

/*****************************************************************************
    EXInitialize
    Build screen extents for each device initialized on the system.
******************************************************************************/
void EXInitialize()
{
    extSubList.len = 0;
    extSubList.ptr = NULL;
    if (deviceList) {
	EXBuildExtents();
	EXDummyScreens();
    }
}


/*****************************************************************************
    EXAddToSubList
    This adds a given layer ptr to the given SubList.  It extends the
    SubList one element and fills in the new element.
******************************************************************************/
static void EXAddToSubList(Layer *layer, SubList *sl)
{
    Layer **newList;

    newList = (Layer **) malloc((sl->len+1)*sizeof(Layer *));
    if (sl->ptr) {
	bcopy(sl->ptr, newList, (sl->len)*sizeof(Layer *));
	free(sl->ptr);
    }
    sl->ptr = newList;
    sl->ptr[sl->len++] = layer;
}


/*****************************************************************************
    EXAllocDummies
    Allocates a dummy offscreen layer in dummySubList if it finds a
    bitpiece which is not obscured.  The bounds of this bitpiece serves
    as the bounds of this new dummy layer.
******************************************************************************/
static void EXAllocDummies(BitPiece *bp)
{
    if (bp->visFlag == OFFSCREEN)
	EXAddToSubList(LNewDummyAt(bp->bounds), &dummySubList);
}


/*****************************************************************************
    EXDummyScreens
    
    This procedure creates off-screen pseudo-Layers that surround the active
    screens. They are used for obscuring windows that go out of the active
    screen bounds. A SubList consisting of these layers is stored in the
    global variable dummySubList.
    
    It uses the bintree routines as a resource to find out what the offscreen
    rectangles are since bintree is so adept at dividing things up!  You
    obscure a bitpiece the size of the WorkSpace with each active screen
    boundary.  Then you create offscreen dummy layers from the bounds of the
    remaining visible bitpieces.
******************************************************************************/
static void EXDummyScreens()
{
    Piece tmp;
    NXDevice *d;

    /* Create fake initial bitpiece the size of the workspace */
    /* We will then divide it up by the screen devices installed */
    tmp.bp = BPNewAt(NULL, NULL, workSpaceBounds, OFFSCREEN, NULL, NULL, NULL);
    dummySubList = extSubList;
    
    /* For each device, obscure the current bintree by its bounds */
    for (d = deviceList; d; d = d->next)
	tmp = PieceObscureInside(tmp, d->bounds, ONSCREENREASON);
    PieceApplyProc(tmp, EXAllocDummies);
    extSubList.ptr = dummySubList.ptr;
    offSubList.ptr = extSubList.ptr + extSubList.len;
    offSubList.len = dummySubList.len - extSubList.len;
}


/*****************************************************************************
    EXBuildExtents
    Algorithm to find screen extents for conversion. (24Jun90 - it takes
    about 868 bytes!)
******************************************************************************/
static void EXBuildExtents()
{
    Piece tmp;
    int i = INF;
    Layer *layer;
    NXDevice *a, *b;

    /* Set all extents to maximum size (-INF,-INF,+INF,+INF) */
    for (a=deviceList; a; a=a->next)
	SE(a).minx = SE(a).miny = -(SE(a).maxx = SE(a).maxy = i);

    /* Perform an initial bounds check */
    for (a=deviceList; a; a=a->next)
	for (b=deviceList; b; b=b->next) {
	    if (a==b) continue;
	    if (SB(a).minx == SB(b).maxx && HADJ(SB(a),SB(b)))
		SE(a).minx = SB(a).minx;
	    else if (SB(a).maxx == SB(b).minx && HADJ(SB(a),SB(b)))
		SE(a).maxx = SB(a).maxx;
	    else if (SB(a).miny == SB(b).maxy && VADJ(SB(a),SB(b)))
		SE(a).miny = SB(a).miny;
	    else if (SB(a).maxy == SB(b).miny && VADJ(SB(a),SB(b)))
		SE(a).maxy = SB(a).maxy;
	}
    for (a=deviceList; a; a=a->next)
	for (b=deviceList; b; b=b->next) {
	    if (a==b) continue;
	    if ((SE(a).minx == -INF) && EXLEFT(SB(b),SB(a)))
		SE(a).minx = SB(b).maxx;
	    else if ((SE(a).maxx == INF) && EXRIGHT(SB(b),SB(a)))
		SE(a).maxx = SB(b).minx;
	    else if ((SE(a).miny == -INF) && EXABOVE(SB(b),SB(a)))
		SE(a).miny = SB(b).maxy;
	    else if ((SE(a).maxy == INF) && EXBELOW(SB(b),SB(a)))
		SE(a).maxy = SB(b).miny;
	}
    for (a=deviceList; a; a=a->next)
	for (b=deviceList; b; b=b->next) {
	    if (a==b) continue;
	    if ((SE(b).maxx > SE(a).minx) && (SE(b).maxx <= SB(a).minx) &&
	    EXLEFT(SE(b),SB(a)))
		SE(a).minx = SE(b).maxx;
	    else if ((SE(b).minx < SE(a).maxx) && (SE(b).minx <= SB(a).maxx) &&
	    EXRIGHT(SE(b),SB(a)))
		SE(a).maxx = SE(b).minx;
	    else if ((SE(b).maxy > SE(a).miny) && (SE(b).maxy <= SB(a).miny) &&
	    EXABOVE(SE(b),SB(a)))
		SE(a).miny = SE(b).maxy;
	    else if ((SE(b).miny < SE(a).maxy) && (SE(b).miny <= SB(a).maxy) &&
	    EXBELOW(SE(b),SB(a)))
		SE(a).maxy = SE(b).miny;
	}
    /* Create extent sublists from the recently create extent rectangles */
    tmp.bp = BPNewAt(NULL, NULL, workSpaceBounds, OFFSCREEN, NULL, holeDevice, NULL);
    for (a=deviceList; a; a=a->next) {
	layer = LNewDummyAt(a->extent);
	layer->device = a;
	layer->extent = true;
	EXAddToSubList(layer, &extSubList);
	tmp = PieceObscureInside(tmp, a->extent, layer->causeId);
    }
    PieceApplyProc(tmp, EXFindHoles);
}


/*****************************************************************************
    EXFindHoles
    Finds extent holes and adds them to the extent list owned by the
    "holeDevice".  Called by EXBuildExtents().
******************************************************************************/
static void EXFindHoles(BitPiece *bp)
{
    Layer *layer;
    
    if (bp->visFlag == OFFSCREEN) {	 /* found a hole in extent space */
	layer = LNewDummyAt(bp->bounds); /* hole becomes unique extent */
	layer->device = holeDevice;	 /* holeDevice assumes control */ 
	layer->extent = true;		 /* yes, this is an extent layer */
	EXAddToSubList(layer, &extSubList);
    }
}









