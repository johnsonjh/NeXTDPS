/* PostScript Character Scan Conversion

		Copyright 1989 -- Adobe Systems, Inc.
	    PostScript is a trademark of Adobe Systems, Inc.
NOTICE:  All information contained herein or attendant hereto is, and
remains, the property of Adobe Systems, Inc.  Many of the intellectual
and technical concepts contained herein are proprietary to Adobe Systems,
Inc. and may be covered by U.S. and Foreign Patents or Patents Pending or
are protected as trade secrets.  Any dissemination of this information or
reproduction of this material are strictly forbidden unless prior written
permission is obtained from Adobe Systems, Inc.

Original version: Mike Byron.  Oct, 1989.
Port to 2ps: Fred Drinkwater. Jun, 1990.
Edit History:
Scott Byer: Fri Aug 24 15:05:43 1990
End Edit History.
*/

#include "atm.h"

#if ATM

#include "pubtypes.h"
#include "fp.h"
#include "font.h"
#include "buildch.h"
#include "procs.h"

/*
 * Callback procedure array
 */
global PBuildCharProcs bprocs;

#define OutOfMemory() BCERROR(BE_MEMORY)


#else /* ATM */
/*
 * MERCURY and PPS
 */
#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include DEVICE
#include ERROR
#include EXCEPT
#include FP
#include GRAPHICS
#include PSLIB

#if PPS
#include VM
#include "cscan.h"
extern Fixed edgeminx, edgeminy, edgemaxx, edgemaxy;
#else /* PPS */
#include "graphicspriv.h"
#endif /* PPS */

#include "cscancommon.h"

#include "path.h"

/* Control ANSI C protyping (not supported in Sun cc) */
#define PROTOTYPES 0

#define IntX integer

/* #defines used in cscanasm.c */
#define FixedHalf 0x8000
#define FixedOne 0x10000

#if PPS
#define MEMMOVE bcopy
#define MEMZERO bzero
#else
#define MEMMOVE os_bcopy
#define MEMZERO os_bzero
#endif /* PPS */

#define OutOfMemory() LimitCheck()

/*
 * End MERCURY and PPS section
 */
#endif /* ATM */


#define DEBUG 0

#define GRAPHCHAR 0		/* Generate a PostScript description of a char */
#define GRAPHPOINTS 0		/* Just generate PostScript for points */
#define PRINTEDITS 0		/* Print out the pixel edits (dropouts and white) */
#define COUNTCROSSES 0		/* Count different kinds of crosses */
#define EDITDROPOUTS 1		/* Finds dropouts, etc. Usually what crashes. */
#define WHITE 1			/* Turn on code to preserve white connections */

#define BUILD_CROSSES 0		/* Return Cross Array directly. Do not build runs. */

/* Limits for executing white fixup connectivity.  These are not inclusive. */
#define WHITE_LOW  (0x68000L)	/* 6.5 pt */
#define WHITE_HIGH (0x118000L)	/* 17.5 pt */


/* SEGMENT_64K means the processor is an Intel 80*86, and the code being produced
   uses a segmented address space, where segments are limited to 64K bytes. */
#define SEGMENT_64K  (ISP == isp_i80286)

/* To speed up accesses, set NEG_INDICES if the base of an array can be set
   anywhere in memory.  This is used to set the base of the yCross array to
   the virtual "zero" value, so that it can be directly indexed by Y pixel
   values.  For instance, the 8086 cannot do this because the yCross array
   must live in a segment, and the base of yCross may correspond to a positive
   value, which would mean the "zero" value is not inside the segment.  */
#define NEG_INDICES  (!(SEGMENT_64K))


/* Private limits */
#define POINT_ARRAY_LEN 70	/* Number of points to collect before processing */



/*
SUGGESTIONS FOR IMPROVING CODE:
 - Change CScan interface to call CSPathPoints more-or-less directly.  That is,
   create an interface that packages up path points and calls into CScan with
   a whole buffer of them.
 - Look at generated code for examining and setting flags in a Cross.
   See if this can be improved upon with macros or whatever.  (Compilers are
   often not very smart about bitfields.)
 - Make the small path routines inline.  Macro-ize them?  Or at least add a gcc
   flag so that gcc can inline them...  May save code space to keep a callable
   routines as well for the low-volume functions.
 - Could save two bytes in a Cross by putting the xRun field in the low order
   part of the Y value, since you only need xRun when Y is on midline.
   This would complicate the code that looks at Y values, of course...
   (Currently, it would also make a Cross a multiple of 16 bits, rather than
   32 bits -- pay attention to alignment of 32-bit items.)
 - The fixup "move pixel" code checks to see whether it would create any new
   pixel problems if the pixel is moved.  It does this by checking whether
   any of the three pixels that were *not* in the Cxtn for the pixel are on --
   if so, it does not do the move on the assumption that this would create
   a new connection.  The code should get smarter.  The pixels need to
   be checked for connectivity with the original pixel somehow, since this
   would not be a new connectivity problem.
 - The fixup code will not move a pixel outside the original bitmap bbox.
   Seems like it could be smarter than this, at least for moving pixels up.
   It would be nice to be able to move the dots on "i" and "j" up.
   Maybe look at path bbox?  What about font bbox?
 - Could change the code that fills in dropouts to fill in *both* pixels if
   there are multiple intersections on the x-midline that would warrant it.
 - Right now the dropout-fixing code iterates through *all* the xtra Crosses
   between two midlines looking for intersections with a particular vertical
   midline.  Instead, it could check the Crosses at the ends of each path,
   and skip the ones that did not straddle the vertical midline.  This method
   would require looking through the x inflection points as well, since the
   Cross endpoints only define the extent of a path segment in "x" when there
   is no x inflection point in the segment.
 - The white pixel splices (splices into black areas) is probably a bad idea.
   Better to make sure the pixel stays black and just leave it (I think).
   Be careful about how the fixup code would treat things...
 - We currently ignore parts of the character that do not intersect
   horizontal midlines.  At REAL small point sizes, many chars have no pixels.
 - I bet the BuildCxtn/ScanLineCxtn code could be made faster...
*/


#if PROTOTYPES
#if MERCURY
/*
 * Initialization (MERCURY only):
 *  The buffer sizes are NOT specified by the caller (as in non-MERCURY
 *  systems); they have defaults specified in IniCScan() itself.
 */
public procedure IniCScan( InitReason reason );
#else
/* 
 * Initialization (non-MERCURY):
 *  Note that the current code only uses b1 & b1;
 *  b3 & b4 are completely ignored.
 */
public procedure IniCScan( PGrowableBuffer b1, PGrowableBuffer b2,
                           PGrowableBuffer b3, PGrowableBuffer b4);
#endif /* MERCURY */
#endif /* PROTOTYPES */


/**********  Crosses  **********/

/*
   NOTES ABOUT CROSSES

   WARNING:  (This is ahead of the game, but make sure you understand it)
   SplicePixel can *MOVE* a Cross in memory, and leave a link Cross.  Therefore
   all pointers to Crosses should be validated after calling a routine that
   might splice a pixel.  These routines should be commented appropriately.

   A path is represented as a number of arrays of Crosses, which contain the
   x,y coordinates of points on the path.  "Normal" Crosses represent intersections
   with horizontal (y) midlines. "Xtra" Crosses are other points on the path that
   give greater accuracy, like inflection points.  The order of Crosses in the
   path is always the same as the order of Crosses in the arrays.

   The arrays are hooked together by "link" Crosses, which are *always* positioned
   at both ends of each array of Crosses in a path.  The link Crosses are used to
   traverse Crosses in path order -- when the end of an array is encountered,
   the link Cross points to the next Cross in the path, which will be in another
   array.  Link Crosses are used to close a path, and to "splice" more path into
   the original path (these splices are marked so that they can be skipped if
   necessary).

   NOTE:  The "xtra" flag is true for both xtra Crosses and link Crosses.  This
   speeds up finding the real Crosses, which are the most numerous.

   The first link Cross in a path is special -- it is put in a linked list of
   all the paths so that the paths can be enumerated.  The forw field in this
   first link points to the next path.
   WARNING: One must use a loop to find the first real Cross in a path when
   enumerating paths using this linked list.  Splices can add an arbitrary
   number of link Crosses into the array of Crosses.
*/

/* Cross Flags.
   These are small bitfields in the Cross data structure.  We use macros to
   fool with them so that we can optimize access methods for different machine
   architectures if we need to.
   The flags are always initialized to zero by default.
 */
typedef struct _CrossFlags {
  Card8 link:1;		/* True: "link" Cross, for connecting Crosses */
  Card8 xtra:1;		/* True: "xtra" (or link) Cross. xtra path points. */
  Card8 up:1;		/* True: Has up connection */
  Card8 splice:1;	/* True: This Cross was created during a splice */
  Card8 isLeft:1;	/* True: Cross is left side (beginning) of run */
  Card8 deleted:1;	/* True: Pixel deleted on this side of pair */
  Card8 hright:2;	/* Path direction for horiz connection to the right */
  Card8 filler2:2;	/* Push "down" into low-order bits */
  Card8 delta:4;	/* The offset of this Cross for offset-center */
  Card8 down:2;		/* Path direction for connection down (2 bits wide) */
  } CrossFlags;
typedef Card16 CrossFlagField;	/* One integer *exactly* as big as CrossFlags */

typedef union _Cross {
  struct {			/* NORMAL AND XTRA CROSS */
    CrossFlags f;		/* WARNING: Must be first */
    Int16 xRun;			/* The value used for the run for this Cross */
    union _Cross *yNext;	/* Next Cross in same scanline in X order */
    Fixed x;			/* X-intercept */
    Fixed y;			/* Y value at X-intercept */
    } c;

  struct {			/* LINK */
    CrossFlagField allFlags;	/* WARNING: Must be first */
    Int16 filler;		/* Make alignment of following fields 32 bits */
    union _Cross *forw;		/* The Cross in PathForw direction */
    union _Cross *back;		/* The Cross in the PathBack direction */
    } l;
  } Cross, *CrossPtr;

#define BADCROSS  ((CrossPtr)(-1))	/* An illegal value different from NULL */



/* Definitions for path directions.
   Must be set up so that PathBoth == PathForw|PathBack.  */
typedef enum {		/* Type that contains a path direction */
  PathNone = 0,		/* No direction */
  PathForw,		/* Forward */
  PathBack,		/* Backward */
  PathBoth		/* When the path goes both directions... */
  } PathDir;
#define RevPathDir(dir) ((~(dir)) & PathBoth)	/* Reverse of path direction */


/* Structure to describe an intersection with a pixel midline */
typedef struct {
  Fixed y;			/* The point of intersection */
  CrossPtr cross;		/* One of the two Crosses in the intersecting line seg */
  PathDir dir;			/* Direction to other Cross in segment */
  } Intersection, *PIntersection;

#define SetIntersection(i, iy, icross, idir) \
  ((i).y = iy, (i).cross = icross, (i).dir = idir)


#define SubtractPtr(larger, smaller)  ((larger) - (smaller))

#if SEGMENT_64K
/* Need this to correctly get ptr subtraction for entire segment */
#undef SubtractPtr
#define SubtractPtr(larger, smaller) \
  ( (unsigned)((unsigned)(((larger)&0xFFFF)-((smaller)&0xFFFF))/sizeof(*(larger))) )
#endif


/* Offset-center variables */
/* Slopes.  WARNING: This must parallel the SlopesArray. */
typedef enum {
  NOSLOPE,		/* Must be first entry */
  SLOPE00,
  SLOPE45,
  SLOPE60,
  SLOPE75,
  SLOPE90,
  MAXSLOPENUMBER
  } SlopeNumber;
private readonly Fixed SlopesArrayInit[MAXSLOPENUMBER] = {
  0,			/* NOSLOPE.  Must be 0. */
  0xFFFFF000,		/* SLOPE00 */
  0xFFFFB505,		/* SLOPE45 */
  0xFFFFA24C,		/* SLOPE60 */
  0xFFFF90DA,		/* SLOPE75 */
  0xFFFF8000		/* SLOPE90 */
  };
private Fixed SlopesArray[MAXSLOPENUMBER];



#define MAXFixed MAXInt32	/* Maximum Fixed value */
#define MINFixed MINInt32	/* Minimum Fixed value */

#define REG register


/* The direction a path is progressing in.  For BuildCrosses */
#define NoDir		0
#define LeftDir		1
#define RightDir	2
#define VertDir		3
#define UpDir		4
#define DownDir		5
#define HorizDir	6
typedef Int16 SegmentDir;


/* Variables for collecting points in path */
internal FCd pointArray[POINT_ARRAY_LEN+1];	/* One extra point */
internal IntX pointCount;		/* Count of points in pointArray */
private FCd firstPoint;			/* First point in path */
private boolean firstPathBuffer;	/* True: Processing first set of points */
internal boolean havePathCross;		/* True: Have non-xtra Cross in path */
/* Global state preserved across BuildCrosses calls */
internal SegmentDir saveHorizDir;	/* Horizontal direction of path */
internal SegmentDir saveVertDir;	/* Vertical direction of path */
internal CrossPtr savePrevCross;	/* Previous non-xtra Cross */
internal FCd savePrevPoint;		/* Last point in prev point buffer */
private Cross fakePrevCross;		/* Somewhere to point first previous Cross */

/* Allocate a new Cross */
#define NEWCROSS(cross) { if (((cross)=currentCross++) == limitCross) OutOfMemory(); }

#if NEG_INDICES
#define YCROSS(pixel) (yCross[pixel])	/* Entry for a scanline in yCross array */
#else
#define YCROSS(pixel) (yCross[(pixel)+yCrossZero])
private IntX yCrossZero;
#endif  /* NEG_INDICES */


/* Convert an integer pixel number to a Fixed midline */
#define MidPixel(pix) ((Fixed)(FixInt(pix) | 0x8000L))
/* Convert a pixel number to a Fixed number at pixel origin */
#define PixelOrigin(pix) (FixInt(pix))
/* Convert Fixed coordinate to a pixel number.  Should work for both x and y. */
#define Pixel(coord) (FTrunc(coord))

#define ExchangeCrossPtr(c1, c2) \
  { CrossPtr ctmp;  ctmp = (c1); (c1) = (c2); (c2) = ctmp; }

#if MERCURY
/*
 * MERCURY buffers are malloc'd, realloc'd to grow, and have locally
 * maintained start and length fields.
 * NOTE that a run requires a contiguous piece of memory, has no
 * internal pointers, and is externally referenced only by its
 * start address and length.  Therefore realloc'ing (with copy)
 * does not require any special pointer finagling.
 * HOWEVER, also note that buffer growing is called from within
 * a routine (ReturnBits()) which DOES maintain pointers into
 * the run; these must be massaged when the buffer is realloc'd
 * (see all conditionally-compiled versions of CHECKRUNBUFF()).
 * NOTE: I think that only short alignment is required for the run buffer.
 */
/* 
 * NOTE: This typedef must match bc/buildch.h:GrowableBuffer
 * both as to field types and names.  This is to allow sharing code
 * in this file.
 */
typedef struct _CScanBuffer {
  char *ptr;
  Card32 len;
} CScanBuffer,*PCScanBuffer;

private CScanBuffer mb1;	/* Main memory area to allocate from */
private PCScanBuffer memoryBuffer1;
private CScanBuffer mb2;	/* We use this for the run */
private PCScanBuffer memoryBuffer2;
/*
 * Since MERCURY systems do not call IniCScan with buffer size arguments,
 * we define initial sizes here.  There's nothing special about these
 * sizes (yet).
 * NOTE: Per Mike Byron, 10-15KB is typically enough Cross memory
 * to handle up to a 30-point character on a 300-dpi device.
 * The maximum requirement is unknown now.
 */
#define INICSCAN_B1LEN 1*1024
#define INICSCAN_B2LEN 1*1024

#else
/*
 * Non-MERCURY systems use growable buffers.  Their initial sizes are 
 * passed in to IniCScan().
 */
private PGrowableBuffer memoryBuffer1;	/* Main memory area to allocate from */
private PGrowableBuffer memoryBuffer2;	/* We use this for the run */
#endif /* MERCURY */
/*
 * All systems use the same increment for "growing" memoryBuffer2
 */
#define MB1ALLOCINCREMENT (Int32)sizeof(Cross)*1024
#define MB2ALLOCINCREMENT (Int32)sizeof(Int16)*10


private CrossPtr firstCross;		/* First Cross we generate */
internal CrossPtr currentCross;		/* Space for the next Cross we generate */
internal CrossPtr limitCross;		/* The last Cross we can generate */
internal CrossPtr *yCross;		/* Array of CrossPtr's, one for each Y */

private CrossPtr startLink;		/* Initial link Cross in current path */
private CrossPtr oldStartLink;		/* Previous version of startLink */

internal Fixed yPathMin, yPathMax, xPathMin, xPathMax;	/* Path limits */
private Int16 yBoxMin, yBoxMax, xBoxMin, xBoxMax;	/* CharBBox limits */
private boolean checkRuns;		/* True: Run pairs may be deleted or out of order */

