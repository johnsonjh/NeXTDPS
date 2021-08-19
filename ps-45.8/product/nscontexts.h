/*****************************************************************************
    nscontexts.h
	Definitions for generic scheduler contexts, to be type-cast
	by the various graphics packages.
	
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 10Feb89 Dave
	
    Modified:
	2Oct89	Dave	Added macros for getting stream info of a context
	04Jun90 Dave	Removed vestigial Stream macros, added notification proc.
	
******************************************************************************/

#ifndef NSCONTEXTS_H
#define NSCONTEXTS_H

#ifndef POSTSCRIPT_H
typedef enum {
  yield_by_request,   /* externally requested to give up control */
  yield_time_limit,   /* exhausted time limit set by PSSetTimeLimit */
  yield_terminate,    /* self-destruct: standard I/O streams have been
                         closed and context is ready for DestroyPSContext */
  yield_wait,         /* awaiting a condition. */
  /* Note: the following 3 are never used in calls of Yield from the
     PS Kernel; they are defined for the convenience of the client's
     stream implementation, which will surely need them. */
  yield_stdin,        /* standard input stream needs to be refilled */
  yield_stdout,       /* standard output stream needs a buffer */
  yield_fflush,       /* standard output stream buffer has been flushed */
  yield_other         /* needs attention for other reason (client-defined) */
  } YieldReason;
/* Reason passed to PSYield */

/* blind typedef for graphics packages */
typedef struct _t_PSSchedulerContextRec *PSSchedulerContext;
#endif

typedef void	*NSContext;

/* an entry in the contextTable for a particular type of contexts */
typedef struct _NSContextType
{
    struct _NSContextType *next;	/* the next one in the list */
    int		ncids;			/* the number of ids for this type */
    int		*cid;			/* the external ID */
    PSSchedulerContext	(*createProc)(); /* create a new context */
    void	(*termProc)();		/* terminate this context type */
    void	(*destroyProc)();	/* really blow the context away */
    PSSchedulerContext (*loadProc)(); 	/* switch in contexts of this
					     type - returns what got really
					     switch in */
    void	(*unloadProc)();	/* switch out contexts of this type */
    void	(*timeoutProc)();	/* called to set timeslice size */
    void	(*notifyProc)();	/* notification message received */
    void	(*checkNotifyProc)();	/* did we get notified? */
    NSContext	*current;		/* the current context of this type */
    unsigned	usesIPCStms : 1;	/* does it have IPC streams? */
    unsigned	filler : 31;
} NSContextType;


/* macros for context table access */

/*
 *  Create a new context of type t.  This procedure must be filled in.
 */
#define NSCreateContext( t, msg )	(*((t)->createProc))( (msg), (t) )

/*
 *  Terminate a context of type t.  Can be called synchronously
 *  from within a context (or asynchronously from somewhere else).
 *  Causes the context to be destroyed next time it wakes up.
 *  This procedure must be filled in.
 */
void NSTermContext( NSContextType *t, PSSchedulerContext psc );

/*
 *  Destroy a context of type t.  Cannot (generally) be called from
 *  within a running context.  This gets called from the scheduler, when
 *  the current context is the one to be destroyed (!).  This
 *  procedure nust be filled in.
 */
void NSDestroyContext( NSContextType *t, PSSchedulerContext psc );

/*
 *  Load in the context of type t.  Really the same as a context switch
 *  to this context.  The scheduler will cause the old context to be
 *  unloaded, then call this macro to load the new context.  This
 *  procedure can be NULL.
 */
#define NSLoadContext( t, psc )	((t)->loadProc) ? \
				(*((t)->loadProc))( (psc), (psc)->context ) : NULL
					    
/*
 *  Unload the context of type t.  Really the same as a context switch
 *  from this context.  The scheduler will call this macro to unload
 *  the old context then call load the new context.  This procedure can
 *  be NULL.
 */
#define NSUnloadContext( t, psc )	if ((t)->unloadProc) \
					    (*((t)->unloadProc))( (psc)->context )

/*
 *  Set the new timeout for the context that is about to run.  The
 *  scheduler will tell the new context how much time it is allowed
 *  to run for before it has to yield on timeout.  This procedure can
 *  be NULL.
 */
#define NSTimeoutContext( t, psc, to )	if ((t)->timeoutProc) \
					(*((t)->timeoutProc))( (psc)->context,(to) )

/*
 *  Tell the context it has just received a notification message.  This procedure
 *  can be NULL.
 */
#define NSNotifyContext( t, psc )	if ((t)->notifyProc) \
					    (*((t)->notifyProc))( (psc)->context )

/*
 *  Return a pointer to the scheduler structure that corelates to
 *  the current context of type t.  This pointer must be filled in.
 */
#define NSCurrentContext( t )		(*((t)->current))

/*
 *  Check for notification messages.  This procedure can be NULL.
 */
#define NSCheckNotifyContext( t, psc )	if ((t)->checkNotifyProc) \
					    (*((t)->checkNotifyProc))( (psc)->context )

/*
 *  A boolean value in the context table: true if the context of type
 *  t uses IPC streams, false otherwise.  Must be set to true or false.
 */
#define NSUsesIPCStms( t )		((t)->usesIPCStms)

NSContextType	*NSLoadContextType( int );
NSContextType	*NSGetContextType( int );

#endif NSCONTEXTS_H


