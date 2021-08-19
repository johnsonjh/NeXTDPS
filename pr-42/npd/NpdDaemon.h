/*
 * NpdDaemon.h	- Interface file for the Next Printer Daemon
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */

#import "Daemon.h"
#import <appkit/errors.h>

/*
 * Types.
 */
typedef enum {
    NPDnetinfofailed = NX_APPBASE,	/* NetInfo error during routine */
    NPDdocformat,			/* Format error in document */
    NPDspooling,			/* Spooling error */
    NPDlocalprinting,			/* Local printing error */
    NPDpsdevice,			/* Error with PostScript device */
} NPDErrorTokens;


/*
 * Global variables.
 */
extern unsigned char	server_key[];


@interface NpdDaemon : Daemon
/*
 * This class implements the Next Printer Daemon.
 */
{
    id		localPrinter;		/* The local printer object */
}

- localPrinter;
/*
 * Return the id of the local printer.
 */

@end

