/*****************************************************************************

    bintree.h
    Interface to the bintree window system routines

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Leo 22Apr86

    Modified:

    26Sep86  Jack Copy Fixed from types.h, express Trapezoid in Fixeds
    02Oct86  Jack Added TrapezoidFull
    07Oct86  Jack Split off nextdevice.h to be shared by Postscript
    22Oct86  Jack Eliminate EPSILON
    19Dec86  Jack Compositing and alpha consts, GrayAlpha struct.
    09Jan87  Jack Add WdScreen.
    18Mar87  Jack Revised CopyBits modes, add DISSOLVE
    16Apr87  Leo  Added ABOVE/OUT/BELOW, and SubList structure
    20Apr87  Jack Eliminate VSourceColor
    22Sep87  Jack Eliminate GrayAlpha, WdScreenRec, frame* (see devsupport.h)
    16Dec87  Leo  Added Layer typedef, modified SubList
    23Dec87  Leo  Changed fields in Bounds to agree with DevBounds
    25Jan88  Jack Move "CopyBits" (now MoveRect) ops to composite.h
    03Feb88  Jack Subsumed stuff from bintreepriv.h (not so private)
    16Apr88  Leo  DeviceStatus record
    20Mar88  Jack Removed ColorState, ColorProcs.
    24Aug88  Jack Removed WdColor, WdMarkInfo, & (to bintreetypes.h) MarkRec.
    22Sep88  Jack Added CompositeInfo.
    11Jan89  Jack v006, add dependency on GRAPHICS ("extension" was "unused")
    13Jan89  Jack Invert alpha so 0 will be correct default, remove MTXSTATE
    08Feb89  Jack Add LOG2BD placeholder for multiple framebuffer devices
    21Mar90  Ted  Removed "screen" from Pattern to avoid including GRAPHICS
    29Mar90  Ted  Added PixelStat structure.
    03Apr90  Ted  Integrated int into work.
    12Apr90  Jack removed colortowhite
    04Jun90  Ted  Removed NEWBITS/FREEBITS macros
    24Jun90  Ted  New API implementation
    20Sep90  Ted  Published NXRenderInBounds.

******************************************************************************/

#ifndef BINTREE_H
#define BINTREE_H

#import DEVICE

typedef struct _Bitmap _Bitmap;
typedef struct _LocalBitmap _LocalBitmap;
typedef struct _BMCompOp _BMCompOp;

/*****************************************************************************
   Composite Object Types

   In composite calls, more than one type of object can be passed. To
   facilitate this, all objects have as their first byte a type field.
   Here we declare the external polymorphic structures.  The types for each
   object are declared here also.  There are other objects that are strictly
   internal to the NeXT WindowServer.
******************************************************************************/
#define BAG	 	((unsigned char) 'g')	/* 103 */
#define PATTERN  	((unsigned char) 'p')	/* 112 */

/*****************************************************************************
    Window Position Specifiers
******************************************************************************/
#define BELOW		-1	/* Place below another window */
#define OUT		 0	/* Remove from window list */
#define ABOVE		 1	/* Place above another window */

/*****************************************************************************
    External Bitmap Types
******************************************************************************/
#define NX_DEFAULTDEPTH			0
#define NX_TWOBITGRAY_DEPTH		258
#define NX_EIGHTBITGRAY_DEPTH		264			
#define NX_TWELVEBITRGB_DEPTH		516
#define NX_TWENTYFOURBITRGB_DEPTH	520

/*****************************************************************************
    Internal Bitmap Types
******************************************************************************/
#define NX_OTHERBMTYPE		0
#define NX_TWOBITGRAY		1
#define NX_EIGHTBITGRAY		2
#define NX_TWELVEBITRGB		3
#define NX_TWENTYFOURBITRGB	4

/*****************************************************************************
    Internal Color Spaces
******************************************************************************/
#define NX_ONEISWHITECOLORSPACE	0
#define NX_ONEISBLACKCOLORSPACE	1
#define NX_RGBCOLORSPACE	2
#define NX_CMYKCOLORSPACE	3

