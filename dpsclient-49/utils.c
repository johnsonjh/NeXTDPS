#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
    utils.c

    This file has some utility routines, such as printing out various
    connection data structures, an internal cover for malloc, and a
    userpath interface.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#include "dpsclient.h"
#include "wraps.h"
#include "defs.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


typedef struct {	/* param block for user path routines */
    void *coords;		/* coords of path */
    int numCoords;		/* number of coords */
    DPSNumberFormat type;	/* type of numbers */
    int numSize;		/* size of this type of number */
    char *ops;			/* operators to use to build the path */
    int numOps;			/* number of operators */
    void *bbox;			/* bounding box of data */
    DPSContext ctxt;		/* context to send to */
    int insertBBoxOp;		/* Do we need to insert a dps_setbbox? */
    float *matrix;		/* optional matrix for stroke ops */
} UserPathParams;

static void binaryUserPath(UserPathParams *p, int action);
static void sendUserPathWithGsave(UserPathParams *p, void (*func)());
static void sendUserPathWithNewpath(UserPathParams *p, void (*func)());
static void sendUserPath(UserPathParams *p);
static void sendCorrectUserPath(UserPathParams *p);
static void sendUserPathChunks(UserPathParams *p, int maxChunk);
static int getNumSize(DPSNumberFormat type);
static void *getNextCoord(void *data, DPSNumberFormat type, float *floatNum);


/* prints out a PostScript error in a nice way. */
void DPSPrintError(FILE *fp, const DPSBinObjSeqRec *e)
{
    register int i;
    const DPSBinObjRec *topObj;
    const DPSBinObjRec *obj;

    ASSERT((e->objects[0].attributedType & 0x7f) == DPS_ARRAY,
					"Non-array PS error");
    topObj = &(e->objects[0]);
    obj = &(e->objects[1]);
    ASSERT((obj->attributedType & 0x7f) == DPS_NAME,
					"Non-name first element in PS error");
    if (obj->length == 5 &&	/* if its a "standard error" */
	    !strncmp(((char *)topObj) + obj->val.nameVal, "Error", 5)) {
	ASSERT(topObj->length == 3 || topObj->length == 4,
					"PS Error array not 3 or 4 elements");
	fprintf(fp, "%%%%[ Error: ");
	obj++;
	fwrite(((char *)topObj) + obj->val.nameVal, sizeof(char),
							obj->length, fp);
	fprintf(fp, "; OffendingCommand: ");
	obj++;
	if ((obj->attributedType & 0x7f) == DPS_STRING)
	    fwrite(((char *)topObj) + obj->val.stringVal, sizeof(char),
							obj->length, fp);
	else {
	    NXStream *st;

	    fflush(fp);
	    if (st = NXOpenFile(fileno(fp), NX_WRITEONLY)) {
		_DPSConvertBinArray(st, obj, 1, 0, e, dps_ascii, dps_strings,
									FALSE);
		NXClose(st);
	    } /* else error */
	}
	fprintf(fp, " ]%%%%\n");
    } else
	ASSERT(FALSE, "unknown error format");
}


/* prints out a PostScript error in a nice way on a NXStream. */
void DPSPrintErrorToStream(NXStream *st, const DPSBinObjSeqRec *e)
{
    register int i;
    const DPSBinObjRec *topObj;
    const DPSBinObjRec *obj;

    ASSERT((e->objects[0].attributedType & 0x7f) == DPS_ARRAY,
					"Non-array PS error");
    topObj = &(e->objects[0]);
    obj = &(e->objects[1]);
    ASSERT((obj->attributedType & 0x7f) == DPS_NAME,
					"Non-name first element in PS error");
    if (obj->length == 5 &&	/* if its a "standard error" */
	    !strncmp(((char *)topObj) + obj->val.nameVal, "Error", 5)) {
	ASSERT(topObj->length == 3 || topObj->length == 4,
					"PS Error array not 3 or 4 elements");
	NXPrintf(st, "%%%%[ Error: ");
	obj++;
	NXWrite(st, ((char *)topObj) + obj->val.nameVal, obj->length);
	NXPrintf(st, "; OffendingCommand: ");
	obj++;
	if ((obj->attributedType & 0x7f) == DPS_STRING)
	    NXWrite(st, ((char *)topObj) + obj->val.stringVal, obj->length);
	else
	    _DPSConvertBinArray(st, obj, 1, 0, e, dps_ascii, dps_strings,
								FALSE);
	NXPrintf(st, " ]%%%%\n");
    } else
	ASSERT(FALSE, "unknown error format");
}


