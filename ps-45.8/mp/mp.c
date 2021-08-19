/*****************************************************************************

    mp.c
    This driver supports one instantiation of the MegaPixel display.

    CONFIDENTIAL
    Copyright (c) 1989 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 27Feb89 Ted Cohn

    Modified:

    10Jan90 Terry Added 32->2 bit conversion support for compositing
    02Feb90 Terry Added MPWakeup, MPSetupImageArgs, MPSetupMark,
		  and MPSetupPattern from windowdevice.c
    06Feb90 Terry Changed from BitMap to MPBitmap
    06Feb90 Jack  Put alpha and premultiplication conversion into MPMark
    07Feb90 Terry Moved MPConvert32to2 and MPGetBricks into convert.c
    09Feb90 Terry Give markInfo defaultHalftone if it doesn't have one
    12Feb90 Jack  mpDevHalftone public, use instead of defaultHalftone
    23Feb90 Ted	  Adding separated cursor support.
    20Mar90 Terry Move device setup routines into mpdev.c
    28Mar90 Terry Added VMAlloc + VMFree macros for windowbitmap support
    02Apr90 Terry Added BitmapInfo to MPNewBitmap
    29Mar90 Ted   Added PixelStats initialization to driver struct
    04Apr90 Terry Added support for marking interleaved alphaimages
    16May90 Ted   Added support for new window-info protocol.
    30May90 Terry MPSetAlpha uses while loop (not copyCO) (6500 to 6388 bytes)
    26Jun90 Peter Fixed pointer dereference in MPFreeWindow
    05Jul90 Ted   Began transformation towards new bitmap classes.
    21Jul90 Ted   Removed MPExportBitmap, MPFreeBitmap and MPNewBitmap.
    
******************************************************************************/

#import <mach.h>
#import <sys/file.h>
#import <sys/ioctl.h>
#import <sys/types.h>
#import PACKAGE_SPECS
#import CUSTOMOPS
#import PUBLICTYPES
#import DEVICETYPES
#import DEVPATTERN
#import BINTREE
#import EXCEPT
#import WINDOWDEVICE
#import "bitmap.h"
#import "mp.h"
#import "mp12.h"
#import <nextdev/video.h>
#import <nextdev/evio.h>
#import <nextdev/kmreg.h>

#define VMAlloc(ptr, size) \
    vm_allocate(task_self(), (vm_address_t *)&ptr, size, TRUE)
#define VMFree(ptr, size) \
    vm_deallocate(task_self(), (vm_address_t)ptr, size)

/* BM34 is not yet finished (no dithering), so we should not use it
   in the class table, unless the software is running on a beta
   Warp9 C.  Only mp12 and bm38 are fully implemented.
 */
static BMClass *bmClassFromDepth_MONO[] = {mp12, mp12, bm38, bm38, bm38};
static BMClass *bmClassFromDepth_COLOR[] = {mp12, mp12, bm38, bm34, bm38};
static BMClass **bmClassFromDepth;

static struct km_console_info km_coni;
int monitorType; /* 0 is 2 bit mono, 1 is 16 bit color */
static int mpFd;
uint *mpAddr;
uint screenOffsets[4];
uint memoryOffsets[4];
static int bitmapAddrInited = 0;
static uint *lowestBitmapAddr;
static uint *highestBitmapAddr;
int use_wf_hardware = 0;

static NXProcs mpProcs = {
    MPComposite,
    MPDisplayCursor2,
    MPFreeWindow,
    0,
    MPInitScreen,
    MPMark,
    MPMoveWindow,
    MPNewAlpha,
    MPNewWindow,
    0,	/* Ping */
    MPPromoteWindow,
    MPRegisterScreen,
    MPRemoveCursor2,
    MPSetCursor,
    0,
    MPWindowSize
};

