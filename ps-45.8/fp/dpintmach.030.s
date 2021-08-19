/*
  dpintmach.s

Copyright (c) 1987, 1988 Adobe Systems Incorporated.
All rights reserved.

CONFIDENTIAL
Copyright (c) 1988 NeXT, Inc. as an unpublished work.  All Rights Reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Fri Nov  6 12:42:08 1987
Edit History:
Ed Taft: Fri Apr 29 16:01:13 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include "DEFSmach.h"


| public procedure dpneg(pa, pResult)
|   PInt64 pa, pResult;

ENTRY(dpneg)
	moveml	PARAMX(0),a0/a1		| a0 = pa, a1 = pResult
	movl	a0@+,d0			| d0 = pa->h
	movl	a0@,d1			| d1 = pa->l
	negl	d1			| negate [d1,d0]
	negxl	d0
	movl	d0,a1@+			| store result
	movl	d1,a1@
	RET


| public procedure dpadd(pa, pb, pResult)
|   PInt64 pa, pb, pResult;

ENTRY(dpadd)
	moveml	PARAMX(0),a0/a1		| a0 = pa, a1 = pb
	movl	a0@+,d0			| d0 = pa->h
	movl	a0@,d1			| d1 = pa->l
	addl	a1@(4),d1		| d1 = pa->l + pb->l
	movl	PARAMX(8),a0		| a0 = pResult
	movl	d1,a0@(4)		| pResult->l = d1
	movl	a1@,d1			| d1 = pb->h
	addxl	d1,d0			| d0 = pa->h + pb->h + carry
	movl	d0,a0@			| pResult->h = d0
	RET


| public procedure dpsub(pa, pb, pResult)
|   PInt64 pa, pb, pResult;

ENTRY(dpsub)
	moveml	PARAMX(0),a0/a1		| a0 = pa, a1 = pb
	movl	a0@+,d0			| d0 = pa->h
	movl	a0@,d1			| d1 = pa->l
	subl	a1@(4),d1		| d1 = pa->l - pb->l
	movl	PARAMX(8),a0		| a0 = pResult
	movl	d1,a0@(4)		| pResult->l = d1
	movl	a1@,d1			| d1 = pb->h
	subxl	d1,d0			| d0 = pa->h - pb->h - borrow
	movl	d0,a0@			| pResult->h = d0
	RET

#if ISP==isp_mc68020
| 68020 implementations of multiply/divide operations

| public procedure dpmul(a, b, pResult)
|   Int32 a, b; PInt64 pResult;

ENTRY(dpmul)
	moveml	PARAMX(0),d0/d1/a0	| d0 = a, d1 = b, a0 = pResult
	mulsl	d1,d1:d0		| [d1,d0] = b * a
	movl	d1,a0@+			| pResult->h = d1
	movl	d0,a0@			| pResult->l = d0
	RET


| public Int32 dpdiv(pa, b, round)
|   PInt64 pa; Int32 b; boolean round;
|
| Strategy: 2 distinct cases, one when round is false and one when true.

ENTRY(dpdiv)
	lea	PARAMX(0),a1		| a1 -> pa
	movl	a1@+,a0			| a0 = pa, a1 -> b
	movl	a0@+,d1			| d1 = pa->h
	movl	a0@,d0			| d0 = pa->l

| Tail of code shared with muldiv.
| [d1,d0] = double precision dividend
| a1 -> divisor, and the next argument is round.
dpdivTail:
	tstl	a1@(4)			| are we to round?
	jne	one			| jump if so

| Not rounding. A simple divsl does it all.
	tstl	a1@			| see whether divisor is zero
	jeq	nine			| jump if so
	divsl	a1@,d1:d0		| [d1,d0]/b -> r: d1, q: d0
	jvs	nine			| jump if overflow
	RET

nine:	eorl	d1,a1@			| overflow, compute sign of result
	jra	ten			| go handle it

| Rounding. Compute (2*a)/b in order to obtain an extra bit of quotient
| to use for rounding. To accomplish this without overflows, we must
| convert a and b to positive unsigned numbers and manage the signs
| separately.
one:	moveml	d2/d3,sp@-		| save 2 registers
	movl	d1,d3			| d3 = pa->h also
	jpl	two			| jump if positive
	negl	d0			| make [d1,d0] positive
	negxl	d1
two:	movl	a1@,d2			| d2 = b
	jeq	eight			| jump if dividing by zero
	jpl	three			| jump if positive
	eorl	d2,d3			| d3 = sign of result
	negl	d2			| make b positive
three:	lsll	#1,d0			| prescale by 2 for later rounding
	roxll	#1,d1
	jcs	eight			| carry out means certain overflow
	divul	d2,d1:d0		| [d1,d0]/d2 -> r: d1, q: d0
	jvs	eight			| jump if overflow
	lsrl	#1,d0			| undo prescale, shift out round bit
	jcc	five			| jump if round bit clear
	addql	#1,d0			| set -- round up
	jvs	eight			| jump if overflow
five:	tstl	d3			| restore correct result sign
	jpl	six
	negl	d0
six:	moveml	sp@+,d2/d3		| restore saved registers
	RET

eight:	tstl	d3			| overflow, get sign of result
	moveml	sp@+,d2/d3		| restore saved registers

| Handle overflow or divide by zero. Condition codes set for sign of
| result when we get here.
ten:	jmi	eleven			| jump if negative
	movl	#0x7fffffff,d0		| return positive infinity
	RET

eleven:	movl	#0x80000000,d0		| return negative infinity
	RET


| public Int32 muldiv(a, b, c, round)
|   Int32 a, b, c; boolean round;
|
| Strategy: just do the multiply in-line, then jump to the tail of dpdiv.

ENTRY(muldiv)
	lea	PARAMX(0),a1		| a1 -> a
	moveml	a1@+,d0/d1		| d0 = a, d1 = b, a1 -> c
	mulsl	d1,d1:d0		| [d1,d0] = b * a
	jra	dpdivTail		| rest same as dpdiv

#else ISP==isp_mc68020
| 68000 implementations of multiply/divide operations

| public procedure dpmul(a, b, pResult)
|   Int32 a, b; PInt64 pResult;

ENTRY(dpmul)
	moveml	d2-d4,sp@-		| save 3 registers
	moveml	PARAMX(12),d0/d1/a0	| d0 = a, d1 = b, a0 = pResult
	jbsr	dpmulSub		| convert signs and do multiply
	tstl	d4			| restore correct result sign
	jpl	Three
	negl	d0
	negxl	d1
Three:	movl	d1,a0@+			| pResult->h = d1
	movl	d0,a0@			| pResult->l = d0
	moveml	sp@+,d2-d4		| restore saved registers
	RET

| Double precision multiply subroutine, shared with muldiv.
| On entry: d0 = a, d1 = b
| On exit: [d1,d0] = magnitude of result, d4 = sign of result,
|   Z condition code reflects entire 64-bit result.
| Clobbers d2-d4

dpmulSub:
	movl	d0,d4			| d4 = a also
	jpl	ONE			| jump if positive
	negl	d0			| make a positive
ONE:	tstl	d1			| test sign of b
	jpl	TWO			| jump if positive
	eorl	d1,d4			| d4 = sign of result
	negl	d1			| make b positive
TWO:	movl	d0,d2
	swap	d2
	mulu	d1,d2			| d2 = a.h * b.l
	movl	d1,d3
	swap	d3
	mulu	d0,d3			| d3 = a.l * b.h
	addl	d3,d2			| d2 = sum of cross products;
					| no carry is possible
	movw	d0,d3
	mulu	d1,d3			| d3 = a.l * b.l
	swap	d0
	swap	d1
	mulu	d0,d1			| d1 = a.h * b.h
	swap	d2			| d2 = [low, high cross product sum]
	movl	d2,d0
	clrw	d0			| d0 = [low cross product sum, 0]
	subl	d0,d2			| d2 = [0 , high cross product sum]
	addl	d3,d0			| combine with low product
	addxl	d2,d1			| combine with high product, carry
	rts


| public Int32 dpdiv(pa, b, round)
|   PInt64 pa; Int32 b; boolean round;
|
| Strategy: 2 distinct cases, one when b < 2**16 and one when b >= 2**16

ENTRY(dpdiv)
	moveml	d2-d4,sp@-		| save 3 registers
	lea	PARAMX(12),a1		| a1 -> pa
	movl	a1@+,a0			| a0 = pa, a1 -> b
	movl	a0@+,d1			| d1 = pa->h
	movl	a0@,d0			| d0 = pa->l
	movl	d1,d4			| d4 = pa->h also
	jpl	TWOTWO			| jump if positive
	negl	d0			| make [d1,d0] positive
	negxl	d1

| Tail of code shared with muldiv.
| [d1,d0] = double precision dividend, converted to unsigned form
| d4 = sign of dividend
| a1 -> divisor, and the next argument is round.
| First, convert a and b to positive unsigned numbers and manage the
| signs separately.
TWOTWO:
dpdivTail:
	movl	a1@,d2			| d2 = b
	jeq	EIGHT			| jump if dividing by zero
	jpl	THREE			| jump if positive
	eorl	d2,d4			| d4 = sign of result
	negl	d2			| make b positive

| Now we are to compute [d1,d0]/d2 as unsigned numbers, where
| [d1,d0] <= 2**62 and d2 <= 2**31. First, prescale the dividend
| by 2; this makes it easy to test in advance whether the
| division is possible and helps later with rounding.
THREE:	lsll	#1,d0			| prescale dividend by 2
	roxll	#1,d1
	jcs	EIGHT			| carry out means certain overflow
	cmpl	d2,d1			| compare divisor with high dividend
	jcc	EIGHT			| divisor < high dividend => overflow
	cmpl	#0x10000,d2		| check magnitude of divisor
	jcc	FOUR			| jump if divisor >= 2**16

| Divisor < 2**16. Compute [d1,d0]/d2 by simple short divisions.
| We refer to the dividend as 4 16-bit parts: a.h.h, a.h.l, a.l.h, a.l.l.
| We can ignore a.h.h since we know it is zero.
	movl	d0,d3
	movw	d1,d0
	swap	d0			| d0 = [a.h.l, a.l.h]
	divu	d2,d0			| d0/d2 -> d0 = [r.h, q.h]
	movl	d0,d1
	movw	d3,d1			| d1 = [r.h, a.l.l]
	divu	d2,d1			| d1/d2 -> d1 = [r.l, q.l]
	swap	d0
	movw	d1,d0			| combine q.h, q.l

| We now have double the quotient in d0. Round or truncate as appropriate.
	lsrl	#1,d0			| undo prescale, shift out round bit
	jcc	FIVE			| jump if round bit clear
	tstl	a1@(4)			| set; are we to round?
	jeq	FIVE			| jump if not
	addql	#1,d0			| yes -- round up
	jvs	EIGHT			| jump if overflow
	jra	FIVE

| Divisor >= 2**16. Compute [d1,d0]/d2 by performing 32 subtract and
| shift steps. At each step, a quotient bit is shifted into the low bit
| of d0 and the next dividend bit is shifted from the high bit of d0
| into the low bit of d1. Note: the first step always produces a zero
| quotient bit that is not significant.
FOUR:	moveq	#32-1,d3		| step count -1 for dbra
FOUR1:	cmpl	d2,d1			| trial subtract dividend - divisor
	jcc	FOUR2			| jump if positive result
	addl	d0,d0			| shift 0 bit into quotient
	addxl	d1,d1			| shift in next dividend bit
	dbra	d3,FOUR1		| repeat until count exhausted
	jra	FOUR3

FOUR2:	subl	d2,d1			| actually do the subtract
	addl	d0,d0			| shift quotient
	addxl	d1,d1			| shift in next dividend bit
	addql	#1,d0			| set quotient bit to 1
	dbra	d3,FOUR1		| repeat until count exhausted

| We now have the quotient, properly scaled, in d0 and double the
| remainder in d1. Round if desired by inspecting the remainder.
FOUR3:	tstl	a1@(4)			| are we to round?
	jeq	FIVE			| jump if not
	cmpl	d2,d1			| what would next quotient bit be?
	jcs	FIVE			| jump if zero
	addql	#1,d0			| one -- round up
	jvs	EIGHT			| jump if overflow

| Restore sign and exit
FIVE:	tstl	d4			| restore correct result sign
	jpl	SIX
	negl	d0
SIX:	moveml	sp@+,d2-d4		| restore saved registers
	RET

| Handle overflow or divide by zero. d4 = sign of result.
EIGHT:	tstl	d4			| get sign of result
	jmi	ELEVEN			| jump if negative
	movl	#0x7fffffff,d0		| return positive infinity
	jra	SIX

ELEVEN:	movl	#0x80000000,d0		| return negative infinity
	jra	SIX


| public Int32 muldiv(a, b, c, round)
|   Int32 a, b, c; boolean round;

ENTRY(muldiv)
	moveml	d2-d4,sp@-		| save 3 registers
	lea	PARAMX(12),a1		| a1 -> a
	moveml	a1@+,d0/d1		| d0 = a, d1 = b, a1 -> c
	jbsr	dpmulSub		| convert signs and do multiply
	jne	dpdivTail		| rest same as dpdiv if nonzero
	clrl	d4			| correct sign of zero
	jra	dpdivTail		| rest same as dpdiv

#endif ISP==isp_mc68020
