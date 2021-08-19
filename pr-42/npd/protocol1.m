/*
 * protocol1.m	- Implementation of npd Protocol version 1.0
 *
 * Copyright (c) 1989, 1990 by NeXT, Inc.  All rights reserved.
 */

/*
 * Include files.
 */
#import "protocol1.h"
#import "log.h"
#import "mach_ipc.h"
#import "NpdDaemon.h"
#import "NextPrinter.h"
#import "SpoolingJob.h"
#import "LocalJob.h"
#import "PrintSpooler.h"

#import <cthreads.h>
#import <errno.h>
#import <libc.h>
#import <string.h>
#import <sound/sound.h>

/*
 * Data types
 */
typedef struct {
    const char	*message;
    const char	*button1;
    const char	*button2;
    const char	*host;
    int		code;
} alert_data_t;


/*
 * Constants.
 */

/* The path of the Inform program */
static char	*INFORM = "/usr/lib/NextPrinter/Inform";

/* Constants for the _send*Reply routines */
#define	FREEPORT	YES
#define DONTFREEPORT	NO

/* Our error strings */
static struct string_entry _strings[] = {
    { "NPD1_noreplyport", "1.0 request with no reply port, ignoring" },
    { "NPD1_replyblocked", "1.0 reply dropped due to full port" },
    { "NPD1_msgsndfailed", "1.0 msg_send failed, error = %d" },
    { "NPD1_audiofailed", "1.0 audio alert failed playing %s: %s" },
    { "NPD1_badsoundname", "1.0 audio alert given garbage soundfile name" },
    { "NPD1_badvisargs", "1.0 visual alert given garbage arguments" },
    { "NPD1_badconargs", "1.0 connect given garbage arguments" },
    { "NPD1_noalertmsg", "1.0 visual alert request with no message"},
    { "NPD1_connpdnonlocal",
	  "1.0 connect from npd to non-local printer \"%s\"" },
    { "NPD1_queuedisabled", "1.0 connect request to disabled printer \"%s\"" },
    { "NPD1_execfailed", "Could not exec %s: %m" },
    { "NPD1_abortfailed", "AbortSelf could not send NPD_1_REMOVE message" },
    { NULL, NULL },
};


/*
 * Static variables.
 */
#if	DEBUG
static int	_debugStopInform = 0;	/* Send SIGSTOP to new Inform process */
#endif	DEBUG


/*
 * Local routines.
 */

/**********************************************************************
 * Routine:	_isaString() - Check that we have a real string.
 *
 * Function:	Check that what is supposed to be a string in a
 *		message is indeed a string.  We start at the end
 *		of the character array and scan backwards for a NULL.
 *		We don't want any string routines running off the
 *		end of this data.
 *
 * Returns:	YES:	A null was found.
 *		NO:	No null found.
 **********************************************************************/
static BOOL
_isaString(char *str, int arraysize)
{
    char	*cp;			/* generic character pointer */

    cp = str + arraysize - 1;
    while (*cp && cp >= str) {
	cp--;
    }
    if (*cp != '\0' || cp < str) {
	return(NO);
    }
    return (YES);
}

/**********************************************************************
 * Routine:	_sendMsg() - Send a message.
 *
 * Function:	This routine handles the msg_send and any errors that
 *		it may return.  We send the message with a timeout of
 *		0 so that we won't block.  If the receiving port
 *		is blocked, so be it.  The app can make the request
 *		of us again at some point.
 *
 * Args:	msg:		The message to send
 *		freePort:	Flag to free remote port.
 *
 * Returns:	return_code from msg_send()
 **********************************************************************/
static msg_return_t
_sendMsg(msg_header_t *msg, BOOL freePort)
{
    msg_return_t	ret_code;	/* Error return from msg routines */

    /* Send the message */
    if ((ret_code = msg_send(msg, SEND_TIMEOUT, 0)) !=
	SEND_SUCCESS) {
	if (ret_code == SEND_TIMED_OUT) {
	    LogDebug("Protocol 1.0 reply timed out");
	} else {
	    LogWarning([NXStringManager stringFor:"NPD1_msgsndfailed"],
		     ret_code);
	}
    }
    if (freePort) {
	PortFree(msg->msg_remote_port);
    }
    return (ret_code);
}

