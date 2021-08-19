/*
 * Daemon.m	- Implementation of the daemon object.
 *
 * Copyright (c) 1990 by NeXT, Inc., All rights reserved.
 */

/*
 * Include files
 */
#import "Daemon.h"
#import "StringManager.h"
#import "log.h"
#import "mach_ipc.h"
#import "atomopen.h"

#import <cthreads.h>
#import <errno.h>
#import <fcntl.h>
#import <libc.h>
#import <string.h>
#import <syslog.h>
#import <stdlib.h>
#import <sys/ioctl.h>
#import <sys/signal.h>
#import <sys/notify.h>


/*
 * Internal data types.
 */
typedef struct child_info {
    NXChildHandler	handler;
    void		*data;
} *child_info_t;

typedef struct signal_msg {
    msg_header_t	head;
    msg_type_t		signal_type;
    int			signal;
    msg_type_t		child_type;
    int			child;
    msg_type_t		status_type;
    union wait		status;
} *signal_msg_t;

/*
 * Constants
 */
static const char	*DEVNULL = "/dev/null";
static const char	*DEVTTY = "/dev/tty";

static struct string_entry _strings[] = {
    { "Daemon_signal", "caught signal, exiting" },
    { "Daemon_portbusy", "another npd already running" },
    { "Daemon_noreg", "couldn't register with nmserver" },
    { "Daemon_unexpevent", "unexpected event received" },
    { "Daemon_toperror", "top level error, code = %d" },
    { "Daemon_noport", "PortCreate failed" },
    { "Daemon_failedhack", "AddPort/RemovePort hack failed, error code = %d" },
    { "Daemon_nonotify", "DPSAddPort() of the notify port failed, code = %d" },
    { "Daemon_nosignotify",
	  "DPSAddPort() of the signal notify port failed, code = %d" },
    { "Daemon_waiterr", "error reaping child process: %s" },
    { "Daemon_badportdel", "unexpected port death" },
    { "Daemon_notifynoadd", "could not DPSAddPort the notify port" },
    { "Daemon_signalportfull",
	  "Daemon could not notify self of signal, panicing" },
    { NULL, NULL },
};


/*
 * Global variables.
 */
id	NXDaemon = NULL;		/* The daemon itself */
StringManager *NXStringManager = NULL;	/* The StringManager */
char	*NXHostName = "localhost";	/* This host's name */

/*
 * Local Variables.
 */
static port_t	_signalNotifyPort;	/* Port to notify on signal */


/*
 * Static routines
 */

/**********************************************************************
 * Routine:	_deathSignalHandler() - Handle death signal.
 *
 * Function:	Cleanly shut down program by calling the shutdown
 *		method. Then exit.
 **********************************************************************/
static void
_deathSignalHandler()
{
    struct signal_msg	signal_msg;
    msg_return_t	ret_code;
    
    /* Block out all interrupts that would cause this routine to be
       called */
    (void) sigblock(sigmask(SIGHUP)|sigmask(SIGINT)|
		     sigmask(SIGQUIT)|sigmask(SIGTERM));

    /* Send a message to ourself to shut down */
    bzero(&signal_msg, sizeof (signal_msg));
    MSGHEAD_SIMPLE_INIT(signal_msg.head, sizeof(signal_msg));
    signal_msg.head.msg_remote_port = _signalNotifyPort;
    MSGTYPE_INT_INIT(signal_msg.signal_type);
    MSGTYPE_INT_INIT(signal_msg.child_type);
    MSGTYPE_INT_INIT(signal_msg.status_type);
    signal_msg.signal = SIGQUIT;	/* No need to differentiate */

    if ((ret_code = msg_send(&signal_msg, SEND_TIMEOUT, 0)) != SEND_SUCCESS) {
	[NXDaemon panic];
    }
}

/**********************************************************************
 * Routine:	_childSignalHander() - Handle child status change.
 *
 * Function:	Handle a status change in a child process.  If there
 *		is a child handler, pass the status change on to it.
 *		If the child exited, unregister the status handler.
 **********************************************************************/
