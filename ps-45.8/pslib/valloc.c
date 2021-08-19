/*
  valloc.c

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
Ed Taft: Sun May  1 14:03:45 1988
End Edit History.

*/

#include PACKAGE_SPECS
#include PSLIB

public char *os_valloc(size)
  long int size;
  {return os_memalign(getpagesize(), size);}