/**********************************************************************
 * Routine:	_sendGenericReply() - Send a generic reply.
 *
 * Function:	A generic reply is one that really has no information in
 *		it whatsoever.
 *
 * Args:	remotePort:	The port to send to.
 *		localPort:	Our local port.
 *		freePort:	Flag to free remote port when done
 *
 * Return:	return code from msg_send()
 **********************************************************************/
static msg_return_t
_sendGenericReply(port_t remotePort, port_t localPort, BOOL freePort)
{
    msg_header_t	returnMsg;	/* The return message */

    bzero(&returnMsg, sizeof (returnMsg));
    MSGHEAD_SIMPLE_INIT(returnMsg, sizeof(returnMsg));
    returnMsg.msg_local_port = localPort;
    returnMsg.msg_remote_port = remotePort;

    return(_sendMsg(&returnMsg, freePort));
}

/**********************************************************************
 * Routine:	_connectRequest() - Process an App's connection request
 *
 * Function:	Make sure the request is well formatted, and the printer
 *		is enabled.  If so, create a PrintJob for the App.
 **********************************************************************/
static void
_connectRequest(npd1_con_msg *msg)
{
    id		spooler;
    id		printJob;
    id		localPrinter;
    BOOL	fromLpd;

    /* Make sure the parameters are grossly reasonable */
    if (! _isaString(msg->printer, sizeof(msg->printer)) ||
	! _isaString(msg->user, sizeof(msg->user)) ||
	! _isaString(msg->orig_host, sizeof(msg->orig_host))) {
	LogError([NXStringManager stringFor:"NPD1_badconargs"]);
	(void) _sendGenericReply(msg->head.msg_remote_port, PORT_NULL,
				 FREEPORT);
	return;
    }

    fromLpd = (msg->head.msg_id == NPD_1_CONNECT_FROM_LPD);
    if (localPrinter = [NXDaemon localPrinter]) {
	[localPrinter refreshSpooler];
    }
    spooler = nil;

    /* Figure our which printer to use and get a reference on the spooler */
    if (fromLpd) {
	/*
	 * npcomm should always print to the local printer.  Because
	 * of the possibility of administrative screw-up, we'll assume
	 * the local printer.  That is, until we get a decent spooling
	 * system.
	 */
	if (localPrinter) {
	    spooler = [[localPrinter spooler] hold];
	}
    } else {
	spooler = [PrintSpooler newPrinter:msg->printer];
    }

    /* If we don't have a spooler or it's unavailable, abort the job */
    if (!spooler || [spooler isUnavailable]) {
	if (spooler) {
	    [spooler free];
	}
	(void) _sendGenericReply(msg->head.msg_remote_port, PORT_NULL,
				 FREEPORT);
	return;
    }

    /*
     * If this is the local printer and it is not busy, try to create
     * a local job.
     */
    if (localPrinter && spooler == [localPrinter spooler] &&
	![localPrinter busy]) {
	printJob = [LocalJob newPrinter: spooler
		  user: msg->user
		  host: msg->orig_host
		  copies: msg->copies
		  remotePort: msg->head.msg_remote_port
		  fromLpd: fromLpd];
    } else {
	printJob = nil;
    }

    /*
     * If we don't have a local job and we are not from Lpd, create
     * a spooling job.
     */
    if (printJob == nil && !fromLpd) {

	/* If the spooler is not enabled, abort the job. */
	if (![spooler queueEnabled]) {
	    LogError([NXStringManager stringFor:"NPD1_queuedisabled"],
		     msg->printer);
	    [spooler free];
	    (void) _sendGenericReply(msg->head.msg_remote_port,
				     PORT_NULL, FREEPORT);
	    return;
	}

	/* OK, create a spooling job */
	printJob = [SpoolingJob newPrinter: spooler
		  user: msg->user
		  host: msg->orig_host
		  copies: msg->copies
		  remotePort: msg->head.msg_remote_port
		  fromLpd:NO];
    }

    /* If we finally have a job, reply with its port */
    if (printJob) {
	if (_sendGenericReply(msg->head.msg_remote_port,
			      [printJob localPort], DONTFREEPORT)
	    != SEND_SUCCESS) {
	    [printJob free];
	}
	return;
    }

    /*
     * Could not set up a print job, reply with a PORT_NULL.  This is
     * the error return in this protocol.
     */
    [spooler free];
    (void) _sendGenericReply(msg->head.msg_remote_port, PORT_NULL, FREEPORT);
    return;
}

