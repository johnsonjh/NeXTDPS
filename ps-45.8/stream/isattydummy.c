/*
  isattydummy.c

Copyright (c) 1988 Adobe Systems Incorporated.
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
Ed Taft: Tue May  3 10:08:31 1988
End Edit History.

Dummy implementation of "isatty" for environments whose C libraries
lack one.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES

public int isatty(fd)
  int fd;
{
  return 0;
}
