/*
  postscript.h

Copyright (c) 1984, '85, '86, '87, '88, '89, '90 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Mon Nov 23 16:13:27 1987
Edit History:
Ed Taft: Sat Jan  6 15:37:42 1990
Ivor Durham: Fri Nov 11 09:19:03 1988
Paul Rovner: Friday, April 29, 1988 9:33:58 AM
Perry Caro: Tue Nov 15 12:02:08 1988
Jim Sandman: Fri Nov  3 16:18:01 1989
Joe Pasqua: Thu Jun 29 14:20:18 1989
End Edit History.
*/

#ifndef    POSTSCRIPT_H
#define    POSTSCRIPT_H

#include PUBLICTYPES
#include DEVICETYPES
#include STREAM

/*
  One of the PostScript "glue" interfaces:
  
  This is an interface between the PostScript kernel and the
  PostScript scheduler.

  No exceptions are raised by any procedure in this interface.

 */

/* Data Structures */

typedef GenericID ContextID;
typedef GenericID SpaceID;

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

typedef enum {
  notify_request_yield, /* request to call PSYield(yield_by_request, NIL) */
  notify_terminate,     /* request to call PSYield(yield_terminate, NIL) */
  notify_interrupt,     /* generate "interrupt" error */
  notify_timeout        /* generate "timeout" error */
  } NotifyReason;
/* Reason passed to NotifyPSContext */

typedef struct _t_PSSchedulerContextRec *PSSchedulerContext;
/* Opaque designator for scheduler data associated with a context */

typedef struct _t_PSSpaceRec *PSSpace;
/* Opaque designator for a PostScript space */

typedef struct _t_PSKernelContextRec *PSKernelContext;
/* Opaque designator for a PostScript Kernel execution context */

typedef struct _t_PSContextRec {
  PSSpace space;          /* ... in which this context operates */
  PSKernelContext kernel;       /* -> PS Kernel context */
  PSSchedulerContext scheduler; /* -> scheduler context */
  Stm in, out;                  /* -> standard input and output streams;
                                   NIL if they have been closed */
  boolean notified;
  NotifyReason nReason;   /* valid if notified is true */
  } PSContextRec, *PSContext;
/* Public designator for a PostScript context */

typedef struct _t_PostScriptParameters {
  Stm errStm;
    /*
     * errStm is the standard error stream to be used for diagnostic output
     * from the interpreter, both during initialization and subsequently.
     */

  Stm vmStm;
    /*
     * vmStm is an input stream from which to read the initial VM.
     * NIL => build VM from scratch.
     */

  procedure (*customProc) ();
    /*
     * customProc is a procedure that will get called to register
     * product-specific operators. NIL means that there is none. See
     * customops.h.
     */

  procedure (*gStateExtProc) (/* char **ext; integer refDelta; */);
    /*
     * gStateExtProc is a procedure that allows the client to manage
     * extensions to the graphics state. It gets called whenever the 
     * reference count on the graphic state changes. refDelta is 1 if
     * count increases and -1 if it decreases. The extension is a char *
     * that is kept as part of the graphics state. ext is a pointer to
     * the extension. The priv field of the DevMarkInfo structure is
     * a pointer to the extension of the graphics state.
     */

  PCard32 realClock;
    /*
     * realClock is the address of a free-running 32-bit clock to be used in
     * keeping per-context execution time.  May be NIL to elect time-keeping by
     * "execution units". (See PSSetTimeLimit).
     */
     
  procedure (*writeVMProc)();
    /*
     * writeVMProc is a procedure that is specifies whether the client 
     * will be writing a new VM. Normally this parameter is NULL. If it 
     * is non-null, then prototype dictionaries are not copied into private
     * VM when a object is written into them but rather the prototype
     * itself is written. When initialization of the kernel is done and
     * the customProc procedure has been called, the writeVMProc will 
     * be called. When it returns, the initialization context will be
     * destroyed. It should register the makevm operator that will 
     * write the vm after it hase been augmented.
     */
     
} PostScriptParameters, *PPostScriptParameters;

/* Procedures exported by the PostScript kernel */

extern procedure InitPostScript(/* PPSParameters parameters*/);
/* Initializes the PostScript Kernel and interpreter.
   InitPostScript must be called once, before any of the procedures below.
   The caller must previously have initialized the PostScript scheduler. */

extern PSSpace CreatePSSpace();
/* Requests the PostScript Kernel to create a new PostScript space
   and return a handle to it. The new space will have no contexts
   initially.
   
   CreatePSSpace returns a handle for the new space, or NIL if one
   cannot be created due to resource constraints.
 */

extern procedure TerminatePSSpace(/* PSSpace space; */);
/* Called from the PostScript scheduler.

   TerminatePSSpace uses NotifyPSContext to cause all contexts in the
   space to terminate.
   
   TerminatePSSpace returns immediately. The caller should then schedule
   ready contexts of the space until there are none left. It can then call
   DestroyPSSpace.
   
   */

