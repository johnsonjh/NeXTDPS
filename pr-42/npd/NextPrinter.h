/*
 * NextPrinter.h - Interface file to a local NeXT Printer
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */


/*
 * Include files.
 */
#import "PSDevice.h"
#import "DocInfo.h"
#import "mach_ipc.h"
#import <cthreads.h>
#import <sys/message.h>
#import <sys/ioctl.h>
#import <nextdev/npio.h>

/*
 * Type definitions.
 */

/* Message id's for message to/from device thread. */
typedef enum { 
    /* Notification messages to thread */
    ImageReady = 42000,			/* There is a page ready to print */
    JobStarting,			/* There is a current job */
    JobEnding,				/* There is no more current job */
    ShutDown,				/* Program is exiting, shutdown */
    /* Status messages from thread */
    ImageDone,				/* The page printed OK */
    PrinterError,			/* There is a printer error */
    PrinterOK,				/* The printer error has gone away */
    DeviceReady,			/* We are ready to init a context */
} device_msg_id;

@interface NextPrinter : PSDevice
/*
 * PSDevice is the class that deals with PostScript devices.  It
 * maintains the connection with the Window Server, sends pages of
 * PostScript down the connection, and handles error recovery.
 */
{
    cthread_t		deviceThread;
    BOOL		deviceBusy;
    int			oldStatus;
@public
    port_t		threadPort;
    port_t		objectPort;
    int			currentRes;	/* Our current resolution */
    int			manualFeed;	/* Whether to manual feed */
    int			topLines;
    int			leftDots;
    int			printerDev;
    mutex_t		messagelock;
    NXPrintPageMessage	*volatile pagemessage;
    mutex_t		statuslock;
    volatile int	devStatus;
    enum np_papersize	currentCassette;
}

/* Factory methods */
+ new;

/* Instance methods */

- notifyThread:(device_msg_id)notify;
/*
 * Notify the device thread of a certain event.
 */

- newThreadStatus:(device_msg_id)status;
/*
 * This method is called when the device thread sends a status update
 * to the NextPrinter object.  "status" is the status reported by the
 * device thread.
 */

- (int)getStatus;
/*
 * Returns the status of the printer as defined in <nextdev/npio.h>.
 * The return value is the same as np_status.flags with the following
 * error conditions masked off: NPNOTONER and NPPAPERDELIVERY.
 */

@end

