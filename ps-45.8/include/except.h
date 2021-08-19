/*
  except.h      -- exception handling

	Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
			All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is stricly forbidden unless prior written
permission is obtained from Adobe.

      PostScript is a registered trademark of Adobe Systems Incorporated.
	Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Jeffrey Mogul, Stanford, 18 February 1983
Edit History:
Ed Taft: Tue Nov 28 17:45:12 1989
Ivor Durham: Mon Aug 22 17:35:54 1988
End Edit History.
*/

#ifndef EXCEPT_H
#define EXCEPT_H

#include ENVIRONMENT
#include PUBLICTYPES

/* If the macro setjmp_h is defined, it is the #include path to be used
   instead of <setjmp.h>
 */
#ifdef setjmp_h
#include setjmp_h
#else setjmp_h
#ifdef VAXC
#include setjmp
#else VAXC
#include <setjmp.h>
#endif VAXC
#endif setjmp_h

/* 
  This interface defines a machine-independent exception handling
  facility. It depends only on the availability of setjmp and longjmp.
  Note that we depend on native setjmp and longjmp facilities; it's
  not possible to interpose a veneer due to the way setjmp works.
  (In fact, in ANSI C, setjmp is a macro, not a procedure.)

  The exception handler is largely defined by a collection of macros
  that provide some additional "syntax". A stretch of code that needs
  to handle exceptions is written thus:

    DURING
      statement1;
      statement2;
      ...
    HANDLER
      statement3
      statement4;
      ...
    END_HANDLER

  The meaning of this is as follows. Normally, the statements between
  DURING and HANDLER are executed. If no exception occurs, the statements
  between HANDLER and END_HANDLER are bypassed; execution resumes at the
  statement after END_HANDLER.

  If an exception is raised while executing the statements between
  DURING and HANDLER (including any procedure called from those statements),
  execution of those statements is aborted and control passes to the
  statements between HANDLER and END_HANDLER. These statements
  comprise the "exception handler" for exceptions occurring between
  the DURING and the HANDLER.

  The exception handler has available to it two local variables,
  Exception.Code and Exception.Message, which are the values passed
  to the call to RAISE that generated the exception (see below).
  These variables have valid contents only between HANDLER and
  END_HANDLER.

  If the exception handler simply falls off the end (or returns, or
  whatever), propagation of the exception ceases. However, if the
  exception handler executes RERAISE, the exception is propagated to
  the next outer dynamically enclosing occurrence of DURING ... HANDLER.

  There are two usual reasons for wanting to handle exceptions:

  1. To clean up any local state (e.g., deallocate dynamically allocated
     storage), then allow the exception to propagate further. In this
     case, the handler should perform its cleanup, then execute RERAISE.

  2. To recover from certain exceptions that might occur, then continue
     normal execution. In this case, the handler should perform a
     "switch" on Exception.Code to determine whether the exception
     is one it is prepared to recover from. If so, it should perform
     the recovery and just fall through; if not, it should execute
     RERAISE to propagate the exception to a higher-level handler.

  Note that in an environment with multiple contexts (i.e., multiple
  coroutines), each context has its own stack of nested exception
  handlers. An exception is raised within the currently executing
  context and transfers control to a handler in the same context; the
  exception does not propagate across context boundaries.

  CAUTIONS:

  It is ILLEGAL to execute a statement between DURING and HANDLER
  that would transfer control outside of those statements. In particular,
  "return" is illegal (an unspecified malfunction will occur).
  To perform a "return", execute E_RTRN_VOID; to perform a "return(x)",
  execute E_RETURN(x). This restriction does not apply to the statements
  between HANDLER and END_HANDLER.

  It is ILLEGAL to execute E_RTRN_VOID or E_RETURN between HANDLER and
  END_HANDLER.

  The state of local variables at the time the HANDLER is invoked may
  be unpredictable. In particular, a local variable that is modified
  between DURING and HANDLER has an unpredictable value between
  HANDLER and END_HANDLER. However, a local variable that is set before
  DURING and not modified between DURING and HANDLER has a predictable
  value.
 */


/* Data structures */

typedef struct _t_Exc_buf {
	struct _t_Exc_buf *Prev;	/* exception chain back-pointer */
	jmp_buf Environ;		/* saved environment */
	char *Message;			/* Human-readable cause */
	int Code;			/* Exception code */
} _Exc_Buf;

extern _Exc_Buf *_Exc_Header;	/* global exception chain header */


/* Macros defining the exception handler "syntax":
     DURING statements HANDLER statements END_HANDLER
   (see the description above)
 */

#define	_E_RESTORE	_Exc_Header = Exception.Prev

