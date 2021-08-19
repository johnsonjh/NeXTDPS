/* PostScript Graphics Package Definitions

Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Edit History:
Jim Sandman: Mon Aug 14 09:31:28 1989
Bill Paxton: Sat Oct 17 09:29:15 1987
Ivor Durham: Wed Aug 17 14:08:52 1988
Linda Gass: Wed Dec  2 14:27:15 1987
Joe Pasqua: Wed Dec 14 14:29:28 1988
Paul Rovner: Thu Dec 28 17:04:43 1989
Ed Taft: Thu Dec 14 12:06:18 1989
End Edit History.
*/

#ifndef DEVICE_H
#define DEVICE_H

#include PUBLICTYPES
#include ENVIRONMENT
#include FP
#include DEVICETYPES

typedef struct _t_DevProcs {
    procedure (*Mark)(/* 
      PDevice device; DevPrim *graphic; DevPrim *clip; DevMarkInfo *info;
      */);
    procedure (*DefaultMtx)(/* PDevice device; PMtx matrix; */);
    procedure (*DefaultBounds)(/* PDevice device; DevLBounds *bBox; */);
    DevHalftone * (*DefaultHalftone)(/* PDevice device; */);
    DevColor (*ConvertColor)(/*
      PDevice device; integer colorSpace; DevInputColor *input;
      DevTfrFcn *tfr; DevPrivate *priv; DevGamutTransfer gt; 
      DevRendering r; DevWhitePoint wp; */);
    procedure (*FreeColor)(/*
      PDevice device; DevColor color; */);
    integer (*SeparationColor)(/*
      PDevice device; char *name; integer *nameLength;
      DevPrivate *priv; */);
    DevGamutTransfer (*ConvertGamutTransfer)(/*
      PDevice device; char *dict; */);
    procedure (*FreeGamutTransfer)(/*
      PDevice device; DevGamutTransfer gt; */);
    DevRendering (*ConvertRendering)(/*
      PDevice device; char *dict; */);
    procedure (*FreeRendering)(/*
      PDevice device; DevRendering r; */);
    procedure (*WinToDevTranslation)(/*
      PDevice device; DevPoint *translation; */);
    procedure (*GoAway)(/* PDevice device; */);
    procedure (*Sleep)(/* PDevice device; */);
    procedure (*Wakeup)(/* PDevice device; */);
    struct _t_Device * (*MakeMaskDevice)(/*
      PDevice device; MakeMaskDevArgs *args; */);
    boolean (*PreBuiltChar)(/* PDevice device; PreBuiltArgs *args; */);
    struct _t_Device * (*MakeNullDevice)(/* PDevice device; */);
    procedure (*DeviceInfo)(/*
      PDevice device; DeviceInfoResults *results */);
    procedure (*InitPage)(/* PDevice device; DevColor color; */);
    boolean (*ShowPage)(/*
      PDevice device; boolean clearPage; integer nCopies;
      unsigned char *pageHook; */);
    DevLong (*ReadRaster)(/*
      PDevice device; DevLong xbyte, ybit, wbytes, hbits;
      procedure (*copybytes)(PCard8 p; DevLong nbytes; char *arg);
      char *arg; */);
    integer (*Proc1)();
    integer (*Proc2)();
    integer (*Proc3)();
    } DevProcs, *PDevProcs;

typedef struct _t_DevProperties *PDevProperties;
/* Opaque designator for a page device property list */

typedef struct _t_Device {
    DevProcs *procs;
    DevPrivate *priv;
    PDevProperties devProps;
    DevShort ref;
    Card16 maskID;
    } Device;  /* *PDevice declared in devicetypes.h */
    
extern boolean DevCheckScreenDims(/* integer width, height; */);
      
extern integer DevFlushMask(/* PMask mask; DevFlushMaskArgs *args; */);

extern procedure DevFlushClip(/* DevPrim *clip */);

extern DevHalftone * DevAllocHalftone(/*
  DevShort wWhite, hWhite, wRed, hRed, wGreen, hGreen, wBlue, hBlue */);

extern procedure DevFreeHalftone(/* DevHalftone *halftone */);

extern DevTfrFcn * DevAllocTfrFcn(/*
  boolean white, red, green, blue, ucr, bg */);

extern procedure DevFreeTfrFcn(/* DevTfrFcn *transfer */);

extern boolean DevIndependentColors();

extern boolean DevMinimizeTrapPrecision();

extern integer DevRgstPrebuiltFontInfo(/*
  integer font; char *name; integer nameLength; 
  boolean stdEncoding; CharNameProc proc; char *procData;*/);
  
extern procedure DevMaskCacheInfo(/* integer *used, *size */);

extern procedure DevSetMaskCacheSize(/* integer size */);


extern integer PSFlushMasks(/* integer needed, maskID; */);

extern BBoxCompareResult BoxTrapCompare(/*
  DevBounds *figbb; DevInterval *inner, *outer; DevTrap *clipt, *rt; */);