/* Global flags that control execution of certain features */
private boolean offsetCenterFlag;	/* True: do offset-center algorithm */
private boolean keepPointsFlag;		/* True: Make Crosses of all points */
#if WHITE
private boolean whiteFixupFlag;		/* True: Do white connectivity fixup */
#endif

/* Lists of xtra Crosses that are x inflection points */
private CrossPtr minXInflections;	/* Minimum X inflection points */
private CrossPtr maxXInflections;	/* Maximum X inflection points */
internal CrossPtr pathMaxX;		/* Max X inflections for current path */
internal CrossPtr pathMinX;		/* Min X inflections for current path */

#if COUNTCROSSES
static Int32 crossCount;		/* Normal Crosses */
static Int32 xtraCrossCount;		/* Xtra Crosses */
static Int32 linkCrossCount;		/* Link Crosses */
#endif




#if PROTOTYPES
private CrossPtr NewCross(Fixed x, Fixed y);
private CrossPtr NewLinkCross(void);
private CrossPtr NewXtraCross(Fixed x, Fixed y);
internal CrossPtr ForwPathCross(CrossPtr cp);
internal CrossPtr BackPathCross(CrossPtr cp);
internal CrossPtr PathXtraCross(CrossPtr cp, PathDir dir);
internal CrossPtr PathUpCross(CrossPtr cp);
internal CrossPtr PathXtraOrig(CrossPtr cp, PathDir dir);
private procedure FixCrossFlags(CrossPtr start, CrossPtr end);
private procedure FinishPath(boolean haveCrosses);
private procedure ExpandCharBBox(IntX x, IntX y);
internal CrossPtr RunPair(CrossPtr cross);
internal Fixed YCrossing(CrossPtr c1, CrossPtr c2, Fixed xval);
private procedure DoXInflections(void);
private procedure SetLeftFlag(CrossPtr cross);
private procedure StartSplice(CrossPtr startc);
private procedure FinishSplice(void);
private procedure AddYCross(CrossPtr cross);
private procedure SplicePixel(IntX x, IntX y, CrossPtr cross, PathDir dir);
private boolean ExtendRunDropOut(IntX x, IntX y, PIntersection ip);
private boolean SetPixelInDropOut(IntX x, PIntersection iknown, PIntersection iother,
				  boolean splice);
private procedure FillInInflection(CrossPtr known, PathDir knownDir, IntX xCol);
private boolean FillInColumn(CrossPtr known, PathDir knownDir, IntX xCol,
			     boolean blackAbove, boolean splice);
private procedure FillInRange(CrossPtr known, PathDir knownDir, IntX start, IntX end,
		              boolean blackAbove);
private procedure FillInPairs(CrossPtr p1, CrossPtr p2, CrossPtr known, PathDir knownDir,
			      boolean connectLeft);
private CrossPtr ConnectedRunOpposite(CrossPtr cross, boolean below);
private procedure CheckHorizWhite(CrossPtr ll, CrossPtr rl, CrossPtr known,
				  PathDir knownDir);
private procedure ReturnBits(procedure(*callBackProc)());
private procedure CheckBlackDown(CrossPtr topc, CrossPtr botc);
private procedure EditBlackHoriz(CrossPtr cross, PathDir dir);
private procedure EditBlackSpace(void);
private boolean BuildYCross(void);
private procedure BuildInitialRuns(boolean doOffsetCenter);
internal procedure BuildCrosses(PFCd points, IntX count, boolean allPoints, boolean offset);
internal procedure CSPathPoints(PFCd buff, IntX count, boolean closePath);
private procedure InitNewPath(void);
#if !ATM
private procedure CSNewFCd(FCd c);
#endif /* !ATM */
#if WHITE || DEBUG
private boolean PixelIsBlack(IntX x, IntX y);
#endif
#if WHITE
private procedure ConnectWhitePairs(CrossPtr top, CrossPtr bottom);
private procedure ExpandWhiteDropOuts(void);
private procedure EditWhiteHoriz(CrossPtr startLeft, PathDir dir);
private procedure EditWhiteSpace(void);
#endif  /* WHITE */
#if !USE68KATM
private SlopeNumber OffsetCenterSlope(Fixed xDelta);
#endif

#else
/* Non-PROTOTYPING compilers */
private CrossPtr NewCross(/* Fixed x, Fixed y */);
private CrossPtr NewLinkCross(/* void */);
private CrossPtr NewXtraCross(/* Fixed x, Fixed y */);
internal CrossPtr ForwPathCross(/* CrossPtr cp */);
internal CrossPtr BackPathCross(/* CrossPtr cp */);
internal CrossPtr PathXtraCross(/* CrossPtr cp, PathDir dir */);
internal CrossPtr PathUpCross(/* CrossPtr cp */);
internal CrossPtr PathXtraOrig(/* CrossPtr cp, PathDir dir */);
private procedure FixCrossFlags(/* CrossPtr start, CrossPtr end */);
private procedure FinishPath(/* boolean haveCrosses */);
private procedure ExpandCharBBox(/* IntX x, IntX y */);
internal CrossPtr RunPair(/* CrossPtr cross */);
internal Fixed YCrossing(/* CrossPtr c1, CrossPtr c2, Fixed xval */);
private procedure DoXInflections(/* void */);
private procedure SetLeftFlag(/* CrossPtr cross */);
private procedure StartSplice(/* CrossPtr startc */);
private procedure FinishSplice(/* void */);
private procedure AddYCross(/* CrossPtr cross */);
private procedure SplicePixel(/* IntX x, IntX y, CrossPtr cross, PathDir dir */);
private boolean ExtendRunDropOut(/* IntX x, IntX y, PIntersection ip */);
private boolean SetPixelInDropOut(/* IntX x, PIntersection iknown, PIntersection iother,
				  boolean splice */);
private procedure FillInInflection(/* CrossPtr known, PathDir knownDir, IntX xCol */);
private boolean FillInColumn(/* CrossPtr known, PathDir knownDir, IntX xCol,
			     boolean blackAbove, boolean splice */);
private procedure FillInRange(/* CrossPtr known, PathDir knownDir, IntX start, IntX end,
		              boolean blackAbove */);
private procedure FillInPairs(/* CrossPtr p1, CrossPtr p2, CrossPtr known, PathDir knownDir,
			      boolean connectLeft */);
private CrossPtr ConnectedRunOpposite(/* CrossPtr cross, boolean below */);
private procedure CheckHorizWhite(/* CrossPtr ll, CrossPtr rl, CrossPtr known,
				  PathDir knownDir */);
private procedure ReturnBits(/* procedure(*callBackProc)() */);
private procedure CheckBlackDown(/* CrossPtr topc, CrossPtr botc */);
private procedure EditBlackHoriz(/* CrossPtr cross, PathDir dir */);
private procedure EditBlackSpace(/* void */);
private boolean BuildYCross(/* void */);
private procedure BuildInitialRuns(/* boolean doOffsetCenter */);
internal procedure BuildCrosses(/* PFCd points, IntX count, boolean allPoints, boolean offset */);
internal procedure CSPathPoints(/* PFCd buff, IntX count, boolean closePath */);
private procedure InitNewPath(/* void */);
#if !ATM
private procedure CSNewFCd(/* FCd c */);
#endif /* !ATM */
#if WHITE || DEBUG
private boolean PixelIsBlack(/* IntX x, IntX y */);
#endif
#if WHITE
private procedure ConnectWhitePairs(/* CrossPtr top, CrossPtr bottom */);
private procedure ExpandWhiteDropOuts(/* void */);
private procedure EditWhiteHoriz(/* CrossPtr startLeft, PathDir dir */);
private procedure EditWhiteSpace(/* void */);
#endif  /* WHITE */
#if !USE68KATM
private SlopeNumber OffsetCenterSlope(/* Fixed xDelta */);
#endif
#endif /* PROTOTYPES */

/* Macro for rotating a byte left.
   This does *not* work for negative shift values.
   It always modifies the argument in place.
 */
#define ROTATELEFT8(p, bits) ( (p<<bits) | (((Card8)p)>>(8-bits)) )

#if 0			/* Never mind -- this doesn't help.  Left for reference. */
#undef ROTATELEFT8
#define ROTATELEFT8(p, bits) (InlineRotateLeft(p, bits))

private inline Card8 InlineRotateLeft(p, bits)
  Card8 p;
  Int32 bits;
  {
  asm ("rol%.b %2,%0" : "=d" (p) : "0" (p), "d" (bits));
  return p;
  }
#endif



#if !DEBUG
#define DEBUGERROR(exp, str)   /* This is intended to be compiled out in production code */
#else
#define DEBUGERROR(exp, str)   { if (exp) DebugAbort(str); }
private procedure DebugAbort(char *str);
private procedure DebugAbort(str)
  char *str;
  {
#if !MACROM
  fprintf(stderr, "ERROR -- %s\n", str);
  printf("ERROR -- %s\n", str);
  exit(1);
#else
  DebugStr(str);
#endif
  }
#endif  /* DEBUG */


/* Allocate a new Cross structure */
private CrossPtr NewCross(x, y)
  Fixed x, y;			/* Coordinates of cross */
  {
  REG CrossPtr p;		/* Current Cross structure */

  NEWCROSS(p);
  p->c.x = x;  p->c.y = y;
  p->l.allFlags = 0;

#if COUNTCROSSES
  crossCount++;
#endif

  return p;
  }

/* Allocate a new xtra Point.  Like Cross, but just helps fill in dropouts. */
private CrossPtr NewXtraCross(x, y)
  Fixed x, y;
  {
  REG CrossPtr p;		/* Current Cross structure */

  NEWCROSS(p);
  p->c.x = x;
  p->c.y = y;
  p->l.allFlags = 0;
  p->c.f.xtra = true;

#if COUNTCROSSES
  xtraCrossCount++;
#endif

  return p;
  }

/* Allocate a link Cross to link together Cross arrays into a path.
   SEE ALSO: StartSplice routine, which creates one of these in the middle of a path. */
private CrossPtr NewLinkCross()
  {
  REG CrossPtr p;

  NEWCROSS(p);
  p->l.allFlags = 0;
  p->c.f.link = true;
  p->c.f.xtra = true;
  p->l.forw = p->l.back = NULL;

#if COUNTCROSSES
  linkCrossCount++;
#endif

  return p;
  }


/* Get the next Cross in the "dir" direction of the path */
#define PathCross(p, dir) ( (dir) == PathForw? ForwPathCross(p) : BackPathCross(p) )


#if !USE68KATM
#include "cscanasm.c"
#endif /* USE68KATM */


/* Fix the flags in a set of Crosses.  The Crosses are defined by the start and end
   Cross, and go from start to end in PathForw direction.
   Both start and end Crosses are modified.
   WARNING:  Do not call this routine until the Crosses in the path have been set up.
   It uses adjacent Crosses to set flags.  */
private procedure FixCrossFlags(start, end)
  CrossPtr start, end;
  {
  REG CrossPtr cross;			/* The current Cross */
  REG CrossPtr c;

  cross = start;
  while (true) {				/* Init */
    cross->c.f.down = cross->c.f.hright = PathNone;
    cross->c.f.up = false;

    c = PathCross(cross, PathForw);		/* Look forward */
    if (cross->c.y > c->c.y)
      cross->c.f.down = PathForw;
    else if (cross->c.y < c->c.y)
      cross->c.f.up = true;
    else if (cross->c.x < c->c.x)
      cross->c.f.hright |= PathForw;

    c = PathCross(cross, PathBack);		/* Look backward */
    if (cross->c.y > c->c.y)
      cross->c.f.down = PathBack;
    else if (cross->c.y < c->c.y)
      cross->c.f.up = true;
    else if (cross->c.x < c->c.x)
      cross->c.f.hright |= PathBack;

    if (cross == end)				/* Loop */
      break;
    cross = PathCross(cross, PathForw);
    }
  }


/* Do final processing on a path */
private procedure FinishPath(haveCrosses)
  boolean haveCrosses;			/* True: The path has non-xtra Crosses */
  {
  CrossPtr lastLink;			/* Last link Cross in this path */
  CrossPtr lastCross;			/* Last Cross in this path */
  CrossPtr c, prevc;
  IntX ifix;

  DEBUGERROR(!startLink->c.f.xtra, "FinishPath: First Cross must be xtra Cross");

  /* Fill in start and end link Crosses */
  lastCross = currentCross - 1;
  startLink->l.back = lastCross;
  lastLink = NewLinkCross();
  lastLink->l.forw = startLink + 1;
  oldStartLink = startLink;
  startLink = NewLinkCross();		/* First Cross in next path */
  oldStartLink->l.forw = startLink + 1;

  /* Make sure mins and maxes are correct */
  ifix = lastCross->c.x;
  if (ifix < xPathMin) xPathMin = ifix;
  if (ifix > xPathMax) xPathMax = ifix;
  ifix = lastCross->c.y;
  if (ifix < yPathMin) yPathMin = ifix;
  if (ifix > yPathMax) yPathMax = ifix;

  /*
   * Export path bounding box
   */
  edgeminx = xPathMin;
  edgemaxx = xPathMax;
  edgeminy = yPathMin;
  edgemaxy = yPathMax;


  if (haveCrosses) {
    if (lastCross->c.f.xtra)
      lastCross = PathCross(lastCross, PathBack);
    FixCrossFlags(lastCross, PathCross(lastCross,PathForw));

    /* Do the x inflection point processing for the first point */
    prevc = oldStartLink + 1;
    c = PathXtraCross(prevc, PathForw);
    if (prevc->c.x <= c->c.x) {
      if (prevc->c.x != c->c.x) {		/* Last connection going RIGHT */
	if (saveHorizDir != RightDir) {
	  XMIN:
	  prevc->c.yNext = pathMinX;  pathMinX = prevc;
	  }
	}
      else {
	if (saveHorizDir != VertDir) {		/* Last connection VERTICAL */
	  if (saveHorizDir == LeftDir)
	    goto XMIN;
	  else
	    goto XMAX;
	  }
	}
      }
    else {					/* Last connection going LEFT */
      if (saveHorizDir != LeftDir) {
	XMAX:
	prevc->c.yNext = pathMaxX;  pathMaxX = prevc;
	}
      }

    /* Add path inflections to inflection lists */
    c = pathMinX;
    if (c != NULL) {
      for (; c->c.yNext!=NULL; c=c->c.yNext);
      c->c.yNext = minXInflections;
      minXInflections = pathMinX;
      }
    c = pathMaxX;
    if (c != NULL) {
      for (; c->c.yNext!=NULL; c=c->c.yNext);
      c->c.yNext = maxXInflections;
      maxXInflections = pathMaxX;
      }
    }

  InitNewPath();
  }


/* Process a new buffer of path points.
   WARNING: This routine assumes there is space for one more point at the end of
   the point buffer.  */
internal procedure CSPathPoints(buff, count, closePath)
  PFCd buff;			/* Buffer of points */
  IntX count;			/* Number of points in buff */
  boolean closePath;		/* True: Last buffer -- close path */
  {
  PFCd start, end;

  if (firstPathBuffer) {
    DEBUGERROR(count == 0, "CSClose: No moveto before closepath");
    pathMinX = pathMaxX = NULL;
    havePathCross = false;
    /* First point gets recorded as y inflection point.  This has no side-effects
       on data structures, but does make sure the point makes an xtra Cross.  */
    saveVertDir = NoDir;
    savePrevCross = &fakePrevCross;
    savePrevCross->c.y = MAXFixed;
    start = buff;  end = buff + 1;

    /*
     * If this is the first buffer of the first path of the
     * character, then initialize the path bbox vars to the
     * first pair of points.
     */
    if (xPathMin == MAXFixed) {
      /* 
       * Path bbox vars still have 'pre-init' values,
       * so, initialize them to the first points.
       */
      if (start->x < end->x) {
        xPathMin = start->x;
        xPathMax = end->x;
      } else {
        xPathMin = end->x;
        xPathMax = start->x;
      }
      if (start->y < end->y) {
        yPathMin = start->y;
        yPathMax = end->y;
      } else {
        yPathMin = end->y;
        yPathMax = start->y;
      }
    }

    saveHorizDir = start->x < end->x? RightDir : start->x > end->x? LeftDir: VertDir;
    savePrevPoint = firstPoint = buff[0];
    buff++;  count--;		/* First point in path is "previous point" */
    }

  if (!closePath) {		/* Not end of path -- do full buffer */
    BuildCrosses(buff, count, keepPointsFlag, offsetCenterFlag);
    firstPathBuffer = false;
    }
  else {			/* Make sure last point is same as first point */
    if (buff[count-1].x != firstPoint.x || buff[count-1].y != firstPoint.y)
      buff[count++] = firstPoint;
    if (!firstPathBuffer || count >= 3) {	/* Skip degenerate path */
      BuildCrosses(buff, count, keepPointsFlag, offsetCenterFlag);
      FinishPath(havePathCross);
#if GRAPHCHAR || GRAPHPOINTS
      printf("CloseRP\n");		/* CloseRP  == Close path for raw points */
#endif
      }
    }
  }


