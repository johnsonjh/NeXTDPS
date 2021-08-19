/*
  bcopy.c

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
Ed Taft: Sun May  1 13:13:08 1988
John Gaffney: Fri Aug  9 12:41:07 1985
Jim Sandman: Wed Mar  9 14:28:17 1988
End Edit History.

***** slow machine-independent version *****

Note: some standard C libraries have this procedure, but it is
not always implemented correctly. In particular:

  1. bcopy must work correctly for all positive values of the byte count
     (even if greater than 2**16) and must do nothing if the count is
     zero or negative.

  2. The count must be treated as a long int as opposed to an int,
     if those two types are of different size.

  3. bcopy must work correctly even if source and destination blocks
     overlap; i.e., its result must be as if all the source bytes
     are read before any destination bytes are written.
*/

#include PACKAGE_SPECS
#include PSLIB

public procedure os_bcopy(from, to, cnt)
  register char *from, *to;
  register long int cnt;
  /* byte copy routine: copies cnt bytes between blocks designated
   * by *from and *to. */
  {
    if (to < from)
      for ( ; --cnt >= 0; ) *to++ = *from++;
    else if (to > from)
      for (to += cnt, from += cnt; --cnt >= 0; ) *--to = *--from;
  }
