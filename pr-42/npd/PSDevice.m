/*
 * PSDevice.m	- Implementation of an abstract PostScript device.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */


/*
 * Include files
 */

#import "PSDevice.h"
#import "NpdDaemon.h"
#import "LocalJob.h"
#import "log.h"
#import "mach_ipc.h"

#import <limits.h>
#import <stdlib.h>
#import <string.h>
#import <objc/error.h>


/*
 * Constants
 */

/* Reconnect retry period */
double	RECONNECT_RETRY = 10.0;

/* Error strings */
static struct string_entry _strings[] = {
    { "PSDevice_errnoctxt",
	  "PSDevice context error on non-existant context, ignoring" },
    { "PSDevice_badevent",
	  "PSDevice received ping for non-existant context" },
    { "PSDevice_replyfailed",
	  "PSDevice could not reply to Window Server: error = %d" },
    { "PSDevice_vmdeallocfail",
	  "PSDevice could not vm_deallocate, error = %d" },
    { "PSDevice_jobjob", "PSDevice received setCurrentJob while busy." },
    { "PSDevice_contextlost",
	  "PSDevice lost connection to window server, reconnecting..." },
    { "PSDevice_doublefailure",
	  "Window Server died twice while imaging same page, aborting job" },
    { "PSDevice_nocontext", "PSDevice could not create DPS context" },
    { "PSDevice_pserror", "PostScript error: " },
    { "PSDevice_unknownerror", "PSDevice received unknown PostScript error" },
    { "PSDevice_noport", "PSDevice could not create local port" },
    { "PSDevice_failedbacklog", "PSDevice could not set port backlog" },
    { "PSDevice_contextrestart", "PSDevice reconnected to window server" },
    { NULL, NULL },
};

/* Built in PostScript commands */
static char	*initSave = "\n/NXnpd_save save def\n";
static char	*pingData = "\nAppdefined 0.0 0.0 0 0 currentcontext 0 0 0 "
    "Bypscontext postevent pop\n";
static char	*finishRestore = "\nNXnpd_save restore\n";

/*
 * Local variables.
 */
static BOOL	_initdone = NO;
static int	_portCounter = 0;
static char	*_portTemplate = "NeXT(TM)NpdDevicePort(c)-%d";
static id	_deviceTable;


/*
 * Static routines.
 */


/**********************************************************************
 * Routine:	_errorHandler()	- Handle context errors.
 *
 * Function:	If the error is a PostScript error, we will try
 *		to print the error out on the next page of the device.
 *		If we lost the context, we call the contextLost
 *		method to try to reestablish the context.
 **********************************************************************/
static void
_errorHandler(DPSContext ctxt, DPSErrorCode errorCode,
	      long unsigned int arg1, long unsigned int arg2)
{
    id		device;

    LogDebug("PSDevice _errorHandler called, code = %d", errorCode);

    /*
     * If this is for a context without a device, we call the default
     * error handler.  The routines causing this error must be ready
     * for the NX_RAISE.
     */
    if (!(device = [_deviceTable valueForKey:ctxt])) {
	LogDebug([NXStringManager stringFor:"PSDevice_errnoctxt"]);
	DPSDefaultErrorProc(ctxt, errorCode, arg1, arg2);
    }

    if (errorCode == dps_err_connectionClosed) {
	[device contextLost];
    } else {
	[device errorReceived:errorCode:arg1:arg2];
    }
}	

/**********************************************************************
 * Routine:	_textHandler() - Handle PostScript text output
 *
 * Function:	This routine handles PostScript text output by logging
 *		it as debugging information.  Since we can't assume
 * 		the text is null terminated, we have to copy it,
 *		terminate it, log it, and free it.
 **********************************************************************/
static void
_textHandler(DPSContext context, char *buf, long unsigned int count)
{
    char	*cp;

    cp = (char *)malloc(count + 1);
    cp[count] = '\0';
    if (!buf) {
	buf = "";
    }
    strncpy(cp, buf, count);
    LogDebug("Postscript output: %s", cp);
    free(cp);
}


