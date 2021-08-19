#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
    input.c

    This file has routines for all input extensions to dps.  There are
    routines for using events, streams, ports, and timed entries.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#include "dpsclient.h"
#include "defs.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <sys/types.h>		/* for fd_set macros */
#include <sys/time_stamp.h>
#include <sys/notify.h>		/* for port notification constants */
#include "mask_ops.h"		/* for operations on fd masks */

/* constants for messaging between main and select threads */
#define MSG_RPC		1
#define MSG_SIMPLE	2

static _DPSList TimedEntries = {NULL, NULL};	/* TEs we have */
static _DPSList Ports = {NULL, NULL};		/* ports to listen to */
static _DPSList Streams = {NULL, NULL};		/* fds to listen to */

static int SelectPipe[2] = {-1, -1};	/* fd to talk to select thread */
static port_t SelectReplyPort = PORT_NULL;   /* port to ack stream deletes */

/* No bit should ever be set in both of these masks */
static fd_set ActiveStreams = {{0}};	/* mask of fds with data */
static fd_set WaitingStreams = {{0}};	/* mask of fds to listen to */
static struct mutex MaskLock = {0};	/* lock for accessing masks */

static int MaxFD = 0;			/* maximum fd number we look at */

/* bitmask of cummulative thresholds from all active calls to getEvent */
static unsigned long ProcThreshold = 0;

/* increments whenever a port is added or removed from the main set */
static unsigned long MainSetCount = 0;

/* set of ports received on in the main loop */
static port_set_name_t MainPorts = 0;

/* If someone has done a DPSAddPort on the notify port, we use the byte in
   the _DPSPort struct to know if the notify port is in the main set.  If
   it hasnt been DPSAddPort'ed, then we use the storage here.
 */
static char NotifyInMainPortsStorage = FALSE;
static char NotifyInPortsList = FALSE;
static char *NotifyInMainPorts = &NotifyInMainPortsStorage;

#ifdef DEBUG		/* make select more palatable to the debugger */

static mySelect( nfds, readfds, writefds, exceptfds, timeout )
int nfds;
fd_set *readfds, *writefds, *exceptfds;
struct timeval *timeout;
{
    return select( nfds, readfds, writefds, exceptfds, timeout );
}

#ifdef SHLIB
#undef select		/* pre-defined as part of shlib scheme */
#endif

#define select( a, b, c, d, e)	mySelect( (a), (b), (c), (d), (e) )
#endif

#if defined(DEBUG) && defined(TRACE_Q_SIZE)
#define TRACE_Q_GROWING(ctxt)					\
	if ((ctxt)->serverCtxtFlags.evQGrowing) {		\
	    _DPSTraceReportQSize(&_DPSEventQ);			\
	    (ctxt)->serverCtxtFlags.evQGrowing = FALSE;		\
	}
#else
#define TRACE_Q_GROWING(ctxt)
#endif

static int procsRunnable (unsigned long thresh);
static _DPSMachContext findByOutPort (port_t port);
static _DPSMachContext findByPingPort (port_t port);
static int checkFDs (unsigned long thresh);
static any_t selectLoop (any_t arg);
static void launchSelectThread();
static int checkTEs (double *nowReturn, unsigned long thresh);
static double getTEInterval (double maxTime, double now, unsigned long thresh);
static unsigned long convertPriority (int intP);
static double getTime();
static void fixMainSet (_DPSMachContext ctxt, int allContexts, int threshold);
static void flushContextAndAckWC(DPSContext ctxtArg);


