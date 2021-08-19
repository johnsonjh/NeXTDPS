/*
 * LocalJob.h	- Interface to a locally printing PrintJob
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */

#import "PrintJob.h"

/*
 * Type definitions.
 */
typedef struct pageq_entry {
    struct pageq_entry	*pq_next;	/* next entry in queue */
    struct pageq_entry	*pq_prev;	/* previous entry in queue */
    char		*pq_data;	/* Honest-to-god data */
    int			pq_size;	/* Size of the data */
    BOOL		pq_mallocd;	/* Whether the data was malloc'd */
    int			pq_copies;	/* Copies of entry left to print */
} pageq_entry_t;
    
typedef enum {
    Panel,
    Voice,
    Both,
} alert_style_t;

@interface LocalJob : PrintJob
{
    alert_style_t	alertStyle;
@public
    id			localPrinter;
    pageq_entry_t	*header;
    pageq_entry_t	*trailer;
    pageq_entry_t	*pageq_head;
    pageq_entry_t	*pageq_tail;
    unsigned int	trailerReceived : 1;
    unsigned int	doing_beforebanner : 1;
    unsigned int	doing_mainjob : 1;
}

/* Instance methods */

- _trueProcessTrailer:(char *)data size:(unsigned int)size
  wasInline:(BOOL)wasInline;
/*
 * This does the work of processing the trailer of the real job.
 */
  
- donePrinting;
/*
 * This is invoked by the localPrinter when it has finished processing
 * all of the work that it has.
 */

- errorPrinting;
/*
 * This is invoked by the localPrinter when it has had an error
 * printing a page.
 */

- handleAlert:(char *)alertStr;
/*
 * Alert the user of this job that there is a printer error of some sort.
 */

- handleNpAlert:(int) errorCode;
/*
 * Alert the user of this job that there is a NeXT Printer specific error.
 */

- bannerFromFile:(const char *)fileName;
/*
 * Submit a banner job using fileName as the banner prologue.
 */
@end
