/*****************************************************************************

	ipcscheduler.c
	Scheduler for the NeXT Display PostScript interpreter
	
	CONFIDENTIAL
	Copyright (c) 1988 NeXT, Inc. as an unpublished work.
	All Rights Reserved.

	Created 25Sep86 Leo
	
	Modified:
	
	09Mar88 Leo     Use multiple fds per PSContext
	18Apr88 Leo     Deleted old change log, gs->window scheme
	23May88 Leo	Initial savelevel for INIT context is -1
	24May88 Leo	Asynchronous vs. Synchronous timeslicing
	07Jun88 Leo	Sysdefined events on pscontext termination
	07Jul88 Leo	Integration with Adobe PSContext scheme;
			removal of windowlist
	11Sep88 Leo	modified from psscheduler
	20Sep88 Jack	Updated to v1003
	23Sep88 Leo	Included GC stuff
	26Sep88 Leo	New treatment of Notify
	10Oct88 Leo	login context
	25Oct88 Leo	Bug fix in PSYield yield_stdin case
	13Jan89 Jim	update to v1004
	08Feb89 Dave	added typed contexts
	14Feb89 Dave	Moved PostScript-specific stuff
			(SelfDestructPSContext, PSMakeRunnable)
			to pscontext.c  Changed PSYield to ContextYield
	11Jun89 Leo	Initialize objectFormat, dispatching
	25Jul89 Leo	Initialize system and restricted bits (they have
			never been initialized!)
	22Sep89 Dave	make lastRun public for the ipcstream impl.,
			changed name to currentSchedulerContext
	2Oct89	Dave	added initialisation for I/O stream pointers, moved PS-
			context specific routines to pscontext.c
	10Nov89 Terry	CustomOps conversion
	04Dec89 Ted	Integratathon!
	06Dec89 Ted	ANSI C Prototyping and reformatting.
	02Feb90	Dave	Added check for printing context.
	02Apr90	Terry	Initialize mono+color accuracies for scheduler contexts
	04Jun90 Dave	Removed references to stmin and stmout fields, changed
			context msg notification to call through proc vector.
	07Jun90	Terry	Moved *WriteBlock + SetNextObjectFormat to listener.c

******************************************************************************/

#import PACKAGE_SPECS
#import CUSTOMOPS
#import ENVIRONMENT
#import EXCEPT
#import POSTSCRIPT
#import STREAM
#import BINTREE
#import MOUSEKEYBOARD	/* for VertRetraceClock */
#import <sys/notify.h>
#import <sys/port.h>
#import <sys/time.h>
#import "ipcstream.h"
#import "ipcscheduler.h"
#import "timelog.h"
#import "timeslice.h"

#if SWAPBITS
#define DEFAULT_FORMAT 2
#else SWAPBITS
#define DEFAULT_FORMAT 1
#endif SWAPBITS

typedef struct _SchedulerMsg {
    msg_header_t	header;
    msg_type_long_t	type;
    unsigned char	contents[MESSAGEDATASIZE];
} SchedulerMsg;


#define	COROUTINESTACKSIZE 32700 /* Max stack usage (32K - 68 bytes) */

public PSSchedulerContext contextList;
				/* Linked list of all states */
public NSContextType *currentContextType;
				/* current context type (index into table) */
public	Coroutine scheduler;	/* coroutine for the scheduler */
private PSContext schedCtxt;	/* Context for the scheduler, if exists */
public PSSchedulerContext currentSchedulerContext;
				/* current running non-scheduler context */
private	char *scStorage;	/* pointer to PSSchedulerContext pool */
private	boolean	stillRunning;	/* Whether the scheduler is in the middle
				   of running */
private port_t notifyPort;	/* Our tasks' notify port (cached for speed) */
private	PSSchedulerContext portNeedsContext; /* Context whose inputPort needs
				   restriction; see HandleMsg and Scheduler */
extern procedure TermScheduler(); /* forward */

/*****************************************************************************
	CoroutineTopLevel
	is called by the coroutine package
	at the beginning of every Coroutine
	except the initial one.  It calls the procedure
	pointed to by proc, which must never return.
******************************************************************************/

private procedure CoroutineTopLevel(Coroutine source, PVoidProc proc)
{
    (*proc)(); /* never returns */
    CantHappen();
}