/*  Gets the next element off the event queue.  If ctxt is DPS_ALLCONTEXTS,
    we look at all of them.  No procs with priority lower than threshold
    will be called.
*/
_DPSGetOrPeekEvent( DPSContext ctxtArg, NXEvent *eventStorage,
		register int mask, double wait, int threshold, int peek )
{
    _DPSMachContext ctxt = STD_TO_MACH(ctxtArg);
    _DPSQueue *q = &_DPSEventQ;
    _DPSMessage *localMsg = alloca(_DPSMsgSize);
    NXEvent *lastQEvent;		/* last event in Q */
    int lastQEventMask;			/* type of last event in Q */
    double now = getTime();
    double exitTime = now + wait;
    double nextWaitSec;			/* timeout arg to msg_recv in sec */
    int nextWaitMicroSec;		/* timeout arg to msg_recv in msec */
    msg_option_t nextOption;		/* option arg to msg_recv */
    msg_return_t ret;			/* return value from msg_recv */
    int calledRoutine = FALSE;	/* did we call any procs this time around? */
    volatile unsigned long prevThreshold;  /* save and restore ProcThreshold */
    long restrLevel;			/* copy of MainSetCount */

    _NXSendPmonEvent(_DPSPmonSrcData, pmon_request_event, mask,
						(int)(wait*1000), peek);

  /*
   *  Flush all context we're dealing with and save threshold.
   */
    if (ctxtArg == DPS_ALLCONTEXTS) {
	register _DPSOutputContext oc;

	for (oc = (_DPSOutputContext)(_DPSOutputContexts.head); oc;
							oc = oc->nextOC) {
	    _NXSetPmonSendMode(OUTPUT_TO_STD(oc), pmon_scheduling_loop);
	    flushContextAndAckWC(OUTPUT_TO_STD(oc));
	}
    } else if (ctxtArg) {
	_NXSetPmonSendMode(ctxtArg, pmon_scheduling_loop);
	flushContextAndAckWC(ctxtArg);
    }
    ASSERT( threshold >= 0 && threshold < 32, "Bad threshold in GetEvent" );
    prevThreshold = ProcThreshold;
    ProcThreshold |= 1<<threshold;
#if defined(DEBUG) && defined(PMON)
    if (threshold <= 5)
	_NXFlushPmonEvents();
#endif

  /*
   *  Check the event Q, and setup things to check if we may coalesce.
   */
    _DPSLastEvent = _DPSGetQEntry(mask, ctxtArg);
    if( _DPS_QUEUEEMPTY(q) ) {
	lastQEvent = NULL;
	lastQEventMask = NX_NULLEVENTMASK;
    } else {
	lastQEvent = _DPS_LASTQELEMENT(q);
	lastQEventMask = NX_EVENTCODEMASK(lastQEvent->type);
    }

  /*
   *  If we dont have a matching event, or its a candidate for coalescing,
   *  or there are procs to check, we cant skip communications overhead.
   */
    if( !_DPSLastEvent ||
	((_DPSLastEvent == lastQEvent) &&
	     (lastQEventMask & (NX_LMOUSEDRAGGEDMASK | NX_RMOUSEDRAGGEDMASK |
				NX_MOUSEMOVEDMASK)) && _DPSTracking) ||
	     procsRunnable( ProcThreshold ) ) {
	NX_DURING
	  /*
	   *  Process any saved data of any PS servers.  Get the ports
	   *  we're interested into the main set.
	   */
	    {
		register _DPSMachContext mc;

		for( mc = (_DPSMachContext)(_DPSMachContexts.head); mc;
							mc = mc->nextMC )
		    if( ctxtArg == DPS_ALLCONTEXTS || mc == ctxt )
			if( mc->savedMsg )
			    _DPSMachCallTokenProc( mask, mc );
	    }
	    fixMainSet( ctxt, ctxtArg == DPS_ALLCONTEXTS, ProcThreshold );
	    restrLevel = MainSetCount;

	  /*
	   *  Check the fds.  We must do this before the do loop because
	   *  there may be fds in the ActiveMask whose procs were not called
	   *  earlier because of priority masking.  If we didnt check them
	   *  we could go sit in msg_receive and ignore them forever.
	   */
	    if(checkFDs(ProcThreshold))
		_DPSLastEvent = _DPSGetQEntry(mask, ctxtArg);

	  /*
	   * Handle messages (at least once) until we either timeout or
	   * find a matching event.
	   */
	    do {
	      /*
	       *  Calculate next timeout and option values.
	       */
		if (_DPSLastEvent) {
		    nextWaitSec = 0.0;
		    nextOption = RCV_TIMEOUT;
		} else {
		    register _DPSTimedEntry *te;

		    nextWaitSec = exitTime;
		    for( te = (_DPSTimedEntry *)TimedEntries.head; te;
							te = te->next ) {
			if( te->nextRun < nextWaitSec &&
					te->routine.priority > ProcThreshold )
			    nextWaitSec = te->nextRun;
		    }
		    nextWaitSec -= now;
		    if ( nextWaitSec < 0.0 )
			nextWaitSec = 0.0;
		    nextOption = (nextWaitSec > NX_FOREVER/2) ?
				MSG_OPTION_NONE : RCV_INTERRUPT|RCV_TIMEOUT;
		}

	      /*
	       *  Process the next group of messages.  If the mainSet was
	       *  changed by a recursively called copy of ourself, restore
	       *  it back to the right ports for this stack frame.  All
	       *  msg_receive's after the first are with a zero timeout,
	       *  just to coalesce any lingering events.
	       */
		nextWaitMicroSec = nextWaitSec * 1000;
		do {
		    localMsg->header.msg_local_port = MainPorts;
		    localMsg->header.msg_size = _DPSMsgSize;
		    if( restrLevel != MainSetCount ) {
			fixMainSet( ctxt, ctxtArg == DPS_ALLCONTEXTS,
							ProcThreshold );
			restrLevel = MainSetCount;
		    }
#ifdef DEBUG
		    {
			extern int (*_DPSZapCheck)();

			if (_DPSZapCheck)
			    (*_DPSZapCheck)();
		    }
#endif
		    _NXSendPmonEvent(_DPSPmonSrcData, pmon_msg_receive,
					nextWaitMicroSec, pmon_scheduling_loop,
					nextOption);
		    ret = msg_receive( (msg_header_t *)localMsg, nextOption,
							nextWaitMicroSec );
		    _NXSendPmonEvent(_DPSPmonSrcData, pmon_receive_data,
					localMsg->header.msg_size,
					pmon_scheduling_loop, ret);
		    _DPSHandleMsg( localMsg, ret, &calledRoutine, mask,
							ProcThreshold );
		    nextOption = RCV_TIMEOUT;
		    nextWaitMicroSec = 0;
		} while( ret == RCV_SUCCESS && !calledRoutine );

	      /*
	       *  If we called any routines, we must try to update
	       *  _DPSLastEvent in case things were added to the event Q.
	       * Note: we must do this here as well as below since the 
	       * _DPSHandleMsg might have called a routine which might 
	       * have mucked with the queue.
	       */
		if( calledRoutine ) {
		    _DPSLastEvent = _DPSGetQEntry(mask, ctxtArg);
		    calledRoutine = FALSE;
#if defined(DEBUG) && defined(PMON)
		    if (threshold <= 5)
			_NXFlushPmonEvents();
#endif
		}

	      /*
	       *  If we found an event we like, hike up the threshold to
	       *  avoid calling procs with equal priority as the threshold.
	       */
		if( _DPSLastEvent )
		    ProcThreshold <<= 1;

	      /*
	       *  Handle any fd's with activity or timed entries that have
	       *  expired.
	       */
		calledRoutine |= checkTEs(&now, ProcThreshold);
		calledRoutine |= checkFDs(ProcThreshold);

	      /*
	       *  If we called any routines, we must try to update
	       *  _DPSLastEvent in case things were added to the event Q.
	       */
		if( calledRoutine ) {
		    _DPSLastEvent = _DPSGetQEntry(mask, ctxtArg);
		    calledRoutine = FALSE;
#if defined(DEBUG) && defined(PMON)
		    if (threshold <= 5)
			_NXFlushPmonEvents();
#endif
		}
	    } while( (exitTime - now) > 0.0 && !_DPSLastEvent );
	NX_HANDLER
	    ProcThreshold = prevThreshold;	/* restore threshold */
	    RERAISE_ERROR(NULL, &NXLocalHandler);
	NX_ENDHANDLER
    }

  /*
   *  If we found an event that matched, return it.  Take it out of the Q
   *  if we're not peeking.
   */
    if( _DPSLastEvent ) {
	*eventStorage = *_DPSLastEvent;
	if( !peek ) {
	    _DPSRemoveQEntry(_DPSLastEvent);
	    if( eventStorage->ctxt ) {
		TRACE_Q_GROWING(STD_TO_MACH(eventStorage->ctxt));
		STD_TO_MACH(eventStorage->ctxt)->lastEventTime = eventStorage->time;
	    }
	}
    }
    ProcThreshold = prevThreshold;	/* restore threshold */
    _NXSendPmonEvent(_DPSPmonSrcData, pmon_return_event,
		_DPSLastEvent ? _DPSLastEvent->type : -1,
		_DPSLastEvent ? _DPSLastEvent->data.compound.subtype : -1, 0);
    return _DPSLastEvent != NULL;
}


