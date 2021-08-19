/*
 * NpdDaemon.m	- Implementation of the Next Printer Daemon
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */

/*
 * Include files
 */
#import "log.h"
#import "mach_ipc.h"
#import "atomopen.h"
#import "npd_prot.h"
#import "protocol1.h"

#import "NpdDaemon.h"
#import "StringManager.h"
#import "NextPrinter.h"
#import "PrintJob.h"

#import <libc.h>
#import <sys/signal.h>
#import <mach.h>
#import <mach_error.h>
#import <sys/file.h>
#import <sys/ioctl.h>
#import <nextdev/snd_msgs.h>
#import <servers/bootstrap.h>

/*
 * Constants
 */

/* Back door name for the window server */
unsigned char	server_key[] = { 0x4e, 0x5e, 0x4e, 0x75, 0 };

/* Our error strings */
static struct string_entry _strings[] = {
    { "Npd_unrecmsg", "Unrecognized message on main port, id = %d" },
    { "Npd_badsound","Could not get sound port: %s" },
    { NULL, NULL },
};


/*
 * Static routines
 */

/**********************************************************************
 * Routine:	check_sound_port_registered() - Make sure sound port is available
 *
 * Function:	Look up the sound device port on the bootstrap port.  If it's not there,
 *			look it up from the driver and check it into the bootstrap port.
 **********************************************************************/
kern_return_t check_sound_port_registered(void)
{
	kern_return_t r;
	port_t dev_port;
	int fd;
	msg_header_t msg;

	
	r = bootstrap_look_up(bootstrap_port, "sound", &dev_port);
	if (r == BOOTSTRAP_SUCCESS)
		return  BOOTSTRAP_SUCCESS;

	if ((fd = open("/dev/sound", O_RDONLY, 0)) < 0)
		return KERN_FAILURE;
	
	if (ioctl(fd, SOUNDIOCDEVPORT, (int)thread_reply()) < 0) {
		close(fd);
		return KERN_FAILURE;
	}

	close(fd);

	msg.msg_local_port = thread_reply();
	msg.msg_size = sizeof(msg);

	r = msg_receive(&msg, MSG_OPTION_NONE, 0);
	if (r != RCV_SUCCESS)
		return r;
	
	if (msg.msg_id != SND_MSG_RET_DEVICE)
		return KERN_FAILURE;

	dev_port = msg.msg_remote_port;

	return bootstrap_register(bootstrap_port, "sound", dev_port);
}
		
/**********************************************************************
 * Routine:	new_bootstrap_port() - Access sound device port
 *
 * Function:	Look up and check in sound device port into private bootstrap port.
 **********************************************************************/
static kern_return_t new_bootstrap_port(void)
{
	int r;
	port_t bs_subset;

	r = bootstrap_subset(bootstrap_port, task_self(), &bs_subset);
	if (r != KERN_SUCCESS)
		return r;

	r = task_set_bootstrap_port(task_self(), bs_subset);
	if (r != KERN_SUCCESS)
		return r;

	bootstrap_port = bs_subset;

	return check_sound_port_registered();
}

/**********************************************************************
 * Routine:	_lpdStyleAbort() - Handle abort request from lpd
 *
 * Function:	lpd, and lprm use SIGINT to get a daemon to drop
 *		a running job.
 **********************************************************************/
static void
_lpdStyleAbort()
{
    /*
     * So that we don't step all over ourselves, we send a
     * protocol 1.0 Abort message to ourself.
     */
     NPD1_AbortSelf();
}

/**********************************************************************
 * Routine:	_npdMainHandler() - Handle messages to public port
 *
 * Function:	This routine interprets the messages coming in on
 *		the main port, decides whether they are part of
 *		a supported protocol version and passes them off
 *		the appropriate handlers.
 **********************************************************************/
static void
_npdMainHandler(msg_header_t *msg, void *userData)
{
    int		msg_id;			/* The message id */

    msg_id = msg->msg_id;

    /* Check to see if it is protocol 1.0 */
    if (msg_id >= NPD_1_INFO && msg_id <=NPD_1_PRINTER_STATUS) {
	return (NPD1_Connect((npd1_con_msg *)msg));
    }

    /* Must be bogus */
    LogError([NXStringManager stringFor:"Npd_unrecmsg"], msg_id);
}


/*
 * Method definition
 */

@implementation	NpdDaemon : Daemon

/*
 * Factory methods
 */

+ newWithArgs:(int)argc:(char **)argv
{
    kern_return_t r;

    self = [super newWithArgs:argc:argv];

    /* Initialize the strings */
    [NXStringManager loadFromArray:_strings];
    
    /* Get our sound port */
    r = new_bootstrap_port();
    if (r != KERN_SUCCESS) {
	LogError([NXStringManager stringFor:"Npd_badsound"], mach_error_string(r));
    }

    /* Initialize the Protocol 1.0 handler */
    NPD1_Init();

    /* Register ourselves with the name server */
    [self registerAs: (NPD_PUBLIC_PORT)
	       withHandler:_npdMainHandler
	       maxMsgSize:(int)(sizeof(npd1_con_msg) + 4096)];

    /* Create a local printer object */
    localPrinter = [NextPrinter new];

    /*
     * Register our own SIGINT handler that will deal with the lpd
     * system's way of aborting a running job.
     */
    signal(SIGINT, _lpdStyleAbort);

    return (self);
}

/*
 * Instance methods
 */
- localPrinter
{

    return (localPrinter);
}

-shutdown
{

    /* Shut down all the running jobs */
    [PrintJob shutdown];

    if (localPrinter) {
	[localPrinter free];
    }
    [super shutdown];

    return (self);
}

@end


/**********************************************************************
 * Routine:	main() - The main program.
 *
 * Function:	It's really rather simple.
 **********************************************************************/
void
main( int argc, char **argv )
{

    init_atom_open();
    
    /* Instantiate the daemon and set it running */
    [[NpdDaemon newWithArgs:argc:argv] run];

    /* Should never get here */
    exit(1);
}