/**********************************************************************
 * Routine:	_eventHandler() - Handle events coming in.
 *
 * Function:	When we send a page to the interpreter, we append a
 *		postevent operator.  This will cause the interpreter
 *		to let us know when it is done with a page.  This 
 *		routine receives that event and causes the next page
 *		to get sent to the interpreter.
 **********************************************************************/
static int
_eventHandler(NXEvent *event)
{
    id		device;

    device = [_deviceTable valueForKey:event->ctxt];

    if (device) {
	[device pingReceived];
    } else {
	LogError([NXStringManager stringFor:"PSDevice_badevent"]);
    }
    return (0);				/* Discard the event */
}

/**********************************************************************
 * Routine:	_imageqEnqueue()
 *
 * Function:	Add msg to the list of images to be printed.
 **********************************************************************/
static void
_imageqEnqueue(PSDevice *object, NXPrintPageMessage *msg)
{
    imageqent_t		iq;

    iq = (imageqent_t) malloc(sizeof (*iq));
    iq->iq_next = NULL;
    iq->iq_msg = msg;
    if (object->imageq_tail) {
	object->imageq_tail->iq_next = iq;
    } else {
	object->imageq_head = iq;
    }
    object->imageq_tail = iq;
}

/**********************************************************************
 * Routine:	_imageqDequeue()
 *
 * Function:	Dequeue the NXPrintPageMessage from the list of images
 *		to be printed and return it.
 **********************************************************************/
static NXPrintPageMessage *
_imageqDequeue(PSDevice *object)
{
    NXPrintPageMessage	*msg = NULL;
    imageqent_t		iq;

    if (iq = object->imageq_head) {
	msg = iq->iq_msg;
	if (!(object->imageq_head = iq->iq_next)) {
	    object->imageq_tail = NULL;
	}
	free(iq);
    }
    return (msg);
}

/**********************************************************************
 * Routine:	_pageimageReply()
 *
 * Function:	Reply to the NXPrintPageMessage RPC.  This will cause
 *		the Window Server to unblock and continue processing
 *		PostScript.
 **********************************************************************/
static void
_pageimageReply(NXPrintPageMessage *msg, PSDevice *device)
{
    msg_return_t	ret_code;
    msg_header_t	replyMsg;

    replyMsg.msg_simple = 1;
    replyMsg.msg_size = sizeof(msg_header_t);
    replyMsg.msg_type = 0;
    replyMsg.msg_local_port = 0;
    replyMsg.msg_remote_port = msg->msgHeader.msg_remote_port;
    replyMsg.msg_id = NX_PRINTPAGEMSGID;

    if ((ret_code = msg_send(&replyMsg, MSG_OPTION_NONE, 0)) != SEND_SUCCESS) {
	LogError([NXStringManager stringFor:"PSDevice_replyfailed"],
		 ret_code);
	[device->currentJob errorPrinting];
    }
}

/**********************************************************************
 * Routine:	_pageimageHandler - Deal with pageimages when they arrive.
 *
 * Function:	This processes a pageimage message by first removing
 *		this port from the list of ports being watched (we want
 *		to use the port_backlog for flowcontrol) and then pass
 *		the message to the device.
 **********************************************************************/
static void
_pageimageHandler(NXPrintPageMessage *msg, PSDevice *device)
{
    NXPrintPageMessage	*newMsg;

    /* Copy the message */
    newMsg = (NXPrintPageMessage *) malloc(sizeof (*newMsg));
    bcopy(msg, newMsg, sizeof(*newMsg));

    if (++device->curBitmaps < device->maxBitmaps) {
	_pageimageReply(msg, device);
    }

    if (device->printing) {
	_imageqEnqueue(device, newMsg);
    } else {
	[device devicePrintImage:newMsg];
	device->printing = 1;
    }
}

/**********************************************************************
 * Routine:	_reconnectProc()
 *
 * Function:	Attempts to reestablish a WindowServer context after
 *		the WindowServer has died.
 **********************************************************************/
