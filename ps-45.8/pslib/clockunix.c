/*
  clockunix.c

Copyright (c) 1984, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version:
Edit History:
Ted Cohn: 03May90 os_clock() now uses gettimeofday instead of ftime (bug 4491)
Larry Baer: Thu Nov 16 09:20:22 1989
Ed Taft: Sun Dec 17 18:31:36 1989
Paul Rovner: Thursday, November 19, 1987 11:21:35 AM
Ivor Durham: Sat Sep 24 12:49:38 1988
End Edit History.

Millisecond clock implementation for Unix
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include PSLIB

#include <sys/param.h>
#include <sys/types.h>
#include <sys/times.h>
#if	(OS != os_sysv)
#include <sys/time.h>
#include <sys/timeb.h>
#endif	(OS != os_sysv)

#ifndef	HZ
#define	HZ	60
#endif	HZ

public integer os_clock()
  {
#if	((OS != os_sysv) && (OS != os_aix))
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return (unsigned long) (1000*tp.tv_sec)+(tp.tv_usec/1000);
#else	((OS != os_sysv) && (OS != os_aix))
  return ((unsigned long) 1000 * (unsigned long) time(0));
#endif	((OS != os_sysv) && (OS != os_aix))
  }

public integer os_userclock()
  {
  struct tms t;
  times(&t);
  return (unsigned long) t.tms_utime * (unsigned long) (1000/HZ);
  }
