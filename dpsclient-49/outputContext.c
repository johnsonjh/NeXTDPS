#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
    outputContext.c

    This file has routines to fill in the procs record defined in
    dpsfriends.h that are shared by many types of contexts.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#include "dpsclient.h"
#include "defs.h"
#include "wraps.h"
#include <string.h>
#include <stdarg.h>

#define TOKEN_BUF_SIZE 20

static void _DPSDestroySpace();

static const DPSSpaceProcsRec _DPSSpaceProcs = {
    _DPSDestroySpace
};

static int formatArray();
static void indent();
static void writeOctal();
static void binObjSeqWriteGuts(DPSContext ctxtArg, char *data,
					int length, int start);

void
_DPSOutputBinObjSeqWrite( ctxtArg, data, length )
DPSContext ctxtArg;
char *data;
int length;
{
    binObjSeqWriteGuts( ctxtArg, data, length, TRUE );
}


/* start is a hint that this is the start of an object sequence */
static void
binObjSeqWriteGuts(DPSContext ctxtArg, char *data, int length, int start)
{
    _DPSOutputContext ctxt = STD_TO_OUTPUT(ctxtArg);
    int startOfSeq;		/* is this the start of a sequence? */
    int seqLength;		/* length according to the data */

    if( !length )	/* so empty strings dont look like bad seq starts */
	return;
    if( start || ctxt->binArrayLeft == 0 ) {
	startOfSeq = TRUE;
	ASSERT( BINARRAY_ESCAPE(data) == DPS_DEF_TOKENTYPE,
			    "Bad start for binarray in DPSBinWrite" );
	ASSERT( ctxt->binArrayLeft == 0, "Caught in middle of binarray" );
	if( ctxt->c.space->lastNameIndex < _DPSLastNameIndex )
	    DPSUpdateNameMap( ctxtArg );
	if( ctxt->c.chainChild )
	    binObjSeqWriteGuts( ctxt->c.chainChild, data, length, start );
	ctxt->binArrayLeft = BINARRAY_TOPOBJS(data) ? BINARRAY_LENGTH(data) :
						BINARRAY_EXLENGTH(data);
	ctxt->lengthMod4 = ctxt->binArrayLeft % 4;
    } else
	startOfSeq = FALSE;
    if( ctxt->c.programEncoding == dps_binObjSeq &&
		ctxt->c.nameEncoding == dps_indexed ) {
	NX_DURING
	    NXWrite( ctxt->outStream, data, length );
	NX_HANDLER
	    RERAISE_ERROR( ctxtArg, &NXLocalHandler );
	NX_ENDHANDLER
	ctxt->binArrayLeft -= length;
	ASSERT( ctxt->binArrayLeft < 256*256, "binArrayLeft negative" );
    } else {
	ASSERT( ctxt->c.programEncoding == dps_ascii,
			"Unknown encoding in _DPSStreamBinObjSeqWrite" );
	if( startOfSeq ) {	/* if start of seq, set up a buffer */
	    if( length == ctxt->binArrayLeft ) {
		ctxt->asciiBuffer = data;	/* say no to copying */
		goto skipTheCopy;
	    } else if ( ctxt->binArrayLeft <= ASCII_BUF_SIZE )
		ctxt->asciiBuffer = ctxt->asciiBufferSpace;
	    else
		MALLOC( ctxt->asciiBuffer, char, ctxt->binArrayLeft );
	    ctxt->asciiBufCurr = ctxt->asciiBuffer;
	}
	bcopy( data, ctxt->asciiBufCurr, length );
	ctxt->asciiBufCurr += length;
skipTheCopy:
	ctxt->binArrayLeft -= length;
	ASSERT( ctxt->binArrayLeft < 256*256, "binArrayLeft negative" );
	if( ctxt->binArrayLeft == 0 ) {
	    seqLength = BINARRAY_TOPOBJS(ctxt->asciiBuffer) ?
	    			BINARRAY_LENGTH(ctxt->asciiBuffer) :
				BINARRAY_EXLENGTH(ctxt->asciiBuffer);
	    NX_DURING
		if( BINARRAY_TOPOBJS(ctxt->asciiBuffer) )
		    _DPSConvertBinArray( ctxt->outStream,
			    BINARRAY_OBJECTS(ctxt->asciiBuffer),
			    BINARRAY_TOPOBJS(ctxt->asciiBuffer),
			    0, (DPSBinObjSeqRec *)(ctxt->asciiBuffer),
			    ctxt->c.programEncoding, ctxt->c.nameEncoding,
			    ctxt->debugging );
		else
		    _DPSConvertBinArray( ctxt->outStream,
			    BINARRAY_EXOBJECTS(ctxt->asciiBuffer),
			    BINARRAY_EXTOPOBJS(ctxt->asciiBuffer),
			    0, (DPSBinObjSeqRec *)(ctxt->asciiBuffer),
			    ctxt->c.programEncoding, ctxt->c.nameEncoding,
			    ctxt->debugging );
		NXWrite( ctxt->outStream, "\n", 1 );
		NXFlush( ctxt->outStream );
	    NX_HANDLER
		RERAISE_ERROR( ctxtArg, &NXLocalHandler );
	    NX_ENDHANDLER
	    if( !(length == seqLength || seqLength <= ASCII_BUF_SIZE) )
		FREE( ctxt->asciiBuffer );
	    ctxt->asciiBuffer = NULL;
	}
    }
}


