/*
  stroke.h

Copyright (c) 1986, 1987 Adobe Systems Incorporated.
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
Ivor Durham: Fri Sep 23 19:14:32 1988
Ed Taft: Sat Jul 11 12:59:09 1987
Jim Sandman: Tue Oct 24 16:50:58 1989
Joe Pasqua: Thu Jan 19 15:17:24 1989
End Edit History.
*/

#ifndef	STROKE_H
#define	STROKE_H

#include BASICTYPES
#include FP
#include "path.h"

#define _stroke extern

#define DASHLIMIT 11

typedef struct quadcorner
  {DevCd c; struct quadcorner *ptr1, *ptr2;} QdCorner, *PQdCorner;

typedef struct {
  DevPrim *devprim;
  Cd center;
  FCd offset;
  PMask mask;
  boolean flushed;
  } CircleRec, *PCircle;

/* Exported Procedures */

_stroke procedure SetLineWidth( /* Preal width */ );
  /* sets the line width in the current graphics state. */

_stroke procedure StrkInternal( /* PPath p; boolean isStrkPth; */ );
  /* Procedure that does the work for both Stroke and StrkPth.
     if "isStrkPth" is true, then a StrkPth results, if false, a
     Stroke results. */

_stroke Path StrkPth( /* PPath p; */ );
  /* Returns a new path which, if filled, would produce the same output
     as "p" when stroked.  */

_stroke integer VecTurn( /* Cd v1, v2 */ );
  /* returns 1 if v1 followed by v2 makes a left turn, 0 if straight or
     u-turn, -1 if right turn. */

_stroke procedure PreCacheTrapCircles();
_stroke procedure FinStroke();
_stroke procedure FastFillQuad();
_stroke procedure DoStrkPth();
_stroke procedure PSStrkPth();
_stroke boolean DoStroke();

#endif	STROKE_H