extern boolean DestroyPSSpace(/* PSSpace space; */);
/* Destroys the specified PostScript space if it has no contexts.
   Returns true upon success, false if the space has any contexts. */

extern PSContext CreatePSContext( /*
  PSSpace space;
  Stm in, out;
  PDevice device;
  char *startup; */ );
/* Requests the PostScript Kernel to create a new PSContext with the
   specified parameters. This supports CreateDPSContext in the client
   library.

   space is the PostScript space in which the new context will operate.
   in and out are the standard input and output streams.
   device represents the window-system-specific arguments given to
   CreateDPSContext.
   
   startup is a null-terminated C string containing a piece of PostScript
   language text that the PostScript interpreter is to execute. This
   string is expected to invoke the main function of the context,
   presumably by calling some procedure already present in the space, such
   as "start". Here is an example of the startup string you would pass
   for the procedure named "start" :
     "/start load stopped {/handleerror load stopped pop systemdict /quit get exec} if"
   
   The startup procedure may never terminate; if it does, the PS Kernel
   will call PSYield(yield_terminate, NIL). The startup string must remain
   intact for the lifetime of the context.

   CreatePSContext establishes the new PSContext and its associated
   data structures, including a PSKernelContext. The PostScript
   interpreter does not actually execute on behalf of the new PSContext.
   CreatePSContext returns a handle for the new PSContext, or NIL if one
   cannot be created due to resource constraints.

   The scheduler may subsequently pass control to the PSContext (i.e.,
   cause the PostScript interpreter to execute on its behalf) by executing
   something like the following:

     if (context == currentPSContext || SwitchPSContext(context))
       context->scheduler->coroutine = CoReturn(context->scheduler->coroutine);

   The PSContext will execute until it gives up control (by calling
   PSYield or one of the stream FilBuf or FlsBuf procedures,
   which may in turn call PSYield).

   (Note: the details of context and coroutine management are up to the
   scheduler; the approach outlined here is simply a suggestion.)
 */

extern procedure DestroyPSContext();
/* Called from the PostScript scheduler.
   Destroys the current PSContext and its associated PSKernelContext.
   The context to be destroyed must previously have invoked
   PSYield(yield_terminate, NIL), perhaps in response to a call on
   NotifyPSContext.
   
   When a PSContext is destroyed, the ContextID, PSContext and
   PSKernelContext handles become invalid; the standard input and output
   streams are closed; any locks held by the context are released; and
   the context's PostScript stacks are cleared, then reclaimed (the
   streams must be closed before DestroyPSContext is called). However,
   DestroyPSContext does not destroy the PSSchedulerContext; that is the
   scheduler's responsibility. */

extern boolean SwitchPSContext(/* PSContext context */);
/* Notifies the PS Kernel that the scheduler is about to pass control to
   the specified PSContext (by executing a CoReturn to its coroutine).
   The scheduler must call this before executing a CoReturn to a PSContext
   different from currentPSContext. SwitchPSContext returns true normally,
   false if switching PSContexts is not allowed because the current
   PSContext has exclusive use of the PostScript interpreter (i.e.,
   the save level in its space is greater than 0 and it is not the
   context in that space that did the save). */

extern PSContext ExclusivePSContext(/* PSSpace space */);
/* Returns the PSContext that has exclusive use of the PostScript
   interpreter, if any, in the given PSSpace; returns NIL otherwise. */

extern procedure NotifyPSContext(
  /* PSContext context; NotifyReason reason */);
/* Asynchronously notifies the specified PSContext of an external event
   or request. reason identifies the event.
   NotifyPSContext may be called at any time, say, from an interrupt
   routine or a separate process. It always returns immediately;
   processing of the event actually takes place next time the PS Kernel
   executes on behalf of the specified context (and may be deferred
   until the next convenient "clean" point).
   
   A notify_terminate reason over-rides any other unprocessed event
   notification for the context. A notify_interrupt reason over-rides
   notify_request_yield or notify_timeout. notify_request_yield and
   notify_timeout events are ignored if there is an unprocessed event
   notification for the context. */

extern procedure CheckForPSNotify();
/* Checks for arrival of an external event or request directed to the
   current PSContext; if one has arrived, causes it to be processed
   immediately. This procedure must be called from the
   PSYield procedure immediately upon being given control (i.e.,
   immediately after each CoReturn).

   CheckForPSNotify may never return (i.e., it may raise an exception
   that causes the call stack to be unwound), or it may invoke the
   PostScript interpreter recursively, possibly resulting in a
   reentrant PSYield or stream procedure call. Therefore, CheckForPSNotify
   must be called only at "clean" points where the caller's data
   structures are in a consistent state. */

extern procedure PSSetTimeLimit(/* Card32 limit */);
/* Establishes an execution time limit for the next time slice.
   If no real clock is available (NIL passed to InitPostScript for the clock
   address), time is measured in "execution units", where one unit is roughly
   equivalent to the execution time of a simple operator such as "add".
   Otherwise the limit must be a future value of the free-running clock.

   When the current PSContext reaches its specified time limit,
   PSYield(yield_time_limit, NIL) is invoked repeatedly until
   PSSetTimeLimit has been invoked to allocate a new time slice.
 */

