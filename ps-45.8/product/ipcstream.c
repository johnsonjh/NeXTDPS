/*****************************************************************************

    ipcstream.c
    Stream implementation for Mach-IPC-based streams.
  
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. All rights reserved.

    Created 09Sep88 Leo
  
    Modified:
  
    24Sep88 Leo   Added cheap ping option
    28Sep88 Leo   Delete send rights on close
    19Jan89 Leo   Multiple types/message
    22Sep89 Dave  changed to handle multiple context types.
    16Nov89 Dave  added I/O byte counters.
    07Nov89 Terry CustomOps conversion
    04Dec89 Ted   Integratathon! 
    06Dec89 Ted   ANSI C Prototyping.
    28Feb90 Terry Integrated Leo's msgImageBitmap changes
    06May90 Trey  Added wait cursor code
    06May90 Ted   Filled in ev driver shmem assignments
    22May90 Trey  added wait cursor support
			call to wait cursor hook in IPCFill
			allocs and frees wait cursor state along with stream
    05Jun90 Dave  Moved CreateIPCStreams to this file from listener.c
    24Sep90 Ted   Made mods for bug 8959: startwaitcursortimer.
    06Oct90 Terry Fixed 10355: missing argument to os_calloc (from 8959 fix)
    25Sep90 Ted   Added call to PingAsyncDrivers() in IPCFillBuf().

******************************************************************************/

#import PACKAGE_SPECS
#import PUBLICTYPES
#import ENVIRONMENT
#import EXCEPT
#import POSTSCRIPT
#import PSLIB
#import STREAM
#import BINTREE
#undef MONITOR
#import <mach.h>
#import <sys/message.h>
#import "ipcstream.h"
#import "ipcscheduler.h"

extern PSContext MousePSContext;
extern PSContext ActiveApp;
void ReceiveNextImage(msg_header_t *m);

    /* current running non-scheduler context (in ipcscheduler.c) */
unsigned int IPCbytesIn = 0;  /* Total number of bytes rec'd by the server */
unsigned int IPCbytesOut = 0; /* Total number of bytes sent by the server */

public procedure CreateIPCStreams(port_t remote_port, Stm *inS, Stm *outS)
{
    register Stm stmIn = NIL, stmOut = NIL;

    DURING
	if ((stmIn = StmCreate(&ipcStmProcs, 0)) == NULL)
	    PSLimitCheck();
	if ((stmOut = StmCreate(&ipcStmProcs, 0)) == NULL)
	   PSLimitCheck();
	stmIn->flags.read = 1; stmOut->flags.write = 1;
	IPCInitializeStm(stmIn, 0);
	IPCInitializeStm(stmOut, remote_port);
	*inS = stmIn;
	*outS = stmOut;
    HANDLER
    {
	if (stmIn) StmDestroy(stmIn);
	if (stmOut) StmDestroy(stmOut);
	RERAISE;
    }
    END_HANDLER;
}

/*****************************************************************************
    The IPCInitializeStm routine must be called when the stm is created to set
    up initial state; it relies on the caller having set the read and write
    flags to determine what it should do for initialization. If it is called
    to initialize a read stream, it sets up a port to receive messages on.
    That port is available as stmPort afterwards. If it is called to set up a
    write stream, it should be passed the remote port to which output from
    this stream is to be sent.  This will be recorded within the stream.
******************************************************************************/

