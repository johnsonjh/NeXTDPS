/*
 * PrintJob.h	- Interface to the abstract PrintJob class
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */

#import <objc/Object.h>
#import <sys/message.h>
#import "DocInfo.h"
#import "PrintSpooler.h"

@interface PrintJob : Object
{
    port_t		localPort;	/* The local port for the job */
    port_t		remotePort;	/* The remote port for this job */
@public
    DocInfo		*docinfo;	/* Information about the document */
    PrintSpooler	*spooler;	/* The PrintSpooler for this job */
    char		*user;		/* The user */
    char		*host;		/* This originating host */
    int			copies;		/* The number of copies */
    int			user_uid;	/* UID of user */
    int			user_gid;	/* GID of user */
    unsigned int	fromLpd : 1;	/* This job is from lpd */
    unsigned int	flushing: 1;	/* We are flushing the output */
}

/* Factory methods */
+ newPrinter:(id)Spooler user:(const char *)User host:(const char *)Host
  copies:(int)Copies remotePort:(port_t)RemotePort fromLpd:(BOOL)FromLpd;
/*
 * Create a new print job for the printer represented by the
 * "PrintSpooler" object.  "user" is the username of the person
 * submitting the job.  "host" is the name of the host the job
 * originated on.  "copies" is the number of copies to make of the
 * document.  "fromLpd" is a flag that means this job is coming from
 * lpd, so don't try to lock the spooling subsystem.
 */

+ shutdown;
/*
 * This factory method shuts down all the existing print jobs.
 */

- (port_t)localPort;
/*
 * Return the local port for the print job.
 */

- (port_t)remotePort;
/*
 * Return the remote port of the print job.
 */

- docinfo;
/*
 * Return the id of the DocInfo object.
 */

- (int)copies;
/*
 * Return the number of copies this job wants.
 */

- setFlushing:(unsigned int)newVal;
/*
 * Set flushing on/off.
 */

- processHeader:(const char *)data size:(unsigned int)size
  wasInline:(BOOL)wasInline;
/*
 * Override this method to process the header data.  If wasInline is YES
 * and the data needs to be retained, it must be copied with bcopy.
 * If NO, the data can be copied with vm_copy.
 */

- processPage:(const char *)data size:(unsigned int)Size
  wasInline:(BOOL)wasInline;
/*
 * Override this method to process the page data.  If wasInline is YES
 * and the data needs to be retained, it must be copied with bcopy.
 * If NO, the data can be copied with vm_copy.
 */

- processTrailer:(const char *)data size:(unsigned int)Size
  wasInline:(BOOL)wasInline;
/*
 * Override this method to process the trailer data.  If wasInline is YES
 * and the data needs to be retained, it must be copied with bcopy.
 * If NO, the data can be copied with vm_copy.
 */

- portDied:(port_t)port;
/*
 * Sent to us when our job port dies.
 */

- free;
/*
 * Called to shut down a print job no matter what state it is in and then
 * free it.
 */

@end

