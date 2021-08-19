/*****************************************************************************

    windowops.c
    
    This file contains routines for managing the layers of the Nextwin window
    system within the PostScript implementation.
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 21Apr87 Leo
    
    Modified:
    
    01Dec87 Leo  Deleted old modified log
    01Dec87 Leo  Called InitGraphics in CreateWindow
    08Dec87 Leo  Changed clickFront and frontFirst to default off,
		 preparatory to deleting them
    29Jan88 Leo  Check eventGlobals before derefing it
    09Feb88 Leo  Force param for RecalcMouseRect
    17Feb88 Jack Remove STAGE condition from dumpwindow
    29Feb88 Leo  Add setowner, currentowner
    11Apr88 Leo  New version where gs->WindowDevice
    05May88 Jack Make sure windowBase ain't NULL, etc.
    22May88 Leo  Have TermWindowsBy only Term Windows that exist
    08Jul88 Leo  Removed SysEventMask, added PSInvalidID and WindowList
    14Sep88 Jack V1003 conversion
    14Sep88 Leo  Added Garbage collector procs
    05Mar89 Jack Add check/fixup/warning code for circular nRect list
    09Mar89 Leo  Bug fix in above in PSMoveWindow
    09Mar89 Jack Add PSWindowDevice
    31Mar89 Dave Added parm to LNewAt() call in Window()
    15May89 Leo  Did conversion of all easy stuff to customops
    24May89 Jack Change FDMID to new LOG2 convention
    12Jun89 Leo  Add WINDOWLIMIT checking to PSWindow, PSMoveWindow,
		 PSPlaceWindow
    18Nov89 Ted  ANSI C prototyping.
    16Oct89 Terry CustomOps conversion
    04Dec89 Ted  Integratathon!
    06Dec89 Ted  Removed InvlIdNumber, changed other files to call PSInvalidID
    13Dec89 Ted  Sorted routines.
    21Dec89 Ted  Removed "nextgs.h" import, defines moved to windowdevice.h
    30Jan90 Terry Made color and monochrome code coexist
    08Feb90 Dave  Changed orderwindow semantics to handle window tiers,
		  added setwindowlevel and currentwindowlevel operators
    21Feb90 Dave  Added windowlist and countwindowlist operators for Appkit
    		  improvements.
    01Mar90 Terry Added REDRAW_CHANGED possibility to RedrawWindow
    08Mar90 Ted   Added remapY to map y coords. between user and device space
    26Mar90 Terry Check exists flag in *screenlist and *windowlist operators
    28Mar90 Ted   Added countframebuffers and framebuffer operators.
    29Mar90 Ted   Modified framebuffer op to return color and monochrome info
    		  and not consume vm for the string object.
    05May90 Ted   Added error check to PSSetOwner (bug 5486)
    05Jun90 Dave  Removed redundant call to ID2Wd() during orderwindow.
    29May90 Terry Reduced code size from 7024 to 6904 bytes
    24Jun90 Ted   Ripped out fixedwindow operator for new API
    24Jun90 Ted   Added setwindowdepthlimit, currentwindowdepthlimit,
    		  currentwindowdepth, setdefaultdepthlimit and
		  currentdefaultdepthlimit operators for new API.
******************************************************************************/

#import PACKAGE_SPECS
#import BINTREE
#import POSTSCRIPT
#import WINDOWDEVICE
#import MOUSEKEYBOARD			/* for cursorLoc */
#import "ipcscheduler.h"
#import "wbcontext.h"

public PWindowDevice windowBase;	/* Linked list of WindowDevices */
public char *nrStorage;			/* Blind ptr to NRects pool */
private char *wdPool;			/* Blind ptr to WindowDevice pool */
private short lastWID;			/* Last window ID created */
private boolean anyRedraws; /* any update rects since last FlushRedrawRects? */ 

/* Forward References */
extern int PopNaturalMax();		/* windowgraphics.c */
extern PWindowDevice mouseWindow;	/* from mousedriver.h */
extern DevProcs *nullProcs;
extern procedure ClearWdNRect();
extern procedure TermWindow();
extern procedure TermNRects();
private procedure SetWdNRect();
public procedure SetWindow();
public procedure SetWindowDevice();
public procedure TermWindowDevice();
public short UniqueWindowId();
public Layer *ID2Layer(int wID);

/* customops.c customizations */
extern PSObject PSLNullObj();
extern PSObject PSLIntObj();

/* Tiered window ordering */
#define TOO_LOW		-1
#define JUST_RIGHT	0
#define TOO_HIGH	1
#define NOT_ON_SCREEN	2
#define NXBASEWINLEVEL	((integer)0x80000000)

static const short mapIntToExtDepths[] = {
    NX_DEFAULTDEPTH,
    NX_TWOBITGRAY_DEPTH,
    NX_EIGHTBITGRAY_DEPTH,			
    NX_TWELVEBITRGB_DEPTH,
    NX_TWENTYFOURBITRGB_DEPTH
};

/*****************************************************************************
    Window creates a window device, but DOES NOT make it appear in any
    graphics state.  That is done via the windowdevice operator.
******************************************************************************/
private int window(Bounds winBounds, int type)
{
    PWindowDevice wd;
    extern DevProcs *wdProcs;	/* windowdevice.c */

    /* Create window, insert in list */
    wd = (PWindowDevice)os_newelement(wdPool);
    wd->next = windowBase;
    windowBase = wd;
    wd->id = UniqueWindowId();

    /* Initialize device portion of record to a windowdevice */
    wd->fd.procs = wdProcs;
    wd->fd.priv = NULL;
    wd->fd.ref = 1;		/* Can't call AddDevRef cuz it calls wakeup */
    wd->fd.maskID = 0;		/* Should be 0 for new device package */
    
    /* Initialize high-level-PostScript-related structures */
    wd->eventMask = 0;
    wd->psEventProcs = false;
    wd->eventProcs = (PPSObject) NULL;
    wd->dict = (PPSObject) NULL;
    wd->nRects = NULL;
    wd->level = 0;
    wd->owner = (int)((PSContextToID(currentPSContext)).stamp);
    wd->redrawSet = wd->changedSet = wd->newDevParams = false;
    wd->exists = true;
    wd->layer = LNewAt(type, winBounds, (int *)wd, 0, NX_TWOBITGRAY,
	currentSchedulerContext->defaultDepthLimit);
    return(wd->id);
}

