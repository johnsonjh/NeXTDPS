/* bm12.h --- class interface file */

#import "bitmap.h"

#define BM12BPP		2
#define BM12BPS		2
#define BM12SPP		1
#define BM12LOG2BD	1
#define BM12GRAYS	4
#define BM12SCANMASK	0x1F
#define BM12SCANSHIFT	5
#define BM12SCANUNIT	32

#define BM12scanmask (BM12SCANMASK >> BM12LOG2BD)
#define BM12scanshift (BM12SCANSHIFT - BM12LOG2BD)

#define BM12_INITIAL_MRBUFFLEN 288

typedef struct _BM12Class {
    LocalBMClass base;
} BM12Class;

extern BM12Class _bm12;
#define bm12 ((BMClass *)&_bm12)

/* BitsOrPatInfo defines the src[3] & dst args to the MoveRect function */
typedef enum {
    bitmapType, patternType, constantType
} BPorC;

typedef struct _bitsorpatinfo {
    BPorC type;			/* Chooses flavor of source */
    union {
	struct {
	    uint *pointer;	/* Beginning of source */
	    int	n;		/* units per row */
	    int	leftShift;	/* Shift count to align with dst */
	} bm;
	struct {
	    uint *yBase;	/* First longword of pattern */
	    int	n;		/* units per row */
	    uint *yInitial;	/* points to start of initial row */
	    uint *xInitial;	/* points to start longword in row */
	    uint *yLimit;	/* Word after last longword of pattern
				   (gives number of rows) */
	} pat;
	struct {
	    unsigned int value;	/* Constant value to propagate */
	} cons;
    } data;
} BitsOrPatInfo;


/* LineSource defines the source of a line transfer, be it a constant, a
 * bitmap, or a pattern.
 */
typedef struct {
    int type;			/* SCON, SBMU, SPAT, SBMA */
    union {
	struct {
	    uint value;		/* 32-bit const value to fill with */
	} cons;
	struct {
	    uint *pointer;	/* Pointer to first source longword */
	    uint leftShift;	/* to get aligned with destination */
	} bm;
	struct {
	    uint *pointer;	/* Ptr to first longword to use */
	    uint *limit;	/* Ptr to longword beyond end of row */
	    uint *base;		/* Value to reset ptr to when limit reached */
	} pat;
    } data;
} LineSource;


typedef struct _lineoperationcommon {
    uint	*dstPtr;	/* First word to be changed in destination */
    int		numInts;	/* Number of integers beyond the one pointed to
				   by dstPtr that we should change */
    uint	leftMask;	/* Mask to use for the leftmost word */
    uint	rightMask;	/* if numInts is > 0, then this gives the
				   mask for the rightmost int */
} LOCommon;

/* LineOperation structure defines a complete line transfer.
   It is the parameter to MoveLine, which accomplishes the complete
   transfer given and returns. */

typedef struct _lineoperation {
    LOCommon    loc;		/* The part most common to all stages */
    LineSource	source;		/* The source for this operation */
    void	(*(proc))();	/* Appropriate flavor of moveLine procedure */
} LineOperation;

#define F0(n) (n)
#define F1(n) (n + FIRSTF1OP)
#define F2(n) (n + FIRSTF2OP)
#define F3(n) (n + FIRSTF3OP)

/* MoveRect modes, 26 total used - some blank spots for hysterical reasons */

#ifndef FIRSTF1OP
/*
		     0: d = 0;
		     1: d = 1;
		     2: d = ~d;
		     3: d = d;
		     4: d = highlight(d) */

#define    FIRSTF1OP 5
/* FIRSTF1OP + 0 (5)  : d = (1-d)s;
   FIRSTF1OP + 1 (6)  : d = sd;
   FIRSTF1OP + 2 (7)  : d = s;
   FIRSTF1OP + 3 (8)  : d = (1-s)d;
   FIRSTF1OP + 4 (9)  : d = (1-s)d + (1-d)s;
   FIRSTF1OP + 5 (10) : d = s + d;
   FIRSTF1OP + 6 (11) : d = s + d - sd;
   FIRSTF1OP + 7 (12) : d = 1-s;
   FIRSTF1OP + 8 (13) : d = 1 - ((1-s) + (1-d)); */
