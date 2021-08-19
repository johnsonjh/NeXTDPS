/*
 * postevent.c	- A program that posts an event in the Window server
 *		  and tries to receive it back.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.  Sheesh!
 */


/*
 * Include files.
 */

#include <dpsclient/dpsclient.h>


/*
 * Constants.
 */
char	postevent[] = "Appdefined 0.0 0.0 0 0 currentcontext 0 0 0 Bypscontext postevent pop";


/**********************************************************************
 * Routine:	errorHander() - Handle a PostScript error.
 *
 * Function:	This routine handles a PostScript error by printing
 *		it out.
 **********************************************************************/
void
errorHandler(DPSContext ctxt, DPSErrorCode errorCode,
	     long unsigned int arg1, long unsigned int arg2)
{
    fprintf(stderr, "errorHandler called, code = %d\n", errorCode);
    if (errorCode == dps_err_ps) {
	DPSPrintError(stderr, (DPSBinObjSeq)arg1);
    }
    exit(1);
}

/**********************************************************************
 * Routine:	_eventHandler() - Handles DPS events.
 *
 * Function:	This handler will receive an event and print that it
 *		received it.
 **********************************************************************/
int
_eventHandler(NXEvent *event)
{
    printf("Event received\n");
    return(0);
}



/**********************************************************************
 * Routine:	main() - The main hammo
 *
 * Function:	This does everything, what else.
 **********************************************************************/
main()
{
    DPSContext	ctxt;

    ctxt = DPSCreateContext(NULL, NULL, NULL, errorHandler);
    DPSSetEventFunc(ctxt, (DPSEventFilterFunc) _eventHandler);

    /* Test 1*/
    printf("Test 1: \"%s\"\n", postevent);
    DPSWriteData(ctxt, postevent, sizeof(postevent));
    DPSFlushContext(ctxt);
    DPSWaitContext(ctxt);
}