/* Close the current path */
public procedure CSClose()
  {
  CSPathPoints(pointArray, pointCount, true);
  }


/* Initialize things for a new path (NOT a new character) */
private procedure InitNewPath()
  {
  pointCount = 0;
  firstPathBuffer = true;
  }


#if !ATM
private procedure CSNewFCd(c)
  FCd c;
{
  CSNewPoint(&(c));
}

/* Resize the cross buffer  and reset CScan.  This happens  when a limitCheck
   is  caught by the  outer  CScan  calling loop.  Then  we  try to build the
   character again.							   */

public procedure ResizeCrossBuf()
  {
   Int16	*newBase;
   Card32	newLength = memoryBuffer1->len + MB1ALLOCINCREMENT;
   
   if (!(newBase = (Int16 *)os_realloc(memoryBuffer1->ptr, newLength)))
     OutOfMemory();
   memoryBuffer1->ptr = (char *)newBase;
   memoryBuffer1->len = newLength;
  }

#endif /* !ATM */

/* Start a character rasterization */
public procedure ResetCScan(fixupOK, locking, len1000, rndwidth, idealwidth, gsfactor)
  boolean fixupOK;		/* Do "fixup" (white connectivity) if appropriate */
  boolean locking;		/* Some part of the char may be "locked" in fontbuild */
  Fixed len1000;		/* Approximation of point size */
  Fixed rndwidth;		/* Stem width rounded to integer */
  Fixed idealwidth;		/* Actual stem width */
  integer gsfactor;
  {

  /* Things that should really be compile-time assertions */
  DEBUGERROR(sizeof(CrossFlags) != sizeof(CrossFlagField),"CScan: flags size mismatch");

  firstCross = currentCross = (CrossPtr)memoryBuffer1->ptr;
  limitCross = firstCross + memoryBuffer1->len/sizeof(Cross);

  yPathMin = xPathMin = MAXFixed;
  yPathMax = xPathMax = MINFixed;

  /* Leave a slot for the "previous" Cross before the first */
  startLink = NewLinkCross();
  oldStartLink = NULL;

  minXInflections = maxXInflections = NULL;

  checkRuns = false;

  InitNewPath();

#if GRAPHCHAR || GRAPHPOINTS
  printf("ResetScan\n");
#endif

#if COUNTCROSSES
  crossCount = xtraCrossCount = linkCrossCount = 0;
#endif

  /* Set up various flags to control things here */
  keepPointsFlag = (len1000 < FixInt(34));
#if WHITE
  whiteFixupFlag = (fixupOK && locking && len1000>WHITE_LOW && len1000<WHITE_HIGH);
#endif
  offsetCenterFlag = (rndwidth!=0 && idealwidth!=0 && rndwidth<=FixedOne);
  if (offsetCenterFlag) {			/* Initialize SlopesArray */
    Fixed g, oc;
    IntX i;
    MEMMOVE(SlopesArrayInit, SlopesArray, sizeof(SlopesArrayInit));
    if (idealwidth > FixedOne) {
      g = (idealwidth << 1) - FixedOne;
      for (i = SLOPE00; i < MAXSLOPENUMBER; i++) {
        oc = fixmul(SlopesArray[i], g);
	if (oc < -FixedOne) oc = -FixedOne;
	SlopesArray[i] = oc;
	}
      }
    }
#if !ATM
  InitFontFlat(CSNewFCd);
#endif /* !ATM */
  }


/* Initialize memory from buffers */
#if !MERCURY
/*
 * Init for ATM and PPS
 */
public procedure IniCScan(b1, b2, b3, b4)
  PGrowableBuffer b1, b2, b3, b4;
  {
  memoryBuffer1 = b1;	/* For crosses; never resized or moved */
  memoryBuffer2 = b2;	/* May be grown (via CHECKRUNBUFF) */
  }

#else
/*
 * Init for MERCURY
 */
public procedure IniCScan(reason)
       InitReason reason;
{
  switch (reason) {
    case init:
      mb1.len = INICSCAN_B1LEN;
      mb1.ptr  = (char *)os_malloc(INICSCAN_B1LEN);
      memoryBuffer1 = &mb1;
      mb2.len = INICSCAN_B2LEN;
      mb2.ptr  = (char *)os_malloc(INICSCAN_B2LEN);
      memoryBuffer2 = &mb2;
      break;
    default: ;
      /*
       * This function is called several times for "reasons" other
       * than "init", but those calls don't apply to cscan.
       */
  }
}


#endif /* !MERCURY */
#if GRAPHCHAR			/* Print out the bitmap */

private procedure BitmapByte(CardX);
private procedure PrintBitmap();

/* Print out a character bitmap */
private char hexchars[] = "0123456789ABCDEF";

private procedure BitmapByte(val)
  register CardX val;
  {
  putc(hexchars[val>>(8-4)], stdout);
  putc(hexchars[(val>>(8-8))&0xF], stdout);
  }

/* Build procs variables */
global unsigned char *bmbase;
global Int32 bmsize;
global Card16 bmbytewidth, bmscanlines;
global DevBBox charBBox;

private procedure PrintBitmap()
  {
  unsigned char *base = bmbase;
  PDevBBox bbox = &charBBox;
  DevCd ur;
  Int16 width;
  Int16 scanline, byte;
  unsigned char *p;

  ur.x = bbox->ur.x - bbox->ll.x;
  ur.y = bbox->ur.y - bbox->ll.y;
  width = (ur.x+8-1) / 8;

  printf("%d %d IM\n", ur.x, ur.y);
  p = base;
  for (scanline=0; scanline<ur.y; scanline++) {
    for( byte=0; byte<width; byte++) {
      BitmapByte(p[byte]);
      }
    putc('\n', stdout);
    p += bmbytewidth;
    }
  }

#endif  /* GRAPHCHAR */


/* Allocate and build the yCross array, and initialize some global Y state.
   Returns true if there are some pixels in the bitmap. */
private boolean BuildYCross()
  {
  IntX yMax, yMin;
  Int32 yCrossSize;			/* Size of yCross *in Crosses* */
  REG CrossPtr finalCross;		/* Old last Cross */
  REG CrossPtr cross;

  if (yPathMax == MINFixed)		/* No path points */
    return false;

  /* Leave room for one extra scanline on each side to show an empty scanline,
     and make sure there is enough room for splicing. "3" is conservative.  */
  yMax = Pixel(yPathMax) + 3;
  yMin = Pixel(yPathMin) - 3;

  yCrossSize = (Int32)(yMax-yMin+1)*sizeof(CrossPtr);	/* Number of bytes we need */
  yCrossSize = yCrossSize/sizeof(Cross) + 1;	/* Number of Crosses in this space */
  finalCross = currentCross;

  /* Allocate and zero out the yCross array */
  if (SubtractPtr(limitCross,currentCross) < yCrossSize)
    OutOfMemory();
  yCross = (CrossPtr *)currentCross;
  MEMZERO(yCross, yCrossSize*sizeof(Cross));
  currentCross += yCrossSize;
#if NEG_INDICES
  yCross -= yMin;
#else
  yCrossZero = -yMin;
#endif

  /* Put all the Crosses in the yCross array */
  /* NOTE: This code is duplicated in AddYCross */
  for (cross=firstCross; cross!=finalCross; cross++) {
    REG CrossPtr *yList, yp, prevyp;
    if (!cross->c.f.xtra) {
      yList = &YCROSS(Pixel(cross->c.y));
      yp = *yList;
      if (yp == NULL) {
	*yList = cross;
	cross->c.yNext = NULL;
	}
      else {
	prevyp = NULL;
	while (yp != NULL) {
	  if (yp->c.x > cross->c.x)	/* Insert before this entry? */
	    break;
	  prevyp = yp;
	  yp = yp->c.yNext;
	  }
	if (prevyp == NULL)		/* Insert at beginning of list */
	  *yList = cross;
	else
	  prevyp->c.yNext = cross;
	cross->c.yNext = yp;
	}
      }
    }

  /* Set yBoxMax and yBoxMin */
  for (yBoxMax=yMax-1; YCROSS(yBoxMax)==NULL; yBoxMax--) {
    if (yBoxMax == yMin)
      return false;
    }
  for (yBoxMin=yMin+1; YCROSS(yBoxMin)==NULL; yBoxMin++);

  return true;
  }


/* Set the x-intercepts for all Crosses.  This creates the initial "runs" for the
   characters -- which pixels will be black.  These are modified by various
   passes later.  */
private procedure BuildInitialRuns(offset)
  boolean offset;			/* Do offset-center algorithm */
  {
  REG IntX yCount;
  REG CrossPtr *yc;
  REG CrossPtr c1, c2;			/* The pair of crosses we are looking at */
  REG Int16 leftRun, rightRun;		/* xRun values */
  REG IntX xmin, xmax;			/* Local copies for optimization */

  xmin = MAXInt16;
  xmax = MINInt16;

  yc = &YCROSS(yBoxMin);

  /* Loop through scanlines */
  for (yCount=yBoxMax-yBoxMin; yCount>=0; yCount--) {
    c1 = *yc;
    if (c1 != NULL) {
      while (true) {
	c1->c.f.isLeft = true;
	c2 = c1->c.yNext;
	DEBUGERROR(c2 == NULL, "BuildInitialRuns: Odd number of intersections");
        if (offset) {
	  Fixed delta1, delta2;
	  delta1 = SlopesArray[c1->c.f.delta];
	  delta2 = SlopesArray[c2->c.f.delta];
	  c1->c.x -= delta1;  c2->c.x += delta2;
	  if (c1->c.x >= c2->c.x) {
	    c1->c.x = (c1->c.x + delta1 + c2->c.x - delta2) >> 1;
	    c2->c.x = c1->c.x + 1;
	    }
	  }
	leftRun = FRound(c1->c.x);
	rightRun = FRound(c2->c.x);
	if (leftRun != rightRun) {	/* Run crosses at least one x midline */
	  c1->c.xRun = leftRun;
	  c2->c.xRun = rightRun;
	  }
	else {				/* Run doesn't cross x midline */
	  c1->c.xRun = Pixel((c1->c.x+c2->c.x)>>1);
	  c2->c.xRun = c1->c.xRun + 1;
	  }
	c1 = c2->c.yNext;
	if (c1 == NULL) {
	  if (c2->c.xRun > xmax)	/* Last run in scanline */
	    xmax = c2->c.xRun;
	  break;
	  }
	}
      c1 = *yc;				/* Have to do this after xRun is set */
      if (c1->c.xRun < xmin)
	xmin = c1->c.xRun;
      }
    yc++;
    }

  xBoxMin = xmin;
  xBoxMax = xmax - 1;		/* This is one past the actual maximum pixel */
  }


/* Make the character bitmap bigger if necessary */
private procedure ExpandCharBBox(x, y)
  IntX x, y;
  {

  /* Get correct character bbox */
  if (x < xBoxMin) xBoxMin = x;
  if (x > xBoxMax) xBoxMax = x;
  if (y < yBoxMin) yBoxMin = y;
  if (y > yBoxMax) yBoxMax = y;
  }


/* Call this to check a CrossPtr after possible splicing */
#define ValidateCrossPtr(ptr) \
    { while ((ptr)->c.f.link) ptr = ptr->l.forw; }


/* WARNING:  Two or more link Crosses can end up adjacent to each other in a
   segment of the path.  Therefore, only *one* link in the link Cross is
   guaranteed to be accurate -- the link that is used to get the next Cross
   in the opposite direction of the adjacent Cross.
 */

/* Some global variables that are only used for splicing in new paths */
private CrossPtr startSplice;		/* The first link Cross in the splice */
private CrossPtr spliceInsert;		/* The insertion point of the splice */

/* This routine begins construction of some Crosses that are to be added into a path.
   NOTE: Splices are always appended after a Cross, never inserted before one.
   They must always be built in PathForw order as well.  */
private procedure StartSplice(startc)
  CrossPtr startc;		/* The Cross this splice will be appended to */
  {
  CrossPtr c;
  CrossPtr newc;		/* New location of Cross that was moved */
  IntX y;

  startSplice = NewLinkCross();
  startSplice->c.f.splice = true;
  spliceInsert = startc;

  /* If the Cross following startc is a link Cross, we just change link ptrs.
     If it is not, we need to move startc into the splice, and put a link Cross
     in its place.
     WARNING:  This routine should not clobber startc, so that ValidateCrossPtr
     need only be called after FinishSplice.  */

  if (!(startc+1)->c.f.link) {
    NEWCROSS(newc);
#if COUNTCROSSES
    crossCount++;
#endif
    *newc = *startc;	/* New copy of startc Cross */
    if (!startc->c.f.xtra) {	/* Somebody points to startc -- fix pointer */
      y = Pixel(startc->c.y);
      c = YCROSS(y);
      if (c == startc) {
	YCROSS(y) = newc;
	}
      else {
	while (c->c.yNext != startc)
	  c = c->c.yNext;
	c->c.yNext = newc;
	}
      }
    }
  }


/* Finish a splice.  This mostly fixes up all the link Cross ptrs. */
private procedure FinishSplice()
  {
  CrossPtr endSplice;		/* The link Cross at the end of the splice */
  CrossPtr firstSplice;		/* First Cross in splice to mark "splice" */

  endSplice = NewLinkCross();

  /* Link Cross directly after insertion point? */
  if ((spliceInsert+1)->c.f.link) {
    CrossPtr oldEnd;		/* The link Cross PathForw from insertion point */

    firstSplice = startSplice + 1;
    oldEnd = spliceInsert + 1;
    endSplice->l.forw = oldEnd->l.forw;
    oldEnd->l.forw = startSplice + 1;
    DEBUGERROR(!(endSplice->l.forw-1)->c.f.link, "FinishSplice: Invalid back link 1");
    (endSplice->l.forw-1)->l.back = endSplice - 1;
    startSplice->l.back = spliceInsert;
    }
  else {
    CrossPtr backc;		/* The PathBack Cross from startc */
    CrossPtr linkInsert;	/* Link Cross at spliceInsert (same mem, diff struct) */

    firstSplice = startSplice + 2;	/* Skip first one -- it was copied */

    /* Make spliceInsert a link Cross */
    linkInsert = spliceInsert;
    linkInsert->c.f.link = linkInsert->c.f.xtra = true;
    linkInsert->l.forw = startSplice + 1;	/* So ValidateCrossPtr will work */

    /* If there is a link behind spliceInsert, need to repair it */
    backc = spliceInsert - 1;
    if (backc->c.f.link)
      backc = backc->l.back;

    /* Fix the forward and back links for beginning of splice */
    DEBUGERROR(!(backc+1)->c.f.link, "FinishSplice: Invalid back link 2");
    (backc+1)->l.forw = startSplice + 1;
    startSplice->l.back = backc;

    linkInsert->l.back = endSplice - 1;
    endSplice->l.forw = spliceInsert + 1;
    }

  /* Mark all new Crosses (including endSplice) as "splice" */
  while (true) {
    firstSplice->c.f.splice = true;
    if (firstSplice == endSplice)
      break;
    firstSplice++;
    }
  }


/* Add a new Cross to the yCross array */
private procedure AddYCross(cross)
  CrossPtr cross;
  {
  CrossPtr *yList, yp, prevyp;

  if (!cross->c.f.xtra) {
    yList = &YCROSS(Pixel(cross->c.y));
    yp = *yList;
    prevyp = NULL;
    while (yp != NULL) {
      if (yp->c.x > cross->c.x)	/* Insert before this entry? */
	break;
      prevyp = yp;
      yp = yp->c.yNext;
      }
    if (prevyp == NULL)		/* Insert at beginning of list */
      *yList = cross;
    else
      prevyp->c.yNext = cross;
    cross->c.yNext = yp;
    }
  }


/* Set the isLeft flag in a Cross */
private procedure SetLeftFlag(cross)
  CrossPtr cross;
  {
  CrossPtr c1, c2;

  c1 = YCROSS(Pixel(cross->c.y));
  while (true) {
    DEBUGERROR(c1 == NULL, "SetLeftFlag: No cross");
    if (c1 == cross) {
      c1->c.f.isLeft = true;
      break;
      }
    c2 = c1->c.yNext;
    if (c2 == cross)
      break;
    c1 = c2->c.yNext;
    }
  }


/* Splice in some path that will make a pixel black.  The line segment to add the
   splice into is a Cross plus the next Cross in the PathForw direction.  */
