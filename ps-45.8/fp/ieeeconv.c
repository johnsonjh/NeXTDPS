/*
  ieeeconv.c

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

Original version: Ed Taft: Thu Mar 31 16:46:04 1988
Edit History:
Ed Taft: Sun May  1 11:33:25 1988
End Edit History.

This module is included only in configurations for architectures whose
floating point format is NOT IEEE.

Note: this implementation is intended only as a model and as a stopgap
for quick prototyping in a new development environment. The operations
defined here are generally much more easily implemented in assembly
language than in C.
*/

#include PACKAGE_SPECS
#include FP
#include "os_math.h"

/* The following macro enables a test program to include both this
   generic implementation and a machine dependent implementation
   whose results are to be compared. */

#ifdef DEBUG
#define PROC(name) CAT(GENERIC,name)
#else DEBUG
#define PROC(name) name
#endif DEBUG

private procedure IEEEToNative(from, to)
  FloatRep *from; real *to;
/* This does the actual conversion from an IEEE float in native order */
  {
  integer exp, frac;
  exp = from->ieee.exponent;
  frac = from->ieee.fraction;
  if (exp == 0) {
    if (frac == 0) *to = fpZero;
    else *to = os_ldexp((real) frac, -(126+23));  /* denormalized */
    }
  else if (exp == 255)
    ReportErrno(ERANGE);  /* infinity or NAN, unrepresentable */
  else
    *to = os_ldexp((real) (frac + (1<<23)), exp - (127+23));
  if (from->ieee.sign != 0) *to = -*to;
  }

public procedure PROC(IEEEHighToNative)(from, to)
  FloatRep *from; real *to;
  {
#if SWAPBITS
  FloatRep fromSwapped;
  CopySwap4(from, &fromSwapped);
  from = &fromSwapped;
#endif SWAPBITS
  IEEEToNative(from, to);
  }

public procedure PROC(IEEELowToNative)(from, to)
  FloatRep *from; real *to;
  {
#if !SWAPBITS
  FloatRep fromSwapped;
  CopySwap4(from, &fromSwapped);
  from = &fromSwapped;
#endif !SWAPBITS
  IEEEToNative(from, to);
  }

private procedure NativeToIEEE(from, to)
  real *from; FloatRep *to;
/* This does the actual conversion to an IEEE float in native order */
  {
  integer exp, frac;
  real native;
  native = *from;
  to->ieee.sign = native < 0;
  if (isnan(native)) {exp = 255; frac = (1<<23) - 1;}
  else if (isinf(native)) {exp = 255; frac = 0;}
  else if (native == fpZero) {exp = 0; frac = 0;}
  else {
    if (native < 0) native = -native;
    native = os_frexp(native, &exp);
    if (native >= fpOne) { /* bug in frexp on VAX... */
      native /= 2; exp += 1;}
    frac = (integer) ((1<<24) * native) - (1<<23);
    exp += 126;
    if (exp <= 0) {frac = (frac + (1<<23)) >> (-exp + 1); exp = 0;}
    else if (exp >= 255) {exp = 255; frac = 0;}
    }
  to->ieee.exponent = exp;
  to->ieee.fraction = frac;
  }

public procedure PROC(NativeToIEEEHigh)(from, to)
  real *from; FloatRep *to;
  {
#if SWAPBITS
  FloatRep toSwapped;
  NativeToIEEE(from, &toSwapped);
  CopySwap4(&toSwapped, to);
#else SWAPBITS
  NativeToIEEE(from, to);
#endif SWAPBITS
  }

public procedure PROC(NativeToIEEELow)(from, to)
  real *from; FloatRep *to;
  {
#if !SWAPBITS
  FloatRep toSwapped;
  NativeToIEEE(from, &toSwapped);
  CopySwap4(&toSwapped, to);
#else !SWAPBITS
  NativeToIEEE(from, to);
#endif !SWAPBITS
  }
