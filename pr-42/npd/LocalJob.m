/*
 * LocalJob.m	- Implementation of a locally printing PrintJob
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */

#import "LocalJob.h"
#import "NextPrinter.h"
#import "NpdDaemon.h"
#import "PrintSpooler.h"
#import "log.h"
#import "protocol1.h"
#import "atomopen.h"

#import <fcntl.h>
#import <libc.h>
#import <mach.h>
#import <stdlib.h>
#import <string.h>
#import <appkit/defaults.h>
#import <vm/vm_param.h>
#import <nextdev/npio.h>
#import <sys/stat.h>


/*
 * Internal type definitions.
 */
struct errorVoiceInfo {
  u_int error;
  char *sound;
};


/*
 * Local variables.
 */

static BOOL	_initdone = NO;

struct errorVoiceInfo errorVoices[] = {
    { NPDOOROPEN, "/usr/lib/NextPrinter/printeropen.snd" },
    { NPPAPERJAM, "/usr/lib/NextPrinter/paperjam.snd" },
    { NPMANUALFEED, "/usr/lib/NextPrinter/manualfeed.snd" },
    { NPNOPAPER, "/usr/lib/NextPrinter/nopaper.snd" },
    { 0, NULL },
};

static struct string_entry _strings[] = {
    { "LocalJob_mallocfail", "malloc failed in LocalJob" },
    { "LocalJob_vmallocfail",
	  "vm_allocate failed in LocalJob: error = %d" },
    { "LocalJob_vmcopyfail",
	  	  "vm_copy failed in LocalJob: error = %d" },
    { "LocalJob_vmdeallocfail",
	  	  "vm_deallocate failed in LocalJob: error = %d" },
    { "LocaJob_nobanner", "Could not open banner file %s: %m" },
    { "LocalJob_nostat", "Could not stat file %s: %m" },
    { "LocalJob_nomap", "Could not map file %s: error=%d" },
    { NULL, NULL },
};

/* Postscript to output to invoke the banner */
static char	*bannerPage = "\n%%Page: 1 1\n(%s)(%s@%s)(%s)(%s)Banner\n";
static char	*bannerTrailer = "\n%%Trailer\n";


/*
 * Static routines.
 */

/**********************************************************************
 * Routine:	_copyData() - Copy the data for future use.
 *
 * Function:	This routine copies the data with vm_copy if it was
 *		was not originally inline data, and with malloc/bcopy
 *		if it was.
 *
 * Args:	data:		The data to be copied.
 *		size:		The size of the data.
 *		wasinline:	Whether the data was in line.
 *
 * Returns:	pointer to new data.
 **********************************************************************/
static char *
_copyData(char *data, int size, BOOL wasinline)
{
    char		*newData;
    vm_offset_t		page_start;
    vm_size_t		page_size;
    int			offset;
    kern_return_t	ret_code;

    if (wasinline) {
	offset = 0;
	if ((newData = (char *)malloc(size)) == NULL) {
	    LogError([NXStringManager stringFor:"LocalJob_mallocfail"]);
	    [NXDaemon panic];
	}
	bcopy(data, newData, size);
    } else {
	page_start = trunc_page(data);
	offset = (vm_offset_t) data - page_start;
	page_size = round_page(offset + size);
	if ((ret_code = vm_allocate(task_self(), (vm_address_t *)&newData,
			(vm_size_t)page_size, TRUE)) != KERN_SUCCESS) {
	    LogError([NXStringManager stringFor:"LocalJob_vmallocfail"],
		     ret_code);
	    [NXDaemon panic];
	}
	if ((ret_code = vm_copy(task_self(), (vm_address_t)page_start,
				(vm_size_t) page_size,
				(vm_address_t)newData)) != KERN_SUCCESS) {
	    LogError([NXStringManager stringFor:"LocalJob_vmcopyfail"],
		     ret_code);
	    if ((ret_code = vm_deallocate(task_self(), (vm_address_t)newData,
				 (vm_size_t)page_size)) != KERN_SUCCESS) {
		/* We are really hosed */
		LogError([NXStringManager stringFor:"LocalJob_vmdeallocfail"],
			 ret_code);
	    }
	    [NXDaemon panic];
	}
    }

    return (newData + offset);
}

