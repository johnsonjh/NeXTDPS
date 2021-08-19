/*
  path.h

Copyright (c) 1986, '87, '88 Adobe Systems Incorporated.
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
Ivor Durham: Mon Feb  8 20:30:47 1988
Ed Taft: Sat Jul 11 16:56:13 1987
Joe Pasqua: Thu Jan 12 15:48:46 1989
Bill Bilodeau: Fri Oct 13 11:20:35 PDT 1989
End Edit History.
*/

#ifndef	PATH_H
#define	PATH_H

#include BASICTYPES
#include GRAPHICS	/* Partial interface exported from package */
#include ENVIRONMENT

/* Constants */

#define FLATTENLIMIT 10

/* Exported Procedures */;

extern procedure AppendCopyToPath( /* PEOffset pe; PPath newPath; */ );
  /*  appends new path elements that copy the elements of the list beginning
      with "pe" onto the tail of "newPath". */

extern procedure DoPath
    (/* PPath path; procedure (*stproc)(Cd), (*ltproc)(Cd), (*clsproc)(),
        (*endproc)(), (*ctproc)(bool);  boolean noendst;  real reps;  */);
  /* Responsible for enumerating and flattening a path.  "stproc", which
     takes a single Cd argument, is called for all pathstart's (except a
     trailing pathstart when "noendst" is true) and for all implicit
     pathstart's (between a pathclose and a pathlineto or pathcurveto.
     "ltproc", which takes a single Cd argument, is called for all
     pathlineto's and for all flattened sections of pathcurveto's.
     "clsproc", which takes no arguments, is called for all pathclose's.
     "endproc", which takes no arguments, is called after any pathlineto
     or pathcurveto that is either the last path element or is followed by
     a pathstart.  "ctproc" is called immediately before and immediately
     after flattening any pathcurveto's.  Its boolean argument indicates
     before (true) or after (false) the flattening.  "reps" is a real
     flatness epsilon, to be used while flattening.  */

extern procedure FeedPathToReducer
       ( /* PPath p; procedure (*newpoint)(Cd); procedure (*close)(); */ );
  /* Calls "newpoint" and "close" as appropriate for all the coordinates in
     "p".  Takes care of flattening Bezier curves to line segments. */

extern Path FltnPth( /* PPath p; real flatness; */ );
  /* Returns a new path that is a copy of "p", except that all curveto's 
     have been flattened to multiple lineto's according to "flatness".  */

extern procedure InitPath( /* PPath p; */ );
/* initializes "p" to empty. */

extern procedure InstallClip();

extern procedure PathBBox( /* BBox bbox; */ );
  /* fills in "bbox" with a bounding box showing the limits of the
     current path in device coordinates. */

extern procedure RCLastPt();
  /* This procedure should be used in a call to FeedPathToReducer in the
     LastPoint position if the rectangle clipping routines are being used.
     RCLastPt flushes the points remaining in the rectangle clipping pipe
     and calls RdcClose.  */

extern procedure RCNextPt( /* Cd coord */ );
  /* This procedure should be used in a call to FeedPathToReducer in the
     NextPoint position if rectangle clipping is desired.  Note: SetUpRectClip
     must be called prior to this procedure being called. */

extern procedure ReversePath( /* PPath path; */ );
  /* Reverses the direction of each sub-path in "path". */

extern procedure SetUpRectClip();
  /* Prepares the data structures for RCNextPt and RCLastPt (the rectangle
     clipping procedures.)  This procedure must be called before calling a
     sequence of those procedures. */

extern procedure DumpVectors();

extern procedure AddVecMoveTo();

extern procedure UpdateVecBounds();

extern procedure AddVecPt();
extern procedure MakeBounds();
extern procedure PathForAll();
extern procedure OFill();
extern procedure ConvertToListPath();
extern procedure FillPath();
extern procedure RemReducedRef();
extern procedure CopyPath();
extern procedure AddRdcTrap();
extern procedure AddRunMark();
extern procedure AddTrapezoidToPath();
extern procedure ArcInternal();
extern procedure ArcToInternal();
extern procedure BuildMultiRectPath();
extern procedure GetPathBBoxUserCds();
extern procedure PutRdc();

#ifndef DPSXA
#define DPSXA 0
#endif
#if DPSXA
/* Define CHUNKSIZE to be a multiple of SCANUNIT==32 */
#if (STAGE == DEVELOP)
#ifndef CHUNKSIZE
#define CHUNKSIZE 224L  /* Not a convenient number like 256 */
#endif CHUNKSIZE
#else (STAGE == DEVELOP)
#define CHUNKSIZE 16000L
#endif (STAGE == DEVELOP)
#define XA_MIN -CHUNKSIZE
#define XA_MAX CHUNKSIZE
#define RDCXTRA 4	/* should be larger (about 50 or so for the real version) */
extern BBoxRec	chunkBBox;
extern QuadPath *chunkqp;
extern DevCd xaOffset;
extern procedure ReduceClip();
extern unsigned long int maxXChunk, maxYChunk;
extern long int xChunkOffset, yChunkOffset;
#endif DPSXA

#endif	PATH_H
