/*
  graphics.h

Copyright (c) 1984, '85, '86, '87, '88, '89, '90 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Doug Brotz, May 11, 1983
Edit History:
Doug Brotz: Fri Jan  9 14:35:38 1987
Chuck Geschke: Mon Jan 20 21:50:00 1986
Bill Paxton: Thu Mar 10 14:29:21 1988
Peter Hibbard: Mon Dec 30 14:55:02 1985
Don Andrews: Mon Jan 13 15:44:27 1986
Ed Taft: Sat Jan  6 15:40:57 1990
Ken Lent: Tue Aug  5 13:32:54 1986
Jim Sandman: Wed Oct 25 09:15:26 1989
Joe Pasqua: Wed Dec 14 16:11:46 1988
Linda Gass: Thu Dec  3 16:29:24 1987
Ivor Durham: Fri Aug 26 10:14:48 1988
End Edit History.
*/

#ifndef	GRAPHICS_H
#define	GRAPHICS_H

#include BASICTYPES
#include ENVIRONMENT
#include DEVICETYPES
#include VM

/* Basic definitions */

/*
 * Matrix and Vector Interface
 */

/* Data types */

/* Exported Procedures */

extern procedure MtxToPAry( /* PMtx m; PAryObj pa; */ );
  /* fills in "*pa"'s storage with real-values from "m". */

extern procedure PAryToMtx( /* PAryObj pa; PMtx m; */ );
  /* fills in "m" with the real-values from "*pa".  The correspondence
     of array locations to matrix components is: m->a = pa[0], m->b = pa[1],
     m->c = pa[2], m->d = pa[3], m->tx = pa[4], m->ty = pa[5]. */

extern procedure PRealValue(/* Object ob; Preal r; */);
  /* returns the value of "ob" in *r iff "ob" has type integer or real.
     Raises "typecheck" if "ob" is neither integer or real. */

extern procedure PopMtx( /* PMtx m;*/ );
  /* pops an AryObj off of the PostScript operand stack, and fills
     in "m" with its values. */

extern procedure PushPMtx( /* PAryObj pa; PMtx m; */ );
  /* fills in "*pa" with values from "m", and pushes "*pa" onto the PostScript
     operand stack. */


/*
 * Path Interface
 */

#define pathstart 0
#define pathlineto 1
#define pathcurveto 2
#define pathclose 3

typedef Card16 PathTag;

typedef struct _t_PthElt {
  Cd			coord;
  struct _t_PthElt *	next;
  PathTag		tag;
} PthElt, *PPthElt;

typedef struct _t_BBoxRec {
  Cd      bl,
          tr;
} BBoxRec, *BBox;

typedef struct _t_DevBBoxRec {
  DevCd   bl,
          tr;
  } DevBBoxRec, *DevBBox;
 
typedef enum {
  listPth,
  intersectPth,
  quadPth,
  strokePth
  } PathType;

typedef struct _t_Path {
  BBoxRec	bbox;
  BitField
  /*PathType*/	type:8,
  /*boolean*/	secret:1,
  /*boolean*/	eoReduced:1,
  /*boolean*/	isRect:1,
  /*boolean*/	checkedForRect:1,
  /*boolean*/   setbbox:1,
  /*boolean*/	unused:3;
  Int16		length;
  struct _t_ReducedPath *rp;
  union
    {
    struct _t_ListPath *lp;
    struct _t_QuadPath *qp;
    struct _t_IntersectPath *ip;
    struct _t_StrkPath *sp;
    } ptr;
  } Path, *PPath;

typedef struct _t_ReducedPath {
  Card16	refcnt;
  char*		devprim;
  } ReducedPath;

typedef struct _t_ListPath {
  Card16	refcnt;
  PPthElt	head,
		tail,
		start;
  } ListPath;

typedef struct _t_QuadPath {
  Card16        refcnt;
  Cd		c0, c1, c2, c3;
  } QuadPath;  

typedef struct _t_IntersectPath {
  Card16        refcnt;
  boolean	evenOdd;
  Path		clip, path;
  } IntersectPath;


typedef struct _t_StrkPath {
  Card16      refcnt;
  Mtx         matrix;
  real        lineWidth;
  real        miterlimit;
  real        devhlw;
  AryObj      dashArray;
  real        dashOffset;
  real        flatEps;
  BitField    lineCap:2;
  BitField    lineJoin:2;
  boolean     strokeAdjust:1;
  boolean     circleAdjust:1;
  BitField    unused:9;
  real        strokeWidth;
  Path        path;
  } StrkPath;
  
  /* Exported Procedures */

