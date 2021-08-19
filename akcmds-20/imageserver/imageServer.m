#import <mach.h>
#import <sys/message.h>
#import <netname_defs.h>
#import "nxjpeg.h"
#import "imagemessage.h"

#import <dpsclient/dpsclient.h>
#import <defaults.h>
#import <appkit/graphics.h>
#import <objc/objc.h>

static port_t   serverPort;
static BOOL     verbose = NO;

extern void NXJpegCompress(int w, int h, int spp, int factor, int planar, unsigned char **tiffdata, unsigned char *jpegdata, int *sizep);

extern void NXJpegDecompress(int *wp, int *hp, int *sppp, int planar, unsigned char **tiffdata, unsigned char *jpegdata, int size);


static void
Err(char *str, int val)
{
    printf("Error %d: %s\n", val, str);
    exit(-1);
}


/*
 * Return the total number of bytes needed to read the whole image. 
 */
int
bitmapSize(int w, int h, int spp, int bps, BOOL planar)
{
    return (((bps * w) * (planar ? 1 : spp)) + 7) / 8 * h * (planar ? spp : 1);
}

static NXImageServerMessage msg;

void Location(what, offset)
int what,offset;
{
    switch(what) {
   	case  QUANT0:
	    msg.JPEGQMatrixData[0] = offset;
	    break;
	case  QUANT1:
	    msg.JPEGQMatrixData[1] = offset;
	    msg.JPEGQMatrixData[2] = offset;
	    break;
	case  HUFFDC0:
	    msg.JPEGDCTables[0] = offset;
	    break;
	case  HUFFAC0:
	    msg.JPEGACTables[0] = offset;
	    break;
	case  HUFFDC1:
	    msg.JPEGDCTables[1] = offset;
	    msg.JPEGDCTables[2] = offset;
	    break;
	case  HUFFAC1:
	    msg.JPEGACTables[1] = offset;
	    msg.JPEGACTables[2] = offset;
	    break;
	case  DATA:
	    msg.dataOffset = offset;
	    break;
    }
}