public int IPCInitializeStm(Stm stm, port_t remote_port)
{
    stmData = (IPCData *)os_malloc(sizeof(IPCData));
    if (!stmData) PSLimitCheck();
    stmWCParams = (WCParams *)os_calloc(sizeof(WCParams), 1);
    if (!stmWCParams) PSLimitCheck();
    stmWCParams->waitCursorEnabled = 1;
    if (stm->flags.write) stmRef = 1;
    if (stm->flags.read) { /* Allocate a port to receive messages on */
	if (port_allocate(task_self_, &stmPort) != KERN_SUCCESS)
	    PSLimitCheck();
	stmMsg = (msg_header_t *)NULL;
	stmType = (msg_type_long_t *)NULL;
	stm->flags.stmRestricted = 1;
    } else { /* Allocate a message buffer to use for sends */
    	msg_header_t *m;
	msg_type_t *mt;
	msg_type_long_t *mlt;
	
    	stmMsg = m = (msg_header_t *)os_malloc(MESSAGETOTALSIZE);
	if (!m) PSLimitCheck();
	m->msg_simple = 1;
	m->msg_size = MESSAGETOTALSIZE;
	m->msg_type = MSG_TYPE_NORMAL;
	m->msg_local_port = 0;
	m->msg_remote_port = remote_port;
	m->msg_id = STREAM_DATA_MESSAGE_ID;
	stmType = mlt = (msg_type_long_t *)(((char *)m)+sizeof(msg_header_t));
	mlt->msg_type_header.msg_type_inline = 1;
	mlt->msg_type_header.msg_type_longform = 1;
	mlt->msg_type_header.msg_type_deallocate = 0;
	mlt->msg_type_long_size = 8;
	mlt->msg_type_long_name = MSG_TYPE_CHAR;
	stm->ptr = stm->base = ((char *)mlt) + sizeof(msg_type_long_t);
	stm->cnt = MESSAGEDATASIZE;
	stm->flags.notifyInProgress = 0;
    }
    return(0);
}

/* Verifies a Mach data item.  The item must be longform and its type must
   match the one passed in.  The number of items must also match (-1 is a
   dont-care value).  Returns a pointer just after type and data if the
   type passes these tests, else NULL.  */
private msg_type_long_t *verifyMachType(msg_type_long_t *data,
						int type, int num)
{
    msg_type_long_t *mlt;
    int dataSize;

    mlt = (msg_type_long_t *)data;
    if (mlt->msg_type_header.msg_type_longform &&
		(mlt->msg_type_long_name == type) &&
		(num == -1 || mlt->msg_type_long_number == num)) {
	dataSize = mlt->msg_type_header.msg_type_inline ?
			mlt->msg_type_long_number * mlt->msg_type_long_size/8 :
			sizeof(char *);
	/* Note:  Although it isn't documented anywhere, the
	   next type in the message will not begin after the
	   dataSize bytes of data, but rather after that many
	   bytes of data _rounded up to a multiple of 4_. */
	dataSize = ((dataSize+3)/4)*4;
	return (msg_type_long_t *)(((char *)(mlt+1)) + dataSize);
    } else
	return NULL;
}

/* acks a client message, with or without ack data.  A true response indicates
   the message was sent sucessfully.  */
private int sendAck(msg_header_t *msg)
{
    struct {
	msg_header_t head;
	msg_type_long_t type;
	int data;
    } ackMsg;
    
    /* Ping all asynchronous framebuffer drivers, if any exist. */
    if (asyncDriversExist)
	PingAsyncDrivers();
    ackMsg.head.msg_simple = 1;
    ackMsg.head.msg_type = 0;
    ackMsg.head.msg_local_port = msg->msg_local_port;
    ackMsg.head.msg_remote_port = msg->msg_remote_port;
    if (msg->msg_id == STREAM_DATA_WITH_ACK_ID) {
	ackMsg.head.msg_size = sizeof(msg_header_t);
	ackMsg.head.msg_id = DATA_ACK_MESSAGE_ID;
    } else {
	ackMsg.head.msg_size = sizeof(ackMsg);
	ackMsg.head.msg_id = DATA_EV_ACK_MSG_ID;
	os_bcopy(((char *)(((msg_type_long_t *)(msg+1))+1))+sizeof(int),
			&ackMsg.type, sizeof(msg_type_long_t) + sizeof(int));
    }

    switch(msg_send((msg_header_t *)&ackMsg, SEND_NOTIFY, 0)) {
	case SEND_SUCCESS:
	    return true;
	case SEND_WILL_NOTIFY:
	    PSYield(yield_stdout, ackMsg.head.msg_remote_port);
	    return true;
	default:
	    return false;
    } /* switch(msg_send(...)) */
}

