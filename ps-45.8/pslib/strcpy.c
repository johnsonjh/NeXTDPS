/*
  strcpy.c

Copyright (c) 1986, 1988 Adobe Systems Incorporated.
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
Ed Taft: Wed May  4 17:14:10 1988
Ivor Durham: Sat May  7 08:04:45 1988
Joe Pasqua: Fri Jan  6 15:54:33 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include PSLIB

public char *os_strcpy(to, from)
  char *to; register char *from;
{
  register char *s = to;
  while ((*s++ = *from++) != '\0') ;
  return to;
}

public char *os_strncpy(to, from, cnt)
  char *to, *from;
  int cnt;
{
  int len = os_strlen (from);

  if (len < cnt) {
    os_bcopy (from, to, (long int)len);
    os_bzero (&to[len], (long int)(cnt - len));
  } else
    os_bcopy (from, to, (long int)cnt);

  return (to);
}
