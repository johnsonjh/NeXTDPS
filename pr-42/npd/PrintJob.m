/*
 * PrintJob.m	- Implementation of an abstract print job.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */


/*
 * Include files.
 */

#import "PrintJob.h"
#import "NpdDaemon.h"
#import "DocInfo.h"
#import "PrintSpooler.h"
#import "mach_ipc.h"
#import "npd_prot.h"
#import "log.h"

#import <objc/List.h>

#import <limits.h>
#import <pwd.h>
#import <string.h>
#import <stddef.h>
#import <stdlib.h>
#import <sys/message.h>


/*
 * Constants
 */

static const char *DEFUSER = "nobody";
static int	DEFUID = -2;		/* Nobody */
static int	DEFGID = -2;		/* Nogroup */


/*
 * Static variables.
 */

static	BOOL	_initdone = NO;
static	id	_jobList = nil;

static struct string_entry _strings[] = {
    { "PrintJob_nouser", "print job requested with no user" },
    { "PrintJob_addportfailed", "PrintJob DPSAddPort failed, code = %d" },
    { "PrintJob_cantrmvport", "PrintJob DPSRemovePort failed, code = %d" },
    { "PrintJob_badportdied", "PrintJob notified of wrong port death" },
    { "PrintJob_vmdealloc",
	  "PrintJob can't deallocate message data, code = %d" },
    { "PrintJob_nospooler", "PrintJob creation attempt without a spooler" },
    { "PrintJob_portconflict",
	  "warning: remote port mismatch of incoming page" },
    { "PrintJob_docformat", "document format error: %s: \"%s\"" },
    { "PrintJob_spoolerror", "spool error: %s - %m: %s" },
    { "PrintJob_getpwfailed", "could not get password entry for %s" },
    { NULL, NULL },
};


/*
 * Static routines.
 */

/**********************************************************************
 * Routine:	_confirmTrailer() - Send a reply to the TRAILER message.
 *
 * Function:	The reply message let's the client know that we have
 *		received and theoretically processed the TRAILER message.
 *
 * Args:	msg:	The message containing the TRAILER.
 **********************************************************************/
static void
_confirmTrailer(port_t replyPort)
{
    msg_header_t	replyMsg;
    msg_return_t	msg_ret;

    bzero(&replyMsg, sizeof(replyMsg));
    MSGHEAD_SIMPLE_INIT(replyMsg, sizeof(replyMsg));
    replyMsg.msg_local_port = PORT_NULL;
    replyMsg.msg_remote_port = replyPort;

    /* We don't want to block */
    /* FIXME: This would do a SEND_NOTIFY if it were a reasonable interface */
    msg_ret = msg_send(&replyMsg, SEND_TIMEOUT, 0);
    if (msg_ret != SEND_SUCCESS) {
	LogDebug("PrintJob reply failed, code = %d", msg_ret);
    }
}

/**********************************************************************
 * Routine:	_pageHandler() - Handle incoming pages.
 *
 * Function:	This is the protocol handler for the incoming document.
 *		It does some sanity checking on the message format
 *		and then dispatches the incoming data.
 *
 * Args:	msg:		The incoming message.
 *		printJob:	id of the printJob this is for.
 **********************************************************************/
static void
_pageHandler(npd1_receive_msg *msg, PrintJob *printJob)
{
    unsigned int	size;
    char		*data;
    msg_type_long_t	*type_long = &msg->type;
    msg_type_t		*type_short = (msg_type_t *)&msg->type;
    kern_return_t	kern_ret;
    
    /* A little sanity checking */
    if (msg->head.msg_remote_port != [printJob remotePort]) {
	LogError([NXStringManager stringFor:"PrintJob_portconflict"]);
    }

    /* Tell the AppKit we have received its messages */
    if (msg->head.msg_id == NPD_SEND_TRAILER && !printJob->fromLpd) {
	/* We know the port is going to die, so don't watch it any more */
	[NXDaemon resetNotifyDelegate:msg->head.msg_remote_port];
	_confirmTrailer(msg->head.msg_remote_port);
    }

    /* Figure out how much data we have and where it is */
    if (type_short->msg_type_longform) {
	size = type_long->msg_type_long_size *
	    type_long->msg_type_long_number / CHAR_BIT;
	data = (char *) (type_long) + sizeof(*type_long);
    } else {
	size = type_short->msg_type_size * type_short->msg_type_number /
	    CHAR_BIT;
	data = (char *) (type_short) + sizeof(*type_short);
    }
    
    /* If the data is not inline, dereference the pointer */
    if (!type_short->msg_type_inline) {
	data = * (char **) data;
    }

    /* Process the message */
    NX_DURING {
	switch (msg->head.msg_id) {

	  case NPD_SEND_HEADER:
	    /* Parse the document information */
	    [[printJob docinfo] parseHeaderFor:printJob data:data
	   size:size];

	    /* Process header */
	    [printJob processHeader:data size:size
		wasInline:(type_short->msg_type_inline ? YES : NO)];
	    break;

	  case NPD_SEND_PAGE:
	    /* Process page */
	    if (!printJob->flushing) {
		[printJob processPage:data size:size
			wasInline:(type_short->msg_type_inline ? YES : NO)];
	    }
	    break;

	  case NPD_SEND_TRAILER:
	    /* Parse the document information */
	    [[printJob docinfo] parseTrailerFor:printJob
		data:data size:size];
	    
	    /* Process trailer */
	    [printJob processTrailer:data size:size
		wasInline:(type_short->msg_type_inline ? YES : NO)];
	    break;
	}
    } NX_HANDLER {
	if (NXLocalHandler.code == NPDdocformat) {
	    LogError([NXStringManager stringFor:"PrintJob_docformat"],
		     NXLocalHandler.data1, NXLocalHandler.data2);
	} else if (NXLocalHandler.code == NPDspooling) {
	    LogError([NXStringManager stringFor:"PrintJob_spoolerror"],
		     NXLocalHandler.data1, NXLocalHandler.data2);
	} else if (NXLocalHandler.code != NPDlocalprinting) {
	    NX_RERAISE();
	}
	[printJob setFlushing:1];
    } NX_ENDHANDLER;

    /* Free up the data if necessary */
    if (!type_short->msg_type_inline) {
	if ((kern_ret = vm_deallocate(task_self(), (vm_address_t) data,
				      size)) != KERN_SUCCESS) {
	    LogError([NXStringManager stringFor:"PrintJob_vmdealloc"],
		     kern_ret);
	    [NXDaemon panic];
	}
    }
}