/* WriteTypedObjectArray and WriteStringChars assume that ctxt->binArrayLeft
   will be set and decremented all in WriteBinObjSeq.
 */   

void
_DPSOutputWriteTypedObjectArray( ctxtArg, type, array, length )
DPSContext ctxtArg;
DPSDefinedType type;
char *array;
unsigned int length;
{
    _DPSOutputContext ctxt = STD_TO_OUTPUT(ctxtArg);
    DPSBinObjRec tokenBuffer[TOKEN_BUF_SIZE];
    int done;

    ASSERT( ctxt->binArrayLeft >= 0, "Bad length in WriteTypedObjectArray" );
    if( ctxt->c.chainChild )
	DPSWriteTypedObjectArray( ctxt->c.chainChild, type, array, length );
    while( length ) {
	done = formatArray( tokenBuffer, type, &array, length );
	binObjSeqWriteGuts( ctxtArg, (char *)tokenBuffer,
					sizeof(DPSBinObjRec) * done, FALSE );
	length -= done;
    }
}


/* common routine for contexts' WriteTypedObjectArray procs.  This routine
   formats up some objects of a given type in the buffer, and returns
   how many are actually prepared for output.
 */
static int
formatArray( tokenBuffer, type, arrayPtr, length )
DPSBinObjRec *tokenBuffer;	/* buffer to format tokens in */
DPSDefinedType type;		/* type of objects */
char **arrayPtr;		/* ptr to data elements (updated on return) */
unsigned int length;		/* # elements to format */
{
    register DPSBinObjRec *tok;
    int done;
    register char *array = *arrayPtr;
    int numToBeDone;

    numToBeDone = MIN( length, TOKEN_BUF_SIZE );
    bzero( (char *)tokenBuffer, numToBeDone * sizeof(DPSBinObjRec) );
    for( done = 0, tok = tokenBuffer; done < numToBeDone; done++, tok++ )
	switch( type ) {
	    case dps_tBoolean:
		tok->attributedType = DPS_BOOL | DPS_LITERAL;
		tok->val.booleanVal = *((int *)array)++ != 0;
		break;
	    case dps_tFloat:
		tok->attributedType = DPS_REAL | DPS_LITERAL;
		tok->val.realVal = *((float *)array)++;
		break;
	    case dps_tDouble:
		tok->attributedType = DPS_REAL | DPS_LITERAL;
		tok->val.realVal = *((double *)array)++;
		break;
	    case dps_tShort:
	    case dps_tUShort:
		tok->attributedType = DPS_INT | DPS_LITERAL;
		tok->val.integerVal = *((short *)array)++;
		break;
	    case dps_tInt:
	    case dps_tUInt:
		tok->attributedType = DPS_INT | DPS_LITERAL;
		tok->val.integerVal = *((int *)array)++;
		break;
	    case dps_tLong:
	    case dps_tULong:
		tok->attributedType = DPS_INT | DPS_LITERAL;
	        tok->val.integerVal = *((long *)array)++;
		break;
#ifdef DEBUG
	    case dps_tChar:
	    case dps_tUChar:
		ASSERT( FALSE, "Char type in WriteTypedObjectArray" );
		break;
	    default:
		ASSERT( FALSE, "Unknown type in WriteTypedObjectArray" );
		break;
#endif
	}
    *arrayPtr = array;
    return done;
}


