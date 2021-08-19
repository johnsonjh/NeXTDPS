/*
  os_atof.c

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
Ed Taft: Sat Apr 30 15:27:30 1988
Ivor Durham: Thu Jul 14 13:26:38 1988
Paul Rovner: Monday, June 6, 1988 1:42:20 PM
End Edit History.

This module implements one of the os_xxx operations as a simple veneer
over the corresponding procedure in the C runtime library.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT	/* For sparc hack */
#include "os_math.h"

/*
   The following hack is necessary because we currently cannot identify
   the compiler dynamically:  On the MIPS machine, procedure prototypes
   are required, so then CHARSTAR is defined appropriately.
 */

#if	(ISP == isp_r2000) && (OS == os_sysv)
#define	CHARSTAR char *
#else	(ISP == isp_r2000) && (OS == os_sysv)
#define	CHARSTAR
#endif	(ISP == isp_r2000) && (OS == os_sysv)

extern double atof(CHARSTAR);  /* in case not declared in <math.h> */

public double os_atof(str)
  char *str;
{
  double  result;

  ResetErrno ();
#if	(OS == os_mpw)
  {
    extern extended DPSCall1Sanitized ();
    result = DPSCall1Sanitized (atof, str);
  }
#else	(OS == os_mpw)
  result = atof (str);
#endif	(OS == os_mpw)
#if	(ISP == isp_sparc)
  if ((errno != 0) && (os_strcmp (str, "0.0"))) {
    errno = 0;
    result = 0.0;
  }
#endif	(ISP == isp_sparc)
  CheckErrno ();
  return result;
}
