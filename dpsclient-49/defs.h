
/*
    defs.h

    This file has defines private to this directory.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <cthreads.h>		/* ??? a pig to include */
#include <sys/message.h>
#include <streams/streams.h>

/* message types in server protocol */
#define PS_DATA_MSG_ID 3049
#define PS_EOF_MSG_ID 3050
#define PS_DATA_PING_MSG_ID 3051
#define PS_DATA_PING_RETURN_MSG_ID 3052
#define PS_NEXTIMAGE_MSG_ID 3053
#define PS_EV_DATA_MSG_ID 3054
#define PS_EV_PING_DATA_MSG_ID 3055
#define PS_EV_PING_DATA_RETURN_MSG_ID 3056

typedef struct __DPSQueueElement {
    struct __DPSQueueElement *next;	/* MUST BE FIRST */
    NXEvent ev;				/* the event itself */
} _DPSQueueElement;


typedef struct __DPSQueue {
    _DPSQueueElement *firstOut;	/* ptr to next Q element to be given out */
    _DPSQueueElement *lastIn;	/* ptr to last Q element added to Q */
} _DPSQueue;


typedef struct __DPSListItem {	/* a linked list element */
    struct __DPSListItem *next;		/* ptr to next element of list */
    int key;				/* key that can be searched for */
    short refs;		/* number of people up the stack ref'ing item */
    char doFree;			/* Does this item need freeing? */
} _DPSListItem;

typedef struct __DPSList {	/* a linked list */
    struct __DPSListItem *head;		/* ptr to first element of list */
    struct __DPSListItem *free;		/* list of free elements */
} _DPSList;


#define PS_MSG_SIZE	(1000-sizeof(msg_header_t)-sizeof(msg_type_long_t))
typedef struct __DPSMessage {	/* a message from PS */
    msg_header_t header;
    msg_type_long_t type;
    char data[PS_MSG_SIZE];
} _DPSMessage;


/* The NeXT client library internally has a number of different types of
   context "classes", who inherit from each other like:
   
   			DPSContext
			     |
			_DPSOutputContext
			|		|
		_DPSStreamContext   _DPSServerContext
		 				|
					_DPSMachContext

    A DPSContext is the only thing ever exported out of this library.  A
    _DPSOutputContext knows how to output PostScript to an NXStream.  All
    such contexts are always kept in a linked list called _DPSOutputContexts.
    A _DPSStreamContext is one created by the user to talk to a particular
    NXStream.  A _DPSServerContext is any context that talks to a window
    server (not implemented).  A _DPSMachContext talks to a window server
    via Mach IPC.
    
    In order to share code for list management, subclasses sometimes add
    instance variables to the context struct at the start AND end of the
    structure.  Because of this, no one should ever convert from one context
    to another with a simple cast, but should -always- go through a macro of
    the form FROMTYPE_TO_TOTYPE.  (For this naming, DPSContext's "type name"
    is "STD").
 */

/* size of ascii-ization buffer; enough for a 50 token wrap */
#define ASCII_BUF_SIZE	 404	

/* The following structure elements should be the first ones in any context
   that will use streams for output.
 */
#define OUTPUT_CONTEXT_ELEMENTS						\
  /* ptr to next element of global context list */			\
    struct __DPSOutputContext *nextOC;					\
  /* key that can be searched for - the stream itself */		\
    NXStream *outStream;						\
  /* the generic DPSContext */						\
    DPSContextRec c;							\
  /* amount of current binArray left */					\
    unsigned int binArrayLeft;						\
  /* length of curr seq % 4 */						\
    char lengthMod4;							\
  /* makes trace output nicer for debugging */				\
    char debugging;							\
  /* a chained context being used for tracing */			\
    DPSContext traceCtxt;						\
  /* state for converting binary object sequences to ascii.  A context	\
     whose encoding is not binary must allocate a buffer of size	\
     ASCII_BUF_SIZE for ascii conversion				\
   */									\
    char *asciiBufferSpace;						\
    char *asciiBuffer;							\
    char *asciiBufCurr;
  /* ??? maybe add arrays of lengths and strings to buffer ptrs to strings
     instead of their data.  asciiBuffer would only contain tokens. */

