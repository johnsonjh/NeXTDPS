/*
 * netinfo.h	- Interface file to NetInfo routines.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */

/*
 * Constants.
 */
#define	LOCALNAME	"Local_Printer"
#define	IGNORE		"_ignore"

/*
 * External routines.
 */

char	*NIDirectoryName(const char *path); /* Does local directory exist? */
char	*NINextPrinterCreate();		/* Create a new NeXT printer entry */
void	NIPrinterDisable(const char *path); /* Disable a local printer */
void	NIPrinterEnable(const char *path); /* Enable a local printer */

