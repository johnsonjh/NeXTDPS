/*
  fixedmach.s

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
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

Original version:
Edit History:
Ed Taft: Fri Apr 29 16:04:16 1988
Leo Hourvitz: Mon Jul 11 23:54:00 1988
pgraff@next 5/30/90 caught rounding bug in pflttofix() fintrzs changed to fints
End Edit History.
*/

|;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
| assembly language fixmul, fixdiv, sqrt, fixratio, fracmul, fracratio,
|	and fracsqrt.
| written by Jerome Coonen, Apple Computer,  8 sep 84.
|	13 sep 84  jtc  added fixratio, facmul, fracratio, and fracsqrt.
|	17 sep 84  Ed Taft  added fixed/float conversions
|	25 sep 84  correct overflow test in fixdiv, per jtc
|	26 sep 84  add ufixratio, per jtc
|	?? ??? 86  Ivor Durham added 68020 implementations of fixmul, fixdiv
|	12 jun 87  Ed Taft merged 68000, 68020 versions; converted to
|		   Sun3.2 floating point runtime routines
|	19 jun 87  Ed Taft re-inserted fixdiv bug fix, per jtc memo of
|		   11/3/86. This was the same as the 25 sep 84 change,
|		   but code and comments were different.
|	 5 Nov 87  Ed Taft completed 68020 implementation; speeded up
|		   fixmul and fixdiv; added tfixdiv.

| all arguments and results are signed except as noted.
| 
| fixed fixmul(x, y)		/* returns x*y */
|	fixed	x,y;
| 
| fixed fixdiv(x, y)		/* returns x/y */
|	fixed	x,y;
| 
| fixed tfixdiv(x, y)		/* returns x/y, truncated instead of rounded */
|	fixed	x,y;
| 
| fracd fixratio(x, y)		/* returns x/y */
|	fixed	x,y;
| 
| fracd fracmul(x, y)		/* returns x*y */
|	fracd	x,y;
| 
| fixed fxfrmul(x, y)		/* returns x*y */
|	fixed x;
|	fracd y;
| 
| fracd fracsqrt(x)		/* returns square root of x */
|	fracd	x;
| 
| fixed fracratio(x, y)		/* returns x/y */
|	fracd	x,y;
|
| fixed ufixratio(x, y)		/* returns x/y */
|	unsigned long int x,y;
|
| double fixtodbl(x)		/* returns double floating value of x */
|	fixed x;
|
| double fractodbl(x)		/* returns double floating value of x */
|	fracd x;
|
| fixed dbltofix(d)		/* returns fixed value of x */
|	double x;
|
| fixed dbltofrac(d)		/* returns fracd value of x */
|	double x;
|
| void fixtopflt(x, pf)		/* stores single float value of x at *pf */
|	fixed x; float *pf;
|
| void fracdtopflt(x, pf)	/* stores single float value of x at *pf */
|	fracd x; float *pf;
|
| fixed pflttofix(pf)		/* returns fixed value of float at *pf */
|	float *pf;
| 
| fracd pflttofix(pf)		/* returns fracd value of float at *pf */
|	float *pf;
| 
| type fixed is 32-bit binary fixed-point consisting of sign,
| 15 integer bits, and 16 fraction bits.
| type fracd is 32-bit binary fixed-point consisting of sign,
| 1 integer bit, and 30 fraction bits.
| 2s complement representation is used for negative numbers.
| 
| exceptional cases:
|	fixed overflow is set to 0x7fffffff when positive, 0x80000000 when
|		negative.
|	division by zero yields 0x7fffffff when x is positive or zero,
|		and 0x80000000 when x is negative.
| 
| calling conventions:
|	arguments are pushed by value, right to left.
|	fixed results are returned in d0.
|	callee pops only the return addesses from the stack.
|;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

#include PACKAGE_SPECS
#include "DEFSmach.h"

	SAVEMASK = 0x3C00	| d2-d5
	RESTMASK = 0x003C
	NSAVED = 4*4

|------------------------------------------------------------------------------

|	68020+68881 Implementations of multiply and divide operations

|
| Fixed fixmul(x, y)
|   Fixed x, y;
|
| Simply compute (x * y) / 2**16 and round the result.
|

ENTRY(fixmul)
	moveml	PARAMX(0),d0/d1		| d0 = x, d1 = y
	mulsl	d0,d0:d1	  	| d0,d1 = x * y

| Round the result. The "halfway" case rounds away from zero, for
| consistency with jtcs 68000 implementation.
	jpl	one			| jump if product is positive
	addl	#0x8000-1,d1		| add 1-epsilon in rounding position
	jcc	three			| jump if no carry
	jra	two

