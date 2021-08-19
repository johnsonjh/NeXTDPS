/* fixed.c

	      Copyright 1984, '87 -- Adobe Systems, Inc.
	    PostScript is a trademark of Adobe Systems, Inc.
NOTICE:  All information contained herein or attendant hereto is, and
remains, the property of Adobe Systems, Inc.  Many of the intellectual
and technical concepts contained herein are proprietary to Adobe Systems,
Inc. and may be covered by U.S. and Foreign Patents or Patents Pending or
are protected as trade secrets.  Any dissemination of this information or
reproduction of this material are strictly forbidden unless prior written
permission is obtained from Adobe Systems, Inc.

Original version: Ed Taft, 1984
Edit History:
Ed Taft: Tue Nov 10 10:43:32 1987
Doug Brotz: Fri Sep 16 10:26:28 1988
End Edit History.

Fixed point multiply, divide, and conversions -- machine independent.
See fp.h for the specification of what these procedures do.

The double-to-int conversion is assumed to truncate rather than round;
this is specified by the C language manual. The direction of truncation
is machine-dependent, but is toward zero rather than toward minus
infinity on the Vax and Sun. This explains the peculiar way in which
fixmul and fixdiv do rounding.
*/

#include PACKAGE_SPECS
#include FP

#define fixedScale 65536.0	/* i=15, f=16, range [-32768, 32768) */
#define fracScale 1073741824.0	/* i=1, f=30 , range [-2, 2) */

/* Macros for software conversion between unsigned long and double
   (some compilers don't implement these conversions correctly) */

#ifndef SOFTUDCONV
#define SOFTUDCONV (ISP==isp_vax && OS==os_bsd)
#endif

#if SOFTUDCONV
#define UnsignedToDouble(u) \
  ((u > (Card32) MAXinteger)? \
    (double) (u - (Card32) MAXinteger - 1) + (double) MAXinteger + 1.0 : \
    (double) u)

#define DoubleToUnsigned(d) \
  ((d >= (double) MAXinteger + 1.0)? \
    (Card32) (d - (double) MAXinteger - 1.0) + (Card32) MAXinteger + 1 : \
    (Card32) d)

#else SOFTUDCONV
#define UnsignedToDouble(u) ((double) u)
#define DoubleToUnsigned(d) ((Card32) d)
#endif SOFTUDCONV

public double fixtodbl(x)	/* converts fixed x to double */
  Fixed x;
  {return (double) x / fixedScale;}

public Fixed dbltofix(d)	/* converts double d to fixed */
  double d;
  {
  if (d >= FixedPosInf / fixedScale) return FixedPosInf;
  if (d <= FixedNegInf / fixedScale) return FixedNegInf;
  return (Fixed) (d * fixedScale);
  }

public double fractodbl(x)	/* converts frac x to double */
  Frac x;
  {return (double) x / fracScale;}

public Frac dbltofrac(d)	/* converts double d to frac */
  double d;
  {
  if (d >= FixedPosInf / fracScale) return FixedPosInf;
  if (d <= FixedNegInf / fracScale) return FixedNegInf;
  return (Frac) (d * fracScale);
  }

public procedure fractopflt(x, pf) /* converts x to float and stores at *pf */
  Frac x; float *pf;
  {*pf = (float) x / fracScale;}