typedef struct __DPSOutputContext {	/* any output context */
    OUTPUT_CONTEXT_ELEMENTS		/* common variables */
} _DPSOutputContextRec, *_DPSOutputContext;

#define LIST_OVERHEAD	8	/* bytes to prepend for list functions */
#define STD_TO_OUTPUT(ctxt)	((_DPSOutputContext)	\
					(((char *)(ctxt))-LIST_OVERHEAD))
#define OUTPUT_TO_STD(ctxt)	((DPSContext)(((char *)(ctxt))+LIST_OVERHEAD))


/* The following elements define a context that the user created to output
   to a given NXStream.
 */
#define STREAM_CONTEXT_ELEMENTS						\
  /* a stream context is an output context */				\
    OUTPUT_CONTEXT_ELEMENTS
 
typedef struct __DPSStreamContext {	/* a stream context */
    STREAM_CONTEXT_ELEMENTS
} _DPSStreamContextRec, *_DPSStreamContext;

#define OUTPUT_TO_STREAM(ctxt)	((_DPSStreamContext)(ctxt))
#define STREAM_TO_OUTPUT(ctxt)	((_DPSOutputContext)(ctxt))
#define STD_TO_STREAM(ctxt)	OUTPUT_TO_STREAM(STD_TO_OUTPUT(ctxt))
#define STREAM_TO_STD(ctxt)	OUTPUT_TO_STD(STREAM_TO_OUTPUT(ctxt))


/* The following structure elements should be the first ones in any context
   that will talk to a window server.
 */
#define SERVER_CONTEXT_ELEMENTS						\
  OUTPUT_CONTEXT_ELEMENTS						\
  /* a function to call to get more characters */			\
    void (*fillBuf)(/* DPSServerContext ctxt; */);			\
  /* parsing ptrs into data buffer */					\
    char *bufCurr;							\
    char *bufEnd;							\
  /* items that can cross buffer boundaries */				\
    NXEvent event;							\
    DPSBinObjSeqRec header;						\
  /* amount of the partial items. zero means no partial */		\
    unsigned char partialEventChars;					\
    unsigned char partialHeaderChars;					\
    struct {								\
      /* are we tracing events? */					\
	unsigned int traceEvents:1;					\
      /* are we suppressing deadkey processing? */			\
	unsigned int doDeadKeys:1;					\
      /* perf flag for monitoring Q size */				\
	unsigned int evQGrowing:1;					\
	unsigned int pad:13;						\
    } serverCtxtFlags;							\
  /* filter routine for events */					\
    DPSEventFilterFunc eventFunc;					\
  /* used during DPSDiscardEvents to keep wait cursor up to date */	\
    int WCFlushTime;							\
  /* dead key events */							\
    NXEvent deadDownEvent;						\
    NXEvent deadUpEvent;

typedef struct __DPSServerContext {	/* any server context */
    SERVER_CONTEXT_ELEMENTS		/* common server variables */
} _DPSServerContextRec, *_DPSServerContext;

#define OUTPUT_TO_SERVER(ctxt)	((_DPSServerContext)(ctxt))
#define SERVER_TO_OUTPUT(ctxt)	((_DPSOutputContext)(ctxt))
#define STD_TO_SERVER(ctxt)	OUTPUT_TO_SERVER(STD_TO_OUTPUT(ctxt))
#define SERVER_TO_STD(ctxt)	OUTPUT_TO_STD(SERVER_TO_OUTPUT(ctxt))

/* constants for portsLocations */
#define IN_NO_SET		0
#define IN_MAIN_SET		1
#define IN_LOCAL_SET		2

/* The following structure elements should be the first ones in any context
   that will talk to a window server via Mach IPC.
 */
