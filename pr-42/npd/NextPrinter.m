/*
 * NextPrinter.m	- Implementation of NextPrinter class
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */

/*
 * Include files
 */
#import "npd_prot.h"
#import "NextPrinter.h"
#import "NpdDaemon.h"
#import "PrintSpooler.h"
#import "LocalJob.h"
#import "netinfo.h"
#import "mach_ipc.h"
#import "paper.h"
#import "log.h"
#import "atomopen.h"

#import <cthreads.h>
#import <errno.h>
#import <fcntl.h>
#import <libc.h>
#import <math.h>
#import <netinfo/ni.h>
#import <appkit/errors.h>
#import <appkit/PageLayout.h>


/*
 * Constants
 */

/* Path to this printers NetInfo directory */
static char *LOCAL_PATH = "/printers/if=\\/usr\\/lib\\/NextPrinter\\/npcomm";

/* Margin around paper */
static const int PAPERMARGIN=10;	/* 10 points */

/* printer device */
static const char *PRINTERDEV = "/dev/np0";

/* Flags we don't report */
static const unsigned int IGNORE_FLAGS = ~(NPPAPERDELIVERY|NPDATARETRANS|
					   NPCOLD|NPNOTONER|NPFUSERBAD|
					   NPLASERBAD|NPMOTORBAD);
/* Default resolution */
static const int  DEFAULTRES = 400;
static const char *DICT400 = "NeXTLaser-400";
static const char *DICT300 = "NeXTLaser-300";
static int	SELECTRETRY = 10;	/* 10 seconds */
static int	CASSETTERETRY = 3000;	/* 3000 milliseconds */
static int	MAXFEEDWAIT = 180;	/* We'll wait three minutes for a
					 * manual feed, then we'll just print
					 */

static struct string_entry _strings[] = {
    { "NextPrinter_sendstatus",
	  "np device thread could not send status msg, error = %d" },
    { "NextPrinter_reserr", "NextPrinter could not set resolution: %s" },
    { "NextPrinter_feederr", "NextPrinter could not set paper feed: %s" },
    { "NextPrinter_marginerr", "NextPrinter could not set margins: %s" },
    { "NextPrinter_writerr", "NextPrinter write failed: %s" },
    { "NextPrinter_select", "np device select failed: %s" },
    { "NextPrinter_select2", "non-sensical return value from select" },
    { "NextPrinter_getstatfail", "NextPrinter can't get status: %s" },
    { "NextPrinter_nomsgrcv",
	  "np device thread msg_receive failed, code = %d" },
    { "NextPrinter_noopen", "open of %s failed: %m" },
    { "NextPrinter_nofgetfl", "F_GETFL of %s failed: %s" },
    { "NextPrinter_nofsetfl", "F_SETFL of %s failed: %s" },
    { "NextPrinter_closeerr", "error on close of %s: %s" },
    { "NextPrinter_baddevnotify",
	  "device thread received bad notify message, id = %d" },
    { "NextPrinter_noport", "NextPrinter could not create port" },
    { "NextPrinter_threadnotify",
	  "NextPrinter could not notify device thread, error = %d" },
    { "NextPrinter_badstatus",
	  "NextPrinter received bad status from device thread, status = %d" },
    { "NextPrinter_nopapersize",
	  "NextPrinter could not get paper size of %s: %s" },
    { NULL, NULL },
};


/*
 * Static variables.
 */
static BOOL	_strings_loaded = NO;


/*
 * Static routines.
 */

/**********************************************************************
 * Routine:	_npSetResolution()
 *
 * Function:	Set the resolution of the device.
 **********************************************************************/
static int
_npSetResolution(int dev, int dpi)
{
    struct npop	op;
    int		error;

    op.np_op = NPSETRESOLUTION;
    op.np_resolution = (dpi > 300) ? DPI400 : DPI300;
    if (ioctl(dev, NPIOCPOP, &op)) {
	error = cthread_errno();
	LogError([NXStringManager stringFor:"NextPrinter_reserr"],
		 strerror(error));
	return (error);
    }
    return (0);
}

