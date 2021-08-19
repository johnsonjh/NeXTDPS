
/*
	imagemessage.h
	Defines the Mach message passed across to the window server to
	do a fast image operation.
	
	Created: Leovitch 03Jan89 

	Modified:

	6Apr90	pgraff	new image description terminology.
	
	*/

#import <sys/message.h>

#define MAXNUMCHANNELS 5


typedef struct _ImageMessage {
	msg_header_t msgHeader;
	msg_type_t integerParams; /* Must be a shortform type describing the 
				following integer data, all inline: */
	int	imageDataTag;	/* Tag for nextimage operator */
	int	imageDataFormat;/* Currently, must always be 1.  In the future,
	 			   other values will indicate more data 
				   following the imageData.  */
	int	pixelsWide;	/* Width of source bitmap */
	int	pixelsHigh;	/* Height of same */
	
	int	samplesPerPixel; /* samples/pixel; also, number of elements of
				    imageData below that are filled in */
	int	bitsPerSample;	/* bits/sample */
	int	isPlanar;	/* planarConfig */
	int 	hasAlpha;	/* alpha included ? */
	int	colorSpace;	/* photo interpret */
	msg_type_t floatParams;	/* Must be a shortform type describing the
		following single-precison floating-point data, all inline: */
	float	originX;	/* X origin of destination rectangle */
	float	originY;	/* Y origin */
	float	sizeWidth;	/* width of destination rect */
	float	sizeHeight;	/* Height */
	msg_type_long_t
		imageDataType; 	/* Must be a longform type describing a
				   correct number of BYTEs.  May be either 
				   inline or out-of-line. */
	/* First actual bits go here */
	/* Followed by 0 to MAXNUMCHANNELS-1 more msg_type_long_t's and their
	associated data */
} ImageMessage;

#define IM_NUMINTPARAMS 9	/* Number of integer values in integerParams */
#define IM_NUMFLOATPARAMS 4	/* Number of float values in floatParams */

#define IM_MINFORMAT 1		/* Minimum defined format value */
#define IM_MAXFORMAT 1		/* Maximum defined format value */