#define MACH_CONTEXT_ELEMENTS						\
  /* ptr to next element of global context list */			\
    struct __DPSMachContext *nextMC;					\
  /* key that can be searched for - port to get PS messages on */	\
    port_t inPort;							\
  SERVER_CONTEXT_ELEMENTS						\
  /* port to send PS messages to */					\
    port_t outPort;							\
  /* out of line data from last msg */					\
    char *ooLineData;							\
  /* size of out of line data from last msg */				\
    int ooLineSize;							\
  /* partial message from error bombout */				\
    char *savedMsg;							\
  /* set of flags */							\
    struct {								\
      /* are this port and its ping port in the main port set? */	\
	unsigned int portsLocations:2;					\
      /* pmon sending mode (used by send_data to make pmon events) */	\
	unsigned int sendMode:4;					\
      /* has someone done a DPSStartWaitCursorTimer? */			\
	unsigned int didStartWCTimer:1;					\
	unsigned int pad:9;						\
    } flags;								\
  /* port to receive Pings on */					\
    port_t pingPort;							\
  /* port set for non-main loop recevies */				\
    port_set_name_t localSet;						\
  /* pings we are expecting to receive */				\
    int expectedPings;							\
  /* the timestamp of the last event we've handed to the app */		\
    int lastEventTime;

typedef struct __DPSMachContext {	/* a MACH context */
    MACH_CONTEXT_ELEMENTS		/* context variables */
} _DPSMachContextRec, *_DPSMachContext;

#define SERVER_TO_MACH(ctxt)	((_DPSMachContext)	\
					(((char *)(ctxt))-LIST_OVERHEAD))
#define MACH_TO_SERVER(ctxt)	((_DPSServerContext)	\
					(((char *)(ctxt))+LIST_OVERHEAD))
#define OUTPUT_TO_MACH(ctxt)	SERVER_TO_MACH(OUTPUT_TO_SERVER(ctxt))
#define MACH_TO_OUTPUT(ctxt)	SERVER_TO_OUTPUT(MACH_TO_SERVER(ctxt))
#define STD_TO_MACH(ctxt)	OUTPUT_TO_MACH(STD_TO_OUTPUT(ctxt))
#define MACH_TO_STD(ctxt)	OUTPUT_TO_STD(MACH_TO_OUTPUT(ctxt))


/* a routine associated with a stream or a timed entry */
typedef struct __DPSRoutine {
    void (*func)();
    void *userData;		/* field for user's context */
    char inUse;			/* dont call one recursively */
    DPSContext server;		/* server of this routine */
    unsigned long priority;	/* mask of routine's priority */
} _DPSRoutine;


/* a timed entry, which calls a routine after a specified interval */
typedef struct  __DPSTimedEntry {
    struct __DPSTimedEntry *next;	/* must be first for list functions */
    int unused;				/* for list searching */
    short refs;				/* for list re-entrant looping */
    char doFree;			/* Does this list item need freeing? */
    double nextRun;
    double interval;
    _DPSRoutine routine;
} _DPSTimedEntry;


/* a stream that we listen to.  When we hear something, we call a proc */
typedef struct __DPSStream {
    struct __DPSStream *next;		/* must be first for list functions */
    int fd;				/* for list searching */
    short refs;				/* for list re-entrant looping */
    char doFree;			/* Does this list item need freeing? */
    _DPSRoutine routine;
} _DPSStream;


/* a port that we listen to.  When we get a message on the port, we call the
   given proc.
 */
typedef struct  __DPSPort {
    struct __DPSPort *next;		/* must be first for list functions */
    port_t port;			/* for list searching */
    short refs;				/* for list re-entrant looping */
    char doFree;			/* Does this list item need freeing? */
    char inMainSet;			/* Is this port in main set? */
    _DPSRoutine routine;
} _DPSPort;


/* internal DPSSpace type.  The real space MUST STAY AT THE FRONT of this
   structure.
 */
typedef struct __DPSPrivSpace {
    DPSSpaceRec s;	/* public data */
    DPSContext ctxt;	/* back ptr to the one context for this space */
} _DPSPrivSpace;


#define BINARRAY_ESCAPE(ptr)	(((DPSBinObjSeq)(ptr))->tokenType)
#define BINARRAY_TOPOBJS(ptr)	(((DPSBinObjSeq)(ptr))->nTopElements)
#define BINARRAY_LENGTH(ptr)	(((DPSBinObjSeq)(ptr))->length)
#define BINARRAY_OBJECTS(ptr)	(((DPSBinObjSeq)(ptr))->objects)
#define BINARRAY_EXTOPOBJS(ptr)	(((DPSExtendedBinObjSeq)(ptr))->nTopElements)
#define BINARRAY_EXLENGTH(ptr)	(((DPSExtendedBinObjSeq)(ptr))->length)
#define BINARRAY_EXOBJECTS(ptr)	(((DPSExtendedBinObjSeq)(ptr))->objects)

