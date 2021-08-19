/*****************************************************************************

    wbprotocol.h

    Typedefs and defines for the windowbitmap message formats and protocol.
    This file is shared between the client library and the server library.
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.

    Dave 22Feb89
    
    Modified:

******************************************************************************/

#ifndef _WBPROTOCOLH_
#define _WBPROTOCOLH_

#import <sys/message.h>
#import <sys/port.h>
#ifndef POSTSCRIPT
/* this file is being included by a client -- use objective C */
#import <appkit/appkit.h>
#else
/* this file is being included by the window server -- turn off objective C */
typedef float   NXCoord;

typedef struct _NXPoint {	/* point */
    NXCoord         x, y;
} NXPoint;

typedef struct _NXSize {	/* size */
    NXCoord         width, height;
} NXSize;

typedef struct _NXRect {	/* rectangle */
    NXPoint         origin;
    NXSize          size;
} NXRect;
#endif POSTSCRIPT
#import "wberror.h"

#ifndef WBSHPATHLEN
#define WBSHPATHLEN	15
#endif

/* message used to rendez-vous with PS */
typedef struct
{
    msg_header_t	header;
    msg_type_t		type;
    port_t		port;
} NXWBConnectMsg;

/* an OpenMsg contins a path name for shmem */
typedef struct
{
    msg_header_t	header;
    msg_type_t		wid_type;
    int			wid;		/* set on input from client */
    int			bitsPerPixel;	/* set by server */
    int			devStatus;	/* set by server: color,
					   zero = white, etc */
    int			rowBytes;	/* set by server */
    msg_type_t		roi_type;
    NXRect		roi;		/* filled in by server */
    msg_type_t		path_type;
    char		path[WBSHPATHLEN];
} NXWBOpenMsg;

/* the general window bitmap message -- just an roi (bits in shmem) */
typedef struct
{
    msg_header_t	header;
    msg_type_t		wid_type;
    int			wid;		/* the wid for this transaction */
    msg_type_t		roi_type;
    NXRect		roi;
} NXWBMessage;


/*
 *  The following manifest constants are the msg_id's you send to
 *  a window bitmap context.  When you want to establish a new wb
 *  context, you send the NXWB_CONTEXTID to the server with the
 *  client-side port that will receive messages from the server.
 *  To perform other operations on specific bitmaps in the context,
 *  you send down the contextID |'ed with the various commands.
 */
 
#define NXWB_CONTEXTID	0x2000	/* the id for opening a new context */
#define NXWB_OPEN	0x2010	/* open a new bitmap */

/* the following 3 defines are |'ed together to make an id */
#define NXWB_PUT	0x2030	/* | this with data to just send */
#define NXWB_FLUSH	0x0001	/* post a flush graphics on the WID */
#define NXWB_SYNCFLUSH	0x0002	/* synchronous flush */

#define NXWB_GET	0x2040	/* get bitmap from server */
#define NXWB_CLOSE	0x2050	/* free the bitmap */

#define NXWB_FUNCTIONS	(NXWB_PUT | NXWB_GET | NXWB_CLOSE)
			/* the functions without modifiers */
#define NXWB_MODIFIERS	0x0003	/* the modifiers without the functions */

#endif _WBPROTOCOLH_

