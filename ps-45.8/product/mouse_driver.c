/*****************************************************************************

    mouse_driver.c

    PostScript driver for interface to generic mouse device.
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created by Leo Hourvitz 22Dec87 from mousereader.c
    
    Modified:
    
    27Jan88 Leo   New event_interface.h
    31Jan88 Leo   Better checking of eventGlobals
    09Feb88 Leo   Added force parameter to RecalcMouseRect
    07Mar88 Leo   Reset CurBlock in TermMouseEvents
    07Mar88 Leo   Acquire BeginFlush lock on event dispatching
    09Mar88 Leo   Move non-mouse-specific stuff to event.c
    09Mar88 Leo   Bug fix in SetPSWinCursor
    12Apr88 Leo   CurrentMouseScaling implemented
    20May88 Leo   Tweak to PSCurrentMouseScaling
    09Jun88 Leo   Floating-point ClickTime
    22Aug88 Leo   Make base window id be 0
    27Aug88 Leo   Make setcursor use corner near origin
    27Aug88 Leo   Add dummy wait cursor operations
    05Mar89 Jack  Add check/fixup/warning code for circular nRect list
    16Oct89 Terry CustomOps conversion
    04Dec89 Ted   Integratathon!
    05Dec89 Ted   ANSI C Prototyping, reformatting.
    01Mar90 Dave  Added parameter to PostEvent for new postbycontext operator
    08Mar90 Ted   Added remapY to map y coords. between user and device space.
    28Mar90 Terry Checked that current device is window in PSSetCursor
    02May90 Ted   Added new operator: adjustcursor (leave adjustmouse in).
    28Mar90 Terry Removed null routines for PS*cursor

******************************************************************************/

#define TIMING 1

#import PACKAGE_SPECS
#import CUSTOMOPS
#import EXCEPT
#import BINTREE
#import WINDOWDEVICE
#import MOUSEKEYBOARD

/* Implementation Constants */

#define CURWINDOW NULL		/* Parameter to some window routines */
#define CURSORSIZE 16

public PWindowDevice mouseWindow; /* Window currently containing the mouse */
public procedure PostNotification();

/*****************************************************************************
    RecalcMouseRect is called to recalculate the largest rect which
    won't change either mouseWindow or the in/out state of any notification 
    rects.  It accepts the mouse location to recalculate with, and a hint 
    called winUnchanged.  If winUnchanged is true, the mouse is guaranteed
    to have not left this window.  If force is true, the rect is recalculated 
    and reset unconditionally; if false, it is recalculated and reset only if 
    it is still valid.
******************************************************************************/

RecalcMouseRect(int x, int y, int winUnchanged, int force)
{
    extern Bounds *FindPieceBounds();
    extern int TestMouseRect();
    PWindowDevice newWindow;	/* The new mouseWindow */
    register NRect *nr, *nrStart;
    Bounds newMouseRect;

    /* First return if it isn't absolutely necessary to do this */
    if ((!force) && (!TestMouseRect()))
	return;
    /* OK, shut things off */
    ClearMouseRect();
    /* Find out what window we're gonna be dealing with */
    if (winUnchanged)
	newWindow = mouseWindow;
    else /* Oh, no, it's expensive! */
	newWindow = Layer2Wd(LFind(x, y, ABOVE, 0));
    if (newWindow != mouseWindow) {
    /* Well, we're outside all the nRects in the old one */
	if (mouseWindow)
	    for(nrStart=nr=mouseWindow->nRects; nr != NULL; nr=nr->next) {
		if (nr->next == nrStart) {
		    nr->next = NULL;
		    os_fprintf(os_stderr,
		    "RecalcMouseRect(1) repaired circular nRects!\n");
		}
		if (nr->state == INSIDE)
		    PostNotification(x, y, nr, 0);
	    }
	mouseWindow = newWindow;
    }
    newMouseRect = *FindPieceBounds(mouseWindow, x, y);
    for(nrStart=nr=mouseWindow->nRects; nr != NULL; nr=nr->next) {
	if (nr->next == nrStart) {
	    nr->next = NULL;
	    os_fprintf(os_stderr,
	    "RecalcMouseRect(2) repaired circular nRects!\n");
	}
	if (y < nr->nRect.miny) /* Point above rect */
	    if (newMouseRect.maxy > nr->nRect.miny)
		newMouseRect.maxy = nr->nRect.miny;
	    else ;
	else
	if (y >= nr->nRect.maxy) /* Point below rect */
	    if (newMouseRect.miny < nr->nRect.maxy)
		newMouseRect.miny = nr->nRect.maxy;
	    else ;
	else
	if (x < nr->nRect.minx) /* Point left of rect */
	    if (newMouseRect.maxx > nr->nRect.minx)
		newMouseRect.maxx = nr->nRect.minx;
	    else ;
	else
	if (x >= nr->nRect.maxx) /* Point right of rect */
	    if (newMouseRect.minx < nr->nRect.maxx)
		newMouseRect.minx = nr->nRect.maxx;
	    else ;
	else /* Point inside rect */
	{
	    if (!sectBounds(&newMouseRect, &nr->nRect, &newMouseRect)) {
		os_fprintf(os_stderr,"RecalcMouseRect(3):In nrect:Yow!");
		CantHappen();
	    }
	    if (nr->state == OUTSIDE)
		PostNotification(x, y, nr, 0);
	    continue; /* Do next nRect */
	}
	/* All the outside cases end up here */
	if (nr->state == INSIDE)
	    PostNotification(x, y, nr, 0);
    } /* End of loop through nRects in new mouseWindow */
    /* Restart the driver */
    SetMouseRect(&newMouseRect);
}