/*
 * Method implementations.
 */

@implementation PrintJob : Object

/* Factory methods */

+ newPrinter:(id)Spooler user:(const char *)User host:(const char *)Host
  copies:(int)Copies remotePort:(port_t)RemotePort fromLpd:(BOOL)FromLpd
{
    struct passwd	*pw;
    port_t		tmpPort;

    if (!_initdone) {
	[NXStringManager loadFromArray:_strings];
	_jobList = [List new];
	_initdone = YES;
    }

    if ((tmpPort = PortCreate(NULL)) == PORT_NULL) {
	return (nil);
    }

    if (!Spooler) {
	LogError([NXStringManager stringFor:"PrintJob_nospooler"]);
	return (nil);
    }

    /* Instantiate */
    self = [super new];
    localPort = tmpPort;
    remotePort = RemotePort;
    spooler = Spooler;
    docinfo = [DocInfo new];
    flushing = 0;

    /* Copy the job information */
    if (!*User) {
	LogWarning([NXStringManager stringFor:"PrintJob_nouser"]);
	User = DEFUSER;
    }
    user = (char *) malloc(strlen(User) + 1);
    if (!User) {
	user[0] = '\0';
    } else {
	strcpy(user, User);
    }

    if (!Host || !*Host) {
	host = (char *) malloc(strlen(NXHostName) + 1);
	strcpy(host, NXHostName);
    } else {
	host = (char *) malloc(strlen(Host) + 1);
	strcpy(host, Host);
    }

    /*
     * If we know who this is and it's not root and this is not a remote
     * job with "RemoteAsNobody" set, then we set the uid to the real user.
     * Otherwise, use nobody.
     */
    if ((pw = getpwnam(user)) && pw->pw_uid &&
	!(strcmp(host, NXHostName) && [spooler remoteAsNobody])) {
	user_uid = pw->pw_uid;
	user_gid = pw->pw_gid;
    } else {
	user_uid = DEFUID;
	user_gid = DEFGID;
    }

    if (Copies <= 0) {
	copies = 1;
    } else {
	copies = Copies;
    }

    fromLpd = FromLpd;

    /* Register our port handler */
    NX_DURING {
	DPSAddPort(localPort, (DPSPortProc)_pageHandler,
		   sizeof(npd1_receive_msg), (void *)self, 0);
    } NX_HANDLER {
	LogError([NXStringManager stringFor:"PrintJob_addportfailed"],
		 NXLocalHandler.code);
	goto errout;
    } NX_ENDHANDLER;

    /* Make us the port delegate for the remote port */
    [NXDaemon setNotifyDelegate:self forPort:RemotePort];

    /* Add ourself to the job list */
    [_jobList addObject:self];

    return (self);

errout:
    PortFree(tmpPort);
    [super free];
    return (nil);
}

+ shutdown
{
    PrintJob	*job;

    if (_initdone) {
	while (job = (PrintJob *)[_jobList removeLastObject]) {
	    [job free];
	}
    }
    return (nil);
}

/* Instance methods */

- (port_t)localPort
{

    return (localPort);
}

- (port_t)remotePort
{

    return (remotePort);
}

- docinfo
{
    return (docinfo);
}

- (int) copies
{
    return (copies);
}

- setFlushing:(unsigned int)newVal {

    flushing = newVal;
    return (self);
}

- processHeader:(char *)data size:(unsigned int)size wasInline:(BOOL)wasInline
{
    /* By default this does nothing.  It is intended to be overridden */
    return (self);
}

- processPage:(char *)data size:(unsigned int)size wasInline:(BOOL)wasInline
{
    /* By default this does nothing.  It is intended to be overridden */
    return (self);
}

- processTrailer:(char *)data size:(unsigned int)size wasInline:(BOOL)wasInline
{
    /* By default this does nothing.  It is intended to be overridden */
    return (self);
}

- portDied:(port_t)port
{

    if (port != remotePort) {
	LogError([NXStringManager stringFor:"PrintJob_badportdied"]);
	return (nil);
    }
    remotePort = PORT_NULL;
    return ([self free]);
}

- free
{

    /* Unregister us as the delegate for the remote port */
    [NXDaemon resetNotifyDelegate:remotePort];

    /* Tell npd we've received its messages */
    if (fromLpd && remotePort) {
	_confirmTrailer(remotePort);
    }

    /* Unregister our port handler */
    NX_DURING {
	DPSRemovePort(localPort);
    } NX_HANDLER {
	LogError([NXStringManager stringFor:"PrintJob_cantrmvport"],
		 NXLocalHandler.code);
    } NX_ENDHANDLER;

    /* Free our job information */
    free(user);
    free(host);
    [spooler free];
    [docinfo release];

    /* Free the ports we've been using */
    PortFree(remotePort);
    PortFree(localPort);
    
    /* Remove ourself from the job list */
    [_jobList removeObject:self];

    /* Uninstantiate ourselves */
    return ([super free]);
}

@end

