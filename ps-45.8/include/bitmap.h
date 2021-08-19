/*

To support the new multiple device API, the window server
needs to make more of a distinction between devices and bitmaps.
Previously, a device implementation consisted of a collection of
routines to manage bitmaps of a given depth.  In the new API, devices
are responsible for managing bitmaps of many depths.  So we are required to
develop a more manageable representation for bitmaps.  Here we follow the adobe `structs-as-objects' formalism, as well as using our current notion of a bitmap, make some abstract bitmap classes.

	The basic class hierarchy is:

			   Bitmap
			     |
		-------------------------
		|	     |		|
	  RemoteBitmap  LocalBitmap   MP12
			     |
		    -------------------
		    |     |     |     |
		   BM12  BM18  BM34  BM38
	       
*/

#import "devimage.h"
#import "bintree.h"
#import "devmark.h"

#define uchar unsigned char
#define uint unsigned int

#define MAXTRAPS 15

/* srcTypes */
#define BM_NOSRC 0
#define BM_BITMAPSRC 1
#define BM_PATTERNSRC 2

/* BmCompOp - composite operation */
typedef struct _BMCompOp {
    short srcType;
    short op;
    union {
	struct _Bitmap *bm;
	Pattern *pat;
    } src;
    Bounds srcBounds;
    Bounds dstBounds;
    unsigned char dissolveFactor;
    unsigned srcAS:1;
    unsigned dstAS:1;
    unsigned reserved:22;
    DevMarkInfo info;
} BMCompOp;
    

typedef struct _Bitmap {
    struct _BMClass *isa;
    
    /* public data */
    Bounds bounds;
    short type;			/* standard type definition */
    short refcount;
    short colorSpace;
    unsigned bps:6;
    unsigned spp:4;
    unsigned vram:1;
    unsigned alpha:1;
    unsigned planar:1;
    unsigned :3;
    void *priv;
} Bitmap;

#define LBM_DONTFREE 0
#define LBM_FREE 1
#define LBM_VMDEALLOCATE 2

typedef struct _LocalBitmap {
    Bitmap base;
    int rowBytes;
    unsigned freeMethod:2;
    unsigned :30;
    unsigned int *bits;
    unsigned int *abits;
    unsigned int byteSize;
} LocalBitmap;

typedef struct _Bitmap12 {
    LocalBitmap base;
} Bitmap12;

typedef struct _Bitmap18 {
    LocalBitmap base;
} Bitmap18;

typedef struct _Bitmap34 {
    LocalBitmap base;
} Bitmap34;

typedef struct _Bitmap38 {
    LocalBitmap base;
} Bitmap38;

/* Bitmap Class Structure */
typedef struct _BMClass {
    /* bootstrap methods and vars (classes are lazily initialized upon first
       invocation of `new' in one of its subclasses).  The `new' message
       cascades a series of `_initClassVars' messages to fill in superclass
       data.  If classes have first_time initialization, they should trap
       the first call to _initClassVars and call their startup code.  Recall
       that _initClassVars gets called once for each subclass that is
       initialized.  Note also that the _initialized field in a static class
       only gets set if an instance of that exact class is created.
       */
    int _initialized;
    void (*_initClassVars)(struct _BMClass *class);
    Bitmap *(*new)(struct _BMClass *class, Bounds *b, int local);

    /* `private' methods and vars */
    void (*_free)(Bitmap *bm);
    int _size;		/* size of instance struct for this class */

    /* `public' methods and vars */
    void (*composite)(Bitmap *bm, BMCompOp *op);
    void (*convertFrom)(Bitmap *db, Bitmap *sb, Bounds *dBounds,
    			Bounds *sBounds, DevPoint phase);
    Bitmap *(*makePublic)(Bitmap *bm, Bounds *hintBounds, int hintDepth);
    void (*mark)(Bitmap *bm, MarkRec *mrec, Bounds *markBds, Bounds *bpBds);
    void (*newAlpha)(Bitmap *bm, int initialize);
    void (*offset)(Bitmap *bm, short dx, short dy);
    void (*sizeBits)(struct _BMClass *class, Bounds *b, int *size,
		     int *rowBytes);
    int (*sizeInfo)(Bitmap *bm, int *localSize, int *remoteSize);
} BMClass;


