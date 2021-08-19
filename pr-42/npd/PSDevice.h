/*
 * PSDevice.h	- Interface file to a PostScript device.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */

#import <objc/Object.h>
#import <dpsclient/dpsclient.h>
#import <windowserver/printmessage.h>
#import "DocInfo.h"
#import "PrintSpooler.h"

/*
 * Type definitions.
 */

/* Type of page in pagebuf */
typedef enum {
    header_page,
    body_page,
    trailer_page,
} pagetype_t;

/* Header information for pages to be printed */
typedef struct pagebuf {
    struct pagebuf	*pb_next;	/* The next buffer in line */
    char		*pb_data;	/* Honest to god data */
    int			pb_size;	/* The size of the data */
    pagetype_t		pb_type;	/* The type of page */
    unsigned int	pb_mallocd : 1;	/* Whether the data was malloced */
    unsigned int	pb_free : 1;	/* Free this when done with it */
} *pagebuf_t;

/* Header to hold NXPrintPageMessages */
typedef struct imageqent {
    struct imageqent	*iq_next;	/* The next buffer in the queue */
    NXPrintPageMessage	*iq_msg;	/* The message with the image */
} *imageqent_t;

typedef float	transform_t[6];
	

@interface PSDevice : Object
/*
 * PSDevice is the class that deals with PostScript devices.  It
 * maintains the connection with the Window Server, sends pages of
 * PostScript down the connection, and handles error recovery.
 */
{
@public
    DPSContext		context;	/* Context with the window server */
    id			currentJob;	/* Our current job */
    PrintSpooler	*spooler;	/* Our PrintSpooler object */
    pagebuf_t		pageq_head;	/* The head of the page queue */
    pagebuf_t		pageq_tail;	/* The tail of the page queue */
    pagebuf_t		header;		/* The header page */
    pagebuf_t		errorPage;	/* Page in process when context lost */
    imageqent_t		imageq_head;	/* The head of the image queue */
    imageqent_t		imageq_tail;	/* The tail of the image queue */
    port_t		devicePort;	/* The port pages will be sent to */
    char		*devicePortName; /* The name of said port */
    DPSTimedEntry	timedEntry;	/* Timed entry for context recover */
    int			maxBitmaps;	/* Max number of bitmaps to queue */
    int			curBitmaps;	/* Current number of bitmaps queued */
    int			pagesPrinted;	/* Number of pages printed */
    unsigned int	processing : 1;	/* We are currently sending to WS */
    unsigned int	printing : 1;	/* Device is currently printing */
    unsigned int	job_done : 1;	/* Ping for trailer received */
    unsigned int	contextlost : 1; /* We have lost our context */
    unsigned int	deviceReady : 1; /* The device is ready to init a context */
}

/* Factory methods */

+ newSpooler:(id) Spooler;
/*
 * Create a new PSDevice associated with "spooler".
 */

/* Instance methods */

- spooler;
/*
 * Returns the id of our PrintSpooler.
 */

- refreshSpooler;
/*
 * Rereads the printer database to make sure the name of this printer
 * hasn't changed.  Should be overidden by the subclass.
 */

- (BOOL)busy;
/*
 * Returns YES if the printer is currently in use and NO otherwise.
 */

- setCurrentJob:(id)job;
/*
 * Sets "job" as the current job.  It must be of the class
 * LocalJob.
 */

- (int) pagesPrinted;
/*
 * Returns the number of pages printed by the current job.
 */

- contextLost;
/*
 * This method is called when the Window Server context is lost.  The
 * device will basically not print anything else until it receives a
 * resetContext method.
 */

- resetContext;
/*
 * This method is called when the window server becomes available after
 * going away.
 */

- pingReceived;
/*
 * This method is called when the window server has finished interpreting
 * a page of data.
 */

- errorReceived:(DPSErrorCode)errorCode :(long unsigned int)arg1
    :(long unsigned int)arg2;
/*
 * An error was received on this context.
 */

- sendPSData:(const char *)data size:(int)size;
/*
 * Send the actual PostScript to the context.
 */

- initContext;
/*
 * Initialize the printer for the document and user.  This basically
 * establishes a device for the PostScript context and initializes the
 * bounding box, user, etc.
 */

- machPortDevice:(int)width :(int)height bbox:(boundbox_t)bbox
  transform:(float *)tfm dict:(const char *)dictInvocation;
/*
 * Calls the machportdevice in the window server to set up a frame buffer
 * for subsequent drawing.  Width and height define the size of the frame
 * buffer in device space.  Bounding box is the bounding box of the
 * imageable area in device space.  transform is the initial transformation
 * matrix.
 */

- print:(char *)data size:(int)size mallocd:(BOOL)mallocd
  deallocate:(BOOL)dealloc type:(pagetype_t)type;
/*
 * Add a page to the list of pages to be printed.  If free is set the
 * data will be freed when finished with depending on mallocd.  If mallocd
 * is YES, the data will be freed with free(), otherwise vm_deallocate will
 * be used.  Type defined whether the is the header, a body page, or the
 * trailer.
 */

- start;
/*
 * This method checks to see if it can send some more data to the
 * window server and if so it does by invoking sendToWS.  This defaults
 * to sending  one pages of data to the window server at a time.
 */

- abort;
/*
 * Abort the currently running print job.
 */

- deviceInitContext;
/*
 * Do any device specific initialization of the Window Server context
 * for the entire document.  This should be overridden by the subclass.
 * If the device expects the same page format (resolution/size/bbox)
 * through the whole document, it should call machPortDevice: to set
 * up the PostScript device.
 */

- deviceInitForPage:(pagebuf_t)pagebuf;
/*
 * Do any device specific initialization of the Window Server context
 * for this page.  This should be overridden by the subclass.  If the device
 * can handle different page formats for each page, it should use this
 * method to call machPortDevice:.
 */

- devicePrintImage:(NXPrintPageMessage *)msg;
/*
 * This is called to actually print a bitmap image on the device.  This
 * should be overridden by the subclass to cause the bits to get imaged
 * on the device.
 */

- (BOOL) deviceBusy;
/*
 * Returns whether the device is in the process of printing an image.
 */

- imageDiscard:(NXPrintPageMessage *)msg;
/*
 * The device subclass calls this to get rid of a page image when it
 * is trying to shut down in the middle of a job.
 */

- imageDone:(NXPrintPageMessage *)msg;
/*
 * The device subclass should send this method to itself when it is done
 * imaging the data.  This will free up the image memory and get the next
 * image from the port queue.
 */
@end
