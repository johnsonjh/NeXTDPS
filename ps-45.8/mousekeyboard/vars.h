/******************************************************************************
	vars.h
	This file defines the memory locations shared between the mainline and
	interrupt-driven portions of the mouse driver code
		
	CONFIDENTIAL
	Copyright (c) 1988 NeXT, Inc. as an unpublished work.
	All Rights Reserved.

	Created by Leo Hourvitz 22Jun87
	
	Modified:
	23Dec87 Leo  Independent of all include files except event*.h
	09Jun88 Leo  TICKSPERSECOND
	03Jan89 Jack remove EvVars (it's now in evio.h)
	13Jan89 Ted  ANSI.
******************************************************************************/

#import PACKAGE_SPECS
#import EVENT
#import MOUSEKEYBOARD
#import "nextdev/evio.h"


/* Implementation Constants */

#define CURSORWIDTH	16	/* pixels */
#define CURSORHEIGHT	16	/* pixels */
#define MOVEDEVENTMASK (NX_MOUSEMOVEDMASK | NX_LMOUSEDRAGGEDMASK | NX_RMOUSEDRAGGEDMASK )

/* ev driver private variables */

EvVars *evp;			/* Pointer to all my shared variables */
EventGlobals *eventGlobals;	/* Pointer to globals */
