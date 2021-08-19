/*****************************************************************************

    event.c

    This file contains routines for posting,
    sending, and generally fooling around
    with events in the NeXT PostScript
    implementation.
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 09Oct86 Leo
    
    Modified:
    
    20Mar89 Leo  Deleted previous change log
    13Sep88 Leo  IPC based
    02Oct88 Leo  Abort event dispatching on error; avoid refStk
    14Jan89 Leo  Add RECYCLER to imports
    20Mar89 Leo  Added SetMouseRect call to PSInitEvents
    16Oct89 Terry CustomOps conversion
    04Dec89 Ted  Integratathon!
    06Dec89 Ted  ANSI C Prototyping, reformatting.
    08Jan90 Terry Changed PSInitEvents to use PSSetRealClockAddress()
    01Mar90 Dave Added posteventbycontext operator, added new how for PostEvent
    01Mar90 Terry Added PostChanged to post a screenChanged event
    08Mar90 Ted   Added remapY to map y coords. between user and device space.
    06May90 Trey  Added wait cursor code
    06May90 Ted   Filled in shmem assignments for wait cursor/ev driver
    22May90 Trey  Added currentwaitcursorenabled, setwaitcursorenabled
			setactiveapp, currentactiveapp
		  Added call to wait cursor hook in SendEvent
    23May90 Trey  Added startwaitcursortimer
    28May90 Trey  when how=NX_POSTEVENTBYCONTEXT, dont play with window
		   field of the event.
    20Sep90 Ted   Added Trey's setlasteventsenttime operator for journaling.
    24Sep90 Ted   Made mods for bug 8959: startwaitcursortimer.
    01Oct90 Jack  Added PSCurrentEventTime
******************************************************************************/

#define TIMING 1

#import PACKAGE_SPECS
#import CUSTOMOPS
#import EXCEPT
#import POSTSCRIPT
#import BINTREE
#import WINDOWDEVICE
#import EVENT
#import MOUSEKEYBOARD
#undef MONITOR
#import <sys/types.h>
#import <sys/file.h>
#import <strings.h>
#import <pwd.h>
#import <mach.h>
#import "ipcscheduler.h"
#import "ipcstream.h"
#import "timelog.h"

#define NX_POSTEVENTBYCONTEXT	1000

#if IEEEFLOAT
#if SWAPBITS
#define EASY_FORMAT 2
#else SWAPBITS
#define EASY_FORMAT 1
#endif SWAPBITS
#else IEEEFLOAT
#if SWAPBITS
#define EASY_FORMAT 4
#else SWAPBITS
#define EASY_FORMAT 3
#endif SWAPBITS
#endif IEEEFLOAT

extern PWindowDevice ID2PSWin(/* int id; */);
extern PWindowDevice GetFrontWindowDevice();
extern PWindowDevice GetNextWindowDevice();
extern PWindowDevice windowBase;	/* from window.c */
extern PWindowDevice mouseWindow;	/* from mousereader.c */

extern procedure ClearEvent();	/* forward */
extern void EventFlush();	/* forward */

public	PSContext MousePSContext; /* PSContext that processes mouse events */
public  PSContext ActiveApp;	/* context representing active app */
private	integer	lastLeft;	/* Window which got last left down */
private	integer	lastRight;	/* Window which got last right down */
private	port_t eventPort;	/* Port to which event notify msgs come */
private int eventErrorPending;
				/* Set when an error occurs in a event
				   procedure; cleared at every entry to 
				   PSGetEvents */
private boolean flushExposures; /* Exposure events are flushed if true */

/* The last two key events and their recipients */
struct {
    integer window;
    unsigned short keyCode;
} lastKey, prevKey;

/* The header for sending out events on the connection */
private readonly struct _eventHeader {
    u_char escapeCode;		/* Escape code fixing byte ordering */
    u_char numObjects;		/* Number of objects in top-level sequence */
    u_char lengthByteOne;	/* Length of everything including itself */
    u_char lengthByteTwo;	/* Length of everything including itself */
    u_char nothing;
    u_char tag;			/* 255 to indicate events */
    u_short nothing2;
} eventHeader =
#if SWAPBITS
    { 129, 1, 
      sizeof(struct _eventHeader)+sizeof(NXEvent), 0, 
      0, 255, 0 };