void _DPSDefaultTextProc(ctxt, buf, count)
DPSContext ctxt;
char *buf;
long unsigned int count;
{
    fwrite(buf, 1, count, stderr);	/* temp??? */
}


void _DPSPrintEvent(FILE *fp, NXEvent *e)
{
    static char * const eNames[NX_LASTEVENT+1] = {
					"NullEvent",
					"LMouseDown",
					"LMouseUp",
					"RMouseDown",
					"RMouseUp",
					"MouseMoved",
					"LMouseDragged",
					"RMouseDragged",
					"MouseEntered",
					"MouseExited",
					"KeyDown",
					"KeyUp",
					"FlagsChanged",
					"Kitdefined",
					"SysDefined",
					"AppDefined",
					"Timer",
					"CursorUpdate",
					"Journaling"
					};

    if (e->type < sizeof(eNames)/sizeof(char*))
	fprintf(fp, "%s", eNames[e->type]);
    else
	fprintf(fp, "Unknown");
    fprintf(fp, " at: %.1f,%.1f", e->location.x, e->location.y);
    fprintf(fp, " time: %d", e->time);
    fprintf(fp, " flags: %#x", e->flags);
    fprintf(fp, " win: %hu", e->window);
    fprintf(fp, " ctxt: %x", e->ctxt);
    switch(e->type) {
	case NX_LMOUSEDOWN:
	case NX_LMOUSEUP:
	case NX_RMOUSEDOWN:
	case NX_RMOUSEUP:
	    fprintf(fp, " data: %hd,%d\n", e->data.mouse.eventNum,
						e->data.mouse.click);
	    break;
	case NX_KEYDOWN:
	case NX_KEYUP:
	    fprintf(fp, " data: %hd,%hu,%hu,%hu,%hd\n", e->data.key.repeat,
		     e->data.key.charSet, e->data.key.charCode,
		     e->data.key.keyCode, e->data.key.keyData);
	    break;
	case NX_MOUSEENTERED:
	case NX_MOUSEEXITED:
	    fprintf(fp, " data: %hd,%d,%x\n",e->data.tracking.eventNum,
						e->data.tracking.trackingNum,
						e->data.tracking.userData);
	    break;
	case NX_MOUSEMOVEDMASK:		/* these dont use the data union */
	case NX_LMOUSEDRAGGEDMASK:
	case NX_RMOUSEDRAGGEDMASK:
	case NX_FLAGSCHANGEDMASK:
	    break;
	default:
	    fprintf(fp, " data: %hd,%x,%x\n", e->data.compound.subtype,
						e->data.compound.misc.L[0],
						e->data.compound.misc.L[1]);
	    break;
    }
}


/* my version of malloc */
void *_DPSLocalMalloc(unsigned size)
{
    register char *ret;

    if (!(ret = malloc(size)))
	RAISE_ERROR(0, dps_err_outOfMemory, (void *)size, 0);
    return ret;
}


/* my version of realloc. uses malloc if ptr is NULL */
void *_DPSLocalRealloc(void *ptr, unsigned size)
{
    register char *ret;

    if (!(ret = ptr ? realloc(ptr, size) : malloc(size)))
	RAISE_ERROR(0, dps_err_outOfMemory, (void *)size, 0);
    return ret;
}

#ifdef DEBUG
/* prints out info about a port */
static void portInfo(port)
port_t port;
{
    port_name_t set;
    int num_msgs;
    int backlog;
    boolean_t owner;
    boolean_t receiver;
    kern_return_t ret;

    ret = port_status(task_self(), port, &set, &num_msgs, &backlog,
							&owner, &receiver);
    fprintf(stderr,
	"ret=%d set=%d num_msgs=%d backlog=%d owner=%d recvr=%d\n",
			ret, set, num_msgs, backlog, owner, receiver);
}


/* globals for tracing */
static FILE *TraceFile = NULL;
extern int _DPSMsgTracingOn;

/* called like _DPSTraceOutput(int printTime, char *fmt, ...) */
void _DPSTraceOutput(int doTime, char *fmt, ...)
{
    register va_list args;
    struct timeval now;

    if (_DPSMsgTracingOn) {
	if (!TraceFile)
	    if (!(TraceFile = fopen("dpsout", "w")))
		fprintf(stderr, "Cant open tracing file\n");
	
	if (doTime) {
	    gettimeofday(&now, NULL);
	    fprintf(TraceFile, "%d.%06d  ", now.tv_sec % 1000,
							now.tv_usec);
	}
	va_start(args, fmt);
	(void)vfprintf(stderr, va_arg(args, char *), args);
	va_end(args);
    }
}

