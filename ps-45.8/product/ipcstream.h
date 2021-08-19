/*
  ipcstream.h
	
  Stream implementation for Mach-IPC-based streams.
  
  Created Leovitch 11Sep88
  
  Modified:
    22May90 Trey  now using stream.data.b for wait cursor state
  
CONFIDENTIAL
Copyright (c) 1988 NeXT, Inc.
All rights reserved.

*/

#import PACKAGE_SPECS
#import PUBLICTYPES
#import ENVIRONMENT
#import PSLIB
#import STREAM
#import MOUSEKEYBOARD

#undef MONITOR
#import <sys/types.h>
#import <mach.h>
#import <sys/message.h>

/* IPC class implementations of the stream operations.
   IPC streams are always either read or write, never both.
   For IPC streams, data.a is a pointer to an IPCData
   structure, filled in as outlined below.
   
   IPC streams interface with the scheduler as follows:
   
   When read streams have consumed all input from a message,
   they call PSYield(yield_stdin,&msgP) where msgP
   is a pointer to the message just consumed (or NULL
   at initialization).  They expect this PSYield call
   only to return when another message on the stream's
   stmPort (as below) has arrived.
   
   When write streams encounter a full message queue,
   they call PSYield(yield_stdout,port).  They expect
   this call to not return until a notify message has
   been received on the given port.
   
   stm->flags.f1, for read streams, indicates whether the
   stream's port is restricted (true) or unrestricted (false).
   
   stm->flags.f2, for write streams, indicates whether some
   previous msg_send operation has resulted in a SND_WILL_NOTIFY
   response.  If this is true, no further msg_sends should be 
   attempted until the the IPCNotifyReceived function has been called.
   */
   
#define stmRestricted f1
#define notifyInProgress f2

typedef struct _IPCData {
	port_t	port;	/* for write streams: nothing
			   for read streams: my port		*/
	msg_header_t *m;/* for write streams: pointer to outgoing
			   message, always valid
			   for read streams: pointer to message
			   being read out of if in progress, or NULL */
	msg_type_long_t *mlt;
			/* same */
	msg_type_long_t *nmt;
			/* The next time we come pick up a type out of
			   this message, where should it be.  This may
			   or may not be inside the bounds of the msg;
			   must check against msg_size in header.  Read
			   streams only.  */
	int	ref;	/* for read streams: initial size of the chunk
			   of data found in the type the last time
			   FillBuf was called.
			   for write streams: refCount.  Do not 
			   close until it reaches 0		*/
} IPCData;

/* For the message used as a buffer by write streams... */
#define MESSAGETOTALSIZE 1000	/* Length of message buffer */
#define MESSAGEDATASIZE (MESSAGETOTALSIZE-sizeof(msg_header_t) \
		-sizeof(msg_type_long_t))
			/* Size of data portion of message */

/* Values for the msg_id field of messages sent to ipc streams */
#define STREAM_DATA_MESSAGE_ID 3049
				/* The msg_id value for stream data */
#define EOF_MESSAGE_ID 3050	/* The msg_id value indicating end of input */
#define STREAM_DATA_WITH_ACK_ID 3051
				/* The msg_id vlaue for stream data that is
				   to be ack'ed when the buffer is empty */
#define DATA_ACK_MESSAGE_ID 3052
				/* The value in the ack message */
#define NEXTIMAGE_ID 3053
#define DATA_EV_ID 3054		/* msg with data and updated ev consumed val */
#define DATA_EV_ACK_ID 3055	/* DATA_EV_ID + request for ack when done */
#define DATA_EV_ACK_MSG_ID 3056	/* reply to DATA_EV_ACK_ID */
#define FIRST_MESSAGE_ID STREAM_DATA_MESSAGE_ID
#define LAST_MESSAGE_ID DATA_EV_ACK_MSG_ID

/* Macros to access the above without too much hassle
   All assume a local variable called 'stm'.
   */
#define	stmData ((IPCData *)stm->data.a)
#define stmPort (stmData->port)
#define stmMsg	(stmData->m)
#define stmType	(stmData->mlt)
#define stmSType ((msg_type_t *)stmType)
#define stmNType (stmData->nmt)
#define stmRef	(stmData->ref)
#define stmCount (stmData->ref)
#define	stmWCParams ((WCParams *)stm->data.b)

extern StmProcs ipcStmProcs;

/* Special IPC Stream functions */

/* The IPCInitializeStm routine
   must be called when the stm is created to set up initial
   state; it relies on the caller having set the read and
   write flags to determine what it should do for initialization.
   If it is called to initialize a read stream, it sets up 
   a port to receive messages on.  That port is available as
   stmPort afterwards.
   If it is called to set up a write stream, it should be
   passed the remote port to which output from this stream
   is to be sent.  This will be recorded within the stream.
   */

extern int IPCInitializeStm(/* stm,remote_port */);
/*
	IPCNotifyReceived informs the stream package that a
	notify message to re-enable send has been received
	for the stream's destination port.  This is only valid
	for write streams.
	*/
extern int IPCNotifyRecevied(/* stm */);

public WCParams *IPCGetWCParams(Stm stm);
/*  Returns a pointer to this streams wait cursor parameters.  If the stream
    isn't an IPC stream, returns NULL.
    */
