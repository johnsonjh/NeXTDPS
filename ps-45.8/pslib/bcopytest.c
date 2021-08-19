/*
  bcopytest.c

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
Ivor Durham: Thu Jun 23 15:59:23 1988
End Edit History.
*/

#include <stdio.h>

#define NBYTES 1000
unsigned char bytes[NBYTES];
long int source, dest, count;

main()
{
register long int i, j;
register unsigned char c, *p;

while (1) {
  for (i = NBYTES, p = &bytes[i]; --i >= 0; ) *--p = i;
  source = rand() % NBYTES;
  dest = rand() % NBYTES;
  count = (source > dest)? NBYTES - source : NBYTES - dest;
  count = rand() % count;
  os_bcopy(&bytes[source], &bytes[dest], count);
  for (c = NBYTES, p = &bytes[NBYTES], j = NBYTES - (dest+count); --j >= 0; )
    if (*--p != --c) Bug1(p, c);
  for (c = source + count, p = &bytes[dest+count], j = count; --j >= 0; )
    if (*--p != --c) Bug2(p, c);
  for (c = dest, p = &bytes[dest], j = dest; --j >= 0; )
    if (*--p != --c) Bug1(p, c);
  os_printf("."); fflush(os_stdout);
  }
}

Bug1(p, c)
unsigned char *p, c;
{
os_printf("Wrote outside dest: source=%D, dest=%D, count=%D, bytes[%D]=%D, should be %D\n",
  source, dest, count, p-bytes, *p, c);
}

Bug2(p, c)
unsigned char *p, c;
{
os_printf("Failed to copy: source=%D, dest=%D, count=%D, bytes[%D]=%D, should be %D\n",
  source, dest, count, p-bytes, *p, c);
}