public PSSchedulerContext CreateTypedContext(PVoidProc proc, NSContext ctx,
    NSContextType *type)
{
    PSSchedulerContext newPSContext;
    
    newPSContext = (PSSchedulerContext)os_newelement(scStorage);
    newPSContext->coroutine = CreateCoroutine(CoroutineTopLevel, (char *)proc,
    	COROUTINESTACKSIZE); /* destroyed in NSDestroyContext() in nscontexts.h */
    /* add the new context to the list */
    newPSContext->next = contextList;
    contextList = newPSContext;
    newPSContext->type = type;
    newPSContext->inputPort = (port_t)0;
    newPSContext->remotePort = (port_t)0;
    newPSContext->wannaRun = true;
    newPSContext->userInteraction = false;
    newPSContext->waitMsg = false;
    newPSContext->waitNotify = false;
    newPSContext->killMe = false;
    newPSContext->restricted = false;
    newPSContext->login = false;
    newPSContext->system = false;
    newPSContext->userSet = false;
    newPSContext->writeBlock = true;
    newPSContext->objectFormat = DEFAULT_FORMAT;
    newPSContext->dispatching = false;
    newPSContext->printerContext = false;
    newPSContext->writeProhibited = false;
    newPSContext->context = ctx;
    newPSContext->defaultDepthLimit = initialDepthLimit;
    return(newPSContext);
}
    
/*****************************************************************************
    SetRemotePort is called by a listener to set up the port the death of
    which will cause the given context to be terminated.
******************************************************************************/

public procedure SetRemotePort(PSSchedulerContext psc, port_t rPort)
{
	psc->remotePort = rPort;
}

/*****************************************************************************
    MarkSystemContexts marks all existing contexts as system contexts.
******************************************************************************/

public procedure MarkSystemContexts()
{
    PSSchedulerContext es;

    for (es = contextList; es != NULL; es = es->next)
	es->system = true;
}

/*****************************************************************************
    TerminateUserContexts causes a terminate notification in all contexts
    that are not system contexts.
******************************************************************************/

public procedure TerminateUserContexts()
{
    PSSchedulerContext es;

    for (es = contextList; es != NULL; es = es->next)
	if (!es->system) NSTermContext(es->type, es);
}

/*****************************************************************************
	Set the printerContext bit to true.  When this bit is
	true, the Scheduler looks to see if anyone else wants to
	run before running this one.  It's kind of a cheap way to
	put other contexts at a higher priority than the printer.
	This routine is called from NpWakeup() and PSPrinterDevice().
******************************************************************************/
public procedure SetPrinterContext( PSSchedulerContext psc )
{
    psc->printerContext = true;
}

public procedure DidInteract()
{
    currentSchedulerContext->userInteraction = true;
}

/*****************************************************************************
    The Message list
    is maintained by the routine AcquireMsg and ReleaseMsg.
    The scheduler maintains only as many messages at any
    one time as are necessary to give to contexts that
    are actively running.  Each such message has room
    for up to about 1K of inline data; it is expected that
    the client will send out-of-line data for anything
    more than that.  Usually a message is acquired from
    AcquireMsg and the msg_receive calls put a message
    destined for some context in it.  Then we CoReturn
    to that context, granting the message to that context.
    It may timeout before finishing executing the contents
    of the message, meaning that that particular message
    is not available for reuse.  SO what we do is maintain
    potentially multiple messages, keeping a free buffer
    of one (to cover the common case where it does finish
    executing the message before getting back to the scheduler).
******************************************************************************/

SchedulerMsg *freeMsg;	/* Free list of one */

msg_header_t *AcquireMsg(SchedulerMsg *m)
{
    if (!m)
	if (freeMsg)
	{
	    m = freeMsg;
	    freeMsg = NULL;
	}
	else
	    m = (SchedulerMsg *) malloc(MESSAGETOTALSIZE);
    m->header.msg_local_port = PORT_DEFAULT;
    m->header.msg_size = MESSAGETOTALSIZE;
    m->header.msg_simple = 1;
    return((msg_header_t *)m);
}

void ReleaseMsg(msg_header_t *m)
{
    msg_type_t *mt;
    
    /* Used to free out-of-line data here.  Now we expect the routine that
       receive the messages to do that.   It also lets them not free the
       data if they want to keep it.  */
    /* Now deal with the message buffer itself */
    if (freeMsg)
	free((char *)m);
    else
	freeMsg = (SchedulerMsg *)m;
}