/**********************************************************************
 * Routine:	_pageqEnqueue() - Put a page on the page queue.
 *
 * Function:	This routine adds a page of information to the queue
 *		of pages that need to get printed.  It keeps track of
 *		how many copies remain to be printed and how the
 *		data was allocated.
 *
 * Args:	self:		The id of the job.
 *		data:		The data to be queued.
 *		size:		Its size.
 *		wasinline:	Whether malloc'd or vm_allocated
 *		copies:		The number of copies to print.
 **********************************************************************/
static void
_pageqEnqueue(LocalJob *self, char *data, int size,
	     BOOL wasinline, int copies)
{
    pageq_entry_t	*pq;

    /* Create an entry for this page */
    pq = (pageq_entry_t *) malloc(sizeof(*pq));
    pq->pq_next = NULL;
    pq->pq_prev = self->pageq_head;
    pq->pq_data = data;
    pq->pq_size = size;
    pq->pq_mallocd = wasinline;
    pq->pq_copies = copies;

    /* Attach it to the queue */
    if (self->pageq_head) {
	self->pageq_head->pq_next = pq;
    } else {
	self->pageq_tail = pq;
    }
    self->pageq_head = pq;
}

/**********************************************************************
 * Routine:	_pageqDealloc()
 *
 * Function:	Deallocate a pageq entry and the memory it points to.
 **********************************************************************/
static void
_pageqDealloc(pageq_entry_t *pq)
{
    kern_return_t	ret_code;

    if (pq->pq_mallocd) {
	free(pq->pq_data);
    } else {
	if ((ret_code = vm_deallocate(task_self(),
				      (vm_offset_t)pq->pq_data,
				      (vm_size_t)pq->pq_size)) !=
	    KERN_SUCCESS) {
	    LogError([NXStringManager stringFor:"LocalJob_vmdeallocfail"],
		     ret_code);
	    [NXDaemon panic];
	}
    }
    free(pq);
}


@implementation LocalJob : PrintJob

/* Factory methods */

+ newPrinter:(id)Spooler user:(char *)User host:(char *)Host
  copies:(int)Copies remotePort:(port_t)RemotePort fromLpd:(BOOL)FromLpd
{
    id		tmpPrinter;
    const char	*oldUser;
    const char	*defaultVal;

    /* Initialize the strings */
    if (!_initdone) {
	[NXStringManager loadFromArray:_strings];
	_initdone = YES;
    }

    /* First lock the spool directory */
    if (!FromLpd && ![Spooler lockForPrinting]) {
	return (nil);
    }

    /* Connect with the local printer */
    if ((tmpPrinter = [NXDaemon localPrinter]) == nil) {
	if (!FromLpd) {
	    [Spooler unlockAndLog:nil pages:0];
	}
	return (nil);
    }

    if ((self = [super newPrinter:Spooler user:User host:Host copies:Copies
	  remotePort:RemotePort fromLpd:FromLpd]) == nil) {
	if (!FromLpd) {
	    [Spooler unlockAndLog:nil pages:0];
	}
	return (nil);
    }

    /* Get the alert style that this user wants to use */
    oldUser = NXSetDefaultsUser(User);
    if (defaultVal = NXReadDefault("System", "SystemAlert")) {
	if (!strcmp(defaultVal, "Panel")) {
	    alertStyle = Panel;
	} else if (!strcmp(defaultVal, "Voice")) {
	    alertStyle = Voice;
	} else {
	    alertStyle = Both;
	}
    } else {
	alertStyle = Both;
    }
    (void) NXSetDefaultsUser(oldUser);

    /* Initialize instance variables */
    localPrinter = tmpPrinter;
    header = NULL;
    trailer = NULL;
    pageq_head = NULL;
    pageq_tail = NULL;
    trailerReceived = 0;
    doing_beforebanner = 0;
    doing_mainjob = 0;

    /* Register ourself as the current job */
    NX_DURING {
	[localPrinter setCurrentJob:self];
    } NX_HANDLER {
	if (NXLocalHandler.code == NPDpsdevice) {
	    flushing = 1;
	} else {
	    NX_RERAISE();
	}
    } NX_ENDHANDLER;

    return (self);
}