static void
_reconnectProc(DPSTimedEntry teNumber, double now, id device)
{
    LogDebug("PSDevice timed entry called");

    /* Just try starting the device, it will take care of the rest */
    [device start];
}

/**********************************************************************
 * Routine:	_pagebufAlloc()
 *
 * Function:	Allocate a new page buffer.
 **********************************************************************/
static pagebuf_t
_pagebufAlloc(char *data, int size, BOOL mallocd, BOOL dealloc,
	      pagetype_t type)
{
    pagebuf_t	pb;

    pb = (pagebuf_t) malloc(sizeof (*pb));
    pb->pb_data = data;
    pb->pb_size = size;
    pb->pb_type = type;
    pb->pb_mallocd = mallocd;
    pb->pb_free = dealloc ? 1 : 0;
    pb->pb_next = NULL;

    return (pb);
}

/**********************************************************************
 * Routine:	_pagebufDealloc()
 *
 * Function:	Free a page buffer and its contents.
 **********************************************************************/
static void
_pagebufDealloc(pagebuf_t pb)
{
    kern_return_t	ret_code;

    if (pb->pb_free) {
	if (pb->pb_mallocd) {
	    free(pb->pb_data);
	} else {
	    if ((ret_code = vm_deallocate(task_self(),
					  (vm_offset_t)pb->pb_data,
					  (vm_size_t)pb->pb_size)) !=
		KERN_SUCCESS) {
		LogError([NXStringManager
			stringFor:"PSDevice_vmdeallocfail"], ret_code);
		[NXDaemon panic];
	    }
	}
    }
    free(pb);
}

/**********************************************************************
 * Routine:	_pagebufEnqueue()
 *
 * Function:	Enqueue a pagebuf to be printed.
 **********************************************************************/
static void
_pagebufEnqueue(PSDevice *object, pagebuf_t pb)
{
    if (object->pageq_head) {
	object->pageq_head->pb_next = pb;
    }
    object->pageq_head = pb;
    if (!object->pageq_tail) {
	object->pageq_tail = pb;
    }
}

/**********************************************************************
 * Routine:	_pagebufDequeue()
 *
 * Function:	Remove a page_buf from the queue and deallocate.
 **********************************************************************/
static void
_pagebufDequeue(PSDevice *object) {
    pagebuf_t	pb;

    pb = object->pageq_tail;
    object->pageq_tail = pb->pb_next;
    _pagebufDealloc(pb);

    if (!object->pageq_tail) {
	object->pageq_head = NULL;
    }
}


/*
 * Method definitions.
 */

@implementation PSDevice : Object

/* Factory methods */

+ newSpooler:(id) Spooler
{
    if (!_initdone) {
	[NXStringManager loadFromArray:_strings];
	_deviceTable = [HashTable newKeyDesc:"i"];
	_initdone = YES;
    }

    self = [super new];

    context = NULL;
    currentJob = nil;
    spooler = (PrintSpooler *)Spooler;
    devicePort = PORT_NULL;
    return (self);
}

/* Instance methods */

- spooler
{
    return ((id)spooler);
}

- refreshSpooler
{
    [spooler refresh];
    return (self);
}

- (BOOL)busy
{
    return (currentJob != nil);
}