#else SWAPBITS
    { 128, 1, 
      0, sizeof(struct _eventHeader)+sizeof(NXEvent),
      0, 255, 0 };
#endif SWAPBITS

/* WriteEvent writes an event to a stream given a object format. */

private procedure Swap4(int *from, int *to)
{
    CopySwap4(((char *)from), ((char *)to)); /* from fp.h */
}

private procedure Swap2(char *from, char *to)
{
    *to++ = *(from+1);
    *to = *from;
}

private procedure WriteEvent(Stm stm, NXEvent *ep, int format)
{
    NXEvent e;
    struct _eventHeader myHeader;

    /* First, deal with ASCII */
    if (!format) {
	fprintf(stm,"%%%%{%d;%f;%f;%d;%d;%d;%d;%d;%d}%%%%\n",
	    ep->type,
	    *((float *)&ep->location.x),*((float *)&ep->location.y),
	    ep->time, ep->flags, ep->window,
	    ep->data.compound.subType,ep->data.compound.misc.L[0],
	    ep->data.compound.misc.L[1]);
	return;
    }
    /* Swap the body if necessary */
    if (format != EASY_FORMAT) {
    	myHeader = eventHeader;
    	myHeader.escapeCode = (129 - eventHeader.escapeCode) + 128;
	myHeader.lengthByteOne = eventHeader.lengthByteTwo;
	myHeader.lengthByteTwo = eventHeader.lengthByteOne;
	fwrite(&myHeader,1,sizeof(myHeader),stm);
    	Swap4(&ep->type, &e.type);
	Swap4(&ep->location.x, &e.location.x);
	Swap4(&ep->location.y, &e.location.y);
	Swap4((int *)&ep->time, (int *)&e.time);
	Swap4(&ep->flags, &e.flags);
	Swap4((int *)&ep->window, (int *)&e.window);
	Swap2((char *)&ep->data.compound.subType,
	    (char *)&e.data.compound.subType);
	if(ep->type == NX_KEYDOWN || ep->type == NX_KEYUP ||
	   ep->type == NX_FLAGSCHANGED) {
	  Swap2((char *)&ep->data.compound.misc.S[0],
	  	(char *)&e.data.compound.misc.S[0]);
	  Swap2((char *)&ep->data.compound.misc.S[1],
	  	(char *)&e.data.compound.misc.S[1]);
	  Swap2((char *)&ep->data.compound.misc.S[2],
	  	(char *)&e.data.compound.misc.S[2]);
	  Swap2((char *)&ep->data.compound.misc.S[3],
		(char *)&e.data.compound.misc.S[3]);
        }
	else {
	  Swap4((int *)&ep->data.compound.misc.L[0],
	  	(int *)&e.data.compound.misc.L[0]);
	  Swap4((int *)&ep->data.compound.misc.L[1],
	  	(int *)&e.data.compound.misc.L[1]);
	}
	ep = &e;
    } else /* it's the easy format */
	fwrite(&eventHeader, 1, sizeof(eventHeader), stm);
    /* And now the event itself */
    fwrite(ep, 1, sizeof(*ep), stm);
}

/*****************************************************************************
    SendEvent sends an event to the window it is marked for. It will set when,
    modifiers, and message.mouse.id if they are not already set.  It keeps
    track of the last window to receive left mouse down, right  mouse down,
    or key down events.  It will only actually send the event if the window
    includes the event type in its event mask. It will send the event to the
    connection itself if the window's psEventProc is false or forceTransmit
    is true; otherwise it will call the window's PostScript event proc.
    If sendByContext is true, it will expect ep->window to be a PostScript
    context id instead of a window id. Lastly, SendEvent will never block on
    output; if that would happen, it throws the event away instead.
******************************************************************************/