/**********************************************************************
 * Routine:	_npSetManualFeed()
 *
 * Function:	Set the resolution of the device.
 **********************************************************************/
static int
_npSetManualFeed(int dev, int manualFeed)
{
    struct npop	op;
    int		error;

    op.np_op = NPSETMANUALFEED;
    op.np_bool = manualFeed;
    if (ioctl(dev, NPIOCPOP, &op)) {	
	error = cthread_errno();
	LogError([NXStringManager stringFor:"NextPrinter_feederr"],
		 strerror(error));
	return (error);
    }
    return (0);
}

/**********************************************************************
 * Routine:	_npSetMargins()
 *
 * Function:	Set the margins of the printer device.
 **********************************************************************/
static int
_npSetMargins(int dev, int top, int left, int longs_per_scanline,
	      int numlines)
{
    struct npop	op;
    int		error;

    op.np_op = NPSETMARGINS;
    op.np_margins.left = left;
    op.np_margins.top = top;
    op.np_margins.width = longs_per_scanline;
    op.np_margins.height = numlines;
    if (ioctl(dev, NPIOCPOP, &op)) {
	error = cthread_errno();
	LogError([NXStringManager stringFor:"NextPrinter_marginerr"],
		strerror(error));
	return (error);
    }
    return (0);
}

/**********************************************************************
 * Routine:	_npGetStatus()
 *
 * Function:	Get the status of the printer device.
 **********************************************************************/
static int
_npGetStatus(int dev, int *statusp)
{
    struct npop	op;
    int		error;

    op.np_op = NPGETSTATUS;
    if (ioctl(dev, NPIOCPOP, &op)) {
	error = cthread_errno();
	LogError([NXStringManager stringFor:"NextPrinter_getstatfail"],
		 strerror(error));
	return (error);
    }
    *statusp = op.np_status.flags;

    /* Mask flags we don't want to report as errors */
    *statusp &= IGNORE_FLAGS;

    if ((*statusp & NPMANUALFEED) && (!(*statusp & NPNOPAPER))) {
	*statusp &= ~(NPMANUALFEED);
    }

    return (0);
}

/**********************************************************************
 * Routine:	_npGetPaperSize()
 *
 * Function:	Get the current paper size of the printer device.
 **********************************************************************/
static int
_npGetPaperSize(int dev, enum np_papersize *paperp)
{
    struct npop	op;
    int		error;

    op.np_op = NPGETPAPERSIZE;
    if (ioctl(dev, NPIOCPOP, &op)) {
	return (cthread_errno());
    }
    *paperp = op.np_size;
    return (0);
}

/**********************************************************************
 * Routine:	_npDeviceSendStatus()
 *
 * Function:	Send a status message to the NextPrinter object.
 *
 **********************************************************************/
static void
_npDeviceSendStatus(NextPrinter *npObject, device_msg_id status)
{
    msg_header_t	msg;
    msg_return_t	ret_code;

    MSGHEAD_SIMPLE_INIT(msg, sizeof(msg));
    msg.msg_local_port = npObject->threadPort;
    msg.msg_remote_port = npObject->objectPort;
    msg.msg_id = (int) status;

    if ((ret_code = msg_send(&msg, MSG_OPTION_NONE, 0)) != SEND_SUCCESS) {
	LogError([NXStringManager stringFor:"NextPrinter_sendstatus"],
		 ret_code);
	[NXDaemon panic];
    }
}

/**********************************************************************
 * Routine:	_npDeviceSetError()
 *
 * Function:	This routine sets the devStatus instance variable of the
 *		object and notifies the object that it has changed with
 *		the appropriate message.
 **********************************************************************/
static void
_npDeviceSetError(NextPrinter *npObject, int error)
{

    mutex_lock(npObject->statuslock);
    npObject->devStatus = error;
    mutex_unlock(npObject->statuslock);
    if (error) {
	_npDeviceSendStatus(npObject, PrinterError);
    } else {
	_npDeviceSendStatus(npObject, PrinterOK);
    }
}
 
    

