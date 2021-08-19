/*
  os_memalign.c

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
Ed Taft: Sun May  1 15:41:48 1988
Paul Rovner: Tue Nov 21 11:36:27 1989
End Edit History.

This module implements one of the os_xxx operations as a simple veneer
over the corresponding procedure in the C runtime library.
*/

#include PACKAGE_SPECS
#include PSLIB
#include FOREGROUND

extern char *memalign();

public char *os_memalign(boundary, size)
  long int boundary, size;
{
  char *p;
  FGEnterMonitor();
  p = memalign(boundary, size);
  FGExitMonitor();
  return p;
}
