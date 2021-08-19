/******************************************************************************

    routines.c
    Mainline-level interface to NeXT mouse driver

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created by Leo Hourvitz 04Jan88 from ev_routines.c

    Modified:

    09Feb88 Leo   Added TestMouseRect
    09Mar88 Leo   Implemented SetWinCursor
    13Sep88 Leo   Added ioctl to set event port
    13Jan88 Ted   Made to reflect my nextdev changes for multiple screens.
    25Jan89 Jack  Changed LCompositeBitsFrom to LCopyBitsFrom
    24Aug89 Ted   ANSI prototyping, update to 1.0 mousekeyboard
    05Sep89 Ted   Changed InitMouseEvents to send screen info to ev driver.
    05Sep89 Ted   #import BINTREE for previous.
    14Nov89 Terry CustomOps conversion to InitMouseEvents
    06Dec89 Ted   Integratathon!
    08Jan90 Terry Changed InitMouseEvents
    24Jan90 Terry Fixed argument order for LCopyBitsFrom in SetWinCursor
    23Feb90 Ted   Removed SetWinCursor and EVIOSS ioctl call.
    08Mar90 Ted   Added PSLimitCheck to occur in SetCursor if ebm is NULL.
    22May90 Trey  Added wait cursor support (all WC* routines).
    20Jun90 Ted   Fixed bug 6464: page size bug with shmem in InitMouseEvents.
    20Sep90 Ted   Added mods for bug 8959: startwaitcursortimer.

******************************************************************************/

#import <mach.h>
#import <sys/types.h>
#import <sys/mman.h>
#import <sys/file.h>
#import <stdlib.h>
#import PACKAGE_SPECS
#import CUSTOMOPS
#import EVENT
#import BINTREE
#import "vars.h"
#import "bitmap.h"

/* macros for WC tracing - turn on by compiling with DO_WCTRACE */
#ifdef DO_WCTRACE

#define WCTRACE(msg)			os_fprintf(os_stderr, (msg))
#define WCTRACE1(msg, a1)		os_fprintf(os_stderr, (msg), (a1))
#define WCTRACE2(msg, a1, a2)		os_fprintf(os_stderr, (msg), (a1), (a2))
#define WCDUMP_SHMEM()			os_fprintf(os_stderr, "snt %d, cons %d, en %d, TOut %d", evp->AALastEventSent, evp->AALastEventConsumed, evp->waitCursorEnabled, evp->ctxtTimedOut)

#else

#define WCTRACE(msg)
#define WCTRACE1(msg, a1)
#define WCTRACE2(msg, a1, a2)
#define WCDUMP_SHMEM()

#endif

int evFd = -1;
NXCursorInfo nxCursorInfo;

#define CHECKOPEN() if (!evp) return

/* called when we dispatch an event.  It updates the lastEventSent of the context if that time is greater than the previous value.  If the context is active, it also updates the value in the shmem. */

extern void WCSendEvent(WCParams *wcParams, int isActive, int sentTime)
{
    if (sentTime == -1) {     /* -1 means to use the current time */
	wcParams->lastFakeEvent = sentTime = eventGlobals->VertRetraceClock;
	WCTRACE2("Sending LESent for %#x to *%d", wcParams, sentTime);
    } else
	WCTRACE2("Sending LESent for %#x to %d", wcParams, sentTime);
    if (wcParams && sentTime > wcParams->lastEventSent) {
	WCTRACE(", setting ctxt");
	wcParams->lastEventSent = sentTime;
	if (isActive && evp) {
	    evp->AALastEventSent = sentTime;
	    WCTRACE(", setting SHMEM: ");
	    WCDUMP_SHMEM();
	}
    }
    WCTRACE("\n");
}

/* called when we receive a message from the client ack'ing an event.
 * If we did a startwctimer-style event with a more recent timestamp,
 * consider this the last event consumed.
 */
extern void WCReceiveEvent(WCParams *wcParams, int isActive, int consumedTime)
{
    if (wcParams->lastFakeEvent > consumedTime) {
	consumedTime = wcParams->lastFakeEvent;
	WCTRACE2("Receving LEConsumed from %#x of *%d", wcParams,
	    consumedTime);
    } else
	WCTRACE2("Receving LEConsumed from %#x of %d", wcParams, consumedTime);
    if (wcParams && consumedTime > wcParams->lastEventConsumed &&
			consumedTime <= wcParams->lastEventSent) {
	WCTRACE(", setting ctxt");
	wcParams->lastEventConsumed = consumedTime;
	if (wcParams->lastEventSent == wcParams->lastEventConsumed)
	    wcParams->ctxtTimedOut = FALSE;
	if (isActive && evp) {
	    evp->waitCursorSema++;
	    evp->AALastEventConsumed = consumedTime;
	    if (wcParams->lastEventSent == wcParams->lastEventConsumed)
		evp->ctxtTimedOut = FALSE;
	    WCTRACE(", setting SHMEM: ");
	    WCDUMP_SHMEM();
	    evp->waitCursorSema--;
	}
    }
    WCTRACE("\n");
}