static void
_childSignalHandler()
{
    union wait		status;
    int			pid;
    msg_return_t	ret_code;
    struct signal_msg	signal_msg;

    /* Initialize the notify message */
    /* Send a message to ourself to shut down */
    bzero(&signal_msg, sizeof (signal_msg));
    MSGHEAD_SIMPLE_INIT(signal_msg.head, sizeof(signal_msg));
    signal_msg.head.msg_remote_port = _signalNotifyPort;
    MSGTYPE_INT_INIT(signal_msg.signal_type);
    MSGTYPE_INT_INIT(signal_msg.child_type);
    MSGTYPE_INT_INIT(signal_msg.status_type);
    signal_msg.signal = SIGCHLD;

    /* Reap each child that needs it */
    while ((pid = wait3(&status, WNOHANG, NULL)) > 0) {
	signal_msg.child = pid;
	signal_msg.status = status;

	/* Send the notify message */
	if ((ret_code = msg_send(&signal_msg, SEND_TIMEOUT, 0)) !=
	    SEND_SUCCESS) {
	    [NXDaemon panic];
	}
    }

#ifdef	NOTDEF
    /*
     * We don't do this here because LogError uses malloc.
     */
    if (pid < 0 && cthread_errno() != ECHILD) {
	LogError([NXStringManager stringFor:"Daemon_waiterr"],
		 strerror(cthread_errno()));
    }
#endif	NOTDEF
}

/**********************************************************************
 * Routine:	_signalMessageHandler()
 *
 * Function:	Handle shutdown signals and child death signal
 *		within the main event loop to keep the objects
 *		and heap safe.
 **********************************************************************/
static void
_signalMessageHandler(signal_msg_t msg, void *userData)
{
    child_info_t	childInfo;
    StringManager	*handlerTable;

    switch (msg->signal) {
      case SIGQUIT:
	/* Shut down the daemon */
	[NXDaemon shutdown];

	/* Log this as an error since it should never really happen */
	LogError([NXStringManager stringFor:"Daemon_signal"]);

	exit(1);
	/* Never gets here */

      case SIGCHLD:
	/* Get the Hash Table of child handlers */
	handlerTable = [NXDaemon childHandlers];

	if (handlerTable &&
	    (childInfo = [handlerTable
			valueForKey:(const void *)msg->child])) {

	    /* There's a child handler, call it */
	    (*childInfo->handler)(msg->child, &msg->status, childInfo->data);
	    [NXDaemon resetChildHandler:msg->child];
	}
	break;

      default:
	LogError([NXStringManager stringFor:"Daemon_unknownsignal"],
		 msg->signal);
	break;
    }
}

/**********************************************************************
 * Routine:	_uncaughtException() - Handle an uncaught exception
 *
 * Function:	This routine handles uncaught exceptions from
 *		library routines.  It will log the error in the
 *		appropriate place and send the panic message to
 *		NXDaemon.
 **********************************************************************/
static void
_uncaughtException(int code, const void *data1, const void *data2)
{

    LogError("uncaught exception, code = %d", code);

    [NXDaemon panic];

    /* Never gets here */
}

/**********************************************************************
 * Routine:	_notifyHandler() - Handle messages on the notify port
 *
 * Function:	If we get a port death message, then see if the is a
 *		port delegate to notify.  Send the delegate the "portDied"
 *		method.
 **********************************************************************/
static void
_notifyHandler(notification_t *msg, void *userData)
{
    id	delegateTable;
    id	delegate;

    if ((delegateTable = [NXDaemon portDelegates]) == nil) {
	return;
    }
    
    /* Get the delegate for this port */
    delegate = [delegateTable valueForKey:(const void *)msg->notify_port];

    switch (msg->notify_header.msg_id) {
      case NOTIFY_PORT_DELETED:
      case NOTIFY_PORT_DESTROYED:

	if (delegate != nil) {
	    /* Remove the delegate from the hashTable */
	    [NXDaemon resetNotifyDelegate: msg->notify_port];

	    if ([delegate respondsTo:@selector(portDied:)]) {
		[delegate portDied:msg->notify_port];
	    } else {
		LogError([NXStringManager stringFor:"Daemon_badportdel"],
			 NAMEOF(delegate));
	    }
	}
	break;

      case NOTIFY_MSG_ACCEPTED:

	if (delegate && [delegate respondsTo:@selector(portMsgReceived:)]) {
	    [delegate portMsgReceived:msg->notify_port];
	} else {
	    LogDebug("NOTIFY_MSG_ACCEPTED received for port without delegate");
	}
	break;

      default:
	LogError("Unknown notify message, id = %d", msg->notify_header.msg_id);
	break;
    }
}

