
/*
  dpsclient.h
    Application interface to the Display PostScript Library.

  This header file is intended to be identical across all
  implementations of the Display PostScript System. It defines types
  and procedures needed by DPS applications to access DPS. It does not
  include operations for creating new DPSContexts.

  Copyright (c) 1988 Adobe Systems Incorporated.
  All rights reserved.
  
*/

#ifndef DPSCLIENT_H
#define DPSCLIENT_H

#ifndef DPSFRIENDS_H
#include "dpsfriends.h"
#endif DPSFRIENDS_H

/*=== TYPES ===*/

/* Standard error codes (type DPSErrorCode)
 * These are intended to be used with dpsexcept.h
 * System-specific extensions can be made. Codes 1000 through 1099 are
 * reserved for use by Adobe.
 *
 * Codes 1100-1999 are reserved by NeXT.
 */

#define DPS_ERRORBASE		1000
#define DPS_NEXTERRORBASE	1100

typedef enum _DPSErrorCode {
	dps_err_ps = DPS_ERRORBASE,
	dps_err_nameTooLong,
	dps_err_resultTagCheck,
	dps_err_resultTypeCheck,
	dps_err_invalidContext,
	dps_err_select = DPS_NEXTERRORBASE,
	dps_err_connectionClosed,
	dps_err_read,
	dps_err_write,
	dps_err_invalidFD,
	dps_err_invalidTE,
	dps_err_invalidPort,
	dps_err_outOfMemory,
	dps_err_cantConnect
} DPSErrorCode;

  /* The above definitions specify the error codes passed to a DPSErrorProc:

       dps_err_ps identifies standard PostScript language interpreter
       errors. The arg1 argument to the errorProc is the address of the
       binary object sequence sent by the handleerror (or resynchandleerror)
       operator to report the error. The sequence has 1 object, which is an
       array of 4 objects. See the language extensions document for details
       on the contents of this sequence. arg2 is the number of bytes in the
       entire binary object sequence.

       dps_err_nameTooLong flags user names that are too long. 128 chars
       is the maximum length for PostScript language names. The name and its
       length are passed as arg1 and arg2 to the error proc.

       dps_err_resultTagCheck flags erroneous result tags, most likely
       due to erroneous explicit use of the printobject operator. The
       pointer to the binary object sequence and its length are passed
       as arg1 and arg2 to the error proc. There is one object in the
       sequence.

       dps_err_resultTypeCheck flags incompatible result types. A pointer
       to the offending binary object is passed as arg1; arg2 is unused.

       dps_err_invalidContext flags an invalid DPSContext argument. An
       attempt to send PostScript language code to a context that has
       terminated is the most likely cause of this error. arg1 to the
       DPSErrorProc is the context id (see the fork operator); arg2 is
       unused.

       dps_err_select, dps_err_read and dps_err_write signal communication
       errors in the connection.  The OS return code is passed in arg1;
       arg2 is unused.

       dps_err_connectionClosed indicates the connection to a windowserver
       went away.  arg1 and arg2 are unused.

       dps_err_invalidFD, dps_err_invalidTE and dps_err_invalidPort 
       indicate a illegal item of the given type was passed to a DPS
       routine.  The illegal item is passed in arg1; arg2 is unused.

       dps_err_outOfMemory indicates a storage allocation failure.  The size
       of the block requested is passed in arg1; arg2 is unused.

       dps_err_cantConnect signals an error during the creation of a
       connection to a window server.  The host that was being connected
       to is passed in arg1; arg2 is unused. 
  */

typedef void (*DPSTextProc)(
  DPSContext ctxt,
  const char *buf,
  long unsigned int count );

  /* Call-back procedure to handle text from the PostScript interpreter.
     'buf' contains 'count' bytes of ASCII text. */

typedef void (*DPSErrorProc)(
  DPSContext ctxt,
  DPSErrorCode errorCode,
  long unsigned int arg1, 
  long unsigned int arg2 );
  
  /* Call-back procedure to report errors from the PostScript interpreter.
     The meaning of arg1 and arg2 depend on 'errorCode', as described above.
     See DPSDefaultErrorProc, below.
   */


/*=== PROCEDURES ===*/

extern void DPSDefaultErrorProc(
  DPSContext ctxt,
  DPSErrorCode errorCode,
  long unsigned int arg1,
  long unsigned int arg2 );
  
  /* This is the default error proc that is used for both contexts'
     individual errors and the global error backstop.  It raises an
     exception using NX_RAISE.  For all DPS errors, the context is the
     first piece of data put in the NXHandler record.  The second
     data field is value passed to this routine in arg1.
     */

extern void DPSSetTextBackstop( DPSTextProc textProc );

  /* Call this to establish textProc as the handler for text output from
     DPSDefaultErrorProc or from contexts created by the 'fork' operator but
     not made known to the dps client library. NULL will be passed as the ctxt
     argument to textProc in the latter case. */

extern DPSTextProc DPSGetCurrentTextBackstop(void);

  /* Returns the textProc passed most recently to DPSSetTextBackstop, or NULL
     if none */

extern void DPSSetErrorBackstop( DPSErrorProc errorProc );

  /* Call this to establish errorProc as the handler for PostScript interpreter
     errors from contexts created by the 'fork' operator but not made
     known to the dps client library. NULL will be passed as the ctxt
     argument to errorProc. */

