/*****************************************************************************
    
    bintreetypes.h
    Private types and defines for use within the bintree package.

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.
    
    Created 16Dec87 Leo
    
    Modified:
     
    23Mar88 Leo   instanceSet bit in Layer
    26Mar88 Leo   New CompositeOperation structure
    31Mar88 Jack  Reorder A_BITS, etc to use directly in bitmap.c
    17Apr88 Leo   WdMarkInfo in CompositeOperation structure and Layer2Wd
    28Apr88 Jack  Remove Layer2Wd because typeclash with windowdevice.h
    30Aug88 Jack  Remove instancing from _layer
    20Jan89 Ted   Formatting, grokking, prep for multiscreen support
    25Jan89 Jack  Changed LCompositeBitsFrom to LCopyBitsFrom
    22Apr89 Ted   Changed "type" fields to unsigned chars from shorts
    16May89 Ted   Converted declarations to standard ANSI function prototypes
    02Feb90 Terry Changed MaxDepth stuff to Format stuff in bintree.h
    06Feb90 Terry Created BMNew macro
    28Mar90 Terry Removed backingbits, shmemfd + lastBounds from layer struct
    10Jun90 Ted   Removed LOffScreenTest, LShouldConvert, and LNewDummyCause
    28Jun90 Ted   Removed PieceOrPat and FindFormat structs (obsolete)
    28Jun90 Ted   Removed defunct "bits" field from BitPiece structure
    29Jun90 Ted   Removed obsolete "INVISIBLE" constant
    
******************************************************************************/

#import PACKAGE_SPECS
#import BINTREE
#import STREAM

/* Typedef all the structure types, so we don't have to chase forward-
 * reference problems everywhere.
 */
typedef struct _divpiece   DivPiece;
typedef struct _bitpiece   BitPiece;

/*****************************************************************************
   Composite Object Types

   In many places in the calls below, more than one type of object can be
   passed. To facilitate this, all objects have as their first short a type
   field. Here we declare the polymorphic structures.  The types for each
   object are declared here also.
******************************************************************************/
 
#define BITPIECE ((unsigned char)'b')	/*  98 */
#define DIVPIECE ((unsigned char)'d')	/* 100 */
#define LAYER    ((unsigned char)'l')	/* 108 */

#define NEXTSTEP_DISPLAY_PATH "/usr/lib/NextStep/Displays"
#define NEXTSTEP_PATH         "/usr/lib/NextStep"
#define START_SYMBOL          "_Start"

#define INF 16000	/* Extent of workspace bounds */

/* Special causes.  (Used only in call parameters, not in layer structure.) */

#define NOREASON	0
#define OFFSCREENREASON 1	/* For obscureInside, win removed from list */
#define ONSCREENREASON	2	/* For revealInside, win put in winList */

#define ISCONVERTCAUSE(c) (c & 0x80000000)
#define MAKECONVERTCAUSE(c) (c | 0x80000000)
#define STRIPCONVERTCAUSE(c) (c & 0x7fffffff)
#define CopybackRetained(l) ((l)->layerType==RETAINED&&(l)->alphaState!=A_BITS)

/* Constants for BitPiece.visFlag field */
/* Obscured BitPieces have a positive integer that indicate how many times
   they've been obscured.  At zero, the BitPiece is revealed. */

#define VISIBLE		0	/* On the screen, visible by user */
#define OFFSCREEN      -1	/* Temporarily cached offscreen */

#define REDRAW_EXPOSED	0
#define REDRAW_CHANGED	1

#define NX_BLACK	0x00
#define NX_DKGRAY	0x55
#define	NX_LTGRAY	0xAA
#define NX_WHITE	0xFF

/* Orientation constants in DivPieces */

#define H 0
#define V 1

#if STAGE==DEVELOP
#define DebugAssert(condition) if (! (condition)) CantHappen();
#else
#define DebugAssert(condition)
#endif

/* Macros for finding max or min values of bounds 'b' given orientation 'o' */
#define MaxBound(b, o) ((o) ? (&((b)->maxx)) : (&((b)->maxy)))
#define MinBound(b, o) ((o) ? (&((b)->minx)) : (&((b)->miny)))

/* BoundsBundle used by LRenderInBounds to pass data to its callback proc */
typedef struct {
    int chan;
    void *data;
    void (*proc)();
} BoundsBundle;

/* CBStruct is used for passing data from LCopyBitsFrom to BPCopyBitsFrom */
typedef struct {
    _Bitmap *bm;
    _BMCompOp *bcop;
} CBStruct;

typedef union _piece {
    BitPiece    *bp;
    DivPiece    *dp;
    struct _any	*any;
} Piece;

typedef struct _causeset {	/* 8 bytes */
    int		*causes;	/* Pointer to list of causes */
    short	capacity;	/* Length of said array */
    short	length;		/* Number of filled in elements */
} CauseSet;

