/*****************************************************************************

    windowbitmap.c
    Procedures for Windowbitmap windows.

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Dave 16Feb89

    Modified:

    28Mar90 Terry Redesigned for multiple framebuffer WindowServer
    02Apr90 Terry Added BitmapInfo support
    24Jun90 Ted   Removed dependency on FIXED window type for new API
    25Jun90 Ted   Updated WBCopyLayer to pass new LNewAt parameters
    28Jun90 Ted   Modified LGetBacking to return error status

******************************************************************************/

#import <mach.h>
#import <sys/types.h>
#import <sys/file.h>
#import <sys/mman.h>
#import <sys/message.h>
#import PACKAGE_SPECS
#import BINTREE
#import WINDOWDEVICE
#import "ipcscheduler.h"
#import "wbcontext.h"

/* static variables */
static int currentwid = 0;
static WindowDevice *currentwin;
static Layer *currentLayer;

/* external declarations */
extern WBContext *currentWBContext;
extern WBContext *WBCList;

/* forward declarations */
static boolean WBCurrentLayer(int);
static Layer *WBID2Layer(int);
static Layer *WBCopyLayer(Layer *, WindowDevice *, int);

int WBOpenBitmap(NXWBOpenMsg *msg)
{
    long size;
    unsigned int *bits;
    int	shmemfd;
    int rowbytes;
    WBContext *wbc;
    Layer *newLayer;
    Bounds *lbounds, *bbounds;
    
    /*  The client is sending the window id for a windowbitmap.
     *  Replace the window's layer with a vm_allocated layer, 
     *  Create a shmem file, map the backing store of the layer to it
     *  and send a message to the client with the file path so he can do same.
     */
    /* Ensure that this window is not already windowbitmapped */
    for (wbc = WBCList; wbc; wbc = wbc->next)
	if (wbc->wid == msg->wid)
	    return NXWB_EDUP;

    if (!WBCurrentLayer(msg->wid))
	return NXWB_ENOWIN;
    
    /* Create and open a temporary file to share the bitmap */
    strcpy(msg->path, "/tmp/wbsXXXXXX");
    if ((shmemfd = mkstemp(msg->path)) < 0)
	return NXWB_ENOSHMEM;
    strcpy(currentWBContext->shmemPath, msg->path);

    /* Create a 2-bit planar, 1 color layer with local memory */
    if (!(newLayer = WBCopyLayer(currentLayer, currentwin, 1)))
    	return NXWB_ENOSHMEM;
    LCopyContents(currentLayer, newLayer);

    /* Share the layer's backing store with the file descriptor */
    lbounds = LBoundsAt(currentLayer);
    if (LGetBacking(newLayer, &bits, &rowbytes)) {
	LFree(newLayer);
        return NXWB_ENOSTAT;
    }
    size = rowbytes * (lbounds->maxy - lbounds->miny) + 4;
    if (mmap(bits, size, PROT_READ|PROT_WRITE, MAP_SHARED, shmemfd, 0) < 0) {
	LFree(newLayer);
	close(shmemfd);
	return NXWB_ENOSHMEM;
    }
    
    /* Copy contents to newLayer and free currentLayer */
    currentwin->layer = newLayer;
    LFree(currentLayer);
    currentLayer = newLayer;
    
    /* Store window id and file descriptor in current wbcontext */
    currentWBContext->wid = msg->wid;
    currentWBContext->shmemfd = shmemfd;

    /* Fill in a return message */
    WBGetDeviceInfo(msg, currentwid);
    msg->header.msg_id = NXWB_NOERR;
    return NXWB_NOERR;
}

void WBChangeBitmap(int wid)
{
    long size;
    unsigned int *bits;
    Layer *layer;
    Bounds *bounds;
    WBContext *wbc;
    
    /* If wid used by some WBContext, update shared memory */
    layer = WBID2Layer(wid);
    for (wbc = WBCList; wbc; wbc = wbc->next)
	if (wbc->wid == wid) {
	    /* Close old file and make new file for shared memory paging */
	    close(wbc->shmemfd);
	    strcpy(wbc->shmemPath, "/tmp/wbsXXXXXX");
	    if ((wbc->shmemfd = mkstemp(wbc->shmemPath)) < 0)
		CantHappen();
    
	    /* Share the layer's backing store with the file descriptor */
	    bounds = LBoundsAt(layer);
	    LGetBacking(layer, &bits, (int *)&size);
	    size = size * (bounds->maxy-bounds->miny) + 4;
	    if (mmap(bits, size, PROT_READ|PROT_WRITE, MAP_SHARED,
	    	     wbc->shmemfd, 0) < 0)
		CantHappen();

	    /* Context will tell its client to adjust its shmem in WBCoProc */
	    wbc->dirtyShmem = wbc->scheduler->wannaRun = true;
	    return;    
	}
}

