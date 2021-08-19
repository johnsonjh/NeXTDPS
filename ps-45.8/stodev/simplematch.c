/*
  simplematch.c

Copyright (c) 1985, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Wed Jun 19 15:00:02 1985
Edit History:
Ed Taft: Wed May  4 13:51:10 1988
Ivor Durham: Mon May 16 13:53:14 1988
End Edit History.

Simple pattern match utility. This version uses a simpleminded
recursive pattern matching algorithm.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include STODEV

public boolean SimpleMatch(name, pattern)
  register char *name, *pattern;
  {
  register char c;
  while (true)
    {
    switch (c = *pattern++)
      {
      case '\0':
        return *name == '\0';
      case '*':
        if (*pattern == '\0') return true;  /* accelerate trailing * match */
        do {if (SimpleMatch(name, pattern)) return true;}
	while (*name++ != '\0');
	return false;
      case '?':
        if (*name++ == '\0') return false;
        break;
      case '\\':
        if ((c = *pattern++) == '\0') return *name == '\0';
	/* fall through */
      default:
        if (c != *name++) return false;
      }
    }
  }
