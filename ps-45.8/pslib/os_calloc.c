/*
  os_calloc.c

	Copyright (c) 1988 Adobe Systems Incorporated.
			All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

      PostScript is a registered trademark of Adobe Systems Incorporated.
	Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft, April 1988
Edit History:
Ed Taft: Sun May  1 15:38:33 1988
Joe Pasqua: Fri Jan  6 15:17:37 1989
Paul Rovner: Tue Nov 21 11:34:17 1989
End Edit History.

This module implements one of the os_xxx operations as a simple veneer
over the corresponding procedure in the C runtime library.
*/

#include PACKAGE_SPECS
#include EXCEPT
#include PSLIB
#include FOREGROUND

extern char *calloc();

public char *os_sureCalloc(num, size)
long int num, size;
{
	register char *mp;

	if (!(mp = calloc(num, size)))
	  CantHappen();
	return(mp);
}