extern procedure Arc( /* Cd center; real radius, angStart, angEnd;
                        boolean ccwise; PPath path; */ );
  /* adds an arc to "path".  "center", "radius", "angStart" and "angEnd"
     are all in the current user space.  "ccwise" indicates whether the arc
     to be added is the counter-clockwise (true) or clockwise (false)
     arc allowed by the other parameters. */

extern integer BBoxVsClip(/* BBox b */);

extern procedure ClosePath( /* PPath p; */ );
/* appends a "closepath" point in the path "p". */

extern procedure CurveTo( /* Cd c1, c2, c3; PPath p; */ );
/*  appends "curveto" Bezier control points "c1" and "c2" and final point
    "c3" in path "p".  If "p" is empty, raises an error. */

extern real Dist( /* Cd v; */ );
  /* returns the length of the vector "v", i.e. sqrt(v.x * v.x + v.y * v.y) */

extern procedure Fill( /* PPath p;  boolean evenOdd; */ );
  /* fills "p" with the current color on the current output device.  If
     "evenOdd" is false, normal non-zero winding number output is used, if
     "evenOdd" is true, ugly odd-only winding number output is used.  */

extern procedure FrPth( /* PPath p; */ );
/* frees all elements of "p" and initializes the Path to an empty path. */

extern procedure LineTo( /* Cd c; PPath p; */ );
/* appends a "lineto" point at "c" in the path "p".  If "p" is empty,
   LineTo inserts a "moveto" point instead. */

extern procedure MoveTo( /* Cd c; PPath p; */);
/* appends a "moveto" point at "c" in the path "p". */

extern procedure NewPath();
/* resets the current path to empty. */

extern procedure OffsetFill( /* PPath p; real offsetwidth, varcoeff; */ );
  /* Fills "path" but offsets its points "offsetwidth" to the left
     along verticals and horizontals (in device space) and by an increased
     distance along other lines proportional to the product of "varcoeff"
     times the absolute value of the sine of 2 times the angle of inclination
     of the line. */

extern procedure Stroke( /* PPath p; */ );
/* writes a stroke along the path "p" to the current output device according
   to the current graphics state parameters (lineWidth, lineCap, etc.) */

extern procedure TlatPath( /* PPath p; Cd delta; */ );
  /* translates every path element in "p" by adding "delta". */

extern procedure RgstStrokeSemaphoreProc (/* PVoidProc proc(integer count) */);
 /* Registers a procedure to be call before and after invoking the interpreter
    to ensure that the context cannot yield while complicated graphics static
    state is still in use. */

extern boolean minTrapPrecision;

extern procedure SetGStateExtProc(/*
  PVoidProc proc(char **ext; integer refDelta); */);



/*
 * Mask and font public types
 *
 * These are here instead of in the fonts interface because they are
 * required by the display list machinery. A Mask is always passed to
 * the Device level with a double indirect pointer, i.e., a Mask **.
 * This permits passing a PCIItem in its place. The device can
 * distinguish these cases by the accompanying DevPrimType:
 * a PCIItem is sent as a charType; a Mask ** that is not a PCIItem
 * is sent as a stringType (see device.h).
 */

typedef unsigned int MID; /* should be a cardinal except for bug in SUN cc */

#define MAXMID 07777	/* maximum of 12 bits */
#define MIDNULL 0	/* valid as MID or as UniqueMID.stamp */

typedef union {			/* permanently unique MID */
  Card32 stamp;			/* entire identifier */
  struct {
    BitField generation: 20;	/* instance of given MID */
    MID mid: 12;		/* index into MT table */
    } rep;
} UniqueMID;

#define CINULL 0

#define maxwrtmodes 2

typedef BitField CItype; /* type of storage currently occupied by cached item;
			    enumeration is private to the fonts package */
typedef	Card16	CIOffset;
typedef	PCard16	PCIOffset;  /* used to point to CacheItem links */

typedef struct _t_CIItem {		/* Cache Item */
  PMask		cmp;		/* -> mask for this cached character;
				   must be first field in struct */
  CharMetrics	metrics[maxwrtmodes];	/* metrics for this mask */
  CItype	type:2;		/* storage type */
  BitField	touched:1;	/* used for aging characters */
  BitField	circle:1;	/* from circle font; cached by graphics */
  MID		mid:12;		/* "setfont" MID of parent font */
  CIOffset	cilink;		/* another CI for same name, different MID */
  CIOffset ageflink, ageblink;
}  CIItem,
  *PCIItem;

#define CIUNLINKED MAXCard16
#define MAXCISize (MAXCard16-1)

/* Exported procedures */

/*
  Note: most of these procedures are actually exported from the fonts
  package and are imported by the graphics package. This constitutes
  a package circularity that is seemingly unavoidable.
 */