- setCurrentJob:(id)job
{
    NXPrintPageMessage	*msg;

    if (job) {
	if (currentJob) {
	    LogError([NXStringManager stringFor:"PSDevice_jobjob"]);
	    [NXDaemon panic];
	}

	/* Initialize job parameters */
	pageq_head = NULL;
	pageq_tail = NULL;
	header = NULL;
	errorPage = NULL;
	processing = 0;
	job_done = 0;
	contextlost = 0;
	pagesPrinted = 0;
	maxBitmaps = 1;
	printing = 0;
	curBitmaps = 0;
	deviceReady = 0;
	timedEntry = NULL;

    } else {
	if (!currentJob) {
	    return(self);
	}

	/*
	 * We get rid of the context first since it causes a flush. We
	 * can't delete any data until the context is gone.  Otherwise
	 * out-of-line data in the context buffer will become invalid
	 * creating all kinds of problems.
	 */
	if (context) {
	    [_deviceTable removeKey:(void *)context];
	    NX_DURING {
		DPSSetErrorProc(context, DPSDefaultErrorProc);
		DPSDestroyContext(context);
	    } NX_HANDLER {
		;
	    } NX_ENDHANDLER;
	    context = NULL;
	}

	/* Get rid of a timed-entry if it is there */
	if (timedEntry) {
	    DPSRemoveTimedEntry(timedEntry);
	    timedEntry = NULL;
	}

	/*
	 * Since we may have been called due to a premature error,
	 * flush pages.  We tack the header onto the end of the page
	 * queue so that it gets flushed.
	 */
	if (header) {
	    _pagebufDealloc(header);
	    header = NULL;
	}
	while (pageq_tail) {
	    _pagebufDequeue(self);
	}

	while (msg = _imageqDequeue(self)) {
	    [self imageDiscard:msg];
	}

	if (devicePort) {
	    PortFree(devicePort);
	    devicePort = PORT_NULL;
	    free(devicePortName);
	}

    }

    currentJob = job;

    return (self);
}

- (int) pagesPrinted
{
    if (currentJob) {
	return (pagesPrinted);
    }

    return (0);
}

- contextLost
{

    LogWarning([NXStringManager stringFor:"PSDevice_contextlost"]);

    /* First clean things up */
    [_deviceTable removeKey:(void *)context];
    NX_DURING {
	DPSSetErrorProc(context, DPSDefaultErrorProc);
	DPSDestroyContext(context);
    } NX_HANDLER {
	;
    } NX_ENDHANDLER;
    context = NULL;
    processing = 0;

    /*
     * If we lost the context on this page last time, then we assume
     * that something in it is crashing the server.  Log an error and
     * abort the print job.
     */
    if (errorPage == pageq_tail) {
	LogError([NXStringManager stringFor:"PSDevice_doublefailure"]);
	[currentJob errorPrinting];
	return (nil);
    } else {
	errorPage = pageq_tail;
    }

    LogDebug("PSDevice setting up timed entry");
    contextlost = 1;
    timedEntry = DPSAddTimedEntry(RECONNECT_RETRY,
				  (DPSTimedEntryProc) _reconnectProc,
				  (void *) self, 1);
}

- resetContext
{

    if (context) {
	/* First clean things up */
	[_deviceTable removeKey:(void *)context];
	NX_DURING {
	    DPSSetErrorProc(context, DPSDefaultErrorProc);
	    DPSDestroyContext(context);
	} NX_HANDLER {
	    ;
	} NX_ENDHANDLER;
    }

    /* Try to connect to the public port */
    NX_DURING {
	context = DPSCreateContext(NULL, NULL, (DPSTextProc)_textHandler,
				   (DPSErrorProc)_errorHandler);
    } NX_HANDLER {
	LogDebug("PSDevice could not create public context");

	/* No luck, try our server_key */
	NX_DURING {
	    context = DPSCreateContext(NULL, (char *)server_key,
				       (DPSTextProc)_textHandler,
				       (DPSErrorProc)_errorHandler);
	} NX_HANDLER {
	    context = NULL;
	    LogDebug("PSDevice could not create back-door context");
	    if (NXLocalHandler.code != dps_err_cantConnect) {
		LogError([NXStringManager stringFor:"PSDevice_nocontext"]);
		NX_RAISE(NPDpsdevice, NULL, NULL);
	    } else {		/* If we cannot connect, try again later */
		if (timedEntry == NULL) {
		    timedEntry = DPSAddTimedEntry(RECONNECT_RETRY,
						    (DPSTimedEntryProc) _reconnectProc,
						    (void *) self, 1);
		}
	    }
	} NX_ENDHANDLER;
    } NX_ENDHANDLER;

    /* If we did not get a context, just return */
    if (!context) {
	return (nil);
    }

    LogDebug("PSDevice context created");

    /* We got a context!  Set it up */
    DPSSetEventFunc(context, (DPSEventFilterFunc) _eventHandler);
    [_deviceTable insertKey:(void *)context value:self];

    [self initContext];
    [self deviceInitContext];

    return (self);
}