/* returns whether there are any TimedEntries, port-procs, or fd-procs with
   a higher priority than the accumulated threshold.
*/
static int
procsRunnable(unsigned long thresh)
{
    register _DPSListItem *item;

    for( item = TimedEntries.head; item; item = item->next )
	if( ((_DPSTimedEntry *)item)->routine.priority > thresh )
	    return TRUE;
    for( item = Ports.head; item; item = item->next )
	if( ((_DPSPort *)item)->routine.priority > thresh )
	    return TRUE;
    for( item = Streams.head; item; item = item->next )
	if( ((_DPSStream *)item)->routine.priority > thresh )
	    return TRUE;
    return FALSE;
}


/* searches for a Mach context with the given output port. */
static _DPSMachContext
findByOutPort(port_t port)
{
    register _DPSMachContext mc;

    for( mc = (_DPSMachContext)(_DPSMachContexts.head); mc; mc = mc->nextMC )
	if( mc->outPort == port )
	    return mc;
    return NULL;
}


/* searches for a Mach context with the given ping port. */
static _DPSMachContext
findByPingPort(port_t port)
{
    register _DPSMachContext mc;

    for( mc = (_DPSMachContext)(_DPSMachContexts.head); mc; mc = mc->nextMC )
	if( mc->pingPort == port )
	    return mc;
    return NULL;
}


/* Gets the right ports in the main port set from those ports that correspond
    to PS servers and ports app as done an AddPort on.
*/
static void
fixMainSet (_DPSMachContext ctxt, int allContexts, int threshold)
{
    register _DPSMachContext mc;
    register _DPSPort *p;

    for( mc = (_DPSMachContext)(_DPSMachContexts.head); mc; mc = mc->nextMC )
	if( allContexts || mc == ctxt ) {
	    if( mc->flags.portsLocations != IN_MAIN_SET ) {
		port_set_add( task_self(), MainPorts, mc->inPort );
		port_set_add( task_self(), MainPorts, mc->pingPort );
		mc->flags.portsLocations = IN_MAIN_SET;
		MainSetCount++;
	    }
	} else
	    if( mc->flags.portsLocations == IN_MAIN_SET ) {
		port_set_remove( task_self(), mc->inPort );
		port_set_remove( task_self(), mc->pingPort );
		mc->flags.portsLocations = IN_NO_SET;
		MainSetCount++;
	    }

    for( p = (_DPSPort *)(Ports.head); p; p = p->next )
	if( p->routine.priority > threshold && !p->routine.inUse ) {
	    if( !p->inMainSet ) {
		port_set_add( task_self(), MainPorts, p->port );
		p->inMainSet = TRUE;
		MainSetCount++;
	    }
	} else
	    if( p->inMainSet ) {
		port_set_remove( task_self(), p->port );
		p->inMainSet = FALSE;
		MainSetCount++;
	    }
    if( !NotifyInPortsList && !NotifyInMainPortsStorage ){
	port_set_add( task_self(), MainPorts, _DPSNotifyPort );
	NotifyInMainPortsStorage = TRUE;
	MainSetCount++;
    }
}


