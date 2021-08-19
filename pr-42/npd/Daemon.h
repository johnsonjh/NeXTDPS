/*
 * Daemon.h	Interface file for the daemon object.
 *
 * Copyright (c) 1990 by NeXT, Inc.,  All rights reserved.
 */

#import "StringManager.h"

#import	<objc/Object.h>
#import	<dpsclient/dpsclient.h>
#import	<mach.h>
#import <sys/wait.h>

/*
 * Type definitions
 */
typedef int	(*NXChildProc)(void *data);
typedef void	(*NXChildHandler)(int pid, union wait *status, void *data);


/*
 * Global variables
 */
extern id		NXDaemon;	/* The daemon itself */
extern StringManager	*NXStringManager; /* The daemon's string manager */
extern char		*NXHostName;	/* This hosts name */


@interface Daemon : Object
    /*
     * The Daemon class provides the framework for a system daemon.  It
     * keeps track of the various objects that make up the daemon and
     * handles the daemon event loop.
     */
{
    char	*programName;
    BOOL	debugMode;
    port_t	mainPort;		/* Our public port */
    id		portDelegates;		/* port delegates */
    id		childHandlers;		/* child process handlers */
}

+ newWithArgs:(int)argc :(char **)argv;
/*
 * Creates a new npd, parses the arguments, and initializes things.
 */


- (char *)programName;
/*
 * Return the name of the program
 */

- (BOOL)getDebugMode;
/*
 * Return whether we are in debug mode
 */

- (port_t) mainPort;
/*
 * Return the daemon's main port.
 */

- portDelegates;
/*
 * Return the portDelegates HashTable
 */

- childHandlers;
/*
 * Return the childHandlers HashTable
 */

- registerAs:(const char *)portName withHandler:(DPSPortProc)handler
    maxMsgSize:(int)maxsize;
/*
 * Creates a main port and registers it with the Mach port network name
 * server.  The daemon will exit if there is a failure doing this.
 */

- setNotifyDelegate:(id)delegate forPort:(port_t)port;
/*
 * setNotifyDelegate registers an object as a port delegate.  PortDelegate
 * methods are defined below.
 */

- resetNotifyDelegate:(port_t)port;
/*
 * resetNotifyDelegate removes an object from being a port Delegate
 * This is done automatically when a port dies.
 */

- setChildHandler:(NXChildHandler)handler forProcess:(int)pid
    withData:(void *)data;
/*
 * Establish a procedure to be called when a child processes exits.
 * The procedure will be called with the pid of the process and the
 * status.
 */

- resetChildHandler:(int)pid;
/*
 * Remove a the handler for a child process from the childHandler table.
 * This is done automatically when a child exits.
 */

- spawnChildProc:(NXChildProc)procedure data:(void *)data
  handler:(NXChildHandler)handler handlerData:(void *)hData;
/*
 * Spawn a child process to run "procedure" with "data" as its argument.
 * "handler" will be called when the child exits.
 */

- run;
/*
 * Start up the event loop.  This method never returns.  It all goes
 * down the stack from here.
 */

- detach;
/*
 * Detach this daemon from the controlling terminal and the parent
 * process.
 */

- shutdown;
/*
 * Shuts down the daemon as gracefully as possible.
 */

- topLevelError:(NXHandler *)handler;
/*
 * method sent to Daemon when a top level error has occured.  Default
 * action not documented yet.
 */

- panic;
/*
 * "panic" is the last resort message sent to the daemon by various
 * error handlers when things are beyond control.  The default action
 * is to dump core.
 */

@end

@interface PortDelegates : Object
/*
 * Interface to an object that gets notified about changes to a port's
 * status
 */
{}

- portDied:(port_t)port;
/*
 * Method sent to a port delegate when it's port dies.  This allows the
 * delegate to clean up if necessary.
 */

- portMsgReceived:(port_t)port;
/*
 * Method sent to a port delegate when the port received a message
 * previously sent with SEND_NOTIFY.
 */

@end

