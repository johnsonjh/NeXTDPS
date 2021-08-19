/*
 * pserrtest.c	- A program that tests the behavior of the DPS
 *		  error and test handling routines.  This is known
 *		  as reverse engineering as a substitute for poor
 *		  documentation.
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
char	test1[] = "boguscommand";


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
 * Routine:	main() - The main hammo
 *
 * Function:	This does everything, what else.
 **********************************************************************/
main()
{
    DPSContext	ctxt;

    ctxt = DPSCreateContext(NULL, NULL, NULL, errorHandler);

    /* Test 1*/
    printf("Test 1: \"%s\"", test1);
    DPSWriteData(ctxt, test1, sizeof(test1));
    DPSFlushContext(ctxt);
    DPSWaitContext(ctxt);
}
