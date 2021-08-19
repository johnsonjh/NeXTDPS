/*
    dpsNeXT.h
    
    This file describes the interface to the DPS routines specific to the
    NeXT implementation of the DPS library.
    
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#ifndef DPSNEXT_H
#define DPSNEXT_H

#ifndef EVENT_H
#include "event.h"
#endif EVENT_H

#ifndef DPSCLIENT_H
#include "dpsclient.h"
#endif DPSCLIENT_H

#ifndef ERROR_H
#include <objc/error.h>
#endif ERROR_H

#import <stdio.h>
#import <sys/port.h>
#import <sys/message.h>
#import <streams/streams.h>
#import <zone.h>

/*=== CONSTANTS ===*/

#define NX_CLEAR	0	/* composite operators */
#define NX_COPY		1
#define NX_SOVER	2
#define NX_SIN		3
#define NX_SOUT		4
#define NX_SATOP	5
#define NX_DOVER	6
#define NX_DIN		7
#define NX_DOUT		8
#define NX_DATOP	9
#define NX_XOR		10
#define NX_PLUSD	11
#define NX_HIGHLIGHT	12
#define NX_PLUSL	13		/* not implemented for 1.0 */
#define NX_PLUS		NX_PLUSD	/* for back compatibility. NX_PLUSD
					 * is preferred */

#define NX_DATA		1	/* special values for alpha */
#define NX_ONES		2

#define NX_RETAINED	0	/* A complete offscreen buffer holds */
				/* the obscured parts of the window */
#define NX_NONRETAINED	1	/* Only the screen parts exist */
#define NX_BUFFERED	2	/* Complete offscreen copy exists, */
				/* holds entire contents */

#define	NX_ABOVE	1	/* Place window above reference window */
#define	NX_BELOW	(-1)	/* Place window below reference window */
#define	NX_OUT		0	/* Remove it from screen and window List */

#define NX_FOREVER	(6307200000.0)	/* 200 years of seconds */

#define DPS_ALLCONTEXTS	((DPSContext)-1) /* refers to all existing contexts */


typedef enum _DPSNumberFormat {
    dps_float = 48,
    dps_long = 0,
    dps_short = 32
} DPSNumberFormat;
  /* Constants for DPSDoUserPath describing what type of coordinates are
     being used.  Other legal values are:

     For 32-bit fixed point numbers, use dps_long plus the number of bits
     in the fractional part.

     For 16-bit fixed point numbers, use dps_short plus the number of bits
     in the fractional part.
  */

typedef enum _DPSUserPathOp {
    dps_setbbox = 0,
    dps_moveto,
    dps_rmoveto,
    dps_lineto,
    dps_rlineto,
    dps_curveto,
    dps_rcurveto,
    dps_arc,
    dps_arcn,
    dps_arct,
    dps_closepath,
    dps_ucache
} DPSUserPathOp;
  /* Constants for constructing operator array parameter of DPSDoUserPath. */

typedef enum _DPSUserPathAction {
    dps_uappend = 176,
    dps_ufill = 179,
    dps_ueofill = 178,
    dps_ustroke = 183,
    dps_ustrokepath = 364,
    dps_inufill = 93,
    dps_inueofill = 92,
    dps_inustroke = 312,
    dps_def = 51,
    dps_put = 120
} DPSUserPathAction;
  /* Constants for the action of DPSDoUserPath.  In addition to these, any
     other system name index may be used.
   */


/*=== TYPES ===*/

typedef int (*DPSEventFilterFunc)( NXEvent *ev );
  /* Callback proc for filtering events of a context.  It is passed the
     event just read from the context before it is put in the global
     event queue.  If the proc returns TRUE, the event will be inserted
     into the queue as usual, otherwise it not put in the queue.
  */

typedef void (*DPSPortProc)( msg_header_t *msg, void *userData );
  /* Callback proc for ports registered by DPSAddPort. */

typedef struct __DPSTimedEntry *DPSTimedEntry;

typedef void (*DPSTimedEntryProc)(
    DPSTimedEntry te,
    double now,
    void *userData );
  /* Callback proc for timed entries registered by DPSAddTimedEntry. */

typedef void (*DPSFDProc)( int fd, void *userData );
  /* Callback proc for fds registered by DPSAddFD. */


/*=== PROCEDURES ===*/

extern DPSContext DPSCreateContext(
    const char *hostName,
    const char *serverName,
    DPSTextProc textProc,
    DPSErrorProc errorProc );
  /* Creates a connection to the window server with default timeout. */

extern DPSContext DPSCreateContextWithTimeoutFromZone(
    const char *hostName,
    const char *serverName,
    DPSTextProc textProc,
    DPSErrorProc errorProc,
    int timeout,
    NXZone *zone );
  /* Creates a connection to the window server with specified ms timeout. */

extern DPSContext DPSCreateStreamContext(
    NXStream *st,
    int debugging,
    DPSProgramEncoding progEnc,
    DPSNameEncoding nameEnc,
    DPSErrorProc errorProc );
  /* Creates a context that writes to a NXStream. */

#define DPSFlush()	DPSFlushContext(DPSGetCurrentContext())
  /* Flushes the current connection */

extern int DPSSetTracking( int flag );
  /* Enables or disables the coalescing of mouse events. */