/**********************************************************************
 * Routine:	_npDevicePrintImage()
 *
 * Function:	This routine does the actual work of printing
 *		the image on the printer.
 **********************************************************************/
static void
_npDevicePrintImage(NextPrinter *npObject)
{
    NXPrintPageMessage	*pmsg;
    int			error;
    int			width;
    int			height;
    int			dataLen;
    fd_set		writefds;
    fd_set		excfds;
    int			nfds;
    int			status;
    struct timeval	tv;
    int			cc;
    int			feedTime;

    mutex_lock(npObject->messagelock);
    if (!npObject->pagemessage) {
	mutex_unlock(npObject->messagelock);
	return;
    }
    width = npObject->pagemessage->bytesPerRow/4;
    height = npObject->pagemessage->pixelsHigh;
    dataLen = npObject->pagemessage->oolImageParam.msg_type_long_number;

    /* Set up the margins */
    if (error = _npSetMargins(npObject->printerDev, npObject->topLines,
			       npObject->leftDots, width, height)) {
	mutex_unlock(npObject->messagelock);
	goto err_out;
    }

    /* Set the resolution on the device */
    if (error = _npSetResolution(npObject->printerDev, npObject->currentRes)) {
	mutex_unlock(npObject->messagelock);
	goto err_out;
    }

    /* Set the manualfeed on the device */
    if (error = _npSetManualFeed(npObject->printerDev, npObject->manualFeed)) {
	mutex_unlock(npObject->messagelock);
	goto err_out;
    }

    feedTime = 0;
    while (TRUE) {
	/* Try the write.  Note: the message is already locked */

	if (write(npObject->printerDev,
		  (char *)npObject->pagemessage->printerData,
		  dataLen) == dataLen) {
	    /* Success */
	    mutex_unlock(npObject->messagelock);
	    _npDeviceSendStatus(npObject, ImageDone);
	    return;
	}
	error = cthread_errno();
	mutex_unlock(npObject->messagelock);

	if (error != EWOULDBLOCK && error != EDEVERR) {
	    LogError([NXStringManager stringFor:"NextPrinter_writerr"],
		     strerror(error));
	    goto err_out;
	}

	/* Wait for the printer to be ready or for a specific device error */
	FD_ZERO(&writefds);
	FD_ZERO(&excfds);
	FD_SET(npObject->printerDev, &writefds);
	FD_SET(npObject->printerDev, &excfds);
	if ((nfds = select(npObject->printerDev + 1, 0, &writefds, &excfds, 0))
	    < 0) {
	    error = cthread_errno();
	    LogError([NXStringManager stringFor:"NextPrinter_select"],
		     strerror(error));
	    goto err_out;
	}
	if (nfds != 1) {
	    LogError([NXStringManager stringFor:"NextPrinter_select2"]);
	    [NXDaemon panic];
	}

	/* Make sure we still want to print */
	mutex_lock(npObject->messagelock);
	if (!npObject->pagemessage) {
	    mutex_unlock(npObject->messagelock);
	    return;
	}
	mutex_unlock(npObject->messagelock);
	
	if (FD_ISSET(npObject->printerDev, &excfds)) {
	    /*
	     * We have an exception, periodically get the status and
	     * report it.  If the printer becomes ready, continue on.
	     */
	    if (error = _npGetStatus(npObject->printerDev, &status)) {
		goto err_out;
	    }
	    _npDeviceSetError(npObject, status);

	    while (TRUE) {
		/* Select for a write with a timeout */
		tv.tv_sec = SELECTRETRY;
		tv.tv_usec = 0;
		FD_SET(npObject->printerDev, &writefds);
		if ((nfds = select(npObject->printerDev + 1, 0, &writefds,
				   0, &tv)) < 0) {
		    error = cthread_errno();
		    LogError([NXStringManager stringFor:"NextPrinter_select"],
			     strerror(error));
		    goto err_out;
		}

		/* Make sure we still want to print */
		mutex_lock(npObject->messagelock);
		if (!npObject->pagemessage) {
		    mutex_unlock(npObject->messagelock);
		    return;
		}
		/* Note: the message is locked here. */

		if (nfds == 1) {
		    /* We are ready to write! */
		    _npDeviceSetError(npObject, 0);
		    /*
		     * Note: we are leaving the message locked for the
		     * write loop.
		     */
		    break;		    
		}

		feedTime += SELECTRETRY;

		/* Get the status. Note the page message is still locked */
		if (error = _npGetStatus(npObject->printerDev, &status)) {
		    goto err_out;
		}

		/* See if the manual feed has timed out */
		if ((status & (NPMANUALFEED|NPNOPAPER)) &&
		    npObject->manualFeed && (feedTime > MAXFEEDWAIT)) {

		    /*
		     * Yep, turn it off and try the write again.
		     * It is the write system call that actually configures
		     * the hardware for manual feed.
		     */
		    status &= ~(NPMANUALFEED|NPNOPAPER);
		    _npDeviceSetError(npObject, status);

		    npObject->manualFeed = 0;
		    if (error = _npSetManualFeed(npObject->printerDev,
						 npObject->manualFeed)) {
			goto err_out;
		    }
		    break;
		}

		mutex_unlock(npObject->messagelock);
		_npDeviceSetError(npObject, status);

		/* We still have an exception, loop through the select */
	    }
	}
    }

err_out:
    _npDeviceSetError(npObject, NPD_1_OS_ERROR|error);
    return;
}