/* called when we switch active contexts.  If oldWCParams is non-NULL, we copy
 * values from the shmem back into the context data.  This only needs to be
 * done for things the EV driver changes, since items changed by the Window-
 * Server will stay in sync.  If newWCParams is NULL, the shmem calues are
 * cleared.  Otherwise, the values contained in newWCParams are copied to the
 * shmem.  At this time, the timedOut field is updated, since the context may
 * have timedOut while not active, and thus not noticed by the EV driver.
 */
extern void WCSetData(WCParams *old, WCParams *new)
{
    CHECKOPEN();
    evp->waitCursorSema++;
    WCTRACE2("Switching from %#x to %#x", old, new);
    if (old)
	old->ctxtTimedOut = evp->ctxtTimedOut;
    if (new) {
	evp->AALastEventSent = new->lastEventSent;
	evp->AALastEventConsumed = new->lastEventConsumed;
	evp->waitCursorEnabled = new->waitCursorEnabled;
	evp->ctxtTimedOut = new->ctxtTimedOut ||
		((new->lastEventSent != new->lastEventConsumed) &&
		 (eventGlobals->VertRetraceClock - new->lastEventSent >
							evp->waitThreshold));
    } else {
	evp->AALastEventSent = 0;
	evp->AALastEventConsumed = 0;
	evp->waitCursorEnabled = FALSE;
	evp->ctxtTimedOut = FALSE;
    }
    WCTRACE(", setting SHMEM: ");
    WCDUMP_SHMEM();
    WCTRACE("\n");
    evp->waitCursorSema--;
}

/* Called to set a context's enabled bit */
extern void WCSetEnabled(WCParams *wcParams, int isActive, int flag)
{
    WCTRACE2("Setting enabled bit for %#x to %d", wcParams, flag);
    if (wcParams) {
	WCTRACE(", setting ctxt");
	wcParams->waitCursorEnabled = flag;
	if (isActive && evp) {
	    evp->waitCursorEnabled = flag;
	    WCTRACE(", setting SHMEM: ");
	    WCDUMP_SHMEM();
	}
    }
    WCTRACE("\n");
}

/* Sets the bit that controls whether WC is enabled globally */
void SetGlobalWCEnabled(char enabled)
{
    CHECKOPEN();
    evp->globalWaitCursorEnabled = enabled;
}

/* Returns the bit that controls whether WC is enabled globally */
char CurrentGlobalWCEnabled(void)
{
    return evp ? evp->globalWaitCursorEnabled : TRUE;
}

/* Routines to implement the various ioctls */

void StartCursor()
{
    Point p = {100,100};
    CHECKOPEN();
    ioctl(MouseFd(), EVIOST, &p);
}

void HideCursor()
{
    CHECKOPEN();
    evp->cursorSema++;
    SysHideCursor();
    evp->cursorSema--;
}

void ShowCursor()
{
    CHECKOPEN();
    evp->cursorSema++;
    SysShowCursor();
    evp->cursorSema--;
}

void ObscureCursor()
{
    CHECKOPEN();
    evp->cursorSema++;
    SysObscureCursor();
    evp->cursorSema--;
}

void RevealCursor()
{
    CHECKOPEN();
    evp->cursorSema++;
    SysRevealCursor();
    evp->cursorSema--;
}

void ShieldCursor(Bounds *aRect)
{
    CHECKOPEN();
    evp->cursorSema++;
    evp->shieldFlag = 1;
    evp->shieldRect = *aRect;
    evp->shielded = 0;
    CheckShield();
    SysSyncCursor(1);
    evp->cursorSema--;
}

void UnShieldCursor()
{
    CHECKOPEN();
    SysSyncCursor(1);
    evp->cursorSema++;
    if (evp->shielded)
	SysShowCursor();
    evp->shielded = 0;
    evp->shieldFlag = 0;
    evp->cursorSema--;
}

void SetMouseMoved(int eMask)
{
    CHECKOPEN();
    evp->movedMask = eMask & MOVEDEVENTMASK;
}

int TestMouseRect()
{
    CHECKOPEN();
    return(evp->mouseRectValid);
}

