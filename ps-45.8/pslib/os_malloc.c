/*
  os_malloc.c

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
Ed Taft: Sun May  1 17:01:55 1988
Ivor Durham: Sat Aug 20 12:09:26 1988
Paul Rovner: Tue Nov 21 11:33:08 1989
Perry Caro: Tue Nov 15 11:39:11 1988
Joe Pasqua: Fri Jan  6 15:22:40 1989
End Edit History.

This module implements one of the os_xxx operations as a simple veneer
over the corresponding procedure in the C runtime library.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include EXCEPT
#include PSLIB
#include FOREGROUND

extern char *malloc(), *realloc();
extern void free ();

public char *os_sureMalloc(size)
  long int size;
{
  char *p;
  if (!(p = (char *)malloc(size))) CantHappen();
  return p;
}