/**********************************************************************
 * Routine:	_npDeviceOpenPrinter()
 *
 * Function:	Open the printer and set it up for the upcoming job.
 **********************************************************************/
static BOOL
_npDeviceOpenPrinter(NextPrinter *npObject)
{
    int			error;
    int			pflags;

    if ((npObject->printerDev = atom_open(PRINTERDEV, O_WRONLY, 0)) < 0) {
	error = cthread_errno();
	LogError([NXStringManager stringFor:"NextPrinter_noopen"],
		 PRINTERDEV, strerror(error));
	goto errout;
    }

    /* Set it to be non-blocking */
    if ((pflags = fcntl(npObject->printerDev, F_GETFL, 0)) == -1) {
	error = cthread_errno();
	LogError([NXStringManager stringFor:"NextPrinter_nofgetfl"],
		 PRINTERDEV, strerror(error));
	goto errout;
    }

    pflags |= (FNDELAY);

    if (fcntl(npObject->printerDev, F_SETFL, pflags) == -1) {
	error = cthread_errno();
	LogError([NXStringManager stringFor:"NextPrinter_nofsetfl"],
		 PRINTERDEV, strerror(error));
	goto errout;
    }
    return TRUE;

errout:
    _npDeviceSetError(npObject, NPD_1_OS_ERROR|error);
    return FALSE;
}

/**********************************************************************
 * Routine:	_npDeviceReadCassette()
 *
 * Function:	Read the paper size in the cassette.
 **********************************************************************/
typedef enum {
    CassetteRead,		/* There was a cassette */
    CassetteNoRead,		/* There was no cassette */
    CassetteError,		/* Error reading cassette */
    } CassetteReadReturn;
static CassetteReadReturn
_npDeviceReadCassette(NextPrinter *npObject)
{
    int			error;
    int			status;
    enum np_papersize	printerPaper;
    
    if (npObject->printerDev < 0)		/* Somebody aborted the job */
    	return CassetteError;

    if (error = _npGetPaperSize(npObject->printerDev, &printerPaper)) {
	LogError([NXStringManager stringFor:"NextPrinter_nopapersize"],
		    PRINTERDEV, strerror(error));
	goto errout;
    }
    
    /* Get current device status */
    mutex_lock(npObject->statuslock);
    status = npObject->devStatus;
    mutex_unlock(npObject->statuslock);
    
    if (printerPaper == NOCASSETTE) {		/* Cassette is not in printer */
	if (!(status & NPNOPAPER)) {
	    _npDeviceSetError(npObject, status|NPNOPAPER);
	}
	return CassetteNoRead;
    }
    
    /* Cassette is in printer.  Got paper size. */
    if (status & NPNOPAPER) {			/* Clear out paper error */
	_npDeviceSetError(npObject, status&~NPNOPAPER);
    }
    npObject->currentCassette = printerPaper;
    _npDeviceSendStatus(npObject, DeviceReady);
    return CassetteRead;

errout:
    _npDeviceSetError(npObject, NPD_1_OS_ERROR|error);
    return CassetteError;
}    

