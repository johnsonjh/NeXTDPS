/*
 * SpoolingJob.m	- Implementation of spooling PrintJob
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */


/*
 * Include files.
 */

#import "SpoolingJob.h"
#import "PrintSpooler.h"
#import "NpdDaemon.h"
#import "log.h"

#import <libc.h>
#import <stdlib.h>
#import <string.h>
#import <errno.h>
#import <stdio.h>


/*
 * Constants
 */
static const char *_fileTemplate = "/usr/spool/appkit/spooled.%d";


/*
 * Local variables.
 */
static BOOL	_initdone = NO;

static int jobSequenceNumber = 0;

static struct string_entry _strings[] = {
    { "SpoolingJob_notemp", "could not open temporary spooling file %s: %m" },
    { "SpoolingJob_chownfail", "could not chown %s: %m" },
    { "SpoolingJob_closefailed", "close of %s failed: %m" },
    { "SpoolingJob_unlinkfailed", "unlink of %s failed: %m" },
    { "SpoolingJob_writeerror", "write error" },
    { NULL, NULL },
};


@implementation SpoolingJob : PrintJob

/* Factory methods */

+ newPrinter:(id)Spooler user:(char *)User host:(char *)Host
  copies:(int)Copies remotePort:(port_t)RemotePort fromLpd:(BOOL)FromLpd
{
    char tempStr[100];		/* A place to build the spool file path */

    if (!_initdone) {
	[NXStringManager loadFromArray:_strings];
	_initdone = YES;
    }

    /* Instantiate */
    if ((self = [super newPrinter:Spooler user:User host:Host
	       copies:Copies remotePort:RemotePort fromLpd:FromLpd]) == nil) {
	return (nil);
    }

    /* Open the spool file */
    while(1) {		/* Open spool file.  Make sure it did not exist before. */
	jobSequenceNumber++;
	sprintf(tempStr, _fileTemplate, jobSequenceNumber);
	spoolFile = open(tempStr, O_CREAT|O_EXCL|O_RDWR, 0600);
	if (spoolFile > 0 || (spoolFile < 0 && cthread_errno() != EEXIST)) break;
    }
    spoolFilePath = (char *) malloc(strlen(tempStr) + 1);
    strcpy(spoolFilePath, tempStr);
    if (spoolFile < 0) {
	LogError([NXStringManager stringFor:"SpoolingJob_notemp"],
		 spoolFilePath);
	[self free];
	return (nil);
    }

    /* Set the ownership and group */

    if (fchown(spoolFile, user_uid, user_gid) == -1) {
	LogError([NXStringManager stringFor:"SpoolingJob_chownfail"],
		 spoolFilePath);
    }

    return (self);
}

/* Instance methods */

- processHeader:(char *)data size:(unsigned int)size wasInline:(BOOL)wasInline
{

    /* Write out the data */
    return([self processPage:data size:size wasInline:wasInline]);
}

- processPage:(char *)data size:(unsigned int)size wasInline:(BOOL)wasInline
{
    int		cc;			/* character counter */

    /* Write out the data */
    while ((cc = write(spoolFile, data, size)) != (int) size) {
	if (cc < 0) {
	    NX_RAISE(NPDspooling,
		     [NXStringManager stringFor:"SpoolingJob_writeerror"],
		     spoolFilePath);
	}
	data += cc;
	size -= cc;
    }

    return (self);
}

- processTrailer:(char *)data size:(unsigned int)size wasInline:(BOOL)wasInline
{
    int		tmpFd;			/* temporary file descriptor */

    /* Write out the data */
    if (!flushing) {
	[self processPage:data size:size wasInline:wasInline];
    }

    /* Close the file */
    tmpFd = spoolFile;
    spoolFile = -1;
    if (close (tmpFd)) {
	LogError([NXStringManager stringFor:"SpoolingJob_closefailed"],
		 spoolFilePath);
	goto errout;
    }

    /* Spool the file */
    [spooler spool:self fromFile:spoolFilePath user:user_uid group:user_gid];

    /* And free up ourself */
    return ([self free]);


errout:
    /* We have had errors, so unlink the file */
    if (unlink(spoolFilePath)) {
	LogError([NXStringManager stringFor:"SpoolingJob_unlinkfailed"],
		 spoolFilePath);
    }

    return([self free]);
}

- free
{

    /* Close and unlink the spool file if it is still open */
    if (spoolFile >= 0) {
	if (close (spoolFile)) {
	    LogError([NXStringManager stringFor:"SpoolingJob_closefailed"],
		     spoolFilePath);
	}

	if (unlink(spoolFilePath)) {
	    LogError([NXStringManager stringFor:"SpoolingJob_unlinkfailed"],
		     spoolFilePath);
	}
    }

    /* Free allocated memory */
    free(spoolFilePath);

    return([super free]);
}
    
@end