- pingReceived
{
    pagebuf_t		pb;
    BOOL		trailer_done = NO;

    /* No more error page. */
    errorPage = NULL;

    if (pageq_tail->pb_type == trailer_page) {
	trailer_done = YES;
    }
    _pagebufDequeue(self);

    /* If there are more pages to print, do so. */
    if (pageq_tail) {
	[self deviceInitForPage:pageq_tail];
	[self sendPSData:pageq_tail->pb_data size:pageq_tail->pb_size];
	if (pageq_tail->pb_type == trailer_page) {
	    [self sendPSData:finishRestore size:strlen(finishRestore)];
	}
	[self sendPSData:pingData size:strlen(pingData)];
	DPSFlushContext(context);
    } else if (trailer_done) {
	job_done = 1;
	/*
	 * If the device port is empty, and nothing is printing,
	 * then tell the job we are done.  Otherwise, imageDone:
	 * will notify the job once the device is done processing
	 * the pageimages.
	 */
	if (PortMsgCount(devicePort) == 0 && ![self deviceBusy]) {
	    [currentJob donePrinting];
	}
    } else {
	processing = 0;
    }

    return (self);
}

- errorReceived:(DPSErrorCode)errorCode :(long unsigned int)arg1
    :(long unsigned int)arg2
{
    NXStream   	*errorStream = NULL;
    char	*buf;
    int		len;
    int		maxlen;

    if (errorCode == dps_err_ps) {
	NX_DURING {
	    errorStream = NXOpenMemory(NULL, 0, NX_WRITEONLY);
	    NXPrintf(errorStream,
		     [NXStringManager stringFor:"PSDevice_pserror"]);
	    DPSPrintErrorToStream(errorStream, (DPSBinObjSeq) arg1);
	    NXGetMemoryBuffer(errorStream, &buf, &len, &maxlen);
	    LogError(buf);
	    [currentJob handleAlert:buf];
	    NXCloseMemory(errorStream, NX_FREEBUFFER);
	} NX_HANDLER {
	    if (errorStream) {
		NX_DURING {
		    NXCloseMemory(errorStream, NX_FREEBUFFER);
		} NX_HANDLER {
		    ;
		} NX_ENDHANDLER;
	    }
	} NX_ENDHANDLER;
    } else {
	LogError([NXStringManager stringFor:"PSDevice_unknownerror"],
		 errorCode);
    }

    /* Let the job know we had an error, it will eventually free us */
    [currentJob errorPrinting];
}

- initContext
{
    PrintJob	*printJob = currentJob;
    char	buf[128];

    /* Save the VM state */
    [self sendPSData:initSave size:strlen(initSave)];

    /* Set the user's uid and gid */
    sprintf(buf, "\n%d %d setjobuser\n", printJob->user_uid,
	    printJob->user_gid);
    [self sendPSData:buf size:strlen(buf)];

    return (self);
}

- machPortDevice:(int)width :(int)height bbox:(boundbox_t)bbox
  transform:(float *)tfm dict:(const char *)dictInvocation

{
    char	buf[1024];

    if (devicePort == PORT_NULL) {
	sprintf(buf, _portTemplate, _portCounter);
	devicePortName = (char *) malloc(strlen(buf) + 1);
	strcpy(devicePortName, buf);
	if ((devicePort = PortCreate(devicePortName)) == PORT_NULL) {
	    LogError([NXStringManager stringFor:"PSDevice_noport"]);
	    NX_RAISE(NPDpsdevice, NULL, NULL);
	}

	/* Set the port up to be watched */
	DPSAddPort(devicePort, (DPSPortProc)_pageimageHandler,
		   sizeof(NXPrintPageMessage), (void *) self, 0);
    }

    sprintf(buf,
	   "\n%d %d [%d %d %d %d] [%f %f %f %f %f %f] () (%s) %s "
	    "machportdevice\n",
	    width, height, bbox.llx, bbox.lly, bbox.urx, bbox.ury,
	    tfm[0], tfm[1], tfm[2], tfm[3], tfm[4], tfm[5],
	    devicePortName, dictInvocation);
    [self sendPSData:buf size:strlen(buf)];

    return (self);
}