one:	addl	#0x8000,d1		| add 1 in rounding position
	jcc	three			| jump if no carry
two:	addql	#1,d0			| propagate carry
	jvs	five			| jump if overflow

| Desired result is in d0[15,0],d1[31,16]. Now shift it all into d0 and
| check for overflow.
three:	movw	#0x10,d1		| d1.w = count (do not clobber d1 high)
	asll	d1,d0			| Shift instead of swap to get overflow
	bvs	five			| jump if overflow
	swap	d1			| insert low part of fraction
	movw	d1,d0
four:	RET

five:	moveml	PARAMX(0),d0/d1		| retrieve parameters again
	jbsr	MulDivOverflow
	bra	four

|
| Frac fracmul(x, y)
|   Frac x, y;
|
| Simply compute (x * y) / 2**30 and round the result.
|
ENTRY(fracmul)
fracmulent:
	moveml	PARAMX(0),d0/d1		| d0 = x, d1 = y
	mulsl	d0,d0:d1	  	| d0,d1 = x * y

| Round the result. The "halfway" case rounds away from zero, for
| consistency with jtc(apostrophe)s 68000 implementation.
	jpl	fmone			| jump if product is positive
	addl	#0x20000000-1,d1	| add 1-epsilon in rounding position
	jcc	fmthree			| jump if no carry
	jra	fmtwo

fmone:	addl	#0x20000000,d1		| add 1 in rounding position
	jcc	fmthree			| jump if no carry
fmtwo:	addql	#1,d0			| propagate carry
	jvs	fmfive			| jump if overflow

| Desired result is in d0[29,0],d1[31,30]. Now shift it all into d0 and
| check for overflow.
fmthree:	asll	#2,d0			| Shift high part
	bvs	fmfive			| jump if overflow
	roll	#2,d1			| insert low part of fraction
	andw	#0x3,d1
	orw	d1,d0
fmfour:	RET

fmfive:	moveml	PARAMX(0),d0/d1		| retrieve parameters again
	jbsr	MulDivOverflow
	bra	fmfour

	| Determine overflow value from signs of parameters (in d0, d1)

MulDivOverflow:
	eorl	d0,d1			| Get sign of result
	bpl	mdione			| Set error result accordingly
	movl	NegativeError,d0
	rts
mdione:	movl	PositiveError,d0
	rts

PositiveError:
	.long	0x7fffffff
NegativeError:
	.long	0x80000000

| fxfrmul
|
ENTRY(fxfrmul)
	jmp	fracmulent	| fixfrmul(x, y) -- same as fracmul(x, y)

|
| Fixed fixdiv(x, y)
|   Fixed x, y;
|
| Strategy: compute (2*x)/y in order to obtain an extra bit of quotient
| to use for rounding. To accomplish this without overflows, we must
| convert x and y to positive unsigned numbers and manage the signs
| separately. Note that we must really compute (2**17 * x)/y in order
| to obtain a result that has the binary point in the correct place.

ENTRY(fixdiv)
fixdivent:
	moveml	d2/d3,sp@-		| save 2 registers
	movl	PARAMX(8),d0		| d0 = x
	movl	d0,d3			| d3 = x also
	jpl	fdtwo			| make x positive
	negl	d0
fdtwo:	swap	d0			| [d1,d0] = abs(x) * 2**16
	moveq	#0,d1
	movw	d0,d1
	clrw	d0
	lsll	#1,d0			| prescale by 2 for later rounding
	roxll	#1,d1
comdiv:
	movl	PARAMX(12),d2		| d2 = y
	jeq	fdeight			| jump if dividing by zero
	jpl	fdthree			| make y positive
	eorl	d2,d3			| d3 = sign of result
	negl	d2
fdthree:	divul	d2,d1:d0		| [d1,d0]/d2 -> r: d1, q: d0
	jvs	fdeight			| jump if overflow
	lsrl	#1,d0			| undo prescale, shift out round bit
	jcc	fdfive			| jump if round bit clear
	addql	#1,d0			| set -- round up
	jvs	fdeight			| jump if overflow
fdfive:	tstl	d3			| restore correct result sign
	jpl	fdsix
	negl	d0
fdsix:	moveml	sp@+,d2/d3		| restore saved registers
	RET

| Handle overflow or divide by zero
fdeight:	moveml	PARAMX(8),d0/d1		| retrieve parameters again
	jsr	MulDivOverflow		| compute correct signed infinity
	jra	fdsix			| return it

|
| fracratio(x, y) -- same as fixdiv(x, y)
ENTRY(fracratio)
	jmp	fixdivent		