/**********************************************************************
 * Routine:	_visualAlertChild - Run the Inform program
 *
 * Function:	This routine exec's the Inform program that
 *		puts an alert message up on the screen.
 **********************************************************************/
static int
_visualAlertChild(alert_data_t *alert)
{
    char	buf[16];		/* Buffer to build integer argument */
    const char	*argv[10];
    const char	**cpp = argv;

#if	DEBUG
    /* Stop ourselves so that we can attach a debugger */
    if (_debugStopInform) {
	kill(getpid(), SIGSTOP);
    }
#endif	DEBUG

    /* Clear argv */
    bzero(argv, sizeof(argv));

    /* Build the arguments */
    *cpp++ = INFORM;			/* argv[0] */

    if (alert->code) {

	/* We have a code, use it. */
	sprintf(buf, "%d", alert->code);
	if (alert->host) {
	    *cpp++ = "-h";
	    *cpp++ = alert->host;
	    
	}
	*cpp++ = "-n";
	*cpp++ = buf;
    } else {

	/* We don't have a code, use the message */
	if (alert->button1) {
	    *cpp++ = "-b1";
	    *cpp++ = alert->button1;
	}
	if (alert->button2) {
	    *cpp++ = "-b2";
	    *cpp++ = alert->button2;
	} 
	*cpp++ = alert->message;
    }

    /* Exec the program */
    execv(INFORM, argv);

    /* If we got here, then the exec failed. */
    LogError([NXStringManager stringFor:"NPD1_execfailed"],
	     INFORM);
	     
    return(0);
}

/**********************************************************************
 * Routine:	_visualAlertHandler() - Handle exit of visual alert.
 *
 * Function:	This just frees the alert structure.
 **********************************************************************/
static void
_visualAlertHandler(int pid, union wait *status, alert_data_t *alert)
{
    free((void *)alert);
}


/**********************************************************************
 * Routine:	_visualAlert() - Display a visual alert message.
 *
 * Function:	This execs the Inform program to display a visual
 *		alert message and to watch for a change in the printer
 *		status.
 **********************************************************************/
static void
_visualAlert(const char *message, const char *button1, const char *button2,
	     const char *orig_host, int alert_code)
{
    alert_data_t	*alert;		/* Structure to hold alert arguments */

    /* Zero out the structure */
    alert = (alert_data_t *) malloc(sizeof (*alert));
    bzero(alert, sizeof (*alert));

    /* If we don't have a code, we should have a message */
    if (!alert_code && !*message) {
	LogError([NXStringManager stringFor:"NPD1_noalertmsg"]);
	return;
    }

    /* Build the alert structure */
    alert->message = message;
    if (*button1) {
	alert->button1 = button1;
    }
    if (*button2) {
	alert->button2 = button2;
    }
    alert->host = orig_host;
    alert->code = alert_code;

    /* Spawn a child to exec the Inform program */
    [NXDaemon spawnChildProc:(NXChildProc)_visualAlertChild
	data:(void *)alert
   	handler:(NXChildHandler)_visualAlertHandler
   	handlerData:(void *)alert];
}    

    
/**********************************************************************
 * Routine:	_printerStatus() - Process a printer status request
 *
 * Function:	Get the current status of the local printer and return
 *		it to the sender.
 **********************************************************************/