#define SPLICEDELTA (0x0001L)	/* The distance between synthetic points and midlines */
private procedure SplicePixel(x, y, cross, dir)
  IntX x, y;			/* Coordinate of the pixel to turn on */
  CrossPtr cross;		/* Line segment to splice pixel into */
  PathDir dir;			/* Rest of definition of line segment */
  {
  Fixed xmid;			/* The x value we put the splice around */
  Fixed ymid;			/* The mid-pixel Y value */
  CrossPtr c1, c2;		/* The line segment to splice into. c1->c2 is PathForw */
  Fixed xDelta, yDelta;		/* Delta from pixel midpoint to create splice points */
  boolean isWhite;		/* The new run pair is *white* */
  CrossPtr cl, cr;		/* Left and right Crosses in splice run pair */

  checkRuns = true;

  c2 = PathXtraCross(cross, dir);
  if (dir == PathForw)
    c1 = cross;
  else
    { c1 = c2;  c2 = cross; }
  DEBUGERROR(c1->c.x==c2->c.x,"SplicePixel: Splice into vertical line");

  /* See if the line segment is already connected to a run containing the pixel */
  cl = c1;
  if (cl->c.f.xtra)
    cl = PathCross(cl, PathBack);
  if (Pixel(cl->c.y) == y) {
    cl = RunPair(cl);
    if (cl->c.xRun <= x && cl->c.yNext->c.xRun > x) {
#if PRINTEDITS
      printf ("   No splice, pixel already black:  (%d.5) %d\n", y, x);
#endif
      return;
      }
    }
  cl = c2;
  if (cl->c.f.xtra)
    cl = PathCross(cl, PathForw);
  if (Pixel(cl->c.y) == y) {
    cl = RunPair(cl);
    if (cl->c.xRun <= x && cl->c.yNext->c.xRun > x) {
#if PRINTEDITS
      printf ("   No splice, pixel already black:  (%d.5) %d\n", y, x);
#endif
      return;
      }
    }

#if GRAPHCHAR
  printf("%d %d DropOutPixel\n", x, y);
#endif

  ExpandCharBBox(x, y);

  StartSplice(c1);

  xmid = MidPixel(x);
  ymid = MidPixel(y);

  /* Find a spot to put the splice.  Either right on pixel or to right. */
  isWhite = false;				/* Assume no overlapping run */
  cl = YCROSS(y);
  while (true) {
    if (cl == NULL || cl->c.x > xmid+SPLICEDELTA)	/* No overlapping run */
      break;
    cr = cl->c.yNext;
    DEBUGERROR(cr==NULL, "SplicePixel: Odd number of Crosses on scanline");
    if (cr->c.x >= xmid-SPLICEDELTA) {		/* Overlapping run */
      isWhite = true;
      if (cr->c.x <= xmid+SPLICEDELTA || cl->c.x >= xmid-SPLICEDELTA) {
	while (true) {			/* Move splice to right */
	  cl = cr->c.yNext;
	  if (cl == NULL || cl->c.x > cr->c.x+4*SPLICEDELTA) {
	    xmid = cr->c.x + 2*SPLICEDELTA;
	    isWhite = false;
	    break;
	    }
	  cr = cl->c.yNext;
	  if (cr->c.x > cl->c.x+4*SPLICEDELTA) {
	    xmid = cl->c.x + 2*SPLICEDELTA;
	    break;
	    }
	  }
	}
      break;
      }
    cl = cr->c.yNext;
    }
#if PRINTEDITS
  printf("  Pixel splice black (%d.5) %d%s\n", y, x, isWhite?"  (white)":"");
#endif

  xDelta = (c1->c.x > xmid? SPLICEDELTA : -SPLICEDELTA);
  yDelta = ((c1->c.y>ymid || c2->c.y>ymid)? -SPLICEDELTA : SPLICEDELTA);

  if (!c1->c.f.xtra)
    NewXtraCross(xmid+xDelta, ymid-yDelta);
  cl = NewCross(xmid+xDelta, ymid);
  AddYCross(cl);
  NewXtraCross(xmid, ymid+yDelta);
  cr = NewCross(xmid-xDelta, ymid);
  AddYCross(cr);
  if (!c2->c.f.xtra)
    NewXtraCross(xmid-xDelta, ymid-yDelta);

  if (cl->c.x > cr->c.x)
    ExchangeCrossPtr(cl, cr);
  cl->c.xRun = x;
  SetLeftFlag(cl);
  cr->c.xRun = (isWhite? x : x+1);
  SetLeftFlag(cr);

  FinishSplice();
  ValidateCrossPtr(c1);
  ValidateCrossPtr(c2);

  FixCrossFlags( !c1->c.f.xtra? c1 : PathCross(c1, PathBack),
                 !c2->c.f.xtra? c2 : PathCross(c2, PathForw) );
  }


/* Check connected run pairs, and extend run pair if specified pixel is
   adjacent to the pair.
   Returns true if a run pair was extended.  */
private boolean ExtendRunDropOut(x, y, ip)
  IntX x, y;			/* Pixel to set */
  PIntersection ip;		/* The intersection defining runs to extend */
  {
  REG CrossPtr c;

  /* One direction */
  c = PathCross(ip->cross, ip->dir);
  if (Pixel(c->c.y) == y) {
    c = RunPair(c);
    if (x == c->c.xRun-1) {
      c->c.xRun = x;
#if PRINTEDITS
      printf("  Extend run left (%d.5) %d\n", y, x);
#endif
      goto SETPIXEL;
      }
    else if (x == c->c.yNext->c.xRun) {
      c->c.yNext->c.xRun = x + 1;
#if PRINTEDITS
      printf("  Extend run right (%d.5) %d\n", y, x);
#endif
      goto SETPIXEL;
      }
    }

  /* The other direction */
  c = ip->cross;
  if (c->c.f.xtra)
    c = PathCross(c, RevPathDir(ip->dir));
  if (Pixel(c->c.y) == y) {
    c = RunPair(c);
    if (x == c->c.xRun-1) {
      c->c.xRun = x;
#if PRINTEDITS
      printf("  Extend run left (%d.5) %d\n", y, x);
#endif
      goto SETPIXEL;
      }
    else if (x == c->c.yNext->c.xRun) {
      c->c.yNext->c.xRun = x + 1;
#if PRINTEDITS
      printf("  Extend run right (%d.5) %d\n", y, x);
#endif
      goto SETPIXEL;
      }
    }

  return false;

  /* Did extend a run */
  SETPIXEL:
#if GRAPHCHAR
  printf("%d %d DropOutPixel\n", x, y);
#endif
  ExpandCharBBox(x, y);
  return true;
  }


/* Set a pixel from a drop out */
private boolean SetPixelInDropOut(x, iknown, iother, splice)
  IntX x;			/* Column to fill in */
  PIntersection iknown;		/* Known intersection */
  PIntersection iother;		/* Other intersection to figure pixel from */
  boolean splice;		/* Splicing a pixel in is OK */
  {
  IntX y;
  PIntersection ip;

  y = Pixel(iknown->y+iother->y) >> 1;

  /* Try extending the known runs */
  if (ExtendRunDropOut(x, y, iknown))
    return true;

  if (!splice)			/* If no splicing, do not try anything else */
    return false;

  /* Try other methods */
  if (ExtendRunDropOut(x, y, iother))
    return true;

  /* Figure intersection closest to pixel */
  ip = iknown;
  if (MidPixel(y) > iknown->y) {
    if (iother->y > iknown->y) ip = iother;
    }
  else {
    if (iother->y < iknown->y) ip = iother;
    }

  SplicePixel(x, y, ip->cross, ip->dir);
  return true;
  }


/* Set a pixel in one column of an x-inflection dropout.
   WARNING: The user must VALIDATE CrossPtr's after calling this routine.
   WARNING: "known" must *not* be an xtra Cross.
   Returns false ONLY if a pixel should have been set on through a splice, but
   splicing is not allowed because of the "splice" flag.  */
private procedure FillInInflection(known, knownDir, xCol)
  CrossPtr known;		/* Path seg that intersects xCol */
  PathDir knownDir;		/* Path direction of path segment for "known" */
  IntX xCol;			/* The column to fill in.  Pixel number */
  {
  Fixed mid;			/* The midline we are interested in */
  CrossPtr c1, c2;
  PIntersection ip;
  Intersection ifirst, isecond;

  mid = MidPixel(xCol);
  ip = &ifirst;

  /* Search the "known" segment */
  c1 = known;
  while (true) {
    c2 = PathXtraOrig(c1, knownDir);
    if (((c1->c.x >= mid && c2->c.x <= mid) || (c1->c.x <= mid && c2->c.x >= mid)) &&
	(c1->c.x != c2->c.x)) {
      SetIntersection(*ip, YCrossing(c1,c2,mid), c1, knownDir);
      if (ip == &isecond)
	break;
      ip = &isecond;
      }
    c1 = c2;
    DEBUGERROR(!c2->c.f.xtra, "FillInInflection: Not two intersections");
    }

  SetPixelInDropOut(xCol, &ifirst, &isecond, true);
  }


/* Set one pixel of a drop out.  The pixel is either set by extended an existing
   run, or if that fails, splicing some new Crosses (which define a new pixel)
   into the path.
   WARNING: The user must VALIDATE CrossPtr's after calling this routine.
   WARNING: "known" must *not* be an xtra Cross.
   Returns false ONLY if a pixel should have been set on through a splice, but
   splicing is not allowed because of the "splice" flag.  */
private boolean FillInColumn(known, knownDir, xCol, blackAbove, splice)
  CrossPtr known;		/* Path seg that intersects xCol */
  PathDir knownDir;		/* Path direction of path segment for "known" */
  IntX xCol;			/* The column to fill in.  Pixel number */
  boolean blackAbove;		/* The black area is *above* the "known" segment */
  boolean splice;		/* Splicing a pixel in is OK */
  {
  REG Fixed mid;		/* The midline we are interested in */
  REG CrossPtr c1, c2;
  REG CrossPtr scanc;		/* Current Cross in scan line */
  PathDir dir;
  REG Fixed y;
  Int16 topPix;			/* Top y value */
  PIntersection ip;
  Intersection ilow, ihigh, iknown;

  mid = MidPixel(xCol);
  ilow.y = MINFixed;
  ihigh.y = MAXFixed;
  iknown.y = blackAbove? MINFixed : MAXFixed;

  /* Search the "known" segment */
  c1 = known;
  while (true) {
    c2 = PathXtraOrig(c1, knownDir);
    if (((c1->c.x >= mid && c2->c.x <= mid) || (c1->c.x <= mid && c2->c.x >= mid)) &&
	(c1->c.x != c2->c.x)) {
      y = YCrossing(c1, c2, mid);
      if (blackAbove) {
	if (y > iknown.y)
	  { ilow = iknown;  SetIntersection(iknown, y, c1, knownDir); }
	else if (y > ilow.y)
	  SetIntersection(ilow, y, c1, knownDir);
	}
      else {
	if (y < iknown.y)
	  { ihigh = iknown;  SetIntersection(iknown, y, c1, knownDir); }
	else if (y < ihigh.y)
	  SetIntersection(ihigh, y, c1, knownDir);
	}
      }
    c1 = c2;
    if (!c2->c.f.xtra)
      break;				/* Done when we run into scanline */
    }

  if (iknown.y==MINFixed || iknown.y==MAXFixed) {
#if PRINTEDITS
    printf("  No intersection with 'known'.  Paths crossed?  (%d.5) %d\n",
      Pixel(known->c.y), xCol);
#endif
    return true;
    }

  topPix = Pixel(known->c.y);
  if (PathXtraCross(known,knownDir)->c.y > known->c.y)		/* known is bottom */
    topPix++;

  /* Search top half of scanline.  Looks down and right. */
  for (scanc=YCROSS(topPix); scanc!=NULL; scanc=scanc->c.yNext) {
    c1 = scanc;
    if (c1 == known)
      goto TOPNEXT;
    dir = c1->c.f.down;
    if (dir == PathNone) {
      dir = c1->c.f.hright;
      if (dir == PathNone)
	goto TOPNEXT;
      if (dir == PathBoth)
	dir = PathXtraOrig(c1,PathForw)->c.y < c1->c.y? PathForw : PathBack;
      else if (PathXtraOrig(c1,dir)->c.y > c1->c.y)
	goto TOPNEXT;
      }
    if (c1->c.f.splice) {		/* If splice, follow other cross */
      c1 = PathCross(c1, dir);
      if (c1->c.f.splice)		/* If other cross is splice, forget it */
	goto TOPNEXT;
      dir = RevPathDir(dir);
      }
    while (true) {
      c2 = PathXtraOrig(c1, dir);
      if (((c1->c.x >= mid && c2->c.x <= mid) || (c1->c.x <= mid && c2->c.x >= mid)) &&
	  (c1->c.x != c2->c.x)) {
	y = YCrossing(c1, c2, mid);
	if (blackAbove) {
	  if (y < ihigh.y)
	    SetIntersection(ihigh, y, c1, dir);
	  }
	else {
	  if (y > ilow.y)
	    SetIntersection(ilow, y, c1, dir);
	  }
	}
      c1 = c2;
      if (!c2->c.f.xtra)
	break;				/* Done when we run into scanline */
      }
    TOPNEXT:;
    }

  /* Search bottom half of scanline.  Looks right only (not down). */
  for (scanc=YCROSS(topPix-1); scanc!=NULL; scanc=scanc->c.yNext) {
    c1 = scanc;
    if (c1 == known)
      goto BOTTOMNEXT;
    dir = c1->c.f.hright;
    if (dir == PathNone)
      goto BOTTOMNEXT;
    if (dir == PathBoth)
      dir = PathXtraOrig(c1,PathForw)->c.y > c1->c.y? PathForw : PathBack;
    else if (PathXtraOrig(c1,dir)->c.y < c1->c.y)
      goto BOTTOMNEXT;
    if (c1->c.f.splice) {		/* If splice, follow other cross */
      c1 = PathCross(c1, dir);
      if (c1->c.f.splice)		/* If other cross is splice, forget it */
	goto BOTTOMNEXT;
      dir = RevPathDir(dir);
      }
    while (true) {
      c2 = PathXtraOrig(c1, dir);
      if (((c1->c.x >= mid && c2->c.x <= mid) || (c1->c.x <= mid && c2->c.x >= mid)) &&
	  (c1->c.x != c2->c.x)) {
	y = YCrossing(c1, c2, mid);
	if (blackAbove) {
	  if (y < ihigh.y)
	    SetIntersection(ihigh, y, c1, dir);
	  }
	else {
	  if (y > ilow.y)
	    SetIntersection(ilow, y, c1, dir);
	  }
	}
      c1 = c2;
      if (!c2->c.f.xtra)
	break;				/* Done when we run into scanline */
      }
    BOTTOMNEXT:;
    }

  if (blackAbove) {
    if (ihigh.y != MAXFixed)
      ip = &ihigh;
    else if (ilow.y != MINFixed)	/* Have two intersections of known */
      ip = &ilow;
    else {
#if PRINTEDITS
      printf("  Ignored dropout (%d.5) %d\n", Pixel(known->c.y), xCol);
#endif
      DEBUGERROR(!PixelIsBlack(xCol,topPix) && !PixelIsBlack(xCol,topPix-1),
	"FillInColumn: Dropout contains one intersection (black above)");
      return true;			/* Only one intersection */
      }
    }
  else {
    if (ilow.y != MINFixed)
      ip = &ilow;
    else if (ihigh.y != MAXFixed)	/* Have two intersections of known */
      ip = &ihigh;
    else {
#if PRINTEDITS
      printf("  Ignored dropout (%d.5) %d\n", Pixel(known->c.y), xCol);
#endif
      DEBUGERROR(!PixelIsBlack(xCol,topPix) && !PixelIsBlack(xCol,topPix-1),
	"FillInColumn: Dropout contains one intersection (black below)");
      return true;			/* Only one intersection */
      }
    }

  return SetPixelInDropOut(xCol, &iknown, ip, splice);
  }


/* Fill in a dropout between two runs.
   WARNING: The user must VALIDATE CrossPtr's after calling this routine.  */
private procedure FillInRange(known, knownDir, start, end, blackAbove)
  CrossPtr known;		/* Path seg that intersects all columns */
  PathDir knownDir;		/* Path direction of path segment for "known" */
  IntX start, end;		/* Range of pixel columns (inclusive) */
  boolean blackAbove;		/* The black area is *above* the "known" segment */
  {

  DEBUGERROR(knownDir==PathNone||knownDir==PathBoth,"FillInRange: invalid path direction");
  DEBUGERROR(start>end, "FillInRange: Pixel range illegal");

  while (true) {
    if (!FillInColumn(known, knownDir, start, blackAbove, false))
      break;
    ValidateCrossPtr(known);
    start++;
    if (start > end)
      return;
    }

  while (start <= end) {
    ValidateCrossPtr(known);
    FillInColumn(known, knownDir, end, blackAbove, true);
    end--;
    }
  }


/* Fill in a dropout between two runs.
   WARNING: The user must VALIDATE CrossPtr's after calling this routine.  */
private procedure FillInPairs(top, bottom, known, knownDir, connectLeft)
  CrossPtr top;			/* Pair on upper scanline */
  CrossPtr bottom;		/* Pair on bottom scanline */
  CrossPtr known;		/* Path seg that intersects all columns */
  PathDir knownDir;		/* Path direction of path segment for "known" */
  boolean connectLeft;		/* Left edge of pairs is connected */
  {

#if PRINTEDITS
  printf("Connect %s: (%d.5) %d %d  to  (%d.5) %d %d\n",
    connectLeft? "left":"right",
    Pixel(top->c.y), top->c.xRun, top->c.yNext->c.xRun,
    Pixel(bottom->c.y), bottom->c.xRun, bottom->c.yNext->c.xRun);
#endif

  if (top->c.xRun < bottom->c.xRun)		/* Top is on left */
    FillInRange(known, knownDir, top->c.yNext->c.xRun, bottom->c.xRun-1, connectLeft);
  else
    FillInRange(known, knownDir, bottom->c.yNext->c.xRun, top->c.xRun-1, !connectLeft);
  }