static void
server_loop()
{
    kern_return_t   ret;
    int             cnt;
    unsigned char   *thedata[4];
    int	  i;
    
    while (1) {
	msg.header.msg_local_port = serverPort;
	msg.header.msg_size = sizeof(NXImageServerMessage);
	ret = msg_receive(&(msg.header), MSG_OPTION_NONE, 0);
	if (ret != RCV_SUCCESS) {
	    mach_error("Image Server msg_receive error ", ret);
	} else {
	    if (verbose) {
		printf("ImageServer got %d\n", msg.header.msg_id);

		printf("w %d h %d spp %d planar %d compression %f\n",
		       msg.pixelsWide, msg.pixelsHigh,
		       msg.samplesPerPixel, msg.isPlanar, msg.compressionFactor);
	    }
	    switch (msg.header.msg_id) {

		case NX_ImageJPEGCompress:{

		    if (msg.samplesPerPixel > 4 ||
			msg.bitsPerSample != 8 ||
			(msg.colorSpace == NX_CMYKColorSpace) ||
			msg.numChannels > 4) {

			if (verbose) {
			    printf("Image Server: Couldn't compress.\n");
			}
			
			/*
			 * Make sure the data is in compressable state; if
			 * not, immediately return error. 
			 */

			for (cnt = 0; cnt < msg.numChannels; cnt++) {
			    vm_deallocate(task_self(),
				 (vm_address_t) msg.channels[cnt].imageData,
				(vm_size_t) msg.channels[cnt].dataParams.msg_type_long_number);
			}

			/*
			 * This indicates no data is being returned... 
			 */

			msg.header.msg_size = sizeof(NXImageServerMessage) - 
			NX_ISM_MAXCHANNELS * (sizeof(msg_type_long_t) + sizeof(unsigned char *));
			msg.numChannels = 0;

		    } 
		    else {

			int	size = bitmapSize(msg.pixelsWide, msg.pixelsHigh,
				     msg.samplesPerPixel, msg.bitsPerSample, msg.isPlanar);
						   
			  /*
			   * We allocate room for the compressed data.
			   * We don't know how big, we know it must be smaller than original size.
			   */
			unsigned char  *compressed =
				(unsigned char *)valloc(size);	
			int             compressedSize;
			int             factor = (int)((msg.compressionFactor < 1.0) ?
					       1.0 : msg.compressionFactor);
			if (verbose) {
			    printf("Image Server: Compress request for %dx%d"
				   "image, compression %d.\n",
				   msg.pixelsWide, msg.pixelsHigh, factor);
			}
			
			for(i = 0; i < msg.numChannels; i++) {
			    thedata[i] = msg.channels[i].imageData;
			}
			
			NXJpegCompress(
			msg.pixelsWide, msg.pixelsHigh, msg.samplesPerPixel,
				       factor, msg.isPlanar, thedata, compressed, &compressedSize);

			if (verbose) {
			    printf("Image Server: Compressed %d to %d bytes.\n",
				   msg.imageDataLen, compressedSize + 1);
			}
			
			for (cnt = 0; cnt < msg.numChannels; cnt++) {
			    vm_deallocate(task_self(),
				 (vm_address_t) msg.channels[cnt].imageData,
				(vm_size_t) msg.channels[cnt].dataParams.msg_type_long_number);
			}

			msg.header.msg_size = sizeof(NXImageServerMessage) - (NX_ISM_MAXCHANNELS - 1) * (sizeof(msg_type_long_t) + sizeof(unsigned char *));
			msg.numChannels = 1;
			msg.channels[0].imageData = compressed;
			msg.channels[0].dataParams.msg_type_long_number = compressedSize;
			msg.imageDataLen = compressedSize;
			
			msg.JPEGQFactor = factor;
			msg.JPEGQMatrixPrecision = 0; /* 8 bit */
			msg.JPEGMode = 0;	/* baseline */
			
			   /* YUV 4:2:2 */
			msg.NewComponentSubsample[0] = 1; /* Y */
			msg.NewComponentSubsample[1] = 1;
			msg.NewComponentSubsample[2] = 2; /* U */
			msg.NewComponentSubsample[3] = 1;
			msg.NewComponentSubsample[4] = 2; /* V */
			msg.NewComponentSubsample[5] = 1;
		    }
		}

		break;

	    case NX_ImageJPEGDecompress:{

		    int             w, h, spp;
		    int             size = bitmapSize(msg.pixelsWide, msg.pixelsHigh,
				     msg.samplesPerPixel, msg.bitsPerSample, msg.isPlanar);
		    unsigned char  *decompressed[4];

		    if (verbose) {
			printf("Image Server: Decompress request for %dx%d"
			       "image  planar %d\n",
			       msg.pixelsWide,msg.pixelsHigh,msg.isPlanar);
		    }
		    /*
		     * This puppy allocates the right size decompressed 
		     */
		    NXJpegDecompress(&w, &h, &spp, msg.isPlanar,
			    thedata, (unsigned char *)msg.channels[0].imageData, msg.imageDataLen);

		    if (verbose) {
			printf("Image Server: Decompressed %d to %d bytes.\n",
			       msg.imageDataLen, size);
		    }
		    vm_deallocate(task_self(),
				  (vm_address_t) msg.channels[0].imageData,
				  (vm_size_t) msg.imageDataLen);

		    msg.header.msg_size = sizeof(NXImageServerMessage) - (NX_ISM_MAXCHANNELS - 1) * (sizeof(msg_type_long_t) + sizeof(unsigned char *));
		    
		    /*
		     * Note that the data is in a single chunk even if
		     * planar.
		     */
		    msg.numChannels = 1;
		    msg.channels[0].imageData = thedata[0];
		    msg.imageDataLen = size;
		    msg.channels[0].dataParams.msg_type_long_number = size;
		}
		break;

	    default:
		if(verbose)
		    printf("Unknown message...\n");
		for (cnt = 0; cnt < msg.numChannels; cnt++) {
		    vm_deallocate(task_self(),
				  (vm_address_t) msg.channels[cnt].imageData,
				  (vm_size_t) msg.channels[cnt].dataParams.msg_type_long_number);
		}

		/*
		 * This indicates no data is being returned... 
		 */

		msg.header.msg_size = sizeof(NXImageServerMessage) - NX_ISM_MAXCHANNELS * (sizeof(msg_type_long_t) + sizeof(unsigned char *));
		msg.numChannels = 0;
		break;
	    }

	    msg.header.msg_local_port = PORT_NULL;
	    if ((ret = msg_send(&(msg.header), MSG_OPTION_NONE, 0)) !=
		KERN_SUCCESS) {
		mach_error("Image Server couldn't reply ", ret);
	    }
	    for(i = 0; i < msg.numChannels; i++) {
	    	if(msg.channels[i].imageData)
		    free(msg.channels[i].imageData);
		msg.channels[i].imageData = NULL;
	    }
	}
    }
}


void
FatalError(int error)
{
    mach_error("Image Server error ", error);
    exit(-1);
}

main()
{
    kern_return_t   ret;

    {
	const char     *verboseStr;
	if (verboseStr = NXGetDefaultValue("imageserver", "Verbose")) {
	    verbose = (verboseStr[0] == 'y' || verboseStr[0] == 'Y') ? YES : NO;
	} else {
	    verbose = NO;
	}
    }

    if ((ret = port_allocate(task_self(), &serverPort)) != KERN_SUCCESS) {
	FatalError(ret);
    }
    if ((ret = netname_check_in(name_server_port, NX_IMAGE_PORT,
				PORT_NULL, serverPort)) != KERN_SUCCESS) {
	(void)port_deallocate(task_self(), serverPort);
	FatalError(ret);
    }
    printf("Image Server started\n");

    server_loop();

}


/*
 * 8/1/90 aozer	Created. 
 *
 * 14 -- 9/7/90 aozer	Switched to new _NXImageServerMessage, allowing for
 * planar data and alpha; also changed protocol for letting app know error
 * conditions, etc. This code needs cleaning up! 
 *
 */
