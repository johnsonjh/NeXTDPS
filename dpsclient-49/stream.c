#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
    stream.c

    A private implementation of a write-only NXStream.

    Copyright (c) 1989 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#include "dpsclient.h"
#include "defs.h"
#include <string.h>
#include <streams/streams.h>
#include <streams/streamsimpl.h>

#include <mach.h>
#include <sys/message.h>

static int local_write(NXStream *s, const void *buf, int count);
static int local_flush(register NXStream *s);
static int local_nop();
static void local_close(register NXStream *s);
static void send_data(NXStream *s, int doEOF, int doPing, int lastEvent, const void *OOData, int OOSize);
void _DPSMsgSend(DPSContext ctxt, msg_header_t *msg, msg_option_t option);

static const struct stream_functions local_functions = {
    (void *)local_nop,
    local_write,
    local_flush,
    (void *)local_nop,
    (void *)local_nop,
    (void *)local_nop,
    local_close,
};


typedef struct {
    msg_type_long_t type;
    int data;
} IntItem;

typedef struct {
    msg_type_long_t type;
    const void *data;
} OOCharsItem;

static const msg_type_long_t IntTypeTemplate =
    { { 0, 0, 0, TRUE, TRUE, FALSE }, MSG_TYPE_INTEGER_32, 32, 1 };

static const msg_type_long_t CharsTypeTemplate =
    { { 0, 0, 0, TRUE, TRUE, FALSE }, MSG_TYPE_CHAR, 8, 0 };

static const msg_type_long_t OOCharsTypeTemplate =
    { { 0, 0, 0, FALSE, TRUE, FALSE }, MSG_TYPE_CHAR, 8, 0 };


/* ??? TUNE THESE */

/* total size we will allocate for the message buffer.  Try to pick a good malloc size, considering rest of StreamInfo struct.  4088 is a good size. */
#define TOTAL_BUF_SIZE		4088

/* max amount of padding we will need before the data in the message. */
#define MAX_HEADER_SIZE		(sizeof(msg_header_t) + sizeof(msg_type_long_t) + sizeof(int) + sizeof(msg_type_long_t) + sizeof(int) + sizeof(msg_type_long_t) + sizeof(char *) + sizeof(msg_type_long_t) + sizeof(char *))

/* max amount of padding we will need before the data in the message. */
#define MAX_FOOTER_SIZE		(sizeof(msg_type_long_t) + sizeof(char *))

/* the remaining space for actual char data.  Includes all msg_type_long_t's except the first.  */
#define INLINE_BUF_SIZE		(TOTAL_BUF_SIZE - MAX_HEADER_SIZE - MAX_FOOTER_SIZE - sizeof(_DPSMachContext))

/* tradeoff point for sending chars in or out of line.  */
#define IN_OUT_THRESH		(1000 - MAX_HEADER_SIZE)

typedef struct {
    unsigned char header[MAX_HEADER_SIZE];	/* headers before data */
    unsigned char data[INLINE_BUF_SIZE];	/* inline data */
    unsigned char footer[MAX_FOOTER_SIZE];	/* footers after data */
    _DPSMachContext ctxt;			/* ctxt using this stream */
} StreamInfo;


/*
 *	local_write:  write to a stream buffer.
 *
 *	If the data being sent is best sent in line, we let the stream package
 *	buffer what it can, maybe calling our flush.  If we want to send
 *	it out of line, we send it and whats buffered with send_data().
 */
static int local_write(NXStream *s, const void *buf, int count)
{
    int flushSize = s->buf_size - s->buf_left;

  /* if we'll do at most one flush */
    if (count < IN_OUT_THRESH)
	return NXDefaultWrite(s, buf, count);
    else {
	send_data(s, FALSE, FALSE, 0, buf, count);
	return count;
    }
}


/*
 *	local_flush:  flush a stream buffer.
 */
static int local_flush(register NXStream *s)
{
    int flushSize = s->buf_size - s->buf_left;

    if (flushSize)
	send_data(s, FALSE, FALSE, 0, NULL, 0);
    return flushSize;
}