/*****************************************************************************
    Bag Channels
******************************************************************************/
#define BPCHAN		 0	/* BitPiece channel */
#define VISCHAN		 1	/* Visible channel */
#define BACKCHAN	 2	/* Backing channel */

#define BLACK_COLOR	0x00000000
#define DKGRAY_COLOR    0x55555555
#define LTGRAY_COLOR    0xAAAAAAAA
#define WHITE_COLOR	0xFFFFFFFF

/*****************************************************************************
    Alpha Contstants
******************************************************************************/
#define TRANSPARENT	0	/* equivalent to (0.0 setalpha) */
#define OPAQUE		255	/* equivalent to (1.0 setalpha) */

/*****************************************************************************
    Window Types	(DON'T CHANGE THE ORDER OF THIS LIST)
    
    DUMMY		An internal, unreal window.
    RETAINED		An backing store holds obscured parts of window.
    NONRETAINED		Only onscreen parts of window exist.
    BUFFERED		Complete offscreen copy holds entire contents.
******************************************************************************/
 
#define DUMMY		-1
#define RETAINED	 0
#define NONRETAINED	 1
#define BUFFERED	 2

/*****************************************************************************
    Window Alpha States
******************************************************************************/

#define A_BITS		0	/* Explicit alpha bits */
#define A_ONE		1	/* Implicit opaque alpha */

/* Data Structure Declarations */

typedef struct _layer Layer;
typedef struct _NXDevice NXDevice;
typedef struct _NXDriver NXDriver;
typedef struct _windowlist WindowList;

typedef struct {
    char *name;
    void (*proc)();
} NXRegOpsVector[];

/*****************************************************************************
    SubList is part of a WindowList.
******************************************************************************/

typedef struct _sublist {
    Layer **ptr;	/* Pointer to array of layer pointers. */
    int len;		/* Length of said array. */
} SubList;


/*****************************************************************************
    Composite Operators

    These operators and their mapping is documented in dpsNeXT.h.
    DISSOLVE is not really an operator and is not exported.
******************************************************************************/
#ifndef CLEAR
#define CLEAR		0	/* Clear destination */
#define COPY		1	/* Copy source to destination */
#define SOVER		2	/* Source over destination */
#define SIN		3	/* Source in destination */
#define SOUT		4	/* Source out of destination */
#define SATOP		5	/* Source atop destination */
#define DOVER		6	/* Destination over source */
#define DIN		7	/* Destination in source */
#define DOUT		8	/* Destination out of source */
#define DATOP		9	/* Destination atop source */
#define XOR		10	/* Exclusive-Or */
#define PLUS		11	/* For backward compatibility (PLUSD) */
#define PLUSD		11	/* Plus darken */
#define HIGHLIGHT	12	/* white > lgray > white */
#define PLUSL		13	/* Plus lighten */
#define DISSOLVE	14	/* Fake operator */
#define NCOMPOSITEOPS	15
#endif CLEAR


/*****************************************************************************
    PostScript Mark Record
******************************************************************************/

/* MarkRec bundles up Mark arguments */
typedef struct _markrec {
    PDevice device;
    DevPrim *graphic;		/* In window coordinates */
    DevPrim *clip;		/* In window coordinates */
    DevMarkInfo info;
} MarkRec;

typedef struct _point {
    short x, y;
} Point;

typedef struct _bounds {
    short minx, maxx, miny, maxy;
} Bounds;

/* Bundle up most arguments for LCompositeFrom */

typedef struct _compositeinfo { 
    int		  op;
    Layer	  *swin;
    Layer	  *dwin;
    Point	  offset;
    unsigned char alpha;
    DevMarkInfo	  markInfo;
} CompositeInfo;

/* Structure for retrieving current device status of a window */

typedef struct _devicestatus {
    unsigned char minbitsperpixel; /* Min bps of any device window is on */
    unsigned char maxbitsperpixel; /* Max bps of any device window is on */
    unsigned char color;	   /* Is window on any color device */
    unsigned char reserved;
} DeviceStatus;