/**********************************************************************
 * Routine:	_notifySetUp()	- Bind _notifyHander to notify port
 *
 * Function:	This routine does a DPSAddPort to register the routin
 *		we use to watch the notify port.
 *
 * FIXME:	There is a workaround to a dpsclient problem in
 *		here.  It turns out that the notify port is set up
 *		by dpsclient.  We need to do something to cause it to
 *		be set up before we get it.  A DPSAddPort does this.
 **********************************************************************/
static void
_notifySetUp()
{
    port_t	dummy_port;		/* XXX */
    port_t	notify_port;

    /*
     * XXX This causes dpsclient to initialize the port routines.  The
     * args to DPSAddPort are bogus which is not a problem as we will
     * remove the port right away.
     */
    if ((dummy_port = PortCreate(NULL)) == PORT_NULL) {
	LogError([NXStringManager stringFor:"Daemon_noport"]);
	[NXDaemon panic];
    }
    NX_DURING {
	DPSAddPort(dummy_port, (DPSPortProc)_notifyHandler, 1024,
		   NULL, 0);
	DPSRemovePort(dummy_port);
    } NX_HANDLER {
	LogError([NXStringManager stringFor:"Daemon_failedhack"],
		 NXLocalHandler.code);
	[NXDaemon panic];
    } NX_ENDHANDLER;

    /* Register the notify port */
    task_get_notify_port(task_self(), &notify_port);
    if (notify_port == PORT_NULL) {
	LogError([NXStringManager stringFor:"Daemon_nonotify"]);
	[NXDaemon panic];
    }

    NX_DURING {
	DPSAddPort(notify_port, (DPSPortProc)_notifyHandler,
	       (int) sizeof(notification_t) + 2048, NULL, 0);
    } NX_HANDLER {
	LogError([NXStringManager stringFor:"Daemon_notifynoadd"],
		 NXLocalHandler.code);
	[NXDaemon panic];
    } NX_ENDHANDLER;
}

/**********************************************************************
 * Routine:	_dumpCore() - Abort this program dumping core.
 *
 * Function:	This routine trys to do a core dump in a logical place
 *		before shutting down the whole program.
 **********************************************************************/
static void
_dumpCore()
{
#ifdef	GCORE
    /*
     * This code will try to use gcore to dump core into a logical
     * place.  Of course, it is dependant on gcore existing.
     */
       
    char	corePath[80];
    char	pidBuf[10];
    int		childPid;

    /*
     * Come up with a file name to dump core into.  Make sure we don't
     * overflow the path buffer.
     */
    bzero(corePath, sizeof(corePath));
    strcpy(corePath, COREDIR);
    strncat(corePath, [NXDaemon programName],
	    (sizeof(corePath) - strlen(COREDIR)
	     - strlen(CORESUFFIX) - 1 /* Null */));
    strcat(corePath, CORESUFFIX);

    /* Get a string representation of our pid */
    sprintf(pidBuf, "%d", getpid());

    /* Log what we are doing */
    LogError("Dumping core into %s", corePath);
    fflush(stderr);

    if ((childPid = fork()) == 0) {
	/* Child: run gcore */
	execl(GCOREPATH, "gcore", pid, corePath, 0);

	/* Never returns */

    } else {
	/* Parent: wait for gcore to finish */
	
	while (childpid != wait(0)) ;
	_exit(1);
    }
#else	GCORE
    LogError("Dumping core");
    abort();				/* generates a core file */
#endif	GCORE

    /* Never gets here */
}


/*
 * Method definitions.
 */

@implementation Daemon : Object