/*****************************************************************************
    checkOrderingValdity
    Checks the validity of an ordering operation with respect to tiers.
    This routine should be passed a non-zero relWin.  If the ordering operation
    is legal, it returns JUST_RIGHT.  If the operation would place the window
    too high or too low in the window list, it returns TOO_HIGH or TOO_LOW.
******************************************************************************/

private int checkOrderingValdity(int op, int relWin, WindowDevice *wd,
				 SubList sl)
{
    Layer *relWinLayer;			/* layer of relWin */
    WindowDevice *lowerWd, *upperWd;	/* layers bounding destination slot */
    int i, slCount;

#if (STAGE != DEVELOP)
    if (relWin == 0)
	CantHappen();
#endif (STAGE != DEVELOP)
    relWinLayer = ID2Layer(relWin);
    slCount = sl.len;
    for ( ; sl.len > 0; sl.ptr++, sl.len--)
	if ((Layer *)(*sl.ptr) == relWinLayer)
	    break;

    if (sl.len == 0)
	return NOT_ON_SCREEN;
	
    if (op == ABOVE) {
	lowerWd = Layer2Wd( relWinLayer );
	if (sl.len < slCount)
	    --sl.ptr, upperWd = Layer2Wd( *sl.ptr );
	else
	    upperWd = NULL;
    } else {
	upperWd = Layer2Wd( relWinLayer );
	if (sl.len > 1)
	    ++sl.ptr, lowerWd = Layer2Wd( *sl.ptr );
	else
	    lowerWd = NULL;
    }
    if (lowerWd && lowerWd->level > wd->level)
	return TOO_HIGH;
    else if (upperWd && upperWd->level < wd->level)
	return TOO_LOW;
   else
	return JUST_RIGHT;
}

/*****************************************************************************
    ClearWdNRect removes the NRect of given id from the given window's NRect
    list, if it exists.  If reCalc is true, it recalculates the mouse rect
    afterwards.
******************************************************************************/
private procedure ClearWdNRect(PWindowDevice win, int tNum, boolean reCalc,
    boolean raiseError)
{
    register NRect *nr, **pnr, *nrStart;

    for (pnr = &win->nRects, nrStart = nr = win->nRects; nr != NULL; ) {
	if (nr->next == nrStart) {
	  nr->next = NULL;
	  os_fprintf(os_stderr, "ClearWdNRect repaired circular nRects!\n");
	}
	if (nr->id == tNum) {
	    *pnr = nr->next;
	    os_freeelement(nrStorage, nr);
	    if (reCalc&&(win == mouseWindow)&&eventGlobals)
		RecalcMouseRect(eventGlobals->cursorLoc.x,
		    eventGlobals->cursorLoc.y, true, false);
	    return;
	}
	pnr = &nr->next;
	nr = nr->next;
    }
    if (raiseError)
	PSInvalidID();
}

public int CurWindowID()
{
    PDevice device = PSGetDevice(NULL);
    if (device->procs != wdProcs)
	PSInvalidID();
    return(((PWindowDevice)device)->id);
}

/*****************************************************************************
    FindPieceBounds returns the address of the smallest BitPiece enclosing the
    given location.  The location is assumed to be within the given window.
******************************************************************************/
public Bounds *FindPieceBounds(PWindowDevice win, int x, int y)
{
    Point pt;

    if (win->layer) {
	pt.x = x; pt.y = y;
	return(LFindPieceBounds(win->layer, pt));
    } else
	return(&wsBounds);
}

/*****************************************************************************
    FlushRedrawRects checks each window's redraw rect, calling PostRedraw if
    necessary.
******************************************************************************/
public procedure FlushRedrawRects()
{
    PWindowDevice win;

    if (anyRedraws)
	for (win = windowBase; win != NULL; win = win->next) {
	    if (win->redrawSet) {
		PostRedraw(win, win->redrawBounds.minx,
		    win->redrawBounds.miny,
		    win->redrawBounds.maxx - win->redrawBounds.minx,
		    win->redrawBounds.maxy - win->redrawBounds.miny);
		win->redrawSet = false;
	    }
	    if (win->changedSet) {
		PostChanged(win, win->changedBounds.minx,
		    win->changedBounds.miny,
		    win->changedBounds.maxx - win->changedBounds.minx,
		    win->changedBounds.maxy - win->changedBounds.miny);
		win->changedSet = false;
	    }
	}
    anyRedraws = false;
}

public PWindowDevice GetFrontWindowDevice()
{
    Layer *layer = GetFrontWindow();
    
    if (!layer)
	PSInvalidID();
    return(Layer2Wd(layer));
}

private Card32 GetMouseMovedMask()
{
    Card32 mask;
    PWindowDevice win;
    
    for (win = windowBase, mask = 0; win != NULL; win = win->next)
	mask |= win->eventMask;
    return(mask);
}

public PWindowDevice GetNextWindowDevice(PWindowDevice win)
{
    return(Layer2Wd(GetNextWindow(Wd2Layer(win))));
}

/*****************************************************************************
    highestWindowInTier
    Sets highestWid to the highest window in the screenlist with the given
    level.  If no on-screen windows have this level, it sets highestWid to the
    highest on-screen window with a lower level.  If no on-screen windows have
    an equal or lower level, it sets highestWid to 0.  The returned value is
    the highest level value less than or equal to `level'.
*****************************************************************************/
private int highestWindowInTier(int level, SubList sl, int *highestWid)
{
    int i;
    WindowDevice *pswin;

    for( ; sl.len > 0; sl.ptr++, sl.len--) {
	if (pswin = Layer2Wd(*sl.ptr)) {
	    if (pswin->level <= level)
		break;
	}
    }
    *highestWid = sl.len ? pswin->id : 0;
    return sl.len ? pswin->level : NXBASEWINLEVEL;
}

/*****************************************************************************
    ID2Layer returns the Layer pointer corresponding to the given window 
    ID, or calls PSInvalidID.
******************************************************************************/
public Layer *ID2Layer(int wID)
{
    PWindowDevice win;
    
    for (win = windowBase; win != NULL; win = win->next)
	if (win->id == wID)
	    return(win->layer);
    PSInvalidID();
}