typedef struct _pattern {
    unsigned char type;		/* Should always be PATTERN */
    unsigned char permanent;	/* Never delete if true */
    unsigned char alpha;	/* Alpha value */
    unsigned char reserved;
    DevColor	  color;	/* Color, captured from gstate */
    DevPoint	  phase;	/* Screen phase */
    DevHalftone   *halftone;
} Pattern;

struct _any {
    unsigned char type;
};

/* Defined are six WindowServer "hooks" that device drivers can capture for
 * specific windows.  This can grow to 32 hooks as limited by the "hook"
 * field in the Bag structure.  Bit position in "hooK" is 1<<n where n is
 * any one of the following enumerated types.
 */
typedef enum _NXHook {
    NX_MOVEWINDOW,
    NX_OBSCUREWINDOW,
    NX_ORDERWINDOW,
    NX_PLACEWINDOW,
    NX_REPAINTWINDOW,
    NX_REVEALWINDOW
} NXHook;

typedef struct _NXHookData {
    NXHook type;
    struct _NXBag *bag;
    union {
	struct {
	    short stage;
	    Point delta;
	} move;
	struct {
	    Bounds bounds;
	} obscure;
	struct {
	    char where;
	    char inWindowList;
	} order;
	struct {
	    Bounds newBounds;
	    struct _NXBag *newBag;
	    struct _layer *newLayer;
	} place;
	struct {
	    Bounds bounds;
	} repaint;
	struct {
	    Bounds bounds;
	} reveal;
    } d;
} NXHookData;


/* A Bag is a common structure that each bitpiece in a bintree points to
 * for bitmap information. "device" points to the device which the bitpiece
 * spatially overlaps.  In the case of Fixed windows, the backbits an
 * alphabits may be owned by a pre-chosen device not necessarily the same as
 * "device". Drawing should be done by calling the device of the bitmap, not
 * of Bag.
 */
typedef struct _NXBag {
    unsigned char type;		/* BAG */
    unsigned    mismatch:1;	/* Retained: screen and backing mismatch? */
    unsigned	:7;
    short	refCount;	/* Reference count to determine death */
    int		hookMask;	/* Mask of 1<<(NXHook values) */
    Layer	*layer;		/* blind ptr to window (for drivers) */
    NXDevice	*device;	/* Device spacially associated with us */
    struct _NXBag *next;	/* Ptr to next bag in layer->baglist */
    struct _NXProcs *procs;	/* Copy of driver's proc ptr */
    _Bitmap	*visbits;	/* Bitmap of screen's visible bits */
    _Bitmap	*backbits;	/* Bitmap of allocated backing store */
    void	*priv;		/* Private storage for device */
} NXBag;

/* CompEl: Only bag and pattern should appear to the device driver. The bitmap
 * field is permitted for the device's use only.
 */
typedef union _compel {
    struct _bitpiece	*bp;
    struct _divpiece	*dp;
    struct _pattern	*pat;
    struct _NXBag	*bag;
    struct _any		*any;
} CompEl;


/*****************************************************************************
    CompositeOperation Structure
    
    A structure that completely describes one composite operation.
    Note that although the types are very permissive here, particular routines
    below impose tighter restrictions.  Also, by convention, whenever a
    CompositeOperation ptr is passed to a routine, that routine may destroy
    the contents of the structure.

    Ultimately, every composite operation affects a destination bag specified
    by the "dst" field before being routed to a device's Composite() procedure.    
    The source of a composite operation may be either real or virtual, that is,
    may be a bag or a pattern.

    "dstH" and "srcCH" specify the channels to use in their associated bag.
    Before they reach the device, these must be either VISCHAN or BACKCHAN.
    
    "dstAS" and "srcAS" tell the device the state of the two operands, i.e.,
    whether or not the alpha is explicitly allocated or implicitly opaque.

    "alpha" is only provided if the operation is a DISSOLVE.

    "srcBounds is a global rect that specifies the area for the operation.
******************************************************************************/
typedef struct {
    CompEl	dst;		/* Destination for rgba or data channel */
    CompEl	src;		/* Source for rgba or data channel */
    unsigned	dstCH:4;	/* Bintree destination channel specifier */
    unsigned	srcCH:4;	/* Bintree source channel specifier */
    unsigned	dstAS:1;	/* Destination alpha state */
    unsigned	srcAS:1;	/* Source alpha state */
    unsigned	srcIsDst:1;	/* Destination is its own source */
    unsigned    doAlpha:1;	/* Whether to do alpha or not */
    unsigned    revealing:1;	/* True when revealing bits (tag support) */
    unsigned    obscuring:1;	/* True when obscuring bits (tag support) */
    unsigned    instancing:1;	/* True if dst only to occur on screen */
    unsigned	:1;
    unsigned char mode;		/* Transfer mode to use (e.g., COPY) */
    unsigned char alpha;	/* Dissolve delta */
    union {	int i;
		Point cd;
    } delta;			/* Delta from source to dest */
    Bounds	srcBounds;	/* Global source rectangle */
    DevMarkInfo info;
} CompositeOperation;

