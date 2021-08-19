/*****************************************************************************

    windowimage.c

    This file contains the implementation of the fast image operator for
    NeXT machines.

    CONFIDENTIAL
    Copyright (c) 1990 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 03Jan90 Leo

    Modified:
	28Feb90 Terry 	Integrated into 2.0 windowserver
	 6Apr90 pgraff	new image description terminology
	13May90 Terry	Better format checking, set source->colorSpace
	11Aug90 Terry	Use proper sourceID and zero out decode, sampleProc,
			and procData source fields to get color working
	14Aug90 Terry	Improved nextimage rowbytes calculation

******************************************************************************/

#import <mach.h>
#import PACKAGE_SPECS
#import CUSTOMOPS
#import POSTSCRIPT
#import EXCEPT
#import FP
#import DEVICE
#import "imagemessage.h"

typedef struct _ImageMessageCell {
	PSContext destContext;
	ImageMessage *m;
	struct _ImageMessageCell *next;
} ImageMessageCell;

ImageMessageCell *imageMessageList;

void ReleaseMsg(msg_header_t *m); /* from the scheduler */

/* From graphics/image.c */
extern integer imageID;

/*****************************************************************************
	FindImageMessage
	looks for a message by the given tag, owned by the current context,
	in the message list.  If it finds such a message, it returns the
	pointer, and, if remove is nonzero, removes the message from the list.
	If it does not find the message, it returns NULL.
*****************************************************************************/

static ImageMessage *FindImageMessage(int tag, int remove)
{
    ImageMessageCell *c,*lastC;
    ImageMessage *retVal;
    
    lastC = NULL;
    for(c = imageMessageList ; c ; c = c->next)
    {
    	if (c->m->imageDataTag == tag && c->destContext == currentPSContext)
	{
	    if (remove)
		if (lastC)
		    lastC->next = c->next;
		else
		    imageMessageList = c->next;
	    retVal = c->m;
	    os_free(c);
	    return(retVal);
	}
	lastC = c;
    }
    /* Not found */
    return(NULL);
}

void ReceiveNextImage(ImageMessage *m)
{
    ImageMessageCell *c;

    /* Error Check received message */
    if ((m->integerParams.msg_type_name != MSG_TYPE_INTEGER_32)||
    	(m->integerParams.msg_type_number != IM_NUMINTPARAMS)||
	(!m->integerParams.msg_type_inline)||
	(m->integerParams.msg_type_longform)||
        (m->floatParams.msg_type_name != MSG_TYPE_REAL)||
	(m->floatParams.msg_type_size != sizeof(float)*8)||
    	(m->floatParams.msg_type_number != IM_NUMFLOATPARAMS)||
	(!m->floatParams.msg_type_inline)||
	(m->floatParams.msg_type_longform)||
	(m->imageDataFormat < IM_MINFORMAT)||
	(m->imageDataFormat > IM_MAXFORMAT)||
	(m->msgHeader.msg_size < (sizeof(ImageMessage) + sizeof(void *)))
	)
    {
	ReleaseMsg((msg_header_t *)m);
	PSRangeCheck(); /* FIX!!! We should examine the message and 
	    deallocate anything we received in it here */
    }
    if (FindImageMessage(m->imageDataTag,0))
    {
    	/* There's already one by this tag!!! */
	ReleaseMsg((msg_header_t *)m);
	PSLimitCheck();
    }
    /* OK, insert it in the imageMessageList */
    c = (ImageMessageCell *)malloc(sizeof(ImageMessageCell));
    c->next = imageMessageList;
    imageMessageList = c;
    c->destContext = currentPSContext;
    c->m = m;
}

