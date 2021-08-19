/*
  os_ldexp.c

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
Ed Taft: Sun May  1 11:42:26 1988
Paul Rovner: Monday, June 6, 1988 1:42:20 PM
Ivor Durham: Wed Jun 22 15:00:47 1988
End Edit History.

This module implements one of the os_xxx operations as a simple veneer
over the corresponding procedure in the C runtime library.
*/

#include PACKAGE_SPECS
#include "os_math.h"

public double os_ldexp(x, exp)
  double x; integer exp;
{
  double  result;

  ResetErrno ();
#if	(OS == os_mpw)
  {
    extern extended DPSCall4Sanitized ();
    result = DPSCall4Sanitized (ldexp, x, exp);
  }
#else	(OS == os_mpw)
  result = ldexp (x, exp);
#endif	(OS == os_mpw)
#if USE_ERRNO
  /* ldexp may report ERANGE for underflow; don't raise exception for that */

  if (errno != 0 && !(errno == ERANGE && result == 0.0))
    ReportErrno (errno);
#endif USE_ERRNO
  return result;
}