private boolean SendEvent(NXEvent *ep, boolean forceTransmit,
    boolean sendByContext, int contextId)
{
    PWindowDevice win;
    boolean sent;
    int eMask;

    sent = false;
    if (sendByContext) forceTransmit = true;
    eMask = EventCodeMask(ep->type);
    if (!sendByContext)
	switch(ep->type) {
	    case NX_LMOUSEDOWN:
		lastLeft = ep->window;
		break;
	    case NX_RMOUSEDOWN:
		lastRight = ep->window;
		break;
	    case NX_KEYDOWN:
		if (!ep->data.key.repeat)
		    if (ep->data.key.keyCode == lastKey.keyCode)
			lastKey.window = ep->window;
		    else {
			prevKey = lastKey;
			lastKey.keyCode = ep->data.key.keyCode;
			lastKey.window = ep->window;
		    }
	    default:
		break;
	}
    if ((!sendByContext) && ((((short)ep->window) == -1) || (ep->window == 0)))
	return(sent);
    if ((ep->flags == -1)||(ep->flags == 0))
	ep->flags = eventGlobals->eventFlags;
    if (ep->time == 0) ep->time = eventGlobals->VertRetraceClock;
    if (!sendByContext) win = ID2Wd(ep->window);
    if (sendByContext || (win->eventMask & eMask)) {
	int iX, iY;
	PSObject eProc;

	/* Change coordinates to local */
	iX = ep->location.x;
	iY = ep->location.y;
	if (!sendByContext) GlobalToLocal(win, &iX, &iY);
	if (forceTransmit || (!win->psEventProcs) ||
	((eProc = win->eventProcs->val.arrayval[ep->type]),
	PSGetObjectType(&eProc) == dpsNullObj)) {
	    /* Transmit the event to client program */
	    /* The great server/client kludge! The client expects location to
	       be in single floating-point format! */
	    float fX,fY;
	    PSContext psc;
	    boolean oldWriteBlock;
	    
	    fX = iX;
	    fY = iY;
	    iX = ep->location.x;
	    iY = ep->location.y;
	    ep->location.x = *(int *)(&fX);
	    ep->location.y = *(int *)(&fY);
	    if (sendByContext)
	    	psc = IDToPSContext(contextId == 0 ? ep->window : contextId);
	    else {
	    	if (!win->owner) return(false);
		psc = IDToPSContext(win->owner);
	    }
	    if (psc == NULL) PSInvalidID();

	    /* The event-writing dance!  First, if there isn't enough
	       room in the output buffer for the complete event now,
	       flush it.  Then, only if there's enough room in the buffer,
	       call fwrite to put it in there.  Then let the normal
	       event-flushing controlled through EventFlush et al
	       take over.  Lastly, in order to implement the non-
	       blocking nature of things, we save and restore the
	       writeBlock value around this. */

	    oldWriteBlock = currentPSContext->scheduler->writeBlock;
	    currentPSContext->scheduler->writeBlock = false;
	    if (psc->out->cnt < (sizeof(eventHeader)+sizeof(*ep)))
	    	fflush(psc->out);
	    if (psc->out->cnt >= (sizeof(eventHeader)+sizeof(*ep))) {
	        if (psc->scheduler->objectFormat == EASY_FORMAT) {
		    fwrite(&eventHeader,1,sizeof(eventHeader),psc->out);
		    fwrite(ep,sizeof(*ep),1,psc->out);
		} else WriteEvent(psc->out,ep,psc->scheduler->objectFormat);
		WCSendEvent(IPCGetWCParams(psc->in), psc == ActiveApp,
								ep->time);
		EventFlush(psc->out);
	    }   /* if there wasn't room, these events end up on the floor */
	    currentPSContext->scheduler->writeBlock = oldWriteBlock;
	    TimedEvent(5);
#if DUMMY_LOG_NEEDED /* Leo 05Mar88 should not be necessary again */
	    /* HACK! Put in keyLog */
	    if ((ep->type == NX_KEYDOWN)||(ep->type == NX_KEYUP))  {
		    keyLog[keyLogIndex].type = ep->type;
		    keyLog[keyLogIndex].charCode = ep->data.key.charCode;
		    keyLogIndex++;
		    if (keyLogIndex >= 20) keyLogIndex = 0;
	    }
#endif DUMMY_LOG_NEEDED
	    ep->location.x = iX;
	    ep->location.y = iY;
	} else {
	    /* Push parts of event on stack, call proc */
	    PSMark();	/* First put a mark */
	    PSPushInteger(ep->type);
	    PSPushInteger(iX);
	    PSPushInteger(iY);
	    PSPushInteger(ep->time);
	    PSPushInteger(ep->flags);
	    PSPushInteger(ep->window);
	    PSPushInteger(ep->data.compound.subType);
	    PSPushInteger(ep->data.compound.misc.L[0]);
	    PSPushInteger(ep->data.compound.misc.L[1]);
	    /* Execute event proc */
	    TimedEvent(4);
	    eventErrorPending = PSExecuteObject(&eProc);
	    PSClrToMrk();
	}
	sent = true;
    }
    return(sent);
}