/*
 *	Used for unimplemented stream procs
 */
static int local_nop()	{ }


/*
 *	local_close:	shut down an ipc stream.
 *
 *	Send a zero length packet to signify end.
 */
static void local_close(register NXStream *s)
{
    send_data(s, TRUE, FALSE, 0, NULL, 0);
}


/*
 *	_DPSOpenStream
 *
 *	open a dps stream for a given context
 */
NXStream *_DPSOpenStream( _DPSMachContext ctxt, NXZone *zone )
{
    NXStream *s;
    StreamInfo *newInfo;

    ASSERT(sizeof(StreamInfo) == TOTAL_BUF_SIZE, "StreamInfo struct mis-sized");

    s = NXStreamCreateFromZone(NX_WRITEONLY, FALSE, zone);
    newInfo = NXZoneMalloc(zone, sizeof(StreamInfo));
    newInfo->ctxt = ctxt;

    s->functions = &local_functions;
    s->buf_base = s->buf_ptr = newInfo->data;
    s->buf_size = INLINE_BUF_SIZE;
    s->buf_left = INLINE_BUF_SIZE;
    s->info = (char *)newInfo;

    return s;
}


/*
 *	_DPSPingStream:  Flushes data with a different msg id using msg_rpc,
 *	then waiting for a response on port.
 */
void _DPSFlushStream(NXStream *s, int doPing, int lastEvent)
{
    if (s->buf_size - s->buf_left || doPing || lastEvent) {
	_NXSetPmonSendMode(MACH_TO_STD(((StreamInfo *)(s->info))->ctxt),
							pmon_ping);
	send_data(s, FALSE, doPing, lastEvent, NULL, 0);
    }
}


/*
 *	_DPSResetStream:  Throws away any pending data.
 */
void _DPSResetStream(NXStream *s)
{
    s->buf_ptr = s->buf_base;
    s->buf_left = s->buf_size;
}


/* sends a stream's data.  If the size is below the out-of-line threshold,
 * the data is assumed to be in the message buffer already.
 */
static void send_data(NXStream *s, int doEOF, int doPing, int lastEvent, const void *OOData, int OOSize)
{
    StreamInfo *info = (StreamInfo *)(s->info);
    _DPSMachContext ctxt = info->ctxt;
    msg_header_t *msg;
    int msgSimple;
    int msgLocalPort;
    int msgID;
    msg_option_t msgSendOption;
    int msgSize;			/* size of whole message */
    int bufferSize;			/* size of buffered data */
    int roundBufferSize;		/* bufferSize rounded to mult of 4 */
    int bufferInLine;			/* will buffer be sent in line? */
    unsigned char *curr;		/* pointer we back up though headers */
    OOCharsItem *oocItem;
    msg_type_long_t *cItem;
    IntItem *iItem;

    bufferSize = s->buf_size - s->buf_left;
    roundBufferSize = ((bufferSize+3)/4)*4;
    bufferInLine = bufferSize < IN_OUT_THRESH;

    msgSize = 0;
    msgSimple = TRUE;
    msgLocalPort = PORT_NULL;
    msgID = PS_DATA_MSG_ID;
    curr = info->data;

  /* deal with OOData component.  This either goes before the inline buffer or right after the inline data. */
    if (OOData) {
	if (bufferSize && bufferInLine)
	    oocItem = (OOCharsItem *)(info->data + roundBufferSize);
	else
	    oocItem = (OOCharsItem *)(curr -= sizeof(OOCharsItem));
	oocItem->type = OOCharsTypeTemplate;
	oocItem->type.msg_type_long_number = OOSize;
	oocItem->data = OOData;
	msgSimple = FALSE;
	msgSize += sizeof(OOCharsItem);
    }

  /* deal with buffer component. */
    if (bufferSize) {
	if (bufferInLine) {
	    cItem = (msg_type_long_t *)(curr -= sizeof(msg_type_long_t));
	    *cItem = CharsTypeTemplate;
	    cItem->msg_type_long_number = bufferSize;
	    msgSize += sizeof(msg_type_long_t) + roundBufferSize;
	} else {
	    oocItem = (OOCharsItem *)(curr -= sizeof(OOCharsItem));
	    oocItem->type = OOCharsTypeTemplate;
	    oocItem->type.msg_type_long_number = bufferSize;
	    oocItem->data = info->data;
	    msgSimple = FALSE;
	    msgSize += sizeof(OOCharsItem);
	}
    }

  /* deal with ping context component. */
    if (doPing) {
	iItem = (IntItem *)(curr -= sizeof(IntItem));
	iItem->type = IntTypeTemplate;
	iItem->data = 0;
	msgSize += sizeof(IntItem);
#ifdef SAME_PING_PORT
	msgLocalPort = ctxt->inPort;
#else
	msgLocalPort = ctxt->pingPort;
#endif
    }

  /* deal with event timestamp component. */
    if (lastEvent || doPing) {
	iItem = (IntItem *)(curr -= sizeof(IntItem));
	iItem->type = IntTypeTemplate;
	iItem->data = lastEvent;
	msgID = doPing ? PS_EV_PING_DATA_MSG_ID : PS_EV_DATA_MSG_ID;
	msgSize += sizeof(IntItem);
    }

  /* do the message header */
    msg = (msg_header_t *)(curr - sizeof(msg_header_t));
    msg->msg_simple = msgSimple;
    msg->msg_size = msgSize + sizeof(msg_header_t);
    msg->msg_type = 0;
    msg->msg_local_port = msgLocalPort;
    msg->msg_id = msgID;

    msgSendOption = (msgID == PS_EV_PING_DATA_MSG_ID) ? SEND_SWITCH : 0;

    ASSERT(msg->msg_size <= 1000, "send_data: message too big for PS");
    _DPSMsgSend(MACH_TO_STD(ctxt), msg, msgSendOption);

    s->buf_ptr = s->buf_base;
    s->buf_left = s->buf_size;
    s->offset += bufferSize + OOSize;
}


