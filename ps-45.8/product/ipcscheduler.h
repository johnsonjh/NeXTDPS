/*******************************************************************************
	ipcscheduler.h
	Definitions of the scheduler context
	for preserving PostScript lightweight processes.
	
	CONFIDENTIAL
	Copyright (c) 1988 NeXT, Inc. as an unpublished work.
	All Rights Reserved.

	Created 25Sep86 Leo
	
	Modified:
	
	18Apr88 Leo	No more current window; deleted old change log
	05Jul88 Leo	Revision for Adobe PSContext scheme
	31Aug88 Leo	Removed outNeedsFlush
	23Sep88 Leo	Updated comments!
	10Oct88 Leo	login context
	08Feb89	Dave	added typed contexts
	14Feb89	Leo	Revision of security scheme
	11Jun89 Leo	swapBytes, dispatching
	02Oct89	Dave	Added I/O pointers for typed contexts
	02Feb90	Dave	Added bit for printing contexts.
	02Apr90	Terry	Added 2 bytes for mono+color accuracies
	04Jun90 Dave	Removed Stm fields in PSSchedulerContext struct
	
*******************************************************************************/

#ifndef IPCSCHEDULER_H
#define IPCSCHEDULER_H

#import PACKAGE_SPECS
#import PUBLICTYPES
#import COROUTINE
#import POSTSCRIPT
#undef MONITOR
#import <mach.h>
#import <sys/types.h>
#import <sys/message.h>
#import "nscontexts.h"

/* PSSchedulerContext -- PostScript scheduler process */

/*
    PostScript contexts in the NeXT scheduler can be in one
    of four states:
    
    Running - Context is the current context.  It is processing
    a message just received from the scheduler and has recorded
    that message somewhere internally.  The context's input port
    is unrestricted.
    
    Awaiting input - Context is awaiting a message on its input
    port.  It has no active message.  It's input port is unrestricted.
    
    Awaiting notification of output - Context is awaiting a notify
    message that the message it just sent has been accepted.  The
    context still has an active message stored away internally.
    The context's port is restricted so no more messages will be
    received on it.
    
    Timed out - Context has consumed at least one unit of execution
    time, and is awaiting another chance to run.  It has an active
    message it has stored away.  Its port is restricted so no more
    messages will be received.
    
    These states are designated as follows in the structure:
    Running - not explicitly shown, implicitly known by 
    	currentPSContext == psc->context
    Awaiting input - waitMsg is set
    Awaiting notification - waitNotify is set, notifyPort indicates
    	port we are waiting for notify on
    Timed out - wannaRun is set
    
    Another concept is that of the login context.  This context
    is the one which is enabled to perform login functions: telling
    the window server to set its effective uid, etc.  There can
    only be one such context, which is designated by having its
    login bit set true.  Also, all contexts that should live past
    a logout are considered system contexts,
    and the appropriate bit is set; all others are user contexts,
    which can be terminated upon instruction from the login context.
    Each context can have a user (and group) associated with it or
    not; if it is not set, then the global user and group ids
    apply.
    
    */

struct _t_PSSchedulerContextRec {
	struct	_t_PSSchedulerContextRec *next; /* Linked list */
	Coroutine coroutine;	/* This context's coroutine */
	NSContextType *type;	/* the internal type code of this context */
	NSContext context;	/* Back pointer to the context */
	port_t	inputPort;	/* Port this context receives messages on */
	port_t	notifyPort;	/* Port this context waiting for notify for */
	port_t	remotePort;	/* Port at other end of connection: this
				   context should be killed if its
				   remotePort goes away */
	struct _SchedulerMsg **msgPtrP;
				/* Place to store pointer to a message
				   that comes in on inputPort */
	uid_t	uid;		/* uid, if userSet is true */
	gid_t	gid;		/* gid, if userSet is true */
	unsigned wannaRun : 1;	/* Set if this context should run */
	unsigned userInteraction : 1;	/* Set if this pscontext
					   interacted with the user the
					   last time it was run	*/
	unsigned killMe : 1;	/* true when Context is to be killed */
	unsigned waitMsg : 1;	/* true iff context is waiting for a
				   message on inputPort */
	unsigned waitNotify : 1;/* true iff this context is waiting for
				   a notify-to-write message for notifyPort */
	unsigned restricted : 1;/* Is my input port currently restricted? */
	unsigned login : 1;	/* Am I the login context? */
	unsigned system : 1;	/* Am I a system context? */
	unsigned userSet : 1;	/* Do I have a particular user above? */
	unsigned writeBlock : 1;/* Should writes ever block, or just fail? */
	unsigned objectFormat:3;/* Current object format for output */
	unsigned dispatching:1;	/* true iff this context is the events context
				   and that context is in the middle of working
				   its way through the event list */
	unsigned printerContext:1; /* true iff context is a printer context */
	unsigned writeProhibited:1; /* prohibit context from opening files
				       for writing ? */
	unsigned unused : 16;	/* Alignment */
	int defaultDepthLimit;	/* default depth limit for new windows */
};

extern PSSchedulerContext contextList, currentSchedulerContext;
extern uid_t globalUid;
extern gid_t globalGid;
PSSchedulerContext CreateTypedContext(PVoidProc, NSContext, NSContextType *);

#define contextUid(psc) ((psc) ? ((psc)->userSet ? (psc)->uid : globalUid) : 0)
#define contextGid(psc) ((psc) ? ((psc)->userSet ? (psc)->gid : globalGid) : 1)

/* Macros for restricted and unrestricting inputPort with setting bit */

#define RestrictInputPort(psc) \
    if (!psc->restricted) \
    { \
	psc->restricted = 1; \
	port_restrict(task_self_,psc->inputPort); \
    }

#define UnrestrictInputPort(psc) \
    if (psc->restricted) \
    { \
	psc->restricted = 0; \
	port_unrestrict(task_self_,psc->inputPort); \
    }

#endif IPCSCHEDULER_H



