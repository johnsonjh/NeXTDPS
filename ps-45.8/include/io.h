/*
  io.h

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

Original version: Ed Taft: Sun May 31 16:03:47 1987
Edit History:
Ed Taft: Mon Jun 26 11:39:15 1989
End Edit History.
*/

#ifndef	IO_H
#define	IO_H

#include BASICTYPES
#include STREAM

/* I/O Driver interface */

typedef struct _t_IOStms {	/* return value from StmCreate procs */
  Stm input, output;
  } IOStms;


/* EEROM interface */

/* Note: the implementation of these procedures is responsible for
   dealing with hardware peculiarities such as minimum time between
   writes, avoiding writes that don't change the value, etc.
 */

extern Card8 EEROMRead(/* Card16 addr */);
extern procedure EEROMWrite(/* Card16 addr; Card8 byte */);
/* These procedures read and write a single byte at a specified address
   in EEROM. The EEROM address space consists of single bytes at
   consecutive addresses, starting at 0 and extending to a limit
   specified in the call to EEROMInit (no bounds checking is done). */

extern procedure EEROMInit(/* Card16 size; boolean useHardware, reset */);
/* Specifies the size of the EEROM in bytes and performs initialization
   as specified by the other two arguments. If useHardware is true,
   subsequent calls of EEROMRead and EEROMWrite should access the EEROM
   hardware directly; if false, they should simulate EEROM in RAM.
   If reset is true, EEROMInit first reads the contents of EEROM into
   the exported array "eerom" (declared below); then it resets the
   entire EEROM (or simulated EEROM) to zero. EEROMInit may be called
   more than once. */

extern PCard8 eerom;
/* An array into which EEROMInit reads the former contents of EEROM
   before resetting it. This array may also be used for software
   simulation of EEROM, but no caller should depend on this. This
   array is allocated by EEROMInit; it may not be allocated if the
   EEROM is not reset. */

#endif	IO_H
/* v003 lent Wed Aug 10 19:38:21 PDT 1988 */
/* v005 durham Fri Apr 8 11:40:40 PDT 1988 */
/* v006 durham Wed Jun 15 14:22:04 PDT 1988 */
/* v007 durham Thu Aug 11 13:08:16 PDT 1988 */
/* v008 caro Tue Nov 8 11:49:22 PST 1988 */
/* v009 pasqua Thu Dec 15 11:09:45 PST 1988 */
/* v010 byer Tue May 16 11:15:48 PDT 1989 */
/* v011 taft Tue Jun 27 16:13:14 PDT 1989 */
/* v012 sandman Mon Oct 16 17:03:58 PDT 1989 */
/* v013 taft Sun Dec 3 12:23:33 PST 1989 */
