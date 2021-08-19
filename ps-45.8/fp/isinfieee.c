/*
  isinfieee.c

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
Ed Taft: Sat Sep 30 17:23:55 1989
Joe Pasqua: Mon Jan  9 10:40:28 1989
End Edit History.

 * Recognize an infinity or a NaN when one is presented.
 * This is for keeping various IO routines out of trouble 

This version is for architectures using standard IEEE representation.
*/

#include PACKAGE_SPECS
#include FPFRIENDS

#if SWAPBITS
#define HI d1
#define LO d0
#else SWAPBITS
#define HI d0
#define LO d1
#endif SWAPBITS

public boolean os_isinf( d0, d1 )
    unsigned long d0,d1;
    /* a lie -- actually its a ``double'' */
{
    if (LO != 0 ) return 0; /* nope -- low-order must be all zeros */
    if (HI != 0x7ff00000 && HI != 0xfff00000) return 0; /* nope */
    return 1;
}

#define EXPONENT 0x7ff00000
#define SIGN     0x80000000
public boolean os_isnan( d0,d1 )
    unsigned long d0,d1;
    /* a lie -- actually its a ``double'' */
{
    if ((HI & EXPONENT) != EXPONENT ) return 0; /* exponent wrong */
    if ((HI & ~(EXPONENT|SIGN)) == 0 && LO == 0 ) return 0; /* must have bits */
    return 1;
}

#if MC68K
/* following is for compatibility with Sun 4.0 version of <math.h> */

double infinity()
{
  union {
    double d;
    struct {unsigned long h, l;} rep;
  } data;
  data.rep.h = 0x7ff00000;
  data.rep.l = 0;
  return data.d;
}
#endif MC68K