/*****************************************************************************
    PostEvent determines the window or windows to which the event should be
    sent and uses SendEvent to send it.
******************************************************************************/

boolean PostEvent(NXEvent *ep, int how, int contextId)
{
    PWindowDevice win;
    boolean sent;
    boolean postByContext;

    postByContext = (how==NX_BYPSCONTEXT)||(how==NX_POSTEVENTBYCONTEXT);
    if (how == NX_BYTYPE) how = PostByCode(ep->type);
    /* If this window was already given to someone, make it
       have a global location again */
    if ((ep->window > 0)&&!postByContext)
	LocalToGlobal(ID2Wd(ep->window), &ep->location.x, &ep->location.y);
    switch(how) {
    case NX_NOWINDOW: /* Do not send event */
	ep->window = 0;
	break;
    case NX_BROADCAST: /* Send to all windows; handled below */
	break;
    case NX_TOPWINDOW: /* Send to Frontmost window */
	win = GetFrontWindowDevice();
	ep->window = (win ? win->id : 0);
	break;
    case NX_FIRSTWINDOW: /* Send to first window that wants it */
    case NX_NEXTWINDOW:  /* Send to next window after ep->window
			    that wants it -- usually used on repost */
	if (how == NX_NEXTWINDOW)
	    win = GetNextWindowDevice(ID2Wd(ep->window));
	else
	    win = GetFrontWindowDevice();
	while(win != NULL)
	{
	    if (win->eventMask & EventCodeMask(ep->type))
		break;
	    win = GetNextWindowDevice(win);
	}
	ep->window = (win ? win->id : 0);
	break;
    case NX_MOUSEWINDOW: /* Send to window under mouse */
	if (mouseWindow != NULL)
	    ep->window = mouseWindow->id;
	else
	    ep->window = 0; /* No window under mouse */
	break;
    case NX_LASTLEFT: /* Send to window that got last mouseDown */
	ep->window = lastLeft;
	break;
    case NX_LASTRIGHT: /* ...or for the right button */
	ep->window = lastRight;
	break;
    case NX_LASTKEY: /* or that got the last keydown */
	ep->window = (ep->data.key.keyCode == lastKey.keyCode ?
	    lastKey.window : prevKey.window );
	break;
    case NX_EXPLICIT:	 /* window already specified in event */
    case NX_TRANSMIT:	 /* window already specified & force transmit */
    case NX_BYPSCONTEXT: /* pscontext already specified & force transmit */
    case NX_POSTEVENTBYCONTEXT: /* same as NX_BYPSCONTEXT -- 01Mar90 Dave */
	break;
    } /* switch(how) */
    sent = false;
    if (how == NX_BROADCAST) {
	for(win = windowBase; win != NULL; win = win->next) {
	    ep->window = win->id;
	    sent |= SendEvent(ep, true, false, 0);
	}
    } else
    	sent = SendEvent(ep,(how==NX_TRANSMIT), postByContext,
			(how==NX_POSTEVENTBYCONTEXT ? contextId : 0));
    return(sent);
}