typedef struct _NXCursorInfo {
    Bounds *cursorRect;		/* Bounds of the cursor in device coords. */
    Bounds *saveRect;		/* Bounds of screen pixels saved in saveData */
    unsigned int *saveData;	/* Saved screen data (1024 max bytes) */
    unsigned int *cursorData2W;	/* 2bpp, 1 is white, gray plane (64 bytes) */
    unsigned int *cursorData2B;	/* 2bpp, 1 is black, gray plane (64 bytes) */
    unsigned int *cursorAlpha2; /* 2bpp, alpha plane (64 bytes) */
    unsigned int *cursorData8;	/* 16bpp, ga meshed (512 bytes) */
    unsigned int *cursorData16;	/* 16bpp, rgba meshed (512 bytes) */
    unsigned int *cursorData32;	/* 32bpp, rgba meshed (1024 bytes) */
    union {
	struct {
	    unsigned state2W:1;
	    unsigned state2B:1;
	    unsigned state8:1;
	    unsigned state16:1;
	    unsigned state32:1;
	} curs;
	unsigned short i;
    } set;
} NXCursorInfo;

typedef struct _NXProcs {
    void (*Composite)(CompositeOperation *cop, Bounds *dstBounds);
    void (*DisplayCursor)(NXDevice *device, NXCursorInfo *nxci);
    void (*FreeWindow)(NXBag *bag, int termflag);
    void (*Hook)(NXHookData *hookData);
    void (*InitScreen)(NXDevice *device);
    void (*Mark)(NXBag *bag, int channel, MarkRec *mrec, Bounds *markRect,
	 Bounds *bpRect);
    void (*MoveWindow)(NXBag *bag, short dx, short dy, Bounds *old, Bounds *new);
    void (*NewAlpha)(NXBag *bag);
    void (*NewWindow)(NXBag *bag, Bounds *bounds, int wType, int wDepth,
	 int local);
    void (*Ping)(NXDevice *device);
    void (*PromoteWindow)(NXBag *bag, Bounds *wBounds, int depth, int wType,
	 DevPoint phase);
    void (*RegisterScreen)(NXDevice *device);
    void (*RemoveCursor)(NXDevice *device, NXCursorInfo *nxci);
    void (*SetCursor)(NXDevice *device, NXCursorInfo *nxci, _LocalBitmap *lbm);
    void (*SyncCursor)(NXDevice *device, int syncFlag); /* 0 = async */
    int  (*WindowSize)(NXBag *bag);
} NXProcs;

/* An NXDriver structure is the interface between WindowServer and driver.
 * We will not remove or reorder the fields of this structue, but we may
 * add fields in the future, so avoid depending on the size of the structure.
 */
struct _NXDriver {
    NXDriver	*next;		/* Ptr to next driver entry */
    struct mach_header *header;	/* Pointer to driver's mach header */
    char	*name;		/* Framebuffer name */
    NXProcs	*procs;		/* Procedure vectors */
    void	*priv;		/* Driver specific use */
};

/* An NXDevice structure defines characteristics of a particular "screen".
 * We will not remove or reorder the fields of this structue, but we may
 * add fields in the future, so avoid depending on the size of the structure.
 */
