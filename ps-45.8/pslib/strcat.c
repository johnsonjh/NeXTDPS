/*
  strcat.c

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
Ed Taft: Wed May  4 17:12:13 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include PSLIB

public char *os_strcat(to, from)
  char *to; register char *from;
{
  register char *s = to;
  while (*s++ != '\0') ;
  --s;
  while ((*s++ = *from++) != '\0') ;
  return to;
}
