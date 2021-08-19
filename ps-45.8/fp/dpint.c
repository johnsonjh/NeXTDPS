/* dpint.c -- double-precision integer operations

Copyright (c) 1987, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of
Adobe Systems Incorporated.  Many of the intellectual and
technical concepts contained herein are proprietary to Adobe,
are protected as trade secrets, and are made available only to
Adobe licensees for their internal use.  Any reproduction or
dissemination of this software is strictly forbidden unless
prior written permission is obtained from Adobe.

Original version: Ed Taft: Fri Nov  6 12:42:08 1987
Edit History:
Ed Taft: Tue Apr  5 13:26:54 1988
End Edit History.

Note: this implementation is intended only as a model and as a stopgap
for quick prototyping in a new development environment. The operations
defined here are generally much more easily implemented in assembly
language than in C.

Note: this implementation depends on two's complement representation
for all integer types.
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

typedef union {
  Card32 val;
#if SWAPBITS
  struct {Card16 l, h;} rep;
#else SWAPBITS
  struct {Card16 h, l;} rep;
#endif SWAPBITS
} Card32Rep;

public procedure PROC(dpneg)(pa, pResult)
  PInt64 pa, pResult;
  {
  if ((pResult->l = - (Int32) pa->l) == 0) pResult->h = - pa->h;
  else pResult->h = ~ pa->h;
  }

public procedure PROC(dpadd)(pa, pb, pResult)
  PInt64 pa, pb, pResult;
  {
  Card32 t = pa->l + pb->l;
  pResult->h = pa->h + pb->h;
  if (t < pa->l || t < pb->l) pResult->h++;
  pResult->l = t;
  }

public procedure PROC(dpsub)(pa, pb, pResult)
  PInt64 pa, pb, pResult;
  {
  Card32 t = pa->l - pb->l;
  pResult->h = pa->h - pb->h;
  if (pa->l < pb->l || pa->l < t) pResult->h--;
  pResult->l = t;
  }

public procedure PROC(dpmul)(a, b, pResult)
  Int32 a, b; PInt64 pResult;
  {
  boolean negative = false;
  Card32Rep ua, ub, ph, pl, px;

  /* Make operands positive and compute result sign */
  if (a < 0) {ua.val = -a; negative = true;}
  else ua.val = a;
  if (b < 0) {ub.val = -b; negative = !negative;}
  else ub.val = b;

  /* Compute product of ua and ub by performing four 16 x 16 multiples
     and then combining terms, being careful not to allow a carry out
     at any time. Resulting product is in [ph,pl]. */
  ph.val = ua.rep.h * ub.rep.h;	/* high product */
  pl.val = ua.rep.l * ub.rep.l;	/* low product */
  px.val = ua.rep.h * ub.rep.l	/* cross product <= 2**31 - 2**15 */
	 + ua.rep.l * ub.rep.h	/* ditto; however, it's impossible for both
				   products to = 2**31 - 2**15 simultaneously,
				   so sum < 2**32 - 2**16 */
	 + pl.rep.h;		/* carry from low product; sum < 2**32 */
  pl.rep.h = px.rep.l;		/* merge cross products into result */
  ph.val += px.rep.h;

  /* Apply sign and store the result */
  if (negative) {
    pResult->h = ~ ph.val;
    if ((pResult->l = - (Int32) pl.val) == 0) pResult->h++;
    }
  else {
    pResult->h = ph.val;
    pResult->l = pl.val;
    }
  }

public Int32 PROC(dpdiv)(pa, b, round)
  PInt64 pa; Int32 b; boolean round;
  {
  boolean negative = false;
  Card32Rep uq;
  register Card32 r, q, d;
  register Int16 i;

  /* Make operands positive and compute result sign */
  if (pa->h < 0) {
    r = ~ pa->h;
    if ((q = - pa->l) == 0) r++;
    negative = true;
    }
  else {
    r = pa->h;
    q = pa->l;
    }
  if (b < 0) {d = -b; negative = !negative;}
  else d = b;

  /* Now we are to compute [r,q]/d as unsigned numbers, where
     [r,q] <= 2**62 and d <= 2**31. First, prescale the dividend
     by 2; this makes it easy to test in advance whether the
     division is possible and helps later with rounding.
     Note: this test treats the case of 2**62 / 2**31 as an overflow;
     this is OK since the case arises only when both operands are
     negative; the result is therefore positive and unrepresentable. */
  r <<= 1; if ((Int32) q < 0) r++; q <<= 1;  /* 64-bit left shift */
  if (r >= d) goto overflow;

  /* See whether d < 2**16 */
  if (d < 0x10000)
    {
    /* Compute [r,q]/d by simple short divisions. We can ignore
       the high half of r since we know it is zero. */
    uq.val = q;
    r = (r << 16) + uq.rep.h;
    q = r / (Card16) d;
    r = ((r - ((Card16) q * (Card16) d)) << 16) + uq.rep.l;
    q = (q << 16) + (r / (Card16) d);
    /* We now have double the quotient; now either round or truncate */
    if (round && (q & 1) != 0)
      {q >>= 1; if ((Int32) ++q < 0) goto overflow;}
    else q >>= 1;
    }
  else
    {
    /* Compute [r,q]/d by performing 32 subtract and shift steps.
       At each step, a quotient bit is shifted into the low bit of q
       and the next dividend bit is shifted from the high bit of q
       into the low bit of r. Note: the first step always produces
       a zero quotient bit that is not significant. */
    i = 32;
    do {
      if (r >= d)
	{r -= d; r <<= 1; if ((Int32) q < 0) r++; q <<= 1; q++;}
      else
	{r <<= 1; if ((Int32) q < 0) r++; q <<= 1;}
      }
    while (--i > 0);

    /* We now have the quotient, properly scaled, in q and double the
       remainder in r; round if desired by inspecting the remainder. */
    if (round && r >= d)
      if ((Int32) ++q < 0) goto overflow;
    }

  /* Apply sign and return the result */
  return (negative)? - (Int32) q : q;

  overflow:
    return (negative)? MINinteger : MAXinteger;
  }

public Int32 PROC(muldiv)(a, b, c, round)
  Int32 a, b, c; boolean round;
  {
  Int64 prod;

  dpmul(a, b, &prod);
  return dpdiv(&prod, c, round);
  }