/*****************************************************************************
    MPStart
    Is called just after the driver is dynamically loaded.
    It sets up the connection to its hardware screen device
    and fills in the proc field of its screen structure which
    is returned.  Returns 1 if errors.	
******************************************************************************/
int MPStart(NXDriver *driver)
{
    int i;
    if ((mpFd = open("/dev/vid0", O_RDWR, 0)) == -1) {
        os_fprintf(os_stderr, "MP Driver: can't open /dev/vid0\n");
	return 1;
    }
    if (ioctl(mpFd, DKIOCGADDR, &mpAddr) == -1) {
	os_fprintf(os_stderr, "MP Driver: can't map MegaPixel video\n");
	return 1;
    }
    monitorType = MP_MONO_SCREEN;
    if (ioctl(mpFd, DKIOCGFBINFO, &km_coni) == -1) {
	/* assume monochrome screen (i.e., old kernel) */
	bmClassFromDepth = bmClassFromDepth_MONO;
    } else {
	if (km_coni.bytes_per_scanline == C16_VIDEO_NBPL) {
	    bmClassFromDepth = bmClassFromDepth_COLOR;
	    monitorType = MP_COLOR_SCREEN;
	    use_wf_hardware = 0;
	    mpProcs.DisplayCursor = MPDisplayCursor16;
	    mpProcs.RemoveCursor = MPRemoveCursor16;
	} else {
	    bmClassFromDepth = bmClassFromDepth_MONO;
	    monitorType = MP_MONO_SCREEN;
	}
    }
    for (i=0; i<4; i++)
	screenOffsets[i] = (i+1)*0x01000000;

    driver->name = "MegaPixel";
    driver->procs = &mpProcs;
    driver->priv = NULL;

    NXRegisterScreen(driver, 0, 0, MPSCREEN_WIDTH, MPSCREEN_HEIGHT);
    return 0;
}


/*****************************************************************************
    MPComposite
******************************************************************************/
void MPComposite(CompositeOperation *cop, Bounds *dstBounds)
{
    BMCompOp bcop;
    Bitmap *psb, *csb;	/* public source bitmap and converted souce bitmap */
    Bitmap *db;		/* destination bitmap */

    psb = csb = NULL;
    bcop.op = cop->mode;
    bcop.dstBounds = *dstBounds;
    bcop.srcBounds = cop->srcBounds;
    bcop.srcAS = cop->srcAS;
    bcop.dstAS = cop->dstAS;
    bcop.info = cop->info;
    bcop.dissolveFactor = cop->alpha;

    db = (cop->dstCH==VISCHAN)?cop->dst.bag->visbits : cop->dst.bag->backbits;
    bcop.srcType = BM_NOSRC;	/* default src */
    if (cop->src.any) {
	switch (cop->src.any->type) {
	case BAG:
	{
	    Bitmap *sb;
	    bcop.srcType = BM_BITMAPSRC;
	    sb = bcop.src.bm = (cop->srcCH==VISCHAN) ? cop->src.bag->visbits :
		cop->src.bag->backbits;
	    if (bcop.src.bm->isa != db->isa) {
		bcop.src.bm = psb = bm_makePublic(bcop.src.bm,
		    &bcop.srcBounds, db->type);
		if (psb->type != db->type) {
		    DevPoint netphase;
		    
		    netphase.x = bcop.info.screenphase.x + bcop.info.offset.x;
		    netphase.y = bcop.info.screenphase.y + bcop.info.offset.y;
		    if (cop->mode == COPY) {
			bm_convertFrom(db, psb, &bcop.dstBounds,
			    &bcop.srcBounds, netphase);
			goto Cleanup;
		    } else {
			csb = bm_new(db->isa, &bcop.srcBounds, false);
			bm_convertFrom(csb, psb, &csb->bounds, &csb->bounds,
			    netphase);
			bcop.src.bm = csb;
		    }
		}
	    }
	    break;
	}
	case PATTERN:
	    bcop.srcType = BM_PATTERNSRC;
	    bcop.src.pat = cop->src.pat;
	    break;
	}
    }
    bm_composite(db, &bcop);
 Cleanup:
    if (psb) bm_delete(psb);
    if (csb) bm_delete(csb);
}


/*****************************************************************************
    MPFreeWindow
    Free backbits.
******************************************************************************/
void MPFreeWindow(NXBag *bag, int termflag)
{
    if (bag->backbits) bm_delete(bag->backbits);
}


/*****************************************************************************
    MPInitScreen
    Initialize our screen.
******************************************************************************/
void MPInitScreen(NXDevice *device)
{
    device->romid = MP_ROMID;
    if (monitorType == MP_COLOR_SCREEN) {
	device->bm =
	    bm_newFromData(bm34, &device->bounds, mpAddr, NULL,
			   (device->bounds.maxy - device->bounds.miny)
			   *km_coni.bytes_per_scanline,
			   km_coni.bytes_per_scanline, true, LBM_DONTFREE);
	device->visDepthLimit = NX_TWELVEBITRGB;
    } else {
	device->bm = bm_newFromData(mp12, &device->bounds, mpAddr, NULL,
			   (device->bounds.maxy - device->bounds.miny)
				    *MPSCREEN_ROWBYTES,
				    MPSCREEN_ROWBYTES, true, LBM_DONTFREE);
	device->visDepthLimit = NX_TWOBITGRAY;
    }
}