/*****************************************************************************
    ID2PrevWd returns the WindowDevice pointer to the window before the given 
    window, or NULL if the given WindowDevice is the first in the list.
******************************************************************************/
private PWindowDevice ID2PrevWd(int wID)
{
    PWindowDevice win, prev = NULL;
    
    for (win = windowBase; win != NULL; win = win->next)
	if (win->id == wID)
	    return(prev);
	else
	    prev = win;
    PSInvalidID();
}

/*****************************************************************************
    ID2Wd returns the WindowDevice pointer corresponding to the given window 
    ID, or calls PSInvalidID.
******************************************************************************/
public PWindowDevice ID2Wd(int wID)
{
    PWindowDevice win;
    
    for (win = windowBase; win != NULL; win = win->next)
	if (win->id == wID)
	    return(win);
    PSInvalidID();
}

/*****************************************************************************
    lowestWindowInTier
    Sets lowestWid to the lowest window in the screenlist with the given level.
    If no windows have this level, it sets lowestWid to the lowest window with
    a higher level.  If no windows have an equal or higher level, it sets
    highestWid to 0.  The returned value is the highest level value greater
    than or equal to `level'.
*****************************************************************************/
private int lowestWindowInTier(int level, SubList sl, int *lowestWid)
{
    int i;
    WindowDevice *pswin, *pswin1 = NULL;

    for( ; sl.len > 0; sl.ptr++, sl.len--) {
	if (pswin = Layer2Wd(*sl.ptr)) {
	    if (pswin->level < level)
		break;
	}
	pswin1 = pswin;
    }
    *lowestWid = (pswin1 ? pswin1->id : 0);
    return pswin1 ? pswin->level : NXBASEWINLEVEL;
}

private int PopIntCoord()
{
    int val;

    if (PSGetOperandType() == dpsIntObj)
	val = PSPopInteger();
    else {
	float r;
	
	PSPopPReal(&r);
	val = (int)r;
    }
    /* Make sure it will fit in a short if necessary */
    if ((val < (int)0xffff8000) || (val > (int)0x00007fff))
	PSRangeCheck();
    return(val);
}

private procedure PSClearNRect()
{
    PWindowDevice win;
    int tNum;

    win = (PWindowDevice)(PSGetDevice(PSPopGState()));
    if (win->fd.procs != wdProcs)
	PSInvalidID();
    tNum = PSPopInteger();
    ClearWdNRect(win, tNum, true, true);
}

/*****************************************************************************
	countframebuffers <count>
	Returns number of framebuffers currently active in the WindowServer.
	ERRORS: stackoverflow
******************************************************************************/
private procedure PSCountFrameBuffers()
{
    PSPushInteger(deviceCount);
}

/*****************************************************************************
    <pscontext id> countscreenlist <window count>
    PSCountScreenList counts the number of windows owned by the given
    pscontext. If the given pscontext is the base pscontext, all windows are
    counted.
******************************************************************************/
private procedure PSCountScreenList()
{
    SubList sl;
    int windowCount, esid;
    PWindowDevice pswin;
    
    esid = PSPopInteger();
    sl = WLBelowButNotBelow(ABOVE, 0, BELOW, 0);
    if (esid == 0)
	windowCount = sl.len;
    else
	for (windowCount=0; sl.len>0; sl.ptr++, sl.len--)
	    if (pswin = Layer2Wd(*sl.ptr)) /* sic */
		if (pswin->exists && pswin->owner == esid)
		    windowCount++;
    PSPushInteger(windowCount);
}

/*****************************************************************************
	<pscontext id> countwindowlist <window count>
	counts the number of windows owned by the
	given pscontext.  If the given pscontext is
	the base pscontext, all windows are counted.
	ERRORS: stackunderflow, typecheck
******************************************************************************/
private procedure PSCountWindowList()
{
    int			windowCount;
    integer		esid;
    WindowDevice	*pswin;
    
    esid = PSPopInteger();
    for (windowCount=0, pswin = windowBase; pswin; pswin = pswin->next)
	if (pswin->exists && (pswin->owner == esid || esid == 0))
	    windowCount++;
    PSPushInteger(windowCount);
}

/*****************************************************************************
	currentdefaultdepthlimit <depth>
	Returns the context's default depth limit.
	ERRORS: stackoverflow
******************************************************************************/
private procedure PSCurrentDefaultDepthLimit()
{
    PSPushInteger( (int) mapIntToExtDepths[
	currentSchedulerContext->defaultDepthLimit] );
}

/*****************************************************************************
	<window> currentDeviceInfo <minbpp> <maxbpp> <colorbool>
	ERRORS: stackoverflow
******************************************************************************/
private procedure PSCurrentDeviceInfo()
{
    DeviceStatus ds;
    
    ds = LGetDeviceStatus(ID2Layer(PSPopInteger()));
    PSPushInteger(ds.minbitsperpixel);
    PSPushInteger(ds.maxbitsperpixel);
    PSPushBoolean(ds.color);
}

/*****************************************************************************
	<window> currenteventprocedures <eprocs>
******************************************************************************/
private procedure PSCurrentEventProcedures()
{
    PWindowDevice win;

    win = ID2Wd(PSPopInteger());
    if (win->psEventProcs)
	PSPushObject(win->eventProcs);
    else {
	PSObject no = PSLNullObj();
	PSPushObject(&no);
    }
}

/*****************************************************************************
	<window> currenteventprocedures <context>
******************************************************************************/
private procedure PSCurrentOwner()
{
    PSPushInteger(ID2Wd(PSPopInteger())->owner);
}

/*****************************************************************************
	currentwindow <window>
******************************************************************************/
private procedure PSCurrentWindow()
{
    PSPushInteger(CurWindowID());
}

/*****************************************************************************
	<window> currentwindowdepth <depth>
	Return the window's current logical depth.
	ERRORS: stackunderflow, typecheck, invalidid
******************************************************************************/
private procedure PSCurrentWindowDepth()
{
    PWindowDevice win;
    
    win = ID2Wd(PSPopInteger());
    if (!win->exists) PSInvalidID();
    PSPushInteger(mapIntToExtDepths[LCurrentDepth(win->layer)]);
}