struct _NXDevice {
    struct _NXDevice *next;	/* Next screen in line or null */
    NXDriver	*driver;	/* Owner */
    void	*priv;		/* Driver's private use */
    _Bitmap	*bm;		/* Screen's bitmap */
    Bounds	bounds;		/* Screen's boundary */
    Bounds	extent;		/* Extent of screen conversion */
    int		romid;		/* unique rom identification for device */
    int		visDepthLimit;	/* Maximum depth visible on me */
    char	slot;		/* Physical slot */
    char	unit;		/* Display index within a slot */
};

typedef struct {
    short id;			/* Window's id */
    unsigned char type;		/* NONRETAINED, RETAINED, or BUFFERED */
    unsigned local:1;		/* Does window require local shared memory? */
    unsigned opaque:1;		/* True if window is implicitly opaque */
    unsigned autofill:1;	/* Automatically fill window on exposure? */
    unsigned sendrepaint:1;	/* Send repaint event to app? */
    unsigned instancing:1;	/* Instance mode turned on? */
    unsigned :3;
    short currentdepth;		/* Window's current logical depth */
    short depthlimit;		/* Window's depth limit */
    NXBag *bags;		/* Window's linear bag list */
    int context;		/* Window's context id */
    Pattern *exposure;		/* Window's current exposure color */
    Bounds bounds;		/* Window's current bounds */
    Bounds dirty;		/* Window's dirty rect (Buffered windows) */
    Bounds instance;		/* Window's current instance rect */
} NXWindowInfo;

/* This is NeXT's gstate extension structure. (see gstate.c)
 * We will not remove or reorder the fields of this structure, but we may
 * add fields in the future, so avoid depending on the size of the structure.
 */
typedef struct _nextgsext {
    real realalpha;		/* floating point alpha */
    unsigned char alpha;	/* alpha truncated to 1/255 */
    unsigned instancing:1;	/* boolean: window instancing state */
    unsigned realscale:1;	/* boolean: window coordinate state */
    unsigned graypatstate:2;	/* pure 2-bit dithers (for scrollbars,etc) */
    unsigned patternpending:1;	/* triggers pattern usage in convert color */
    unsigned reserved:19;	/* padding */
} NextGSExt, *PNextGSExt;

#define ALPHAVALUE(ext) ((*((NextGSExt **)ext))->alpha)
#define REALALPHA(ext) ((*((NextGSExt **)ext))->realalpha)
#define INSTANCING(ext) ((*((NextGSExt **)ext))->instancing)
#define REALSCALE(ext) ((*((NextGSExt **)ext))->realscale)
#define GRAYPATSTATE(ext)((*((NextGSExt **)ext))->graypatstate)

#define NOGRAYPAT 0
#define DARKGRAYPAT 1
#define MEDGRAYPAT 2
#define LITEGRAYPAT 3


/*****************************************************************************
	Public Bintree Functions
******************************************************************************/

/**** bag ****/
extern NXBag *NXWID2Bag(short wid, NXDevice *device);
extern void NXSetHookMask(NXBag *bag, int mask);
extern int NXGetWindowInfo(Layer *layer, NXWindowInfo *wi);

/**** bounds ****/
extern int boundBounds(Bounds *one, Bounds *two, Bounds *result);
extern void clipBounds(Bounds *one, Bounds *two, Bounds *three);
extern void collapseBounds(Bounds *one, Bounds *two);
extern BBoxCompareResult DevBoundsCompare(DevBounds *figBds, DevBounds
    *clipBds);
extern int sectBounds(Bounds *one, Bounds *two, Bounds *result);
extern int withinBounds(Bounds *one, Bounds *two);

#define SETBOUNDS(one, a, b, c, d) \
    (one)->minx = a, (one)->maxx = b, (one)->miny = c, (one)->maxy = d

#define TOUCHBOUNDS(one, two) \
    (((one.minx < two.maxx) && (two.minx < one.maxx)) && \
    ((one.miny < two.maxy) && (two.miny < one.maxy)))

#define OFFSETBOUNDS(one, dx, dy) \
    one.minx += dx, one.maxx += dx, one.miny += dy, one.maxy += dy

