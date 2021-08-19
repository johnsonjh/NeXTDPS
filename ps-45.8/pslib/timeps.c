/*
  timeps.c

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
Ed Taft: Sun Jul  2 16:20:27 1989
End Edit History.

Implementation of os_time based on os_clock, for stand-alone use
*/

#include PACKAGE_SPECS
#include PSLIB
#include <sys/time.h>

static unsigned long int timeZero, timePrev;

unsigned long int os_time(tloc)
  unsigned long int *tloc;
  {
  unsigned long int c = timeZero + os_clock()/1000;
  if (c < timePrev) {		/* clock wrapped around; update base by */
    timeZero += 4294967;	/* (2**32)/1000 */
    c += 4294967;
    }
  timePrev = c;
  if (tloc) *tloc = c;
  return c;
  }

int os_stime(tloc)
  long int *tloc;
  {
  timeZero = (timePrev = *tloc) - os_clock()/1000;
  return 0;
  }
