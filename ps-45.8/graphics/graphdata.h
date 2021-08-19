/*
  graphdata.h

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
Joe Pasqua: Tue Feb 21 15:08:45 1989
Bill Paxton: Sat Mar 12 11:25:13 1988
Jim Sandman: Tue Apr 26 12:55:19 1988
Ivor Durham: Thu Jan 12 15:52:35 1989
End Edit History.
*/

#ifndef	GRAPHDATA_H
#define	GRAPHDATA_H

#include "viewclip.h"
#include "gstack.h"

typedef struct {
  BitField _savelevel:16;	/* current save-restore level           */
  PGStack _gstack;      	/* Current gstack for this ctxt         */
  PGState _gs;			/* Head of gstack for this context	*/
  } GStatesData;


typedef struct {	/* Data for viewclip.c			*/
  ViewClip *_viewClips;	/* Linked list of this ctxt's ViewClips	*/
  ViewClip *_curVC;	/* current viewclip for this ctxt. 	*/
  } ViewClipData;

typedef struct {
  integer _blimit;
} UCacheData;

typedef struct {
  long int _randx;
} GrayData;

typedef struct {
  GStatesData gStates;
  ViewClipData viewClip;
  UCacheData  uCache;
  GrayData grayData;
} GraphicsData, *PGraphicsData;

extern PGraphicsData graphicsStatics;
#endif	GRAPHDATA_H
