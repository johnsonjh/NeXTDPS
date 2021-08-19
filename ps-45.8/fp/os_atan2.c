/*
  os_atan2.c

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
Ed Taft: Thu Apr 28 17:27:11 1988
Paul Rovner: Monday, June 6, 1988 1:42:20 PM
Ivor Durham: Mon Jun 20 13:29:09 1988
End Edit History.

This module implements one of the os_xxx operations as a simple veneer
over the corresponding procedure in the C runtime library.
*/

#include PACKAGE_SPECS
#include "os_math.h"

public double os_atan2(num, den)
  double num, den;
{
  double  result;

  ResetErrno ();
#if	(OS == os_mpw)
  {
    extern extended DPSCall6Sanitized ();
    result = DPSCall6Sanitized (atan2, num, den);
  }
#else	(OS == os_mpw)
  result = atan2 (num, den);
#endif	(OS == os_mpw)
  CheckErrno ();
  return result;
}