struct _windowlist {		/* 12 bytes */
    Layer	**contents;	/* Pointer to array of layers */
    int		capacity;	/* Length of said array */
    int		length;		/* Number actually set */
};

struct _divpiece {		/* 16 bytes */
    unsigned char type;		/* DIVPIECE */
    unsigned char orient;	/* Either H or V for orientation of div    */
    short	divCoord;	/* Coordinate at which I divide the window */
    CauseSet	*causes;	/* The causes of me */
    Piece	l;		/* The lesser half */
    Piece	g;		/* The greater half */
};

struct _bitpiece {		/* 28 bytes */
    unsigned char type;		/* BITPIECE */
    unsigned char chan;		/* Current channel (visible or backing) */
    short	visFlag;	/* VISIBLE, OFFSCREEN, or obscure count */
    NXBag	*bag;		/* My device bag */
    Layer	*layer;		/* My window layer */
    NXDevice	*device;	/* Device I'm associated with */
    NXDevice	*conv;		/* Device to convert to */
    Bounds	bounds;		/* BitPiece rectangle in device coordinates */
};

struct _layer {			/* 64 bytes */
    unsigned char type;		/* LAYER */
    char	layerType;	/* DUMMY, RETAINED, NONRETAINED, or BUFFERED */
    unsigned    local:1;	/* Should backing be allocated on host? */
    unsigned    extent:1;	/* True if layer is a dummy extent layer */
    unsigned    capped:1;	/* Has layer reached the depth limit? */
    unsigned    autoFill:1;	/* NONRETAINED windows: auto fill w/pattern? */
    unsigned	alphaState:1;	/* A_ONE(1) or A_BITS(0) */
    unsigned	windowList:1;	/* Am I in the window list? */
    unsigned    sendRepaint:1;	/* NONRETAINED windows: send repaint event? */
    unsigned    instanceSet:1;	/* Does instance rect below contain drawing? */
    unsigned    processHooks:1;	/* True if layer should call hooks */
    unsigned    :9;		/* Future flags */
    Piece	tree;		/* Bintree for this layer */
    int		causeId;	/* The causeId when created */
    int		*psWin;		/* Ptr back to PSWindow structure */
    int		currentDepth;	/* Current "logical" depth of layer */
    int		depthLimit;	/* Current maximum depth layer can attain */
    NXBag	*bags;		/* linked list of bags per device */
    NXDevice	*device;	/* Device layer represents */
    Pattern	*exposure;	/* Exposure color */
    Bounds	bounds;		/* bounds of the layer */
    Bounds	dirty;		/* Dirty rect for BUFFERED windows only */
    Bounds	instance;	/* Instancing occurring in this rect */
    BitPiece	*lastPiece;	/* Piece where the last drawing occurred */
};

/**** bag ****/

extern void	    BAGInitialize();
extern NXBag	    *BAGNew(Layer *,NXDevice *);
extern NXBag	    *BAGNewAt(Layer *,NXDevice *,Bounds,int,int,int,int);
extern void	    BAGCompositeFrom(CompositeOperation *);
#define		    BAGDelete(bag) if (--bag->refCount==0) BAGFree(bag)
extern NXBag	    *BAGFind(Layer *,NXDevice *);
extern void	    BAGFree(NXBag *);
#define		    BAGDup(bag) ++bag->refCount, bag

/**** bitpiece ****/

extern void	    BPInitialize();
extern BitPiece     *BPNewAt(NXBag *,int,Bounds,int,Layer *,NXDevice *,
		    NXDevice *);
extern void	    BPAdjust(BitPiece *,short,short);
extern void	    BPAllocBag(BitPiece *);
extern void	    BPApplyBounds(BitPiece *bp, Bounds *bounds, void *data);
extern void	    BPApplyBoundsProc(BitPiece *, Bounds *, void *, void(*)());
extern void	    BPApplyProc(BitPiece *,void(*)());
extern DivPiece     *BPBecomeDivAt(BitPiece *,int, unsigned char);
extern void	    BPCompositeFrom(CompositeOperation *);
extern void	    BPCompositeTo(CompositeOperation *);
extern BitPiece     *BPCopy(BitPiece *);
extern void	    BPCopyback(BitPiece *);
extern void	    BPCopyBitsFrom(BitPiece *, Bounds *, void *data);
extern DivPiece     *BPDivideAt(BitPiece *,int, unsigned char,int);
extern Bounds       *BPFindPieceBounds(BitPiece *,Point);
extern void	    BPFree(BitPiece *);
extern int	    BPIsObscured(BitPiece *);
extern BitPiece     *BPMark(BitPiece *,MarkRec *,Bounds *,int,int);
extern BitPiece     *BPObscureBecause(BitPiece *,int);
extern Piece	    BPObscureInside(BitPiece *,Bounds,int);
extern void	    BPPointScreen(BitPiece *);
extern void	    BPPrintOn(BitPiece *,int);
extern void	    BPRenderInBounds(BitPiece *bp, Bounds *bounds, void *data);
extern void	    BPReplaceBits(BitPiece *);
extern BitPiece     *BPRevealBecause(BitPiece *,int);
extern BitPiece     *BPRevealInside(BitPiece *,Bounds,int);