#endif


/* old version without matrix.  We fill the old branch table slow with this
   version.  New code writers must use new API.
 */
void DPSDoUserPath(
    void *coords,		/* coords of path */
    int numCoords,		/* number of coords */
    DPSNumberFormat type,	/* type of numbers */
    char *ops,			/* operators to use to build the path */
    int numOps,			/* number of operators */
    void *bbox,			/* bounding box of data */
    int action			/* action to perform on path */
)
{
    DPSDoUserPathWithMatrix(coords, numCoords, type, ops, numOps, bbox, action, NULL);
}


/* convenient way to send a userpath and an operator to work on it.  Works
   for both binary and ascii contexts.  For ascii contexts, if for certain
   graphics operators we just generate the path and then call the correpsonding
   non-userpath operator (fill instead of ufill).  For other, we enumerate
   the path within an executable array, so it is a real userpath when the
   unknown op receives it.
 */
void DPSDoUserPathWithMatrix(
    void *coords,		/* coords of path */
    int numCoords,		/* number of coords */
    DPSNumberFormat type,	/* type of numbers */
    char *ops,			/* operators to use to build the path */
    int numOps,			/* number of operators */
    void *bbox,			/* bounding box of data */
    int action,			/* action to perform on path */
    float matrix[6]		/* optional matrix for stroke ops */
)
{
    UserPathParams p;
    const char *actionString;

    p.coords = coords;
    p.numCoords = numCoords;
    p.type = type;
    p.numSize = getNumSize(type);
    if (!p.numSize) {
	fprintf(stderr, "DPSDoUserPath: Invalid number type: %d\n", type);
	return;
    }
    p.ops = ops;
    p.numOps = numOps;
    p.bbox = bbox;
    p.ctxt = DPSGetCurrentContext();
    p.insertBBoxOp = (ops[0] != dps_setbbox &&
			!(ops[0] == dps_ucache && ops[1] == dps_setbbox));
    p.matrix = matrix;
    if (matrix && action != dps_ustrokepath &&
		action != dps_ustroke && action != dps_inustroke) {
	fprintf(stderr,
		"DPSDoUserPath: Matrix used with invalid action %d\n", action);
	return;
    }

    if (p.ctxt->programEncoding == dps_binObjSeq)
	binaryUserPath(&p, action);
    else
	switch (action) {
	    case dps_uappend:
		sendUserPath(&p);
		break;
	    case dps_ustrokepath:
		sendUserPathWithNewpath(&p, &PSstrokepath);
		break;
	    case dps_ufill:
		sendUserPathWithGsave(&p, &PSfill);
		break;
	    case dps_ueofill:
		sendUserPathWithGsave(&p, &PSeofill);
		break;
	    case dps_ustroke:
		sendUserPathWithGsave(&p, &PSstroke);
		break;
	    default:
		sendCorrectUserPath(&p);
		if (matrix)
		    PSconcat(matrix);
		actionString = DPSNameFromTypeAndIndex(-1, action);
		if (actionString)
		    DPSPrintf(p.ctxt, "%s\n", actionString);
		else {
		    fprintf(stderr,
			"DPSDoUserPath: Invalid action: %d\n", action);
		    PSpop();
		}
	}
}


