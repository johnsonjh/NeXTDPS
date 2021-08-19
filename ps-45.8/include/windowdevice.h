/*****************************************************************************

    windowdevice.h

    Structures for the window devices
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.
    
    Created 31Jul86 Leo
    
    Modified:
    
    16Sep86 Leo   Added window field here, removed from GState
    01Oct86 Leo   Added creator, id of PSContext creating window
    20Oct86 Leo   Added modes for composite
    25Oct86 Leo   CURWINDOW constant
    22Apr87 Leo   Removed BoundsListEl
    27Apr87 Leo   dict
    18Jun87 Jack  post-cataclysm BINTREE name
    21Sep87 Leo   new fields for gstates, gstacks, etc.
    29Sep87 Leo   Rationalization of data structures
    24May89 Jack  change FDMID to new LOG2 convention
    07Nov89 Terry CustomOps conversion
    04Dec89 Ted   Integratathon!
    13Dec89 Ted   Added NextGSExt structure for gstate extension field
    13Dec89 Ted   Moved general #defines into windowdevice.h
    30Jan90 Terry Made color and monochrome code coexist
    08Feb90 Dave  Added window level to windowdevice struct to support tiers
    01Mar90 Terry Added changedBounds to windowdevice for screenchanged events

******************************************************************************/

#ifndef WINDOWDEVICE_H
#define WINDOWDEVICE_H

#import CUSTOMOPS

/*****************************************************************************
    Notification Rects are rectangles that the client gets notified whenever
    the mouse goes in or out of.
******************************************************************************/

typedef struct _nrect {
    struct _nrect *next;	/* Linked list */
    int id;			/* Notification Rect id */
    int userData;		/* user-supplied data */
    Bounds nRect;		/* Rectangle for notification */
    char state;			/* 1: inside  0: outside */
    char buttons;		/* 0: all the time
			  	   1: while right button is down
				   4: while left button is down
				   5: while either button is down */
    char reserved[2];		/* Do YOU trust the compiler? */
} NRect;

/* The structure for a window */

#define Bit unsigned short
typedef struct _wd {
    Device	fd;		/* MUST BE FIRST: PostScript device struct */
    struct _wd *next;		/* Linked list */
    int		level;		/* the tier for this window */
    short	id;		/* Window id for this puppy */
    Bit	    	psEventProcs:1;	/* Has eventProcs been set? */
    Bit		redrawSet:1;	/* Is my redraw rect (below) set? */
    Bit		changedSet:1;	/* Is my changed rect (below) set? */
    Bit		newDevParams:1;	/* Have my device parameters changed? */
    Bit		exists:1;	/* termwindow has not been called on me */
    Bit		alignMe:11;
    Layer *	layer;		/* The layer for this window */
    int		owner;		/* PSContext id of my owner */
    Card32	eventMask;	/* Which events get posted to this window */
    PPSObject	eventProcs;	/* My PostScript array of event procedures */
    PPSObject	dict;		/* My window dictionary */
    Bounds	redrawBounds;	/* Where I was exposed */
    Bounds	changedBounds;	/* Where I was screen changed */
    NRect *	nRects;		/* Linked list of NotificationRects */
} WindowDevice, *PWindowDevice;
#undef Bit

extern PWindowDevice windowBase; /* Base of list */
extern PWindowDevice Layer2Wd();
extern PWindowDevice ID2Wd();
extern DevProcs *wdProcs;

#if (STAGE == DEVELOP)
extern Layer *Wd2Layer(); /* Get layer pointer for WindowDevice */
#else (STAGE == DEVELOP)
#define Wd2Layer(psw) (((PWindowDevice)(psw))->layer)
#endif (STAGE == DEVELOP)

#define WHILERIGHT 0x01
#define WHILELEFT 0x04
#define WHILEEITHER 0x05

#define INSIDE 1
#define OUTSIDE 0

#define BASEPSWINDOWID	13	/* Id of initial window */
#define WIDTHSANITY	10000	/* Maximum width of window (sanity check) */
#define HEIGHTSANITY    10000	/* Maximum height of window (sanity check) */
#define WINDOWLIMIT	16000	/* Maximum pos. or neg. extent of a window */
#define WIN_MAX	    	256	/* Maximum concurrent windows */
#define NRECT_MAX	2000    /* Maximum concurrent notification rects */
#define CURWINDOW	NULL	/* abbreviation accepted by some routines */

#define CheckWindow() if (PSGetDevice(NULL)->procs != wdProcs) PSInvalidID()
#endif WINDOWDEVICE_H





