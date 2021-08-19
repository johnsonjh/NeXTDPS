/*
 * PrintSpooler.h	- Interface file for the PrintSpooler object.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */

#import <objc/Object.h>

/*
 * Type definitions.
 */

/* Information about a particular queue entry */
typedef struct queue_entry {
    int			refcnt;
    char		*controlFile;
    char		*host;
    char		*user;
    char		*files;
    int			job;
    int			size;
    time_t		time;
    DocInfo		*docinfo;
} *queue_entry_t;
    
/* Information about the spooler's queue at a particular point in time. */
typedef struct queue_info {
    mutex_t		lock;
    condition_t		readerdone;
    int			readcount;
    int			nentries;
    queue_entry_t	*entries;
} *queue_info_t;


@interface	PrintSpooler : Object
/*
 * A PrintSpooler object holds the administrative information about a
 * printer which it gets from NetInfo.  It provides a nice caching
 * mechanism and interface to the NetInfo data.  It also provides routines
 * to interface to the spooler for this printer.
 */
{
    int		refcnt;			/* Reference count */
@public
    char	**names;		/* The names of this printer */
    char	*spoolDir;		/* Spool directory */
    char	*lockFile;		/* Lock file */
    char	*accountFile;		/* Accounting file */
    char	*devicePath;		/* Path to printer device */
    char	*remoteHost;		/* Host if printer is remote */
    char	*remotePrinter;		/* Printer on remote host */
    char	*inputFilter;		/* LPD input filter */
    char	*bannerBefore;		/* Banner page to print before job */
    char	*bannerAfter;		/* Banner page to print after job */
    BOOL	unavailable;		/* _ignore flag is set */
    BOOL	remoteAsNobody;		/* RemoteAsNobody flag set */
    int		lockFd;			/* fd for the lock file */
    int		lockOffset;		/* offset into lockfile for cf info */
    char	*cfname;		/* Name of temp control file */
    id		entryCache;		/* Hash table for caching queue
					   entries */
    id		currentDocInfo;		/* Current DocInfo if local printing */
    time_t	cacheTime;		/* Modification time of queue cache */
    queue_info_t	queueInfo;	/* Cache of the print queue */
    BOOL	delayedFree;		/* Delay before finally freeing */
    DPSTimedEntry	freeTE;		/* Timed entry for delayed free */
}

/* Factory methods */
+newPrinter:(const char *)name;
/*
 * Create a new PrintSpooler object for the printer "name".
 */

/* Instance methods */

- (queue_entry_t) _queueEntryLookup:(const char *)cfFile;
/*
 * Get a queue entry for a particular control file in the entry cache.
 * If the entry has not been read yet, create a new one.  This also knows
 * to deal with the fake queue entry for local jobs.
 */

- (void) _queueEntryRelease:(queue_entry_t) qEntry;
/*
 * Decrement the reference count on a queue entry.  If it is zero, free
 * up the structure.
 */

- (queue_info_t) _queueInfoRead;
/*
 * Read the current queue information out of the spooling directory
 * and return it in a new queue_info struct.
 */

- (void) _queueInfoFree:(queue_info_t)queueInfo;
/*
 * Free up the queue_info structure.  Also free's any entries that are
 * no longer in use.
 */

- (BOOL)lockForPrinting;
/*
 * Lock the spooling directory.  This keeps lpd from running in the
 * spool directory for a local printer.  Returns YES on success, otherwise
 * returns NO.
 */

- fakeQueueEntry:(id)job;
/*
 * Create a fake "cf" file so that lpq and displayq routines can see
 * the current job if it is a local one.
 */

- unlockAndLog:(id)job pages:(int)pages;
/*
 * Unlock the spooling directory for printing.  If there is an accounting
 * file, log the number of pages printed.
 */

- logUsage:(id)job pages:(int)pages;
/*
 * If there is an accounting file, log the number of pages printed.
 */

- (BOOL)queueEnabled;
/*
 * Let us know whether the printer is enabled for spooling.
 */

- spool:(id)job fromFile:(const char *)filePath user:(int)uid group:(int)gid;
/*
 * Spool the document that is stored in filePath.
 */

- sendNPD1InfoTo: (port_t)port;
/*
 * Send information about the current queue to port using the NPD 1.0
 * protocol.
 */

- (BOOL)isNamed:(const char *)name;
/*
 * Returns YES if "name" is one of the spoolers names.
 */

- (BOOL)isUnavailable;
/*
 * Returns YES if the printer is not available.
 */

- (BOOL)remoteAsNobody;
/*
 * Returns YES if remote jobs should be run as nobody.
 */

- setDelayedFree:(BOOL)dFree;
/*
 * Set whether to use a delayed free or not.
 */

- (DPSTimedEntry) freeTE;
/*
 * Return the timed entry used to execute the delayed free.
 */

- refresh;
/*
 * Reread the printer database and update the administrative information.
 */

- hold;
/*
 * Increment the reference count of the spooler so that it doesn't get
 * freed while still in use.
 */
@end

