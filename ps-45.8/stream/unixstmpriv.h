/*
  unixstmpriv.h

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Wed Mar 21 13:51:26 1984
Edit History:
Ed Taft: Mon May  2 18:06:24 1988
Ivor Durham: Sun Feb  7 12:41:53 1988
Jim Sandman: Wed Mar 16 15:46:55 1988
Joe Pasqua: Mon Jan  9 11:11:52 1989
End Edit History.

Private details of the Unix stream implementation
*/

#ifndef UNIXSTMPRIV_H
#define UNIXSTMPRIV_H

#include PUBLICTYPES
#include UNIXSTREAM

#define BUFSIZ 1024		/* usually overridden by file buffer size */

typedef struct {		/* Interpretation of StmRec.data */
  unsigned file:8;		/* Unix file number */
  unsigned noBuffer: 1;		/* unbuffered stream */
  unsigned myBuffer: 1;		/* stream owns buffer */
  unsigned lineBuffer: 1;	/* line-buffered I/O */
  unsigned :5;
  unsigned bufsiz;		/* buffer size for this file */
  } UnixData;

extern readonly StmProcs unixStmProcs;

#define GetPUnixData(stm) ((UnixData *) &stm->data)
#define fileno(stm) (GetPUnixData(stm)->file)

#endif UNIXSTMPRIV_H
