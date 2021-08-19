/*
  gets.c

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
Ed Taft: Mon May  2 14:00:31 1988
Perry Caro: Thu Nov  3 13:33:38 1988
End Edit History.

*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include STREAM

public char *os_gets(str)
  char *str;
{
  register char *ptr;
  register int c;

  ptr = str;
  while (true)
    {
    if ((c = getc(os_stdin)) < 0)
      if (ptr == str) return NULL;
      else break;
    if (c == '\n') break;
    *ptr++ = c;
    }

  *ptr = '\0';
  return str;
}
