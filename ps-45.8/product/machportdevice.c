/*****************************************************************************

    machportdevice.c

    Implementation of the NeXT rendering service.

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.  All Rights Reserved.
	
    Created 9 Mar90 pgraff from printerdevice.c

    Modified:

	27Mar90 Terry  	Integrate with new adobe device package
         3Apr90 pgraff	update to new printmessage.h
	 6Apr90 pgraff	new image description terminology
	30Apr90 Jack	mpdShowPage return(false) (bug#5124)
	29May90 Terry  	Code size reduction from 2312 to 2244 bytes
	15Jun90 Peter	Added PSHandleExecError after PSSetTransfer (bug#6334)
	15Jun90 Peter 	Added RPC protocol.
******************************************************************************/

#define MULTICHROME 1
#import <mach.h>
#import <sys/message.h>
#import <servers/netname.h>
#import PACKAGE_SPECS
#import POSTSCRIPT
#import CUSTOMOPS
#import EXCEPT
#import ENVIRONMENT
#import PSLIB
#import "ipcscheduler.h"
#import "framedev.h"
#import "printmessage.h"


enum NXColorSpaceType {
    NX_ONEISBLACK_COLORSPACE = 0,
    NX_ONEISWHITE_COLORSPACE = 1,
    NX_RGB_COLORSPACE = 2,
    NX_CMYK_COLORSPACE = 5,
};
#if NX_PRINTPAGEVERSION != 3
	Warning, static declaration out of sync with header!
#endif    
private readonly NXPrintPageMessage PPMprototype = {
    /* the leading zero is needed to initialize the bitpad */
    {0,0,sizeof(NXPrintPageMessage),MSG_TYPE_NORMAL,0,0,NX_PRINTPAGEMSGID},
    {MSG_TYPE_INTEGER_32, 32, NX_PPMNUMINTS, 1, 0, 0},
    NX_PRINTPAGEVERSION,
    0,0,0,0,0,0,0,0,0,	/* (NX_PPMNUMINTS - 1) worth of dummy integer data */
    { {0,0,0,0,1,0}, MSG_TYPE_CHAR, 8, 0 /* size placeholder */},
    (unsigned char *) 0
    };

/* helpers for simplifying {get,set}ting various message fields  */
#define PrinterDataBytes 	oolImageParam.msg_type_long_number
#define PrinterDestPort 	msgHeader.msg_remote_port

struct MachPortDevice {
    FmStuff fmStuff;		/* must be first, (we are specializing
				   framedevice) */
    char *PortName;		/* ascii port name for print msgs */
    char *HostName;		/* ascii host name */
    port_t PortId;		/* resolved Port reference */
    port_t replyPort;		/* reply port for syncronous RPC */
    int useRPC;			/* use syncronous rpc protocol for page
				   bitmaps? */
    int dotwidth;		/* width in pixels of framebuffer */
    int dotheight;		/* height in pixels of framebuffer */
    int size;			/* size of framebuffer in bytes */
    int spp, bps;		/* samples per pixel, bits per sample */
    int is_planar, colorspace;	/* planar config., photometric interpretation*/
    int jobtag;			/* tag name for job from opt arg dict */
    int pagenum;		/* pages rendered so far */
    DevHalftone *halftone;	/* default halftone screen */
    int isAwake;		/* am I active ? */
    PSSchedulerContext ownerContext;  
};
    
private	DevProcs *mpdProcs;
extern DevHalftone *defaultHalftone;

static char *SafeMalloc(int size)
{
    char *ptr = os_malloc(size);
    if(!ptr)
	PSLimitCheck();

    return( ptr );
}

#define New(thing) ((thing *) SafeMalloc(sizeof(thing)))
#define CarefulFree(p) if(p) os_free(p)


/* PopMallocString -> pop a string from the operand stack and stash it
   away in malloc'ed memory (returned). The PS* calls will raise typecheck
   errors if needed, and SafeMalloc (above) will raise limitcheck 
*/
static char *PopMallocString()
{
    int n = PSStringLength() + 1;
    char *p = SafeMalloc(n);
    PSPopString(p, n);
    return(p);
}

/* support for squirrelling away halftone screens */

static void CopyThresholds(DevScreen *src, DevScreen *dst)
{
    os_bcopy(src->thresholds, dst->thresholds, dst->width*dst->height);
}

static void MakeDefaultHalftone(struct MachPortDevice *mpd, DevHalftone *h)
{
    mpd->halftone = DevAllocHalftone(h->white->width,
				     h->white->height,
				     h->red->width,
				     h->red->height,
				     h->green->width,
				     h->green->height,
				     h->blue->width,
				     h->blue->height);
    CopyThresholds(h->red, mpd->halftone->red);
    CopyThresholds(h->green, mpd->halftone->green);
    CopyThresholds(h->blue, mpd->halftone->blue);
    CopyThresholds(h->white, mpd->halftone->white);
}