/* handles a message on a port */
void
_DPSHandleMsg(
    _DPSMessage *msg,	/* message to process */
    msg_return_t ret,	/* return value from last msg_receive */
    int *calledRoutine,	/* did we call a proc? */
    int mask,		/* mask of things we're looking for */
    unsigned long thresh )
{
    _DPSMachContext mc;
    _DPSPort *p;
    DPSContext currServer;	/* place to save current server */
    DPSContext portServer;	/* server for port proc */
    NXHandler except;		/* params for raising an exception */
    _DPSMessage *msgCopy;

    except.code = 0;
    if( ret == RCV_SUCCESS ) {
      /*
       * check for a message on a server port
       */
	if( mc = _DPSFindListItemByKey( &_DPSMachContexts, msg->header.msg_local_port )) {
	    if (msg->header.msg_id == PS_DATA_MSG_ID) {
		_DPSMachPrepareToParseMsg( mc, msg );
		_DPSMachCallTokenProc( mask, mc );
#ifdef SAME_PING_PORT
	    } else if (msg->header.msg_id == PS_EV_PING_DATA_RETURN_MSG_ID) {
		if( mc->expectedPings > 0 )
		    mc->expectedPings--;
		else
		    ASSERT(FALSE, "Extra ping reply received");
#endif
	    } else
		ASSERT(FALSE, "Odd message type received on data port");
      /*
       * check for a message on a ping port
       */
	} else if( mc = findByPingPort( msg->header.msg_local_port )) {
#ifndef SAME_PING_PORT
	    if( msg->header.msg_id == PS_EV_PING_DATA_RETURN_MSG_ID ) {
		if( mc->expectedPings > 0 )
		    mc->expectedPings--;
		else
		    ASSERT(FALSE, "Extra ping reply received");
	    } else
#endif
		ASSERT(FALSE, "Odd message type received on ping port");
      /*
       * check for a message on the notify port that refers to a server
       */
	} else if( (msg->header.msg_local_port == _DPSNotifyPort) &&
	       (mc = findByOutPort( ((notification_t *)msg)->notify_port ))) {
	    ASSERT( msg->header.msg_id == NOTIFY_PORT_DELETED,
			"unexpected type of notification on server port");
	    RAISE_ERROR( MACH_TO_STD(mc), dps_err_connectionClosed, 0, 0 );
      /*
       * check for a message on the some other port we were told to listen to
       */
	} else if( (p = _DPSFindListItemByKey( &Ports, msg->header.msg_local_port )) &&
		  !p->doFree ) {
	    ASSERT( p->routine.priority > thresh && !p->routine.inUse,
				"msg received for blocked out port" );
	    ASSERT( !p->routine.inUse, "msg received for port in use" );
	    p->routine.inUse = TRUE;
	    currServer = DPSGetCurrentContext();   /* save current server */
	    portServer = p->routine.server;
	    DPSSetContext( portServer );
	    p->refs++;		/* protect case of removal within proc */
	    msgCopy = alloca((msg->header.msg_size + 3) & ~3);
	    bcopy(msg, msgCopy, msg->header.msg_size);
	    NX_DURING
		_NXSendPmonEvent(_DPSPmonSrcData, pmon_port_proc, p->port,
						0, msg->header.msg_size);
		(p->routine.func)( msgCopy, p->routine.userData );
		_NXSendPmonEvent(_DPSPmonSrcData, pmon_port_proc, p->port,
						1, msg->header.msg_size);
	    NX_HANDLER
		except = NXLocalHandler;	/* save to re-raise later */
	    NX_ENDHANDLER
	    p->refs--;
	  /* if removed out from under us in the proc */
	    if( p->doFree && !p->refs )
		(void)_DPSRemoveListItem( &Ports, p, TRUE );
	    else
		p->routine.inUse = FALSE;
	    if (!except.code && _DPSFindListItemByItem( &_DPSOutputContexts,
						STD_TO_OUTPUT(portServer))) {
		_NXSetPmonSendMode(portServer, pmon_proc);
		flushContextAndAckWC(portServer);
	    }
	  /* if the the old server is still around */
	    if( !currServer || _DPSFindListItemByItem( &_DPSOutputContexts,
						STD_TO_OUTPUT(currServer) ))
		DPSSetContext( currServer );	/* restore server */
	    if( except.code )
		RERAISE_ERROR( NULL, &except );
	    if( calledRoutine )
		*calledRoutine = TRUE;
	}
      /*
       * else drop that message on the floor, including any messages from
       * from the select kludge thread
       */
    } else if( ret != RCV_TIMED_OUT && ret != RCV_INTERRUPTED )
	RAISE_ERROR( 0, dps_err_read, (void *)ret, 0 );
}