/*****************************************************************************
	<window> currentwindowdepthlimit <depth>
	Return the window's depth limit.
	ERRORS: stackunderflow, typecheck, invalidid
******************************************************************************/
private procedure PSCurrentWindowDepthLimit()
{
    PWindowDevice win;
    
    win = ID2Wd(PSPopInteger());
    if (!win->exists) PSInvalidID();
    PSPushInteger((int)mapIntToExtDepths[LDepthLimit(win->layer)]);
}

/*****************************************************************************
	<window> currentwindowdict <dict>
******************************************************************************/
private procedure PSCurrentWindowDict()
{
    PWindowDevice win;
    
    win = ID2Wd(PSPopInteger());
    if (win->dict != NULL) {
	PSPushObject(win->dict);
    } else {
	PSObject no = PSLNullObj();
	PSPushObject(&no);
    }
}

/*****************************************************************************
	<window> currentwindowlevel <level>
	Query the level of a window.
	ERRORS: stackunderflow, typecheck, invalidid
******************************************************************************/
private procedure PSCurrentWindowLevel()
{
    WindowDevice *wd;

    wd = ID2Wd(PSPopInteger());
    if (!wd->exists) PSInvalidID();
    PSPushInteger(wd->level);
}

/*****************************************************************************
	<level> <window> dumpwindow
	Output status of window given dump level (only zero supported for
	release version).
******************************************************************************/
private procedure PSDumpWindow()
{
    int dumpLevel;
    Layer *layer;
    
    layer = ID2Layer(PSPopInteger());
    dumpLevel = PSPopInteger();
#if (STAGE != DEVELOP)
    dumpLevel = 0; /* EXPORT versions can only do level 0 */
#endif (STAGE != DEVELOP)
    LPrintOn(layer, dumpLevel);
}

private procedure PSDumpWindows()
{
    int dumpLevel, pscid;
    PWindowDevice window;
    
    pscid = PSPopInteger();
    dumpLevel = PSPopInteger();
#if (STAGE != DEVELOP)
    dumpLevel = 0; /* EXPORT versions can only do level 0 */
#endif (STAGE != DEVELOP)
    for (window = windowBase; window; window = window->next)
	if ((pscid == 0) || (window->owner == pscid))
	    LPrintOn(Wd2Layer(window), dumpLevel);
}

/*****************************************************************************
	<x> <y> <op> <window> findwindow <x> <y> <window> <bool>
******************************************************************************/
private procedure PSFindWindow()
{
    short x, y;
    Layer *layer;
    int op, wid;
    
    wid = PSPopInteger();
    op = PSPopInteger();
    y = PopIntCoord();
    x = PopIntCoord();
    if ((op != 1) && (op != -1))
    	PSRangeCheck();
    if (wid)
	layer = ID2Layer(wid);
    else
	layer = NULL;
    if (!(layer = (Layer *)LFind(x, remapY-y, op, layer))) {
	PSPushInteger(0);
	PSPushInteger(0);
	PSPushInteger(0);
	PSPushBoolean(false);
    } else {
	Cd pt;
    
	pt.x = x; pt.y = y;
	ScreenToBase(Layer2Wd(layer), &pt);
	PSPushPCd(&pt);
	PSPushInteger(Layer2Wd(layer)->id);
	PSPushBoolean(true);
    }
}

/*****************************************************************************
	<index> <string> framebuffer <name> <slot> <unit> <id> <x> <y> <width>
				     <height> <maxvisdepth>
	ERRORS: stackunderflow, typecheck, rangecheck, limitcheck
******************************************************************************/
private procedure PSFrameBuffer()
{
    int n;
    NXDevice *device;
    NXDriver *driver;
    PSObject str;
    
    PSPopTempObject(dpsStrObj, &str);
    n = PopNaturalMax(deviceCount-1);
    for (device=deviceList; n>0; n--, device=device->next);
    driver = device->driver;
    if (str.length < strlen(driver->name))
    	PSLimitCheck();
    strcpy(str.val.strval, driver->name);
    PSPushObject(&str);
    PSPushInteger(device->slot);
    PSPushInteger(device->unit);
    PSPushInteger(device->romid);
    PSPushInteger(device->bounds.minx);
    PSPushInteger(remapY-device->bounds.maxy);
    PSPushInteger(device->bounds.maxx-device->bounds.minx);
    PSPushInteger(device->bounds.maxy-device->bounds.miny);
    PSPushInteger(mapIntToExtDepths[device->visDepthLimit]);
}

/*****************************************************************************
	getfrontwindow <window>
******************************************************************************/
private procedure PSGetFrontWindow()
{
    PSPushInteger(GetFrontWindowDevice()->id);
}

/*****************************************************************************
	<window> getwindoweventmask <mask>
******************************************************************************/
private procedure PSGetWindowEventMask()
{
    PSPushInteger(ID2Wd(PSPopInteger())->eventMask);
}

/*****************************************************************************
	<x> <y> <window> movewindow
******************************************************************************/
private procedure PSMoveWindow()
{
    Point p;
    int newX, newY;
    Bounds tlBounds;
    NRect *nr, *nrStart;
    PWindowDevice pswin;

    pswin = ID2Wd(PSPopInteger());
    newY = remapY - PopIntCoord();
    newX = PopIntCoord();

    /* A little special case:  don't let him move window 14,
       the gray desktop window created at startup.  It turns out 
       the cursor window mechanism is dependent on this window. */
    if (pswin->id == (BASEPSWINDOWID+1))
	PSInvlAccess();

    /* Check whether the final bounds will be within window */
    GetTLWinBounds(pswin->layer, &tlBounds);
    if ((newX < -WINDOWLIMIT) || (newX > WINDOWLIMIT) ||
    (newY < -WINDOWLIMIT) || (newY > WINDOWLIMIT) ||
    (newX + (tlBounds.maxx-tlBounds.minx) > WINDOWLIMIT) ||
    (newY + (tlBounds.maxy-tlBounds.miny) > WINDOWLIMIT))
	PSRangeCheck();

    p.x = newX;
    p.y = newY;
    LMoveTo(pswin->layer, p);
    if (nr = pswin->nRects) {	/* Make newX and newY relative */
	newX -= tlBounds.minx;
	newY -= tlBounds.maxy;
	/* Move all nRects by relative amount */
	for (nrStart = nr; nr != NULL; nr = nr->next) {
	    if (nr->next == nrStart) {
		nr->next = NULL;
		os_fprintf(os_stderr,
		    "PSMoveWindow repaired circular nRects!\n");
	    }
	    OFFSETBOUNDS(nr->nRect, newX, newY);
	}
    }
    if (eventGlobals)
	RecalcMouseRect(eventGlobals->cursorLoc.x,
	    eventGlobals->cursorLoc.y, false, false);
}

