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
Ed Taft: Wed Dec 13 18:18:14 1989
Ivor Durham: Wed Mar 23 13:11:32 1988
Bill Paxton: Tue Mar 29 09:25:41 1988
Paul Rovner: Wednesday, January 27, 1988 4:28:47 PM
Jim Sandman: Wed May 31 17:11:14 1989

End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include DEVICETYPES
#include DEVIMAGE
#include DEVPATTERN
#include EXCEPT
#include FP
#include PSLIB


#include "imagepriv.h"

/* 2, 4, 8 bit source; 2 bit dest */

#define LOG2BPP 1
#define BPP 2
#define SMASK (SCANMASK >> LOG2BPP)
#define SSHIFT (SCANSHIFT - LOG2BPP)
#define PIXDELTA pixelDelta

#define PROCNAME ImS1XD12

#define DECLAREVARS \
  integer pixelDelta;
  
#define SETUPPARAMS(args) \
  if (args->bitsPerPixel != 2) RAISE(ecLimitCheck, (char *)NIL); \
  pixelDelta = args->gray.delta; 


#include "ims1xd1xmain.c"

  
