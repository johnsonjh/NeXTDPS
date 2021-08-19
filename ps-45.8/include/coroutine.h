/*
  coroutine.h

Copyright (c) 1987, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Thu Sep 17 09:05:48 1987
Edit History:
Ed Taft: Tue May  3 12:54:58 1988
Ivor Durham: Thu Aug 11 08:45:50 1988
End Edit History.

Note: procedures in this interface require the caller to specify
an explicit maximum stack size for each context that is created.
This permits the interface to be implemented in environments that
do not support multiple stacks of dynamically varying size. However,
in environments that do support such a capability, the implementation
may ignore the stack size arguments, implement CallAsCoroutine as
a simple procedure call, etc.
*/

#ifndef	COROUTINE_H
#define	COROUTINE_H

#include PUBLICTYPES

/* Data Structures */

typedef struct _t_CoroutineRec *Coroutine;
/* Opaque designator for a coroutine execution context */


/* Exported Procedures */

extern Coroutine InitCoroutine(
  /* integer stackSize, extraStack; boolean check */);
/* Initializes the coroutine machinery and creates a coroutine context
   within which the caller will subsequently execute. Returns a designator
   for that coroutine and makes it be the current coroutine. Must be
   called exactly once, before any calls to the procedures below.
   stackSize is the maximum number of bytes of stack space that the caller
   will ever require (see CreateCoroutine). extraStack is an additional
   amount of stack space to be reserved for asynchronous events (e.g.,
   interrupts) whose processing makes use of the interrupted program's
   stack; this amount is added to the stackSize argument in all
   subsequent calls to CreateCoroutine. If check is true, it activates
   some runtime consistency checking and enables use of the MinStackUnused
   procedure. */

extern Coroutine CreateCoroutine(
  /* procedure (*proc)( Coroutine source; char *info );
     char *info; integer stackSize */);
/* Creates a new coroutine and returns a designator for it. When that
   coroutine is first invoked, it will call proc(source, info), where
   source is the CoReturn source coroutine (see below) and info is
   as specified in the call to CreateCoroutine. It is illegal
   for proc ever to return. info is an uninterpreted pointer argument
   that is simply passed through to proc (and is also available via
   GetCoroutineInfo); it enables higher-level information to be
   associated with the coroutine context. stackSize is the maximum number
   of bytes of stack space that the coroutine will ever require; if
   stackSize is exceeded, unpredictable behavior may ensue. The
   extraStack argument of InitCoroutine is added to stackSize in order
   to deal with asynchronous event processing; see InitCoroutine.
   CreateCoroutine returns zero if it is unable to create a new
   coroutine for any reason. */

extern DestroyCoroutine(/* Coroutine coroutine */);
/* Destroys the specified coroutine context, which must not be the one
   that is currently executing. */

extern Coroutine CoReturn(/* Coroutine destination */);
/* Passes control to the specified destination coroutine, passing it the
   current coroutine as the source. If the destination coroutine
   has never been executed, its top-level procedure is called with
   the source coroutine as an argument (see CreateCoroutine). Otherwise,
   the destination coroutine returns from its last call to CoReturn
   with the source coroutine as its return value. */

extern char *GetCoroutineInfo(/* Coroutine coroutine */);
/* Returns the uninterpreted "info" that was passed to CreateCoroutine
   when the specified coroutine was created. If "info" was a pointer
   to higher-level information associated with that coroutine,
   this permits anyone holding the coroutine handle to access
   that information. */

extern integer CallAsCoroutine(
  /* integer (*proc)(char *arg); char *arg; integer stackSize */);
/* Calls proc(arg) within the context of a new, temporary coroutine
   and returns the result returned by proc. stackSize is interpreted as
   described under CreateCoroutine. arg is an uninterpreted pointer that
   is simply passed to proc; it may be recast to any pointer type and
   used to communicate arbitrary arguments and/or results.
   The execution of proc must not do anything that would cause the
   context of proc to be left by a non-local goto (in other words,
   proc must not allow any exceptions to escape from its control).
   The temporary coroutine inherits the "info" parameter of the
   caller's coroutine. */

extern integer CurStackUnused();
/* Returns the amount of stack space unused in the current coroutine.
   This is in bytes and is only approximate. */

extern integer MinStackUnused(/* Coroutine coroutine */);
/* Returns the minimum amount of unused stack space during the lifetime
   of the specified coroutine. This procedure requires InitCoroutine
   to have been called with a "check" argument of true; otherwise it
   returns a meaningless result. */


/* Exported Data */

extern Coroutine currentCoroutine;
/* Designator for the coroutine that is currently executing */

#endif	COROUTINE_H
/* v002 durham Fri Feb 26 11:56:30 PST 1988 */
/* v003 sandman Thu Mar 10 09:34:45 PST 1988 */
/* v004 durham Fri Apr 8 11:34:34 PDT 1988 */
/* v006 durham Thu Aug 11 13:10:40 PDT 1988 */
/* v007 caro Wed Nov 9 15:13:52 PST 1988 */
/* v008 byer Tue May 16 11:14:27 PDT 1989 */