private	struct {
    short entries[NX_NUMPROCS+1];
} postHow, defaultPostHow = { {
	NX_NOWINDOW,
	NX_MOUSEWINDOW,	NX_LASTLEFT,	NX_MOUSEWINDOW,	NX_LASTRIGHT,
	NX_FIRSTWINDOW,	NX_LASTLEFT,	NX_LASTRIGHT, 	NX_EXPLICIT,
	NX_EXPLICIT,	NX_FIRSTWINDOW,	NX_LASTKEY,	NX_FIRSTWINDOW,
	NX_EXPLICIT,	NX_FIRSTWINDOW,	NX_EXPLICIT } };

/*****************************************************************************
    PostByCode will look at the event code and return one of the how constants.
******************************************************************************/

public int PostByCode(int what)
{
    if ((what >= 0) && (what <= NX_NUMPROCS))
	return(postHow.entries[what]);
    else
	return(NX_NOWINDOW);
}

/*****************************************************************************
    SetHowPost changes the postTypes entry for what to how.
******************************************************************************/

public procedure SetHowPost(int what, short how)
{
	postHow.entries[what] = how;
}

/*****************************************************************************
    EventFlush
    Whenever the code that writes events wants to flush an event,
    it calls EventFlush.  EventFlush will
    - go ahead and flush the stream if we're not the event context
    - if we are the event context, and we are in the middle of a 
      GetEvents loop, it will not immediately flush the stream,
      but will just remember it, and not flush it until either
      - We finish dispatching all events, or
      - An event needs to be flushed to a different stream
    The reason we worry about this is so that we won't flush the
    same stream <n> times, each time with one event; we want to
    cache those up until we've looked at all the events there are.
    
    BeginFlush and EndFlush are used only by PSGetEvents, below,
    to mark the beginning and end of the getevents loop through
    all available events.
******************************************************************************/

private Stm lastEventStm; /* The last place we wrote events, or NULL */

static void BeginFlush()
{
    currentPSContext->scheduler->dispatching = true;
}

static void EventFlush(Stm stm)
{
    if (!currentPSContext->scheduler->dispatching)
    {
	fflush(stm);
	return;
    } else if ((lastEventStm != NULL)&&(lastEventStm != stm))
	fflush(lastEventStm);
    lastEventStm = stm;
}

static void EndFlush()
{
    if (lastEventStm != NULL)
	fflush(lastEventStm);
    lastEventStm = NULL;
    currentPSContext->scheduler->dispatching = 0;
}

/*****************************************************************************
    DispatchEvents dispatches all the events it can find in the low-level
    event queue.  When we find a mouse event in the low-level queue, though,
    that isn't yet a real event, but just an indication that we need to
    recalculate tracking rects (and potentially post a number of
    entered/exited events).
******************************************************************************/

#define MOUSEEVENTMASK (NX_LMOUSEDOWNMASK | NX_LMOUSEUPMASK | \
	NX_RMOUSEDOWNMASK | NX_RMOUSEUPMASK | NX_MOUSEEXITEDMASK)

public procedure DispatchEvents()
{
    NXEvent event;
    NXEQElement	*oldHead;

    /* DispatchEvents will, when called, dispatch all
       events (of any type) that are in the low-level
       event queue. */
    while (1) {
	if (eventGlobals->LLEHead != eventGlobals->LLETail) {
	    /* Get event out of queue */
	    oldHead=(NXEQElement *)&eventGlobals->lleq[eventGlobals->LLEHead];
	    oldHead->sema = 1;
	    event = oldHead->event;
	    ClearEvent(&oldHead->event);
	    eventGlobals->LLEHead = oldHead->next;
	    oldHead->sema = 0;
	    /* Now send it on to its destination */
	    if (EventCodeMask(event.type)&MOUSEEVENTMASK)
		RecalcMouseRect(event.location.x, event.location.y, 0, 1);
	    if (event.type != NX_MOUSEEXITED) {
		event.location.y = remapY - event.location.y;
		(void)PostEvent(&event, NX_BYTYPE, 0);
		if (eventErrorPending) break;
	    }
	} else break;
    }
}

