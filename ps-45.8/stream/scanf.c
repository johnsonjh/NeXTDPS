/*
  scanf.c

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
Ed Taft: Thu May 12 13:14:21 1988
Perry Caro: Mon Jan 23 16:47:24 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include PSLIB
#include PUBLICTYPES
#include STREAM
#include <varargs.h>

extern readonly StmProcs closedStmProcs;

public int os_scanf(va_alist)
  va_dcl
{
  char *format;
  va_list argptr;

  va_start(argptr);
  format = va_arg(argptr, char *);
  return os_doscan(os_stdin, format, &argptr);
}

public int os_fscanf(va_alist)
  va_dcl
{
  Stm stm;
  char *format;
  va_list argptr;

  va_start(argptr);
  stm = va_arg(argptr, Stm);
  format = va_arg(argptr, char *);
  return os_doscan(stm, format, &argptr);
}

/*ARGSUSED*/
private int SSUnGetc(ch, stm)
  int ch; Stm stm;
/* private ungetc needed because the standard one chokes on closedStm */
{
  stm->ptr--; stm->cnt++;
  return ch;
}

public int os_sscanf(va_alist)
  va_dcl
{
  register char *str;
  char *format;
  StmRec srec;
  StmProcs sprocs;
  va_list argptr;

  va_start(argptr);
  str = va_arg(argptr, char *);
  format = va_arg(argptr, char *);

  os_bzero((char *) &srec, (integer) sizeof(StmRec));
  sprocs = closedStmProcs;
  sprocs.UnGetc = SSUnGetc;
  srec.procs = &sprocs;
  srec.ptr = srec.base = str;
  while (*str++ != '\0') ;
  srec.cnt = --str - srec.base;

  return os_doscan(&srec, format, &argptr);
}