extern procedure ShowMask( /* PMask mask; DevCd dc; */ );
  /* ships "mask" to the current device, to be placed with its lower left
     corner at "dc".  ShowMask checks "mask" for clipping.  If it is
     not clipped, ShowMask passes it on to the device's BlkMsk or
     ColorMask procedure depending on the current gray value.  If it is
     clipped, ShowMask passes the visible pieces on to the device's
     TrapMaskColor procedure. */

extern procedure SetMaskDevice(/* PDevice maskDevice*/);
 /*
   TBW
  */

extern procedure SetFont(/* DictObj dict */);
/* Sets the current font dictionary in the graphics state */

extern PCIItem FindInCache(/* integer c */);
/* Attempts to find a character in the cache; c is a character code
   selecting a character in the current font. Returns a pointer to
   the cached CIItem if successful, NIL if unsuccessful.
 */

extern procedure SimpleShow(/* StrObj str */);
/* Simply does a "show" of the specified string */

extern boolean FreeCircle(/* PMask mask; */);


/*
 * Gray, Color, Screen Interface
 */

/* Data Types */

typedef Card8 ClrPart, *PClrPart;

#define MAXCOLOR MAXCard8

/* gray is stored as the NTSC weighted average of red, green and blue
 * to save computation. */

#define colorIndexRed 0
#define colorIndexGreen 1
#define colorIndexBlue 2
#define colorIndexGray 3

typedef struct _t_SpotFunction {
  real angle, freq;
  Object ob;
  } SpotFunction, *PSpotFunction;
  
typedef struct _t_ThresholdArray {
  int width, height;
  StrObj thresholds;
  } ThresholdArray, *PThresholdArray;
  
typedef struct _t_ScreenRec {
  DevHalftone *halftone;
  integer ref;
  DictObj dict;
  Card8 type;
  BitField haveThresholds:1;
  BitField devHalftone:1;
  struct _t_ScreenRec *next;
  SpotFunction fcns[4];
  } ScreenRec, *Screen;
  


/*
 * Graphics State interface
 */

typedef struct _t_GamutTfrRec /* gamut transfer objects */
  {
  cardinal ref;
  DevGamutTransfer devGT;
  DictObj dict;
  } GamutTfrRec, *GamutTfr;

typedef struct _t_RenderingRec /* color rendering objects */
  {
  cardinal ref;
  DevRendering devRndr;
  DictObj dict;
  } RenderingRec, *Rendering;

typedef struct _t_ColorSpaceParams {
  DevWhitePoint wp;
  Object key, procs, procc, table;
  integer maxIndex;
  } ColorSpaceParams;


typedef struct _t_TfrFncRec /* transfer function objects */
  {
  boolean active:8;
  boolean isDevTfr:8;
  cardinal refcnt;
  integer tfrID;
  DevTfrFcn *devTfr;
  Object tfrGryFunc;
  Object tfrRedFunc;
  Object tfrGrnFunc;
  Object tfrBluFunc;
  Object tfrUCRFunc;
  Object tfrBGFunc;
  struct _t_TfrFncRec *next;
  } TfrFcnRec, *TfrFcn;

typedef struct _t_ColorRec /* color rendering objects */
  {
  cardinal ref;
  boolean emulatedSeparation:1;
  boolean indexed:1;
  DevColor color;
  DevInputColor inputColor;
  real tint;
  integer index;
  integer colorSpace;
  ColorSpaceParams cParams;
  } ColorRec, *Color;

/* Graphics State */

typedef struct _t_GState {
  GenericBody header;
  Mtx     matrix;
  UniqueMID matrixID;
  AryObj  encAry;
  DictObj fontDict;
  Path    path;
  Path    clip;
  Cd      cp;
  real    lineWidth;
  real    miterlimit;
  real    devhlw;
  AryObj  dashArray;
  real    dashOffset;
  Color color;
  Screen  screen;
  TfrFcn tfrFcn;
  Rendering rendering;
  GamutTfr gt;
  real    flatEps;		/* epsilon for curve flatness test */
  DevPoint   screenphase;
  PDevice device;
  BitField scale:8;		/* scaling to and from reducer */
  boolean  isCharPath:1;
  boolean  noColorAllowed:1;
  boolean  saveState:1;
  boolean  strokeAdjust:1;
  boolean  circleAdjust:1;
  boolean  overprint:1;
  BitField lineCap:2;
  BitField lineJoin:2;
  BitField unused2:6; 
  Card8 writingmode, fmap, escapechar;
  Card16 minEncodingLen;
  DictObj pfontDict;
  AryObj fdary, peary;
  StrObj kvary, svary;
  UniqueMID *miary;
  UniqueMID compfontMID;
  real     strokeWidth;
  PGState  previous;
  char *extension;		/* for misc experimental additions */
} GState;