|
| Frac fixratio(x, y)
|   Fixed x, y;
|
| Same as fixdiv except that we compute (x * 2**30)/y. Including the
| prescale required for rounding, this is (x * 2**31)/y

ENTRY(fixratio)
	moveml	d2/d3,sp@-		| save 2 registers
	movl	PARAMX(8),d1		| d1 = x
	movl	d1,d3			| d3 = x also
	jpl	frtwo			| make x positive
	negl	d1
frtwo:	moveq	#0,d0			| [d1,d0] = abs(x) * 2**31
	lsrl	#1,d1
	roxrl	#1,d0
	jra	comdiv			| rest same as fixdiv

|
| Fixed tfixdiv(x, y)
|   Fixed x, y;
|
| Same as fixdiv except that the result is truncated toward zero
| instead of rounded.

ENTRY(tfixdiv)
	movl	PARAMX(0),d1		| d1 = x
	swap	d1			| [d1,d0] = x * 2**16
	movl	d1,d0
	extl	d1
	clrw	d0
	tstl	PARAMX(4)		| test for divide by zero
	jeq	tfdtwo			| jump if so
	divsl	PARAMX(4),d1:d0		| [d1,d0]/y -> d1 = r, d0 = q
	jvs	tfdtwo			| jump if overflow
tfdone:	RET				| unrounded result in d0

tfdtwo:	movl	PARAMX(4),d0		| handle overflow
	jbsr	MulDivOverflow
	jra	tfdone

|
| Fixed ufixratio(x, y)
|   unsigned long int x, y;
|
| Same as fixdiv except that we treat both operands as positive.

ENTRY(ufixratio)
	movl	d2,sp@-			| save d2
	movl	PARAMX(4),d0		| d0 = x
	swap	d0			| [d1,d0] = x * 2**16
	moveq	#0,d1
	movw	d0,d1
	clrw	d0
	lsll	#1,d0			| prescale by 2 for later rounding
	roxll	#1,d1
	movl	PARAMX(8),d2		| d2 = y
	jeq	ufreight		| jump if dividing by zero
	divul	d2,d1:d0		| [d1,d0]/d2 -> r: d1, q: d0
	jvs	ufreight		| jump if overflow
	lsrl	#1,d0			| undo prescale, shift out round bit
	jcc	ufrsix			| jump if round bit clear
	addql	#1,d0			| set -- round up
	jvs	ufreight		| jump if overflow
ufrsix:	movl	sp@+,d2			| restore d2
	RET

| Handle overflow or divide by zero
ufreight:	movl	PositiveError,d0| always return positive infinity
	jra	ufrsix


| End of 68020 operation implementations



|;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
| square root of a frac xx.xxxxxxxx .  in this case, the leading two
| bits of the fract are both taken to be integer, that is, the value is
| interpreted as unsigned.
|;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ENTRY(fracsqrt)
	moveml	#SAVEMASK,sp@-
	movl	PARAMX(NSAVED),d3 | fetch input frac

|;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
| know that lead bit of root is 0, so only 31 bits plus a round bit are
| required.  alas, this just spills over a longword, so compute in a
| 64-bit field.  compute root into d0-d1.  maintain radicand in d2-d3.
| loop counter in d4.  before each step, the root has the form:
| <current root>01  and after each step: <new root>1, where the number
| shown spans d0 and the highest two bits of d1.
|;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	moveq	#0,d0
	moveq	#1,d1
	rorl	#2,d1		| d1 := 4000 0000
	moveq	#0,d2		| clear high radicand
	moveq	#31,d4		| need 32 bits less 1-dbra
		
frsqrtloop:
|;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
| try radicand minus trial root.  use subtle fact that c is the complement
| of the next root bit at each step.
|;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	subl	d1,d3
	subxl	d0,d2		| no carry means root=1
	jcc	fs72
	
	addl	d1,d3
	addxl	d0,d2
fs72:
	eorb	#16,cc		| complement x bit
	addxl	d0,d0		| set root bit while shifting

	addl	d3,d3		| shift radicand two places
	addxl	d2,d2
	addl	d3,d3
	addxl	d2,d2
	
	dbra	d4,frsqrtloop
	
|;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
| shift the 32-bit root in d0 right one bit and round.
|;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	lsrl	#1,d0
	jcc	fs79
	addql	#1,d0
fs79:	moveml	sp@+,#RESTMASK	| restore registers
	RET

|;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
| fixed/float and fracd/float conversions
|;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ENTRY(fixtodbl)
	moveq #0,d1		| in case zero
	movl PARAM,d0		| get the fixed arg
	jeq ftdone			| return zero for zero