/**********************************************************************
 * Routine:	_npDeviceThread() - The thread that deals with the printer.
 *
 * Function:	This thread waits for messages from the NextPrinter
 *		object and when it gets them prints a page.  While
 *		it is active, it is responsible for maintaining the
 *		printer status.
 *		Most state is kept in the shared threadInfo struct.
 *		Notification between the thread and object is done
 *		via Mach-IPC.
 **********************************************************************/
void
_npDeviceThread(NextPrinter *npObject)
{
    msg_return_t	ret_code;
    msg_header_t	msg;
    int			error;
    int			cassetteWait = 0;	/* Only time out to poll for cassette */

    /* Loop forever */
    while (TRUE) {

	/* Wait for notification from the object */
	msg.msg_local_port = npObject->threadPort;
	msg.msg_size = sizeof(msg);
	if (cassetteWait == 0)
	    ret_code = msg_receive(&msg, MSG_OPTION_NONE, 0);
	else
	    ret_code = msg_receive(&msg, RCV_TIMEOUT, cassetteWait);
	if (ret_code == RCV_TIMED_OUT) {
	    if (_npDeviceReadCassette(npObject) != CassetteNoRead)
		cassetteWait = 0;
	    continue;
	}
	
	/* Get here if we really got a message (no timeout) */
	if (ret_code != RCV_SUCCESS){
	    LogError([NXStringManager stringFor:"NextPrinter_nomsgrcv"],
		     ret_code);
	    [NXDaemon panic];
	}

	switch (msg.msg_id) {

	  case ImageReady:
	    _npDevicePrintImage(npObject);
	    break;

	  case JobStarting:
	    /* Get the printer */
	    cassetteWait = 0;
	    if (_npDeviceOpenPrinter(npObject))
		if (_npDeviceReadCassette(npObject) == CassetteNoRead)
		    cassetteWait = CASSETTERETRY;
	    break;

	  case JobEnding:
	    cassetteWait = 0;
	    if (npObject->printerDev >= 0) {
		if (close(npObject->printerDev)) {
		    LogError([NXStringManager stringFor:"NextPrinter_closeerr"],
			     PRINTERDEV, strerror(cthread_errno()));
		}
		npObject->printerDev = -1;
		_npDeviceSetError(npObject, 0);		/* No job = no errors (for Inform) */
	    }
	    break;

	  case ShutDown:
	    cthread_exit(0);

	  default:
	    LogError([NXStringManager stringFor:"NextPrinter_baddevnotify"],
		     msg.msg_id);
	    [NXDaemon panic];
	}
    }
}

/**********************************************************************
 * Routine:	_threadStatusHandler()
 *
 * Function:	Handles a status message sent from the device thread.
 **********************************************************************/
static void
_threadStatusHandler(msg_header_t *msg, NextPrinter *npObject)
{

    [npObject newThreadStatus:(device_msg_id)msg->msg_id];
}


/*
 * Method definitions.
 */

@implementation NextPrinter : PSDevice

/*
 * Factory methods
 */
