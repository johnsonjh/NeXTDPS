/*
  sprintf.c

Copyright (c) 1985, 1988 Adobe Systems Incorporated.
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
Ed Taft: Thu May 12 13:46:12 1988
Perry Caro: Mon Jan 23 16:48:04 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include PSLIB
#include PUBLICTYPES
#include STREAM
#include <varargs.h>

extern int os_doprint();
extern readonly StmProcs closedStmProcs;

public char *os_sprintf (va_alist)
  va_dcl
{
  char *str;
  char *format;
  StmRec srec;
  va_list argptr;

  va_start(argptr);
  str = va_arg(argptr, char *);
  format = va_arg(argptr, char *);

  os_bzero((char *) &srec, (integer) sizeof(StmRec));
  srec.procs = &closedStmProcs;
  srec.ptr = str;
  srec.cnt = 32767;

  (void) os_doprint (&srec, format, &argptr);
  *srec.ptr = '\0';
  return str;
}