/*****************************************************************************
    This routine exists only to help
    out the debugger.  For some reason
    you need that extra stack frame,
    or the debugger's stack backtrace
    fails when you're in msg_receive.
******************************************************************************/

private msg_return_t Msg_Receive(msg_header_t *header, msg_option_t option,
    msg_timeout_t timeout)
{
    return(msg_receive(header, option, timeout));
}

/*****************************************************************************
	ContextYield can be called by anyone who
	wants to yield now.  It causes the scheduler to run
	again.  In all cases where the context had a SchedulerMsg
	it was not done with, we need to restrict its port so that
	we will not try to receive another message on that port.
	We also set or clear wannaRun for the context based on
	the exact nature of the ContextYield.
******************************************************************************/

public procedure ContextYield(YieldReason  reason, int data)
{
    PSSchedulerContext psc = currentSchedulerContext;
	/*currentPSContext->scheduler*/
    boolean restrictPort;
    extern Coroutine scheduler;	/* coroutine for the scheduler */
    
    restrictPort = false;
    switch (reason) {
    case yield_time_limit:
    case yield_by_request: /* Yield, run again ASAP */
	psc->wannaRun = true;
	restrictPort = true; /* Not done with current message yet */
	break;
    case yield_terminate: /* Get rid of me */
    	psc->killMe = true;
	break;
    case yield_stdin: {
    	/* Yield, data points to a pointer to a message I'm
	   done with, wait for another message on the same port. */
	SchedulerMsg **mpp = (SchedulerMsg **)data;
	
	if (psc->inputPort != (*mpp)->header.msg_local_port) {
	    port_restrict(task_self_,psc->inputPort);
	    psc->inputPort = (*mpp)->header.msg_local_port;
	    psc->restricted = 1;
	}
	psc->waitMsg = true;
	psc->msgPtrP = mpp;
	ReleaseMsg((msg_header_t *) *mpp);
        UnrestrictInputPort(psc);
	}
	break;
    case yield_stdout: /* Yield, waiting for notify_msg_accepted on data */
    	if (psc->killMe) return; /* This is so that a dying context can't
				    block on output */
    	psc->waitNotify = true;
	psc->notifyPort = (port_t)data;
	restrictPort = true;
	break;
    case yield_fflush: /* Never used */
    	CantHappen();
    case yield_wait: /* waiting on condition */
    	psc->wannaRun = false;
	restrictPort = true;
	break;
    case yield_other: /* Initialization: set my input port to (port_t)*data,
    	and set my msgPtrP to data, and unrestrict my input port */
	if (psc->inputPort&&(psc->inputPort != (*(port_t *)data)))
	    port_restrict(task_self_,psc->inputPort);
    	psc->waitMsg = true;
	psc->inputPort = *(port_t *)data;
	psc->msgPtrP = (SchedulerMsg **)data;
	psc->restricted = 0;
        port_unrestrict(task_self_, psc->inputPort);
	break;
    } /* switch (reason) */
    if (restrictPort) RestrictInputPort(psc);
    CoReturn(scheduler);
    NSCheckNotifyContext(psc->type, psc);
} /* ContextYield */

/*****************************************************************************
    HandleNotifyMsg takes care of changing any context status related 
    to receipt of the given notify message.
******************************************************************************/

private boolean HandleNotifyMsg(notification_t *m)
{
    switch (m->notify_header.msg_id) {
    case NOTIFY_PORT_DELETED:
    	/* This message means that we have lost access to the
	   port, most likely because the remote process has gone
	   away.  So, we need to do two things:  destroy any processes
	   whose remote port is that port, and wake up any processes
	   that are blocked for output on that port. */
	{
	    PSSchedulerContext es;
	
	    for (es = contextList; es != NULL; es = es->next)
	    {
		if (es->remotePort == m->notify_port)
		    NSTermContext(es->type, es);
		else
		    if (es->notifyPort == m->notify_port)
		    {
		    	es->wannaRun = true;
			es->waitNotify = false;
			es->notifyPort = (port_t) 0;
		    }
	    }
	}
    	break;
    case NOTIFY_MSG_ACCEPTED:
    	/* This message means that at some previous point we
	   received a SEND_WILL_NOTIFY message from a msg_send
	   to this port.  Now, the kernel is notifying us that
	   there is room for more messages at that port.  We do
	   two things:  for every context which is blocked for
	   output against that port, we wake it up; and, we look
	   for a context whose remote port is that guy.  Him, we
	   call IPCNotifyRecevied on.  The ipc streams mechanism
	   requires this to clear some state in the stream struct. */
    	{
	    PSSchedulerContext psc;
	    
	    for (psc = contextList; psc; psc = psc->next)
	    {
	        if (psc->waitNotify && (psc->notifyPort == m->notify_port))
		{
		    psc->wannaRun = true;
		    psc->waitNotify = false;
		    psc->notifyPort = (port_t) 0;
		}
		if (NSUsesIPCStms(psc->type))
		{
		    if (psc->remotePort == m->notify_port)
			NSNotifyContext( psc->type, psc );
		}
	    }
	}
    	break;
    default:
    	break;
    } /* switch (message type) */
    return(false); /* message is never in use afterwards */
} /* HandleNotifyMsg */

