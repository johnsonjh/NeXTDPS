/*
  rename.c

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

Original version:
Edit History:
Ed Taft: Sun May  1 14:05:56 1988
End Edit History.

 * The 4.2bsd implementation guarantees that if the system crashes
 * while performing the rename, that an instance of the 'to' file
 * will always exist.
 * This Q&D implementation does not.  mjf
*/

#include <errno.h>

rename ( from, to )
  char *from, *to;
  {
  int i;
  if ((i = unlink(to)) &&
    (errno != ENOENT)) return(i); /*non-existence of 'to' is okay*/
  if (i = link(from, to)) return(i);
  if (i = unlink(from)) return(i);
  return(0);
  }
 
