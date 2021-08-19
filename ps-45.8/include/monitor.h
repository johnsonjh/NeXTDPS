/*
  monitor.h

Copyright (c) 1987 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Sat Jun  6 14:02:48 1987
Edit History:
Ed Taft: Mon Jun 26 11:42:06 1989
End Edit History.

This is the interface to the runtime monitor. Some of the items defined
here are fairly generic, while others are relevant only for 68000-based
systems. This should ultimately be replaced by a better structured and
more comprehensive interface to runtime facilities.
*/

#ifndef	MONITOR_H
#define	MONITOR_H

/* Exported Data */

extern int
  ramsize,		/* bytes of RAM in controller */
  oringincr,		/* offset from normal to ORing memory */
  debugmode,		/* debugger control:
			   0: disabled; all traps re-boot PostScript
			   1: enabled, but "x" (download) command disabled
			   2: enabled, all capabilities available */
  monflags;		/* ?? */

extern char
  *ramend,		/* address of end of RAM */
  *iobase;		/* base of I/O register address space */

extern char brgen1, brgen2, brgen3, brgen4;

extern unsigned short *pattregloc;

/*
 * The following are pointers to C procedures, which, if non-NIL,
 * are called by the monitor when the specified exceptions occur.
 */
extern int
  (*zerodiv)(),		/* zero divide exception */
  (*chkinst)(),		/* CHK instruction exception */
  (*trpvinst)(),	/* TRAP instruction */
  (*intvec1)(),		/* interrupt at level 1 */
  (*intvec2)(),		/* ... */
  (*intvec3)(),
  (*intvec4)(),
  (*intvec5)(),
  (*intvec6)(),
  (*intvec7)();		/* interrupt at level 7 */

/*
 * sccint is an array of 8 pointers to procedures for handling SCC
 * interrupts, indexed by SCC Read Register 2 vector bits 1-3
 * (see table 4-3 in SCC manual)
 */
extern int (*sccint[])();

#define BTXBE 0		/* channel B transmit buffer empty */
#define BSTCHG 1	/* channel B external status change */
#define BRXCA 2		/* channel B receive character available */
#define BSPCOND 3	/* channel B special receive condition */
#define ATXBE 4		/* channel A transmit buffer empty */
#define ASTCHG 5	/* channel A external status change */
#define ARXCA 6		/* channel A receive character available */
#define ASPCOND 7	/* channel A special receive condition */


/* Exported Procedures */

extern int entermon();


#endif	MONITOR_H