/* BoxTrapCompare returns the result of comparing the figure bounding
   box with the clipping trapezoid.  Returns "inside" iff "figbb" is
   completely inside "clipt"; returns "outside" iff they are disjoint;
   and returns "overlap" iff "figbb" is partially inside "clipt" and
   partially outside "clipt". If (rt) interpolation will be made
   on diagonal edges of the clipt, storing the result in *rt, where
   necessary to refine the accuracy of the result. In the case that
   "overlap" is returned, a complete reduced trapezoid will be stored.
   A reduced trapezoid is the smallest possible trapezoid whose
   intersection with figbb is the same as clipt's.  (In other cases,
   rt may be altered but meaningless.) */
  
extern integer TrapTrapInt(/*
  DevTrap *t0, *t1; DevInterval *y; int (*callback)(); char *args; */);
  /* intersects the traps in t0 and t1 and passes the results to callback */
  /* (*callback)(t, args) where t is a DevTrap * for intersection */

extern integer BytesForRunIndex(/* DevRun *r; */);
  /* tells how big the run index needs to be for the run r */

extern procedure BuildRunIndex(/* DevRun *r; */);
  /* builds a run index for r */
  /* if r->indx is null, will alloc one.
     else will assume caller has provided one of correct size */

extern DevShort *RunArrayRow(/* DevRun *r; DevShort yr; */);
  /* returns pointer to start of run data for row yr */
  /* uses run index if it exists */

extern procedure QIntersect(/*
  DevRun *r1, *r2; procedure (*callback)(); char *args; */);
  /* forms the intersection of two run arrays, r1 and r2 */
  /* callback gets two arguments: r, args
     where r is part of the intersection */

extern procedure QIntersectTrp(/*
  DevRun *r1; DevTrap *trp; procedure (*callback)(); char *args; */);
  /* forms the intersection of a run array and a trapezoid */
  /* callback gets same two arguments as for QIntersect */

extern procedure QIntersectBounds(/*
  DevRun *r1; DevBounds *b;  procedure (*callback)(); char *args; */);
  /* forms the intersection of a run array and a rectangle */
  /* callback gets same two arguments as for QIntersect */

extern BBoxCompareResult QCompareBounds(/*
  DevRun *r; DevBounds *b; char *args; */ );
  /* tests to see if the rectangle is inside, outside, or overlaps
     the run array */

/*
 * DevBounds procedures
 */

extern DevBounds *EmptyDevBounds(/*DevBounds *self*/);

extern DevBounds *MergeDevBounds(/*DevBounds *self, *a, *b*/);

extern DevBounds *DevTrapDevBounds(/*DevBounds *self; DevTrap *trap*/);

extern DevBounds *DevRunDevBounds(/*DevBounds *self; DevRun *run*/);

extern DevBounds *DevMaskDevBounds(/*DevBounds *self; DevMask *mask*/);

extern boolean OverlapDevBounds(/*DevBounds *self, *a*/);

extern BBoxCompareResult BoundsCompare(/* DevBounds *figb, *clipb */ );

extern void FullBounds(/* DevPrim *p; DevBounds *b; */ );

/*
 * DevPrim procedures.
 */

extern DevPrim * NewDevPrim();
  /* returns a new uninitialized DevPrim */

extern DevPrim * InitDevPrim( /* DevPrim *self, *next; */ );
  /* initializes the DevPrim self and makes it point to next */

extern DevPrim * CopyDevPrim( /* DevPrim *from; */ );
  /* returns a new DevPrim that is a copy of the arg DevPrim */

extern DevPrim * ClipDevPrim( /* DevPrim *clip1, *clip2; */ );
  /* returns a new DevPrim that is the intersection of the args */
  /* the arg DevPrims must be trapType, runType, or noneType */

extern DevPrim *AddDevPrim(/*
  DevPrim *self; DevPrimType type; DevPrivate *value; int length;
  DevBounds *bounds*/);

extern DevPrim *AddRunDevPrim(/* DevPrim *self; DevRun *run*/);

extern DevPrim *AddBoxDevPrim(/* DevPrim *self; DevBounds *b; */);

extern void DisposeDevPrim( /* DevPrim *self; */ );
  /* frees the storage for the arg DevPrim */

extern void AddRunIndexes( /* DevPrim *self; */);

extern boolean DevPrimIsRect(/* DevPrim *self;  */ );

extern integer DevPrimBytes(/* DevPrim *self; */);

#endif DEVICE_H


/* v017 sandman Fri Aug 12 16:35:32 PDT 1988 */
/* v017 caro Wed Nov  9 15:29:47 PST 1988 */

/* v018 pasqua Wed Dec 14 14:33:54 PST 1988 */
/* v019 sandman Mon Mar 6 10:40:25 PST 1989 */
/* v020 sandman Tue May 2 10:43:19 PDT 1989 */
/* v021 sandman Fri May 26 12:18:43 PDT 1989 */
/* v022 sandman Mon Oct 16 15:18:07 PDT 1989 */
/* v024 taft Fri Jan 5 15:34:42 PST 1990 */

