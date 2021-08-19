/*****************************************************************************

    mousekeyboard.h
    interface to the mousekeyboard package
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.
    
    Original version: leo: Fri Jan 22 14:43:26 PST 1988
    
    Modified:
    15May90 Ted  Added function prototypes
    22May90 Trey Prototypes for cursor support functions (all WC* routines)
    20Sep90 Ted  Added Trey's mods for bug 8959.
	
******************************************************************************/

#ifndef	MOUSEKEYBOARD_H
#define	MOUSEKEYBOARD_H

#import <nextdev/evio.h>

/* Info needed per context to do wait cursor */
typedef struct _WCParams {
    int lastEventSent;	 /* The timestamp of the last event sent to this
			    context.  Zero if no event has been sent. */
    int lastEventConsumed;
    			/* The timestamp of the last event processed by
			   this context.  Zero if no event has been 
			   processed.  An event has been processed when
			   all the PostScript sent in response to it has been
			   interpreted. */
    int lastFakeEvent;	/* The timestamp used the last time
			   startwaitcursortimer was called. */
    char waitCursorEnabled;
    			/* Is wait cursor enabled for this context? */
    char ctxtTimedOut;
			/* Has this context timed out? */
} WCParams;

extern EventGlobals *eventGlobals;
struct _layer;	/* Doesn't affect compile, just prevents warnings */

/* common.c */
extern void LLEventPost(int /*what*/, Point /*location*/, NXEventData *);

/* kbdroutines.c */
extern void CurrentMouse(int *, int *);
extern void GetKbdEvents(void);
extern int  InitKbdEvents(void);
extern int  RightStillDown(int);
extern void SetMouse(int, int);
extern int  StillDown(int);
extern void TermKbdEvents(void);

/* routines.c */
extern void ClearMouseRect(void);
extern void HideCursor(void);
extern int  InitMouseEvents(port_t);
extern void ObscureCursor(void);
extern void RevealCursor(void);
extern void SetMouseRect(Bounds *);
extern void SetMouseMoved(int);
extern void SetWinCursor(struct _layer *, int, int, int, int);
extern void ShieldCursor(Bounds *);
extern void ShowCursor(void);
extern void TermMouseEvents(void);
extern int  TestMouseRect(void);
extern void UnShieldCursor(void);
extern void WCSendEvent(WCParams *wcParams, int isActive, int sentTime);
    /* called when we dispatch an event */
extern void WCReceiveEvent(WCParams *wcParams, int isActive, int consumedTime);
    /* called when we receive a message from the client ack'ing an event */
extern void WCSetData(WCParams *oldWCParams, WCParams *newWCParams);
    /* called when we switch active contexts */
extern void WCSetEnabled(WCParams *wcParams, int isActive, int flag);
    /* called to set a context's enabled bit */
extern void SetGlobalWCEnabled(char enabled);
    /* sets and gets the bit that controls whether WC is enabled globally */
extern char CurrentGlobalWCEnabled(void);

#endif MOUSEKEYBOARD_H