/*****************************************************************************
	<op> <otherwin> <window> orderwindow
******************************************************************************/
private procedure PSOrderWindow()
{
    SubList sl;
    int otherNum, op;
    WindowDevice *wd;
    Layer *win, *otherWin;

    sl = WLBelowButNotBelow(ABOVE, 0, BELOW, 0);
    wd = ID2Wd(PSPopInteger());
    win = wd->layer;
    otherNum = PSPopInteger();
    op = PSPopInteger();

    /* Ensure that operation is legal and window exists.
     * A little special case:  can't change the tier of window 14,
     * the gray desktop window created at startup.  It turns out 
     * the cursor window mechanism is dependent on this window.
     */
    if ((op < -1) || (op > 1)) PSRangeCheck();
    if (!wd->exists) PSInvalidID();
    if ((wd->id == (BASEPSWINDOWID+1)) && (op == OUT)) PSInvlAccess();

    /* Validate args with respect to tiers */
    if (op != OUT) {
	if (otherNum != 0) {
	    /* If invalid, just order to top or bottom of window's tier */
	    switch(checkOrderingValdity(op, otherNum, wd, sl)) {
		case TOO_HIGH:
		    op = ABOVE;
		    otherNum = 0;
		    break;
		case TOO_LOW:
		    op = BELOW;
		    otherNum = 0;
		    break;
		case JUST_RIGHT:
		case NOT_ON_SCREEN:
		    /* Window is on screen, go ahead and LOrder it */
		    goto lorder;
		    break;
	    }
	}
	if (op == ABOVE) {
	    highestWindowInTier(wd->level, sl, &otherNum);
	    if (!otherNum)
		op = BELOW;	/* there are no windows we can be above */
	} else {
	    lowestWindowInTier(wd->level, sl, &otherNum);
	    if (!otherNum)
		op = ABOVE;	/* there are no windows we can be below */
	}
    }

lorder:
    /* Now we have an op and an otherNum that obeys the tiers */
    if (op && otherNum)
	otherWin = ID2Layer(otherNum);
    else
	otherWin = NULL;
    LOrder(win, op, otherWin);
    if (eventGlobals)
	RecalcMouseRect(eventGlobals->cursorLoc.x, eventGlobals->cursorLoc.y,
	    false, false);
}

/*****************************************************************************
	<x> <y> <w> <h> <window> placewindow
******************************************************************************/
private procedure PSPlaceWindow()
{
    Bounds newBounds;
    PWindowDevice pswin;
    extern WBContext *WBCList;
    int newX, newY, newWidth, newHeight;

    pswin = ID2Wd(PSPopInteger());
    newHeight = PopIntCoord();
    newWidth = PopIntCoord();
    newY = remapY - PopIntCoord() - newHeight;
    newX = PopIntCoord();
    if ((newWidth <= 0) || (newHeight <= 0) ||
    (newWidth > WIDTHSANITY) || (newWidth > HEIGHTSANITY))
	PSRangeCheck();
    if ((newX < -WINDOWLIMIT) || (newX > WINDOWLIMIT) ||
    (newY < -WINDOWLIMIT) || (newY > WINDOWLIMIT) ||
    ((newX+newWidth) > WINDOWLIMIT) || ((newY+newHeight) > WINDOWLIMIT))
	PSRangeCheck();
    /* A little special case:  don't let him reorder window 14,
       the gray desktop window created at startup.  It turns out 
       the cursor window mechanism is dependent on this window. */
    if (pswin->id == (BASEPSWINDOWID+1))
	PSInvlAccess();
    /* OK, now assign into newBounds structure.  Had to do above range-checking
       with full ints to be sure we could assign coordinates into
       shorts in the structures below.  */
    newBounds.miny = newY;
    newBounds.minx = newX;
    newBounds.maxx = newBounds.minx + newWidth;
    newBounds.maxy = newBounds.miny + newHeight;
    pswin->layer = LPlaceAt(pswin->layer, newBounds);
    /* If there are any WindowBitmap contexts, this may be a windowbitmap,
     * so we have to inform it of the size change.  Otherwise, we want to
     * avoid calling WBChangeBitmap, since it is probably scatterloaded onto
     * a very cold non-resident page of VM, since windowbitmap is rarely used.
     */
    if (WBCList)
        WBChangeBitmap(pswin->id);
    if (eventGlobals)
	RecalcMouseRect(eventGlobals->cursorLoc.x,
	    eventGlobals->cursorLoc.y, false, false);
}

/*****************************************************************************
    <array> <pscontext id> screenlist <subarray>
    Returns the Z-ordered list of windows belonging to the given pscontext.
    If the given pscontext id is 0, all windows in the window list are
    returned. If the list will not fit in the given array, truncates.
******************************************************************************/
private procedure PSScreenList()
{
    SubList sl;
    PSObject ao, io;
    PWindowDevice pswin;
    int arrayIndex=0, esid;
    
    esid = PSPopInteger();
    PSPopTempObject(dpsArrayObj, &ao);
    io = PSLIntObj(0);
    sl = WLBelowButNotBelow(ABOVE, 0, BELOW, 0);
    for ( ; sl.len > 0; sl.ptr++, sl.len--)
	if (pswin = Layer2Wd(*sl.ptr))
	    if (pswin->exists && (pswin->owner == esid || esid == 0))
		if (arrayIndex < ao.length) {
		    io.val.ival = pswin->id;
		    PSPutArray(&ao, arrayIndex++, &io);
		}
    ao.length = arrayIndex;
    PSPushObject(&ao);
}

/*****************************************************************************
    <bool> <window> setautofill
******************************************************************************/
private procedure PSSetAutofill()
{
    int wID;
    boolean newState;

    wID = PSPopInteger();
    newState = PSPopBoolean();
    LSetAutofill(ID2Layer(wID), newState);
}