/* Return the left-hand Cross of a run connected to a specified Cross on the
   opposite scanline.  "Opposite" is defined by the "below" flag.  Returns NULL
   if there is no run, or if the run is empty.  */
private CrossPtr ConnectedRunOpposite(cross, below)
  CrossPtr cross;		/* The Cross to look on other scanline */
  boolean below;		/* True: scanline is below "cross" */
  {
  CrossPtr result;

  if (cross == NULL)
    return NULL;

  result = NULL;
  if (below) {
    if (cross->c.f.down != PathNone)
      result = PathCross(cross, cross->c.f.down);
    }
  else {
    if (cross->c.f.up)
      result = PathUpCross(cross);
    }
  if (result != NULL) {
    result = RunPair(result);
    if (result->c.xRun >= result->c.yNext->c.xRun)
      result = NULL;
    }

  return result;
  }


/* Two run pairs are connected via a horizontal line.  This routine checks to
   see whether the pixels are connected.
   WARNING: The user must VALIDATE CrossPtr's after calling this routine.
   Note that this horizontal connection may have intervening (black) pairs on
   the same scanline...
   The path direction is specified to resolve the ambiguity of checking two
   white horizontal connections that both start with the same Cross.  */
private procedure CheckHorizWhite(ll, rl, known, knownDir)
  CrossPtr ll;			/* Left-hand half of the left pair */
  CrossPtr rl;			/* Left-hand half of right pair */
  CrossPtr known;		/* Cross that starts connection between ll and rl */
  PathDir knownDir;		/* Path direction for connection */
  {
  CrossPtr lr;			/* Right half of left pair */
  REG CrossPtr oleft, oright;	/* The runs on the opposite scan line */
  boolean below;		/* True: The white connection is below the scanline */
  REG IntX start, end;		/* Columns of dropout (inclusive) */
  IntX itmp;

  lr = ll->c.yNext;
  below = (PathXtraCross(known,knownDir)->c.y < known->c.y);
  start = lr->c.xRun;
  end = rl->c.xRun - 1;

  /* Get the two runs on the opposite scanline that are connected to left and right of lr */
  oleft = ConnectedRunOpposite(ll, below);
  oright = ConnectedRunOpposite(rl->c.yNext, below);
  /* Make oleft left of oright, and if one is NULL, make oright NULL. */
  if (oright==NULL || (oleft!=NULL && oright->c.xRun<oleft->c.xRun))
    ExchangeCrossPtr(oright, oleft);

  /* Skip over any pixels that are in oleft or oright */
  if (oleft != NULL) {
    if (oleft->c.xRun > start) {	/* Part of dropout is left of oleft */
      itmp = MIN(oleft->c.xRun-1, end);
#if PRINTEDITS
      printf("Horiz white: (%d.5) %d %d\n", Pixel(ll->c.y), start, itmp);
#endif
      FillInRange(known, knownDir, start, itmp, !below);
      ValidateCrossPtr(known);
      ValidateCrossPtr(oleft);
      }
    if (oleft->c.yNext->c.xRun > start)	/* Move dropout past oleft */
      start = oleft->c.yNext->c.xRun;
    }
  if (oright != NULL && start <= end) {
    ValidateCrossPtr(oright);
    if (oright->c.xRun > start) {	/* Part of dropout is left of oright */
      itmp = MIN(oright->c.xRun-1, end);
#if PRINTEDITS
      printf("Horiz white: (%d.5) %d %d\n", Pixel(ll->c.y), start, itmp);
#endif
      FillInRange(known, knownDir, start, itmp, !below);
      ValidateCrossPtr(known);
      ValidateCrossPtr(oright);
      }
    if (oright->c.yNext->c.xRun > start)	/* Move dropout past oright */
      start = oright->c.yNext->c.xRun;
    }
  if (start <= end) {
#if PRINTEDITS
    printf("Horiz white: (%d.5) %d %d\n", Pixel(ll->c.y), start, end);
#endif
    FillInRange(known, knownDir, start, end, !below);
    }
  }


/* Go through the list of x inflection points, and see if there are any
   places where the path moves over an x midline and the pixel is not set.
   WARNING:  This routine often sees more than one inflection point in the
   same interval between two Crosses.  The code below does not do anything
   special for this case.  It assumes that the first inflection point will
   fix things, so others will not cause any more fill-in work.  */
private procedure DoXInflections()
  {
  REG CrossPtr c;		/* Current Cross */
  REG CrossPtr close;		/* The run closest to "c" */
  REG CrossPtr other;		/* Other run */
  REG CrossPtr back;		/* Cross PathBack from "c" */
  REG IntX pixel;

  /* Minimum inflections */
  for (c=minXInflections; c!=NULL; c=c->c.yNext) {
    ValidateCrossPtr(c);
    DEBUGERROR(!c->c.f.xtra, "DoXInflections: Non-xtra inflection point (min)");
    if ((c->c.x & 0xFFFFL) == 0x8000)
      c->c.x += 1;
    close = PathCross(c, PathForw);	/* Assume PathForw is closest */
    if (!close->c.f.isLeft)
      close = RunPair(close);
    back = other = PathCross(c, PathBack);
    if (!other->c.f.isLeft)
      other = RunPair(other);
    if (close->c.xRun > other->c.xRun)
      close = other;
#if PRINTEDITS
    if (close->c.xRun-1 >= FRound(c->c.x)) {
      float f1, f2;
      fixtopflt(c->c.x, &f1);  fixtopflt(c->c.y, &f2);
      printf("x inflection dropout (min):  %g %g\n", f1, f2);
      }
#endif
    for (pixel=close->c.xRun-1; pixel>=FRound(c->c.x); pixel--) {
      FillInInflection(back, PathForw, pixel);
      ValidateCrossPtr(back);
      ValidateCrossPtr(c);
      }
    }

  /* Maximum inflections */
  for (c=maxXInflections; c!=NULL; c=c->c.yNext) {
    ValidateCrossPtr(c);
    DEBUGERROR(!c->c.f.xtra, "DoXInflections: Non-xtra inflection point (max)");
    if ((c->c.x & 0xFFFFL) == 0x8000)
      c->c.x -= 1;
    close = PathCross(c, PathForw);	/* Assume PathForw is closest */
    if (!close->c.f.isLeft)
      close = RunPair(close);
    back = other = PathCross(c, PathBack);
    if (!other->c.f.isLeft)
      other = RunPair(other);
    if (close->c.yNext->c.xRun < other->c.yNext->c.xRun)
      close = other;
#if PRINTEDITS
    if (close->c.yNext->c.xRun <= FRound(c->c.x)-1) {
      float f1, f2;
      fixtopflt(c->c.x, &f1);  fixtopflt(c->c.y, &f2);
      printf("x inflection dropout (max):  %g %g\n", f1, f2);
      }
#endif
    for (pixel=close->c.yNext->c.xRun; pixel<=FRound(c->c.x)-1; pixel++) {
      FillInInflection(back, PathForw, pixel);
      ValidateCrossPtr(back);
      ValidateCrossPtr(c);
      }
    }
  }


/* Check a horizontal-right connection to see if we should call CheckHorizWhite.
   This checks to see whether the two runs look connected already...
   WARNING: The user must VALIDATE CrossPtr's after calling this routine.
   NOTE:  This routine is recursive.  */
private procedure EditBlackHoriz(cross, dir)
  REG CrossPtr cross;		/* The horizontally-connected Cross */
  REG PathDir dir;
  {
  REG CrossPtr p1, p2;		/* Left-hand Crosses in the two connected runs */

  if (dir == PathBoth) {
    EditBlackHoriz(cross, PathForw);
    ValidateCrossPtr(cross);
    EditBlackHoriz(cross, PathBack);
    return;
    }

  p2 = PathCross(cross, dir);
  if (cross->c.f.isLeft && !p2->c.f.isLeft)	/* Clearly black connection */
    return;

  p1 = cross;
  if (!p1->c.f.isLeft)
    p1 = RunPair(p1);
  if (!p2->c.f.isLeft)
    p2 = RunPair(p2);

  if (p1->c.yNext->c.xRun < p2->c.xRun)
    CheckHorizWhite(p1, p2, cross, dir);
  }


/* Check whether a run is connected to another run.
   WARNING: The user must VALIDATE CrossPtr's after calling this routine.  */
private procedure CheckBlackDown(topc, botc)
  CrossPtr topc;		/* Cross on top scanline.  Not necessarily left. */
  CrossPtr botc;		/* Cross on bottom scanline.  Not necessarily left. */
  {
  CrossPtr tl;			/* Top-left Cross */

  /* Guarantee our crosses are on the left */
  tl = topc;
  if (!topc->c.f.isLeft)
    tl = RunPair(topc);
  if (!botc->c.f.isLeft)
    botc = RunPair(botc);

  if (tl->c.xRun > botc->c.yNext->c.xRun || tl->c.yNext->c.xRun < botc->c.xRun)
    FillInPairs(tl, botc, topc, topc->c.f.down, topc==tl);
  }


/* Get rid of black dropouts.
   WARNING: The user must VALIDATE CrossPtr's after calling this routine.
   NOTE: This routine assumes there is a NULL scanline above yBoxMax and below yBoxMin.  */
private procedure EditBlackSpace()
  {
  REG CrossPtr pl, pr;		/* Primary (top) scanline, left and right Crosses */
  REG CrossPtr sl, sr;		/* Secondary (bottom) scanline, left and right Crosses */
  REG CrossPtr *yc;		/* Ptr to YCROSS entry for primary scanline */
  REG IntX yCount;

  yc = &YCROSS(yBoxMax);

  /* Loop through scanlines */
  for (yCount=yBoxMax-yBoxMin; yCount>=0; yCount--) {
    pl = *yc--;

    /* Loop through primary scanline */
    while (pl != NULL) {

      pr = pl->c.yNext;
      sr = BADCROSS;
      if (pr->c.f.down != PathNone)
	sr = PathCross(pr, pr->c.f.down);
      if (pl->c.f.down != PathNone) {	/* Got "down" connection */
	sl = PathCross(pl, pl->c.f.down);
	if (sl->c.f.isLeft && sl->c.yNext == sr) {	/* Fast case */
	  if (pl->c.xRun > sr->c.xRun || pr->c.xRun < sl->c.xRun) {
	    FillInPairs(pl, sl, pl, pl->c.f.down, true);
	    ValidateCrossPtr(pl);  ValidateCrossPtr(pr);
	    }
	  }
	else {
	  CheckBlackDown(pl, sl);
	  ValidateCrossPtr(pl);  ValidateCrossPtr(pr);
	  if (sr != BADCROSS && pr->c.f.down != PathNone) {
	    ValidateCrossPtr(sr);
	    CheckBlackDown(pr, sr);
	    ValidateCrossPtr(pl);  ValidateCrossPtr(pr);
	    }
	  }
	}
      else {
	if (sr != BADCROSS) {
	  CheckBlackDown(pr, sr);
	  ValidateCrossPtr(pl);  ValidateCrossPtr(pr);
	  }
	}

      if (pl->c.f.hright != PathNone) {
	EditBlackHoriz(pl, pl->c.f.hright);
	ValidateCrossPtr(pl);  ValidateCrossPtr(pr);
	}
      if (pr->c.f.hright != PathNone) {
	EditBlackHoriz(pr, pr->c.f.hright);
	ValidateCrossPtr(pl);  ValidateCrossPtr(pr);
	}

      pl = pr->c.yNext;
      }
    }

  DoXInflections();		/* Also look for horizontal "bumps" */
  }


/****************   START OF WHITE SPACE PROCESSING  **************/
#if WHITE


/****************   START OF CONNECTION ANALYSIS  **************/

/* Pixel-to-Cxtn mapping looks like:  (center coord = x, y)
   0  1  2			y + 1
   7  .  3			y
   6  5  4			y - 1
 */

/* This structure describes the pixels that are "connected" to the middle pixel
   in a 3x3 grid around that pixel.  Every pixel that is considered connected is
   on (set).  The middle pixel is always set.  */
typedef Card8 Cxtn;		/* WARNING: Can't use type as param (unless ANSI) */
#define BADCXTN (0xFF)		/* Illegal Cxtn: Can't figure it out.  Must be (-1). */
#define NOCXTN 0		/* There are no connected pixels */

/* Map relative pixel position to a Cxtn bit (or pixel number) */
#define RelPixelToCxtn(dx, dy) PixelToCxtnArray[(((dy)+1)*3) + ((dx)+1)]
#define RelPixelToPixNum(dx, dy) PixelToPixNumAry[(((dy)+1)*3) + ((dx)+1)]

/* Return whether a pixel is part of a Cxtn */
#define PixelInCxtn(cxtn, pixNum) ( (cxtn) & (1<<(pixNum)) )

/*                                      6     5     4     7       3    0    1    2 */
private IntX  PixelToPixNumAry[9] = {   6,    5,    4,    7,  8,  3,   0,   1,   2 };
private Card8 PixelToCxtnArray[9] = { 0x40, 0x20, 0x10, 0x80, 0, 0x8, 0x1, 0x2, 0x4 };

#define ROTATECXTN(cxtn, bits) (ROTATELEFT8(cxtn, bits))
#define ROTATEPIXNUM(num, bits) ((num + bits) & 0x7)


/* A FixupPattern is derived from a Cxtn and the location of an "intruder pixel"
   (which has caused the connection problem).  The Cxtn is a set of bits or'ed
   together, where each bit represents a pixel.  The pixels are represented
   relative to the middle pixel in a 3x3 "cell", using the numbering given above.
   To look up a FixupPattern, the Cxtn must be normalized by choosing the lowest
   value among four even rotations (0, 2, 4, and 6).  This means there will always
   be a pixel in the upper left or middle location.  The intruder pixel is
   rotated as well.  The normalized Cxtn+intruder (FixupPattern) is looked up
   in the PatternArray.  If the Cxtn is zero (no connections), everything is
   rotated to place the intruder in the bottom left or middle (position 5 or 6).

   Fixup Patterns are used to get a list of
   FixupAction(s), which are possible operations to perform to fix the problem
   along with the "merit" of the action -- how appealing the action is compared to
   other actions that might be tried in other patterns (or with other actions in
   the same pattern).

   A fixup action is an operation that will fix the "problem" between two pixels.
   These are moves or deletes.  Actions are *possible* solutions to the problem.
   If one action cannot be applied (usually because it creates a pixel problem
   itself), the next is tried in "merit" order.  There are tie-breaking rules for
   actions of equal merit.

   For a given pattern, all actions should be stored in order of "merit".
   For equal merit, they should be stored in the order in which they will be tried.

   Merit numbers are totally arbitrary.  They are just used to compare against
   each other.  Right now, they must fit in a Card8, so they must be 0-255.
   Greater numerical value indicates higher merit -- these actions are done first.
   The NULLMERIT value is zero, and should not be used.
   WARNING: The merit of an action may be adjusted slightly in the code to
   break ties.  MERITQUANTUM is the minimum amount of space necessary between
   merit values.

   The structures that map between a FixupPattern and a set of FixupActions have
   been tailored for the current approach to simplifying this mapping.  There
   is one routine that takes a FixupPattern and returns a FixupAction:
   PatternToActions.  It is assumed that a different approach will require
   different structures.  Don't hesitate to rewrite PatternToActions to implement
   a new mapping.
 */

typedef Card16 FixupPatternKey;
#define PATTERNKEY(cxtn, intruder)  ( (((Card8)(cxtn))<<8) | (Card8)(intruder) )

#define MERITQUANTUM 2		/* Minimum space between merit values */
#define NULLMERIT 0		/* Null action */
#define MAXPATTERNACTIONS 3	/* Maximum number of actions stored in a pattern */

typedef enum {			/* This goes in "operation" field */
  NoAct,			/* No action */
  DeleteAct,			/* Delete the pixel */
  MoveAct			/* Move the pixel to "argument" (see FixupAction) */
  } ActionOp;

typedef struct {
  Card8 operation;		/* The operation to perform (delete, move) */
  Card8 merit;			/* How appealing compared to other actions */
  Card8 moveTarget;		/* The pixel to move middle pixel to */
  } FixupAction, *FixupActionPtr;

/* A type for clients that want to build FixupAction interpreters.
   The list of FixupActions always ends with a NULLMERIT action.  */
typedef struct {
  Cxtn cxtn;			/* The Cxtn that indicated the actions */
  FixupAction actions[MAXPATTERNACTIONS+1];
  } RuleDesc, *RuleDescPtr;


/* The PatternArray maps a PatternKey onto an action class.  Normally there
   is a single action associated with the pattern.  If so, the class is found
   in the ActionClassArray.  The "argument" field contains the move target for
   a MoveAct.  If the class is NOCLASS, the argument field contains the index
   of the beginning of a list of class entries that ends with a NOCLASS entry.
 */


