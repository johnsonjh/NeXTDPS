/*
  bvaluemach.s

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
Ed Taft: Sun May  1 15:24:21 1988
Leo Hourvitz: Mon Jun 27 1988
End Edit History.
*/

| Fast set block to value
| derived from bzerosun.s

#include PACKAGE_SPECS
#include "DEFSsun.h"

| Set block of storage to an arbitrary byte value
| Usage: bvalue(addr, length, value)
	.globl	_os_bvalue
_os_bvalue:
	LINK
	movb	PARAMX(11),d1	| d1 = value
	movb	d1,d0		| copy value to all 4 bytes of d1
	lslw	#8,d1
	movb	d0,d1
	movw	d1,d0
	swap	d1
	movw	d0,d1
	movl	PARAM,a1	| address
	movl	PARAM2,d0	| length
	jle	five		| do nothing if length <= 0
	btst	#0,PARAMX(3)	| odd address?
	jeq	one		| no, skip
	movb	d1,a1@+		| do one byte
	subql	#1,d0		| to adjust to even address
one:	movl	d0,a0		| save possibly adjusted count

| Decide whether the block is large enough to amortize the setup overhead
| for the fastest loop (moveml/dbra).
	cmpl	#239,d0		| approx. break-even point on 68010
	jlt	nine		| jump if too small

| Do most of block with moveml, 16 longs (64 bytes) at a time. Must work
| from high to low addresses because moveml to memory does not allow
| autoincrement addressing. Approximate overhead time: 258 clocks.
	moveml	#0x3f30,sp@-	| save d2..d7, a2..a3
	movl	d1,d2		| d1..d7, a3 will be used as source
	movl	d1,d3
	movl	d1,d4
	movl	d1,d5
	movl	d1,d6
	movl	d1,d7
	movl	d1,a3
	andw	#0xffc0,d0	| round count down to multiple of 64 bytes
	subl	d0,a0		| compute remainder
	addl	d0,a1		| adjust address to end of block
	movl	a1,a2		| make working copy (a1 needed later)
	lsrl	#6,d0		| div by 64 to get block count
	jra	eight
| 64-byte block-at-a-time loop (does not run in loop mode).
| Timing: 8 clocks per long + 26 per block of 16 longs
| = 9.68 clocks per long = 2.42 clocks per byte.
seven:	moveml	#0x7f10,a2@-	| set 8 longs; decr a2 by 32
	moveml	#0x7f10,a2@-	| likewise, for total of 16 longs or 64 bytes
eight:	dbra	d0,seven
| If the count was greater than 2^16-1 we may not be done yet,
| because dbra decrements and tests only the low 16 bits (yuk). This happens
| only for truly huge blocks (>4 megabytes), but better safe than sorry!
	subl	#0x10000,d0	| propagate the borrow that dbra lost
	jge	seven		| continue loop if count remaining
	moveml	sp@+,#0x0cfc	| restore d2..d7, a2..a3
	movl	a0,d0		| recover remaining count

| Do remainder of block with some combination of single long and byte moves
nine:	lsrl	#2,d0		| get count of longs
	jra	three		| go to loop test
| Here is the fast inner loop.
| 68000 timing: 22 clocks per long = 5.5 clocks per byte.
| 68010 timing (loop mode): 14 clocks per long = 3.5 clocks per byte.
two:	movl	d1,a1@+		| store long
three:	dbra	d0,two		| decr count; br until done
| Now up to 3 bytes remain to be set
	movw	a0,d0		| restore count
	andw	#3,d0		| mod 4
	jra	six
| Byte-at-a-time loop, executed for at most 3 iterations
four:	movb	d1,a1@+		| do a byte
six:	dbra	d0,four
five:	RET			| all done