private int IPCFillBuf(Stm stm)
{
    int a;
    msg_header_t *m;
    msg_type_long_t *mlt;
    YieldReason because;
    
    if (stm->flags.eof || (!stm->flags.read)) return(EOF);
    /* Check to see if the type we parsed last time has out-of-line
       data.  If so, deallocate it */
    if (stmMsg && stmType && !stmSType->msg_type_inline && stmCount)
    	vm_deallocate(task_self_, (vm_address_t)*(char **)(stmType+1),
	    (vm_size_t) stmCount);
    while (stm->cnt <= 0) { /* Should always be true at entry */
    	/* Keep going until you have a message with an uncomsumed type
	   structure in it */
	while ((!stmMsg) ||
	(((char *)stmNType) >= (((char *)stmMsg)+stmMsg->msg_size))) {
	    if (!stmMsg) {
		m = (msg_header_t *)stmPort;
		because = yield_other;
	    } else {
		/* OK, we've finished the last message and are coming
		   around for another one.  See if we have to ack the
		   last one */
		m = stmMsg;
		if (m->msg_id == STREAM_DATA_WITH_ACK_ID)
		{   /* Ack this message */
		    if (!sendAck(m))
		    {
			stm->flags.error = 1;
			return EOF;
		    }
		}
		else if (m->msg_id == DATA_EV_ID ||
				m->msg_id == DATA_EV_ACK_ID)
		{
		    WCReceiveEvent(stmWCParams,
				ActiveApp && ActiveApp->in == stm,
				*(int *)(((msg_type_long_t *)(m+1))+1));
		    if (m->msg_id == DATA_EV_ACK_ID)
		    {
		    	if (!sendAck(m))
			{
			    stm->flags.error = 1;
			    return EOF;
			}
		    }
		}
		/* OK, we've acked it if necessary.  Now go to PSYield
		   to get another message; we're through with this one. */
		because = yield_stdin;
		stmMsg = NULL;
	    }
	    /* Now wait for a message.  If we get one whose msg_id is outside
	       the acceptable range, just wait for another one. */
	    while (1) {
		PSYield(because,&m);
		
		/* KLUDGE! Grab any image msg and send to Image msg proc */
		if (m->msg_id == NEXTIMAGE_ID)
		{
		    ReceiveNextImage(m);
		    m = (msg_header_t *)stmPort;
		    because = yield_other;
		}
		else
		{
		    if ((m->msg_id >= FIRST_MESSAGE_ID) &&
		         (m->msg_id <= LAST_MESSAGE_ID) &&
		         (m->msg_id != DATA_ACK_MESSAGE_ID) &&
		         (m->msg_id != DATA_EV_ACK_MSG_ID))
		        break;
		    else
			because = yield_stdin;
		}
	    }
	    stmMsg = m;
	    /* OK, it's a real message, predict location of next
	       type struct in it, then go around while loop to make
	       sure message actually has data.  For the EV message types
	       we also check to make sure that the first 1 or 2 items
	       in the message are ints.  We then skip over them, so as
	       to leave stmNType pointing to the first type containing
	       real stream data. */
	    stmNType = NULL;
	    switch(m->msg_id) {
		case EOF_MESSAGE_ID:
		    stm->flags.eof = 1;
		    ReleaseMsg(m); /* from IPCSscheduler.c */
		    stmMsg = NULL;
		    return(EOF);
		case STREAM_DATA_MESSAGE_ID:
		case STREAM_DATA_WITH_ACK_ID:
		case NEXTIMAGE_ID:
		    stmNType = (msg_type_long_t *)(stmMsg+1);
		    break;
		case DATA_EV_ID:
		    stmNType = verifyMachType((msg_type_long_t *)(stmMsg+1),
						MSG_TYPE_INTEGER_32, 1);
		    break;
		case DATA_EV_ACK_ID:
		    stmNType = verifyMachType((msg_type_long_t *)(stmMsg+1),
						MSG_TYPE_INTEGER_32, 1);
		    if (stmNType)
			stmNType = verifyMachType(stmNType,
						    MSG_TYPE_INTEGER_32, 1);
		    break;
	    }
	    if (stmNType == NULL) {
		stm->flags.error = 1;
		stm->cnt = 0;
		return (-1);	/* ??? Do we realize this is the same as EOF?  Will we leak the message?  When we return EOF above we release the message. */
	    }
	}
	/* OK, stmNType points at a new type.  Advance all pointers to
	   that type, and predict new NType. */
	mlt = stmType = stmNType;
	stmNType = verifyMachType(mlt, MSG_TYPE_CHAR, -1);
	if (!stmNType)
	    stmNType = verifyMachType(mlt, MSG_TYPE_BYTE, -1);
	if (stmNType) {
	    stm->cnt = stmCount = (mlt->msg_type_long_size>>3)*
					    mlt->msg_type_long_number;
	    IPCbytesIn += stm->cnt;
	    if (mlt->msg_type_header.msg_type_inline)
		stm->ptr = (char *)(mlt+1);
	    else
		stm->ptr = *(char **)((char *)(mlt+1));
	} else {   /* All messages should be longform! */
	    stm->flags.error = 1;
	    stm->cnt = 0;
	    return(-1);		/* ??? Do we realize this is the same as EOF?  Will we leak the message?  When we return EOF above we release the message. */
	}
    } /* while (stm->cnt <= 0) */
    stm->cnt--;
    return(unsigned char)*stm->ptr++;
} /* IPCFillBuf */

