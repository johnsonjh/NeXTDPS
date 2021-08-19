/*
  os_fcvt.c

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
Ed Taft: Sat Apr 30 15:57:18 1988
Paul Rovner: Monday, June 6, 1988 1:42:20 PM
Ivor Durham: Mon Aug 22 13:42:58 1988
End Edit History.

This module implements one of the os_xxx operations as a simple veneer
over the corresponding procedure in the C runtime library.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#if (OS == os_mpw)
#include DPSMACSANITIZE
#endif

#include "os_math.h"

extern char *fcvt(); /* in case not declared in <math.h> */


public char *os_fcvt(value, ndigits, decpt, sign)
  double value; int ndigits, *decpt, *sign;
{
  char   *result;

  ResetErrno ();
#if	(OS == os_mpw)
  {
    result = (char *) DPSCall6Sanitized (fcvt, value, ndigits, decpt, sign);
  }
#else	(OS == os_mpw)
  result = fcvt (value, ndigits, decpt, sign);
#endif	(OS == os_mpw)
  CheckErrno ();
  return result;
}
