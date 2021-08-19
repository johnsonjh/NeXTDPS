/*
  fixunsdfsi.s

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

Original version: Ivor Durham: Mon Aug 15 07:45:17 1988
Edit History:
Scott Byer: Mon May  8 15:23:40 1989
End Edit History.
*/

| Implementation of the one runtime procedure required by gcc-generated code.
| ___ version added for SUNOS4.0

	.globl	__fixunsdfsi
	.globl	___fixunsdfsi
__fixunsdfsi:
___fixunsdfsi:
	jmp	Fund