/*  Posts an event to the client side Q.  Returns -1 if it can't
    post the event (Q full).
*/
DPSPostEvent( event, atStart )
NXEvent *event;
int atStart;
{
    return _DPSPostQElement(event, atStart );
}


/* checks the mask in a message and calls the right stream procs */
static int
checkFDs( unsigned long thresh )
{
    int calledProc = FALSE;
    _DPSStream *st, *nextSt;
    int currFD;
    DPSContext currServer, streamServer;
    char notifyMsg = MSG_SIMPLE;
    NXHandler except;		/* params for raising an exception */

    except.code = 0;
    if( st = (_DPSStream *)Streams.head ) {
	do {
	    if( !st->routine.inUse && FD_ISSET( st->fd, &ActiveStreams ) &&
			!st->doFree && st->routine.priority > thresh ) {
		currFD = st->fd;
		st->routine.inUse = TRUE;
		currServer = DPSGetCurrentContext();  /* save current server */
		streamServer = st->routine.server;
		DPSSetContext( streamServer );
		st->refs++;	/* protect case of removal within proc */
		NX_DURING
		    _NXSendPmonEvent(_DPSPmonSrcData, pmon_fd_proc, currFD,
									0, 0);
		    (st->routine.func)(st->fd, st->routine.userData);
		    _NXSendPmonEvent(_DPSPmonSrcData, pmon_fd_proc, currFD,
									1, 0);
		NX_HANDLER
		    except = NXLocalHandler;	/* save to re-raise later */
		NX_ENDHANDLER
		st->refs--;
		nextSt = st->next;
	      /* if removed out from under us in the proc */
		if (st->doFree && !st->refs)
		    (void)_DPSRemoveListItem(&Streams, st, TRUE);
		else {
		    st->routine.inUse = FALSE;
		    mutex_lock(&MaskLock);
		    FD_SET(currFD, &WaitingStreams);
		    FD_CLR(currFD, &ActiveStreams);
		    mutex_unlock( &MaskLock );
		  /* signal select that we added fds back to mask */
#ifdef DEBUG
		    if( write( SelectPipe[1], &notifyMsg, 1 ) == -1 )
			ASSERT(FALSE, "checkFDs:write to selectLoop failed");
#else
		    (void)write( SelectPipe[1], &notifyMsg, 1 );
#endif
		}
		if (!except.code &&
			_DPSFindListItemByItem(&_DPSOutputContexts,
						STD_TO_OUTPUT(streamServer))) {
		    _NXSetPmonSendMode(streamServer, pmon_proc);
		    flushContextAndAckWC(streamServer);
		}
	      /* if the the old server is still around */
		if( !currServer || _DPSFindListItemByItem( &_DPSOutputContexts,
						STD_TO_OUTPUT( currServer) ))
		    DPSSetContext( currServer );	/* restore server */
		calledProc = TRUE;
		st = nextSt;
	    } else
		st = st->next;
	} while( st && !except.code );
	if( except.code )
	    RERAISE_ERROR( NULL, &except );
    }
    return calledProc;
}


/* does a select on fds, and sends a message when there is activity */
static any_t
selectLoop( any_t arg )
{
/* ??? rpc if only one fd */
    fd_set mask;
    extern int errno;
    int inPipe = SelectPipe[0];
    int numFound;
#define DUM_SIZE  128
    char dumBuf[DUM_SIZE];	/* buf to clean out pipe from main thread */
    int numRead;
    char *c;
    int numRPCsToDo = 0;
    msg_header_t msg;
    msg_return_t ret;
    port_t selectPort = (port_t)arg;

    msg.msg_simple = TRUE;
    msg.msg_size = sizeof(msg_header_t);
    msg.msg_type = MSG_TYPE_NORMAL;
    msg.msg_local_port = PORT_NULL;
    msg.msg_remote_port = selectPort;
    while(1) {
	mask = WaitingStreams;
	if( (numFound = select( MaxFD+1, &mask, NULL, NULL, NULL )) == -1 ) {
	    ASSERT( errno == EINTR, "select error in selectLoop thread" );
	} else {
	    if( FD_ISSET( inPipe, &mask )) {
	      /* empty out connection buffer, looking for RPC requests */
		while( (numRead = read( inPipe, dumBuf, DUM_SIZE )) > 0 )
		    for( c = dumBuf; numRead--; )
			if( *c++ == MSG_RPC )
			    numRPCsToDo++;
		if( numRPCsToDo ) {
		    ASSERT( numRPCsToDo == 1, "dropping RPCS in selectLoop" );
		    numRPCsToDo = 0;
		    msg.msg_remote_port = SelectReplyPort;
#ifdef DEBUG
		    if( msg_send( &msg, MSG_OPTION_NONE, 0 ) != SEND_SUCCESS )
			ASSERT( FALSE, "send #1 failed in selectLoop" );
#else
		    (void)msg_send( &msg, MSG_OPTION_NONE, 0 );
#endif
		    msg.msg_remote_port = selectPort;
		}
		if( --numFound )	/* if more bits to be checked */
		    FD_CLR( inPipe, &mask );
	    }
	    if( numFound ) {
		mutex_lock( &MaskLock );
	      /* make sure all set bits are still legit */
		FD_AND( &mask, &WaitingStreams )
	      /* transfer bits from waiting to active status */
		FD_NAND( &WaitingStreams, &mask )
		FD_OR( &ActiveStreams, &mask )
		mutex_unlock( &MaskLock );
		ret = msg_send( &msg, SEND_TIMEOUT, 0 );
		if( ret == SEND_INVALID_PORT )
		    break;
#ifdef DEBUG
		else if( ret != SEND_SUCCESS && ret != SEND_TIMED_OUT )
		    ASSERT( FALSE, "send #2 failed in selectLoop" );
#endif
	    }
	}
    }
    return 0;
}


