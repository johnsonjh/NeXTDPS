/*
 * SpoolingJob.h	- Interface file to an actively printing PrintJob
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */

#import "PrintJob.h"

@interface SpoolingJob : PrintJob
{
    int			spoolFile;	/* fd for file to spool to */
    char		*spoolFilePath;	/* Path to spool file */
}

@end