void
_DPSOutputWriteStringChars( ctxtArg, buf, count )
DPSContext ctxtArg;
char *buf;
unsigned int count;
{
    _DPSOutputContext ctxt = STD_TO_OUTPUT(ctxtArg);
    int i;
    char space = ' ';

    if( ctxt->c.chainChild )
	DPSWriteStringChars( ctxt->c.chainChild, buf, count );
    binObjSeqWriteGuts( ctxtArg, buf, count, FALSE );
  /* round up sequence alignment in the output buffer */
    if( !ctxt->binArrayLeft && ctxt->lengthMod4 &&
		(ctxt->c.programEncoding == dps_binObjSeq &&
		 ctxt->c.nameEncoding == dps_indexed))
	NX_DURING
	    for( i = 4 - ctxt->lengthMod4; i--; )
		NXWrite( ctxt->outStream, &space, 1 );
	NX_HANDLER
	    RERAISE_ERROR( ctxtArg, &NXLocalHandler );
	NX_ENDHANDLER
}


void
_DPSOutputWriteData( ctxtArg, buf, count )
DPSContext ctxtArg;
char *buf;
unsigned int count;
{
    if( ctxtArg->chainChild )
	DPSWriteData( ctxtArg->chainChild, buf, count );
    NX_DURING
	NXWrite( STD_TO_OUTPUT(ctxtArg)->outStream, buf, count );
    NX_HANDLER
	RERAISE_ERROR( ctxtArg, &NXLocalHandler );
    NX_ENDHANDLER
}


void
_DPSOutputWritePostScript( ctxtArg, buf, count )
DPSContext ctxtArg;
unsigned char *buf;
unsigned int count;
{
    _DPSOutputContext ctxt = STD_TO_OUTPUT(ctxtArg);

    if( ctxt->binArrayLeft || *buf == DPS_DEF_TOKENTYPE )
	DPSBinObjSeqWrite( ctxtArg, buf, count );
    else
	DPSWriteData( ctxtArg, buf, count );
}


void
_DPSOutputFlushContext( ctxtArg )
DPSContext ctxtArg;
{
    if( ctxtArg->chainChild )
	DPSFlushContext( ctxtArg->chainChild );
    NX_DURING
	_NXSetPmonSendMode(ctxtArg, pmon_explicit);
	NXFlush( STD_TO_OUTPUT(ctxtArg)->outStream );
    NX_HANDLER
	RERAISE_ERROR( ctxtArg, &NXLocalHandler );
    NX_ENDHANDLER
}


void
_DPSOutputPrintf( ctxtArg, fmt, argList )
DPSContext ctxtArg;
char *fmt;
va_list argList;
{
    if( ctxtArg->chainChild )
	(*ctxtArg->chainChild->procs->Printf)(
				ctxtArg->chainChild, fmt, argList );
    NX_DURING
	NXVPrintf( STD_TO_OUTPUT(ctxtArg)->outStream, fmt, argList );
    NX_HANDLER
	RERAISE_ERROR( ctxtArg, &NXLocalHandler );
    NX_ENDHANDLER
}


void
_DPSOutputUpdateNameMap( ctxtArg )
DPSContext ctxtArg;
{
    register int i;
    register const char *name;
    DPSContext kids;

    if( ctxtArg->nameEncoding == dps_indexed )
	for( i = ctxtArg->space->lastNameIndex+1; i <= _DPSLastNameIndex; i++ )
	    if( name = DPSNameFromTypeAndIndex(0, i) )
		DPSPrintf( ctxtArg, "%d /%s defineusername\n", i, name );
    for( kids = ctxtArg; kids->chainChild; kids = kids->chainChild )
	kids->space->lastNameIndex = _DPSLastNameIndex;
    ctxtArg->space->lastNameIndex = _DPSLastNameIndex;
}


