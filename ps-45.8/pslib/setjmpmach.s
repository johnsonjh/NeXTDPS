/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)_setjmp.s	5.5 (Berkeley) 3/9/86"
#endif LIBC_SCCS

/*
	Leo Hourvitz, 27Jun88
	There are two uses of setjmp/longjmp in PostScript.
	The coroutine package uses the routines _setjmp and
	_longjmp, and the exception handling code uses
	setjmp and longjmp.
	
	In reality, we want both of those to be references
	to _setjmp and _longjmp, which are the ones that
	don't make system calls.  Also, the coroutine
	mechanism needs to be able to longjmp to
	places that are lower on the stack than the current
	sp; that's why we override the normal bsd implementations.
	
	*/

/*
 * C library -- _setjmp, _longjmp
 *
 *	_longjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	_setjmp(a)
 * by restoring registers from the stack,
 * The previous signal state is NOT restored.
 */

#include "cframe.h"
#include <setjmp.h>

.file_definition_libsys_s.B.shlib__setjmp.o = 0
.globl .file_definition_libsys_s.B.shlib__setjmp.o
.file_definition_libsys_s.B.shlib_setjmp.o = 0
.globl .file_definition_libsys_s.B.shlib_setjmp.o

	.globl	__setjmp		| Do just one name, other on link line
__setjmp:
	movl	a_p0,a0			| address of jmpbuf
	movl	a_ra,a0@(JB_PC*4)	| return address
	lea	a_p0,a1
	movl	a1,a0@(JB_SP*4)		| callers sp
	movl	#JB_MAGICNUM,a0@(JB_MAGIC*4)
	moveml	#0x7cfc,a0@(JB_D2*4)	| save d2-d7, a2-a6
#ifndef FP_BUG
#ifdef __GNU__
	.long	0xf228bc00
	.word	0x8c
	.long	0xf228f03f
	.word	0x44
#else !__GNU__
	fmovem	fpc/fps/fpi,a0@(JB_FPCR*4)
	fmovem	fp2/fp3/fp4/fp5/fp6/fp7,a0@(JB_FP2*4)
#endif !__GNU_
#endif FP_BUG
	moveq	#0,d0
	rts

	.globl	__longjmp		| Do just one name, other on link line
__longjmp:
	movl	a_p0,a0			| address of jmp_buf
	movl	#JB_MAGICNUM,d0
	cmpl	a0@(JB_MAGIC*4),d0
	bne	botch			| jmp_buf was trashed
	movl	a0@(JB_SP*4),a1
|
| Leo 13Apr88 We can longjmp to a higher PC with new coroutine setup
|	cmpl	a1,sp
|	bhi	botch			| cant longjmp to deeper stack!
#ifndef FP_BUG
#ifdef __GNU__
	.long	0xf228d03f
	.word	0x44
	.long	0xf2289c00
	.word	0x8c
#else !__GNU__
	fmovem	a0@(JB_FP2*4),fp2/fp3/fp4/fp5/fp6/fp7
	fmovem	a0@(JB_FPCR*4),fpc/fps/fpi
#endif !__GNU__
#endif FP_BUG
	movl	a_p1,d0			| return value
	movl	a1,sp			| back to old stack
	moveml	a0@(JB_D2*4),#0x7cfc	| restore d2-d7, a2-a7
	movl	a0@(JB_PC*4),a0		| return to _setjmps ra
	jmp	a0@

botch:
	jsr	_longjmperror		| prints error message
	jsr	_abort
