/*
  sbrk.c

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
Ed Taft: Tue Sep 19 11:17:49 1989
End Edit History.

Emulation of sbrk for standalone configurations
*/


char *curbrk;
char *maxbrk;

char *sbrk(incr)
	int incr;
{
  char *temp = curbrk;
  if ((curbrk + incr) > maxbrk) return (char *)-1;
  curbrk += incr;
  return (temp);
}