extern DPSErrorProc DPSGetCurrentErrorBackstop(void);

  /* Returns the errorProc passed most recently to DPSSetErrorBackstop, or NULL
     if none */

#define DPSWritePostScript(ctxt, buf, count)\
  (*(ctxt)->procs->WritePostScript)((ctxt), (buf), (count))

  /* Send as input to 'ctxt' 'count' bytes of PostScript language contained
     in 'buf'. The code may be in either of the 3 encodings for PostScript
     language programs: plain text, encoded tokens, or binary object sequence.
     If the form is encoded tokens or binary object sequence, an entire
     one of these must be sent (perhaps in a series of calls on
     DPSWritePostScript) before PostScript language in a different encoding
     can be sent.
     
     DPSWritePostScript may transform the PostScript language to a different
     encoding, depending on the characteristics established for 'ctxt' when
     it was created. For example, a context created to store its PostScript
     in an ASCII file would convert binary object sequences to ASCII.

     If 'ctxt' represents an invalid context, for example because
     the context has terminated in the server, the dps_err_invalidContext
     error will be reported via ctxt's error proc.
     
     If 'ctxt' is incapable of accepting more PostScript language, for example
     because it is backlogged with code that was sent earlier to be executed,
     this will block until transmission of 'count' bytes can be completed. */

extern void DPSPrintf( DPSContext ctxt, const char *fmt, ... );

  /* Write string 'fmt' to ctxt with the optional arguments converted,
     formatted and logically inserted into the string in a manner
     identical to the C library routine printf.

     If 'ctxt' represents an invalid context, for example because
     the context has terminated in the server, the dps_err_invalidContext
     error will be reported via ctxt's error proc.
     
     If 'ctxt' is incapable of accepting more PostScript language, for example
     because it is backlogged with code that was sent earlier to be executed,
     this will block until transmission of the string can be completed. */

#define DPSWriteData(ctxt, buf, count)\
  (*(ctxt)->procs->WriteData)((ctxt), (buf), (count))
  
  /* Write 'count' bytes of data from 'buf' to 'ctxt'. This will not
     change the data.

     If 'ctxt' represents an invalid context, for example because
     the context has terminated in the server, the dps_err_invalidContext
     error will be reported via ctxt's error proc.
     
     If 'ctxt' is incapable of accepting more PostScript language, for example
     because it is backlogged with code that was sent earlier to be executed,
     this will block until transmission of 'count' bytes can be completed. */
  
#define DPSFlushContext(ctxt) (*(ctxt)->procs->FlushContext)((ctxt))
  
  /* Force any buffered data to be sent to ctxt.

     If 'ctxt' represents an invalid context, for example because
     the context has terminated in the server, the dps_err_invalidContext
     error will be reported via ctxt's error proc.
     
     If 'ctxt' is incapable of accepting more PostScript language, for example
     because it is backlogged with code that was sent earlier to be executed,
     this will block until transmission of all buffered bytes can be completed.
     */
  
extern int DPSChainContext( DPSContext parent, DPSContext child );

  /* This links child and all of child's children onto parent's 'chainChild'
     list. The 'chainChild' list threads those contexts that automatically
     receive copies of any PostScript code sent to parent. A context may
     appear on only one such list.
     
     Normally, DPSChainContext returns 0. It returns -1 if child already
     appears on some other context's 'chainChild' list. */

extern void DPSUnchainContext( DPSContext ctxt );

  /* This unlinks ctxt from the chain that it is on, if any. It leaves
	ctxt->chainParent == ctxt->chainChild == NULL
   */

#define DPSResetContext(ctxt) (*(ctxt)->procs->ResetContext)((ctxt))
  
  /* This routine is unimplemented in the NeXT implementation of
     Display PostScript. */
  
#define DPSWaitContext(ctxt) (*(ctxt)->procs->WaitContext)(ctxt)

  /* Waits until the PostScript interpreter is ready for more input to
     this context.  This is useful for synchronizing an application
     with the DPS server.

     If 'ctxt' represents an invalid context, for example because
     the context has terminated in the server, the dps_err_invalidContext
     error will be reported via ctxt's error proc. */

#define DPSSetTextProc(ctxt, tp) ((ctxt)->textProc = (tp))

  /* Change ctxt's textProc. */
  
#define DPSSetErrorProc(ctxt, ep) ((ctxt)->errorProc = (ep))

  /* Change ctxt's errorProc. */
  
#define DPSInterruptContext(ctxt) (*(ctxt)->procs->Interrupt)((ctxt))

  /* This routine is unimplemented in the NeXT implementation of
     Display PostScript. */

#define DPSDestroyContext(ctxt) (*(ctxt)->procs->DestroyContext)((ctxt))

  /* This calls DPSUnchainContext, then (for an interpreter context) sends a
     request to the interpreter to terminate ctxt, then frees the storage
     referenced by ctxt. The termination request is ignored by the
     server if the context is invalid, for example if it has already
     terminated. */
  
#define DPSSpaceFromContext(ctxt) ((ctxt)->space)

  /* Extract space handle from context. */

#define DPSDestroySpace(spc) (*(spc)->procs->DestroySpace)((spc))

  /* Calls DPSDestroyContext for each of the contexts in the space, then
     sends a request to the server to terminate the space, then frees the
     storage referenced by spc. */

#ifndef DPSNEXT_H
#include "dpsNeXT.h"
#endif DPSNEXT_H

#endif DPSCLIENT_H

