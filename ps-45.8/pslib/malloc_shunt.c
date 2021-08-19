/*
  malloc_shunt.c

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

Original version: Ivor Durham: Sat Feb 13 10:08:53 1988
Edit History:
Ivor Durham: Sat Feb 13 10:45:35 1988
Ed Taft: Sun May  1 13:45:04 1988
End Edit History.

This is a "shunt" implementation of a standard C library facility (e.g.,
malloc) in terms of the corresponding PostScript library procedure (e.g.,
os_malloc). It is needed when using other C library facilities that call
malloc rather than os_malloc, since the two storage allocators cannot
coexist. Use of this shunt makes sense only when os_malloc is itself the
full implementation; if os_malloc is simply a veneer over malloc, the shunt
shouldn't be used.
*/

#include PACKAGE_SPECS
#include PSLIB

public char *malloc(nbytes)
  long int nbytes;
{
  return(os_malloc(nbytes));
}

public procedure addmem(ptr, nbytes)
  char *ptr;
  long int nbytes;
{
  os_addmem(ptr, nbytes);
}

public procedure free(ap)
  char *ap;
{
  os_free(ap);
}

public char *realloc(p, nbytes)
  union store *p;
  long int nbytes;
{
  return (os_realloc(p, nbytes));
}
