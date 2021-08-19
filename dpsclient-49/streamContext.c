#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
    streamContext.c

    This file has the implementation of a context that writes to a NXStream.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#include "dpsclient.h"
#include "defs.h"
#include <string.h>


typedef void voidFunc();

static voidFunc		_DPSStreamDestroy;

extern voidFunc		_DPSOutputBinObjSeqWrite,
			_DPSOutputWriteTypedObjectArray,
			_DPSOutputWriteStringChars,
			_DPSOutputWriteData,
			_DPSOutputWritePostScript,
			_DPSOutputFlushContext,
			_DPSOutputUpdateNameMap,
			_DPSOutputPrintf,
			_DPSNop,		/* maybe do flush??? */
			_DPSNotImplemented;

static const DPSProcsRec _DPSStreamProcs = {
    _DPSOutputBinObjSeqWrite,
    _DPSOutputWriteTypedObjectArray,
    _DPSOutputWriteStringChars,
    _DPSOutputWriteData,
    _DPSOutputWritePostScript,
    _DPSOutputFlushContext,
    _DPSNop,				/* StreamResetContext */
    _DPSOutputUpdateNameMap,
    _DPSNop,				/* StreamAwaitReturnValues */
    _DPSNop,				/* StreamInterrupt */
    _DPSStreamDestroy,
    _DPSNop,				/* StreamWaitContext */
    _DPSOutputPrintf
};


DPSContext
_DPSLeanCreateStreamContext( st, debugging, progEnc, nameEnc, errorProc )
NXStream *st;
int debugging;
DPSProgramEncoding progEnc;
DPSNameEncoding nameEnc;
DPSErrorProc errorProc;
{
    register _DPSStreamContext newCtxt;
    register DPSContext stdCtxt;

    MALLOC( newCtxt, _DPSStreamContextRec, 1 );
    _DPSInitMainPortSet();
    stdCtxt = STREAM_TO_STD(newCtxt);
    bzero( (char *)newCtxt, sizeof(_DPSStreamContextRec) );
    newCtxt->c.space = _DPSCreateSpace( stdCtxt, NXDefaultMallocZone() );
    newCtxt->c.programEncoding = progEnc;
    newCtxt->c.nameEncoding = nameEnc;
    newCtxt->c.errorProc = errorProc ? errorProc : DPSDefaultErrorProc;
    newCtxt->c.procs = &_DPSStreamProcs;
    newCtxt->c.type = dps_stream;
    newCtxt->outStream = st;
    newCtxt->debugging = debugging;
    MALLOC( newCtxt->asciiBufferSpace, char, ASCII_BUF_SIZE );
    DPSSetContext( stdCtxt );
    _DPSAddListItem( &_DPSOutputContexts, (_DPSListItem *)STD_TO_OUTPUT( stdCtxt ));
    return stdCtxt;
}


DPSContext
DPSCreateStreamContext( st, debugging, progEnc, nameEnc, errorProc )
NXStream *st;
int debugging;
DPSProgramEncoding progEnc;
DPSNameEncoding nameEnc;
DPSErrorProc errorProc;
{
    register DPSContext stdCtxt;

    stdCtxt = _DPSLeanCreateStreamContext(st, debugging, progEnc,
    						nameEnc, errorProc);
    if( _DPSTracingOn )
	DPSTraceContext( stdCtxt, TRUE );
    return stdCtxt;
}


static void
_DPSStreamDestroy( ctxtArg )
DPSContext ctxtArg;
{
    _DPSOutputContext ctxt = STD_TO_OUTPUT(ctxtArg);

    if( ctxtArg->chainParent )
	DPSUnchainContext( ctxtArg );
    if( DPSGetCurrentContext() == ctxtArg )
	DPSSetContext( NULL );
    NXFlush(ctxt->outStream);
    (void)_DPSRemoveListItem( &_DPSOutputContexts, ctxt, FALSE);
    FREE( ctxt->asciiBufferSpace );
    FREE( ctxtArg->space );
    FREE( ctxt );
}