/* tests to see if a queue is empty */
#define  _DPS_QUEUEEMPTY(q)	(!(q)->firstOut)

/* returns first item in a queue. Assumes something is in the Q. */
#define  _DPS_FIRSTQELEMENT(q)	(&((q)->firstOut->ev))

/* returns last item in a queue */
#define  _DPS_LASTQELEMENT(q)	(&((q)->lastIn->ev))

/* private DPS routines */
extern void *_DPSNewListItem(_DPSList *list, int size, NXZone *zone);
extern void _DPSAddListItem(_DPSList *list, _DPSListItem *newItem);
extern void *_DPSFindListItemByKey(_DPSList *list, int key);
extern void *_DPSFindListItemByItem(_DPSList *list, void *item);
extern int _DPSRemoveListItem(_DPSList *list, void *item, int save);
extern void _DPSConvertBinArray(NXStream *st, const DPSBinObjRec *curr, 
			int numObjs, int level, const DPSBinObjSeqRec *start,
			DPSProgramEncoding progEnc, DPSNameEncoding nameEnc,
			char debugging);
extern void _DPSDefaultTextProc();	/* default text proc ???makepublic */
extern void _DPSMachPrepareToParseMsg();
extern void _DPSMachCallTokenProc();
extern NXEvent *_DPSGetQEntry(int mask, DPSContext ctxt);
extern int _DPSPostQElement(NXEvent *event, int postAtStart);
extern void _DPSRemoveQEntry(NXEvent *event);
extern NXEvent *_DPSGetNextAvail(void);
extern void _DPSTraceOutput( int time, char *fmt, ... );
extern DPSContext _DPSLeanCreateStreamContext();
extern NXStream *_DPSOpenStream( _DPSMachContext ctxt, NXZone *zone );
extern void _DPSFlushStream( NXStream *s, int doPing, int lastEvent );
extern void _DPSResetStream( NXStream *s );
extern void _DPSInitMainPortSet();
extern void _DPSAddNotifyToSet( port_set_name_t set );
extern void _DPSHandleMsg (_DPSMessage *msg, msg_return_t ret, int *calledRoutine, int mask, unsigned long thresh );
extern void _DPSProcessError( DPSContext ctxt, DPSErrorCode errorCode, const void *arg1, const void *arg2 );
extern void _DPSReprocessError( DPSContext ctxt, NXHandler *except );
extern int _DPSTokenProc( int mask, _DPSServerContext cc );
extern DPSSpace _DPSCreateSpace( DPSContext ctxt, NXZone *zone );
extern void _DPSAssemblePorts( _DPSMachContext ctxt );
extern msg_return_t _DPSLocalSetMsgReceive( _DPSMachContext ctxt, _DPSMessage *msg, msg_option_t option, msg_timeout_t time );
extern void _DPSPrintEvent(FILE *fp, NXEvent *e);
extern void _NXOldOrderwindow(void);
#if defined(DEBUG) && defined(TRACE_Q_SIZE)
extern void _DPSTraceReportQSize(_DPSQueue *q);
#endif
extern void _DPSListenToNewContexts(void);


extern _DPSList _DPSOutputContexts;	/* all output contexts we have made */
extern _DPSList _DPSMachContexts;	/* all Mach contexts we have made */
extern _DPSQueue _DPSEventQ;		/* global event queue */
extern port_t _DPSNotifyPort;		/* notify port we create */
extern char _DPSTracking;		/* Do we coalesce events? */
extern char _DPSTracingOn;		/* Is output tracing on? */
extern char _DPSEventTracingOn;		/* Is event tracing on? */
extern NXEvent *_DPSLastEvent;		/* the last event read with correct mask */
extern short _DPSMsgSize;		/* size of message buffer */
extern int _DPSLastNameIndex;		/* last name index assigned for all contexts */
extern char _DPSDo10Compatibility;	/* do hacks for 1.0 compatibility? */
extern char _DPSDoNSC;			/* do setnextstepcontext? */

/* what to do when we find an error */
#define RAISE_ERROR( ctxt, errCode, arg1, arg2 ) \
	    _DPSProcessError( (ctxt), (errCode), (arg1), (arg2) )
