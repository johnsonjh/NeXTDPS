/*
  os_ecvt.c

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
Ed Taft: Sat Apr 30 15:56:16 1988
Paul Rovner: Tuesday, June 7, 1988 5:29:34 PM
Ivor Durham: Sat Aug 20 11:34:39 1988
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

extern char *ecvt(); /* in case not declared in <math.h> */

public char *os_ecvt(value, ndigits, decpt, sign)
  double value; int ndigits, *decpt, *sign;
{
  char   *result;

  ResetErrno ();
#if	(OS == os_mpw)
  {
    result = (char *) DPSCall6Sanitized (ecvt, value, ndigits, decpt, sign);
  }
#else	(OS == os_mpw)
  result = ecvt (value, ndigits, decpt, sign);
#endif	(OS == os_mpw)
  CheckErrno ();
  return result;
}
