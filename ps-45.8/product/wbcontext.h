/*****************************************************************************

    wbcontext.h
    Typedefs and defines for window bitmap Contexts.
	
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Dave 16Feb89
	
    Modified:
    
	28Mar90 Terry Removed references to layer from WBDeviceInfo
	
******************************************************************************/

#ifndef _WBCONTEXTH_
#define _WBCONTEXTH_

#import "nscontexts.h"
#ifndef POSTSCRIPT
#define POSTSCRIPT
#endif
#import "wbprotocol.h"	/* from the client library */

typedef struct _WBContext
{
    PSSchedulerContext	scheduler;	/* -> scheduler context */
    struct _WBContext	*next;
    unsigned int	deathKnell:1;	/* when I'm about to be killed */
    unsigned int	dirtyShmem:1;	/* client needs to get new shmem */
    unsigned int	filler:30;	/* filler(!) */
    port_t		inPortCache;	/* for initializing */
    int			shmemfd;	/* for shmem */
    char		shmemPath[WBSHPATHLEN]; /* path of the shmem file */
    int			wid;		/* the window id */
} WBContext, *WBContextP;

void	WBGetDeviceInfo( NXWBOpenMsg *, int );
int	WBOpenBitmap( NXWBOpenMsg * );
int	WBGetBitmap( NXWBMessage * );
int	WBMarkBitmap( NXWBMessage * );
int	WBFlushBitmap( NXWBMessage * );
int	WBCloseBitmap( int );

#endif _WBCONTEXTH_