/*****************************************************************************
	<depth> setdefaultdepthlimit
	Set the context's default depth limit for new windows.
	ERRORS: stackunderflow, typecheck, rangecheck
******************************************************************************/
private procedure PSSetDefaultDepthLimit()
{
    int depth = PSPopInteger();
    switch(depth) {
	case NX_DEFAULTDEPTH:
	    depth = initialDepthLimit;
	    break;
	case NX_TWOBITGRAY_DEPTH:
	    depth = NX_TWOBITGRAY;
	    break;
	case NX_EIGHTBITGRAY_DEPTH:
	    depth = NX_EIGHTBITGRAY;
	    break;
	case NX_TWELVEBITRGB_DEPTH:
	    depth = NX_TWELVEBITRGB;
	    break;
	case NX_TWENTYFOURBITRGB_DEPTH:
	    depth = NX_TWENTYFOURBITRGB;
	    break;
	default:
	    PSRangeCheck();
	    break;
    }
    currentSchedulerContext->defaultDepthLimit = depth;
}

/*****************************************************************************
    <array> <window id> seteventprocedures
    PSSetEventProcedures gives an array containing either the null object or a
    PostScript procedure for each event type, to be installed as the vector of 
    event procedures for the given window.
******************************************************************************/
private procedure PSSetEventProcedures()
{
    PPSObject ao;
    PWindowDevice win;

    win = ID2Wd(PSPopInteger());

    /* Pop an array off the stack as a managed object */
    ao = PSPopManagedObject(dpsArrayObj);

    /* If it is not shared, release the managed object and abort */
    if (!PSSharedObject(ao)) {
	PSReleaseManagedObject(ao);
	PSInvlAccess();
    }

    /* Release the previous eventProcs object if it exists */
    if (win->psEventProcs)
	PSReleaseManagedObject(win->eventProcs);
    
    /* Set the window's eventProcs to the new object */
    win->eventProcs = ao;
    win->psEventProcs = true;
}

/*****************************************************************************
    PSSetNRect adds an NRect of the given origin and size to the given window's 
    NRect list, replacing any current NRect by that id.
******************************************************************************/
private procedure PSSetNRect()
{
    NRect *newNR;
    Bounds winBounds;
    PWindowDevice win;
    PPSGState thisgs;
    Cd userPt, userSize;
    boolean initState, rightButton, leftButton;
    int tNum, userData;

    thisgs = PSPopGState();
    tNum = PSPopInteger();
    userData = PSPopInteger();
    initState = PSPopBoolean();
    rightButton = PSPopBoolean();
    leftButton = PSPopBoolean();
    win = (PWindowDevice)PSGetDevice(thisgs);
    if (win->fd.procs != wdProcs)
	PSInvalidID();

    /* Remove nRect with that id if it exists */
    ClearWdNRect(win, tNum, false, false);
    /* Insert new nRect */
    newNR = (NRect *)os_newelement(nrStorage);
    newNR->next = win->nRects;
    win->nRects = newNR;
    /* Pop size+pt off stack to compute global-coordinate, upper-left bounds */
    PopBounds(PSGetMatrix(thisgs), &newNR->nRect);
    GetTLWinBounds(Wd2Layer(win), &winBounds);
    OFFSETBOUNDS(newNR->nRect, winBounds.minx, winBounds.miny);
    newNR->id = tNum;
    newNR->userData = userData;
    newNR->state = initState;
    newNR->buttons = (leftButton<<2) | rightButton;
    if (eventGlobals)
	if (win == mouseWindow)
	    RecalcMouseRect(eventGlobals->cursorLoc.x,
	    eventGlobals->cursorLoc.y, false, false);
	else if (initState) /* It's not inside him */
	    PostNotification(eventGlobals->cursorLoc.x,
		eventGlobals->cursorLoc.y, newNR, win->id);
				/* TED: added win->id */
}

private procedure PSSetOwner()
{
    int wID, pscID;
    
    wID = PSPopInteger();
    if ((pscID = PSPopInteger()) != 0)
	if (!IDToPSContext(pscID))
	    PSInvalidID();
    ID2Wd(wID)->owner = pscID;
}

private procedure PSSetSendExposed()
{
    int wID;

    wID = PSPopInteger();
    LSetSendRepaint(ID2Layer(wID), PSPopBoolean());
}


/*****************************************************************************
	<depth> <window> setwindowdepthlimit
	Limit a window's depth.  If depth is zero, the window's depth limit
	will be set to the context's current defaultdepthlimit.  Also, if the
	new depth limit is less than the current depth of the window, the
	window is dithered down to this depth.
	Errors: stackunderflow, typecheck, rangecheck, invalidid
******************************************************************************/
private procedure PSSetWindowDepthLimit()
{
    int depth;
    PWindowDevice win;
    
    win = ID2Wd(PSPopInteger());
    if (!win->exists) PSInvalidID();
    depth = PSPopInteger();
    switch(depth) {
	case NX_DEFAULTDEPTH:
	    depth = currentSchedulerContext->defaultDepthLimit;
	    break;
	case NX_TWOBITGRAY_DEPTH:
	    depth = NX_TWOBITGRAY;
	    break;
	case NX_EIGHTBITGRAY_DEPTH:
	    depth = NX_EIGHTBITGRAY;
	    break;
	case NX_TWELVEBITRGB_DEPTH:
	    depth = NX_TWELVEBITRGB;
	    break;
	case NX_TWENTYFOURBITRGB_DEPTH:
	    depth = NX_TWENTYFOURBITRGB;
	    break;
	default:
	    PSRangeCheck();
	    break;
    }
    LSetDepthLimit(win->layer, depth);
}

private procedure PSSetWindowDict()
{
    PPSObject dobj;
    PWindowDevice win;
    
    win = ID2Wd(PSPopInteger());
    dobj = PSPopManagedObject(dpsAnyObj);
    if (!PSSharedObject(dobj)) {
	PSReleaseManagedObject(dobj);
	PSInvlAccess();
    }
    if (PSGetObjectType(dobj) != dpsDictObj && 
    PSGetObjectType(dobj) != dpsNullObj) {
	PSReleaseManagedObject(dobj);
	PSTypeCheck();
    }
    if(win->dict != NULL) PSReleaseManagedObject(win->dict);
    win->dict = dobj;
}