/*****************************************************************************
    IPCFlushOutput implements the flush operation for a stream guaranteed
    to be a write stream. This routine will attempt to write as long as there
    is data in the output buffer. For each attempt, then if there is no notify
    in progress on this stream, it sends the accumulated data. If there was a
    notify in progress or if the msg_send produced  a SEND_WILL_NOTIFY result,
    it attempts to block.  An attempt  to block is a PSYield if permitted by
    the parameter; otherwise, an attempt to block simply causes Flush to
    return 0. IPCFlush assumes that whomever wakes the contexts up out of
    notify condition also calls IPCNotifyReceived, which is what takes care
    of resetting the stm state variables.
******************************************************************************/

public int IPCFlushOutput(Stm stm, boolean yieldPermitted)
{
    kern_return_t ret;
    
    /* Since we may PSYield, keep this up until there's no data */
    while (stm->flags.write && (stm->ptr > stm->base)) {
	if (!stm->flags.notifyInProgress) {
	    stmType->msg_type_long_number = stm->ptr - stm->base;
	    stmMsg->msg_size = sizeof(msg_header_t) + sizeof(msg_type_long_t)
		+ stmType->msg_type_long_number;
	    ret = msg_send(stmMsg, SEND_NOTIFY, 0);
	    switch(ret) {
		case SEND_SUCCESS:     /* In either of these two cases, the */
		case SEND_WILL_NOTIFY: /* message got out there */
		    stm->ptr = stm->base;
		    stm->cnt = MESSAGEDATASIZE;
		    break;
		default:
		    stm->flags.error = 1;
		    return(-1);
	    }
	}
	if (stm->flags.notifyInProgress || (ret == SEND_WILL_NOTIFY)) {
	    stm->flags.notifyInProgress = 1;
	    if (yieldPermitted) {
	    	/* Yield, waiting for a notify.  Afterwards, continue
		   to look for more data in our stream buffer */
		PSYield(yield_stdout,stmMsg->msg_remote_port);
		continue;
	    } else {
	    	/* We can't block, so we will give up.  If we've actually
	           sent all the data, this isn't an error yet (though, note,
		   the next time through this routine we'll get right to
		   here and trigger an error. */
		if (stm->ptr > stm->base) /* Still data? */
		    return(-1);
		else
		    break;
	    }
	}
    } /* while (stm->ptr > stm->base) */
    return(0);
} /* IPCFlushOutput */

/*****************************************************************************
    IPCFlush is the flush operation for IPC streams.  Just calls
    IPCFlushOutput for output streams, tosses input for input streams.
******************************************************************************/

private int IPCFlush(Stm stm)
{
    if (stm->flags.write)
        return(IPCFlushOutput(stm, currentSchedulerContext->writeBlock
	/*currentPSContext->scheduler*/ ));
    /* else read stream */
    if (stm->cnt)
	stm->cnt = 0;
    return(0);
}

/*****************************************************************************
    IPCNotifyReceived is the second half of a PSYield for write.  It is called
    when a notify has been received for the stm's remote port. It will reset
    the stream state variables.  Note that after this is called, the above
    PSYield will return (if we're inside of it). Note also that
    IPCNotifyReceived is intended to be called asynchronously from the thing
    that receives notify messages, thus it will never itself call PSYield.
******************************************************************************/

