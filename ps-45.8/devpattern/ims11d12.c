/* PostScript generic image module

Copyright (c) 1983, '84, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Doug Brotz: Sept. 13, 1983
Edit History:
Doug Brotz: Mon Aug 25 14:41:39 1986
Chuck Geschke: Mon Aug  6 23:25:04 1984
Ed Taft: Tue Jul  3 12:55:59 1984
Ivor Durham: Sun Feb  7 14:02:56 1988
Bill Paxton: Tue Mar 29 09:20:13 1988
Paul Rovner: Monday, November 23, 1987 10:11:58 AM
Jim Sandman: Wed May 31 16:52:57 1989
Joe Pasqua: Tue Feb 28 11:26:38 1989
End Edit History.
*/

#define SMASK (SCANMASK >> 1)
#define SSHIFT (SCANSHIFT - 1)
#define SUNIT (SCANUNIT >> 1)
#define LOG2BPP 1

#define PROCNAME ImS11D12

#define DECLAREVARS

#define SETUPPARAMS(args) \
  if (args->bitsPerPixel != 2) RAISE(ecLimitCheck, (char *)NIL);

#include "ims11d1xmain.c"