void
_DPSNotImplemented()
{
    ASSERT( FALSE, "Routine not implemented for contexts" );
}


void
_DPSNop()
{
}


/* prints out a binary array in ascii to the given st.  It only puts c/r's
   after internal arrays, but not at the end of writing the given array.
 */
void
_DPSConvertBinArray(NXStream *st, const DPSBinObjRec *curr, int numObjs,
			int level, const DPSBinObjSeqRec *start,
			DPSProgramEncoding progEnc, DPSNameEncoding nameEnc,
			char debugging)
{
    register int count;
    register const char *s;
    DPSBinObjRec *firstObj;

    firstObj = BINARRAY_TOPOBJS(start) ? BINARRAY_OBJECTS(start) :
					BINARRAY_EXOBJECTS(start);
    indent( level, st );
    for( ; numObjs--; curr++ ) {
	switch( curr->attributedType & 0x0f ) {
	    case DPS_INT:
		NXPrintf( st, "%d", curr->val.integerVal );
		break;
	    case DPS_REAL:
		NXPrintf( st, "%g", curr->val.realVal );
		break;
	    case DPS_NAME:
	      /* NOTE: length field is SIGNED for names */
		if( (short)(curr->length) > 0 ) {	/* textual name */
		    if( !(curr->attributedType & DPS_EXEC) )
		        NXWrite( st, "/", 1 );
		    s = (char *)firstObj + curr->val.nameVal;
		    for( count = curr->length; count--; s++ )
		        NXWrite( st, s, 1 );
		} else if( s = DPSNameFromTypeAndIndex((short)curr->length,
							curr->val.nameVal)) {
		  /* its a hard-wired name or user name */
		    if( !(curr->attributedType & DPS_EXEC) )
		        NXWrite( st, "/", 1 );
		    NXPrintf( st, "%s", s );
		} else
		    ASSERT( !curr->length, "Unknown system index" );
		break;
	    case DPS_BOOL:
		NXPrintf( st, "%s", curr->val.booleanVal ? "true" : "false" );
		break;
	    case DPS_STRING:
		if( debugging && curr->length > 150 )
		    NXPrintf( st, "( <%d byte string> )", curr->length );
		else {
		    s = (char *)firstObj + curr->val.stringVal;
		    if( (*s >= ' ' && *s <= '~') || curr->length == 0 ) {
			NXWrite( st, "(", 1 );
			for( count = curr->length; count--; s++ )
			    if( *s < ' ' || *s > '~' )
				writeOctal( (unsigned char)*s, st );
			    else if( *s == '(' || *s == ')' || *s == '\\' )
				NXPrintf( st, "\\%c", *s );
			    else
				NXWrite( st, s, 1 );
			NXWrite( st, ")", 1 );
		    } else {
			int hexBuf;	/* used to print just a byte of hex */
    
			NXWrite( st, "<", 1 );
			for( count = curr->length; count--; s++) {
			    hexBuf = (unsigned char)*s;
			    NXPrintf( st, "%02x", hexBuf );
			}
			NXWrite( st, ">", 1 );
		    }
		}
		break;
	    case DPS_ARRAY:
		if(curr->attributedType & DPS_EXEC) {
		    NXWrite( st, "{\n", 2 );
		    _DPSConvertBinArray( st, (DPSBinObjRec *)((char *)firstObj
					+ curr->val.arrayVal), curr->length,
					level+1, start, progEnc, nameEnc,
					debugging );
		    NXWrite( st, "\n", 1 );
		    indent( level, st );
		    NXWrite( st, "}", 1 );
		} else {
		    NXWrite( st, "[", 1 );
		    _DPSConvertBinArray( st, (DPSBinObjRec *)((char *)firstObj
					+ curr->val.arrayVal), curr->length,
					0, start, progEnc, nameEnc,
					debugging );
		    NXWrite( st, "]", 1 );
		}
		break;
	}
	if( numObjs )
	    NXWrite( st, " ", 1 );
    }
}