#define NULLBOUNDS(one) \
    ((one.minx >= one.maxx) || (one.miny >= one.maxy)) 

/**** drivers ****/
extern void NXRegisterScreen(NXDriver *driver, short slot, short unit,
	    short width, short height);
extern void NXRegisterOps(const NXRegOpsVector);
extern void NXStartDriver(char *name);
extern void PingAsyncDrivers();

/***** layer *****/
extern struct _wd *Layer2Wd(Layer *layer);
extern void LayerInit(int reason);
extern void LAddToDirty(Layer *layer, Bounds *bounds);
extern Bounds *LBackingBounds(Layer *layer);
extern Bounds *LBoundsAt(Layer *layer);
extern void LCompositeFrom(CompositeInfo *ci, Bounds dstBounds);
extern _LocalBitmap *LCopyBitsFrom(Layer *layer, Bounds bounds,
	    boolean alphaWanted);
extern void LCopyContents(Layer *from, Layer *to);
extern int LCurrentAlphaState(Layer *layer);
extern int LCurrentDepth(Layer *layer);
extern int LDepthLimit(Layer *layer);
extern void LFill(Layer *layer, int op, Layer *other);
extern Layer *LFind(int x, int y, int op, Layer *other);
extern Bounds *LFindPieceBounds(Layer *layer, Point point);
extern void LFlushBits(Layer *layer);
extern void LFree(Layer *layer);
extern int LGetBacking(Layer *layer, unsigned int **bits, int *rowBytes);
extern DeviceStatus LGetDeviceStatus(Layer *layer);
extern void LGetSize(Layer *layer, short *width, short *height);
extern void LHideInstance(Layer *layer, Bounds bounds);
extern void LInitPage(PDevice device);
extern void LMark(PDevice device, DevPrim *graphic, DevPrim *clip,
	    DevMarkInfo *info);
extern void LMoveTo(Layer *layer, Point delta);
extern Layer *LNewAt(int layerType, Bounds bounds, int *win, int local,
	    int startDepth, int depthLimit);
extern void LNewInstance(Layer *layer);
extern void LOrder(Layer *layer, int place, Layer *other);
extern Layer *LPlaceAt(Layer *layer, Bounds newBounds);
extern int LPreCopyBitsFrom(Layer *layer);
extern void LPrintOn(Layer *layer, int dumpLevel);
extern void NXRenderBounds(Layer *layer, Bounds *b, void (*proc)(),
	    void *data);
extern void NXApplyBounds(Layer *layer, Bounds *b, void (*proc)(),
	    void *data);
extern void LSetAutofill(Layer *layer, int bool);
extern void LSetDepthLimit(Layer *layer, int depthLimit);
extern void LSetExposureColor(Layer *layer);
extern void LSetSendRepaint(Layer *layer, int flag);
extern int LSetType(Layer *layer, int newType);

/**** pattern ****/
extern Pattern *PNewColorAlpha(unsigned int color, unsigned char alpha,
    DevHalftone *htone);

/**** windowlist ****/
extern Layer *GetFrontWindow(void);
extern Layer *GetNextWindow(Layer *layer);
extern void GetTLWinBounds(Layer *layer, Bounds *bounds);
extern void GetWinBounds(Layer *layer, Bounds *bounds);
extern SubList WLAboveButNotAbove(int op1, Layer *win1, int op2, Layer *win2);
extern SubList WLBelowButNotBelow(int op1, Layer *win1, int op2, Layer *win2);


/*****************************************************************************
	Bintree Globals
******************************************************************************/
extern int driverCount;		/* Number of active drivers */
extern int deviceCount;		/* Number of active devices */
extern NXDevice *deviceList;	/* Head of device list */
extern NXDriver *driverList;	/* Head of driver list */
extern Bounds wsBounds;		/* Enormous bound of all screens */
extern short remapY;		/* Maps screen space to device space */
extern int initialDepthLimit;	/* WindowServer default depth limit */
extern int termwindowflag;	/* Signal that window is terminating */
extern int asyncDriversExist;	/* Global boolean informs ipcstream.c */

#endif BINTREE_H	