/* launches a thread to do a select */
static void
launchSelectThread()
{
    cthread_t newThread;
    port_t selectPort;

    if( pipe( SelectPipe ) == -1 )
	goto just_bombout;
  /* make non-blocking on both sides */
    fcntl( SelectPipe[0], F_SETFL,
		    (fcntl( SelectPipe[0], F_GETFL, 0 ) | FNDELAY));
    fcntl( SelectPipe[1], F_SETFL,
		    (fcntl( SelectPipe[1], F_GETFL, 0 ) | FNDELAY));
    if( port_allocate( task_self(), &selectPort ) != KERN_SUCCESS )
	goto cleanup_pipe_bombout;
    port_set_add( task_self(), MainPorts, selectPort );
    if( port_allocate( task_self(), &SelectReplyPort ) != KERN_SUCCESS )
	goto cleanup_port_bombout;
  /* one message is enough, since we're using messages as signals */
    port_set_backlog( task_self(), selectPort, 1 );
    FD_ZERO( &WaitingStreams );
    FD_ZERO( &ActiveStreams );
    FD_SET( SelectPipe[0], &WaitingStreams );
    MaxFD = SelectPipe[0];
    mutex_init( &MaskLock );
    if( !(newThread = cthread_fork( selectLoop, (any_t)selectPort )))
	goto cleanup_port_bombout;
#ifdef DEBUG
    cthread_set_name( newThread, "Select Thread" );
#endif
    cthread_detach( newThread );
    return;

cleanup_port_bombout:
    port_deallocate( task_self(), selectPort );
cleanup_pipe_bombout:
    close( SelectPipe[0] );
    close( SelectPipe[1] );
just_bombout:
    RAISE_ERROR( 0, dps_err_outOfMemory, (void *)sizeof(SelectPipe), 0 );
}


/*  Adds fd to our list of things to select on.
    Having more than one routine per fd is not allowed.
*/
void
DPSAddFD( fd, routine, userData, priority )
int fd;
DPSFDProc routine;
void *userData;		/* some context to be passed to routine */
int priority;
{
    register _DPSStream *st;
    char notifyMsg = MSG_SIMPLE;

    ASSERT( routine, "NULL routine in DPSAddFD" );
    ASSERT( priority >= 0 && priority < 31, "Bad priority in DPSAddFD" );
    if( (st = _DPSFindListItemByKey( &Streams, fd )) && st->doFree == FALSE )
	RAISE_ERROR( 0, dps_err_invalidFD, (void *)fd, 0 );
    _DPSInitMainPortSet();
    if( SelectPipe[0] == -1 )
	launchSelectThread();
    st = _DPSNewListItem( &Streams, sizeof(_DPSStream), NULL );
    mutex_lock( &MaskLock );
    FD_SET( fd, &WaitingStreams );
    mutex_unlock( &MaskLock );
    st->fd = fd;
    st->refs = 0;
    st->doFree = FALSE;
    st->routine.func = routine;
    st->routine.userData = userData;
    st->routine.inUse = FALSE;
    st->routine.server = DPSGetCurrentContext();
    st->routine.priority = convertPriority( priority );
    if( fd > MaxFD )
	MaxFD = fd;
  /* signal select that we have a new fd */
#ifdef DEBUG
    if( write( SelectPipe[1], &notifyMsg, 1 ) == -1 )
	ASSERT( FALSE, "write to selectLoop failed in DPSAddFD" );
#else
    (void)write( SelectPipe[1], &notifyMsg, 1 );
#endif
}


/*  Removes an fd from the list we're selecting on.
    Return an error if the stream isn't in the list now.
*/
void
DPSRemoveFD(fd)
int fd;
{
    _DPSStream *st;
    char notifyMsg = MSG_RPC;
    msg_header_t replyMsg;

    if( st = _DPSFindListItemByKey( &Streams, fd )) {
	replyMsg.msg_simple = TRUE;
	replyMsg.msg_size = sizeof(msg_header_t);
	replyMsg.msg_type = MSG_TYPE_NORMAL;
	replyMsg.msg_local_port = SelectReplyPort;
	mutex_lock( &MaskLock );
	FD_CLR( fd, &WaitingStreams );
	FD_CLR( fd, &ActiveStreams );
	mutex_unlock( &MaskLock );
      /* signal select thread.  Must wait for reply to know when its safe
         to close the fd (and maybe even open another).
       */
	if( write( SelectPipe[1], &notifyMsg, 1 ) == 1 )
	    msg_receive( &replyMsg, MSG_OPTION_NONE, 0 );
	else
	    ASSERT( FALSE, "write to selectLoop failed in DPSAddFD" );
	if( st->refs )		/* if someone up the stack has this ptr */
	    st->doFree = TRUE;
	else
	    (void)_DPSRemoveListItem( &Streams, st, TRUE );
	if( MaxFD == fd )
	    for( st = (_DPSStream *)Streams.head, MaxFD = SelectPipe[0]; st;
								st = st->next )
		if( st->fd > MaxFD )
		    MaxFD = st->fd;
    } else
	RAISE_ERROR( 0, dps_err_invalidFD, (void *)fd, 0 );
}