static void
_printerStatus(npd1_con_msg *msg)
{
    int			status;
    msg_header_t	send_msg;
    id			localPrinter;

    /* Sanity check */
    if (msg->head.msg_remote_port == PORT_NULL) {
	LogWarning([NXStringManager stringFor:"NPD1_noreplyport"]);
	return;
    }

    /* Get the status of the printer */
    if (!(localPrinter = [NXDaemon localPrinter])) {
	status = (int) (NPD_1_OS_ERROR|ENODEV);
    } else {
	status = [localPrinter getStatus];
    }

    /* Cobble up the reply message */
    bzero(&send_msg, sizeof (send_msg));
    MSGHEAD_SIMPLE_INIT(send_msg, sizeof(send_msg));
    send_msg.msg_local_port = PORT_NULL;
    send_msg.msg_remote_port = msg->head.msg_remote_port;
    send_msg.msg_id = status;

    /* And send it */
    (void) _sendMsg(&send_msg, FREEPORT);
}

/**********************************************************************
 * Routine:	_queueInfo() - Return the local print queue
 *
 * Function:	Return a list of entries in the spooler's queue.
 **********************************************************************/
static void
_queueInfo(npd1_con_msg *msg)
{
    npd1_info_msg	imsg;
    id			spooler;

    /* Sanity check */
    if (msg->head.msg_remote_port == PORT_NULL) {
	LogWarning([NXStringManager stringFor:"NPD1_noreplyport"]);
	return;
    }

    /* Look up the printer information */
    if (!(spooler = [PrintSpooler newPrinter:msg->printer])) {

	/* No spooler, send an empty reply message */
	bzero(&imsg, sizeof (imsg));
	MSGHEAD_SIMPLE_INIT(imsg.head, sizeof(imsg));
	imsg.head.msg_local_port = PORT_NULL;
	imsg.head.msg_remote_port = msg->head.msg_remote_port;
	MSGTYPE_CHAR_INIT(imsg.printer_type, sizeof(imsg.printer));
	MSGTYPE_CHAR_INIT(imsg.host_type, sizeof(imsg.host));
	MSGTYPE_CHAR_INIT(imsg.user_type, sizeof(imsg.user));
	MSGTYPE_CHAR_INIT(imsg.doc_type, sizeof(imsg.doc));
	MSGTYPE_CHAR_INIT(imsg.creator_type, sizeof(imsg.creator));
	MSGTYPE_INT_INIT(imsg.size_type);
	MSGTYPE_INT_INIT(imsg.pages_type);
	MSGTYPE_INT_INIT(imsg.feed_type);
	MSGTYPE_INT_INIT(imsg.job_type);
	MSGTYPE_INT_INIT(imsg.time_type);

	imsg.head.msg_id = NPD_NO_MORE_INFO;

	(void) _sendMsg((msg_header_t *)&imsg, FREEPORT);
    }

    [spooler sendNPD1InfoTo:msg->head.msg_remote_port];

    /* FIXME: We should cache PrintSpoolers */
    [spooler free];
}


/*
 * Exported routines.
 */

/**********************************************************************
 * Routine:	NPD1_Init() - Initialize this protocol module.
 *
 * Function:	Load our error strings into the StringManager.
 **********************************************************************/
void
NPD1_Init()
{

    /* Load our error strings */
    [NXStringManager loadFromArray:_strings];
}


/**********************************************************************
 * Routine:	NPD1_Connect() - Process Protocol 1.0 initial request.
 *
 * Function:	This routine deals with Protocol 1.0.  The possible
 *		requests are as follows:
 *
 *		NPD_CONNECT
 *		A request for a print connection from a NextStep
 *		application or an Protocol 1.0 npd.
 *
 *		NPD_CONNECT_FROM_LPD
 *		These are requests for a print connection from npcomm
 *		which should only be accepted if nothing is currently
 *		using the printer.
 *
 *		NPD_AUDIO_ALERT
 *		Post an audio alert with the given file.
 *
 *		NPD_VISUAL_ALERT
 *		Post a visual alert with the given message.
 *
 *		NPD_PRINTER_STATUS
 *		Return the printer status
 *
 *		NPD_INFO
 *		Return the queue information.
 *
 *		NPD_REMOVE
 *		Remove a print job from the queue.
 **********************************************************************/
