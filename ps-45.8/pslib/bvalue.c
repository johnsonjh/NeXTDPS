/*
  bvalue.c

Copyright (c) 1987, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Wed Dec  2 11:35:55 1987
Edit History:
Ed Taft: Wed Dec  2 11:35:55 1987
Jim Sandman: Wed Mar  9 14:31:08 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include PSLIB

public procedure os_bvalue(to, cnt, value)
  register char *to;
  register long int cnt;
  register char value;
  {
    for ( ; --cnt >= 0; ) *to++ = value;
  }