private procedure mpdWakeup(struct MachPortDevice *mpd)
{
    framebase = mpd->fmStuff.base;
    framebytewidth = mpd->fmStuff.bytewidth;
    framelog2BD = mpd->fmStuff.log2BD;
    defaultHalftone = mpd->halftone;
    mpd->isAwake = 1;
    SetPrinterContext( mpd->ownerContext );
}

private procedure
mpdSleep(struct MachPortDevice *mpd)
{
    mpd->isAwake = 0;
}

private procedure mpdGoAway(struct MachPortDevice *mpd)
{
    kern_return_t error;

    if (mpd->fmStuff.base != NULL) 
	vm_deallocate(task_self(),(vm_address_t)mpd->fmStuff.base,
		      mpd->size);
    if ( mpd->useRPC &&
	( error = port_deallocate(task_self(), mpd->replyPort)))
	mach_error("machportdevice: problem freeing reply port",error);
    
		  
    DevFreeHalftone(mpd->halftone);
    os_free(mpd->PortName);
    os_free(mpd->HostName);
    
    /* Inheirit old behavior, do this last because
       the device structure will be freed */
    (fmProcs->GoAway)((FmStuff *) mpd);
} 

private procedure mpdInitPage (struct MachPortDevice *mpd, DevColor c) 
{
    if (mpd->fmStuff.base != NULL)
	vm_deallocate(task_self(),(vm_address_t)mpd->fmStuff.base,
		      (vm_size_t)mpd->size);
    if (vm_allocate(task_self(),(vm_address_t *)&mpd->fmStuff.base,
		    (vm_size_t)mpd->size,TRUE)
	!= KERN_SUCCESS)
	PSLimitCheck();
    
    if (mpd->isAwake) /* re-establish globals */
	(mpd->fmStuff.gen.d.procs->Wakeup)((Device *) mpd);

    /* Unfortunately, if our device is not subtractive,
     * i.e. one is white, then we must initialize all of the pages to
     * 0xff, so much for a cheap zero fill!
     */
    if(mpd->colorspace == NX_ONEISWHITE_COLORSPACE ||
       mpd->colorspace == NX_RGB_COLORSPACE) {
	unsigned long *p = (unsigned long *) mpd->fmStuff.base;
	unsigned long white = 0xffffffff;
	long size = mpd->size >> 2;
	while(--size >= 0) *p++ = white;
    }
}


static void
send_asyncronous(struct MachPortDevice *mpd, NXPrintPageMessage *m)
{
    msg_return_t msg_return;
    if(( msg_return = msg_send(m, SEND_NOTIFY, (msg_timeout_t) 0))
       != SEND_SUCCESS)
	if( msg_return == SEND_WILL_NOTIFY) {
	    /* message will be delivered by kernel when possible
	       and send us notification.  So, we yield control back
	       to the PS scheduler, saying that we are waiting for
	       notification relating to our output port.  When
	       control returns here, the message will have been
	       sucessfully devlivered by the kernel.
	       */
	    PSYield(yield_stdout, mpd->PortId);
	} else {
	    mach_error("machportdevice: ShowPage Message Error",
		       msg_return);
	    PSInvlAccess();
	}
}
static void
send_rpc(struct MachPortDevice *mpd, NXPrintPageMessage *m)
{
    msg_return_t msg_return;
    msg_header_t *reply;
    void *port_or_message;
    /* set up our local port in the message for reply */
    port_or_message = (void *) m->msgHeader.msg_local_port = mpd->replyPort;

    /* now we just send along the message asyncronously */
    send_asyncronous(mpd, m);

    /* yield,  waiting for rpc reply */
    PSYield(yield_other, &port_or_message);
    reply = (msg_header_t *) port_or_message;
    if(reply->msg_id != NX_PRINTPAGEMSGID) {
	ReleaseMsg(reply);
	PSInvlAccess();
    }
    ReleaseMsg(reply);
}


private boolean mpdShowPage(struct MachPortDevice *mpd, boolean clear,
			    int copies, unsigned char *pageHook)
{
    NXPrintPageMessage msg;

    for(; copies>0; copies--) {
	msg = PPMprototype;
	msg.PrinterDestPort = mpd->PortId;
	msg.jobTag = mpd->jobtag;
	msg.pageNum = ++mpd->pagenum;
	msg.pixelsWide = mpd->dotwidth;
	msg.pixelsHigh = mpd->dotheight;
	msg.bytesPerRow = mpd->fmStuff.bytewidth;
	msg.PrinterDataBytes = msg.bytesPerRow*msg.pixelsHigh;
	msg.samplesPerPixel = mpd->spp;
	msg.bitsPerSample = mpd->bps;
	msg.colorSpace = mpd->colorspace;
	msg.isPlanar = mpd->is_planar;
	msg.printerData = (unsigned char *)mpd->fmStuff.base;
	if( mpd->useRPC) 
	    send_rpc(mpd, &msg);
	else
	    send_asyncronous(mpd, &msg);
    }
    return(false);
} 