/* Instance methods */

- donePrinting
{
    pageq_entry_t	*pq;
    pageq_entry_t	*npq;

    /* If we have more to submit to the device, reset it */
    if (doing_beforebanner ||
	(doing_mainjob && spooler->bannerAfter)) {
	NX_DURING {
	    /* Re-register ourself as the current job */
	    [localPrinter setCurrentJob:nil];
	    [localPrinter setCurrentJob:self];
	} NX_HANDLER {
	    if (NXLocalHandler.code == NPDpsdevice) {
		if (trailerReceived) {
		    return([self free]);
		} else {
		    flushing = 1;
		}
	    } else {
		NX_RERAISE();
	    }
	} NX_ENDHANDLER;
    
	if (doing_beforebanner) {
	    doing_beforebanner = 0;
	    doing_mainjob = 1;
	    
	    /* Submit the header */
	    [localPrinter print:header->pq_data size:header->pq_size
	   mallocd:header->pq_mallocd deallocate:YES type:header_page];
	    free(header);
	    header = NULL;
	    
	    /*
	     * If we've gotten the trailer, process it, otherwise if pages
	     * are in the correct order, submit the first copy of the ones
	     * that we have.
	     */
	    if (trailerReceived) {
		[self _trueProcessTrailer:trailer->pq_data size:trailer->pq_size
	       wasInline:trailer->pq_mallocd];
		free(trailer);
		trailer = NULL;
	    } else if (docinfo->version == PS_NonConforming ||
		       (!docinfo->pageOrderAtEnd && docinfo->pageOrder != 1)) {
		pq = pageq_tail;
		while (pq) {
		    npq = pq->pq_next;
		    pq->pq_copies--;
		    
		    [localPrinter print:pq->pq_data size:pq->pq_size
		   mallocd:pq->pq_mallocd deallocate:(!pq->pq_copies)
		   type:body_page];
		    
		    if (!pq->pq_copies) {
			if (npq) {
			    npq->pq_prev = NULL;
			}
			pageq_tail = npq;
			free(pq);
		    }
		    pq = npq;
		}
	    }
	} else {
	    doing_mainjob = 0;

	    [self bannerFromFile:spooler->bannerAfter];
	}	
	return (self);
    }

    /* Nothing left to print, we're really done */
    return ([self free]);
}

- errorPrinting
{

    if (trailerReceived) {
	return ([self free]);
    } else {
	[self setFlushing:1];
    }
}

- processHeader:(char *)data size:(unsigned int)size wasInline:(BOOL)wasInline
{
    char		*newData;
    pageq_entry_t	*pq;

    /* Our header has been parsed, set up the fake "cf" file */
    if (!fromLpd) {
	NX_DURING {
	    [spooler fakeQueueEntry:self];
	} NX_HANDLER {
	    if (NXLocalHandler.code != NPDspooling) {
		NX_RERAISE();
	    }
	    /* An error, oh well. */
	    ;
	} NX_ENDHANDLER;
    }

    /* Copy the data */
    newData = _copyData(data, size, wasInline);

    /*
     * If we are supposed to start with a banner, save the header
     * away and submit the banner instead.
     */
    if (spooler->bannerBefore) {
	header = (pageq_entry_t *) malloc(sizeof(*pq));
	header->pq_data = newData;
	header->pq_size = size;
	header->pq_mallocd = wasInline;

	doing_beforebanner = 1;
	[self bannerFromFile:spooler->bannerBefore];
	return(self);
    }
	
    /* Start the main job */
    doing_mainjob = 1;
    [localPrinter print:newData size:size mallocd:wasInline deallocate:YES
     type:header_page];

    return (self);
}