/* Class variables for local bitmaps */
typedef struct _LocalBMClass {
    BMClass base;
    Bitmap *(*newFromData)(struct _LocalBMClass *class, Bounds *b,
	void *bits, void *abits, int byteSize, int rowBytes, int vram,
			   int freeMethod);
    void (*becomePSDevice)(LocalBitmap *bm);
    /* _mark: called after bitpiece clipping is done */
    void (*_mark)(Bitmap *bm, MarkRec *mrec, DevPrim *clip);

    /* Vars for PostScript device package */
    ImageArgs bmImArgs;		/* postscript args for setup image args */
    MarkProcsRec bmMarkProcs;	/* postscript marking procedures */
    PatternHandle bmPattern;	/* postscript pattern */
    PatternHandle grayPattern;	/* gray pattern for ui objects(bmpattern.c) */
    DevHalftone *defaultHalftone;
    int log2bd;

    /* Procedure callbacks used by PostScript device package */
    void (*wakeup)(PDevice device);
    void (*setupPattern)(PatternHandle h, DevMarkInfo *markInfo,
			 PatternData *data);
    void (*setupMark)(PDevice device, DevPrim **clip, MarkArgs *args);
    void (*setupImageArgs)(PDevice device, ImageArgs *args);
} LocalBMClass;    


/* BM12's, etc. have no new class variables, but for completeness
   we define them here trivially */


typedef struct _BM12Class { /* two-bit gray bitmaps */
    LocalBMClass base;
} BM12Class;

typedef struct _BM18Class { /* eight-bit gray bitmaps */
    LocalBMClass base;
} BM18Class;

typedef struct _BM34Class { /* twelve-bit RGB bitmaps */
    LocalBMClass base;
} BM34Class;

typedef struct _BM38Class { /* twenty-four-bit RGB bitmaps */
    LocalBMClass base;
} BM38Class;


typedef struct trptrp {
    Bitmap *bm;
    MarkRec *mrec;
    DevPrim trapsPrim;
} TrapTrapInfo;

/* Bitmap factory methods */

#define bm_sizeBits(class,bounds,size,rowBytes) \
	(*((BMClass *)class)->sizeBits)((BMClass *)class,bounds,size,rowBytes)

#define bm_new(class,bounds,local) \
	(*((BMClass *)class)->new)((BMClass *)class,bounds,local)

#define bm_free(bm) (*((BMClass *)((Bitmap *)bm)->isa)->_free)((Bitmap *)bm)

#define bm_delete(bm) if(--((Bitmap *)bm)->refcount == 0) bm_free(bm)

/* Bitmap methods */

#define bm_dup(bm) (((Bitmap *)bm)->refcount++,bm)

#define bm_composite(bm,bcop) \
	(*((BMClass *)((Bitmap *)bm)->isa)->composite)(bm,bcop);

#define bm_convertFrom(db,sb,dBounds,sBounds,phase) \
	(*((BMClass *)((Bitmap *)db)->isa)->convertFrom) \
	(db,sb,dBounds,sBounds,phase);

#define bm_makePublic(bm,hintBounds,hintDepth) \
	(*((BMClass *)((Bitmap *)bm)->isa)->makePublic)(bm,hintBounds,hintDepth)

#define bm_mark(bm,mrec,markBds,bpBds) \
	(*((BMClass *)((Bitmap *)bm)->isa)->mark)(bm,mrec,markBds,bpBds)

#define bm_newAlpha(bm,i) (*((BMClass *)((Bitmap *)bm)->isa)->newAlpha)(bm,i)

#define bm_offset(bm,dx,dy) (*((BMClass *)((Bitmap *)bm)->isa)->offset)(bm,dx,dy)

#define bm_sizeInfo(bm,localSize,remoteSize) \
	(*((BMClass *)((Bitmap *)bm)->isa)->sizeInfo)(bm,localSize,remoteSize)

/* LocalBitmap factory methods */

#define bm_newFromData(class,bounds,bits,abits,byteSize,rowBytes,vram,\
		       freeMethod) \
	(*((LocalBMClass *)class)->newFromData) \
	((LocalBMClass *)class,bounds,bits,abits,byteSize,rowBytes,vram,\
	 freeMethod)

/* LocalBitmap methods */

#define bm_becomePSDevice(bm) (*((LocalBMClass *) \
	((Bitmap *)bm)->isa)->becomePSDevice)((LocalBitmap *)bm)

#define bm__mark(bm,mrec,clip) (*((LocalBMClass *) \
	((Bitmap *)bm)->isa)->_mark)(bm,mrec,clip)

/* extern functions */

/* bmpattern.c */
PatternHandle NXGrayPat(int numpats, PatternData *pats, int alpha);

/* extern data */

/* bitmap.c */
extern const char BMCompositeShortcut[NCOMPOSITEOPS][2][2];

/* global bitmap class variables */
extern BMClass _bmClass;
extern LocalBMClass _localBM;
extern BM12Class _bm12;
extern BM18Class _bm18;
extern BM34Class _bm34;
extern BM38Class _bm38;

#define bmClass ((BMClass *)&_bmClass)
#define localBM ((BMClass *)&_localBM)
#define bm12    ((BMClass *)&_bm12)
#define bm18	((BMClass *)&_bm18)
#define bm34	((BMClass *)&_bm34)
#define bm38	((BMClass *)&_bm38)
