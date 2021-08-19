/*****************************************************************************

    pscontext.c

    The code implement the procs for PS Contexts.
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 08Feb89 Dave
    
    Modified:

	03Oct89 Dave  Retrofit PS context types to new stream resource scheme
	10Nov89 Terry CustomOps conversion
	04Dec89 Ted   Integratathon!
	07Dec89 Ted   ANSI C Prototyping, reformatting.
	24Apr89 Terry Context streams no longer destroyed in NSDestroyPS
	23May90 Trey  Added wait cursor cleanup code when contexts go away
	04Jun90 Dave  removed extraneous reference to psc->stmin and ->stmout,
		      added NSNotifyPS().
	05Jun90 Dave  moved PS-context type specific stuff to this file.
		      upgraded context calls to new format.

******************************************************************************/

#import PACKAGE_SPECS
#import PUBLICTYPES
#import CUSTOMOPS
#import EXCEPT
#import POSTSCRIPT
#import STREAM

#if (OS == os_mach) /* Leo 10Apr88 Get NeXT entensions */
#define NeXT 1
#endif

#import <sys/types.h>
#import <sys/stat.h>
#import <sys/socket.h>
#import <sys/time.h>
#import <sys/un.h>
#import <sys/notify.h>
#import <servers/netname.h>
#import <netinet/in.h>
#import <sgtty.h>
#import <fcntl.h>
#import <signal.h>
#import "ipcstream.h"
#import "ipcscheduler.h"

#define PSCONTEXTID	1000

/* String to be executed by new context */
private char *initString = "shareddict /contextExecutive get exec";

public PSSchedulerContext NSCreatePS(notification_t *msg, NSContextType type)
{
    Stm		stm, stmOut;
    PSSpace	newSpace;
    PSContext	newPSC;
    PSContext	myContext;

    /* Save current context for later */
    myContext = currentPSContext;

    /* Give it a new space to play in */
    if ((newSpace = CreatePSSpace()) == NULL) PSLimitCheck();

    /* Create the PostScript Stms for in and out */
    CreateIPCStreams(msg->notify_header.msg_remote_port, &stm, &stmOut);

    /* Create the new context.  Note use of global initString */
    if ((newPSC = CreatePSContext(newSpace, stm, stmOut,
    CreateNullDevice(), initString)) == NULL)
    {
	DestroyPSSpace(newSpace);
	PSLimitCheck();
    }

    /* Set the remote port for this context */
    SetRemotePort(newPSC->scheduler, msg->notify_header.msg_remote_port);
    
    /* OK, it's all created.  Now send back the reply message. */
    msg->notify_port = stmPort;
    msg_send((msg_header_t *)msg, SEND_TIMEOUT, 2000); /* Could fail! */
    
    /*
     *  Creating a new context potentially reset the current
     *  context to the new one.  Since we want to stay in the
     *  listener, reset it to ourselves.
     */
    SwitchPSContext(myContext);
    return(newPSC->scheduler);
}

public PSSchedulerContext CreateSchedulerContext(PVoidProc proc, NSContext ctx) 
{
    PSSchedulerContext	newpsc;
    
    newpsc = CreateTypedContext( proc, ctx, NSGetContextType( PSCONTEXTID ) );
    return newpsc;
}

public procedure NSTermPS(PSContext ctxt)
{
    NotifyPSContext(ctxt, notify_terminate);
}

public procedure NSDestroyPS(PSContext theContext)
{
    extern PSContext	MousePSContext;
    extern PSContext	ActiveApp;
    PSSpace		deadSpace;

    deadSpace = theContext->space;
    
    /* Destroy all windows owned by this pscontext */
    TermWindowsBy(PSContextToID(theContext));

    /* If it's a login context, notify the scheduler */
    if (theContext->scheduler->login) LoginContextKilled();

    /* Make sure it's not in any global variables */
    if (theContext == ActiveApp) {
	ActiveApp = NULL;
	WCSetData(NULL, NULL);
    }

    /* Make sure it's not in any global variables */
    if (theContext == MousePSContext)
    {
	MousePSContext = NULL;
	os_fprintf( os_stderr, "Extreme Danger: Event Context Dead!\n" );
    }
    if (MousePSContext)
        PostDeathEvent(PSContextToID(theContext)); /* from event.c */
    DestroyPSContext();

    DestroyPSSpace(deadSpace); /* Just go for it, sometimes it fails */
}

public PSSchedulerContext NSLoadPS(PSSchedulerContext psc, PSContext ctxt)
{
    SwitchPSContext(ctxt);
    return currentPSContext->scheduler;  /* switch to REAL PSContext */
}

public procedure NSTimeoutPS(PSContext ctxt, int to)
{
    PSSetTimeLimit(to);
}

public procedure NSNotifyPS(PSContext ctxt)
{
    IPCNotifyReceived( ctxt->out );
}

public procedure NSCheckNotifyPS(PSContext ctxt)
{
    CheckForPSNotify();
}

/* Leo 11Jul88 This routine is called from fork.  In our system, fork
   does not change a process' standard output stream, and the input
   stream is simply initialized to the closed stream. */

public boolean PSNewContextStms(Stm *in, Stm *out)
{
	Stm stm;
	extern StmProcs closedStmProcs, ipcStmProcs;

	*in = StmCreate(&closedStmProcs, 0);
	*out = stm = currentPSContext->out;
	if (stm->procs == &ipcStmProcs)
	    stmRef++; /* Increment ipcstreams' refcount so it won't
		close until both contexts go away */
}


/*****************************************************************************
    SelfDestructPSContext is called when some routine has detected a fatal
    flaw in the current PSContext, and realized that the only hope for the
    server is to destroy it immediately (e.g., a SIGPIPE on a write).
    SelfDestructPSContext will close the context without flushing anything,
    and will then run the scheduler.
******************************************************************************/

public procedure SelfDestructPSContext()
{
#if (STAGE == DEVELOP)
    os_fprintf(os_stderr,"%s: Attempting to destroy ps context %d.\n",
	"SelfDestructPSContext",PSContextToID(currentPSContext));
#endif (STAGE == DEVELOP)
    ContextYield(yield_terminate, 0);
}

public procedure PSMakeRunnable(PSContext context)
{
    context->scheduler->wannaRun = true;
}


static int pscontextid[2] = {PSCONTEXTID, 3049}; /* hack for old world */
static NSContextType PSEntry =
{
    /* next = */		NULL,
    /* ncids, *cid = */		2, pscontextid,	/* initialised with PostScript */
    /* createProc = */		NSCreatePS,
    /* termProc = */		NSTermPS,
    /* destroyProc = */		NSDestroyPS,
    /* loadProc = */		NSLoadPS,
    /* unloadProc = */		NULL,
    /* timeoutProc = */		NSTimeoutPS,
    /* notifyProc = */		NSNotifyPS,
    /* checkNotifyProc = */	NSCheckNotifyPS,
    /* current = */		(NSContext *)(&currentPSContext),
    /* usesIPCStms, filler = */	1, 0
};

public procedure InitPSContextType()
{
    NSAddContextType( &PSEntry );
}