/*  Adds a TimedEntry to the TimedEntry list.  Returns -1 for failure,
    or the new timed entry for success.
*/
DPSTimedEntry
DPSAddTimedEntry( period, handler, userData, priority )
double period;
DPSTimedEntryProc handler;
void *userData;
int priority;
{
    register _DPSTimedEntry *te;

    ASSERT( handler, "NULL routine in DPSAddTimedEntry" );
    ASSERT( priority >= 0 && priority < 31,
				"Bad priority in DPSAddTimedEntry" );
    _DPSInitMainPortSet();
    te = _DPSNewListItem( &TimedEntries, sizeof(_DPSTimedEntry), NULL );
    te->routine.func = handler;
    te->routine.userData = userData;
    te->routine.inUse = FALSE;
    te->routine.server = DPSGetCurrentContext();
    te->routine.priority = convertPriority( priority );
    te->refs = 0;
    te->doFree = FALSE;
    te->interval = period;
    te->nextRun = getTime() + period;
    return te;
}


/*  removes a given TimedEntry */
void
DPSRemoveTimedEntry(te)
register DPSTimedEntry te;
{
    if( te->refs )		/* if someone up the stack has this ptr */
	te->doFree = TRUE;
    else if( !_DPSRemoveListItem( &TimedEntries, te, TRUE ))
        RAISE_ERROR( 0, dps_err_invalidTE, te, 0 );
}


/*  Runs any timed entries that are runnable.  Returns whether any
    procs were actually run.
*/
static int
checkTEs (double *nowReturn, unsigned long thresh)
{
    register _DPSTimedEntry *te, *nextTe;
    double now;
    int calledProc = FALSE;
    register DPSContext currServer, teServer;
    NXHandler except;		/* params for raising an exception */

    except.code = 0;
    now = getTime();
    if( te = (_DPSTimedEntry *)TimedEntries.head )
	do {
	    if( !te->routine.inUse && !te->doFree && now >= te->nextRun && 
				te->routine.priority > thresh ) {
		te->routine.inUse = TRUE;
		teServer = te->routine.server;
		currServer = DPSGetCurrentContext();  /* save current server */
		DPSSetContext( teServer );
		te->refs++;	/* protect case of removal within proc */
		NX_DURING
		    _NXSendPmonEvent(_DPSPmonSrcData, pmon_te_proc, (int)te,
						0, (int)(te->interval*1000));
		    (te->routine.func)( te, now, te->routine.userData );
		    _NXSendPmonEvent(_DPSPmonSrcData, pmon_te_proc, (int)te,
						1, (int)(te->interval*1000));
		NX_HANDLER
		    except = NXLocalHandler;	/* save to re-raise later */
		NX_ENDHANDLER
		te->refs--;
		now = getTime();
		nextTe = te->next;
	      /* if removed out from under us in the proc */
		if( te->doFree && !te->refs )
		    (void)_DPSRemoveListItem( &TimedEntries, te, TRUE );
		else {
		    te->nextRun = now + te->interval;  /* set next time */
		    te->routine.inUse = FALSE;
		}
		if (!except.code &&
			_DPSFindListItemByItem( &_DPSOutputContexts,
						STD_TO_OUTPUT(teServer))) {
		    _NXSetPmonSendMode(teServer, pmon_proc);
		    flushContextAndAckWC(teServer);
		}
	      /* if the the old server is still around */
		if( !currServer || _DPSFindListItemByItem( &_DPSOutputContexts,
						STD_TO_OUTPUT( currServer) ))
		    DPSSetContext( currServer );	/* restore server */
		calledProc = TRUE;
		te = nextTe;
	    } else
		te = te->next;
	} while( te && !except.code );
    *nowReturn = now;
    if( except.code )
	RERAISE_ERROR( NULL, &except );
    return calledProc;
}


/*  gets the difference from now to wait in the msg_receive. */
static double
getTEInterval (double maxTime, double now, unsigned long thresh)
{
    register _DPSTimedEntry *te;
    double ret;

    for( te = (_DPSTimedEntry *)TimedEntries.head; te; te = te->next ) {
	if( te->nextRun < maxTime && te->routine.priority > thresh )
	    maxTime = te->nextRun;
    }
    ret = maxTime - now;
    return ret > 0.0 ? ret : 0.0;
}


