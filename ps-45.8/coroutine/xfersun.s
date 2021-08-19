/*
  xfersun.s

Copyright (c) 1987, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Fri Jan  8 12:52:06 1988
Edit History:
Ed Taft: Thu Apr 28 12:02:54 1988
Leo Hourvitz: Sun Jun 26 1988 Ansi-ized
End Edit History.

Minimum length of XferState = 2 longwords = 8 bytes
*/

#include PACKAGE_SPECS
#define	 C_LANGUAGE 0
#include ENVIRONMENT

#define ENTRY(name) .globl CAT(_,name); CAT(_,name):

| Layout of XferState block:
#define mysp 0		/* sp after all saved registers have been pushed */
#define chksum 4	/* minor error checking */

| offsets from sp for items in xfer''s frame
#define savedd2 0
#define savedd3 4
#define savedd4 8
#define savedd5 12
#define savedd6 16
#define savedd7 20
#define saveda2 24	/* initial proc */
#define saveda3 28
#define saveda4 32
#define saveda5 36
#define saveda6 40	/* frame pointer */
#define savedpc 44	/* caller''s pc */


| initxfer(state, stack, stackSize, proc)
|   XferState *state; char *stack; integer stackSize;
|   procedure (*proc)(char *arg);

ENTRY(initxfer)
	moveml	sp@(4),a0-a1		| a0 -> state; a1 -> stack
	addl	sp@(12),a1		| a1 -> end of stack
	movl	#firstXfer,d1		| get start address
	movl	d1,a1@-			| push initial "return" PC
	moveml	d2-d7/a2-a6,a1@-	| push snapshot of current registers
	clrl	a1@(saveda6)		| no previous frame pointer
	movl	sp@(16),a1@(saveda2)	| saved a2 = proc
	movl	a1,a0@+			| state->mysp = new frame
	addl	a1,d1			| make a checksum
	movl	d1,a0@			| state->chksum = checksum
	rts


| First xfer to newly initialized XferState comes here.
| d0 = "arg" passed to xfer
| a2 = "proc" passed to initxfer
firstXfer:
	movl	d0,sp@-			| push arg
	jsr	a2@			| call proc(arg)
	jbsr	_CantHappen		| proc must never return


| char *xfer(source, destination, arg)
|   XferState *source, *destination; char *arg;

ENTRY(xfer)
	moveml	sp@,d1/a0-a1		| d1 = return pc;
					| a0 -> source; a1 -> destination
	movl	sp@(12),d0		| d0 = arg
	moveml	d2-d7/a2-a6,sp@-	| push saved registers onto stack
	movl	sp,a0@+			| source->mysp = sp
	addl	sp,d1			| make a checksum
	movl	d1,a0@			| source->checksum = sp + pc
	movl	a1@+,a0			| a0 = destination->mysp
	movl	a0,d1
	addl	a0@(savedpc),d1		| compute destination checksum
	cmpl	a1@,d1			| check it
	jne	1f			| jump if wrong
	movl	a0,sp			| switch to destination stack
	moveml	sp@+,d2-d7/a2-a6	| load destination registers
	rts				| return in destination context

1:	lea	sp@(savedpc),sp		| undo pushes
	link	a6,#0			| make a normal frame (for debugger)
	jbsr	_CantHappen		| report failure (never returns)
