/*
  stack.h

Copyright (c) 1983, '86, '87, '88 Adobe Systems Incorporated.
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
Ivor Durham: Wed May 18 14:36:06 1988
Ed Taft: Wed Oct  4 09:47:32 1989
Linda Gass: Thu Aug  6 09:16:38 1987
Joe Pasqua: Wed Jan  4 18:09:51 1989
End Edit History.
*/

#ifndef	STACK_H
#define	STACK_H

/*
 * Constants
 */

#define	STKGROWINC	20	/* Stack growth increment	*/
#define	STKINITSIZE	10	/* Initial Stack size		*/
#define	STKSIZELIMIT	2048	/* Default maximum stack size	*/
#define	UPPERSTKLIMIT	4096	/* Absolute maximum stack size	*/

/*
 * Procedures
 */

extern procedure StackInit(/* InitReason reason */);
extern procedure PushStackRoots(/* GC_Info info */);
extern procedure EPush(/* Object ob */);
extern procedure StackPopDiscard(/* PStack stack */);

extern Card16 PopLimitCard();
/* Pops integer from opStk and returns it as a Card16. This is similar
   to PopCardinal, but it raises a limitcheck rather than a rangecheck
   if the value is >= 2**16. This is the appropriate behavior for the
   length operand of the array, dict, and string operators.
 */

#endif	STACK_H