+ newWithArgs:(int)argc :(char **)argv
{
    char	*cp;			/* generic pointer */
    char	hostBuffer[128];

    if (NXDaemon) {
	return (NXDaemon);
    } else {
	self = [super new];
	NXDaemon = self;
    }

    debugMode = NO;

    /* Get the name of the program */
    if (cp = strrchr(*argv, '/')) {
	programName = cp + 1;
    } else {
	programName = *argv;
    }

    /* Temporarily set up syslog */
    openlog(programName, LOG_CONS|LOG_PID, LOG_LPR);

    /* Bump to the next argument and scan the rest */
    argv++;	argc--;
    while (argc--) {
	cp = *argv++;
	if (*cp == '-') {
	    for (cp++; *cp; cp++) {
		switch (*cp) {

		  case 'd':
		    debugMode = YES;
		    break;

		  default:
		    syslog(LOG_ERR, "unknown switch: -%c",
			     *cp);
		    break;
		}
	    }
	} else {
	    syslog(LOG_ERR, "invalid argument: %s", cp);
	}
    }

    /* Get the name of the host making sure it is null terminated */
    hostBuffer[sizeof(hostBuffer) - 1] = '\0';
    if (gethostname(hostBuffer, sizeof(hostBuffer) - 1)) {
	syslog(LOG_ERR, "gethostname system call failed: %m");
    } else {
	NXHostName = (char *)malloc(strlen(hostBuffer) + 1);
	strcpy(NXHostName, hostBuffer);
    }

    /* Shut down temporary syslog */
    closelog();

    if (!debugMode) {
	[self detach];
    }

    /* Set up logging */
    LogInit();
    malloc_debug(debugMode ? 15 : 5);
    malloc_error(LogMallocDebug);

    /* Set up a default StringManager */
    NXStringManager = [StringManager new];
    [NXStringManager loadFromArray:_strings];

    /* Initialize hash tables */
    portDelegates = nil;
    childHandlers = nil;

    /* Set up handler for signal message */
    if ((_signalNotifyPort = PortCreate(NULL)) == PORT_NULL) {
	LogError([NXStringManager stringFor:"Daemon_noport"]);
	[NXDaemon panic];
    }
    NX_DURING {
	DPSAddPort(_signalNotifyPort, (DPSPortProc)_signalMessageHandler,
		   sizeof(struct signal_msg), NULL, 0);
    } NX_HANDLER {
	LogError([NXStringManager stringFor:"Daemon_nosignotify"],
		 NXLocalHandler.code);
	[NXDaemon panic];
    } NX_ENDHANDLER;

    /* Set up default signal handler for death signals */
    signal(SIGHUP, _deathSignalHandler);
    signal(SIGINT, _deathSignalHandler);
    signal(SIGQUIT, _deathSignalHandler);
    signal(SIGTERM, _deathSignalHandler);

    /* Set up the child process signal handler */
    signal (SIGCHLD, _childSignalHandler);

    /* Set up the uncaught exception handler */
    NXSetUncaughtExceptionHandler(_uncaughtException);

    /* Set up routine to watch the Notify port */
    _notifySetUp();

    return (self);
}

- (char *)programName
{

    return (programName);
}

- (BOOL)getDebugMode
{

    return (debugMode);
}

- (port_t)mainPort
{
    return (mainPort);
}

- portDelegates
{

    return (portDelegates);
}

- childHandlers
{

    return (childHandlers);
}

- registerAs:(char *)portName withHandler:(DPSPortProc)handler
  maxMsgSize:(int)maxsize 
{
    port_t	tmpPort;

    /* See if there is an np already running */
    if ( (tmpPort = PortLookup(portName, NULL)) != PORT_NULL ) {
	LogError([NXStringManager stringFor:"Daemon_portbusy"]);
	PortFree(tmpPort);
	[self shutdown];
	exit(1);
    }

    /* Create a port and register us with the name server */
    if ( (mainPort = PortCreate(portName)) == PORT_NULL ) {
	LogError([NXStringManager stringFor:"Daemon_noreg"]);
	[self shutdown];
	exit (1);
    }

    /* Bind the port to its handler */
    DPSAddPort(mainPort, handler, maxsize, NULL, 0);

    return (self);
}

- setNotifyDelegate:(id) delegate forPort:(port_t)port
{
    id	oldDelegate;			/* old port delegate for a port */

    if (portDelegates == nil) {
	portDelegates = [HashTable newKeyDesc:"i"];
    }

    if (oldDelegate = [portDelegates insertKey:(const void *)port
		     value:delegate]) {
	LogDebug("Delegate for port %d replaced", port);
    }

    return (self);
}