static void binaryUserPath(UserPathParams *p, int action)
{
    typedef struct {
	unsigned char tokenType;
	unsigned char escape;
	unsigned short nTopElements;
	unsigned long length;
	DPSBinObjGeneric dataArrayObj;
	DPSBinObjGeneric actionObj;
	DPSBinObjGeneric dataStringObj;
	DPSBinObjGeneric opStringObj;
    } noMatrixBinSeq;
    static const noMatrixBinSeq noMatrixTemplate = {
	DPS_DEF_TOKENTYPE, 0, 2, sizeof(noMatrixBinSeq),
	{DPS_LITERAL|DPS_ARRAY, 0, 2, 16},
	{DPS_EXEC|DPS_NAME, 0, -1, 0},
	{DPS_LITERAL|DPS_STRING, 0, 0, 32},
	{DPS_LITERAL|DPS_STRING, 0, 0, 0}
    };

    typedef struct {
	unsigned char tokenType;
	unsigned char escape;
	unsigned short nTopElements;
	unsigned long length;
	DPSBinObjGeneric dataArrayObj;
	DPSBinObjGeneric matrixObj;
	DPSBinObjGeneric actionObj;
	DPSBinObjGeneric dataStringObj;
	DPSBinObjGeneric opStringObj;
    } matrixBinSeq;
    static const matrixBinSeq matrixTemplate = {
	DPS_DEF_TOKENTYPE, 0, 3,
		sizeof(matrixTemplate) + 6 * sizeof(DPSBinObjGeneric),
	{DPS_LITERAL|DPS_ARRAY, 0, 2, 24},
	{DPS_LITERAL|DPS_ARRAY, 0, 6, 40},
	{DPS_EXEC|DPS_NAME, 0, -1, 0},
	{DPS_LITERAL|DPS_STRING, 0, 0, 88},
	{DPS_LITERAL|DPS_STRING, 0, 0, 0}
    };

    typedef struct {
	unsigned char tokenType;
	unsigned char escape;
	unsigned short nTopElements;
	unsigned long length;
	DPSBinObjRec objects[5];
    } generalBinSeq;
    generalBinSeq seq;

    struct {
	unsigned char intro;
	unsigned char numType;
	unsigned short size;
    } numHeader;
    char *ops = p->ops;
    int numOps = p->numOps;
    int currObj;		/* current object of objSeq */
    int currStringPos;		/* current pos in string segment of objseq */

  /* init header */
    numHeader.intro = 149;
    numHeader.numType = p->type;
    numHeader.size = p->numCoords + 4;

  /* copy template, start currObj at the actionObj */
    if (p->matrix) {
	seq = *(generalBinSeq *)&matrixTemplate;
	currObj = 2;
    } else {
	seq = *(generalBinSeq *)&noMatrixTemplate;
	currObj = 1;
    }

  /* init action object */
    seq.objects[currObj].val.nameVal = action;
    currObj++;

  /* init dataString object */
    seq.objects[currObj].length = sizeof(numHeader) + (4 + p->numCoords) *
    								p->numSize;
    /* dataStringObj.val is pre-set to the end of the objects */
    currStringPos = seq.objects[currObj].val.stringVal +
				seq.objects[currObj].length;
    seq.length += seq.objects[currObj].length;
    currObj++;

  /* init opsString object */
    seq.objects[currObj].length = numOps + p->insertBBoxOp;
    seq.objects[currObj].val.stringVal = currStringPos;
    seq.length += seq.objects[currObj].length;

    if (p->matrix) {
	DPSBinObjSeqWrite(p->ctxt, &seq, sizeof(matrixBinSeq));
	DPSWriteTypedObjectArray(p->ctxt, dps_tFloat, p->matrix, 6);
    } else
	DPSBinObjSeqWrite(p->ctxt, &seq, sizeof(noMatrixBinSeq));
    DPSWriteStringChars(p->ctxt, (char *)&numHeader, sizeof(numHeader));
    DPSWriteStringChars(p->ctxt, p->bbox, 4 * p->numSize);
    DPSWriteStringChars(p->ctxt, p->coords, p->numCoords * p->numSize);
    if (p->insertBBoxOp) {
	char setBBoxOp = dps_setbbox;

	if (ops[0] == dps_ucache) {
	    DPSWriteStringChars(p->ctxt, ops++, 1);
	    numOps--;
	}
	DPSWriteStringChars(p->ctxt, &setBBoxOp, 1);
    }
    DPSWriteStringChars(p->ctxt, ops, numOps);
}


static void sendUserPathWithGsave(UserPathParams *p, void (*func)())
{
    PSgsave();
    sendUserPathWithNewpath(p, func);
    PSgrestore();
}


static void sendUserPathWithNewpath(UserPathParams *p, void (*func)())
{
    PSnewpath();
    sendUserPath(p);
    if (p->matrix)
	PSconcat(p->matrix);
    (*func)();
}



static void sendUserPath(UserPathParams *p)
{
    PSsystemdict();
    PSbegin();
    sendUserPathChunks(p, 0);
    PSend();
}

#define MAX_OP_STACK	400

