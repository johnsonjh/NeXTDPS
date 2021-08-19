/*
  fputs.c

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
Ed Taft: Mon May  2 13:55:54 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include STREAM

int os_fputs(str, stm)
  register char *str;
  register Stm stm;
{
  register int result, c;

  while ((c = *str++) != '\0')
    result = putc(c, stm);

  return result;
}
