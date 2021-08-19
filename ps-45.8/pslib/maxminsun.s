/*
  maxminmach.s

Copyright (c) 1985, 1988 Adobe Systems Incorporated.
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
Ed Taft: Sun May  1 15:26:31 1988
Leo Hourvitz: Mon Jun 27 1988
End Edit History.
*/

#include PACKAGE_SPECS

| long int os_max(a, b)
| long int os_min(a, b)
|   long int a, b;

	.globl	_os_max
_os_max:
	movl	sp@(4),d0
	movl	sp@(8),d1
	cmpl	d0,d1
	jlt	one
	movl	d1,d0
one:	rts

	.globl	_os_min
_os_min:
	movl	sp@(4),d0
	movl	sp@(8),d1
	cmpl	d0,d1
	jgt	two
	movl	d1,d0
two:	rts