- processPage:(char *)data size:(unsigned int)size wasInline:(BOOL)wasInline
{
    char		*newData;
    pageq_entry_t	*pq;		/* Page queue entry */

    /* Copy the data */
    newData = _copyData(data, size, wasInline);

    /*
     * If we are printing the main job and we can send a page right to
     * the printer do so, otherwise queue it up for later.
     */
    if (doing_mainjob &&
	(docinfo->version == PS_NonConforming ||
	 (!docinfo->pageOrderAtEnd && docinfo->pageOrder != 1))) {
	[localPrinter print:newData size:size mallocd:wasInline
         deallocate:(copies == 1) type:body_page];

	if (copies > 1) {
	    _pageqEnqueue(self, newData, size, wasInline, copies - 1);
	}
    } else {
	_pageqEnqueue(self, newData, size, wasInline, copies);
    }

    return (self);
}

- processTrailer:(char *)data size:(unsigned int)size wasInline:(BOOL)wasInline
{
    char		*newData;

    /* Abort everything if we are flushing */
    if (flushing) {
	return ([self free]);
    }

    /* Copy the data */
    newData = _copyData(data, size, wasInline);

    /* If we're not doing the main job, tuck the trailer away for later */
    if (doing_mainjob) {
	[self _trueProcessTrailer:newData size:size wasInline:wasInline];
    } else {
	trailer = (pageq_entry_t *) malloc(sizeof(*trailer));
	trailer->pq_data = newData;
	trailer->pq_size = size;
	trailer->pq_mallocd = wasInline;
    }
    trailerReceived = 1;

    return (self);
}	

- _trueProcessTrailer:(char *)data size:(unsigned int)size
  wasInline:(BOOL)wasInline
{
    pageq_entry_t	*newQHead = NULL;
    pageq_entry_t	*newQTail;
    pageq_entry_t	*pq;
    pageq_entry_t	*npq;
    
    
    /* If the pages came in ascending order, reorder the queue */
    if (docinfo->version != PS_NonConforming &&
	docinfo->pageOrder == 1 && pageq_head) {
	pq = pageq_head;
	while (pq) {
	    npq = pq->pq_prev;
	    pq->pq_next = NULL;
	    pq->pq_prev = newQHead;
	    if (newQHead) {
		newQHead->pq_next = pq;
	    } else {
		newQTail = pq;
	    }
	    newQHead = pq;
	    pq = npq;
	}
	pageq_head = newQHead;
	pageq_tail = newQTail;
    }
    
    /* Reordering done, submit the pages for printing */
    while (pageq_tail) {
	pq = pageq_tail;
	while (pq) {
	    npq = pq->pq_next;
	    pq->pq_copies--;
	    
	    [localPrinter print:pq->pq_data size:pq->pq_size
	   mallocd:pq->pq_mallocd deallocate:(!pq->pq_copies)
	   type:body_page];
	    
	    /* No need to deallocate data here, the device will free it */
	    if (!pq->pq_copies) {
		if (npq) {
		    npq->pq_prev = NULL;
		}
		pageq_tail = npq;
		free(pq);
	    }
	    pq = npq;
	}
    }
    
    /* Now submit the trailer */
    [localPrinter print:data size:size mallocd:wasInline deallocate:YES
     type:trailer_page];
    
    return (self);
}


- handleAlert:(char *)alertStr;
{
    if (alertStyle == Panel || alertStyle == Both) {
	if (strcmp(host, NXHostName)) {
	    NPD1_RemoteVisualMessage(host, alertStr, "OK", NULL);
	}
	NPD1_VisualMessage(alertStr, "OK", NULL);
    }

    return (self);
}