- resetNotifyDelegate:(port_t)port
{
    id	oldDelegate;

    if (portDelegates == nil) {
	LogDebug("resetNotifyDelegate: portDelegates == nil");
	return (self);
    }

    oldDelegate = [portDelegates removeKey:(const void *)port];
    return (self);
}

- setChildHandler:(NXChildHandler)handler forProcess:(int)pid
  withData:(void *)data
{
    child_info_t	childInfo;
    int			oldmask;

    /* Block any child signals */
    oldmask = sigblock(sigmask(SIGCHLD));

    /* Create a handlers table is we need it. */
    if (childHandlers == nil) {
	childHandlers = [HashTable newKeyDesc:"i" valueDesc:"i"];
    }

    /* Create a new child info structure */
    childInfo = (child_info_t) malloc(sizeof (*childInfo));
    childInfo->handler = handler;
    childInfo->data = data;
    

    /* Add this handler to the table */
    if (childInfo = [childHandlers insertKey:(const void *)pid
		    value:childInfo]) {
	LogDebug("Handler for process %d replaced", pid);
	free((void *)childInfo);
    }

    /* Unblock the child signals */
    (void) sigsetmask(oldmask);

    return (self);
}

- resetChildHandler:(int)pid
{
    child_info_t	childInfo;
    int			oldmask;

    /* Block the child signals */
    oldmask = sigblock(sigmask(SIGCHLD));

    /* Sanity check */
    if (childHandlers == nil) {
	LogDebug("resetChildHandler: childHandlers == nil");
	(void) sigsetmask(oldmask);
	return (self);
    }

    /* Remove the handler */
    childInfo = [childHandlers removeKey:(const void *)pid];
    free((void *)childInfo);

    (void) sigsetmask(oldmask);    
    return (self);
}

- spawnChildProc:(NXChildProc)procedure data:(void *)data
  handler:(NXChildHandler)handler handlerData:(void *)hdata
{
    int		pid;
    int		oldmask;

    /* Block SIGCHLD */
    oldmask = sigblock(sigmask(SIGCHLD));

    if (pid = atom_fork()) {
	/* Parent */

	/* Set up the handler */
	if (handler) {
	    [self setChildHandler:handler forProcess:pid withData:hdata];
	}

	/* Unblock SIGCHLD */
	(void) sigsetmask(oldmask);
    } else {
	/* Child */

	/* Call the procedure */
	_exit((*procedure)(data));

	/* Never gets here */

    }
    return (self);
}

- run
{
    NXEvent dummyEvent;			/* dummy event for DPSGetEvent */

    while (TRUE) {
	NX_DURING {
	    while (TRUE) {
		DPSGetEvent(DPS_ALLCONTEXTS, &dummyEvent, -1, NX_FOREVER, 0);
		LogError([NXStringManager stringFor:"Daemon_unexpevent"]);
	    }
	} NX_HANDLER {
	    [self topLevelError:&NXLocalHandler];
	} NX_ENDHANDLER;
    }
}

- detach
{
    int	fd;			/* file descriptor */
    int	ndescriptors;		/* max number of file descriptors */

    /* Detach from parent */
    if (fork()) {
	exit(0);
    }

    /* Close all files */
    ndescriptors = getdtablesize();
    for (fd = 0; fd < ndescriptors; fd++) {
	(void) close(fd);
    }
    
    /* Open /dev/null as stdin, stdout, and stderr */
    if ((fd = open(DEVNULL, O_RDWR)) == 0) {
	(void) dup2(0, 1);
	(void) dup2(0, 2);
    } else if (fd > 0) {
	close(fd);
    }
    
    /* Clear controlling tty */
    if ((fd = open(DEVTTY, O_RDWR)) > 0) {
	(void) ioctl(fd, TIOCNOTTY, (char *)0);
	(void) close(fd);
    }

    return (self);
}

- shutdown
{

    if (mainPort != PORT_NULL) {
	PortFree(mainPort);
    }

    return (self);
}

- topLevelError:(NXHandler *)handler
{

    LogError([NXStringManager stringFor:"Daemon_toperror"], handler->code);
    return (self);
}

- panic
{

    /* Default activity is to dump core */
    _dumpCore();

    /* Doesn't get here, but let's keep the compuler quiet */
    return (self);
}

@end