public int IPCNotifyReceived(Stm stm)
{
    if ((!stm->flags.write)||(stm->procs != &ipcStmProcs)) return(-1);
    if (!stm->flags.notifyInProgress) return(-1);
    stm->flags.notifyInProgress = 0;
    /* Now, if there's more output in the buffer, we need to
       flush it, so call.  Note that we disallow yielding, since
       this routine is called asynchronously. */
    IPCFlushOutput(stm, false);
    return(0);
}

private int IPCFlushBuf(int ch, Stm stm)
{
    int	ret;
    
    if (!stm->flags.write) return(EOF);
    ret = IPCFlush(stm);
    stm->cnt--;
    *stm->ptr++ = ch;
    return(ret);
}

private int IPCClose(Stm stm)
{
    if (!(stm->flags.read || stm->flags.write)) return(-1);
    if (stm->flags.read) {
	if (stmMsg) ReleaseMsg(stmMsg);
    	port_deallocate(task_self(), stmPort);
    } else {
    	if (--stmRef > 0) return(0);
	    /* Don't really close it unless we're the last reference */
    	port_deallocate(task_self_, stmMsg->msg_remote_port);
    	os_free(stmMsg);
    }
    os_free(stmData);
    os_free(stmWCParams);
    stm->data.a = stm->data.b = 0;
    stm->flags.read = stm->flags.write =  0;
    return(0);
}

private int IPCAvail(Stm stm)
{
    return(stm->cnt);
}

private int IPCPutEOF(Stm stm)
{
    if (stm->flags.write) {
	IPCFlush(stm);
	stmMsg->msg_id = EOF_MESSAGE_ID;
	stm->ptr++; stm->cnt--;
	return(IPCFlush(stm));
    } else
	return(EOF);
}

public long int IPCWrite(char *ptr, int itemSize, int nItems, Stm stm)
{
    int chars;
    
    chars = nItems*itemSize;
    
    if ((!stm->flags.write) || (stm->flags.error)) return(-1);
    IPCbytesOut += chars;
    if (chars < MESSAGEDATASIZE) {	/* It can fit in one buffer */
    	while (chars > stm->cnt) if (IPCFlush(stm) == -1) return(-1);
	os_bcopy(ptr,stm->ptr,chars);
	stm->ptr += chars;
	stm->cnt -= chars;
	return(nItems);
    }
    /* It can't fit in one buffer */
    /* So, empty the buffer */
    while (stm->ptr != stm->base) if (IPCFlush(stm) == -1) return(-1);
    /* Now send the data as out-of-line data */
    {
    	struct {
		msg_header_t h;
		msg_type_long_t t;
		char *chars;
	} msg;
	
	msg.h.msg_simple = 0;
	msg.h.msg_size = sizeof(msg);
	msg.h.msg_type = 0;
	msg.h.msg_local_port = 0;
	msg.h.msg_remote_port = stmMsg->msg_remote_port;
	msg.h.msg_id = STREAM_DATA_MESSAGE_ID;
	msg.t.msg_type_header.msg_type_inline = 0;
	msg.t.msg_type_header.msg_type_longform = 1;
	msg.t.msg_type_header.msg_type_deallocate = 0;
	msg.t.msg_type_long_name = MSG_TYPE_CHAR;
	msg.t.msg_type_long_size = 8;
	msg.t.msg_type_long_number = chars;
	msg.chars = ptr;
	if (msg_send((msg_header_t *) &msg, SEND_NOTIFY, 0) ==
	SEND_WILL_NOTIFY) {
	    stm->flags.notifyInProgress = 1;
	    /* if (currentPSContext->scheduler)... */
	    if (currentSchedulerContext->writeBlock)
	        PSYield(yield_stdout, stmMsg->msg_remote_port);
	}
    }
    return nItems;
} /* IPCWrite */

/*****************************************************************************
    Returns a pointer to this streams event parameters.  If the stream
    isn't an IPC stream, returns NULL.
******************************************************************************/

public WCParams *IPCGetWCParams(Stm stm)
{
    return (stm->procs == &ipcStmProcs) ? stmWCParams : NULL;
}

public readonly StmProcs ipcStmProcs = {
  IPCFillBuf, IPCFlushBuf, StmFRead, IPCWrite, StmUnGetc, IPCFlush,
  IPCClose, IPCAvail, StmErr, IPCPutEOF, StmErr, StmErrLong,
  "Mach IPC"};