#define NX_WINEXPOSED 		0
#define NX_APPACT 		1
#define NX_APPDEACT 		2
#define NX_WINRESIZED 		3
#define NX_WINMOVED 		4
#define NX_WINDRAGGED		5
/* 6 and 7 are already used */
#define NX_WINCHANGED		8

/*****************************************************************************
    PostRedraw posts a NX_WINEXPOSED event for the given rect.
******************************************************************************/

public procedure PostRedraw(PWindowDevice win, int x, int y, int w, int h)
{
	NXEvent	e;

	ClearEvent(&e);
	e.type = NX_KITDEFINED;
	e.location.x = x;
	e.location.y = y;
	e.data.compound.subType = NX_WINEXPOSED;
	e.data.compound.misc.L[0] = w;
	e.data.compound.misc.L[1] = h;
	e.window = win->id;
	(void)PostEvent(&e, NX_EXPLICIT, 0);
}

/*****************************************************************************
    PostChanged posts a NX_WINCHANGED event for the given rect.
******************************************************************************/

public procedure PostChanged(PWindowDevice win, int x, int y, int w, int h)
{
	NXEvent	e;

	ClearEvent(&e);
	e.type = NX_KITDEFINED;
	e.location.x = x;
	e.location.y = y;
	e.data.compound.subType = NX_WINCHANGED;
	e.data.compound.misc.L[0] = w;
	e.data.compound.misc.L[1] = h;
	e.window = win->id;
	(void)PostEvent(&e, NX_EXPLICIT, 0);
}

/*****************************************************************************
    PostDeathEvent posts a Sysdefined event with the subType 0 and the first
    long word of data set to the given id. This is used to notify higher-level
    software of the death of pscontexts.
******************************************************************************/

public procedure PostDeathEvent(int id)
{
    static Point there = {0, 0};
    NXEventData data;
    
    data.compound.subType = 0;
    data.compound.misc.L[0] = id;
    data.compound.misc.L[1] = 0;
    LLEventPost(NX_SYSDEFINED, there, &data);
}

/*****************************************************************************
    ClearEvent makes the given event into a real no-op of an event.
******************************************************************************/

public procedure ClearEvent(NXEvent *ep)
{
    static NXEvent nullEvent = {NX_NULLEVENT, {0, 0 }, 0, -1, 0 };

    *ep = nullEvent;
    ep->data.compound.subType = ep->data.compound.misc.L[0] =
	ep->data.compound.misc.L[1] = 0;
}

private int PopIntValue()
{
    if (PSGetOperandType() == dpsIntObj)
    	return(PSPopInteger());
    else {
    	float r;
	
	PSPopPReal(&r);
	return((int)r);
    }
}

private procedure popEvent( NXEvent *ep )
{
    ep->data.compound.misc.L[1] = PopIntValue();
    ep->data.compound.misc.L[0] = PopIntValue();
    ep->data.compound.subType = PSPopInteger();
    ep->window = PSPopInteger();
    ep->flags = PSPopInteger();
    ep->time = PSPopInteger();
    ep->location.y = PopIntValue();
    ep->location.x = PopIntValue();
    ep->type = PSPopInteger();
}

private procedure PSPostEvent()
{
    NXEvent event;
    int how;
    
    how = PSPopInteger();
    popEvent(&event);
    PSPushBoolean(PostEvent(&event, how, 0));
}

private procedure PSPostEventByContext()
{
    NXEvent event;
    int contextId;
    
    contextId = PSPopInteger();
    popEvent(&event);
    PSPushBoolean(PostEvent(&event, NX_POSTEVENTBYCONTEXT, contextId));
}

private procedure PSSetHowPost()
{
    int what, how;
    
    how = PSPopInteger();
    what = PSPopInteger();
    if ((how < NX_NOWINDOW) || (how > NX_BYPSCONTEXT))
	PSRangeCheck();
    if ((what < NX_FIRSTEVENT) || (what > NX_LASTEVENT))
	PSRangeCheck();
    SetHowPost(what, how);
}