- sendPSData:(const char *)data size:(int)size
{
    DPSWriteData(context, data, size);
}


- print:(char *)data size:(int)size mallocd:(BOOL)mallocd
  deallocate:(BOOL)dealloc type:(pagetype_t)type
{
    pagebuf_t	npb;

    /* Create and initialize the new pagebuf */
    npb = _pagebufAlloc(data, size, mallocd, dealloc, type);

    if (type == header_page) {
	header = npb;
    } else {
	/* Enqueue it for printing */
	_pagebufEnqueue(self, npb);

	if (!processing && deviceReady) {
	    [self start];
	}
    }
    return (self);
}

- start
{
    BOOL	didReset = NO;

    LogDebug("PSDevice start called");

    /* If there is no context, set one up */
    if (!context) {
	if (![self resetContext]) {
	    return (self);
	}
	[self sendPSData:header->pb_data size:header->pb_size];
    }

    /* Get rid of a timed-entry if it is there */
    if (timedEntry) {
	DPSRemoveTimedEntry(timedEntry);
	timedEntry = NULL;
    }

    /* If we are recovering from a lost context, remove the timed entry */
    if (contextlost) {
	LogWarning([NXStringManager stringFor:"PSDevice_contextrestart"]);
	contextlost = 0;
    }

    /* If we have data to send, send it. */
    if (pageq_tail) {
	processing = 1;
	[self deviceInitForPage:pageq_tail];
	[self sendPSData:pageq_tail->pb_data size:pageq_tail->pb_size];
	if (pageq_tail->pb_type == trailer_page) {
	    [self sendPSData:finishRestore size:strlen(finishRestore)];
	}
	[self sendPSData:pingData size:strlen(pingData)];
	DPSFlushContext(context);
    }
    return (self);
}

- abort
{
    if (currentJob) {
	[currentJob errorPrinting];
    }
    return (nil);
}

- free
{

    [self setCurrentJob:nil];
    return ([super free]);
}

- deviceInitContext
{
    /* This should be subclassed by the device */
    return (self);
}

- deviceInitForPage:(pagebuf_t)pagebuf
{
    /* This should be subclassed by the device */
    return (self);
}

- devicePrintImage:(NXPrintPageMessage *)msg
{
    /*
     * This should be subclassed by the device.
     * By default we free up the message.
     */
    return ([self imageDone:msg]);
}

- (BOOL)deviceBusy
{
    /*
     * This should be subclassed by the device, by default we
     * are not busy.
     */
    return (NO);
}

- imageDiscard:(NXPrintPageMessage *)msg
{
    kern_return_t	ret_code;
    unsigned int	size;

    /* Free up the out-of-line data */
    size = msg->oolImageParam.msg_type_long_size *
	msg->oolImageParam.msg_type_long_number / CHAR_BIT;
    if ((ret_code = vm_deallocate(task_self(), (vm_offset_t) msg->printerData,
		  (vm_size_t) size)) != KERN_SUCCESS) {
	LogError([NXStringManager stringFor:"PSDevice_vmdeallocfail"],
		 ret_code);
	[NXDaemon panic];
    }
    free(msg);

    return (self);
}    

- imageDone:(NXPrintPageMessage *)msg
{
    msg_return_t	ret_code;

    /* Count this as a printed page */
    pagesPrinted++;

    /* If we have room for more images, unblock the window server */
    if (curBitmaps-- == maxBitmaps && !contextlost) {
	_pageimageReply(msg, self);
    }

    /* Free up the out-of-line data */
    [self imageDiscard:msg];

    if (msg = _imageqDequeue(self)) {
	[self devicePrintImage:msg];
    } else {
	printing = 0;

	/* If this was the last page, notify that we are done */
	if (job_done) {
	    /* Tell the job that we are done */
	    return([currentJob donePrinting]);
	}
    }
    return (self);
}
 
@end