void MarkNextImage(int tag)
{
    ImageMessage *m;
    int i,j,k,numChannels;
    unsigned char *mpp;
    msg_type_long_t *mlt;
    DevPrim devPrim,*clip;
    DevImage devImage;
    unsigned char *samples[MAXNUMCHANNELS];
    DevImageSource devImageSource;
    Mtx mtx;
    DevTrap traps[7];
    DevMarkInfo	devMarkInfo;

    /* use our auto pointer array for sample pointers */
    devImageSource.samples = samples;
    
    if (!(m = FindImageMessage(tag,1)))
    	PSInvalidID();
    mpp = (unsigned char *)&(m->imageDataType);
    numChannels = (m->isPlanar) ? m->samplesPerPixel : 1;

    /* Fill in the DevImageSource */
    for(i=0;i<numChannels;i++)
    {
    	mlt = (msg_type_long_t *)mpp;
	if ((mlt->msg_type_long_name != MSG_TYPE_BYTE)||
    	    (!mlt->msg_type_header.msg_type_longform))
	    PSRangeCheck(); /* FIX!!! We should examine the message and 
		deallocate anything we received in it here */
       	mpp += sizeof(msg_type_long_t);
    	if (mlt->msg_type_header.msg_type_inline)
	{
	    if (mpp + mlt->msg_type_long_number >
				(unsigned char *)m + m->msgHeader.msg_size)
		PSRangeCheck(); /* FIX!!! We should examine the message and 
		    deallocate anything we received in it here */
	    devImageSource.samples[i] = mpp;
	    mpp += (mlt->msg_type_long_number+3)&~0x3;
	}
	else
	{
	    if (mpp + sizeof(char *) >
				(unsigned char *)m + m->msgHeader.msg_size)
		PSRangeCheck(); /* FIX!!! We should examine the message and 
		    deallocate anything we received in it here */
	    devImageSource.samples[i] = *(unsigned char **)mpp;
	    mpp += sizeof(char *);
	}
    }
    devImageSource.nComponents = m->hasAlpha ? m->samplesPerPixel-1 : m->samplesPerPixel;
    switch (devImageSource.nComponents) {
	case 1: devImageSource.colorSpace = DEVGRAY_COLOR_SPACE; break;
	case 3: devImageSource.colorSpace = DEVRGB_COLOR_SPACE; break;
	case 4: devImageSource.colorSpace = DEVCMYK_COLOR_SPACE; break;
	default: PSRangeCheck(); break;
    }

    devImageSource.bitspersample = m->bitsPerSample;
    devImageSource.interleaved = !m->isPlanar;
    /* Was m->imageDataType.msg_type_long_number/m->pixelsHigh; */
    k = devImageSource.bitspersample * m->pixelsWide;
    if (devImageSource.interleaved) k *= m->samplesPerPixel;
    devImageSource.wbytes = (k + 7) >> 3;
    devImageSource.height = m->pixelsHigh;
    devImageSource.sourceID = (++imageID == 0 ? ++imageID : imageID);
    devImageSource.decode = 0;
    devImageSource.sampleProc = (PVoidProc)NULL;
    devImageSource.procData = NULL;

    /* Now fill in the DevTrap(s) */
    {
	DevPrim dp;
	
	dp.type = trapType;
	dp.next = NULL;
	dp.items = 0;
	dp.maxItems = 7;
	dp.value.trap = traps;
	PSReduceRect(m->originX, m->originY, m->sizeWidth, m->sizeHeight,
		     NULL, &dp);
	devImage.info.trapcnt = dp.items;
	devImage.info.trap = dp.value.trap;
	devPrim.bounds = dp.bounds;
    }
    {
    	Mtx	mtx1,mtx2;
	
       /* Construct a matrix (mtx2) which maps the user space rectangle:
	*
	*    m->originX, m->originY, m->sizeHeight, m->sizeWidth
	*
	* to the sample space rectangle:
	*
	*    0, 0, m->pixelsWide, m->pixelsHigh
	*
	*	|  pw/w        0	|
	*	|			|
	*	|    0	      ph/h	|
	*	|			|
	*	| -(pw/w)*x -(ph/h)*y	|
	*
	*/
	mtx2.a  =   m->pixelsWide / m->sizeWidth;
	mtx2.b  =   0.0;
	mtx2.c  =   0.0;
	mtx2.d  =   m->pixelsHigh / m->sizeHeight;
	mtx2.tx = - mtx2.a * m->originX; 
	mtx2.ty = - mtx2.d * m->originY;

       /* Construct the device primitive matrix (device -> sample) 
        * by taking the inverse of the CTM (device -> user) and
	* concatenating the above matrix (user -> sample).
	*/
	MtxInvert(PSGetMatrix(NULL),&mtx1);
	MtxCnct(&mtx1,&mtx2,&mtx);
	devImage.info.mtx = &mtx;
    }
    
    /* Now fill in the DevImage */
    devImage.source = &devImageSource;
    devImage.transfer = PSGetTfrFcn();
    devImage.invert = 0;    /*used to worry about NX_MONOTONIC removed(pg)*/
    devImage.imagemask = 0;
    devImage.unused = m->hasAlpha;
    devImage.info.sourceorigin.x = devImage.info.sourceorigin.y = 0;
    devImage.info.sourcebounds.x.l = devImage.info.sourcebounds.y.l = 0;
    devImage.info.sourcebounds.x.g = m->pixelsWide;
    devImage.info.sourcebounds.y.g = m->pixelsHigh;

    /* And last but not least, the DevPrim itself */
    devPrim.type = imageType;
    devPrim.next = NULL;
    devPrim.priv = 0;
    devPrim.items = devPrim.maxItems = 1;
    devPrim.value.image = &devImage;
    PSGetClip(&clip);
    PSGetMarkInfo(NULL,&devMarkInfo);
    {	/* Finally, call the device's mark procedure */
	PDevice device;
	
    	device = PSGetDevice(NULL);
    	(*(device->procs->Mark))(device,&devPrim,clip,&devMarkInfo);
    }
    if (!m->imageDataType.msg_type_header.msg_type_inline)
    	for(i=0;i<numChannels;i++)
	    vm_deallocate(task_self_,
		    (vm_address_t) devPrim.value.image->source->samples[i],
		    (vm_size_t) m->imageDataType.msg_type_long_number);
    /* The ipcstream guy gives us the message permanently, so release it */
    ReleaseMsg((msg_header_t *)m);
}

private procedure PSNextImage()
{
    MarkNextImage(PSPopInteger());
}

private readonly RgOpTable cmdWindowImage = {
    "nextimage", PSNextImage,
    NIL};

public procedure IniWindowImage(int reason)
{
    if (reason==1)
	PSRgstOps(cmdWindowImage);
}