void
NPD1_Connect(npd1_con_msg *msg)
{
    id		localPrinter;

    /* What kind of message do we have? */
    switch (msg->head.msg_id) {

      case NPD_1_CONNECT_FROM_LPD:
      case NPD_1_CONNECT:
	/* Connection */
	_connectRequest(msg);
	break;

      case NPD_1_AUDIO_ALERT:
	/* Request for an audio alert */

	{
	    npd1_audio_alert_msg *audiomsg = (npd1_audio_alert_msg *)msg;

	    /* Verify we have a real string name */
	    if ( ! _isaString(audiomsg->soundfile,
			      sizeof(audiomsg->soundfile)) ) {
		LogError([NXStringManager stringFor:"NPD1_badsoundname"]);
		break;
	    }

	    /* Request for an audio alert */
	    NPD1_AudioAlert(audiomsg->soundfile);
	    break;
	}

      case NPD_1_VISUAL_ALERT:
	/* Request for a visual alert */

	{
	    npd1_visual_alert_msg	*vismsg = (npd1_visual_alert_msg *)msg;

	    /* Make sure the arguments are grossly reasonable */
	    if (! _isaString(vismsg->message, sizeof (vismsg->message)) ||
		! _isaString(vismsg->button1, sizeof (vismsg->button1)) ||
		! _isaString(vismsg->button2, sizeof (vismsg->button2)) ||
		! _isaString(vismsg->orig_host, sizeof(vismsg->orig_host))) {
		LogError([NXStringManager stringFor:"NPD1_badvisargs"]);
	    } else {
		_visualAlert(vismsg->message, vismsg->button1, vismsg->button2,
			     vismsg->orig_host, vismsg->alert_code);
	    }
	}
	break;

      case NPD_1_PRINTER_STATUS:
	/* Request for the printer status */
	_printerStatus(msg);
	break;
	
      case NPD_1_INFO:
	/* Request for printer queue information */
	_queueInfo(msg);
	break;

      case NPD_1_REMOVE:
	/* Abort the current print job */
	if (localPrinter = [NXDaemon localPrinter]) {
	    [localPrinter abort];
	}
	break;

      default:
	LogDebug("unrecognized Protocol 1.0 request, code = %d",
		 msg->head.msg_id);
    }
}

void
NPD1_VisualMessage(const char *message, const char *button1,
		   const char *button2)
{
    if (!button1) {
	button1 = "";
    }
    if (!button2) {
	button2 = "";
    }
    _visualAlert(message, button1, button2, NXHostName, 0);
}

void
NPD1_RemoteVisualMessage(const char *host, const char *message,
			 const char *button1, const char *button2)
{
    npd1_visual_alert_msg	msg;
    port_t			destPort;

    /* Get a port for the remote npd */
    if ((destPort = PortLookup(NPD_PUBLIC_PORT, host)) == PORT_NULL) {
	return;
    }

    /* Cobble up a message */
    bzero(&msg, sizeof(msg));
    MSGHEAD_SIMPLE_INIT(msg.head, sizeof(msg));
    msg.head.msg_local_port = PORT_NULL;
    msg.head.msg_remote_port = destPort;
    msg.head.msg_id = NPD_1_VISUAL_ALERT;
    MSGTYPE_INT_INIT(msg.alert_code_type);
    msg.alert_code = 0;
    MSGTYPE_CHAR_INIT(msg.message_type, sizeof(msg.message));
    if (message) {
	strncpy(msg.message, message, sizeof(msg.message) - 1);
    } else {
	msg.message[0] = '\0';
    }
    MSGTYPE_CHAR_INIT(msg.button1_type, sizeof(msg.button1));
    if (button1) {
	strncpy(msg.button1, button1, sizeof(msg.button1) - 1);
    } else {
	msg.button1[0] = '\0';
    }
    MSGTYPE_CHAR_INIT(msg.button2_type, sizeof(msg.button2));
    if (button2) {
	strncpy(msg.button2, button2, sizeof(msg.button2) - 1);
    } else {
	msg.button2[0] = '\0';
    }
    MSGTYPE_CHAR_INIT(msg.orig_host_type, sizeof(msg.orig_host));
    strncpy(msg.orig_host, NXHostName, sizeof(msg.orig_host) - 1);

    /* And send it */
    (void) _sendMsg((msg_header_t *)&msg, FREEPORT);
}