/* The following is a system for defining merit "classes".  Each class is
   composed of a number of letters.  The letters are defined below.  The
   capital letter is the one used for the class, as in "M" for Move.
   ("Strong" connections are horizontal or vertical, "weak" are diagonal.)

   1.  Move  /  Delete		Operation

   2.  (Depends on the operation)
       Move  (result of move):
       Nothing		Nothing special  (pixel is at end of "feature")
       Straight line	Forms a straight line
       Diagonal line	Forms a diagonal line

       Delete  (connection(s) before operation):
       Strong		Horizontal/vertical connection to one pixel
       Weak		Diagonal connection to one pixel
       Corner		End of a three-pixel corner
       Outside corner	Middle of a three-pixel corner
       Bump		More complicated structure

   3.  _			Nothing special
       eXtra connection		There is connection *after* operation

   Some assumptions used for this set of rules:
   - Operations that break strong connections and leave around weak connections
     are OK.  These always have less merit than operations that have no
     connections at the end.
   - Moves have more merit than deletes, in general.
   - A pixel is never moved to only a weak connection.  This was observed to hurt
     in more cases than it helped.
   - There are 4 disjoint collections of merit classes:
     Moves,  Moves with extra connections, Deletes, Deletes with extra.
 */


typedef struct {
  Card8 action;				/* The action to take */
  Card8 merit;				/* The merit for the action */
  } ActionClass, *ActionClassPtr;

/* WARNING: These must correspond EXACTLY to the entries in ActionClassArray */
enum {
	MN,		/* Move */
	MS,		/* Move into Straight line */
	MD,		/* Move into Diagonal line */

	MDX,		/* Move Diagonal, leftover cXtn */

	DS,		/* Delete from Strong */
	DW,		/* Delete from Weak */
	DC,		/* Delete from Corner */
	DO,		/* Delete from Outside corner */
	DB,		/* Delete from Bump */

	DSX,		/* Delete from Strong, leftover cXtn */
	DCX,		/* Delete from Corner, leftover cXtn */
	DOX,		/* Delete from Outside corner, leftover cXtn */
	DBX,		/* Delete from Bump, leftover cXtn */
	MAXACTIONCLASS
	};

/* Description of each action class.
   WARNING: Each class is obtained using the class index.  Don't move entries around! */
#define ACTCLASS(op, merit)  { op, merit * MERITQUANTUM }
private ActionClass ActionClassArray[] = {
  ACTCLASS(MoveAct, 20),		/* MN: Move */
  ACTCLASS(MoveAct, 16),		/* MS: Move into Straight line */
  ACTCLASS(MoveAct, 18),		/* MD: Move into Diagonal line */

  ACTCLASS(MoveAct,  7),		/* MDX: Move Diagonal, leftover cXtn */

  ACTCLASS(DeleteAct, 12),		/* DS: Delete from Strong */
  ACTCLASS(DeleteAct, 12),		/* DW: Delete from Weak */
  ACTCLASS(DeleteAct, 10),		/* DC: Delete from Corner */
  ACTCLASS(DeleteAct, 11),		/* DO: Delete from Outside corner */
  ACTCLASS(DeleteAct,  9),		/* DB: Delete from Bump */

  ACTCLASS(DeleteAct,  5),		/* DSX: Delete from Strong, leftover cXtn */
  ACTCLASS(DeleteAct,  3),		/* DCX: Delete from Corner, leftover cXtn */
  ACTCLASS(DeleteAct,  4),		/* DOX: Delete from Outside corner, leftover cXtn */
  ACTCLASS(DeleteAct,  2),		/* DBX: Delete from Bump, leftover cXtn */
  };


#define MOVEUP    1
#define MOVELEFT  7
#define MOVERIGHT 3


/* For lists of multiple actions */
#define ACTLIST MAXACTIONCLASS		/* List of multiple actions */
#define NULLACTCLASS MAXACTIONCLASS	/* No class */

typedef struct {
  Card8 index;				/* Look up in ActionClassArray */
  Card8 argument;			/* Move target, or ACTLIST index */
  } ActionIndex, *ActionIndexPtr;

/* The names below correspond to pattern/intruder numbers.  For instance,
   LIST13 means cxtn = 1, intruder pixel = 3.
   WARNING: The values must reflect the entries in the ActionListArray.  */
enum { LIST12, LIST13, LIST14, LIST15, LIST16 };

typedef struct {
  ActionIndex actions[MAXPATTERNACTIONS+1];
  } ActionList;

/* WARNING: These entries must match EXACTLY with the LIST* enum values above */
private ActionList ActionListArray[] = {
  {{ {MN, MOVELEFT}, {DW, 0},      {NULLACTCLASS} }},		/* LIST12 */
  {{ {MN, MOVELEFT}, {DW, 0},      {NULLACTCLASS} }},		/* LIST13 */
  {{ {MN, MOVELEFT}, {MN, MOVEUP}, {DW, 0}, {NULLACTCLASS} }},	/* LIST14 */
  {{ {MN, MOVEUP},   {DW, 0},      {NULLACTCLASS} }},		/* LIST15 */
  {{ {MN, MOVEUP},   {DW, 0},      {NULLACTCLASS} }}		/* LIST16 */
  };


typedef struct {
  FixupPatternKey key;			/* The pattern/intruder pair */
  ActionIndex act;			/* The action(s) to take */
  } FixupPattern, *FixupPatternPtr;

#define PATTERN(cxtn, intruder, rule, arg) \
  { PATTERNKEY(cxtn,intruder), {rule, arg} }
/* WARNING:  These entries must be in ascending order by FixupPatternKey value */
private FixupPattern PatternArray[] = {
  PATTERN( 0x1, 2, ACTLIST, LIST12),
  PATTERN( 0x1, 3, ACTLIST, LIST13),
  PATTERN( 0x1, 4, ACTLIST, LIST14),
  PATTERN( 0x1, 5, ACTLIST, LIST15),
  PATTERN( 0x1, 6, ACTLIST, LIST16),

  PATTERN( 0x2, 3, DSX, 0),
  PATTERN( 0x2, 4, DS,  0),
  PATTERN( 0x2, 5, DS,  0),
  PATTERN( 0x2, 6, DS,  0),
  PATTERN( 0x2, 7, DSX, 0),

  PATTERN( 0x3, 3, DCX, 0),
  PATTERN( 0x3, 4, DC,  0),
  PATTERN( 0x3, 5, DC,  0),
  PATTERN( 0x3, 6, DC,  0),

  PATTERN( 0x5, 4, MS,  MOVEUP),
  PATTERN( 0x5, 5, MS,  MOVEUP),
  PATTERN( 0x5, 6, MS,  MOVEUP),

  PATTERN( 0x6, 4, DC,  0),
  PATTERN( 0x6, 5, DC,  0),
  PATTERN( 0x6, 6, DC,  0),
  PATTERN( 0x6, 7, DCX, 0),

  PATTERN( 0x7, 4, DB,  0),
  PATTERN( 0x7, 5, DB,  0),
  PATTERN( 0x7, 6, DB,  0),

  PATTERN( 0x9, 5, MDX, MOVEUP),
  PATTERN( 0x9, 6, MD,  MOVEUP),

  PATTERN( 0xa, 5, DOX, 0),
  PATTERN( 0xa, 6, DO,  0),
  PATTERN( 0xa, 7, DOX, 0),

  PATTERN( 0xb, 5, DOX, 0),
  PATTERN( 0xb, 6, DO,  0),

  PATTERN( 0xe, 5, DBX, 0),
  PATTERN( 0xe, 6, DB,  0),
  PATTERN( 0xe, 7, DBX, 0),

  PATTERN( 0xf, 5, DBX, 0),
  PATTERN( 0xf, 6, DB,  0),

  PATTERN(0x12, 6, MD,  MOVERIGHT),
  PATTERN(0x12, 7, MDX, MOVERIGHT),

  PATTERN(0x13, 6, MD,  MOVERIGHT),

  PATTERN(0x17, 6, MD,  MOVERIGHT),

  PATTERN(0x19, 6, MD,  MOVEUP),

  PATTERN(0x1a, 6, DO,  0),
  PATTERN(0x1a, 7, DOX, 0),

  PATTERN(0x1b, 6, DB,  0),

  PATTERN(0x1e, 6, DB,  0),
  PATTERN(0x1e, 7, DBX, 0),

  PATTERN(0x1f, 6, DB,  0)
  };
#define PatternArrayLen (sizeof(PatternArray)/sizeof(FixupPattern))


#if PROTOTYPES
private Cxtn ScanLineCxtn(CrossPtr cstart, CrossPtr cend, IntX x, IntX dy, boolean deleted);
private Cxtn BuildCxtn (CrossPtr cleft, IntX x, boolean deleted);
private procedure PatternToActions(RuleDescPtr actDesc, FixupPatternPtr pattern, IntX rot);
private procedure GetRuleDesc(RuleDescPtr act, CrossPtr ll, IntX x, CrossPtr ci, IntX xi);
private boolean CheckDeletePixel(CrossPtr ll, IntX x);
private procedure DeletePixel(CrossPtr ll, IntX x);
private boolean FixupExtendRight(IntX x, IntX y);
private boolean FixupExtendLeft(IntX x, IntX y);
private boolean DoFixupAction(RuleDescPtr actDesc, FixupActionPtr act, CrossPtr ll, IntX x);
private boolean DetermineTieBreaker(CrossPtr p1, IntX x1, CrossPtr p2, IntX x2);
private procedure FixPixelProblem(CrossPtr ll, IntX lx, CrossPtr rl, IntX rx);
private procedure FixRuns(CrossPtr run1, CrossPtr run2);
#endif  /* PROTOTYPES */


/* Return the bits in a Cxtn for one scanline */
private Cxtn ScanLineCxtn(cstart, cend, x, dy, deleted)
  REG CrossPtr cstart;		/* Left half of first pair */
  REG CrossPtr cend;		/* Right half of last pair */
  REG IntX x;			/* X-value of middle pixel in cxtn */
  IntX dy;			/* Delta-y of this scanline from middle pixel */
  REG boolean deleted;		/* True: Return deleted pixels as part of result */
  {
  REG Cxtn result;		/* Final result */
  REG IntX dx;			/* Delta-x of current pixel from middle pixel */
  REG IntX endx;		/* Rightmost delta-x to use in loop */
  REG CrossPtr right;		/* Right half of current pair */
  boolean leftBreak;		/* There is a discontinuity between dx=(-1) & dx=0 */
  boolean lastPair;		/* True: Process this one last pair */

  if (cend->c.x < cstart->c.x)		/* Do we have something to look at? */
    return NOCXTN;

  result = NOCXTN;
  leftBreak = lastPair = false;
  while (true) {
    right = cstart->c.yNext;
    endx = right->c.xRun - x - 1;
    dx = cstart->c.xRun - x;
    if (deleted) {
      if (right->c.f.deleted)
	endx++;
      if (cstart->c.f.deleted)
	dx--;
      }
    if (endx > 1) endx = 1;
    if (dx < (-1)) dx = (-1);
    if (right->c.f.hright == PathNone) {	/* Pair not connected to the right */
      if (endx == (-1)) {		/* Left-middle break */
	leftBreak = true;		/* Flag it */
	if (dy == 0)
	  goto SKIPPAIR;		/* If middle line, skip this pair */
	}
      else if (endx == 0) {		/* Middle-right break */
	if (leftBreak || dy == 0)	/* If middle line or left break, last pair */
	  lastPair = true;
	}
      }
    while (dx <= endx) {
      result |= RelPixelToCxtn(dx, dy);
      dx++;
      }
    if (lastPair)
      break;
    SKIPPAIR:
    if (right == cend)
      break;
    cstart = right->c.yNext;
    }

  return result;
  }


/* Build a Cxtn from the Cross data structures */
private Cxtn BuildCxtn(cleft, x, deleted)
  CrossPtr cleft;		/* The left half of the pair containing the pixel */
  IntX x;			/* The x value of the pixel */
  boolean deleted;		/* True: Return deleted pixels as part of result */
  {
  REG CrossPtr c, c1;
  CrossPtr cright;		/* Right half of pair */
  CrossPtr ostart, oend;	/* Crosses on opposite scan line */
  IntX y;			/* The y value of the pixel */
  REG boolean upOK;		/* Maybe connected to scanline above */
  REG boolean downOK;		/* Maybe connected to scanline below */
  CrossPtr mstart=NULL, mend=NULL;	/* Start and end for the middle line */
  Fixed limitX;
  Cxtn result;

  result = NOCXTN;
  y = Pixel(cleft->c.y);
  cright = cleft->c.yNext;
  if (cleft->c.x == cright->c.x)
    goto NOCONNECTION;		/* Must be a crossed path here somewhere... */

  /* If there is a horizontal connection over this pair, no connected pixels
     on the same side as that connection.  */
  upOK = downOK = true;
  for (c=YCROSS(y); c!=cright; c=c->c.yNext) {
    DEBUGERROR(c==NULL,"BuildCxtn: No loop termination!");
    if (c->c.f.hright & PathForw) {
      c1 = PathCross(c, PathForw);
      if (c1->c.x > cleft->c.x) {
	if (c1->c.f.isLeft == c->c.f.isLeft)
	  goto NOCONNECTION;
	if (PathXtraCross(c,PathForw)->c.y > cleft->c.y)
	  upOK = false;
	else
	  downOK = false;
	}
      }
    if (c->c.f.hright & PathBack) {
      c1 = PathCross(c, PathBack);
      if (c1->c.x > cleft->c.x) {
	if (c1->c.f.isLeft == c->c.f.isLeft)
	  goto NOCONNECTION;
	if (PathXtraCross(c,PathBack)->c.y > cleft->c.y)
	  upOK = false;
	else
	  downOK = false;
	}
      }
    }

  /* Lower scanline */
  if (downOK) {
    ostart = NULL;  limitX = MINFixed;
    for(c=YCROSS(y); c!=cright; c=c->c.yNext) {
      if (c->c.f.down != PathNone) {
	c1 = PathCross(c,c->c.f.down);
	if (c1->c.x > limitX)
	  { ostart = c;  limitX = c1->c.x; }
	}
      }
    DEBUGERROR(ostart==NULL,"BuildCxtn: No down cxtn and no horiz cxtn (start)");
    oend = NULL;  limitX = MAXFixed;
    for(c=ostart->c.yNext; c!=NULL; c=c->c.yNext) {
      if (c->c.f.down != PathNone) {
	c1 = PathCross(c,c->c.f.down);
	if (c1->c.x < limitX)
	  { oend = c;  limitX = c1->c.x; }
	}
      }
    DEBUGERROR(oend==NULL,"BuildCxtn: No down cxtn and no horiz cxtn (end)");
    mstart = ostart;  mend = oend;
    ostart = PathCross(ostart, ostart->c.f.down);
    oend = PathCross(oend, oend->c.f.down);
    if (!ostart->c.f.isLeft || oend->c.f.isLeft)
      goto NOCONNECTION;
    if (ostart->c.x >= oend->c.x)
      goto NOCONNECTION;
    result |= ScanLineCxtn(ostart, oend, x, -1, deleted);
    if (result == BADCXTN)
      goto NOCONNECTION;
    }
  else {
    if (cleft->c.f.down != PathNone || cright->c.f.down != PathNone)
      goto NOCONNECTION;
    }

  /* Upper scanline */
  if (upOK) {
    ostart = NULL;  limitX = MINFixed;
    for(c=YCROSS(y); c!=cright; c=c->c.yNext) {
      if (c->c.f.up) {
	c1 = PathUpCross(c);
	if (c1->c.x > limitX)
	  { ostart = c;  limitX = c1->c.x; }
	}
      }
    DEBUGERROR(ostart==NULL,"BuildCxtn: No up cxtn and no horiz cxtn (start)");
    oend = NULL;  limitX = MAXFixed;
    for(c=ostart->c.yNext; c!=NULL; c=c->c.yNext) {
      if (c->c.f.up) {
	c1 = PathUpCross(c);
	if (c1->c.x < limitX)
	  { oend = c;  limitX = c1->c.x; }
	}
      }
    DEBUGERROR(oend==NULL,"BuildCxtn: No up cxtn and no horiz cxtn (end)");
    if (downOK) {			/* Use "outside" Crosses to limit connections */
      if (ostart->c.x < mstart->c.x)
	mstart = ostart;
      if (oend->c.x > mend->c.x)
	mend = oend;
      }
    else
      { mstart = ostart;  mend = oend; }
    ostart = PathUpCross(ostart);
    oend = PathUpCross(oend);
    if (!ostart->c.f.isLeft || oend->c.f.isLeft)
      goto NOCONNECTION;
    if (ostart->c.x >= oend->c.x)
      goto NOCONNECTION;
    result |= ScanLineCxtn(ostart, oend, x, 1, deleted);
    if (result == BADCXTN)
      goto NOCONNECTION;
    }
  else {
    if (cleft->c.f.up || cright->c.f.up)
      goto NOCONNECTION;
    if (!downOK) {
      mstart = mend = YCROSS(y);
      while (mend->c.yNext != NULL)
	mend = mend->c.yNext;
      }
    }

  /* OK, now do the middle line */
  if (!mstart->c.f.isLeft || mend->c.f.isLeft)
    goto NOCONNECTION;
  result |= ScanLineCxtn(mstart, mend, x, 0, deleted);
  if (result == BADCXTN)
    goto NOCONNECTION;

  return result;

  /* Cannot figure out the connection matrix */
  NOCONNECTION:
  return BADCXTN;
  }