private procedure mpdWinToDevXlation (PDevice device, DevPoint *translation)
{
    translation->x = 0;
    translation->y = 0;
    /* add this to a buffer coordinate to get a PS device coordinate */
}

/* decode the bounding box from the array on the operand stack.  If
   the array is of length zero we return -1, indicating that the entire
   raster is to be used.  This allows the common case of [0 0 width height]
   to be expressed simply.  If the bbox is succesfully decoded, we return 0.
   If an error occurs, this fuction will generate the appropiate exception.
 */
static int DecodeBbox(DevLBounds *bbox)
{
    PSObject aryobj, *a;

    PSPopTempObject(dpsArrayObj, &aryobj);

    if(aryobj.length == 0)
	return(-1);
    else if(aryobj.length != 4)
	PSRangeCheck();
    a = aryobj.val.arrayval;
    
    if((a[0].type | a[1].type | a[2].type | a[3].type) != intType)
    	PSTypeCheck();
    bbox->x.l = a[0].val.ival;
    bbox->y.l = a[1].val.ival;
    bbox->x.g = a[2].val.ival;
    bbox->y.g = a[3].val.ival;
    return(0);
}    

/* some helpers to extract values from dicts, (use local PSObject tval) */

#define ExtractInt(d,str) (PSDictGetPObj((d),str, dpsIntObj, &tval), \
			   tval.val.ival)
#define ExtractBoolean(d,str) (PSDictGetPObj((d), str, dpsBoolObj, &tval), \
			       tval.val.bval)
#define ExtractBooleanDefault(d,str,deflt) ((PSDictGetTestPObj((d), str, \
			       dpsBoolObj, &tval)) ? tval.val.bval : deflt)
			    
static void DecodePixelDict(struct MachPortDevice *mpd)
{
    PSObject dict, tval;
    DevMarkInfo info;
    int isSubtractive;
    int isPlanar;

    PSPopTempObject(dpsDictObj, &dict);
    /* first the sample definitions */
    mpd->jobtag = 	ExtractInt(&dict, "jobTag");
    mpd->spp = 		ExtractInt(&dict, "samplesPerPixel");
    mpd->bps =		ExtractInt(&dict, "bitsPerSample");
    mpd->colorspace =	ExtractInt(&dict, "colorSpace");
    isPlanar = 		ExtractBoolean(&dict, "isPlanar");
    mpd->useRPC = 	ExtractBooleanDefault(&dict, "useRPC", true);

    /* sanity check the pixel specification */
    if(mpd->spp != 1 ||
       (mpd->bps != 1 && mpd->bps != 2 && mpd->bps != 4 && mpd->bps != 8) ||
       (mpd->bps == 1 && mpd->colorspace != NX_ONEISBLACK_COLORSPACE) ||
       isPlanar) PSRangeCheck();

    /* framedevice cannot directly generate planar data, so for now
       mpd->planar is always NX_MESHED */
    mpd->is_planar = isPlanar;

    /* now for the default halftone screen . . . we merely push the
       DefaultHalftone dict onto the operstack and then call PSSetHalftone
       (after making it public in gray.c, of course!).  Then we use
       PSGetMarkInfo, to get a pointer to the calculated DevScreen which
       we then copy an squirel away.
       */
    PSDictGetPObj(&dict, "defaultHalftone", dpsDictObj, &tval);
    PSPushObject(&tval);
    PSSetHalftone();
    PSGetMarkInfo(0 /* i.e. currentgstate */, &info);
    MakeDefaultHalftone(mpd,info.halftone);

    /* and finally, the initial transfer function */
    PSDictGetPObj(&dict, "initialTransfer", dpsAnyObj, &tval);
    PSPushObject(&tval);
    PSSetTransfer();
    /* since settransfer executes ps code, we must propagate any exceptions
       it may generate
       */
    PSHandleExecError();
}

