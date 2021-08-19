/*
  printf.c

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
Ed Taft: Mon Dec  4 14:27:20 1989
Ivor Durham: Fri Jun 10 11:53:22 1988
Perry Caro: Mon Jan 23 16:44:43 1989
End Edit History.

*/
#include PACKAGE_SPECS
#include PUBLICTYPES
#include STREAM
#include <varargs.h>

extern int os_doprint();

public int os_fprintf (va_alist)
  va_dcl
{
  Stm stm;
  char *format;
  va_list argptr;

  va_start(argptr);
  stm = va_arg(argptr, Stm);
  format = va_arg(argptr, char *);
  return os_doprint(stm, format, &argptr);
}

public int os_printf (va_alist)
  va_dcl
{
  char *format;
  va_list argptr;

  va_start(argptr);
  format = va_arg(argptr, char *);
  return os_doprint(os_stdout, format, &argptr);
}

public int os_eprintf (va_alist)
  va_dcl
{
  char *format;
  va_list argptr;
  int result;

  va_start(argptr);
  format = va_arg(argptr, char *);
  result = os_doprint(os_stderr, format, &argptr);
  fflush(os_stderr);
  return result;
}
