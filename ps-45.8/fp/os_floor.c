/*
  os_floor.c

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
Ed Taft: Thu Apr 28 17:42:30 1988
Paul Rovner: Tuesday, June 7, 1988 9:11:28 AM
Ivor Durham: Wed Jun 22 08:51:16 1988
End Edit History.

This module implements one of the os_xxx operations as a simple veneer
over the corresponding procedure in the C runtime library.
*/

#include PACKAGE_SPECS
#include "os_math.h"

public double os_floor(x)
  double x;
{
  double  result;

  ResetErrno ();
#if	(OS == os_mpw)
  {
    extern extended DPSCall3Sanitized ();
    result = DPSCall3Sanitized (floor, x);
  }
#else	(OS == os_mpw)
  result = floor (x);
#endif	(OS == os_mpw)
  CheckErrno ();
  return result;
}
