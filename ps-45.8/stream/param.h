/*
  param.h

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

This module is derived from a C library module provided to Adobe
under a Unix source license from AT&T Bell Laboratories, U. C. Berkeley,
and/or Sun Microsystems. Under the terms of the source license,
this module cannot be further redistributed in source form.

Original version:
Edit History:
Ed Taft: Mon May  2 10:52:08 1988
End Edit History.
*/

#include ENVIRONMENT

/* Maximum number of digits in any integer (long) representation */
#define	MAXDIGS	11

/* Largest (normal length) positive integer */
#define	MAXINT	2147483647

/* A long with only the high-order bit turned on */
#define	HIBIT	0x80000000L

/* Convert a digit character to the corresponding number */
#define	tonumber(x)	((x)-'0')

/* Convert a number between 0 and 9 to the corresponding digit */
#define	todigit(x)	((x)+'0')

/* Data type for flags */
typedef	char	bool;

/* Maximum total number of digits in E format */
#define	MAXECVT	17

/* Maximum number of digits after decimal point in F format */
#define	MAXFCVT	60

/* Maximum significant figures in a floating-point number */
#define	MAXFSIG	17

/* Maximum number of characters in an exponent */
#define	MAXESIZ	4

/* Maximum (positive) exponent or greater */
#if IEEEFLOAT
/* IEEE floating point -- maximum double representation */
#define MAXEXP 309
#else
#define	MAXEXP	40
#endif