#define RERAISE_ERROR( ctxt, except ) \
	    _DPSReprocessError( (ctxt), (except) )

/* debug time check for an error that prints a message */
#undef ASSERT	/* this collision courtesy of <cthreads.h> */
#if DEBUG
#define ASSERT( cond, str )  {if(!(cond)) fprintf( stderr, "dpsclient assertion failed: %s\n", (str));}
#else
#define ASSERT( cond, str )  {}
#endif

extern void *_DPSLocalMalloc( unsigned size );
#define  MALLOC( VAR, TYPE, NUM )				\
   ((VAR) = (TYPE *) _DPSLocalMalloc( (unsigned)(NUM)*sizeof(TYPE) )) 

extern void *_DPSLocalRealloc( void *ptr, unsigned size );
#define  REALLOC( VAR, TYPE, NUM )				\
   ((VAR) = (TYPE *) _DPSLocalRealloc((VAR), (unsigned)(NUM)*sizeof(TYPE)))

#define  FREE( PTR )	free( (char *) (PTR) );

#ifndef TRUE
#define  TRUE		1
#endif
#ifndef FALSE
#define  FALSE		0
#endif

#define  MIN(a,b)	((a) < (b) ? (a) : (b))
#define  MAX(a,b)	((a) > (b) ? (a) : (b))

#define  STREQUAL(s1,s2) (*(s1) == *(s2) && !strcmp((s1),(s2)))


/*??? garbage in my code because there are no externs for system calls */
extern int select(int nfds, fd_set *readfds, fd_set *writefds,
			fd_set *exceptfds, struct timeval *timeout);
extern int read(int fd, char *buf, int nbytes);
extern int write(int fd, char *buf, int nbytes);
extern int pipe(int fildes[2]);
extern int close(int fd);
extern int fcntl(int fd, int cmd, int arg);
extern int kern_timestamp();
extern int gethostname(char *name, int namelen);

/* whole bunch of globals for tracing */
#ifndef SHLIB
#ifdef DEBUG

extern FILE *DPSTracingFile;
extern int DPSTracingOn;
extern int DPSBacklog;

#endif
#endif

#if defined(DEBUG) && defined(PMON)

typedef enum _PmonEventTypes {
	pmon_te_proc = 1,
	    /* x = teNum, y = call or return (0 or 1), z = period */
	pmon_port_proc = 2,
	    /* x = procNum, y = call or return (0 or 1) */
	pmon_fd_proc = 4,
	    /* x = fd, y = call or return (0 or 1) */
	pmon_send_data = 8,
	    /* x = length, y = mode, z = inline or out-of-line (0 or 1) */
	pmon_msg_receive = 16,
	    /* x = timeout, y = mode, z = msg_receive options */
	pmon_receive_data = 32,
	    /* x = length, y = mode, z = returnVal */
	pmon_parsed_data = 64,
	    /* x = #events, y = #tokens */
	pmon_request_event = 128,
	    /* x = mask, y = timeout, z = peek? */
	pmon_return_event = 256
	    /* x = type, y = subtype */
} PmonEventTypes;

typedef enum _PmonEventModes {
	pmon_auto_flush = 0,		/* auto flush when buffer filled */
	pmon_scheduling_loop,		/* in main get event */
	pmon_get_tokens,		/* getting tokens */
	pmon_ping,			/* doing a ping */
	pmon_proc,			/* flushing after calling proc */
	pmon_explicit,			/* an explicit flush, or buffer fill */
	pmon_avoid_deadlock,		/* op done because send timed out */
	pmon_destroy			/* done as part of killing context */
} PmonEventModes;

extern void _NXInitPmon(void);
extern void _NXSendPmonEvent(void *srcData, int eventType,
					int d1, int d2, int d3);
extern void _NXFlushPmonEvents(void);

extern void *_DPSPmonSrcData;

#define _NXSetPmonSendMode(ctxt, mode)				\
	if (ctxt->type == dps_machServer) {			\
	    STD_TO_MACH(ctxt)->flags.sendMode = mode;		\
	}

#else

#define _NXSendPmonEvent(srcData, eventType, d1, d2, d3)
#define _NXSetPmonSendMode(ctxt, mode)

#endif