/* indents so many spaces per n */
static void
indent( n, st )
register int n;
register NXStream *st;
{
    while( n-- )
	NXWrite( st, "    ", 4 );
}


/* writes out a number as escape and three digits of octal */
static void
writeOctal( c, st )
register unsigned char c;
register NXStream *st;
{
    char buf[4];

    buf[0] = '\\';
    buf[1] = '0' + (c >> 6);
    c &= ~(0xc0);		/* mask against %11000000 */
    buf[2] = '0' + (c >> 3);
    c &= ~(0x38);		/* mask against %00111000 */
    buf[3] = '0' + c;
    NXWrite( st, buf, 4 );
}


/* associates a tracing context with an output context.  Since this will
   usually be called for debugging purposes, this routine catches all
   exceptions and returns -1 on failure, 0 on success.  This routine sets the
   lastNameIndex of the tracing context to that of the traced context.  This
   makes the output true to what the parent is producing, but leaves that
   output unable to stand on its own, in general.
 */
int
DPSTraceContext( ctxtArg, flag )
DPSContext ctxtArg;
int flag;		/* turn tracing on? (or off) */
{
    if( ctxtArg != DPS_ALLCONTEXTS ) {
	NXStream *st;
	DPSContext new;
	_DPSOutputContext ctxt = STD_TO_OUTPUT( ctxtArg );
    
	NX_DURING
	    if( flag && !ctxt->traceCtxt) {
		st = NXOpenFile( fileno(stderr), NX_WRITEONLY );
		new = _DPSLeanCreateStreamContext( st, 1, dps_ascii,
						    dps_strings, NULL );
		new->space->lastNameIndex = ctxtArg->space->lastNameIndex;
		DPSChainContext( ctxtArg, new );
		DPSSetContext( ctxtArg );
		ctxt->traceCtxt = new;
		NX_VALRETURN(0);
	    } else if( !flag && ctxt->traceCtxt ) {
		DPSUnchainContext( ctxt->traceCtxt );
		st = STD_TO_OUTPUT(ctxt->traceCtxt)->outStream;
		DPSDestroyContext( ctxt->traceCtxt );
		NXClose( st );
		ctxt->traceCtxt = NULL;
		NX_VALRETURN(0);
	    }
	NX_HANDLER
	NX_ENDHANDLER
	return -1;
    } else {
	_DPSOutputContext ctxt;

	_DPSTracingOn = flag;	/* context creation routines check this */
	for( ctxt = (_DPSOutputContext)(_DPSOutputContexts.head); ctxt;
							ctxt = ctxt->nextOC )
	    if( !ctxt->c.chainParent )
		DPSTraceContext( OUTPUT_TO_STD(ctxt), flag );
    }
    return 0;
}


/* makes one of our simple spaces */
DPSSpace
_DPSCreateSpace( DPSContext ctxt, NXZone *zone )
{
    _DPSPrivSpace *newSpace;

    newSpace = NXZoneMalloc(zone, sizeof(_DPSPrivSpace));
    newSpace->s.lastNameIndex = 0;
    newSpace->s.procs = &_DPSSpaceProcs;
    newSpace->ctxt = ctxt;
    return (DPSSpace)newSpace;
}


static void
_DPSDestroySpace( s )
DPSSpace s;
{
    DPSDestroyContext( ((_DPSPrivSpace *)s)->ctxt );
}


/*  turns on and off tracing of events for a context  */
void DPSTraceEvents(DPSContext ctxtArg, int flag)
{
    if( ctxtArg != DPS_ALLCONTEXTS ) {
	if (ctxtArg->type == dps_machServer)
	    STD_TO_SERVER(ctxtArg)->serverCtxtFlags.traceEvents = flag;
    } else {
	_DPSOutputContext ctxt;

	_DPSEventTracingOn = flag;  /* context creation routines check this */
	for (ctxt = (_DPSOutputContext)(_DPSOutputContexts.head); ctxt;
							ctxt = ctxt->nextOC)
	    if (ctxt->c.type == dps_machServer)
		OUTPUT_TO_SERVER(ctxt)->serverCtxtFlags.traceEvents = flag;
    }
}