#define    FIRSTF2OP (FIRSTF1OP+9)    /* 14 */
/* FIRSTF2OP + 0 (13):  d = s0 + (d(1-s1));
   FIRSTF2OP + 2 (15):  d = s0(1-s1);
   FIRSTF2OP + 3 (16):  d = s0 * s1;
   FIRSTF2OP + 4 (17):  d = d(s0 + (1-s1));
   FIRSTF2OP + 6 (18):  d = d + (s0(1-s1));
   FIRSTF2OP + 8 (19):  d = s0(1-d) + d(1-s1);
   FIRSTF2OP + 9 (20):  d = s0(1-d) + (d * s1);
   FIRSTF2OP + 10 (21): d = s0(1-s1) + d(1-s0);
   FIRSTF2OP + 11 (22): d = s0(1-s1) + (d * s0);
   FIRSTF2OP + 13 (24): d = (s0 * s1) + (d * s0); */
#define    FIRSTF3OP (FIRSTF2OP+14) /* 28 */
/* FIRSTF3OP + 0 (27) : d = (s0 * s1) + (d(1-s2));
   FIRSTF3OP + 2 (28) : d = (s0(1-s1)) + (d(1-s2));
   FIRSTF3OP + 3 (29) : d = (s0(1-s1)) + (d * s2); */
#define    FIRSTF4OP (FIRSTF3OP+4)
#endif

/* Values for lineOp:  Either straight copy or one of the write functions */

#define WCOPY 0
#define WF0 1	/* SD */
#define WF1 2	/* CEILING(S+D) */
#define WF2 3	/* (1-S)D */
#define WF3 4	/* S+D-SD */
#define WF4 5	/* 1 - CEILING((1-S) + (1-D)) *** (the PLUSL op) */

#define SCON 0	/* constant */
#define SBMU 1	/* bitmap, unaligned, 0 < leftShift < 32 */
#define SPAT 2	/* pattern */
#define SBMA 3	/* bitmap, aligned with destination, no shift */

#define PAABS(a) ((a) > 0 ? (a) : (-(a)))
#define ONES ((uint)0xFFFFFFFF)

void BM12Convert32to2(uint *, uint *, uint *, DevPoint, DevPoint, int,
	DevPoint, int, DevPoint);
void BM12Convert16to2(unsigned short *, uint *, uint *, DevPoint, DevPoint,
	int, DevPoint, int, DevPoint);
void BM12SetupBits(LocalBitmap *, Bounds *, int, int, int, BitsOrPatInfo *,
	BitsOrPatInfo *, DevMarkInfo *);
void BM12SetupPat(Pattern *, DevPoint, Bounds *, int, int, BitsOrPatInfo *);

/**** copyline ****/

void WCOPYBmULine(LineOperation *);
void WCOPYBmALine(LineOperation *);
void WCOPYPatLine(LineOperation *);
void WCOPYConLine(LineOperation *);
void RWCOPYBmULine(LineOperation *);
void RWCOPYBmALine(LineOperation *);

/**** funcline ****/

void WF0ConLine(), WF0BmULine(), WF0PatLine(), WF0BmALine(), WF0BmRLine(),
     WF1ConLine(), WF1BmULine(), WF1PatLine(), WF1BmALine(), WF1BmRLine(),
     WF2ConLine(), WF2BmULine(), WF2PatLine(), WF2BmALine(), WF2BmRLine(),
     WF3ConLine(), WF3BmULine(), WF3PatLine(), WF3BmALine(), WF3BmRLine(),
     WF4ConLine(), WF4BmULine(), WF4PatLine(), WF4BmALine(), WF4BmRLine();

/**** hlrect ****/

void	BM12HighlightRect(LineOperation *, uint, uint, uint);

/**** soverrect ****/

void	BM12SoverRect(LineOperation *, LineOperation *, int, int, int);
	       
/**** moverect ****/

void	BM12MRInitialize();
void	BM12SetUpSource(BitsOrPatInfo **, LineOperation *, int, int);
void	BM12MRMasks(uint *, uint *, int, int *, int);
void	BM12MRMoveRect(BitsOrPatInfo *[], BitsOrPatInfo *,  int,int, int, int,
	    int, int);
void	BM12MRPatternAdvance(LineSource *, BitsOrPatInfo *);