/* The following procedures allow the glue to influence scheduling.
   The glue provides two pointers to the DPS kernel. The values
   referenced by these pointers are compared between execution
   of DPS operators. If the values are different, then PSYield
   is invoked causing the current context to yield. If the values
   are the same, no action is taken.
   
   These pointers can be used to indicate that some important event
   (external to DPS) has happened and that processing must be performed
   to handle this event in a timely manner (so DPS yields and allows
   that processing to occur).
*/

extern procedure PSSetYieldLocations(/* long int *loc1, *loc2 */);
/* Establishes loc1 and loc2 as the locations to compare to
   determine whether the interpreter should yield (see description
   above). A call to this procedure must be made before yield checking
   is enabled (see PSSetYieldChecking below). If either of the
   pointers is NIL, then yield checking can not be enabled.
*/

extern int PSSetYieldChecking(/* int on */);
/* Enables or disables yield checking. If yield checking is enabled
   (on == 1), the kernel compares the long ints at the locations
   specified in the call to PSSetYieldLocations (see above).  If the
   values are equal, the current context will Yield as if its time
   slice had elapsed. If yield checking is disabled (on == 0), then no
   checking is performed. Yield checking is disabled before the first
   call to PSSetYieldChecking.
   
   This function normally returns the previous value for yield
   checking. If the value of the parameter "on" is not 0 or 1, then -1
   is returned. If the value of the parameter "on" is 1, but no
   previous call to PSSetYieldLocations has been made, then -2 is
   returned.
*/

extern procedure PSSetDevice(/* PDevice device; boolean erase */);
/*
  Establish new device for the currentPSContext. If erase is true,
  performs an erasepage operator.
 */

extern PDevice PSGetDevice();
/*
  Returns current device for currentPSContext, which must not be NIL.
 */

extern PSContext IDToPSContext(/* ContextID id */);
/* Returns the PSContext that has the given id, or NIL if id is invalid */

extern PSSpace IDToPSSpace(/* SpaceID id */);
/* Returns the PSSpace that has the given id, or NIL if id is invalid */

extern ContextID PSContextToID(/* PSContext context */);
/* Returns the ContextID for context */

extern SpaceID PSSpaceToID(/* PSSpace space */);
/* Returns the SpaceID for space */

/* Procedures imported from the PostScript scheduler by the kernel */

extern procedure PSYield(/* YieldReason reason, char *info */);
/* Suspends execution of the current PSContext and returns to the
   scheduler; see YieldReason above. When called by the PostScript
   Kernel, info is always NIL. The implementation of scheduler-provided
   stream procedures (e.g., FilBuf, FlsBuf) may also call Yield; they
   may use info to pass a pointer to implementation specific information.
 */

extern procedure PSMakeRunnable(/* PSContext context */);
/* Ensures that context is on the ready list. */

extern boolean PSNewContextStms(/* Stm *in, *out */);
/* Creates a new pair of streams for a new context. Returns true normally,
   false if it can't create the streams for some reason. */

extern PSSchedulerContext
  CreateSchedulerContext( /* procedure (*proc)(), PSContext context */ );
/*
  Called when a new context is created, e.g. on behalf of the
  PostScript fork operator, to construct a new scheduler context for
  it. When the new context is first run, it begins with a call on
  proc(). It is illegal for proc ever to return.  CreateSchedulerContext
  returns NIL if it is unable to create a new scheduler context for any
  reason.  The context parameter is the fully initialized PSContext.
*/

/* Exported Data */

extern PSContext currentPSContext;
/* The PSContext that the PostScript interpreter is or was most recently
   executing. This is changed only by calling SwitchPSContext.
   If this variable is NIL, no PSContext has yet run or the most recent
   PSContext has been destroyed. */

#endif    POSTSCRIPT_H
/* v002 durham Fri Apr 8 11:36:01 PDT 1988 */
/* v003 durham Wed Jun 15 14:23:00 PDT 1988 */
/* v004 durham Thu Aug 11 13:11:17 PDT 1988 */
/* v005 caro Wed Nov 9 17:36:33 PST 1988 */
/* v006 pasqua Wed Dec 14 17:50:07 PST 1988 */
/* v007 pasqua Mon Feb 6 13:21:01 PST 1989 */
/* v008 bilodeau Fri Apr 21 16:10:36 PDT 1989 */
/* v009 byer Tue May 16 17:21:28 PDT 1989 */
/* v010 pasqua Mon Jul 10 18:26:08 PDT 1989 */
/* v010 francis Thu Jul 13 17:55:57 PDT 1989 */
/* v010 sandman Mon Oct 16 15:19:46 PDT 1989 */
/* v011 taft Thu Nov 23 15:02:56 PST 1989 */
/* v012 taft Fri Jan 5 15:35:32 PST 1990 */