/* Constants */

/* Exported Procedures */

extern procedure Cnct( /* PMtx m; */ );
  /* premultiplies "m" * the current transformation matrix,
     storing the result into the current transformation matrix */

extern procedure CrMtx( /* PMtx m; */ );
  /* fills in "m" with the current transformation matrix. */

extern procedure DfMtx( /* PMtx m; */ );
  /* fills in "m" with the current device's default transformation matrix */

extern procedure DTfmP( /* Cd c; PCd rc; */ );
  /* (Delta-Tfm) returns in *rc the result of multiplying "c" * the current
     transformation matrix with its translation components zero. */

extern DevPrim *GetDevClipPrim();
  /* return DevPrim of current clip. If there is a viewclip and we are 
     not in a maskdevice, the clip is intersected with the viewclip. */

extern procedure GRstr();
  /* destroys the current graphics state and restores the graphics state
     saved by the matching GSave.  GRstr may raise the error,
     "gsaveunderflow".  This function also implements the "grestore"
     operator in PostScript. */

extern procedure GRstrAll();
  /* calls GRstr repeatedly until the graphics state created at the
     time of the current PostScript "save" operator execution is at the top
     of the graphics state stack.  This may result in GRstr being called
     no times if that graphics state is already the top of the graphics state
     stack.  GRstrAll cannot raise the "gsaveunderflow" error. */

extern procedure GSave();
  /* saves the graphics state on the graphics state stack.  This state
     may be recovered by a matching GRstr.  This function also implements
     the "gsave" operator in PostScript. */

extern procedure GStackCopy (/* PGState gs */);
 /*
   Duplicate the graphics state stack pointed at by gs as the current graphics
   state stack (cf. fork)
  */

extern procedure IDTfmP( /* Cd c; PCd rc; */ );
  /* (Inverse-Delta-Transform) returns the Cd *rc, such that
     *rc * the current transformation matrix with zero translation components
     = "c".  This function may signal "undefinedresult". */

extern procedure ITfmP( /* Cd c; PCd rc; */ );
  /* (Inverse-Transform) returns the Cd *rc, such that
      *rc * the current transformation matrix = "c".
      This function may signal "undefinedresult". */

extern procedure Rtat( /* Preal angle; */ );
  /* premultiplies a rotation by "angle" degrees * the current
     transformation matrix, storing the result into the current transformation
     matrix. */

extern procedure Scal( /* Cd c; */ );
  /* premultiplies a scaling by (c.x, c.y) * the current transformation
     matrix, storing the result into the current transformation matrix. */

extern procedure SetGray( /* real gray; */ );
  /* sets the color in the current graphics state to "gray".  Each color
     component is set equal to the scaled "gray" value, and the color.gray
     component is set to the scaled current transfer function of "gray". */

extern procedure SetMtx( /* PMtx pmtx */ );
  /* sets the matrix in the current graphics state. */

extern procedure TfmP( /* Cd c; PCd rc; */ );
  /* returns in *rc the Cd result of c * the current transformation matrix. */

extern procedure Tlat( /* Cd c; */ );
  /* premultiplies a translation by (c.x, c.y) * the current transformation
     matrix, storing the result into the current transformation matrix. */

/*	----- Exported Per-Context Data -----			*/

extern PGState	gs;

#endif	GRAPHICS_H
/* v009 pasqua Wed Oct 28 10:16:52 PST 1987 */
/* v010 gass Mon Nov 9 11:49:06 PST 1987 */
/* v011 durham Sun Feb 7 13:40:06 PST 1988 */
/* v012 paxton Sat Feb 20 14:20:39 PST 1988 */
/* v013 paxton Sat Feb 27 10:30:02 PST 1988 */
/* v014 sandman Mon Apr 18 12:16:43 PDT 1988 */
/* v015 durham Wed Jun 15 14:21:15 PDT 1988 */
/* v016 sandman Fri Aug 5 12:42:43 PDT 1988 */
/* v017 caro Wed Nov  9 15:54:41 PST 1988 */
/* v018 pasqua Wed Dec 14 17:49:22 PST 1988 */
/* v019 pasqua Wed Jan 18 11:43:06 PST 1989 */
/* v020 sandman Mon Mar 6 10:55:15 PST 1989 */
/* v021 sandman Tue May 2 10:44:34 PDT 1989 */
/* v022 sandman Fri May 26 12:20:10 PDT 1989 */
/* v023 sandman Mon Oct 16 15:19:23 PDT 1989 */
/* v024 taft Thu Nov 23 15:00:38 PST 1989 */
/* v025 taft Fri Jan 5 15:35:06 PST 1990 */