+ new
{
    int		fd;			/* Generic file descriptor */
    id		tmpSpooler;
    char	*printerName;

    /* Load our error strings if needed */
    if (!_strings_loaded) {
	[NXStringManager loadFromArray:_strings];
	_strings_loaded = YES;
    }

    /* Make sure we have a NetInfo entry */
    NX_DURING {
	if (!(printerName = NIDirectoryName(LOCAL_PATH))) {
	    printerName = NINextPrinterCreate();
	}
    } NX_HANDLER {
	if (NXLocalHandler.code != NPDnetinfofailed) {
	    NX_RERAISE();
	}
    } NX_ENDHANDLER;

    /* Open the printer device */
    if ((fd = atom_open(PRINTERDEV, O_RDONLY, 0)) < 0) {
	if (cthread_errno() == ENODEV) {
	    LogDebug("No local printer attached");
	} else {
	    /* Unknown error */
	    LogWarning([NXStringManager stringFor:"NextPrinter_noopen"],
		      PRINTERDEV);
	}

	NX_DURING {
	    NIPrinterDisable(LOCAL_PATH);
	} NX_HANDLER {
	    if (NXLocalHandler.code != NPDnetinfofailed) {
		NX_RERAISE();
	    }
	} NX_ENDHANDLER;
    } else {
	LogDebug("open(%s) succeeded", PRINTERDEV);

	/* Make sure the entry is registered in NetInfo */
	NX_DURING {
	    NIPrinterEnable(LOCAL_PATH);
	} NX_HANDLER {
	    if (NXLocalHandler.code != NPDnetinfofailed) {
		NX_RERAISE();
	    }
	} NX_ENDHANDLER;

	/* Free up the printer for now */
	(void) close(fd);
    }

    /* Get our spooler */
    tmpSpooler = [PrintSpooler newPrinter:printerName];
    free(printerName);

    /* Instantiate */
    self = [super newSpooler:tmpSpooler];

    /* Initialize our state */
    deviceBusy = NO;
    statuslock = mutex_alloc();
    messagelock = mutex_alloc();
   
    pagemessage = NULL;
    if((threadPort = PortCreate(NULL)) == PORT_NULL) {
	LogError([NXStringManager stringFor:"NextPrinter_noport"]);
	return([self free]);
    }
    if((objectPort = PortCreate(NULL)) == PORT_NULL) {
	LogError([NXStringManager stringFor:"NextPrinter_noport"]);
	return([self free]);
    }

    DPSAddPort(objectPort, (DPSPortProc) _threadStatusHandler,
	       sizeof(msg_header_t), (void *)self, 0);

    /* Spawn the device thread */
    deviceThread = cthread_fork((cthread_fn_t)_npDeviceThread, self);

    return (self);
}

- refreshSpooler
{
    char	*printerName;

    if (!(printerName = NIDirectoryName(LOCAL_PATH))) {
	[spooler free];
	printerName = NINextPrinterCreate();
	spooler = [PrintSpooler newPrinter:printerName];
    } else {
	if (![spooler isNamed:printerName]) {
	    [spooler free];
	    spooler = [PrintSpooler newPrinter:printerName];
	} else {
	    [spooler refresh];
	}
    }
    free(printerName);
    return (self);
}

- notifyThread:(device_msg_id)notify
{
    msg_header_t	msg;
    msg_return_t	ret_code;

    MSGHEAD_SIMPLE_INIT(msg, sizeof(msg));
    msg.msg_local_port = objectPort;
    msg.msg_remote_port = threadPort;
    msg.msg_id		= (int) notify;

    if ((ret_code = msg_send(&msg, MSG_OPTION_NONE, 0)) != SEND_SUCCESS) {
	LogError([NXStringManager stringFor:"NextPrinter_threadnotify"],
		 ret_code);
	[NXDaemon panic];
    }

    return (self);
}

