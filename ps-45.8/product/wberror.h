/*****************************************************************************

    wberror.h
    Typedefs and defines for the windowbitmap errors.
    This file is shared between the client library and the server library.
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
    Dave 22Feb89
    
    Modified:


******************************************************************************/

#ifndef _WBERRORH_
#define _WBERRORH_

#define WBERROR		(NXWindowBitmapError != NULL)

/*
 *  Various errors that can be committed both on the server or the
 *  client side.
 */

#define NXWB_NOERR	0
#define NXWB_ENOWIN	1	/* no such window in the server */
#define NXWB_ENOSYNC	2	/* can't sync the bitmap to the server */
#define NXWB_ENOSTAT	3	/* can't stat the bitmap */
#define NXWB_ENOMEM	4	/* out of memory (?) */
#define NXWB_ENOPORT	5	/* no more ports to allocate */
#define NXWB_ENOSERV	6	/* can't connect to the server */
#define NXWB_ENOSHMEM	7	/* can't get shmem area */
#define NXWB_ELOCAL	8	/* requested window not in local server */
#define NXWB_EBUFFER	9	/* requested window has no backing store */
#define NXWB_EDUP	10	/* can't put >1 WB on a window */

void	NXClearWBError();
void	NXWBError( /* int etype */ );

#endif _WBERRORH_

