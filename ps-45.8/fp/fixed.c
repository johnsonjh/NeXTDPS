/*
  fixed.c

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft, 1984
Edit History:
Ed Taft: Sat Apr 30 15:08:51 1988
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

/* The following macro enables a test program to include both this
   generic implementation and a machine dependent implementation
   whose results are to be compared. */

#ifdef DEBUG
#define PROC(name) CAT(GENERIC,name)
#else DEBUG
#define PROC(name) name
#endif DEBUG

#define fixedScale 65536.0	/* i=15, f=16, range [-32768, 32768) */
#define fracScale 1073741824.0	/* i=1, f=30 , range [-2, 2) */

/* Macros for software conversion between unsigned long and double
   (some compilers don't implement these conversions correctly) */

#ifndef SOFTUDCONV
#define SOFTUDCONV (OS==os_bsd || OS==os_ultrix)
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

public Fixed PROC(fixmul)(x, y)
  Fixed x, y;
  {
  double d = (double) x * (double) y / fixedScale;
  d += (d < 0)? -0.5 : 0.5;
  if (d >= FixedPosInf) return FixedPosInf;
  if (d <= FixedNegInf) return FixedNegInf;
  return (Fixed) d;
  }

public Frac PROC(fracmul)(x, y)
  Frac x, y;
  {
  double d = (double) x * (double) y / fracScale;
  d += (d < 0)? -0.5 : 0.5;
  if (d >= FixedPosInf) return FixedPosInf;
  if (d <= FixedNegInf) return FixedNegInf;
  return (Frac) d;
  }

public Fixed PROC(fxfrmul)(x, y)
  Fixed x; Frac y;
  {return fracmul(x, y);}

public Fixed PROC(fixdiv)(x, y)
  Fixed x, y;
  {
  double d;
  if (y == 0) return (x < 0)? FixedNegInf : FixedPosInf;
  d = (double) x / (double) y * fixedScale;
  d += (d < 0)? -0.5 : 0.5;
  if (d >= FixedPosInf) return FixedPosInf;
  if (d <= FixedNegInf) return FixedNegInf;
  return (Fixed) d;
  }

public Fixed PROC(tfixdiv)(x, y)
  Fixed x, y;
  {
  double d;
  if (y == 0) return (x < 0)? FixedNegInf : FixedPosInf;
  d = (double) x / (double) y * fixedScale;
  if (d >= FixedPosInf) return FixedPosInf;
  if (d <= FixedNegInf) return FixedNegInf;
  return (Fixed) d;
  }

public Fixed PROC(fracratio)(x, y)
  Frac x, y;
  {return fixdiv(x, y);}

public Fixed PROC(ufixratio)(x, y)
  Card32 x, y;
  {
  double dx, dy, d;
  if (y == 0) return FixedPosInf;
  dx = UnsignedToDouble(x);
  dy = UnsignedToDouble(y);
  d = dx / dy * fixedScale + 0.5;
  if (d >= FixedPosInf) return FixedPosInf;
  return (Fixed) d;
  }

public Frac PROC(fixratio)(x, y)
  Fixed x, y;
  {
  double d;
  if (y == 0) return (x < 0)? FixedNegInf : FixedPosInf;
  d = (double) x / (double) y * fracScale;
  d += (d < 0)? -0.5 : 0.5;
  if (d >= FixedPosInf) return FixedPosInf;
  if (d <= FixedNegInf) return FixedNegInf;
  return (Frac) d;
  }

public double PROC(fixtodbl)(x)
  Fixed x;
  {return (double) x / fixedScale;}

public Fixed PROC(dbltofix)(d)
  double d;
  {
  if (d >= FixedPosInf / fixedScale) return FixedPosInf;
  if (d <= FixedNegInf / fixedScale) return FixedNegInf;
  return (Fixed) (d * fixedScale);
  }

public double PROC(fractodbl)(x)
  Frac x;
  {return (double) x / fracScale;}

public Frac PROC(dbltofrac)(d)
  double d;
  {
  if (d >= FixedPosInf / fracScale) return FixedPosInf;
  if (d <= FixedNegInf / fracScale) return FixedNegInf;
  return (Frac) (d * fracScale);
  }

public procedure PROC(fixtopflt)(x, pf)
  Fixed x; float *pf;
  {*pf = (float) x / fixedScale;}

public Fixed PROC(pflttofix)(pf)
  float *pf;
  {
  float f = *pf;
  if (f >= FixedPosInf / fixedScale) return FixedPosInf;
  if (f <= FixedNegInf / fixedScale) return FixedNegInf;
  return (Fixed) (f * fixedScale);
  }

public procedure PROC(fractopflt)(x, pf)
  Frac x; float *pf;
  {*pf = (float) x / fracScale;}

public Frac PROC(pflttofrac)(pf)
  float *pf;
  {
  float f = *pf;
  if (f >= FixedPosInf / fracScale) return FixedPosInf;
  if (f <= FixedNegInf / fracScale) return FixedNegInf;
  return (Frac) (f * fracScale);
  }

public UFrac PROC(fracsqrt)(x)
  UFrac x;
  {
  double d = UnsignedToDouble(x);
  d = os_sqrt(d / fracScale) * fracScale + 0.5;
  return DoubleToUnsigned(d);
  }