void
NPD1_VisualCode(int alert_code)
{
    _visualAlert("", "", "", NXHostName, alert_code);
}

void
NPD1_RemoteVisualCode(const char *host, int alert_code)
{
    npd1_visual_alert_msg	msg;
    port_t			destPort;

    /* Get a port for the remote npd */
    if ((destPort = PortLookup(NPD_PUBLIC_PORT, host)) == PORT_NULL) {
	return;
    }

    /* Cobble up a message */
    bzero(&msg, sizeof(msg));
    MSGHEAD_SIMPLE_INIT(msg.head, sizeof(msg));
    msg.head.msg_local_port = PORT_NULL;
    msg.head.msg_remote_port = destPort;
    msg.head.msg_id = NPD_1_VISUAL_ALERT;
    MSGTYPE_INT_INIT(msg.alert_code_type);
    msg.alert_code = alert_code;
    MSGTYPE_CHAR_INIT(msg.message_type, sizeof(msg.message));
    MSGTYPE_CHAR_INIT(msg.button1_type, sizeof(msg.button1));
    MSGTYPE_CHAR_INIT(msg.button2_type, sizeof(msg.button2));
    MSGTYPE_CHAR_INIT(msg.orig_host_type, sizeof(msg.orig_host));
    strncpy(msg.orig_host, NXHostName, sizeof(msg.orig_host) - 1);

    /* And send it */
    (void) _sendMsg((msg_header_t *)&msg, FREEPORT);
}

/**********************************************************************
 * Routine:	NPD1_AudioAlert() - Play an audio alert message.
 *
 * Function:	This calls SNDPlaySoundFile on the file passed in
 *		the message.
 **********************************************************************/
void
NPD1_AudioAlert(const char *soundFile)
{
    int		error;			/* error return */
    extern kern_return_t check_sound_port_registered(void);
    extern const char *mach_error_string(int);

    /* Play the sound */
    if (error = check_sound_port_registered()) {
	LogError([NXStringManager stringFor:"NPD1_audiofailed"],
		 soundFile, mach_error_string(error));
    } else if (error = SNDPlaySoundfile((char *)soundFile, 0)) {
	LogError([NXStringManager stringFor:"NPD1_audiofailed"],
		 soundFile, SNDSoundError(error));
    }
}

void
NPD1_RemoteAudioAlert(const char *host, const char *soundFile)
{
    npd1_audio_alert_msg	msg;
    port_t			destPort;

    /* Get a port for the remote npd */
    if ((destPort = PortLookup(NPD_PUBLIC_PORT, host)) == PORT_NULL) {
	return;
    }

    /* Cobble up a message */
    bzero(&msg, sizeof(msg));
    MSGHEAD_SIMPLE_INIT(msg.head, sizeof(msg));
    msg.head.msg_local_port = PORT_NULL;
    msg.head.msg_remote_port = destPort;
    msg.head.msg_id = NPD_1_AUDIO_ALERT;
    MSGTYPE_CHAR_INIT(msg.soundfile_type, sizeof(msg.soundfile));
    if (soundFile) {
	strncpy(msg.soundfile, soundFile, sizeof(msg.soundfile) - 1);
    } else {
	strcpy(msg.soundfile, "");
    }

    /* And send it */
    (void) _sendMsg((msg_header_t *)&msg, FREEPORT);
}

void
NPD1_AbortSelf()
{
    npd1_con_msg	msg;
    msg_return_t	ret_code;

    bzero(&msg, sizeof (msg));
    MSGHEAD_SIMPLE_INIT(msg.head, sizeof(msg));
    msg.head.msg_remote_port = [NXDaemon mainPort];
    msg.head.msg_local_port = PORT_NULL;
    msg.head.msg_id = NPD_1_REMOVE;

    if ((ret_code = msg_send(&msg, SEND_TIMEOUT, 0)) != SEND_SUCCESS) {
#ifdef	NOTDEF
	/*
	 * We don't do this because logging uses malloc.
	 */
	LogError([NXStringManager stringFor:"NPD1_abortfailed"]);
#endif	NOTDEF
    }
}