/*****************************************************************************
    HandleDataMsg looks to see if any context is waiting for a message on the
    port the given mesage arrived on, and if so gives the message to that
    context and makes it runnable.  It returns whether that message is
    'in use' by some context after looking through the list.
******************************************************************************/

private boolean HandleDataMsg(msg_header_t *m)
{
    PSSchedulerContext psc = contextList;
    
    for (psc = contextList; psc; psc = psc->next)
	if (psc->waitMsg&&(psc->inputPort == m->msg_local_port)) {
	    psc->wannaRun = true;
	    psc->waitMsg = false;
	    *psc->msgPtrP = (SchedulerMsg *)m;
	    /* Now, strictly speaking we should set this guy's port to be
	    restricted now, because we don't want any more messages to be
	    received for it.  However, because we now know somebody is
	    runnable, we know we won't actually do another msg_receive
	    until somebody has run, we'll lazy evaluate the restriction
	    of this port by setting portNeedsContext, and the input port
	    will get restricted down in the Scheduler routine if needed. */
	    portNeedsContext = psc;
	    return(true);
	}
#if (STAGE == DEVELOP)
    os_fprintf(os_stderr,"HandleDataMsg: message unused!\n");
#endif (STAGE == DEVELOP)
    return(false);
}

private boolean HandleMsg(msg_header_t *newMsg)
{
    if (newMsg->msg_local_port == notifyPort)
	return(HandleNotifyMsg((notification_t *)newMsg));
    else
	return(HandleDataMsg(newMsg));
}

/*****************************************************************************
    PickPSContext is called by SelectPSContext.  Given a message that has just
    arrived, it will choose what context if any should be run next.  It also
    will set *inUseP to indicate whether the given message is now in use.

    If the context that wants to run is a printer context, then
    simply clear it's printerContext bit and keep looping around.
    That way, if there are otehr who want to run, they will; if no-one
    else wants to run, we'll end up back at the printer context (whos
    wannaRun bit is still on) and it will get run.  The printerContext
    bit gets turned back on in NpWakeup().
    
    Should check event dispatch state first, then listener state... 
    Should look at several priority levels...  FIX
******************************************************************************/

private PSSchedulerContext PickContext(PSSchedulerContext lastPS)
{
    PSSchedulerContext psc, first;

    /* Run through context list looking for somebody runnable */
    if (lastPS == NULL)
	psc = contextList;
    else
	psc = lastPS;
    first = psc;
    do {
    loopTop:
	if ((psc = psc->next) == NULL) psc = contextList;
	if (psc->wannaRun)
	{
	    /* check to see if this is really a printer context */
	    if (psc->printerContext)
	    {
		psc->printerContext = false;
		first = psc;
		goto loopTop; /* avoid (first != psc) check */
	    }
	    return(psc);
	}
    } while(first != psc);
    return(NULL);
}

/*****************************************************************************
    SelectPSContext will return with the PSSchedulerContext that should be
    run next.  It will sleep until some PSSchedulerContext wishes to run.
******************************************************************************/