/* Fill up an RuleDesc given an entry in the PatternArray */
private procedure PatternToActions(actDesc, pattern, rotation)
  RuleDescPtr actDesc;	/* Place to fill in the actions */
  FixupPatternPtr pattern;	/* The pattern we found */
  IntX rotation;		/* Rotation to get from real pixels to pattern */
  {
  FixupActionPtr dst;		/* The current action we are building */
  ActionClassPtr ac;		/* The ActionClass we are using */
  ActionIndexPtr ai;		/* The current ActionIndex in the list */

  dst = actDesc->actions;
  rotation = (8-rotation) & 0x7;	/* Invert rotation */
  if (pattern->act.index != ACTLIST) {
    ac = &ActionClassArray[pattern->act.index];
    dst->operation = ac->action;
    dst->merit = ac->merit;
    dst->moveTarget = ROTATEPIXNUM(pattern->act.argument, rotation);
    dst++;
    }
  else {			/* Have an action list */
    ai = ActionListArray[pattern->act.argument].actions;
    while (ai->index != NULLACTCLASS) {
      ac = &ActionClassArray[ai->index];
      dst->operation = ac->action;
      dst->merit = ac->merit;
      dst->moveTarget = ROTATEPIXNUM(ai->argument, rotation);
      dst++;
      ai++;
      }
    }

  dst->merit = NULLMERIT;
  }


/* This routine looks at the pixel around a particular pixel and returns a
   list of possible actions on the pixel in merit order.  */
private procedure GetRuleDesc(act, ll, x, ci, xi)
  RuleDescPtr act;		/* Place to fill in the actions */
  CrossPtr ll;			/* Left half of pair containing pixel */
  IntX x;			/* X-value of pixel */
  CrossPtr ci;			/* Left half of intruder pixel pair */
  IntX xi;			/* X-value of intruder */
  {
  Cxtn cxtn;
  Cxtn normCxtn;		/* Normalized cxtn for looking up pattern */
  IntX rotation;		/* Rotation to get normCxtn */
  REG IntX lowSpan, highSpan;	/* Current span we are searching in PatternArray */
  REG IntX pattern;		/* Current pattern */
  IntX r;
  IntX intruder;		/* Pixel number of intruder pixel */
  REG FixupPatternKey key;	/* The pattern key we are looking for */
  REG FixupPatternKey testKey;

  /* Get connecting pixels */
  cxtn = BuildCxtn(ll, x, true);	/* Include deleted pixels in Cxtn */
  act->cxtn = cxtn;
  if (cxtn == BADCXTN)	/* Problem getting connection */
    goto NOACTIONS;

  TRYAGAIN:

  normCxtn = cxtn;
  intruder = RelPixelToPixNum(xi-x, Pixel(ci->c.y)-Pixel(ll->c.y));
  if (cxtn == NOCXTN) {		/* Not connected to anything */
    /* Normalize.  We place the intruder in the bottom left or center (6 or 5) */
    rotation = (6-intruder) & 0x6;
    }
  else {
    /* Normalize.  We are looking for the minimum value cxtn that has a bit
       in the upper-left or upper-middle corner.  */
    rotation = 0;
    for (r=2; r<8; r+=2) {
      cxtn = ROTATECXTN(cxtn, 2);
      if (cxtn < normCxtn && (cxtn&0x3))
	{ normCxtn = cxtn;  rotation = r; }
      }
    /* NOTE: Special case.  Deleted pixels are normally in a Cxtn.  If the Cxtn
       shows the pixel is at the *end* of a stem, make sure it is not the only
       pixel left.  If it is, use NOCXTN rules instead. */
    if (normCxtn == 0x1 || normCxtn == 2) {
      if (BuildCxtn(ll, x, false) == NOCXTN) {
	cxtn = NOCXTN;
	goto TRYAGAIN;
	}
      }
    }
  intruder = ROTATEPIXNUM(intruder, rotation);

  /* Find actions -- Binary search the PatternArray for the key */
  lowSpan = 0;
  highSpan = PatternArrayLen - 1;
  key = PATTERNKEY(normCxtn, intruder);
  while (true) {
    pattern = (highSpan + lowSpan) >> 1;
    testKey = PatternArray[pattern].key;
    if (testKey >= key) {
      if (testKey == key)
	break;			/* Found it! */
      if (pattern == lowSpan)
	goto NOACTIONS;		/* Not in pattern array */
      highSpan = pattern - 1;
      }
    else {
      if (pattern == highSpan)
	goto NOACTIONS;
      lowSpan = pattern + 1;
      }
    }

#if PRINTEDITS
  printf("  Cxtn: (%d.5) %d  cxtn=0x%x intruder=%d rotation=%d\n",
    Pixel(ll->c.y), x, normCxtn, intruder, rotation);
#endif
  PatternToActions(act, &PatternArray[pattern], rotation);
  return;

  /* Come here if no actions could be found to fixup problem */
  NOACTIONS:
  act->actions[0].merit = NULLMERIT;
#if PRINTEDITS
  if (cxtn == BADCXTN)
    printf("  No fixup actions found  (%d.5) %d  cxtn=(bad)   Crossed paths?\n",
      Pixel(ll->c.y), x);
  else
    printf("  No fixup actions found  (%d.5) %d  cxtn=0x%x intruder=%d rotation=%d\n",
      Pixel(ll->c.y), x, normCxtn, intruder, rotation);
#endif
  }


/* Check to see if a DeletePixel call will succeed.  This is easier than
   undoing the delete later...  Returns true if it  would succeed. */
private boolean CheckDeletePixel(ll, x)
  REG CrossPtr ll;		/* Left half of pair containing pixel */
  IntX x;			/* X-value of pixel */
  {
  REG CrossPtr lr;		/* Right half */

  if (x == ll->c.xRun) {	/* Left pixel in a pair */
    if (ll->c.f.deleted)
      return false;		/* Cannot delete more than one pixel on side of pair */
    }
  else {			/* Right pixel in a pair */
    lr = ll->c.yNext;
    DEBUGERROR(x!=lr->c.xRun-1, "DeletePixel: Delete in middle of run");
    if (lr->c.f.deleted)
      return false;
    }

  return true;
  }


/* Delete a pixel.  This only works from the *end* of a run.
   Must call CheckDeletePixel first. */
private procedure DeletePixel(ll, x)
  REG CrossPtr ll;		/* Left half of pair containing pixel */
  IntX x;			/* X-value of pixel */
  {
  REG CrossPtr lr;		/* Right half */

  if (x == ll->c.xRun) {	/* Left pixel in a pair */
    DEBUGERROR(ll->c.f.deleted, "DeletePixel: Already deleted left side pixel");
    ll->c.xRun++;
    ll->c.f.deleted = true;
    }
  else {			/* Right pixel in a pair */
    lr = ll->c.yNext;
    DEBUGERROR(x!=lr->c.xRun-1, "DeletePixel: Delete in middle of run");
    DEBUGERROR(lr->c.f.deleted, "DeletePixel: Already deleted right side pixel");
    lr->c.xRun--;
    lr->c.f.deleted = true;
    }
  }


/* Extend a run right to set a pixel.  Returns true if successful. */
private boolean FixupExtendRight(x, y)
  REG IntX x, y;		/* Coordinate of pixel to set */
  {
  REG CrossPtr left, right;

  left = YCROSS(y);
  while (true) {
    if (left == NULL)
      return false;
    right = left->c.yNext;
    if (right->c.xRun == x)	/* Found normal pair to extend */
      break;
    if (right->c.f.deleted && right->c.xRun == x-1 && left->c.xRun >= right->c.xRun) {
      left->c.xRun = x;		/* Extend deleted pair */
      left->c.f.deleted = true;
      right->c.f.deleted = false;
      break;
      }
    left = right->c.yNext;
    }

  right->c.xRun = x + 1;
  return true;
  }


/* Extend a run left to set a pixel.  Returns true if successful. */
private boolean FixupExtendLeft(x, y)
  REG IntX x, y;		/* Coordinate of pixel to set */
  {
  REG CrossPtr left, right;

  left = YCROSS(y);
  while (true) {
    if (left == NULL)
      return false;
    if (left->c.xRun == x + 1)	/* Found normal pair to extend */
      break;
    right = left->c.yNext;
    if (left->c.f.deleted && left->c.xRun == x+2 && left->c.xRun >= right->c.xRun) {
      right->c.xRun = x + 1;	/* Extend deleted pair */
      left->c.f.deleted = false;
      right->c.f.deleted = true;
      break;
      }
    left = right->c.yNext;
    }

  left->c.xRun = x;
  return true;
  }


/* Return true if a particular pixel is painted black.  It's OK for the
   coordinates to be far outside the bitmap being painted. */
private boolean PixelIsBlack(x, y)
  IntX x,y;			/* Coordinates of pixel */
  {
  REG CrossPtr c;

  if (y >= yBoxMin && y <= yBoxMax) {
    c = YCROSS(y);
    while (c != NULL) {
      if (c->c.xRun > x)		/* Left side */
	break;
      c = c->c.yNext;		/* Right side */
      if (c->c.xRun > x)
	return true;		/* FOUND IT! */
      c = c->c.yNext;
      }
    }

  return false;
  }


/* Check a few pixels in a row to see if they are black */
#define CheckRow(x, y) ( PixelIsBlack(x-1,y) || PixelIsBlack(x,y) || PixelIsBlack(x+1,y) )


/* Check a few pixels in a column to see if they are black */
#define CheckCol(x, y) ( PixelIsBlack(x,y-1) || PixelIsBlack(x,y) || PixelIsBlack(x,y+1) )


/* This routine tries to execute a FixupAction on a pixel.
   It returns true if the action is successful.  */
private boolean DoFixupAction(actDesc, act, ll, x)
  REG RuleDescPtr actDesc;	/* "Actions" descriptor */
  REG FixupActionPtr act;	/* The action to try */
  REG CrossPtr ll;		/* Left half of pair containing pixel */
  REG IntX x;			/* X-value of pixel */
  {
  REG IntX y;
#if PRINTEDITS
  char moveString[10];		/* Kind of move operation.  Empty string == delete */
#endif

  y = Pixel(ll->c.y);

  /*** DELETE ***/
  if (act->operation == DeleteAct) {
#if PRINTEDITS
    moveString[0] = '\0';
#endif
    if (!CheckDeletePixel(ll, x))
      goto FAILURE;
    goto DELETE;
    }

  /*** MOVE ***/
  else {
    /* NOTE: A move operation cannot change the char bbox.  It cannot expand it
       because that is prevented explicitly by the code.  It cannot make it
       smaller because the code always moves a pixel *away* from an intruder.  */

    DEBUGERROR(act->operation!=MoveAct,"DoFixupAction: Illegal operation");

    switch (act->moveTarget) {

      case 7:				/* Move pixel left */
	DEBUGERROR(ll->c.xRun!=ll->c.yNext->c.xRun-1,"DoFixupAction: Move horiz, bad run");
#if PRINTEDITS
	strcpy (moveString, "left");
#endif
	if (x == xBoxMin || CheckCol(x-2,y) || PixelIsBlack(x-1,y))
	  goto FAILURE;
	ll->c.xRun--;  ll->c.yNext->c.xRun--;
	break;

      case 3:				/* Move pixel right */
	DEBUGERROR(ll->c.xRun!=ll->c.yNext->c.xRun-1,"DoFixupAction: Move horiz, bad run");
#if PRINTEDITS
	strcpy (moveString, "right");
#endif
	if (x == xBoxMax || CheckCol(x+2,y) || PixelIsBlack(x+1,y))
	  goto FAILURE;
	ll->c.xRun++;  ll->c.yNext->c.xRun++;
	break;

      case 1:				/* Move pixel up */
#if PRINTEDITS
	strcpy (moveString, "up");
#endif
	if (y == yBoxMax || CheckRow(x,y+2) || PixelIsBlack(x,y+1) ||
	  !CheckDeletePixel(ll,x))
	  goto FAILURE;
	if (PixelInCxtn(actDesc->cxtn, 0)) {
	  if (FixupExtendRight(x, y+1))
	    goto DELETE;
	  }
	if (PixelInCxtn(actDesc->cxtn, 2)) {
	  if (FixupExtendLeft(x, y+1))
	    goto DELETE;
	  }
	goto FAILURE;

      case 5:				/* Move pixel down */
#if PRINTEDITS
	strcpy (moveString, "down");
#endif
	if (y == yBoxMin || CheckRow(x,y-2) || PixelIsBlack(x,y-1) ||
	  !CheckDeletePixel(ll,x))
	  goto FAILURE;
	if (PixelInCxtn(actDesc->cxtn, 6)) {
	  if (FixupExtendRight(x, y-1))
	    goto DELETE;
	  }
	if (PixelInCxtn(actDesc->cxtn, 4)) {
	  if (FixupExtendLeft(x, y-1))
	    goto DELETE;
	  }
	goto FAILURE;

      default:
	DEBUGERROR(1, "DoFixupAction: Move to illegal pixel");
      }
    }

  /* We succeeded if we get here */
  SUCCESS:
#if PRINTEDITS
  if (moveString[0] == '\0')
    printf("   delete pixel: (%d.5) %d\n", y, x);
  else
    printf("   move pixel %s: (%d.5) %d\n", moveString, y, x);
#endif
  checkRuns = true;
  return true;

  /* Delete the pixel */
  DELETE:
  DeletePixel(ll, x);
  goto SUCCESS;


  /* Cannot execute this action */
  FAILURE:
#if PRINTEDITS
  if (moveString[0] == '\0')
    printf("   delete failed: (%d.5) %d\n", y, x);
  else
    printf("   move %s failed: (%d.5) %d\n", moveString, y, x);
#endif
  return false;
  }


/* This routine takes two pixels and decides which one should be operated on
   first.  It is used in the case "merit" ties between fixup rules.
   Returns true if "pair 1" should be operated on first.  */
private boolean DetermineTieBreaker(p1, x1, p2, x2)
  CrossPtr p1;			/* Left half of pair 1 */
  IntX x1;			/* X-value of pixel 1 */
  CrossPtr p2;			/* Left half of pair 2 */
  IntX x2;			/* X-value of pixel 2 */
  {
  FCd cd1, cd2;			/* For inverse-transforming p1 and p2 */

  /* Decide which pixel based on its position in character space.
     We operate first on lower pixels and rightmost pixels.  */

#if PRINTEDITS
  printf("  *** tie: used pixel orientation\n");
#endif

  cd1.x = MidPixel(x1);  cd1.y = p1->c.y;
  cd2.x = MidPixel(x2);  cd2.y = p2->c.y;
  FntITfmP(cd1, &cd1);
  FntITfmP(cd2, &cd2);

  if (cd1.y != cd2.y)
    return (cd1.y < cd2.y);
  else
    return (cd1.x > cd2.x);
  }


/* This routine tries to move or delete pixel(s) so that two runs that are
   touching no longer touch.  */
private procedure FixPixelProblem(ll, lx, rl, rx)
  CrossPtr ll;			/* Left half of leftmost pair */
  IntX lx;			/* X-value of pixel in left pair */
  CrossPtr rl;			/* Left half of rightmost pair */
  IntX rx;			/* X-value of pixel in right pair */
  {
  REG FixupActionPtr lap, rap;	/* Current actions for each pixel */
  REG FixupActionPtr fp;
  RuleDesc lactions, ractions;	/* Actions for left and right pixel */

#if PRINTEDITS
  printf("Black touching: (%d.5) %d  and  (%d.5) %d\n",
    Pixel(ll->c.y), lx, Pixel(rl->c.y), rx);
#endif

  GetRuleDesc(&lactions, ll, lx, rl, rx);
  lap = lactions.actions;
  GetRuleDesc(&ractions, rl, rx, ll, lx);
  rap = ractions.actions;

  while (true) {
    if (lap->merit == rap->merit) {
      if (lap->merit == NULLMERIT) {
#if PRINTEDITS
	printf("  No pixel fix that works.\n");
#endif
	break;
	}
      fp = DetermineTieBreaker(ll, lx, rl, rx)? lap : rap;
      while (fp->merit != NULLMERIT)	/* Fix up all merits -- nobody is equal */
	{ fp->merit++;  fp++; }
      }
    if (lap->merit > rap->merit) {
      if (DoFixupAction(&lactions, lap, ll, lx))
	break;
      lap++;
      }
    else {
      if (DoFixupAction(&ractions, rap, rl, rx))
	break;
      rap++;
      }
    }
  }


