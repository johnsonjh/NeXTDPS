/*
  printmessage.h
  WindowServer printing protocol - 
  definition of page rendering message sent by machportdevice(s)
  Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/


#import <sys/message.h>

#define NX_PRINTPAGEVERSION 3

typedef struct _NXPrintPageMessage {
	msg_header_t msgHeader;
	msg_type_t integerParams;

	int printPageVersion;		/* version marker of this msg format */
	int jobTag;			/* from JobTag entry in opt arg dict */
	int pageNum;			/* page number (starting at 1) */
	
	int pixelsWide;
	int pixelsHigh;
	int bytesPerRow;		/* size of a scanline in bytes
					   (if planar, size of scanline in
					   one plane) */
	int bitsPerSample;		/* standard tiffisms: bits per sample*/
	int samplesPerPixel;			/* samples per pixel */
	int colorSpace;			/* i.e. tiff photometric interp. */
	int isPlanar;			/* i.e. tiff: PlanarConfig == 2 */

	msg_type_long_t oolImageParam;
	unsigned char *printerData;
} NXPrintPageMessage;

#define NX_PPMNUMINTS 10		/* number of integer params */
#define NX_PRINTPAGEMSGID 0xdeed	/* for message header */