#define	DURING {_Exc_Buf Exception;\
		 Exception.Prev=_Exc_Header;\
		 _Exc_Header= &Exception;\
		 if (! setjmp(Exception.Environ)) {

#define	HANDLER	_E_RESTORE;} else {

#define	END_HANDLER }}

#define	E_RETURN(x) {_E_RESTORE; return(x);}

#define	E_RTRN_VOID {_E_RESTORE; return;}


/* Exported Procedures */

extern procedure os_raise(/* int Code, char *Message */);
/* Initiates an exception; always called via the RAISE macro.
   This procedure never returns; instead, the stack is unwound and
   control passes to the beginning of the exception handler statements
   for the innermost dynamically enclosing occurrence of
   DURING ... HANDLER. The Code and Message arguments are passed to
   that handler as described above.

   It is legal to call os_raise from within a "signal" handler for a
   synchronous event such as floating point overflow. It is not reasonable
   to do so from within a "signal" handler for an asynchronous event
   (e.g., interrupt), since the exception handler's data structures
   are not updated atomically with respect to asynchronous events.

   If there is no dynamically enclosing exception handler, os_raise
   writes an error message to os_stderr and aborts with CantHappen.
 */

extern procedure CantHappen();
/* Writes an error message to os_stderr and aborts the PostScript
   interpreter. This is used only to handle "impossible" errors;
   there is no recovery, and CantHappen does not return.
 */

extern procedure os_exit(/* int code */);
/* Terminates execution of the entire PostScript interpreter.
   The interpreter cannot be resumed; its data structures (including
   any PostScript contexts that may be in existence) are in an
   undefined state. By convention, a code of zero indicates normal
   termination; nonzero indicates abnormal termination.

   Note that os_exit is not used for normal termination of a single
   PostScript context; see postscript.h for that.
 */

extern procedure os_abort();
/* Similar to os_exit, but indicates that an "impossible" condition
   has been detected (e.g., CantHappen or an unhandled exception).
   In a development configuration, this may cause additional
   debugging information (e.g., a memory dump) to be collected.
 */


/* The following two operations are invoked only from the exception
   handler macros and from the os_raise procedure. Note that they are
   not actually declared here but in <setjmp.h> (or a substitute
   specified by the macro SETJMP, above).

   Note that the exception handler facility uses setjmp/longjmp in
   a stylized way that may not require the full generality of the
   standard setjmp/longjmp mechanism. In particular, setjmp is never
   called during execution of a signal handler; thus, the signal
   mask saved by setjmp can be constant rather than obtained from
   the operating system on each call. This can have considerable
   performance consequences.
 */

#if false
extern int setjmp(/* jmp_buf buf */);
/* Saves the caller's environment in the buffer (which is an array
   type and therefore passed by pointer), then returns the value zero.
   It may return again if longjmp is executed subsequently (see below).
 */

extern procedure longjmp(/* jmp_buf buf, int value */);
/* Restores the environment saved by an earlier call to setjmp,
   unwinding the stack and causing setjmp to return again with
   value as its return value (which must be non-zero).
   The procedure that called setjmp must not have returned or
   otherwise terminated. The saved environment must be at an earlier
   place on the same stack as the call to longjmp (in other words,
   it must not be in a different coroutine). It is legal to call
   longjmp in a signal handler and to restore an environment
   outside the signal handler; longjmp must be able to unwind
   the signal cleanly in such a case.
 */
#endif false

/* In-line Procedures */

#define RAISE os_raise
/* See os_raise above; defined as a macro simply for consistency */

#define	RERAISE	RAISE(Exception.Code, Exception.Message)
/* Called from within an exception handler (between HANDLER and
   END_HANDLER), propagates the same exception to the next outer
   dynamically enclosing exception handler; does not return.
 */

#define Assert(condition) \
  if (! (condition)) CantHappen();
/* Tests whether the specified condition is true and aborts the
   entire system if not.
 */

#if STAGE==DEVELOP
#define DebugAssert(condition) \
  if (! (condition)) CantHappen();
#else STAGE==DEVELOP
#define DebugAssert(condition)
#endif STAGE==DEVELOP
/* Same as assert, but the test is made only if the code is compiled
   in the development configuration (which is never the case for
   those packages not compiled with distinct STAGEs).
 */


/*
  Error code enumerations

  By convention, error codes are divided into blocks belonging to
  different components of the PostScript system. Some are defined
  here, others in interfaces for specific components.

  Negative error codes are reserved for use within the PostScript kernel
  (defined in error.h); they are never visible at external interfaces.
 */

#define ecFileSystemBase 0
/* Error codes 0-255 are filesystem related (defined in filetypes.h) */

#define ecDeviceBase 256
/* Error codes 256-511 are device related (defined in device.h) */

#define ecPostScript 512
/* Error codes 512-767 correspond to PostScript language level errors.
   These errors are handled by the PostScript interpreter; the code
   selects the error that the interpreter is to invoke. Only a subset
   of all possible PostScript errors are enumerated here, namely those
   that can reasonably be raised from Device or OS related procedures.
 */
#define ecInvalidAccess (ecPostScript+0)	/* invalidaccess */
#define ecInvalidFileAccess (ecPostScript+1)	/* invalidfileaccess */
#define ecIOError (ecPostScript+2)		/* ioerror */
#define ecLimitCheck (ecPostScript+3)		/* limitcheck */
#define ecRangeCheck (ecPostScript+4)		/* rangecheck */
#define ecUndef (ecPostScript+5)		/* undefined */
#define ecUndefFileName (ecPostScript+6)	/* undefinedfilename */
#define ecUndefResult (ecPostScript+7)		/* undefinedresult */
#define	ecInvalidID (ecPostScript+8)		/* invalidid */


#endif EXCEPT_H