- newThreadStatus:(device_msg_id)status
{
    NXPrintPageMessage	*pmsg;
    int			newstatus;

    switch (status) {

      case ImageDone:
	if (deviceBusy) {
	    deviceBusy = NO;
	    mutex_lock(messagelock);
	    pmsg = pagemessage;
	    pagemessage = NULL;
	    mutex_unlock(messagelock);
	    [self imageDone:pmsg];
	}
	break;

      case PrinterError:
	mutex_lock(statuslock);
	newstatus = devStatus;
	mutex_unlock(statuslock);
	if (newstatus != oldStatus) {
	    oldStatus = newstatus;
	    if (newstatus) {
		[currentJob handleNpAlert:newstatus];
	    }
	    /* If there was an OS_ERROR, no recovery, notify job of error */
	    if (newstatus & NPD_1_OS_ERROR) {
		[currentJob errorPrinting];
	    }
	}
	break;

      case PrinterOK:
	/*
	 * FIXME:  This is a hook for status callback.  We want error
	 * messages to go away when the printer is OK.  We don't want
	 * things polling to find this out.
	 */
	oldStatus = 0;
	break;
	
	
      case DeviceReady:
        deviceReady = 1;
	if (!processing  && header)
	    [self start];
	break;

      default:
	LogError([NXStringManager stringFor:"NextPrinter_badstatus"],
		 status);
	[NXDaemon panic];
	break;
    }

    return (self);
}
 


- (int) getStatus
{
    struct npop		op;		/* Printer ioctl structure */
    unsigned int	flags;		/* Status flags */

    /* If device thread isn't active, return everything OK */
    mutex_lock(statuslock);
    flags = devStatus;
    mutex_unlock(statuslock);
    return ((int) flags);
}

- setCurrentJob:(id)job
{
    int			fd;
    NXPrintPageMessage	*pmsg;

    if (job) {
	if (currentJob) {
	    LogError([NXStringManager stringFor:"PSDevice_jobjob"]);
	    [NXDaemon panic];
	}
	oldStatus = 0;
	devStatus = 0;
	[self notifyThread:JobStarting];
    } else {
	if (!currentJob) {
	    return (self);
	}

	/*
	 * Shut things down.  There is a potential that we will block
	 * during the course of a device write.  We can't really free the
	 * message while it is being printed.
	 */
	if (deviceBusy) {
	    mutex_lock(messagelock);
	    pmsg = pagemessage;
	    pagemessage = NULL;
	    mutex_unlock(messagelock);
	    [self imageDiscard:pmsg];
	    deviceBusy = NO;
	}
	[self notifyThread:JobEnding];
    }
	
    [super setCurrentJob:job];

    return (self);
}