private procedure PSSetFlushExposures()
{
    flushExposures = PSPopBoolean();
}

private procedure PSCurrentHowPost()
{
    int what;

    what = PSPopInteger();
    if ((what < NX_FIRSTEVENT) || (what > NX_LASTEVENT))
	PSRangeCheck();
    PSPushInteger(PostByCode(what));
}

/* Initialize the mouse and keyboard event system.
 * Sets PostScript ClockAddress to point to the vertical retrace clock.
 * Sets the Mouse PostScript context to the current context.
 * Has every screen device register itself with the ev driver.
 * Puts up the cursor.
 */
private procedure PSInitEvents()
{
    void (*proc)();
    NXDevice *d;

    if (port_allocate(task_self_, &eventPort) != KERN_SUCCESS)
	PSLimitCheck();
    if (port_set_backlog(task_self_, eventPort,1) != KERN_SUCCESS)
	PSLimitCheck();
    InitMouseEvents(eventPort);
    
    /* Leo 20Mar89.  We had to initialize the mouse rect to some real rectangle
     * so that when the first window is created, TestMouseRect will return
     * true, thus enabling resetting of the mouseWindow.  See the routine
     * RecalcMouseRect in mouse_driver.c.
     */
    SetMouseRect(&wsBounds);
    PSSetRealClockAddress((PCard32)&eventGlobals->VertRetraceClock);
    MousePSContext = currentPSContext;
    /* Ted 01Mar90.  Have each screen device register itself with the
     * ev driver for cursor and brightness callback procedures.
     */
    for (d=deviceList; d; d=d->next) {
        if (proc = d->driver->procs->RegisterScreen)
	    (*proc)(d);
    }
    StartCursor();
}

private procedure PSTermEvents()
{
    TermMouseEvents();
    MousePSContext = NULL;
}

private procedure PSGetEvents()
{
    YieldReason because;
    static msg_header_t *msg;
    static int beenHere;
    
    if (!beenHere) {
	beenHere = 1;
	/* Initialization: get scheduler to block us on eventPort */
	because = yield_other;
	msg = (msg_header_t *)eventPort;
    } else /* msg already contains pointer to last message */
	because = yield_stdin;
    eventErrorPending = 0;
    /* Set up an error handler for the deferred flush stuff */
    DURING
    /* Loop forever, dispatching events.  We'll get bounced
       out of this loop if an event procedure encounters an
       error; that's why the executive that calls us has
       it's own loop,  This loop, however, allows us to not
       go back to PostScript and to not pay the penalty of
       the establishing the DURING every time. */
    while(!eventErrorPending) {
	if (eventGlobals->LLEHead == eventGlobals->LLETail)
	    PSYield(because, &msg);
	TimedEvent(3);
	BeginFlush();
	DispatchEvents();
	if (flushExposures)
	    FlushRedrawRects(); /* that may have accumulated in event procs */
	EndFlush();
	because = yield_stdin;
    } /* while(!eventErrorPending) */
    HANDLER
    {
        if (currentPSContext->scheduler->dispatching)
	    EndFlush();
	RERAISE;
    }
    END_HANDLER;
}

/*****************************************************************************
      <pscontext id> setactiveapp -
    Sets the "currently active app" a context representing the active app.
    A zero argument means there is no active app.
******************************************************************************/

private procedure PSSetActiveApp()
{
    int ctxtID;
    PSContext newCtxt;
    char previousValue;

    ctxtID = PSPopInteger();

    if (ctxtID == 0)
      newCtxt = NULL;
    else {
      newCtxt = IDToPSContext(ctxtID);
      if (newCtxt == NULL)
          PSInvalidID();
    }

    if (newCtxt != ActiveApp) {
      WCSetData(ActiveApp ? IPCGetWCParams(ActiveApp->in) : NULL,
                      newCtxt ? IPCGetWCParams(newCtxt->in) : NULL);
      ActiveApp = newCtxt;
    }
}

/*****************************************************************************
      - currentactiveapp <pscontext id>
    Returns the "currently active app" a context representing the active app.
    A zero returned means there is no active app.
******************************************************************************/