static void sendCorrectUserPath(UserPathParams *p)
{
    int numElements;

    numElements = p->numCoords + p->numOps + p->insertBBoxOp + 4;
    if (numElements < MAX_OP_STACK)
	sendUserPathChunks(p, MAX_OP_STACK);
    else {
	PSarray(numElements);
	PSmark();
	sendUserPathChunks(p, MAX_OP_STACK);
	DPSPrintf(p->ctxt, "_NXCombineArrays\n");
    }
}


/* generic proto for the wraps we call for each op */
typedef void (*pathFunc)(float, float, float, float, float, float);

static const struct {
    pathFunc func;
    int numArgs;
} opTable[] = {	{(pathFunc)PSsetbbox, 4},
		{(pathFunc)PSmoveto, 2},
		{(pathFunc)PSrmoveto, 2},
		{(pathFunc)PSlineto, 2},
		{(pathFunc)PSrlineto, 2},
		{(pathFunc)PScurveto, 6},
		{(pathFunc)PSrcurveto, 6},
		{(pathFunc)PSarc, 5},
		{(pathFunc)PSarcn, 5},
		{(pathFunc)PSarcto, 5},
		{(pathFunc)PSclosepath, 0},
		{(pathFunc)PSucache, 0}
	};

#define MAX_ARGS	6

/* sends a user path down in exec array chunks no bigger than maxChunk.
   Is maxChunk is zero, then it uses no enclosing array structure.
 */
static void sendUserPathChunks(UserPathParams *p, int maxChunk)
{
    int i;
    float args[MAX_ARGS];
    float *currArg;
    int numArgs;
    int opCount;
    void *coords = p->coords;
    int numCoords = p->numCoords;
    char *ops = p->ops;
    int numOps = p->numOps;
    int chunkOps;
    float floatBBox[4];
    void *bboxPtr;

    if (maxChunk)
	DPSWriteData(p->ctxt, "{\n", 2);
    opCount = 1;
    chunkOps = 0;
    if (*ops == dps_ucache) {
	PSucache();
	ops++;
	numOps--;
	chunkOps++;
    }
    for (i = 0, bboxPtr = p->bbox; i < 4; i++)
	bboxPtr = getNextCoord(bboxPtr, p->type, floatBBox + i);
    chunkOps += 5;
    if (*ops == dps_setbbox) {
	ops++;
	numOps--;
    }
    PSsetbbox(floatBBox[0], floatBBox[1], floatBBox[2], floatBBox[3]);
    for ( ; numOps > 0 && numCoords >= 0; numOps--, ops++) {
	if (*ops > 32)
	    opCount = *ops - 32;
	else if (*ops >= dps_setbbox && *ops <= dps_ucache) {
	    while (opCount--) {
		numArgs = opTable[*ops].numArgs;
		chunkOps += numArgs + 1;
		if (maxChunk && chunkOps > maxChunk)
		    DPSWriteData(p->ctxt, "} {\n", 4);
		for (currArg = args, i = numArgs; i--; currArg++)
		    coords = getNextCoord(coords, p->type, currArg);
		(*(opTable[*ops].func))(args[0], args[1], args[2], args[3],
							args[4], args[5]);
		numCoords -= numArgs;
	    }
	    opCount = 1;
	} else
	    fprintf(stderr, "DPSDoUserPath: Invalid operator: %d\n", *ops);
    }
    if (numOps)
	fprintf(stderr, "DPSDoUserPath: %d extra operators\n", numOps);
    if (numCoords > 0)
	fprintf(stderr, "DPSDoUserPath: %d extra coordinates\n", numCoords);
    else if (numCoords < 0)
	fprintf(stderr, "DPSDoUserPath: %d too few coordinates\n", -numCoords);
    if (maxChunk)
	DPSWriteData(p->ctxt, "}\n", 3);
}


/* returns the size of a number specified by a userpath DPSNumberFormat */
static int getNumSize(DPSNumberFormat type)
{
    if ((type >= 0 && type <= 31) || (type == 48))
	return 4;
    else if (type >= 32 && type <= 47)
	return 2;
    else
	return 0;
}


static void *getNextCoord(void *data, DPSNumberFormat type, float *floatNum)
{
    int numSize;		/* size in bytes of one data element */

    if (type == 48) {
	numSize = 4;
	*floatNum = *(float *)data;
    } else if (type >= 0 && type <= 31) {
	numSize = 4;
	*floatNum = *(int *)data / (1 << type);
    } else if (type >= 32 && type <= 47) {
	numSize = 2;
	*floatNum = *(short *)data / (1 << (type-32));
    }
    return (char *)data + numSize;
}
