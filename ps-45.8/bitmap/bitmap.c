/*
    bitmap.c -- abstract superclasses for all bitmaps

    Modifications:
    
    26Jul90 Terry Moved shortcut table here from bm3[84].c
    
*/

/* bitmap.c -- abstract superclasses for all bitmaps */

#import PACKAGE_SPECS
#import "bitmap.h"

/* BMCompositeShortcut returns the most efficient compositing op for a mode,
 * source and destination alphastates ([mode][srcAS][dstAS]).
 */
const char BMCompositeShortcut[NCOMPOSITEOPS][2][2] = {
    {{CLEAR,CLEAR},{CLEAR,CLEAR}},			// CLEAR
    {{COPY,COPY},{COPY,COPY}},				// COPY
    {{SOVER,SOVER},{COPY,COPY}},			// SOVER
    {{SIN,COPY},{SIN,COPY}},				// SIN
    {{SOUT,CLEAR},{SOUT,CLEAR}},			// SOUT
    {{SATOP,SOVER},{SIN,COPY}},				// SATOP
    {{DOVER,-1},{DOVER,-1}},				// DOVER
    {{DIN,DIN},{-1,-1}},				// DIN
    {{DOUT,DOUT},{CLEAR,CLEAR}},			// DOUT
    {{DATOP,DIN},{DOVER,-1}},				// DATOP
    {{XOR,DOUT},{SOUT,CLEAR}},				// XOR
    {{PLUSD,PLUSD},{PLUSD,PLUSD}},			// PLUSD
    {{HIGHLIGHT,HIGHLIGHT},{HIGHLIGHT,HIGHLIGHT}},	// HIGHLIGHT
    {{PLUSL,PLUSL},{PLUSL,PLUSL}},			// PLUSL
    {{DISSOLVE,DISSOLVE},{DISSOLVE,DISSOLVE}}		// DISSOLVE
};


static Bitmap *BMNew(BMClass *class, Bounds *b, int vmAllocate)
{
    Bitmap *bm;

    if (!class->_initialized) {
	(*class->_initClassVars)(class);
	class->_initialized = 1;
    }
    bm = (Bitmap *) calloc(class->_size,1);
    bm->isa = class;
    bm->bounds = *b;
    bm->refcount = 1;
    bm->type = NX_OTHERBMTYPE;	/* default is non-standard */
    return(bm);
}

static void BMFree(Bitmap *bm)
{
    free(bm);
}

static void BMInitClassVars(BMClass *class)
{
    class->_free = BMFree;
    class->_size = sizeof(Bitmap);
    /* subclass responsibility for the other methods */
}

/* The following three variables must be initialized in the static class
 * declaration (the rest are lazily filled in by the initialize method).
 */

BMClass _bmClass = {
    0,			/* private initialized boolean */
    BMInitClassVars,	/* subclass initialization method */
    BMNew,		/* new method */
    BMFree,		/* _free method */
    /* the rest is filled in on initialization */
};