private procedure PSCurrentActiveApp()
{
    if (ActiveApp)
      PSPushInteger((integer)PSContextToID(ActiveApp).stamp);
    else
      PSPushInteger(0);
}

/*****************************************************************************
    <time> setlasteventsenttime
    This is a private hack for the benefit of journaling.  It is used when
    the journaling stuff posts an event to more accurately simulate what
    really happens in the window server when we post a real event.
******************************************************************************/

private procedure PSSetLastEventSentTime()
{
    WCSendEvent(IPCGetWCParams(currentPSContext->in),
                      currentPSContext == ActiveApp,
                      PSPopInteger());
}


/*****************************************************************************
      <bool> <pscontext id> setwaitcursorenabled -
    Sets whether the automatic wait cursor is enabled when the given context
    is the active app. A zero context argument sets a global flag that is
    and'ed with the per context flags.
******************************************************************************/

private procedure PSSetWaitCursorEnabled()
{
    int ctxtID;
    int flag;
    PSContext ctxt;

    ctxtID = PSPopInteger();
    flag = PSPopBoolean();

    if (ctxtID == 0)
      SetGlobalWCEnabled(flag);
    else {
      ctxt = IDToPSContext(ctxtID);
      if (ctxt == NIL)
          PSInvalidID();
      WCSetEnabled(IPCGetWCParams(ctxt->in), ctxt == ActiveApp, flag);
    }
}

/*****************************************************************************
      <pscontext id> currentwaitcursorenabled <flag>
    Returns whether the automatic wait cursor is enabled for the given context.
    A zero context argument returns a global flag that is and'ed with the
    per context flags.
******************************************************************************/

private procedure PSCurrentWaitCursorEnabled()
{
    int ctxtID;
    PSContext ctxt;
    WCParams *wcParams;

    ctxtID = PSPopInteger();
    if (ctxtID == 0)
      PSPushBoolean(CurrentGlobalWCEnabled());
    else {
      ctxt = IDToPSContext(ctxtID);
      if (ctxt == NIL)
          PSInvalidID();
      wcParams = IPCGetWCParams(ctxt->in);
      if (wcParams)
          PSPushBoolean(wcParams->waitCursorEnabled);
      else
          PSPushBoolean(0);
    }
}

/*****************************************************************************
      startwaitcursortimer
    Does the equivalent wait cursor stuff as when an event is generated.
******************************************************************************/

private procedure PSStartWaitCursorTimer()
{
    WCSendEvent(IPCGetWCParams(currentPSContext->in),
	currentPSContext == ActiveApp, -1);
}

/*****************************************************************************
      currenteventtime
    Is a way to get the current time in the units that events are timestamped.
******************************************************************************/

private procedure PSCurrentEventTime()
{
    PSPushInteger(eventGlobals->VertRetraceClock);
}

private readonly RgOpTable cmdEvents = {
  "currenteventtime", PSCurrentEventTime,
  "currenthowpost", PSCurrentHowPost,
  "currentactiveapp", PSCurrentActiveApp,
  "currentwaitcursorenabled", PSCurrentWaitCursorEnabled,
  "initevents", PSInitEvents,
  "getevents", PSGetEvents,
  "postevent", PSPostEvent,
  "posteventbycontext", PSPostEventByContext,
  "setactiveapp", PSSetActiveApp,
  "setflushexposures", PSSetFlushExposures,
  "sethowpost", PSSetHowPost,
  "setlasteventsenttime", PSSetLastEventSentTime,
  "setwaitcursorenabled", PSSetWaitCursorEnabled,
  "startwaitcursortimer", PSStartWaitCursorTimer,
  "termevents", PSTermEvents,
  NIL};

public procedure EventInit(int reason)
{
    switch(reason) {
        case 0:
	    postHow = defaultPostHow;
	    flushExposures = true;
	    lastEventStm = NULL;
    	    break;
	case 1:
	    MousePSContext = NULL;
	    PSRgstOps(cmdEvents);
	    break;
    }
}












