#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
    generic.c

    This file has routines to implement the generic DPS client library
    interface.  There should be no NeXT extensions in this file.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#include "dpsclient.h"
#include "defs.h"
#include <stdarg.h>

static DPSContext GlobalContext = NULL;
static DPSTextProc CurrentTextBackstop = _DPSDefaultTextProc;
static DPSErrorProc CurrentErrorBackstop = DPSDefaultErrorProc;


/* internal routine to handle errors initiated by dpsclient */
void _DPSProcessError( DPSContext ctxt, DPSErrorCode code,
					const void *data1, const void *data2 )
{
    void (*func)();

    func = ctxt ? ctxt->errorProc : CurrentErrorBackstop;
    (*func)( ctxt, code, data1, data2 );
}


/* Internal routine to handle errors re-raised by dpsclient.  Certain
   stream errors are convered into dpsclient errors, the rest are passed
   on.
 */
void _DPSReprocessError( DPSContext ctxt, NXHandler *ex )
{
    if ((ex->code == NX_illegalWrite || ex->code == NX_illegalStream) &&
	    ctxt &&
	    (ctxt->type == dps_machServer || ctxt->type == dps_stream) &&
	    STD_TO_OUTPUT(ctxt)->outStream == (NXStream *)(ex->data1))
	_DPSProcessError(ctxt, dps_err_write, ex->data2, NULL);
    else
	NX_RAISE(ex->code, ex->data1, ex->data2);
}


void DPSDefaultErrorProc( ctxt, errorCode, arg1, arg2 )
DPSContext ctxt;
DPSErrorCode errorCode;
long unsigned int arg1, arg2;
{
    NX_RAISE( errorCode, ctxt, (void *)arg1 );
}


void DPSPrintf( DPSContext ctxt, const char *fmt, ... )
{
    register va_list argsPtr;

    va_start( argsPtr, fmt );
    (*ctxt->procs->Printf)( ctxt, fmt, argsPtr );
    va_end( argsPtr );
}

/* Links parent to child and vice-versa */
#define LINK( p, c )	{	(c)->chainParent = (p);	\
				(p)->chainChild = (c);	}

int
DPSChainContext( parent, child )
DPSContext parent, child;
{
    DPSContext oldChildren;		/* any existing children of parent */
    DPSContext firstNode, lastNode;

    if( child->chainParent )
	return -1;
    oldChildren = parent->chainChild;

    firstNode = parent;			/* find start of parent chain */
    while( firstNode->chainParent )
        firstNode = firstNode->chainParent;

  /* get everyone's name maps in sync */
    if( parent->space->lastNameIndex != child->space->lastNameIndex ) {
	(*firstNode->procs->UpdateNameMap)( firstNode );
	(*child->procs->UpdateNameMap)( child );
    }

    LINK( parent, child );

    if( oldChildren ) {
	lastNode = child;		/* find end of new chain */
	while( lastNode->chainChild )
	    lastNode = lastNode->chainChild;
	LINK( lastNode, oldChildren );
    }
    return 0;
}


void
DPSUnchainContext( ctxt )
DPSContext ctxt;
{
    if( !ctxt->chainParent )
	RAISE_ERROR( ctxt, dps_err_invalidContext, ctxt, 0 );
    ctxt->chainParent->chainChild = ctxt->chainChild;
    if( ctxt->chainChild )
	ctxt->chainChild->chainParent = ctxt->chainParent;
    ctxt->chainParent = NULL;
    ctxt->chainChild = NULL;
}


void
DPSSetContext( ctxt )
DPSContext ctxt;
{
    GlobalContext = ctxt;
}

DPSContext
DPSGetCurrentContext()
{
    return GlobalContext;
}


void
DPSSetTextBackstop( textProc )
DPSTextProc textProc;
{
    CurrentTextBackstop = textProc;
}

DPSTextProc
DPSGetCurrentTextBackstop()
{
    return CurrentTextBackstop;
}

void
DPSSetErrorBackstop( errorProc )
DPSErrorProc errorProc;
{
    CurrentErrorBackstop = errorProc;
}

DPSErrorProc
DPSGetCurrentErrorBackstop()
{
    return CurrentErrorBackstop;
}