private procedure PSSetWindowEventMask()
{
    int id;
    Card32 newMask;
    
    id = PSPopInteger();
    newMask = PSPopInteger();
    
    ID2Wd(id)->eventMask = newMask;
    SetMouseMoved(GetMouseMovedMask());
}

/*****************************************************************************
	<level> <window> setwindowlevel -
	Set the level of a window.  Causes an implicit orderwindow.
	ERRORS: invalidaccess, invalidid, rangecheck, stackunderflow, typecheck
******************************************************************************/
private procedure PSSetWindowLevel()
{
    int level, otherWinId, highestTier;
    Layer *win;
    WindowDevice *wd;
    SubList sl = WLBelowButNotBelow( ABOVE, 0, BELOW, 0 );

    wd = ID2Wd(PSPopInteger());
    win = wd->layer;
    level = PSPopInteger();
    if (level <= NXBASEWINLEVEL)
    {
	/* only window 14 can be NXBASEWINLEVEL! */
	if (wd->id == (BASEPSWINDOWID+1))
	{
	    /* HACK: this is only called at initialisation, so it's
	     * OK to just set the level and return.  Maybe it would
	     * be better to set the level during building of window 14?
	     */
	    wd->level = level;
	    return;
	}
	else
	    PSRangeCheck();
    }
    if (!wd->exists) PSInvalidID();

    /* Can't change the level of window 14, except during initialization */
    if (wd->id == (BASEPSWINDOWID+1))
	PSInvlAccess();
    if (level == wd->level)
	return;

    /* There is always an otherWin, since window 14 has been made by now... */
    highestWindowInTier( level, sl, &otherWinId );

    /* Check to see if the window is in the screen list */
    for(; sl.len > 0; sl.ptr++, sl.len--)
    {
	if (wd->layer == *sl.ptr)
	    break;  /* it's in the screen list, proceed */
    }

    /* If the window is not in the screen list, we're finished */
    wd->level = level;
    if (sl.len == 0) return;

    /* Order it highest in new tier */
    LOrder(win, ABOVE, ID2Layer(otherWinId));
    if (eventGlobals)
	RecalcMouseRect(eventGlobals->cursorLoc.x,eventGlobals->cursorLoc.y,
		false,false);
}

private procedure PSSetWindowType()
{
    PWindowDevice wd;

    wd = ID2Wd(PSPopInteger());
    if (wd->id == BASEPSWINDOWID+1)
	PSInvlAccess();
    if (!LSetType(wd->layer, PopNaturalMax(BUFFERED)))
        PSLimitCheck();
}

private procedure PSTermWindow()
{
    PWindowDevice wd;
    
    wd = ID2Wd(PSPopInteger());
    /* A little special case:  don't let him terminate window 14,
       the gray desktop window created at startup.  It turns out 
       the cursor window mechanism is dependent on this window. */
    if (wd->id == (BASEPSWINDOWID+1))
	PSInvlAccess();
    TermWindow(wd);
}

private procedure PSWindow()
{
    int x, y, width, height, type;
    Bounds winBounds;     /* expressed in top-left coordinates */

    type = PopNaturalMax(BUFFERED);
    height = PopIntCoord();
    width = PopIntCoord();
    if (height<0 || width<0 || width>WIDTHSANITY || height>HEIGHTSANITY)
	PSRangeCheck();
    y = remapY - PopIntCoord(); /* bottom */
    x = PopIntCoord(); /* left */
    if (x < -WINDOWLIMIT || x > WINDOWLIMIT ||
        y < -WINDOWLIMIT || y > WINDOWLIMIT ||
        (x+width) > WINDOWLIMIT || (y+height) > WINDOWLIMIT)
	PSRangeCheck();

    /* Copy all geometry into winBounds structure.  The reason
     * we pop them into full ints first is so that we can do the
     * above range-checking in full coordinates; that is what allows
     * us to safely assign them to shorts below.
     */
    winBounds.maxy = y;
    winBounds.minx = x;
    winBounds.maxx = winBounds.minx + width;
    winBounds.miny = winBounds.maxy - height;
    PSPushInteger(window(winBounds, type));
}

private procedure PSWindowDevice()
{
    SetWindow(PSPopInteger(), true);
}

private procedure PSWindowDeviceRound()
{
    SetWindow(PSPopInteger(), false);
}

/*****************************************************************************
	<array> <pscontext id> windowlist <subarray>
	Returns the Z-ordered list of windows belonging to the given pscontext.
	If the given pscontext id is 0, all windows in the window list are
	returned.  If the list will not fit in the given array, truncates.
******************************************************************************/
private procedure PSWindowList()
{
    PSObject ao, io;
    int arrayIndex = 0,	esid;
    PWindowDevice pswin;
    
    esid = PSPopInteger();
    PSPopTempObject(dpsArrayObj, &ao);
    io = PSLIntObj(0);
    for (pswin = windowBase; pswin; pswin = pswin->next)
	if (pswin->exists && (pswin->owner == esid || esid == 0))
	    if (arrayIndex < ao.length) {
		io.val.ival = pswin->id;
		PSPutArray(&ao, arrayIndex++, &io);
	    }
    ao.length = arrayIndex;
    PSPushObject(&ao);
}

public procedure RedrawWindowDevice(PWindowDevice win, Bounds *redrawHere)
{
    if (win->redrawSet)
	boundBounds(&win->redrawBounds, redrawHere, &win->redrawBounds);
    else
	win->redrawBounds = *redrawHere;
    win->redrawSet = true;
    anyRedraws = true;
}

public procedure ChangedWindowDevice(PWindowDevice win, Bounds *changedHere)
{
    if (win->changedSet)
	boundBounds(&win->changedBounds, changedHere, &win->changedBounds);
    else
	win->changedBounds = *changedHere;
    win->changedSet = true;
    anyRedraws = true;
}

procedure SetWindow(int windowID, boolean realScale)
{
    PWindowDevice win = ID2Wd(windowID); /* Assert only possible failure pt */
    /* now clobber that value in current state */
    REALSCALE(PSGetGStateExt(NULL)) = realScale;
    SetWindowDevice(win); /* so that WdDefaultMtx does the right thing */
}

procedure SetWindowDevice(PWindowDevice win)
{
    if (!win->exists)
	PSInvalidID();
    PSSetDevice(win,false);
}