void ClearMouseRect()
{
    CHECKOPEN();
    evp->mouseRectValid = 0;
}

void SetMouseRect(Bounds *aRect)
{
    CHECKOPEN();
    evp->cursorSema++;
    evp->mouseRect = *aRect;
    evp->mouseRectValid = 1;
    evp->cursorSema--;
}

int InitMouseEvents(port_t eventPort)
{
    int size, pagemask;
    EvOffsets *evo;

    /* If we can't open ev0, there is another PS running, so give up */
    if ((evFd = open("/dev/ev0", O_RDWR, 0)) < 0) {
	perror("Error: ev driver already open. Exiting PS.");
	exit(-1);
    }
    pagemask = getpagesize() - 1;
    size = sizeof(EvOffsets) + sizeof(EvVars) + sizeof(EventGlobals);
    size = (size + pagemask) & ~pagemask;
    evo = (EvOffsets *) valloc((size_t) size);
    if (mmap((char *)evo, size, PROT_READ|PROT_WRITE, MAP_SHARED, evFd, 0)<0) {
	perror("Error: evmmap failure. Exiting PS.");
	exit(-1);
    }
    evp = (EvVars *)(((char *)evo) + evo->evVarsOffset);
    eventGlobals = (EventGlobals *)(((char *)evo) + evo->evGlobalsOffset);
    if (ioctl(evFd, EVIOSEP, &eventPort) == -1) {
	perror("Error: Can't set event port (EVIOSEP). Exiting PS.");
	exit(-1);
    }
    nxCursorInfo.cursorRect = (Bounds *)&evp->cursorRect;
    nxCursorInfo.saveRect = (Bounds *)&evp->saveRect;
    nxCursorInfo.saveData = (unsigned int *)&evp->saveData[0];
    nxCursorInfo.cursorData32 = (unsigned int *)&evp->nd_cursorData[0];
    nxCursorInfo.cursorData2W = (unsigned int *)&evp->cursorData2W[0];
    nxCursorInfo.cursorData2B = (unsigned int *)&evp->cursorData2B[0];
    nxCursorInfo.cursorAlpha2 = (unsigned int *)&evp->cursorAlpha2[0];
    nxCursorInfo.cursorData8 = (unsigned int *)&evp->cursorData8[0];
    nxCursorInfo.cursorData16 = (unsigned int *)&evp->cursorData16[0];
    return 0;
}

int MouseFd()
{
    if (evFd == -1)
	PSInvlAccess();
    return(evFd);
}

void TermMouseEvents()
{
    close(evFd);
    evFd = -1;
}

void SetWinCursor(Layer *layer, int cursorPtX, int cursorPtY, int hotSpotX,
    int hotSpotY)
{
    int waitup;
    LocalBitmap *lbm;
    Bounds cursorBounds;
    NXDevice *device;
    void (*proc)();

    CHECKOPEN();
    /* Calculate cursor bounds and get the cursor in standard form */
    cursorBounds.minx = cursorPtX;
    cursorBounds.miny = cursorPtY;
    cursorBounds.maxx = cursorPtX + CURSORWIDTH;
    cursorBounds.maxy = cursorPtY + CURSORHEIGHT;
    lbm = LCopyBitsFrom(layer, cursorBounds, true /* we want alpha */);
    if (!lbm) PSLimitCheck();

    nxCursorInfo.set.i = 0;
    /* Critical section: hide the cursor before changing it! */
    evp->cursorSema++;
    if (waitup = evp->waitCursorUp) {
	evp->oldCursorSpot.x = hotSpotX;
	evp->oldCursorSpot.y = hotSpotY;
    } else {
	SysHideCursor();
    
	/* Only the cursor hotspot and cursorRect change */
	evp->cursorSpot.x = hotSpotX;
	evp->cursorSpot.y = hotSpotY;
	evp->cursorRect.minx = eventGlobals->cursorLoc.x - evp->cursorSpot.x;
	evp->cursorRect.miny = eventGlobals->cursorLoc.y - evp->cursorSpot.y;
	evp->cursorRect.maxx = evp->cursorRect.minx + CURSORWIDTH;
	evp->cursorRect.maxy = evp->cursorRect.miny + CURSORHEIGHT;
    }
    /* SetCursor for every device */
    for (device = deviceList; device; device = device->next) {
	if (proc = device->driver->procs->SetCursor) {
	    (*proc)(device, &nxCursorInfo, lbm);
	}
    }
    if (!waitup) {
	evp->oldCursorLoc = eventGlobals->cursorLoc;
	SysShowCursor();
    }
    SysSyncCursor(1);
    evp->cursorSema--;
    bm_delete(lbm);
}