/*****************************************************************************
	PostNotification is called to post a mouse-entered or mouse-exited
	event to some window.  It checks the event against the button
	mask in the NRect and calls PostEvent if appropriate. It then inverts
	the state of nr->state.
******************************************************************************/

public procedure PostNotification(int x, int y, NRect *nr, int explicitWin)
{
    /* Check button state against nr record */
    if (((!(nr->buttons & WHILELEFT)) || (eventGlobals->buttons & WHILELEFT))
    && ((!(nr->buttons & WHILERIGHT)) || (eventGlobals->buttons & WHILERIGHT)))
    {
	NXEvent	e;

	ClearEvent(&e);
	e.type = ((nr->state == OUTSIDE) ? NX_MOUSEENTERED : NX_MOUSEEXITED);
	/* PostEvent thinks in screen cordinates (RH) */
	e.location.x = x;
	e.location.y = remapY - y;
	e.flags = e.time = 0;
	e.data.tracking.eventNum = eventGlobals->eNum;
	e.data.tracking.trackingNum = nr->id;
	e.data.tracking.userData = nr->userData;
	if (explicitWin) {
	    e.window = explicitWin;
	    PostEvent(&e, NX_EXPLICIT);
	} else
	    PostEvent(&e, NX_MOUSEWINDOW, 0);
    }
    /* Do this unconditionally -- reflect reality */
    nr->state = !nr->state;
}

private procedure PSCurrentMouse()
{
    int mouseX, mouseY, windowNum;
    PWindowDevice win;

    CurrentMouse(&mouseX, &mouseY);
    mouseY = remapY - mouseY;
    windowNum = PSPopInteger();
    if (windowNum != 0) {
	win = ID2Wd(windowNum);
	GlobalToLocal(win, &mouseX, &mouseY);
    }
    PSPushInteger(mouseX);
    PSPushInteger(mouseY);
    DidInteract(win);
}

private procedure PSButton()
{
    PSPushBoolean(eventGlobals ? (eventGlobals->buttons & LB) : false);
    DidInteract(CURWINDOW);
}

private procedure PSRightButton()
{
    PSPushBoolean(eventGlobals ? (eventGlobals->buttons & RB) : false);
    DidInteract(CURWINDOW);
}

private procedure PSStillDown()
{
    PSPushBoolean(StillDown(PSPopInteger()));
    DidInteract(CURWINDOW);
}
  
private procedure PSRightStillDown()
{
    PSPushBoolean(RightStillDown(PSPopInteger()));
    DidInteract(CURWINDOW);
}
 