private procedure TermNRects(PWindowDevice win)
{
    NRect *nr, *nnr, *nrStart;
    
    for (nrStart = nr = win->nRects; nr != NULL; nr = nnr) {
	if (nr->next == nrStart) {
	    nr->next = NULL;
	    os_fprintf(os_stderr, "TermNRects repaired circular nRects!\n");
	}
	nnr = nr->next;
	os_freeelement(nrStorage, nr);
    }
}

public procedure TermWindow(PWindowDevice wd)
{
    if (!wd->exists)
	PSInvalidID();
    wd->exists = false;
    TermNRects(wd);
    /* Make sure it's out of the screen list */
    LOrder(wd->layer, OUT, NULL);
    if (mouseWindow == wd)
	mouseWindow = NULL;
    /* Reset mouse rect for tracking */
    if (eventGlobals)
	RecalcMouseRect(eventGlobals->cursorLoc.x, eventGlobals->cursorLoc.y,
	    (mouseWindow != NULL), false);
    /* Do the equivalent of the old RemDevRef call */
    if (--wd->fd.ref == 0)
	TermWindowDevice(wd);
}    

/*****************************************************************************
    precondition: TermWindow has already been called at some previous point on
    this window
******************************************************************************/

public procedure TermWindowDevice(PWindowDevice deadWin)
{
    PWindowDevice prevWd;
    
    /* Reset global event mask */
    SetMouseMoved(GetMouseMovedMask());
    LFree(deadWin->layer);
    /* release managed objects stored in device */
    if (deadWin->psEventProcs) PSReleaseManagedObject(deadWin->eventProcs);
    if (deadWin->dict) PSReleaseManagedObject(deadWin->dict);

    deadWin->fd.procs = nullProcs;
    if (prevWd = ID2PrevWd(deadWin->id))
	prevWd->next = deadWin->next;
    else
	windowBase = deadWin->next;
    os_freeelement(wdPool, deadWin);
}

/*****************************************************************************
    TermWindowsBy kills all windows owned by the given PSContext.
******************************************************************************/

public procedure TermWindowsBy(int id)
{
    PWindowDevice wd;

    /* Look through the window list */
    for (wd=windowBase; wd != NULL; )
	/* If this guy was created by the given PSContext,
	   get rid of him and keep looking */
	if (wd->owner == id) {
	    PWindowDevice dwd;
	    
	    dwd = wd;
	    wd = wd->next;
	    if (dwd->exists)
		TermWindow(dwd);
	}
	else
	    wd = wd->next;
}

/*****************************************************************************
    UniqueWindowId returns a unique id for things in the given list.
    It will never use 0 or -1 as an id. It starts out by incrementing lastWID
    until it gets to the maximum positive short.  After that, it uses the
    low-order bits of lastWID as a starting point and checks the list for
    uniqueness.
******************************************************************************/

#define WRAPPEDBIT ((short)0x8000) /* sign bit */

public short UniqueWindowId()
{
    if (lastWID & WRAPPEDBIT) {
	PWindowDevice wd;
    
	/* Look through the window list */
	while (true) {		/* Until unique id found */
	    short unWrappedId;
    
	    lastWID = (lastWID+1) | WRAPPEDBIT;
	    unWrappedId = lastWID & (~WRAPPEDBIT);
	    for (wd=windowBase; wd!=NULL; wd=wd->next)
		if (wd->id == unWrappedId)
		    break;
	    if (!wd)
		return(unWrappedId);
	}
    } else {
	while ((lastWID == 0) || (lastWID == -1))
	    lastWID++;
	return(lastWID++);
    }
}

#if (STAGE == DEVELOP) /* At EXPORT stage, this is a macro */
public Layer *Wd2Layer(PWindowDevice wd)
{
    return(wd->layer);
}
#endif (STAGE == DEVELOP)

private readonly RgOpTable cmdWindowOps = {
    "cleartrackingrect", PSClearNRect,
    "countframebuffers", PSCountFrameBuffers,
    "countscreenlist", PSCountScreenList,
    "countwindowlist", PSCountWindowList,
    "currentdefaultdepthlimit", PSCurrentDefaultDepthLimit,
    "currentdeviceinfo", PSCurrentDeviceInfo,
    "currenteventmask", PSGetWindowEventMask,
    "currenteventprocedures", PSCurrentEventProcedures,
    "currentowner", PSCurrentOwner,
    "currentwindow", PSCurrentWindow,
    "currentwindowdepth", PSCurrentWindowDepth,
    "currentwindowdepthlimit", PSCurrentWindowDepthLimit,
    "currentwindowdict", PSCurrentWindowDict,
    "currentwindowlevel", PSCurrentWindowLevel,
    "dumpwindow", PSDumpWindow,
    "dumpwindows", PSDumpWindows,
    "findwindow", PSFindWindow,
    "framebuffer", PSFrameBuffer,
    "frontwindow", PSGetFrontWindow,
    "movewindow", PSMoveWindow,
    "orderwindow", PSOrderWindow,
    "placewindow", PSPlaceWindow,
    "screenlist", PSScreenList,
    "setautofill", PSSetAutofill,
    "setdefaultdepthlimit", PSSetDefaultDepthLimit,
    "seteventmask", PSSetWindowEventMask,
    "seteventprocedures", PSSetEventProcedures,
    "setowner", PSSetOwner,
    "setsendexposed", PSSetSendExposed,
    "settrackingrect", PSSetNRect,
    "setwindowdict", PSSetWindowDict,
    "setwindowdepthlimit", PSSetWindowDepthLimit,
    "setwindowlevel", PSSetWindowLevel,
    "setwindowtype", PSSetWindowType,
    "termwindow", PSTermWindow,
    "window", PSWindow,
    "windowdevice", PSWindowDevice,
    "windowdeviceround", PSWindowDeviceRound,
    "windowlist", PSWindowList,
    NIL};

public procedure IniWindowOps(int reason)
{
    switch (reason)
    {
    case 0:
	lastWID = BASEPSWINDOWID + 1;
	wdPool = (char *)os_newpool(sizeof(WindowDevice), 5, 0);
	nrStorage = (char *)os_newpool(sizeof(NRect), 10, 0);
	windowBase = NULL;
	break;
    case 1:
	PSRgstOps(cmdWindowOps);
	break;
    }
}