extern void DPSStartWaitCursorTimer(void);
  /* Starts the wait cursor timeout.  To be used before a time-consuming
     operations that is NOT initiated by a user event.
   */

extern void DPSAddPort(
    port_t newPort,
    DPSPortProc handler,
    int maxSize,
    void *userData,
    int priority );
  /* Adds a MACH port to be listened to. */

extern void DPSRemovePort( port_t port );
  /* Removes a MACH port previously added. */

extern DPSTimedEntry DPSAddTimedEntry(
    double period,
    DPSTimedEntryProc handler,
    void *userData,
    int priority );
  /* Creates a timed entry. */

extern void DPSRemoveTimedEntry( DPSTimedEntry te );
  /* Destroys a timed entry. */

extern void DPSAddFD(
    int fd,
    DPSFDProc routine,
    void *data,
    int priority );
  /* Adds a file descriptor to be listened to. */

extern void DPSRemoveFD( int fd );
  /* Removes a file descriptor previously added. */

extern void DPSSetDeadKeysEnabled(DPSContext ctxt, int flag);
  /* Enables and disabled dead key processing for a context's events. */

extern DPSEventFilterFunc DPSSetEventFunc(
    DPSContext ctxt,
    DPSEventFilterFunc func );
  /* Installs a function to filter events from a given context. */

extern int _DPSGetOrPeekEvent( DPSContext ctxt, NXEvent *eventStorage,
			int mask, double wait, int threshold, int peek );
#define DPSGetEvent( ctxt, evPtr, mask, timeout, thresh )	\
	_DPSGetOrPeekEvent( (ctxt), (evPtr), (mask), (timeout), (thresh), 0 )
  /* Finds a matching event, removing it from the queue. */

#define DPSPeekEvent( ctxt, evPtr, mask, timeout, thresh )	\
	_DPSGetOrPeekEvent( (ctxt), (evPtr), (mask), (timeout), (thresh), 1 )
  /* Finds a matching event, but does not remove it from the queue. */

extern int DPSPostEvent( NXEvent *event, int atStart );
  /* Posts an event to the front or back of the client side event queue. */

extern void DPSDiscardEvents( DPSContext ctxt, int mask );
  /* Removes matching events from the event queue.  DPS_ALLCONTEXTS can
     be used as the first argument to match to all contexts.
   */

extern int DPSTraceContext( DPSContext ctxt, int flag );
  /* Turns on and off debugging tracing of a context's input and output.
     DPS_ALLCONTEXTS can be used as the first argument to match to
     all contexts.
   */

void DPSTraceEvents(DPSContext ctxt, int flag);
  /* Turns on and off debugging tracing of the events a context receives.
     DPS_ALLCONTEXTS can be used as the first argument to match to
     all contexts.
   */

extern void DPSPrintError( FILE *fp, const DPSBinObjSeq error );
  /* Prints out a binary encoded error to the fp. */

extern void DPSPrintErrorToStream( NXStream *st, const DPSBinObjSeq error );
  /* Prints our a binary encoded error to the stream. */

extern const char *DPSNameFromTypeAndIndex( short type, int index );
  /* This routine returns the text for the user name with the given index of
     the given type.  Type 0 is for usernames, and type -1 is for systems
     names.  The string returned is owned by the library (treat it as readonly).
   */

extern int DPSDefineUserObject( int index );
  /* Maps a PostScript object to a user object index.  If index is 0, a new
     userobject index is allocated;  otherwise the supplied index is used.
     In either case, the new index for the object is returned.  This index
     can be passed to a pswrap generated function taking a "userobject"
     parameter.  This routine should be called with the object that is
     to be indexed on the top of the operand stack.
  */

extern void DPSUndefineUserObject( int index );
  /* Unmaps a previously created user object. */

extern void DPSDoUserPath(  
    void *coords,
    int numCoords,
    DPSNumberFormat numType,
    char *ops,
    int numOps,
    void *bbox,
    int action);
  /* Sends a user path to the window server and one other operator.  See DPS
     extensions documentation on encoded user paths and homogeneous number
     arrays.  Coords is an array of numbers, which combined with the bbox,
     will form the first strings of the encoded user path.  numCoords is the
     number of elements in the coords array.  numType describes the type
     of numbers in the coords array (see the DPSNumberFormat enum).  ops
     is a character array of the operands to be applied to the coords in
     building the user path (see the DPSUserPathOp enum).  This becomes
     the second string of the encoded user path.  "dps_setbbox" will
     be inserted if needed into this list when it is sent to the window
     server.  numOps is the size of the ops array you pass.  bbox
     is an array of numbers of the same type as described by numType that
     holds the minimum x, minimum y, maximum x, and maximum y values of the
     user path.  This data is prepended to the coords array to comprise the
     first string in the encoded user path.  action is the system named index
     of an operator to be executed immediately after the userpath is places
     on the stack (See DPSUserPathAction).
  */

extern void DPSDoUserPathWithMatrix(  
    void *coords,
    int numCoords,
    DPSNumberFormat numType,
    char *ops,
    int numOps,
    void *bbox,
    int action,
    float matrix[6]);
  /* Same as DPSDoUserPath except it takes an additional matrix argument which
     represents the optional matrix argument used by the ustroke, inustroke and
     ustrokepath operators.  If matrix is NULL, it is ignored.
   */

#endif DPSNEXT_H