/*
  <dw> <dh> <devicebboxarray> <defaultxform> <hostname> <portname> <dict> => -
*/
private PDevice PSMachPortDevice() 
{
    DCPixelArgs pixelArgs;
    DevShort width, height, scanlineBytes;
    struct MachPortDevice *mpd = 0;
    kern_return_t error;
    DevLBounds bbox;
    int useDefaultBounds;



    DURING { /* trap exceptions in order to cleanup allocations */
	
	/* allocate our device data structure, these routines RAISE limitchecks
	   if mallocs fail.
	   */
	mpd = New(struct MachPortDevice);
	os_bzero((char *)mpd, sizeof(*mpd));
	
	DecodePixelDict(mpd);

	/* PortName, and HostName are use in exception handling */
	mpd->PortName = PopMallocString();
	mpd->HostName = PopMallocString();

 	/* Default transformation matrix */
 	PSPopPMtx(&mpd->fmStuff.gen.matrix);

	/* get the device boundary */
	useDefaultBounds = DecodeBbox(&bbox);

	/* finally, the bitmap size */
	height = PSPopInteger();
	width = PSPopInteger();

	if(useDefaultBounds) {
	    /* use the whole raster as the bounding box */
	    bbox.x.l = bbox.y.l = 0;
	    bbox.x.g = width;
	    bbox.y.g = height;
	} else {
	    /* sanity check width, height and bbox. width and height must
	       be positive, bbox must be well formed, and within device limits
	       */
	    if(width <= 0 || height <= 0 ||  
	       bbox.x.l > bbox.x.g ||	
	       bbox.y.l > bbox.y.g ||    
	       bbox.x.l < 0 || bbox.x.g > width ||
	       bbox.y.l < 0 || bbox.y.g > height
	       ) {
		PSRangeCheck();
	    }
	}

	/* get the port from the name server */
	if(( error = netname_look_up(name_server_port, mpd->HostName,
				     mpd->PortName, &mpd->PortId))
	   != NETNAME_SUCCESS) {
	    printf("Failure looking up: <%s> <%s>\n", mpd->HostName,
		   mpd->PortName);
	    mach_error("machportdevice: netname_look_up returned ", error);
	    PSRangeCheck();
	}
	if( mpd->useRPC &&
	   (error = port_allocate(task_self(), &mpd->replyPort)))
	    mach_error("machportdevice: problem allocating reply port ",
		       error);
	
    } HANDLER {
	
	if(mpd) {
	    CarefulFree(mpd->HostName);
	    CarefulFree(mpd->PortName);
	    DevFreeHalftone(mpd->halftone);
	    os_free(mpd);
	}
	RERAISE;		/* propogate exception back to caller */
	
    } END_HANDLER;
    
    /* round up to longword */	
    scanlineBytes = ((width*mpd->spp*mpd->bps+31) >> 5) * 4; 
    mpd->size = scanlineBytes*height;
    mpd->dotwidth = width;
    mpd->dotheight = height;
    mpd->fmStuff.gen.bbox = bbox;
    mpd->pagenum = 0;
    mpd->ownerContext = currentPSContext->scheduler;
    SetPrinterContext(mpd->ownerContext);

    pixelArgs.depth 	 = mpd->spp * mpd->bps;
    pixelArgs.colors 	 = 1;
    pixelArgs.firstColor = 0;
    pixelArgs.nReds 	 = 0;
    pixelArgs.nGreens 	 = 0;
    pixelArgs.nBlues 	 = 0;
    pixelArgs.nGrays 	 = 1 << mpd->bps;
    pixelArgs.invert 	 = ((mpd->colorspace == NX_ONEISWHITE_COLORSPACE) ||
			    (mpd->colorspace == NX_RGB_COLORSPACE));
    SetFmDeviceMetrics(&mpd->fmStuff, NULL, scanlineBytes, height, &pixelArgs);
    mpd->fmStuff.gen.d.procs  = mpdProcs;
    mpd->fmStuff.gen.d.maskID = 0;

    /* Set mpd to be the current device and perform an InitPage */
    PSSetDevice(mpd, true);
}

public procedure IniMpdDevImpl() {
    mpdProcs = (DevProcs *)  os_malloc (sizeof (DevProcs));
    /* Start out using the procs for framedevice; override some */
    *mpdProcs = *fmProcs;
    mpdProcs->Wakeup = mpdWakeup;
    mpdProcs->Sleep = mpdSleep;
    mpdProcs->GoAway = mpdGoAway;
    mpdProcs->InitPage = mpdInitPage;
    mpdProcs->ShowPage = mpdShowPage;
    mpdProcs->WinToDevTranslation =  mpdWinToDevXlation; 
#if 0
{
    extern void Mark(PDevice device, DevPrim *graphic, DevPrim *devClip,
		     DevMarkInfo *markInfo);
    mpdProcs->Mark = Mark;
}
#endif
} 

public procedure MpdDevInit(int reason)
{
    if(reason == 1) 
	PSRegister("machportdevice",PSMachPortDevice);
} 