|	jbsr FVSUBR(fltd)	| treat as int and convert to double
	fmovel d0,fp0		| Leo 12Jul88 Inline float conversion
	fmoved fp0,sp@-
	moveml sp@+,#0x3
	subl #16*0x100000,d0	| divide by 2^16
ftdone:	RET

ENTRY(fractodbl)
	moveq #0,d1		| in case zero
	movl PARAM,d0		| get the fracd arg
	jeq ftd1			| return zero for zero
|	jbsr FVSUBR(fltd)	| treat as int and convert to double
	fmovel d0,fp0		| Leo 12Jul88 Inline float conversion
	fmoved fp0,sp@-
	moveml sp@+,#0x3
	subl #30*0x100000,d0	| divide by 2^30
ftd1:	RET

ENTRY(dbltofix)
	movl PARAM,d0		| get high part of double arg
	movl d0,d1		| make a copy
	roll #1,d1		| left-justify exponent
	cmpl #(1023+15)*0x200000,d1  | in range for fixed?
	jcc toobig
| just do the conversion; no need to test for zero since the "multiply"
| will produce a double very much smaller than 1, so zero comes out anyway.
	addl #16*0x100000,d0	| multiply by 2^16
	movl d0,PARAM		| Leo 12Jul88 Inline fp work
	fintd PARAM,fp0
	fmovel fp0,d0
|	movl PARAM2,d1		| get low part of double arg
|	jbsr FVSUBR(intd)	| convert to int and treat as fixed
	RET

ENTRY(dbltofrac)
	movl PARAM,d0		| get high part of double arg
	movl d0,d1		| make a copy
	roll #1,d1		| left-justify exponent
	cmpl #(1023+1)*0x200000,d1  | in range for fracd?
	jcc toobig
| just do the conversion; no need to test for zero since the "multiply"
| will produce a double very much smaller than 1, so zero comes out anyway.
	addl #30*0x100000,d0	| multiply by 2^30
	movl d0,PARAM		| Leo 12Jul88 Inline fp work
	fmoved PARAM,fp0
	fmovel fp0,d0
|	movl PARAM2,d1		| get low part of double arg
|	jbsr FVSUBR(intd)	| convert to int and treat as fracd
	RET

ENTRY(fixtopflt)
	movl PARAM,d0		| get the fixed arg
	jeq ftp1			| return zero for zero
|	jbsr FVSUBR(flts)	| treat as int and convert to float
	fmovel d0,fp0		| Inline fp instr Leo 12Jul88
	fmoves fp0,d0
	subl #16*0x800000,d0	| divide by 2^16
ftp1:	movl PARAM2,a0		| where to put result
	movl d0,a0@
	RET

ENTRY(fractopflt)
	movl PARAM,d0		| get the fracd arg
	jeq ftpone			| return zero for zero
|	jbsr FVSUBR(flts)	| treat as int and convert to float
	fmovel d0,fp0		| Leo 12Jul88
	fmoves fp0,d0
	subl #30*0x800000,d0	| divide by 2^30
ftpone:	movl PARAM2,a0		| where to put result
	movl d0,a0@
	RET

ENTRY(pflttofix)
	movl PARAM,a0		| pointer to float arg
	movl a0@,d0		| get float
	movl d0,d1		| make a copy
	roll #1,d1		| left-justify exponent
	cmpl #(127+15)*0x1000000,d1  | in range for fixed?
	jcc toobig
| just do the conversion; no need to test for zero since the "multiply"
| will produce a double very much smaller than 1, so zero comes out anyway.
	addl #16*0x800000,d0	| multiply by 2^16
|	jbsr FVSUBR(ints)	| convert to int and treat as fixed
	fmoves d0,fp0		| Leo 12Jul88 Inline fp
	fmovel fp0,d0
	RET

ENTRY(pflttofrac)
	movl PARAM,a0		| pointer to float arg
	movl a0@,d0		| get float
	movl d0,d1		| make a copy
	roll #1,d1		| left-justify exponent
	cmpl #(127+1)*0x1000000,d1  | in range for fracd?
	jcc toobig
| just do the conversion; no need to test for zero since the "multiply"
| will produce a double very much smaller than 1, so zero comes out anyway.
	addl #30*0x800000,d0	| multiply by 2^30
|	jbsr FVSUBR(ints)	| convert to int and treat as fracd
	fmoves d0,fp0		| Leo 12Jul88 Inline fp
	fmovel fp0,d0
	RET

toobig:	moveq #1,d0		| isolate sign now in bit 0
	andl d1,d0
	addl #0x7FFFFFFF,d0	| positive => 7FFFFFFF, negative => 80000000
	RET