private PSSchedulerContext SelectPSContext()
{
    PSSchedulerContext newPSC;
    msg_option_t options;
    msg_header_t *newMsg;
    boolean msgInUse;
    int	msgResult;

    /* OK, here's what we do.  Do a msg_receive with an
       0 time timeout.  If we get any message, call HandleMsg
       to set anybody's wannaRun bits that need to be set.
       Then call PickContext to look for anybody runnable.
       If we don't find anybody, going around the loop again will
       cause us to do the same msg_receive, but with an infinite
       timeout, where we'll hang until a message is received. */

    TimedEvent(1);
    options = RCV_TIMEOUT;
    newPSC = NULL;
    msgInUse = true;
    while(newPSC == NULL)
    {
	newMsg = AcquireMsg(msgInUse ? NULL : (SchedulerMsg *)newMsg);
	msgInUse = false;
	if ((msgResult = Msg_Receive(newMsg,options,0)) == RCV_SUCCESS)
	    msgInUse = HandleMsg(newMsg);
#if (STAGE == DEVELOP)
	else if (msgResult != RCV_TIMED_OUT)
	    os_fprintf(os_stderr,"SelectPSContext: msg_receive got %d!\n",
			    msgResult);
#endif (STAGE == DEVELOP)
	newPSC = PickContext(currentSchedulerContext);
	options = MSG_OPTION_NONE;
    }
    if (!msgInUse) ReleaseMsg(newMsg);
    /* Now set the last rescedule time */
    if (eventGlobals)
	NSTimeoutContext(newPSC->type, newPSC, eventGlobals->VertRetraceClock
	    + TIMESLICE);
    return(newPSC);
}

public procedure Scheduler()
{
    PSSchedulerContext newPSC;

    stillRunning = true; /* I need no initialization */
    if (port_allocate(task_self_, &notifyPort) != KERN_SUCCESS)
    	PSLimitCheck();
    if (task_set_special_port(task_self_, TASK_NOTIFY_PORT, notifyPort) != 
    KERN_SUCCESS)
	PSLimitCheck();
    port_unrestrict(task_self_, notifyPort);
    
    /* Start PostScript at first */
    currentSchedulerContext = currentPSContext->scheduler;
    currentContextType = currentSchedulerContext->type;	
    while(stillRunning) {
	newPSC = SelectPSContext();
	if ((currentContextType == NULL) /* no contexts available */
	|| (newPSC->context != NSCurrentContext(currentContextType)))
	/* selected ctxt not equal to current (or not same type) */
	{
	    uid_t oldUid,uid;
	    
	    /* switch out the current context (different type than new?) */
	    NSUnloadContext(currentContextType, currentSchedulerContext);
	    
	    /*
	     *  Switch in the new context, make it current.  Since
	     *  it is possible that the user-supplied swithc-in
	     *  procedure might not want to swithc in what we have
	     *  selected, it returns what it thinks is the real
	     *  context to get run.
	     */
	    newPSC = NSLoadContext(newPSC->type, newPSC);

	    oldUid = contextUid(currentSchedulerContext);
	    uid = contextUid(newPSC);
	    if (oldUid != uid)
	    	SetEffectiveUser(uid,contextGid(newPSC));
	}
	TimedEvent(2);

	currentSchedulerContext = newPSC;
	currentSchedulerContext->wannaRun = false;
	currentSchedulerContext->userInteraction = false;
	currentContextType = currentSchedulerContext->type;
	
	/* Perform the lazy evaluation of the restriction of the port here */
	if (portNeedsContext) {
	    if (currentSchedulerContext != portNeedsContext)
		RestrictInputPort(portNeedsContext);
	    portNeedsContext = NULL;
	}
	
	CoReturn(currentSchedulerContext->coroutine);
    	if (currentSchedulerContext->killMe)
	{
	    NSDestroyContext(currentSchedulerContext->type,
	    	currentSchedulerContext);
	    /* Remove from PSContext list */
	    if (contextList == currentSchedulerContext)
		contextList = currentSchedulerContext->next;
	    else
	    {
		PSSchedulerContext prevC;
		for (prevC = contextList;
		     prevC->next != currentSchedulerContext;
		     prevC = prevC->next)
		    ;
		prevC->next = currentSchedulerContext->next;
	    }
	    os_freeelement(scStorage, currentSchedulerContext);
	    currentSchedulerContext = NULL;
	    /* Now see if we're all through */
	    if (contextList == NULL)
		stillRunning = false;
	}
    } /* while(stillRunning) */
} /* Scheduler */


public procedure SchedulerInit()
{
    InitPSContextType();
    scStorage = (char *)os_newpool(
	sizeof(struct _t_PSSchedulerContextRec), 3, -1);
    scheduler = InitCoroutine(COROUTINESTACKSIZE,0,false);
    contextList = NULL;
    currentContextType = NULL;	/* no contexts yet */
}
