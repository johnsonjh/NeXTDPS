/*
  gstack.h

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
Scott Byer: Thu Jun  1 15:25:32 1989
Joe Pasqua: Fri Feb  3 13:42:58 1989
Ivor Durham: Mon Feb  8 21:00:11 1988
Bill Paxton: Wed Mar  9 08:24:32 1988
Jim Sandman: Fri Feb  3 09:05:51 1989
End Edit History.
*/

#ifndef GSTACK_H
#define GSTACK_H

typedef struct _gstack {
        PGState gss;
	integer	gsCount;	/* number of gstates in the stack */
} GStack, *PGStack;

/* Exported Procedures */

private procedure GStackRestore();
extern procedure GStackClear();
extern PGStack CreateGStack();

#endif GSTACK_H