- handleNpAlert:(int) errorCode
{
    struct errorVoiceInfo	*evp;

    if (alertStyle == Panel || alertStyle == Both) {
	if (strcmp(host, NXHostName)) {
	    NPD1_RemoteVisualCode(host, errorCode);
	}
	NPD1_VisualCode(errorCode);
    }

    /* See if we have a voice file to spew forth */
    if (!(errorCode & NPD_1_OS_ERROR) &&
	(alertStyle == Voice || alertStyle == Both)) {
	for (evp = errorVoices; evp->error; evp++) {
	    if (errorCode & evp->error) {
		break;
	    }
	}
	if (evp->sound) {
	    if (strcmp(host, NXHostName)) {
		NPD1_RemoteAudioAlert(host, evp->sound);
	    }
	    NPD1_AudioAlert(evp->sound);
	}
    }

    return (self);
} 

- bannerFromFile:(const char *)fileName
{
    int			fd;		/* File descriptor */
    struct stat		lstat;
    kern_return_t	ret_code;
    pagebuf_t		pb;
    char		*addr;
    int			buflen;
    char		*buf;
    char		*timep;
    time_t		tvec;
    const char		**namep;
    const char		*jobTitle;

    /* Open the file */
    if ((fd = atom_open(fileName, O_RDONLY, 0)) < 0) {
	LogError([NXStringManager stringFor:"LocaJob_nobanner"],
		 fileName);
	return (NULL);
    }

    /* Get its length */
    if (fstat(fd, &lstat)) {
	LogError([NXStringManager stringFor:"LocalJob_nostat"],
		 fileName);
	(void) close(fd);
	return (NULL);
    }

    /* Map the file into VM */
    if ((ret_code = map_fd(fd, 0, (vm_offset_t *)&addr, TRUE, lstat.st_size))
	!= KERN_SUCCESS) {
	LogError([NXStringManager stringFor:"LocalJob_nomap"], fileName,
		 ret_code);
	(void) close(fd);
	return (NULL);
    }
    (void) close(fd);


    /* Submit it as the banner header */
    [localPrinter print:addr size:lstat.st_size mallocd:NO deallocate:YES
     type:header_page];

    /* Find the best looking name for the printer (the last in the list) */
    for (namep = spooler->names; *(namep + 1); namep++)
	;

    /* Get a string version of the current time */
    time(&tvec);
    timep = ctime(&tvec);

    /* Get a reasonable job title */
    if (!(jobTitle = docinfo->title)) {
	jobTitle = "";
    }

    /* Build the banner page */
    buflen = strlen(bannerPage) + strlen(user) + strlen(timep) +
	strlen(*namep) + strlen(jobTitle) + strlen(host);
    buf = (char *) malloc(buflen + 1);
    sprintf(buf, bannerPage, *namep, user, host, jobTitle, timep);

    /* Send it to the device */
    [localPrinter print:buf size:strlen(buf) mallocd:YES deallocate:YES
     type:body_page];

    /* Send the banner trailer */
    [localPrinter print:bannerTrailer size:strlen(bannerTrailer)
     mallocd:NO deallocate:NO type:trailer_page];

    return (self);
}


- free
{
    pageq_entry_t	*pq;
    pageq_entry_t	*npq;
    int			pagesPrinted;

    pagesPrinted = [localPrinter pagesPrinted];

    NX_DURING {
	[localPrinter setCurrentJob:nil];
    } NX_HANDLER {
	/* We are wrapping up anyway */
	;
    } NX_ENDHANDLER;

    /* Flush headers and trailers if we are holding them */
    if (header) {
	_pageqDealloc(header);
    }
    if (trailer) {
	_pageqDealloc(trailer);
    }

    /* Flush page queue if it still exists */
    while (pq = pageq_tail) {
	npq = pq->pq_next;
	_pageqDealloc(pq);
	pageq_tail = npq;
    }

    /* Unlock the spool directory */
    /* FIXME: We need to get a page count from device and do accounting */
    if (!fromLpd) {
	[spooler unlockAndLog:self pages:pagesPrinted];
    } else {
	[spooler logUsage:self pages:pagesPrinted];
    }

    return ([super free]);
}

@end