/*  Adds a port to the list of ports we know about.  Returns -1 for failure. */
void
DPSAddPort( newPort, handler, maxSize, userData, priority )
port_t newPort;
DPSPortProc handler;
int maxSize;
void *userData;
int priority;
{
    register _DPSPort *p;
    register int i;

    ASSERT( handler, "NULL routine in DPSAddFD" );
    ASSERT( priority >= 0 && priority < 31, "Bad priority in DPSAddPort" );
    if( (p = _DPSFindListItemByKey( &Ports, newPort )) &&
							p->doFree == FALSE)
	RAISE_ERROR( 0, dps_err_invalidPort, (void *)newPort, 0 );
    _DPSInitMainPortSet();
    p = _DPSNewListItem( &Ports, sizeof(_DPSPort), NULL );
    if( newPort == _DPSNotifyPort ) {
	NotifyInMainPorts = &(p->inMainSet);
	NotifyInPortsList = TRUE;
    }
    MainSetCount++;		/* re-evaluate who's in the main set */
    p->routine.func = handler;
    p->routine.userData = userData;
    p->routine.inUse = FALSE;
    p->routine.server = DPSGetCurrentContext();
    p->routine.priority = convertPriority( priority );
    p->port = newPort;
    p->inMainSet = FALSE;
    p->refs = 0;
    p->doFree = FALSE;
    if (maxSize > _DPSMsgSize)
	_DPSMsgSize = maxSize;
}


/*  removes a given port */
void
DPSRemovePort( machPort )
port_t machPort;
{
    _DPSPort *p;

    if( p = _DPSFindListItemByKey( &Ports, machPort )) {
	if( machPort == _DPSNotifyPort ) {
	    NotifyInPortsList = FALSE;
	    NotifyInMainPorts = &NotifyInMainPortsStorage;
	    NotifyInMainPortsStorage = p->inMainSet;
	} else if( p->inMainSet )
	    port_set_remove( task_self(), p->port );
	if( p->refs ) {		/* if someone up the stack has this ptr */
	    p->doFree = TRUE;
	    p->inMainSet = FALSE;
	} else
	    (void)_DPSRemoveListItem( &Ports, p, TRUE );
    } else
	RAISE_ERROR( 0, dps_err_invalidPort, (void *)machPort, 0 );
}


/* converts an integer priority into a mask.  We add one to these so that
   we can compare them against a threshold mask with a > test.
 */
static unsigned long
convertPriority (int intP)
{
    if( intP < 0 )
	intP = 0;
    else if( intP >= 31 )
	intP = 30;
    return 1 << ++intP;
}


/* returns the time in seconds since we started the app.  A 52 bit mantissa
   in a IEEE double gives us 16-17 digits of accuracy.  Taking off six digits
   for the microseconds in a struct timeval, this leaves 10 digits of seconds,
   which is ~300 years.
*/
static double
getTime()
{
    struct tsval ts;
#define MAX_TIME ((double)0xffffffff)

    kern_timestamp(&ts);
    return (((double)ts.high_val) * MAX_TIME + ((double)ts.low_val))/1000000;
}


/* initializes the main port set.  Called whenever theres a new port we'll
   need to listen to, so we know the main port set has been created.
 */
void
_DPSInitMainPortSet()
{
#if defined(DEBUG) && defined(PMON)
    _NXInitPmon();
#endif
  /* to get notified if someone that we can send to dies */
    if( !_DPSNotifyPort ) {
	_DPSNotifyPort = task_notify();
	if( !_DPSNotifyPort ) {
	    if( port_allocate( task_self(), &_DPSNotifyPort ) != KERN_SUCCESS )
		RAISE_ERROR( 0, dps_err_outOfMemory, (void *)sizeof(port_t), 0 );
	    task_set_special_port( task_self(), TASK_NOTIFY_PORT, _DPSNotifyPort );
	}
    }
    if( !MainPorts )
	if( port_set_allocate( task_self(), &MainPorts ) != KERN_SUCCESS )
	    RAISE_ERROR(0, dps_err_outOfMemory,
				(void *)sizeof(port_set_name_t), 0);
}


/* Puts the notify port in the given port set, remembering that its no
   longer in the main port set.
 */
void
_DPSAddNotifyToSet( port_set_name_t set )
{
    ASSERT( set != MainPorts, "_DPSAddNotifyToSet called with main set" );
    port_set_add( task_self(), set, _DPSNotifyPort );
    MainSetCount++;
    *NotifyInMainPorts = FALSE;
}


static void flushContextAndAckWC(DPSContext ctxtArg)
{
    _DPSMachContext machCtxt;
    int ackTime;

    if (ctxtArg->type == dps_machServer) {
	machCtxt = STD_TO_MACH(ctxtArg);
	if (machCtxt->flags.didStartWCTimer && !machCtxt->lastEventTime)
	    ackTime = 1;	/* force an ack to be sent */
	else
	    ackTime = machCtxt->lastEventTime;
	_DPSFlushStream(machCtxt->outStream, FALSE, ackTime);
	machCtxt->lastEventTime = 0;
	machCtxt->flags.didStartWCTimer = FALSE;
    } else
	DPSFlushContext(ctxtArg);
}


void _DPSListenToNewContexts(void)
{
    MainSetCount++;
}
