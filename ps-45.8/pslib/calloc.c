/*
  calloc.c

Copyright (c) 1986, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

This module is derived from a C library module provided to Adobe
under a Unix source license from AT&T Bell Laboratories, U. C. Berkeley,
and/or Sun Microsystems. Under the terms of the source license,
this module cannot be further redistributed in source form.

Original version:
Edit History:
Ed Taft: Sun May  1 17:07:02 1988
Ivor Durham: Sat Feb 13 10:38:01 1988
Perry Caro: Thu Nov  3 16:04:07 1988
End Edit History.

	os_calloc - allocate and clear memory block
*/

#include PACKAGE_SPECS
#include PSLIB

public char *os_calloc(num, size)
long int num, size;
{
	register char *mp;

	num *= size;
	mp = os_malloc(num);
	if(mp == NULL)
		return(NULL);
	os_bzero(mp, num);
	return(mp);
}

public char *os_sureCalloc(num, size)
long int num, size;
{
	register char *mp;

	num *= size;
	mp = os_malloc(num);
	if (mp == NULL)
	  CantHappen();
	os_bzero(mp, num);
	return(mp);
}
