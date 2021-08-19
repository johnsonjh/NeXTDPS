/*
 * DocInfo.h	- Interface file to document info object.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */

#import <objc/Object.h>
#import <cthreads.h>
#import <dpsclient/dpsclient.h>

/*
 * Type definitions
 */

/* Known PostScript versions */
typedef enum {
    PS_NonConforming,			/* Unknown format */
    PS_Adobe_10,			/* %!PS-Adobe-1.0 */
    PS_Adobe_20,			/* %!PS-Adobe-2.0 */
    PS_Adobe_New,			/* %!PS-Adobe-<greater than 2.1> */
} version_t;

/* Proof Modes */
typedef enum {
    PM_TrustMe,				/* Trust me */
    PM_Substitute,			/* Substitute */
    PM_NotifyMe,			/* NotifyMe */
    PM_Unknown,				/* not yet set */
} proof_mode_t;

/* Bounding box */
typedef struct {
    int	llx;				/* X coord, lower left corner */
    int	lly;				/* Y coord, lower left corner */
    int	urx;				/* X coord, upper right corner */
    int	ury;				/* Y coord, upper right corner */
} boundbox_t;

/* Document or page property */
typedef struct prop_t {
    char		*p_name;	/* Name of the property */
    struct prop_t	*p_next;	/* Next property in list */
    BOOL		p_valsatend;	/* More values in the trailer */
    int			p_nvals;	/* Number of values */
    char		**p_vals;	/* Array of values */
} prop_t;

/* Paper information */
typedef struct {
    char	*p_name;		/* Paper name */
    NXSize	p_size;
} paper_t;


@interface DocInfo : Object
/*
 * The DocInfo object holds PostScript document information that is
 * parsed from the conforming comments.  This is used both by PrintJobs
 * and by the queue info routines in PrintSpooler.  Some weird locking
 * goes on because the queue info routines in PrintSpooler may run
 * in a different thread.
 *
 * To insure reasonable performance, the main thread is the only thread
 * that writes data into the DocInfo object which it locks during the
 * process.  All routines that read data from the DocInfo object that
 * can run in another thread must also lock the object before reading.
 * Any routines that read data from the object that only run in the main
 * thread do not have to lock the object.
 */
{
    mutex_t		lock;
    int			refcnt;
@public
    version_t		version;	/* PostScript version */
    char		*title;		/* document title */
    char		*creator;	/* creator of the document */
    char		*creationDate;	/* creation date */
    char		*forUser;	/* user who created document */
    char		*routing;	/* routing back to user */
    char		*printerRequired; /* printer requirements */
    proof_mode_t 	proofMode;	/* proof mode */
    boundbox_t		boundBox;	/* bounding box */
    int			pages;		/* number of pages */
    int			pageOrder;	/* page order */
    prop_t		*complexProps;	/* complex property lists */
    int			atEnd;		/* Document info in %%Trailer */
    unsigned int	bboxAtEnd : 1;
    unsigned int	pagesAtEnd : 1;
    unsigned int	pageOrderAtEnd : 1;
    unsigned int	pageOrderUnknown : 1;
/*
 * FIXME: These are a quick hack that should be replaced with general
 *	  resource support.
 */
    paper_t	paper;			/* default paper */
    int		resolution;		/* default resolution */
    BOOL	manualfeed;		/* default manual-feed */
}

/* Factory methods */

+ new;
/*
 * Creates a new DocInfo object with default values.
 */

+ newFromFile:(const char *) fileName size:(int)size;
/*
 * Creates a new DocInfo object and parses the file to initialize its data.
 */

/* Instance methods */
- parseHeaderFor:(id)printJob data:(const char *)data size:(int)size;
/*
 * Parse the document header for needed information.  This should be called
 * within an NX_DURING block.
 */

- parseTrailerFor:(id)printJob data:(const char *)data size:(int)size;
/*
 * Parse the document trailer for needed information.  This should be called
 * within an NX_DURING block.
 */

- (prop_t *)getComplexProp:(const char *)name;
/*
 * Get the property named "name" or return NULL if the property doesn't
 * exist.
 */

- hold;
/*
 * Increment the reference count of the object.
 */

- release;
/*
 * Decrement the reference count of the object.  If it becomes zero, free
 * the object.
 */

- lock;
/*
 * Get a mutual exclusion lock on the object.
 */

- unlock;
/*
 * Release the mutual exclusion lock.
 */
@end

