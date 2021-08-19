/*
  graphicspriv.h

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
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
Ivor Durham: Fri Sep 23 15:49:18 1988
Ed Taft: Sat Jul 11 13:46:11 1987
Bill Paxton: Fri Aug 18 15:59:33 1988
Jim Sandman: Fri Sep 29 13:00:07 1989
Joe Pasqua: Fri Feb  3 13:42:28 1989
End Edit History.
*/

#ifndef	GRAPHICSPRIV_H
#define	GRAPHICSPRIV_H

#include PACKAGE_SPECS
#include ENVIRONMENT
#include FP
#include BASICTYPES
#include GRAPHICS
#include DEVICE
#include PSLIB

/* graphics data types */

#define buttCap 0
#define roundCap 1
#define tenonCap 2
#define miterJoin 0
#define roundJoin 1
#define bevelJoin 2

#define ColorArray(c) ((PClrPart)(&c))
#define ColorIsWhite(c) ((boolean)((*((PInt32)(&(c))))== -1))

typedef Card16 RunAr, *PRunAr;

#define MAXDCMPUNITS 128
/* maximum width in SCANUNIT's of a compressable line */

typedef struct /* argument record for flattening */
  {
  integer limit;
  real reps;
  Fixed feps;
  procedure (*report)( /* Cd or DevCd */ );
  boolean reportFixed;
  DevCd ll, ur;
  Fixed llx, lly;
  } FltnRec, *PFltnRec;

#define ATRAPLENGTH 64

typedef struct {
  procedure (*initMark)();
  procedure (*termMark)();
  procedure (*trapsFilled)();
  procedure (*strokeMasksFilled)();
  } MarkStateProcs, *PMarkStateProcs;

typedef struct {
  BBoxCompareResult bbCompMark;
  boolean markClip:1;
  boolean haveTrapBounds:1;
    BitField unused:14;
  DevPrim *trapsDP;
  DevPrim **rdcDevPrim;
  PMarkStateProcs procs;
  } MarkState, *PMarkState;

#if (OS == os_vms)
globalref PMarkState ms;
#else (OS == os_vms)
extern PMarkState ms;
#endif (OS == os_vms)

/* Exported Procedures */

extern BBoxCompareResult BBCompare( /* BBox figbb, clipbb; */ );
extern BBoxCompareResult DevBBCompare( /* DevBBox figbb, clipbb; */ ); 
  /* returns the result of comparing the two given bounding boxes.  Returns
     "inside" iff "figbb" is completely inside "clipbb"; returns "outside" iff
     they are disjoint; and returns "overlap" iff "figbb" is partially inside
     "clipbb" and partially outside "clipbb". */

extern procedure ClNewPt( /* Cd c; */ );
  /* A procedure suitable for passing to FeedPathToReducer that calls
     the reducer's NewPoint routine with a scaled coordinate. */

extern procedure FClNewPt( /* FCd c; */ );
  /* A procedure suitable for passing to FeedPathToReducer that calls
     the reducer's NewPoint routine with a scaled coordinate. */

extern Fixed RdcToDev( /* Fixed r */ );
  /* Converts numbers from reducer space into device space.  */

extern procedure RTfmPCd( /* Cd c; PMtx m; Cd cur; PCd rc */ );
  /* Returns (in rc) "cur" + DTfm("c"), for relative transforms. */

extern procedure SetScal();
extern BBox GetDevClipBBox();
extern DevBBox GetDevClipDevBBox();
extern procedure GetBBoxFromDevBounds();
extern boolean DevClipIsRect();
extern procedure TermClipPathDevPrim(/* DevPrim *clip */);
extern procedure TermClipDevPrim();
extern procedure FFltnCurve();
extern procedure FltnCurve();
extern boolean PathIsRect();
extern procedure CheckForCurrentPoint();
extern procedure RemPathRef();
extern procedure ReducePath();
extern boolean ReduceQuadPath();
extern procedure InitMtx();
extern procedure InitClip();
extern procedure InitClipPath();
extern procedure NullDevice();
extern procedure NewDevice();
extern procedure MarkDevPrim();
extern procedure SetRdcScal();
extern procedure StdTermMark();
extern procedure Clip();
extern procedure MakeRectPath();
extern procedure ReducePathClipInt();

extern procedure InitFontFlat(/*proc*/);

/*	----- Exported Data -----	*/
/* Actually these are definitions to get at the Exported Data	*/

extern Fixed edgeminx, edgeminy, edgemaxx, edgemaxy;

extern Pool lpStorage; /* pool for ListPath records */
extern Pool qpStorage; /* pool for QuadPath records */
extern Pool ipStorage; /* pool for IntersectPath records */
extern Pool spStorage; /* pool for StrkPath records */
extern Pool rpStorage; /* pool for ReducedPath records */

extern PDevice psNulDev;

/* put in graphics.h */

typedef struct {
  char *ptr;		/* Ptr to bytes in buffer */
  Card32 len;		/* Number of bytes in buffer */
  } GrowableBuffer, *PGrowableBuffer;

extern GrowableBuffer gbuf[4];

#endif	GRAPHICSPRIV_H
