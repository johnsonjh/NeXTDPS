/* Required for compatiblity with 1.0 to turn off .const and .cstring */
#pragma CC_NO_MACH_TEXT_SECTIONS

#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
    globals.c

    This file has declarations for all globals.  They are separated out
    to facilitate shared libraries.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#include "dpsclient.h"
#include "defs.h"

/* Global const data (in the text section) */
static const char _dpsclient_constdata_pad1[128] = { 0 }; 

/* Literal const data (in the text section) */
static const char _dpsclient_constdata_pad2[128] = { 0 }; 

/* here are the ones used to implement features common to every interface */
/* Ooops, Adobe took it out of the interface so now its a private global */
int _DPSLastNameIndex = 1;

/* here are the ones used in the NeXT additions */

/* all output contexts we have made */
_DPSList _DPSOutputContexts = {NULL,NULL};

/* all Mach contexts we have made */
_DPSList _DPSMachContexts = {NULL,NULL};

_DPSQueue _DPSEventQ = {NULL, NULL};	/* global event queue */

port_t _DPSNotifyPort = 0;		/* notify port we create */

char _DPSTracking = TRUE;		/* do we coalesce events? */

/* global flag for output tracing */
char _DPSTracingOn = FALSE;

/* the last event read with correct mask.  Essentially, a global shared by
   the scanner and _DPSGetOrPeekEvent
 */
NXEvent *_DPSLastEvent = NULL;

void *_DPSUnused1 = NULL;	
/* size of message we must alloc for a msg_receive */
short _DPSMsgSize = 0;

int (*_DPSZapCheck)() = NULL;		/* function called in event loop */
int _DPSMsgTracingOn = 0;		/* turns on message tracing */

#if defined(DEBUG) && defined(PMON)

/* #import "/Net/harlie/mk/osdev/PMON/pmon/include/pmon/pmon.h" */

#define PMON_SOURCE_APPKIT 200 /*temp*/

struct {
    int src;
    void *data;
} _DPSPmonSrcDataStorage = {PMON_SOURCE_APPKIT, NULL};

void *_DPSPmonSrcData = &_DPSPmonSrcDataStorage;

#else

int _DPSPmonFiller[3] = {0};

#endif

/* global flag for event tracing */
char _DPSEventTracingOn = FALSE;

/* global flag for whether to do 1.0 compatibility hacks */
char _DPSDo10Compatibility = FALSE;

/* global flag for whether to do setnextstepcontext */
char _DPSDoNSC = TRUE;

char _DPSUnused2 = 0;

/* Global data (in the data section) */
char _dpsclient_data_pad[188] = { 0 };
