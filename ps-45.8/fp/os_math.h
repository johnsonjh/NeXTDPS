/*
  os_math.h

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
Scott Byer: Tue May 16 10:12:02 1989
Ed Taft: Sun May  1 12:04:01 1988
Ivor Durham: Thu Aug 25 14:49:27 1988
End Edit History.

These definitions are used in the "veneer" implementations of all the
os_xxx math procedures.
*/

#ifndef	OS_MATH_H
#define	OS_MATH_H

#include ENVIRONMENT
#include EXCEPT
#include FP
#include FPFRIENDS
#include <math.h>
#include <errno.h>

/*
  C math libraries typically report errors in one of two ways:

    1. Store a value in the external variable "errno";
    2. Generate a SIGFPE signal that can be caught by a signal handler.

  The generic veneer implementations are normally prepared for errors to
  be reported either way. The ResetErrno and CheckErrno macros deal with
  case 1. The fp package initialization arms a SIGFPE signal handler
  to deal with case 2. However, one or the other of these methods may
  not work in some configurations and may need to be conditionally
  suppressed or altered. In particular, some environments define "errno"
  in a way that makes it effectively read-only; other environments
  do not implement a SIGFPE signal.
*/

#ifndef USE_ERRNO
#define USE_ERRNO (OS != os_vaxeln)
#endif USE_ERRNO

#ifndef USE_SIGNAL
#define USE_SIGNAL (OS != os_mpw)
#endif USE_SIGNAL

#if USE_ERRNO
#define ResetErrno() {errno = 0;}
#define CheckErrno() {if (errno != 0) ReportErrno(errno);}
#else USE_ERRNO
#define ResetErrno()
#define CheckErrno()
#endif USE_ERRNO

extern procedure ReportErrno(/* int n */);
/* Raises the exception appropriate for the errno value n; does not return */

#endif	OS_MATH_H