void _DPSMsgSend(DPSContext ctxtArg, msg_header_t *msg, msg_option_t option)
{
    int sendRet;
    int rcvRet;
    _DPSMachContext ctxt = STD_TO_MACH(ctxtArg);
    _DPSMessage *localMsg = alloca(_DPSMsgSize);

    option |= SEND_TIMEOUT | SEND_INTERRUPT;
    msg->msg_remote_port = ctxt->outPort;
    NX_DURING {
	do {
	    _NXSendPmonEvent(_DPSPmonSrcData, pmon_send_data, size,
				ctxt->flags.sendMode, size > INOUT_THRESH);
	    sendRet = msg_send(msg, option, 300);
	    switch (sendRet) {
		case SEND_SUCCESS:
		    break;
		case SEND_TIMED_OUT:
		case SEND_INTERRUPTED:
		    _NXSetPmonSendMode(MACH_TO_STD(ctxt), pmon_avoid_deadlock);
		    _DPSAssemblePorts(ctxt);
		    do {
			_NXSendPmonEvent(_DPSPmonSrcData, pmon_msg_receive,
					0, pmon_avoid_deadlock, RCV_TIMEOUT);
			rcvRet = _DPSLocalSetMsgReceive(ctxt, localMsg,
							RCV_TIMEOUT, 0);
			_NXSendPmonEvent(_DPSPmonSrcData, pmon_receive_data,
						msg->msg_size,
						pmon_avoid_deadlock, rcvRet);
			_DPSHandleMsg(localMsg, rcvRet, NULL, 0, 0);
		    } while (rcvRet == RCV_SUCCESS);
		    break;
		default:
		    RAISE_ERROR(MACH_TO_STD(ctxt), dps_err_write,
						    (void *)sendRet, 0);
	    }
	} while (sendRet != SEND_SUCCESS);
    } NX_HANDLER {
	_NXSetPmonSendMode(MACH_TO_STD(ctxt), pmon_auto_flush);
	NX_RERAISE();
    } NX_ENDHANDLER
    _NXSetPmonSendMode(MACH_TO_STD(ctxt), pmon_auto_flush);
}