private procedure SetGSCursor(Cd userPt, Cd userSpot)
{
    Mtx *m;
    DevCd devPt, devSpot;
    Layer *layer;
    Bounds *layerBounds;

    /* Transform everything into device coordinates */
    m = PSGetMatrix(NULL);
    UserToDevice(m, userPt, &devPt);
    DUserToDevice(m, userSpot, &devSpot);
    /* Now account for flipped coordinate systems.  If 
       Spot < Pt, subtract size from Pt and make Spot
       be size - Spot (for each axis). */
    if (devSpot.x < 0) {
    	devPt.x -= CURSORSIZE;
	devSpot.x = CURSORSIZE - devSpot.x;
    }
    if (devSpot.y < 0) {
    	devPt.y -= CURSORSIZE;
	devSpot.y = CURSORSIZE - devSpot.y;
    }
    /* Get info about layer */
    layer = Wd2Layer(PSGetDevice(NULL));
    layerBounds = (Bounds *)LBoundsAt(layer);
    /* to check that all results are within acceptable ranges */
    if ((devPt.x < 0) || (devPt.y < 0)
    || ((devPt.x + CURSORSIZE) > (layerBounds->maxx-layerBounds->minx))
    || ((devPt.y + CURSORSIZE) > (layerBounds->maxy-layerBounds->miny))
    || (devSpot.x < 0)||(devSpot.y < 0)
    || (devSpot.x > CURSORSIZE) || (devSpot.y > CURSORSIZE))
	PSRangeCheck();
    /* Great! Off it goes! */
    SetWinCursor(layer, devPt.x, devPt.y, devSpot.x, devSpot.y);
}

private procedure PSSetCursor()
{
	Cd pt, spot;

	CheckWindow();
	PSPopPCd(&spot);
	PSPopPCd(&pt);
	SetGSCursor(pt, spot);
}

private procedure PSSetMouse()
{
	Cd user;
	Bounds b;
	DevCd delta;
	PDevice	device = PSGetDevice(NULL);
	
	CheckWindow();
	PSPopPCd(&user);
	UserToDevice(PSGetMatrix(NULL), user, &delta);
	GetTLWinBounds(Wd2Layer(device), &b);
	SetMouse(delta.x + b.minx, delta.y + b.miny);
}

/* Here is the corrected version of the old adjustmouse operator */
private procedure PSAdjustCursor()
{
	Cd user;
	int mx, my;
	DevCd delta;

	CheckWindow();
	PSPopPCd(&user);
	DUserToDevice(PSGetMatrix(NULL), user, &delta);
	CurrentMouse(&mx, &my);
	SetMouse(delta.x + mx, delta.y + my);
}

/* PSAdjustMouse is intentially left "broken" so developer code won't break */
private procedure PSAdjustMouse()
{
	Cd userPos;
	DevCd devPos;
	Bounds wBounds;
	Layer *layer;
	int mouseX, mouseY;
	PDevice	device = PSGetDevice(NULL);

	CheckWindow();
	CurrentMouse(&mouseX, &mouseY);
	GlobalToLocal((PWindowDevice)device, &mouseX, &mouseY);
	PSPopPCd(&userPos);
	UserToDevice(PSGetMatrix(NULL), userPos, &devPos);
	if (layer = Wd2Layer(device))
	    GetTLWinBounds(layer, &wBounds);
	else {
	    wBounds.minx=wBounds.miny=0;
	    wBounds.maxx=1120;
	    wBounds.maxy=832;
	}
	SetMouse(devPos.x+wBounds.minx+mouseX, devPos.y+wBounds.miny+mouseY);
}
  
private procedure PSSetWaitEnabled()
{
    (void)PSPopBoolean();
}

private procedure PSResetWait() {}

private procedure PSSetWaitCursor()
{
    Cd userPt, userSpot;
    
    PSPopPCd(&userPt);
    PSPopPCd(&userSpot);
}

private procedure PSSetWaitTime()
{
    float time;

    PSPopPReal(&time);
}


private readonly RgOpTable cmdMouse = {
    "adjustmouse",	PSAdjustMouse,
    "adjustcursor",	PSAdjustCursor,
    "buttondown",	PSButton,
    "currentmouse",	PSCurrentMouse,
    "hidecursor",	HideCursor,
    "obscurecursor",	ObscureCursor,
    "resetwait",	PSResetWait,
    "revealcursor",	RevealCursor,
    "rightbuttondown",	PSRightButton,
    "rightstilldown",	PSRightStillDown,
    "setcursor",	PSSetCursor,
    "setmouse",		PSSetMouse,
    "setwaitenabled",	PSSetWaitEnabled,
    "setwaitcursor",	PSSetWaitCursor,
    "setwaittime",	PSSetWaitTime,
    "showcursor",	ShowCursor,
    "stilldown",	PSStillDown,
    NIL};

public procedure MouseInit(int reason)
{
    if (reason==1)
	PSRgstOps(cmdMouse);
}











