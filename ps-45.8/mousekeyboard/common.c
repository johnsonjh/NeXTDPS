/******************************************************************************

    common.c

    Routines for the NeXT mouse driver that are
    shared between PostScript and the ev driver.
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 29Dec87 Leo from nextmouse_common.c
    
    Modified:
    
    21Sep88 Leo  Made LLEventPost an ioctl
    13Jan89 Ted  Made reflect multiple screen code in nextdev.
    17Jan89 Ted  Redesigned screen structs (can't use ptrs in shared ram!).
    19Aug89 Ted  Removed volatiles in DisplayCursor and RemoveCursor
    24Aug89 Ted  ANSI prototyping, updated to 1.0 mousekeyboard
    18Dec89 Ted  Fixed 32bit cursor code; used to hit color maps.

******************************************************************************/

#import PACKAGE_SPECS
#import EVENT
#import BINTREE
#import <sys/types.h>
#import <sys/ioctl.h>
#import "vars.h"

extern NXCursorInfo nxCursorInfo;

void DisplayCursor()
{
    void (*proc)();
    NXDevice *device;

    /* Calculate new cursor rectangle */
    evp->cursorRect.maxx = (evp->cursorRect.minx =
	eventGlobals->cursorLoc.x - evp->cursorSpot.x) + CURSORWIDTH;
    evp->cursorRect.maxy = (evp->cursorRect.miny =
	eventGlobals->cursorLoc.y - evp->cursorSpot.y) + CURSORHEIGHT;
    /* If screen's display vector present, then display the cursor! */
    device = (NXDevice *) evp->screen[evp->crsrScreen].device;
    if (proc = device->driver->procs->DisplayCursor)
	(*proc)(device, &nxCursorInfo);
    evp->prevScreen = evp->crsrScreen;
}

void RemoveCursor()
{
    void (*proc)();
    NXDevice *device;

    /* If screen's remove vector present, then remove the cursor! */
    device = (NXDevice *) evp->screen[evp->prevScreen].device;
    if (proc = device->driver->procs->RemoveCursor)
	(*proc)(device, &nxCursorInfo);
}

void SysHideCursor()
{
    if (!evp->cursorShow++)
	RemoveCursor();
}

void SysShowCursor()
{
    if (evp->cursorShow)
	if (!--evp->cursorShow)
	    if (evp->waitCursorUp)
		ioctl(MouseFd(), EVIODC);
	    else
		DisplayCursor();
}

void SysObscureCursor()
{
    if (!evp->cursorObscured) {
	evp->cursorObscured = 1;
	SysHideCursor();
    }
}

void SysRevealCursor()
{
    if (evp->cursorObscured) {
	evp->cursorObscured = 0;
	SysShowCursor();
    }
}

void SysSyncCursor(int sync)
{
    void (*proc)();
    NXDevice *device;

    for(device=deviceList; device; device=device->next)
	if (proc = device->driver->procs->SyncCursor)
	    (*proc)(device, sync);
}

void CheckShield()
{
    int touches;
    Bounds temp;

    /* Calculate cursor rectangle */
    temp.maxx = (temp.minx =
	eventGlobals->cursorLoc.x - evp->cursorSpot.x) + CURSORWIDTH;
    temp.maxy = (temp.miny =
	eventGlobals->cursorLoc.y - evp->cursorSpot.y) + CURSORHEIGHT;
    /* evp->shield rect is volatile */
    touches = TOUCHBOUNDS(temp, evp->shieldRect);
    if (touches != evp->shielded) {
	if (touches)
	    SysHideCursor();
	else
	    SysShowCursor();
	evp->shielded = touches;
    }
}

void LLEventPost(int what, Point location, NXEventData *myData)
{
    struct evioLLEvent lle;
    
    lle.type = what;
    lle.location = location;
    lle.data = *myData;
    ioctl(MouseFd(), EVIOLLPE, &lle);
}