/*****************************************************************************
    MPMark
******************************************************************************/
void MPMark(NXBag *bag, int channel, MarkRec *mrec, Bounds *markBds,
    Bounds *bpBds)
{
    Bitmap *bm = (channel == VISCHAN) ? bag->visbits : bag->backbits;
    bm_mark(bm, mrec, markBds, bpBds);
}


/*****************************************************************************
    MPMoveWindow
******************************************************************************/
void MPMoveWindow(NXBag *bag, short dx, short dy, Bounds *old, Bounds *new)
{
    if (bag->backbits) bm_offset(bag->backbits, dx, dy);
}


/*****************************************************************************
    MPNewAlpha
    Allocate and initialize alpha bits.
******************************************************************************/
void MPNewAlpha(NXBag *bag)
{
    if (bag->backbits) bm_newAlpha(bag->backbits, true);
}


/*****************************************************************************
    MPNewWindow
******************************************************************************/
void MPNewWindow(NXBag *bag, Bounds *bounds, int windowType, int depth,
		 int local)
{
    NXWindowInfo wi;
    bag->visbits = bag->device->bm;
    if (windowType == NONRETAINED) return;
    depth = (depth<NX_OTHERBMTYPE)?NX_OTHERBMTYPE:(depth>NX_TWENTYFOURBITRGB)?
	NX_TWENTYFOURBITRGB:depth;
    /* If screen depth doesn't match initial depth, then set mismatch flag.
     * NOTE: Even though this is done for Retained windows only, Buffered
     * windows can become Retained via the setwindowtype operator.
     */
    if (depth != NX_TWOBITGRAY)
	bag->mismatch = 1;
    bag->backbits = bm_new(bmClassFromDepth[depth], bounds, local);
}


/*****************************************************************************
    MPPromoteWindow
******************************************************************************/
void MPPromoteWindow(NXBag *bag, Bounds *bounds, int newDepth,
		     int windowType, DevPoint phase)
{
    Bitmap *bm, *pub;
    BMClass *class;

    bm = bag->backbits;
    class = (BMClass *) bmClassFromDepth[newDepth];
    if (windowType == NONRETAINED || newDepth == bag->backbits->type ||
    class == bm->isa)
	return;
    /* allocate a new bitmap with the same vm characteristics */
    bm = bm_new(class, bounds, ((LocalBitmap *)bm)->freeMethod ==
		LBM_VMDEALLOCATE);
    pub = bm_makePublic(bag->backbits, bounds, newDepth);
    bm_convertFrom(bm, pub, bounds, bounds, phase);
    bm_delete(bag->backbits);
    bm_delete(pub);
    bag->backbits = bm;
    bag->mismatch = (bm->isa != mp12);
}


/*****************************************************************************
    MPRegisterScreen
    When windowserver running, we need to register our screen(s) to the
    ev driver for cursor pinning and control.
******************************************************************************/
void MPRegisterScreen(NXDevice *device)
{
    evioScreen es;
    es.bounds = device->bounds;
    es.device = (int)device;
    ioctl(mpFd, DKIOCREGISTER, &es);
}


/*****************************************************************************
	MP12SetBitmapExtent
	First time will set up memoryOffset array.
******************************************************************************/
void MP12SetBitmapExtent(unsigned int *first, unsigned int *last)
{
    struct mwf myMwf;
    int i, resetIt = 0;

    if (bitmapAddrInited) {
	if (first < lowestBitmapAddr) {
	    lowestBitmapAddr = first;
	    resetIt = 1;
	}
	if (last > highestBitmapAddr) {
	    highestBitmapAddr = last;
	    resetIt = 1;
	}
    } else {
	lowestBitmapAddr = first;
	highestBitmapAddr = last;
	resetIt = 1;
    }
    if (resetIt) {
	myMwf.min = (int) lowestBitmapAddr;
	myMwf.max = (int) highestBitmapAddr;
	ioctl(mpFd, DKIOCSMWF, &myMwf);
	if (!bitmapAddrInited) {
	    for (i=0; i<4; i++)
		memoryOffsets[i] = myMwf.min + i*myMwf.max;
	    bitmapAddrInited = 1;
	}
    }
}


/*****************************************************************************
	MPWindowSize
	Return storage used by this window on this device.
******************************************************************************/
int MPWindowSize(NXBag *bag)
{
    int l, r;
    if (!bag->backbits) return 0;
    return bm_sizeInfo(bag->backbits, &l, &r);
}




