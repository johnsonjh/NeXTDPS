/*
  timempw.c

	Copyright (c) 1989 Adobe Systems Incorporated.
			All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

      PostScript is a registered trademark of Adobe Systems Incorporated.
	Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Paul Rovner, 1989
Edit History:
Ed Taft: Sun Jul  2 16:38:42 1989
End Edit History.

Implementation of os_time for mpw.
*/

#include PACKAGE_SPECS
#include PSLIB

#ifdef MPW3
#include <osutils.h>
#endif /* MPW3 */

#define Fixed MPWFixed

#include DPSMACSANITIZE

private long int innerTime() {
  unsigned long int t;
  GetDateTime(&t);
  return t;
}

public long int os_time(ignore) int ignore; {
  return (long int)DPSCall0Sanitized(innerTime);
}

public long int os_stime(ignore) long int ignore; {}