/* This routine tries to fix two runs that are overlapping.
   The interesting question is which set of two pixels to choose as the
   "problem" to fix.  Right now, this routine only tries to fix *one* "problem".  */
private procedure FixRuns(run1, run2)
  CrossPtr run1;		/* Left half of a run */
  CrossPtr run2;		/* Left half of other run */
  {
  CrossPtr run1r;		/* Right half of run1 */

  /* If one of the runs is "empty" (no black pixels), ignore problem */
  if (run1->c.xRun >= run1->c.yNext->c.xRun || run2->c.xRun >= run2->c.yNext->c.xRun)
    return;

  /* Make run1 the leftmost run */
  if (run1->c.xRun > run2->c.xRun)
    ExchangeCrossPtr(run1, run2);

  run1r = run1->c.yNext;

  if (run1r->c.xRun == run2->c.xRun)		/* Ends of runs are adjacent */
    FixPixelProblem(run1, run1r->c.xRun-1, run2, run2->c.xRun);

  else if (run1->c.y != run2->c.y) {		/* Not on same scanline? */
    if (run1r->c.xRun-1 == run2->c.xRun)	/* One pixel overlap */
      FixPixelProblem(run1, run2->c.xRun, run2, run2->c.xRun);
    else if (run1->c.xRun == run1r->c.xRun-1)	/* Left is single pixel */
      FixPixelProblem(run1, run1->c.xRun, run2, run1->c.xRun);
    else if (run2->c.xRun == run2->c.yNext->c.xRun-1)	/* Right is single pixel */
      FixPixelProblem(run1, run2->c.xRun, run2, run2->c.xRun);
    }
  }



/****************   END OF CONNECTION ANALYSIS  **************/


/* Two *white* pairs are not connected.  This routine tries to fix the problem.
   NOTE: This routine is only called for pairs on *different* scanlines.  */
private procedure ConnectWhitePairs(top, bottom)
  REG CrossPtr top, bottom;		/* The two pairs that seem to overlap */
  {
  CrossPtr left, right;

  /* Ignore empty pairs */
  if (top->c.xRun >= top->c.yNext->c.xRun || bottom->c.xRun >= bottom->c.yNext->c.xRun)
    return;

  if (top->c.xRun <= bottom->c.xRun)		/* Top is on left */
    { left = top;  right = bottom; }
  else
    { left = bottom;  right = top; }

  if (left->c.yNext->c.xRun == right->c.xRun) {
    FixRuns(RunPair(right), left->c.yNext);
    }
  }


/* This routine finds zero-width white pairs, and expands them to one pixel
   if appropriate.  We do this before doing other white fixup stuff so that
   we can ignore the zero-width features if they don't get fixed up here.
   The zero-width white pairs is considered a "problem" if the left half of
   the white pair has no horizontal-right connection.
 */
private procedure ExpandWhiteDropOuts()
  {
  REG CrossPtr wl, wr;		/* Left and right Crosses in *white* pairs */
  REG CrossPtr lastwr;		/* Previous "wr" value: Left half of left black pair */
  REG CrossPtr *yc;		/* Ptr to YCROSS entry for primary scanline */
  REG IntX yCount;

  yc = &YCROSS(yBoxMax);

  /* Loop through scanlines */
  for (yCount=yBoxMax-yBoxMin; yCount>=0; yCount--) {
    wr = *yc--;
    if (wr == NULL) continue;
    while (true) {
      wl = wr->c.yNext;
      lastwr = wr;
      wr = wl->c.yNext;
      if (wr == NULL) break;
      if (wl->c.xRun >= wr->c.xRun && wl->c.f.hright == PathNone) {
	FixRuns(lastwr, wr);
	}
      }
    }
  }


/* Check whether a black horizontal connection has white around it.
   NOTE: This routine is recursive.
   WARNING: This only checks the *other* scanline, not the current one.
     Any problems in the current one would be discovered by looking for
     white horizontal dropouts (white runs that are painted black).  */
private procedure EditWhiteHoriz(startLeft, dir)
  REG CrossPtr startLeft;	/* Left Cross of black pair */
  REG PathDir dir;		/* Path direction to right */
  {
  REG CrossPtr c1, c2;		/* Current black pair */
  IntX leftX, rightX;		/* Coordinates of pair we are checking */
  boolean up;

  if (dir == PathBoth) {
    EditWhiteHoriz(startLeft, PathForw);
    EditWhiteHoriz(startLeft, PathBack);
    return;
    }

  if (PathCross(startLeft,dir)->c.f.isLeft)	/* Must look like normal black */
    return;

  leftX = startLeft->c.xRun;
  rightX = startLeft->c.yNext->c.xRun;

  up = (PathXtraCross(startLeft,dir)->c.y > startLeft->c.y);

  c1 = YCROSS(Pixel(startLeft->c.y) + (up? 1 : -1));
  while (c1 != NULL) {
    if (c1->c.xRun > rightX)
      return;
    c2 = c1->c.yNext;
    if (c2->c.xRun >= leftX) {	/* Part of this run makes black connection w/ startLeft */
      FixRuns(startLeft, c1);
      leftX = startLeft->c.xRun;
      rightX = startLeft->c.yNext->c.xRun;
      }
    c1 = c2->c.yNext;
    }
  }


/* This routine tries to get rid of black connections that are inappropriate -- it
   edits white connections *in*.  White connections need to be "stronger" than black
   connections -- they are not diagonal.
   Two white runs are not considered to be connected unless the connection is "clean".
   That is, the connection between the runs needs to be on the same side of the run
   for both runs.  If it is not, paths must have crossed somewhere, so we ignore it.
   NOTE: This routine assumes there is a NULL scanline above yBoxMax and below yBoxMin.  */
private procedure EditWhiteSpace()
  {
  REG CrossPtr pl, pr;		/* Primary (top) scanline, left and right Crosses */
  REG CrossPtr sl, sr;		/* Secondary (bottom) scanline, left and right Crosses */
  REG CrossPtr *yc;		/* Ptr to YCROSS entry for primary scanline */
  REG IntX yCount;

  ExpandWhiteDropOuts();	/* Do this first */

  yc = &YCROSS(yBoxMax);

  /* Loop through scanlines */
  for (yCount=yBoxMax-yBoxMin; yCount>=0; yCount--) {
    pr = *yc--;
    if (pr == NULL) continue;

    /* Loop through primary scanline */
    while (true) {
      if (pr->c.f.hright != PathNone)
	EditWhiteHoriz(pr, pr->c.f.hright);

      pl = pr->c.yNext;
      pr = pl->c.yNext;
      if (pr == NULL)
	break;

      sr = BADCROSS;
      if (pr->c.f.down != PathNone)
	sr = PathCross(pr, pr->c.f.down);
      if (pl->c.f.down != PathNone) {	/* Got "down" connection */
	sl = PathCross(pl, pl->c.f.down);
	if (sl->c.f.isLeft && sl->c.yNext == sr) {	/* Fast case */
	  if (pl->c.xRun > sr->c.xRun || pr->c.xRun < sl->c.xRun)
	    ConnectWhitePairs(pl, sl);
	  }
	else {
	  if (!sl->c.f.isLeft && sl->c.yNext != NULL) {
	    if (pl->c.xRun >= sl->c.yNext->c.xRun || pr->c.xRun <= sl->c.xRun)
	      ConnectWhitePairs(pl, sl);
	    }
	  goto CHECKRIGHT;
	  }
	}
      else {
	CHECKRIGHT:
	if (sr != BADCROSS && sr->c.f.isLeft) {
	  sl = *yc;
	  if (sl != sr) {
	    while (sl->c.yNext != sr)
	      sl = sl->c.yNext;
	    if (pl->c.xRun >= sr->c.xRun || pr->c.xRun <= sl->c.xRun)
	      ConnectWhitePairs(pl, sl);
	    }
	  }
	}
      }
    }
  }

#endif  /* WHITE */
/****************   END OF WHITE SPACE PROCESSING  **************/


/* Check and possibly "grow" the buffer containing the run we are building */
#define EXTRA_RUNS 1
#define CHECKRUNBUFF() \
  { if (curRun >= endRun) { \
      newStart = startRun; newEnd = endRun; newCur = curRun; \
      GrowRunBuff(&newCur, &newStart, &newEnd);	\
      startRun = newStart; endRun = newEnd; curRun = newCur; \
      } \
  }

/* Make the run buffer larger */
private procedure GrowRunBuff(pCur, pStart, pEnd)
  Int16 **pCur, **pStart, **pEnd;
  {
  Int16 *oldBase, *newBase;

  oldBase = (Int16 *)memoryBuffer2->ptr;
#if ATM
  if (!(*bprocs->GrowBuff)(memoryBuffer2, MB2ALLOCINCREMENT, true))
    OutOfMemory();
  newBase = (Int16 *)memoryBuffer2->ptr;
#else
  if (!(newBase = (Int16 *)os_realloc(memoryBuffer2->ptr,
                             memoryBuffer2->len + MB2ALLOCINCREMENT)))
    OutOfMemory();
  memoryBuffer2->ptr = (char *)newBase;
  memoryBuffer2->len += MB2ALLOCINCREMENT;
#endif /* ATM */  
  *pCur = newBase + SubtractPtr(*pCur, oldBase);
  *pStart = newBase + SubtractPtr(*pStart, oldBase);
  /* Leave one extra Int16 to check only once for a pair */
  *pEnd = newBase + memoryBuffer2->len/sizeof(Int16) - EXTRA_RUNS;
  }

/* Convert the list of crosses to a run */
private procedure ReturnBits(callBackProc)
  procedure (*callBackProc)();
  {
  DevRun run;
  REG IntX i;
  REG CrossPtr c1;				/* The current Cross */
  REG Int16 *startRun, *curRun;
  REG Int16 *endRun;
  REG IntX pairs;
  REG IntX left, right;
  REG IntX lastLeft;
  Int16 *newStart, *newCur, *newEnd;

  run.bounds.y.l = yBoxMin;  run.bounds.y.g = yBoxMax + 1;
  run.bounds.x.l = xBoxMin;  run.bounds.x.g = xBoxMax + 1;

  startRun = curRun = (Int16 *)memoryBuffer2->ptr;
  endRun = (Int16 *)(memoryBuffer2->ptr+memoryBuffer2->len) - EXTRA_RUNS;
  if (memoryBuffer2->len <= EXTRA_RUNS*sizeof(Int16)) {
    endRun = curRun;		/* Force a GrowBuff */
    CHECKRUNBUFF();
    }

  /* Loop through scanlines */
  if (!checkRuns) {
    for (i=yBoxMin; i<=yBoxMax; i++) {
      CHECKRUNBUFF();
      if ((c1=YCROSS(i)) == NULL) {
	*curRun++ = 0;			/* Mark this as an empty scanline */
	}
      else {
	startRun = curRun++;
	pairs = 0;
	while (true) {
	  pairs++;
	  CHECKRUNBUFF();
	  *curRun++ = c1->c.xRun;
	  c1 = c1->c.yNext;
	  *curRun++ = c1->c.xRun;
	  c1 = c1->c.yNext;
	  if (c1 == NULL) break;
	  }
	*startRun = pairs;
	}
      }
    }
  else {
    for (i=yBoxMin; i<=yBoxMax; i++) {
      CHECKRUNBUFF();
      if ((c1=YCROSS(i)) == NULL) {
	*curRun++ = 0;			/* Empty */
	}
      else {
	startRun = curRun++;
	pairs = 0;
	lastLeft = MINInt16;
	while (true) {
	  left = c1->c.xRun;
	  c1 = c1->c.yNext;
	  right = c1->c.xRun;
	  c1 = c1->c.yNext;
	  if (left < right) {		/* Skip a deleted pixel */
	    if (left >= lastLeft) {
	      lastLeft = left;
	      pairs++;
	      CHECKRUNBUFF();
	      *curRun++ = left;
	      *curRun++ = right;
	      }
	    else {
#if PRINTEDITS
	      printf("Pair out of sequence: (%d.5)  %d %d  and  %d %d\n", i,
		(int)*(curRun-2), (int)*(curRun-1), (int)left, (int)right);
#endif
	      *(curRun-2) = left;	/* Assumes no white between unsorted pairs */
	      lastLeft = left;
	      if (*(curRun-1) < right)
		*(curRun-1) = right;
	      }
	    }
	  if (c1 == NULL) break;
	  }
	*startRun = pairs;
	}
      }
    }

  run.data = (Int16 *)memoryBuffer2->ptr;
  run.datalen = SubtractPtr(curRun, run.data);
  run.indx = 0;

  (*callBackProc)(&run);
  }




#if PPS

FCd cacheDelta;

public procedure CScan(callBack, delta)
  procedure (*callBack)();
  FCd delta;

#else /* PPS */

public procedure CScan(callBack)
  procedure (*callBack)();

#endif /* PPS */

{

#if ATM
  if (!(*bprocs->PathBBox)(xPathMin, yPathMin, xPathMax, yPathMax))
    return;
#else  /* not ATM */
   (*ms->procs->initMark)(ms, true);
#endif /* ATM */

#if GRAPHCHAR || GRAPHPOINTS
  cacheDelta = delta;
#endif /* PPS */

#if GRAPHCHAR
  {
  float xmin, xmax, ymin, ymax;
  fixtopflt(xPathMin, &xmin);
  fixtopflt(xPathMax, &xmax);
  fixtopflt(yPathMin, &ymin);
  fixtopflt(yPathMax, &ymax);
  printf("%g %g %g %g PathBBox\n", xmin, ymin, xmax, ymax);
  }
#endif

  /* Get rid of leftover stuff for "next" path */
  currentCross--;
  if (oldStartLink != NULL)
    oldStartLink->l.forw = NULL;

  if (!BuildYCross()) {		/* No bits.  Treat like a "space" char. */
#if ATM
    (*bprocs->CharBBox)((IntX)0, (IntX)0, (IntX)0, (IntX)0);
#endif /* ATM */ 
    return;
    }

  BuildInitialRuns(offsetCenterFlag);

  /* Make sure we have a reasonable CharBBox */
  DEBUGERROR(yBoxMax>8000||yBoxMax<-8000, "CScan: yBoxMax out of bounds");
  DEBUGERROR(yBoxMin>8000||yBoxMin<-8000, "CScan: yBoxMin out of bounds");
  DEBUGERROR(xBoxMax>8000||xBoxMax<-8000, "CScan: xBoxMax out of bounds");
  DEBUGERROR(xBoxMin>8000||xBoxMin<-8000, "CScan: xBoxMin out of bounds");

#if EDITDROPOUTS
  EditBlackSpace();
#if WHITE
  if (whiteFixupFlag)
    EditWhiteSpace();
#endif  /* WHITE */
#endif  /* EDITDROPOUTS */

#if ATM
  if (!(*bprocs->CharBBox)((IntX)xBoxMin, (IntX)yBoxMin, (IntX)xBoxMax+1, (IntX)yBoxMax+1))
    return;
#endif /* ATM */

#if GRAPHCHAR
  printf("%d %d %d %d CharBBox\n", xBoxMin, yBoxMin, xBoxMax+1, yBoxMax+1);
#endif /* GRAPHCHAR */

#if GRAPHCHAR			/* Output the crosses */
  {
  float f1, f2;
  CrossPtr first, p;

  first = firstCross + 1;		/* Skip initial xtra Cross */
  while (first != NULL) {		/* Loop through paths */
    while (first->c.f.link) first++;
    p = first;
    while (true) {
      fixtopflt(p->c.x, &f1);
      fixtopflt(p->c.y, &f2);
      if (!p->c.f.xtra) {
	printf("%g %g %d %d %d CR\n", f1, f2, p->c.f.down, p->c.f.hright, p->c.f.up);
	}
      else {
	printf("%g %g 0 0 0 XR\n", f1, f2);
	}
      p = PathXtraCross(p, PathForw);
      if (p == first) {
	printf("CloseCR\n");		/* CloseCR  == Close path on one set of crosses */
	break;
	}
      }
    p -= 1;				/* Get link Cross */
    first = p->l.forw;
    }
  printf("EndCrosses\n");
  }
#endif  /* GRAPHCHAR */

  ReturnBits(callBack);

#if GRAPHCHAR			/* Print out the bitmap */
  PrintBitmap();
#endif  /* GRAPHCHAR */

#if COUNTCROSSES
  {
  Int16 crossSize;
  Int32 total;

  crossSize = sizeof(Cross);
  total = crossCount + xtraCrossCount + linkCrossCount;

  printf("Total Crosses:  %-5ld(%d bytes)\n", total, crossSize);
  printf("Crosses:        %ld\n", crossCount);
  printf("Xtra:           %ld\n", xtraCrossCount);
  printf("Links:          %ld\n", linkCrossCount);
  printf("Approx Index:   %-5d(%d bytes)\n", yBoxMax-yBoxMin+1, sizeof(CrossPtr));
  printf("Mem Total.......%ld\n", (currentCross-firstCross)*sizeof(Cross));
  }
#endif  /* COUNTCROSSES */
  }
/* BC Version:  v007  Fri Jun 1 10:09:39 PDT 1990 */