- deviceInitContext
{
    boundbox_t		bbox;
    float		tfm[6];
    DocInfo		*docinfo;
    const NXSize	*docPaperSize;
    const NXSize	*printerPaperSize = NULL;
    NXSize		paperSize;
    float		leftMargin;
    int			whitespaceWidth;

    /* Get the document information */
    docinfo = (DocInfo *)[currentJob docinfo];

    /* Find our resolution */
    currentRes = docinfo->resolution;
    if (currentRes != 300 && currentRes != 400) {
	currentRes = DEFAULTRES;
    }

    /* Find our manual feed and paper size */
    manualFeed = docinfo->manualfeed;

    if (!manualFeed) {
	switch (currentCassette) {
	  case NOCASSETTE:
	    printerPaperSize = NULL;		/* Should never get NOCASSETTE */
	    break;
	  case LEGAL:
	    printerPaperSize = NXFindPaperSize("Legal");
	    break;
	  case LETTER:
	    printerPaperSize = NXFindPaperSize("Letter");
	    break;
	  case A4:
	    printerPaperSize = NXFindPaperSize("A4");
	    break;
	  case B5:
	    printerPaperSize = NXFindPaperSize("B5");
	    break;
	}
    }

    /*
     * Figure out desired paper size from comments.  If we can't do
     * that, just use what's in the printer.  If we don't know what's in the
     * printer, use Letter.
     */
    if (!docinfo->paper.p_name ||
	!(docPaperSize = NXFindPaperSize(docinfo->paper.p_name))) {
	docPaperSize = &docinfo->paper.p_size;
    }

    if (!docPaperSize->width || !docPaperSize->height) {
	/* We do not know the paper size from the document */
	if (printerPaperSize == NULL) {
	    printerPaperSize = NXFindPaperSize("Letter");
	}
	docPaperSize = printerPaperSize;
    }
    else if (printerPaperSize == NULL) {
	/* If we know doc size and not printer paper size, use doc size for printer */
	printerPaperSize = docPaperSize;
    }

    /*
     * Figure out the paper size.  This is the intersection of the
     * paper asked for, and the paper in the printer.
     */
    paperSize.width = MIN(docPaperSize->width, printerPaperSize->width);
    paperSize.height = MIN(docPaperSize->height, printerPaperSize->height);

    /*
     * 648 is the nominal printer path width in points.  Attempt to
     * center the paper in the printer path.
     */
    leftMargin = (648.0 - paperSize.width) / 2;

    /* Put a 10-point margin around the paper */
    bbox.llx = bbox.lly = PAPERMARGIN;
    bbox.urx = paperSize.width - PAPERMARGIN;
    bbox.ury = paperSize.height - PAPERMARGIN;

    /*
     * Convert bounding box and margin to pixels.
     */
    bbox.llx = floor((double) bbox.llx * currentRes / 72.0);
    bbox.lly = floor((double) bbox.lly * currentRes / 72.0);
    bbox.urx = ceil((double) bbox.urx * currentRes / 72.0);
    bbox.ury = ceil((double) bbox.ury * currentRes / 72.0);
    leftDots = floor((double)leftMargin * currentRes / 72.0) + bbox.llx;

    /*
     * The hardware only supports a left margin of 511 pixels, we have
     * to do the rest by adding to the left side of the bounding box.
     */
    if (leftDots > 511) {
	whitespaceWidth = leftDots - 511;
	leftDots = 511;
    } else {
	whitespaceWidth = 0;
    }

    /* Calculate the top margin */
    if (currentRes == 400) {
	topLines = 85 + (paperSize.height * currentRes / 72.0) - bbox.ury;
    } else {
	topLines = 60 + (paperSize.height * currentRes / 72.0)- bbox.ury;
    }

    /*
     * Cobble up a transformation matrix that inverts the coordinate
     * system and moves the origin from the upper left corner of the
     * frame to (0,0).
     */
    tfm[0] = currentRes / 72.0;
    tfm[1] = tfm[2] = 0.0;
    tfm[3] = currentRes / -72.0;
    tfm[4] = whitespaceWidth - bbox.llx;
    tfm[5] = bbox.ury;

    /*
     * Convert the bounding box to represent the portion of the frame
     * that should be the imageable area.
     */
    bbox.ury -= bbox.lly;
    bbox.lly = 0;
    bbox.urx += whitespaceWidth - bbox.llx;
    bbox.llx = whitespaceWidth;

    /*
     * Initialize the machportdevice and the topLines, the values for
     * which came from empirical testing.
     */
    if (currentRes == 400) {
	[self machPortDevice:bbox.urx:bbox.ury bbox:bbox
	 transform:tfm dict:DICT400];
    } else {
	[self machPortDevice:bbox.urx:bbox.ury bbox:bbox
	 transform:tfm dict:DICT300];
    }

    return (self);
}

- devicePrintImage:(NXPrintPageMessage *)msg
{

    mutex_lock(messagelock);
    pagemessage = msg;
    mutex_unlock(messagelock);
    deviceBusy = YES;
    [self notifyThread:ImageReady];

}

- (BOOL)deviceBusy
{
    return (deviceBusy);
}
    
- free
{
    NXPrintPageMessage	*pmsg;

    if (deviceBusy) {
	mutex_lock(messagelock);
	pmsg = pagemessage;
	pagemessage = NULL;
	mutex_unlock(messagelock);
	[self imageDiscard:pmsg];
	deviceBusy = NO;
    }
    
    /* Tell the device thread to exit, and wait for it */
    [self notifyThread:ShutDown];
    (void) cthread_join(deviceThread);

    /* Free up our resources */
    mutex_free(statuslock);
    mutex_free(messagelock);

    if (threadPort != PORT_NULL) {
	PortFree(threadPort);
    }
    if (objectPort != PORT_NULL) {
	PortFree(objectPort);
    }

    return ([super free]);
}

@end