int WBCloseBitmap(int wid)
{
    Layer *newLayer;
    
    if (!WBCurrentLayer(wid))
	return NXWB_ENOWIN;

    if (currentWBContext->shmemfd == -1) /* layer already deleted */
	return NXWB_NOERR;
    
    /* Close the shmem file and ensure that we don't do it again */
    close(currentWBContext->shmemfd);
    currentWBContext->shmemfd = -1;

    /* Switch to a new layer if window is going to stick around */
    if (currentwin->exists) {
	/* Create a 2-bit planar, 1 color layer without localMemory */
	newLayer = WBCopyLayer(currentLayer, currentwin, 0);
    	LFree(currentLayer);
    
	/* Place the new layer into the current window */
	currentwin->layer = newLayer;
	currentLayer = newLayer;
    }
    return NXWB_NOERR;
}

int WBGetBitmap(NXWBMessage *msg)
{
    Bounds *lbounds, *bbounds;
    
    if (!WBCurrentLayer(msg->wid))
	return NXWB_ENOWIN;
        
    /*  The client wants to get a window bitmap, most likely
     *  for synchronisation.
     *  In this case, the inline data contains information
     *  about the size of the bitmap and a pointer to the bits.
     */
    lbounds = LBoundsAt(currentLayer);
    bbounds = LBackingBounds(currentLayer);
    msg->roi.origin.x = (NXCoord)(lbounds->minx - bbounds->minx);
    msg->roi.origin.y = (NXCoord)(lbounds->miny - bbounds->miny);
    msg->roi.size.width = (NXCoord)(lbounds->maxx - lbounds->minx);
    msg->roi.size.height = (NXCoord)(lbounds->maxy - lbounds->miny);
    msg->header.msg_id = NXWB_NOERR;
    return NXWB_NOERR;
}

void WBGetDeviceInfo(NXWBOpenMsg *msg, int wid)
{
    unsigned int *bits;
    int rowBytes;
    Layer *layer;
    DeviceStatus d;
    Bounds *bbounds, *lbounds;
    
    if ((layer = WBID2Layer(wid)) == NULL)
        return;
    
    lbounds = LBoundsAt(layer);
    bbounds = LBackingBounds(layer);
    LGetBacking(layer, &bits, &rowBytes);
    d = LGetDeviceStatus(layer);
    msg->bitsPerPixel = d.maxbitsperpixel;
    msg->devStatus = (d.color ? 1 : 0) | 2; /* zero is white is true */
    msg->rowBytes = rowBytes;
    msg->roi.origin.x = (NXCoord)(lbounds->minx - bbounds->minx);
    msg->roi.origin.y = (NXCoord)(lbounds->miny - bbounds->miny);
    msg->roi.size.width = (NXCoord)(lbounds->maxx - lbounds->minx);
    msg->roi.size.height = (NXCoord)(lbounds->maxy - lbounds->miny);
}
    
int WBMarkBitmap(NXWBMessage *msg)
{
    Bounds *bbounds, mbounds;

    if (!WBCurrentLayer(msg->wid))
	return NXWB_ENOWIN;

    /*  The client wants to draw on a window bitmap.
     *  In this case, the inline data contains information
     *  about the size of the bitmap and a pointer to the bits.
     */
    /* Create a mark rect in global coords and mark it dirty */
    bbounds = LBackingBounds(currentLayer);
    mbounds.minx = bbounds->minx + msg->roi.origin.x;
    mbounds.maxx = mbounds.minx + msg->roi.size.width;
    mbounds.miny = bbounds->miny + msg->roi.origin.y;
    mbounds.maxy = mbounds.miny + msg->roi.size.height;
    LAddToDirty(currentLayer, &mbounds);
    return NXWB_NOERR;
}

int WBFlushBitmap(NXWBMessage *msg)
{
    if (!WBCurrentLayer(msg->wid))
	return NXWB_ENOWIN;
    
    LFlushBits(currentLayer);
    return NXWB_NOERR;
}

static boolean WBCurrentLayer(int wid)
{
    Layer *l;

    if (currentwid == 0 || wid != currentwid) {
	if ((l = WBID2Layer(wid)) == NULL)
	    return false;
	currentwid = wid;
	currentLayer = l;
    } else
	/* in case the layer on the window has changed (resize, say) */
	currentLayer = currentwin->layer;
    return true;
}

static Layer *WBID2Layer(int wid)
{
    PWindowDevice win;
    extern PWindowDevice windowBase;
    
    for (win = windowBase; win != NULL; win = win->next) {
	if (win->id == wid) {
	    currentwin = win;
	    return (win->layer);
	}
    }
    return NULL;
}

static Layer *WBCopyLayer(Layer *oldLayer, WindowDevice *oldwin, int local)
{
    Bounds *bounds;

    bounds = LBoundsAt(oldLayer);
    return LNewAt(BUFFERED, *bounds, (int *)oldwin, local, NX_TWOBITGRAY,
	NX_TWOBITGRAY);
}



