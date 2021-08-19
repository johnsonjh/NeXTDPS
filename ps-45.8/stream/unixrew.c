/*
  unixrew.c

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
Ed Taft: Tue May  3 09:49:08 1988
Paul Rovner: Tuesday, June 7, 1988 8:54:45 AM
Ivor Durham: Sat Aug 20 12:14:12 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include PUBLICTYPES
#include STREAM

#if	(OS == os_mpw)
#include DPSMACSANITIZE
#endif	(OS == os_mpw)

#include "unixstmpriv.h"

extern procedure lseek();

public procedure os_rewind(stm)
  register Stm stm;
{
  fflush(stm);

#if	(OS == os_mpw)
  {
  DPSCall3Sanitized(lseek, fileno(stm), 0L, 0);
  }
#else	(OS == os_mpw)
  lseek(fileno(stm), 0L, 0);
#endif	(OS == os_mpw)
  stm->cnt = 0;
  stm->ptr = stm->base;
  stm->flags.error = stm->flags.eof = 0;
  if (stm->flags.readWrite)
    stm->flags.read = stm->flags.write = 0;
}
