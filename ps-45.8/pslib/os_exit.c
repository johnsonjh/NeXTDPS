/*
  os_exit.c

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

Original version: Ivor Durham: Sat May  7 10:23:31 1988
Edit History:
Ivor Durham: Sat May  7 10:48:26 1988
End Edit History.

This module implements one of the os_xxx operations as a simple veneer
over the corresponding procedure in the C runtime library.
*/

#include PACKAGE_SPECS
#include PSLIB

public void os_exit(code)
  int code;
{
  exit(code);
}

public void os_abort()
{
  abort();
}