/**** bounds ****/

extern void	    BoundsFromIPrim(DevPrim *,Bounds *);
extern void	    divBoundsAt(Bounds *,Bounds *,Bounds *,int, unsigned char);
extern void	    FindDiffBounds(Bounds *,Bounds *,void(*)(),void *);
extern BBoxCompareResult IntersectAndCompareBounds(Bounds *,Bounds *,Bounds *);

/**** causeset ****/

extern void	    CSInitialize();
extern CauseSet     *CSNew();
extern CauseSet     *CSAdd(CauseSet *,int);
extern CauseSet     *CSAddSet(CauseSet *,CauseSet *);
extern CauseSet     *CSCopy(CauseSet *);
extern void	    CSFree(CauseSet *);
extern void	    CSPrintOn(CauseSet *);
extern int	    CSRemove(CauseSet *,int);
extern void	    CSSwapFor(CauseSet *,int,int);

/**** divpiece ****/

extern void	    DPInitialize();
extern DivPiece     *DPNewAt(int,int,Piece,Piece,Layer *, unsigned char);
extern void	    DPAdjust(DivPiece *,short,short);
extern void	    DPApplyBoundsProc(DivPiece *, Bounds *, void *, void(*)());
extern void	    DPApplyProc(DivPiece *,void(*)());
extern DivPiece     *DPBecomeDivAt(DivPiece *,int, unsigned char);
extern void	    DPCompositeFrom(CompositeOperation *);
extern void	    DPCompositeTo(CompositeOperation *);
extern DivPiece     *DPDivideAt(DivPiece *,int, unsigned char,int);
extern Bounds       *DPFindPieceBounds(DivPiece *,Point);
extern void	    DPFree(DivPiece *);
extern BitPiece     *DPMark(DivPiece *,MarkRec *,Bounds *,int,int);
extern DivPiece     *DPObscureBecause(DivPiece *,int);
extern DivPiece     *DPObscureInside(DivPiece *,Bounds,int);
extern void	    DPPrintOn(DivPiece *,int);
extern DivPiece     *DPRevealBecause(DivPiece *,int);
extern Piece	    DPRevealInside(DivPiece *,Bounds,int);
extern void	    DPSwapCause(DivPiece *,int,int);

/**** extent ****/

extern void	    EXInitialize();

/**** layer ****/

extern void	    copyCO(CompositeOperation *);
extern Layer	    *LNewDummyAt(Bounds);
extern void	    LRedraw(Layer *,Bounds *,int);
extern void	    LRepaintIn(Layer *,Bounds,NXBag *);

/**** pattern ****/

extern void	    PInitialize();
extern Pattern      *PNew();
extern void	    PFree(Pattern *);
extern Pattern      *PPrintOn(Pattern *);
extern void	    PSetHalftone(Pattern *,DevMarkInfo *);

/**** piece ****/

extern void	    PieceAdjust(Piece,short,short);
extern void	    PieceApplyBoundsProc(Piece, Bounds *, void *, void(*)());
extern void	    PieceApplyProc(Piece,void(*)());
extern DivPiece     *PieceBecomeDivAt(Piece,int, unsigned char);
extern void	    PieceCompositeFrom(CompositeOperation *);
extern void	    PieceCompositeTo(CompositeOperation *);
extern DivPiece     *PieceDivideAt(Piece,int, unsigned char,int);
extern Bounds       *PieceFindPieceBounds(Piece,Point);
extern void	    PieceFree(Piece);
extern BitPiece     *PieceMark(Piece,MarkRec *,Bounds *,int,int);
extern Piece	    PieceObscureBecause(Piece,int);
extern Piece	    PieceObscureInside(Piece,Bounds,int);
extern void	    PiecePrintOn(Piece,int);
extern Piece	    PieceRevealBecause(Piece,int);
extern Piece	    PieceRevealInside(Piece,Bounds,int);

/**** windowlist ****/

extern void	    WLInitialize();
extern Layer	    *WLAt(int);
extern int	    WLOffsetOf(Layer *);
extern Layer	    *WLPutAfter(Layer *,Layer *);
extern Layer	    *WLPutBefore(Layer *,Layer *);
extern void	    WLRemove(Layer *);

/*****************************************************************************
	Private Bintree Globals
******************************************************************************/
extern NXDevice *deviceCause;		/* Device of current cause */
extern SubList dummySubList;		/* SubList of on&offscreen layers */
extern SubList offSubList;		/* SubList of offscreen layers */
extern SubList extSubList;		/* SubList of extent layers */
extern NXDevice *holeDevice;		/* Device to own holes in space */
extern Pattern *whitepattern;		/* General purpose opaque white pattern */
extern Pattern *blackpattern;		/* General purpose opaque black pattern */
extern NXHookData hookData;
