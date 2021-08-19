#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
    machContext.c

    This file has routines to fill in the procs record defined in
    dpsfriends.h for a context talking to a window server via Mach IPC.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#include "dpsclient.h"
#include "defs.h"
#include "wraps.h"
#include <mach.h>
#include <string.h>
#include <servers/netname.h>
#include <servers/bootstrap.h>

extern port_t name_server_port;		/* ??? should be in header file */

#define PS_RENDEZVOUS_MSG_ID 1000

typedef void voidFunc();

static voidFunc		_DPSMachAwaitReturnValues,
			_DPSMachDestroyContext,
			_DPSMachWaitContext,
			_DPSMachFillBuf,
			_DPSMachResetContext;

extern voidFunc		_DPSOutputBinObjSeqWrite,
			_DPSOutputWriteTypedObjectArray,
			_DPSOutputWriteStringChars,
			_DPSOutputWriteData,
			_DPSOutputWritePostScript,
			_DPSOutputFlushContext,
			_DPSOutputUpdateNameMap,
			_DPSOutputPrintf,
			_DPSNotImplemented;

static const DPSProcsRec _DPSMachProcs = {
    _DPSOutputBinObjSeqWrite,
    _DPSOutputWriteTypedObjectArray,
    _DPSOutputWriteStringChars,
    _DPSOutputWriteData,
    _DPSOutputWritePostScript,
    _DPSOutputFlushContext,
    _DPSMachResetContext,
    _DPSOutputUpdateNameMap,
    _DPSMachAwaitReturnValues,
    _DPSNotImplemented,			/* MachInterrupt */
    _DPSMachDestroyContext,
    _DPSMachWaitContext,
    _DPSOutputPrintf
};


static const char DefaultServerName[] = "NextStep(tm) Window Server";

static port_t getPSPort(const char *host, const char *name, port_t ourPort, int timeout);
static port_t rendezVous(port_t ourPort, port_t theirPort, int timeout);
static void freeMsgData(_DPSMachContext ctxt);
static DPSContext doCreateContext(const char *hostName, const char *serverName, DPSTextProc textProc, DPSErrorProc errorProc, int timeout, NXZone *zone, int doSecurely);

#ifdef DEBUG
static int DPSBacklog = 5;
#endif

DPSContext DPSCreateContext( const char *hostName, const char *serverName, DPSTextProc textProc, DPSErrorProc errorProc )
{
    return DPSCreateContextWithTimeoutFromZone(hostName, serverName, textProc, errorProc, 60000, NXDefaultMallocZone());
}

DPSContext DPSCreateContextWithTimeoutFromZone( const char *hostName, const char *serverName, DPSTextProc textProc, DPSErrorProc errorProc, int timeout, NXZone *zone )
{
    return doCreateContext(hostName, serverName, textProc, errorProc, timeout, zone, _DPSDoNSC);
}

DPSContext _DPSCreateContext( const char *hostName, const char *serverName, DPSTextProc textProc, DPSErrorProc errorProc, int timeout, NXZone *zone )
{
    return doCreateContext(hostName, serverName, textProc, errorProc, timeout, zone, FALSE);
}

static DPSContext doCreateContext( const char *hostName, const char *serverName, DPSTextProc textProc, DPSErrorProc errorProc, int timeout, NXZone *zone, int doSecurely )
{
    _DPSMachContext newServer = NULL;

    _DPSInitMainPortSet();
    newServer = _DPSNewListItem( &_DPSMachContexts, sizeof(_DPSMachContextRec), zone );
  /* zero new struct except for the first linked list ptr */
    bzero( (char *)(&newServer->inPort),
		sizeof(_DPSMachContextRec) -
		((char *)(&newServer->inPort) - (char *)(newServer)));
    if( port_allocate( task_self(), &newServer->pingPort ) != KERN_SUCCESS )
	RAISE_ERROR( 0, dps_err_outOfMemory, (void *)sizeof(port_t), 0 );
    if( port_set_allocate( task_self(), &newServer->localSet ) != KERN_SUCCESS)
	RAISE_ERROR( 0, dps_err_outOfMemory,
				(void *)sizeof(port_set_name_t), 0 );
    if( port_allocate( task_self(), &(newServer->inPort) ) != KERN_SUCCESS )
	RAISE_ERROR( 0, dps_err_outOfMemory, (void *)sizeof(port_t), 0 );
    newServer->flags.portsLocations = IN_NO_SET;
    NX_DURING
#ifdef DEBUG
	if( DPSBacklog != -1 )
	    port_set_backlog( task_self(), newServer->inPort, DPSBacklog );
#endif
	newServer->outPort = getPSPort( hostName, serverName,
						newServer->inPort, timeout );
	if( !newServer->outPort )
	    RAISE_ERROR( 0, dps_err_cantConnect,
				hostName ? hostName : "local host", 0 );
	newServer->outStream = _DPSOpenStream(newServer, zone);
	if( !newServer->outStream )
	    RAISE_ERROR( 0, dps_err_cantConnect,
				hostName ? hostName : "local host", 0 );

	DPSSetContext( MACH_TO_STD(newServer) );
	ASSERT( sizeof(_DPSMessage) <= MSG_SIZE_MAX, "Default msg too big");
	if (sizeof(_DPSMessage) > _DPSMsgSize)
	    _DPSMsgSize = sizeof(_DPSMessage);
	newServer->c.space = _DPSCreateSpace( MACH_TO_STD(newServer), zone );
	newServer->c.programEncoding = dps_binObjSeq;
	newServer->c.nameEncoding = dps_indexed;
	newServer->c.textProc = textProc ? textProc : _DPSDefaultTextProc;
	newServer->c.errorProc = errorProc ? errorProc : DPSDefaultErrorProc;
	newServer->c.procs = &_DPSMachProcs;
	newServer->c.type = dps_machServer;
	newServer->fillBuf = _DPSMachFillBuf;
	_DPSAddListItem( &_DPSOutputContexts, (_DPSListItem *)MACH_TO_OUTPUT( newServer ));
    NX_HANDLER
      /* cleanup whatever we managed to allocate before the error */
	if( newServer ) {
	    if( newServer->inPort ) {
		if( newServer->outStream )
		    NXClose( newServer->outStream );
		port_deallocate( task_self(), newServer->inPort );
	    }
	    (void)_DPSRemoveListItem( &_DPSMachContexts, newServer, TRUE );
	}
      /* ??? maybe NULL out any ptr to context in error data? */
	RERAISE_ERROR( NULL, &NXLocalHandler );
    NX_ENDHANDLER
    newServer->serverCtxtFlags.doDeadKeys = TRUE;
    if( doSecurely )
        DPSPrintf( MACH_TO_STD(newServer), "\n systemdict /setnextstepcontext known { true systemdict /setnextstepcontext get exec } if \n" );
    if( _DPSTracingOn )
	DPSTraceContext( MACH_TO_STD(newServer), TRUE );
    if( _DPSEventTracingOn )
	DPSTraceEvents( MACH_TO_STD(newServer), TRUE );
    PSsendint( 0 );
    PSsendint( 0 );
    PSdefineuserobject();
    PSsetobjectformat( 1 );	/* high byte first, IEEE floats */
    if (_DPSDo10Compatibility)
	_NXOldOrderwindow();	/* preserves old Below 0 semantics */
    _DPSListenToNewContexts();
    return MACH_TO_STD(newServer);
}


static void
_DPSMachAwaitReturnValues( ctxtArg )
DPSContext ctxtArg;
{
    _DPSMachContext ctxt = STD_TO_MACH(ctxtArg);
    msg_return_t ret;
    int size;
    _DPSMessage *localMsg = alloca(_DPSMsgSize);

    NX_DURING
	NX_DURING
	    _NXSetPmonSendMode(ctxtArg, pmon_get_tokens);
	    NXFlush( ctxt->outStream );
	NX_HANDLER
	    RERAISE_ERROR( ctxtArg, &NXLocalHandler );
	NX_ENDHANDLER
	if( ctxt->c.chainParent ) {		/* if we're part of a chain */
	    if( ctxt->savedMsg ) {		/* gobble everything we can */
		FREE( ctxt->savedMsg );
		ctxt->savedMsg = NULL;
	    }
	    while(1) {
		localMsg->header.msg_local_port = ctxt->inPort;
		localMsg->header.msg_size = _DPSMsgSize;
		ret = msg_receive( (msg_header_t *)localMsg, RCV_TIMEOUT, 0 );
		if( ret == RCV_TIMED_OUT )
		    break;
		else if( ret != RCV_SUCCESS )
		    RAISE_ERROR( ctxtArg, dps_err_read, (void *)ret, 0 );
	    }
	} else {
	    while( ctxt->c.resultTable ) {
		if( !ctxt->savedMsg ) {
		    _DPSAssemblePorts( ctxt );
		    _NXSendPmonEvent(_DPSPmonSrcData, pmon_msg_receive,
					0, pmon_get_tokens, MSG_OPTION_NONE);
		    ret = _DPSLocalSetMsgReceive( ctxt, localMsg, MSG_OPTION_NONE, 0 );
		    _NXSendPmonEvent(_DPSPmonSrcData, pmon_receive_data,
			localMsg->header.msg_size, pmon_get_tokens, ret);
		    _DPSHandleMsg( localMsg, ret, NULL, 0, 0 );
		} else
		    _DPSMachCallTokenProc( 0, ctxt );
	    }
	}
	if( ctxt->c.chainChild )
	    DPSAwaitReturnValues( ctxt->c.chainChild );
    NX_HANDLER
	ctxt->c.resultTable = NULL;
	RERAISE_ERROR( ctxtArg, &NXLocalHandler );
    NX_ENDHANDLER
}


static void
_DPSMachDestroyContext( ctxtArg )
DPSContext ctxtArg;
{
    _DPSMachContext ctxt = STD_TO_MACH(ctxtArg);
    NXHandler except;

    except.code = 0;
    if( ctxt->c.chainParent )
	DPSUnchainContext( ctxtArg );
    if( ctxt->c.chainChild ) {
	_NXSetPmonSendMode(ctxtArg, pmon_destroy);
	DPSFlushContext(ctxt->c.chainChild);
    }
    DPSDiscardEvents( ctxtArg, NX_ALLEVENTS );
    NX_DURING
	NXClose( ctxt->outStream );
    NX_HANDLER
	except = NXLocalHandler;
	/* catch this error so that we are sure to close this context down */
    NX_ENDHANDLER
    port_deallocate( task_self(), ctxt->inPort );
    port_deallocate( task_self(), ctxt->outPort );
    port_deallocate( task_self(), ctxt->pingPort );
    port_set_deallocate( task_self(), ctxt->localSet );
    if (DPSGetCurrentContext() == ctxtArg)
	DPSSetContext( NULL );
    FREE(ctxtArg->space);
    (void)_DPSRemoveListItem(&_DPSOutputContexts, MACH_TO_OUTPUT(ctxt), FALSE);
    if (!_DPSRemoveListItem(&_DPSMachContexts, ctxt, TRUE))
	RAISE_ERROR(ctxtArg, dps_err_invalidContext, ctxtArg, 0);
    if (except.code)
	RERAISE_ERROR(ctxtArg, &except);
}


static void
_DPSMachWaitContext( ctxtArg )
DPSContext ctxtArg;
{
    _DPSMachContext ctxt = STD_TO_MACH(ctxtArg);
    msg_return_t ret;
    _DPSMessage *localMsg = alloca(_DPSMsgSize);

    _DPSFlushStream( ctxt->outStream, TRUE, 0 );
    ctxt->expectedPings++;
    _DPSAssemblePorts( ctxt );
    while(ctxt->expectedPings > 0) {
	_NXSendPmonEvent(_DPSPmonSrcData, pmon_msg_receive,
				0, pmon_ping, MSG_OPTION_NONE);
	ret = _DPSLocalSetMsgReceive( ctxt, localMsg, MSG_OPTION_NONE, 0 );
	_NXSendPmonEvent(_DPSPmonSrcData, pmon_receive_data,
				localMsg->header.msg_size, pmon_ping, ret);
	_DPSHandleMsg( localMsg, ret, NULL, 0, 0 );
    }
}


/* Waits for more data from PS for this context.  If the data comes out of line, we can use _DPSMachPrepareToParseMsg.  If it comes in line, we need to malloc space for it and copy, since the original message buffer is on the stack.  In this case we call it a saved message data so it gets freed. */
static void
_DPSMachFillBuf( ctxtArg )
DPSContext ctxtArg;
{
    _DPSMachContext ctxt = STD_TO_MACH(ctxtArg);
    msg_return_t ret;
    _DPSMessage *localMsg = alloca(_DPSMsgSize);
    int dataSize;

    _DPSAssemblePorts( ctxt );
    while(1) {
	_NXSendPmonEvent(_DPSPmonSrcData, pmon_msg_receive,
				0, pmon_explicit, MSG_OPTION_NONE);
	ret = _DPSLocalSetMsgReceive( ctxt, localMsg, MSG_OPTION_NONE, 0 );
	_NXSendPmonEvent(_DPSPmonSrcData, pmon_receive_data,
				localMsg->header.msg_size, pmon_explicit, ret);
	if( ret == RCV_SUCCESS &&
			localMsg->header.msg_local_port == ctxt->inPort ) {
	    if( localMsg->type.msg_type_header.msg_type_inline ) {
		dataSize = localMsg->type.msg_type_long_number;
		MALLOC( ctxt->savedMsg, char, dataSize );
		bcopy( localMsg->data, ctxt->savedMsg, dataSize );
		ctxt->bufCurr = ctxt->savedMsg;
		ctxt->bufEnd = ctxt->savedMsg + dataSize;
	    } else
		_DPSMachPrepareToParseMsg( ctxt, localMsg );
	    break;
	} else		/* its a notification or error */
	    _DPSHandleMsg( localMsg, ret, NULL, 0, 0 );
    }
}


static void
_DPSMachResetContext( ctxtArg )
DPSContext ctxtArg;
{
    _DPSMachContext ctxt = STD_TO_MACH(ctxtArg);

    _DPSResetStream( ctxt->outStream );
}


/* Prepares a context for _DPSTokenProc to parse a given Mach msg.  This
   requires that we fix up bufCurr and bufEnd, and handle any out of line
   data.
 */
void
_DPSMachPrepareToParseMsg( ctxt, msg )
_DPSMachContext ctxt;
_DPSMessage *msg;
{
    struct ooLineMsgType {
	msg_header_t header;
	msg_type_long_t type;
	char *data;
    } *ooMsg;

    freeMsgData(ctxt);
    if( msg->type.msg_type_header.msg_type_inline ) {
	ctxt->bufCurr = msg->data;
	ctxt->bufEnd = ((char *)msg) + msg->header.msg_size;
    } else {
	ooMsg = (struct ooLineMsgType *)msg;
	ctxt->ooLineData = ooMsg->data;
	ctxt->ooLineSize = ooMsg->type.msg_type_long_number;
	ctxt->bufCurr = ooMsg->data;
	ctxt->bufEnd = ctxt->bufCurr + ooMsg->type.msg_type_long_number;
    }
}


/* Calls the _DPSTokenProc parser within error handling code to catch PS
   errors and other bombouts, asving away any unparsed data.
 */
void
_DPSMachCallTokenProc( mask, ctxt )
int mask;
_DPSMachContext ctxt;
{
    int size;

    NX_DURING
	_DPSTokenProc( mask, MACH_TO_SERVER(ctxt) );
    NX_HANDLER
	if( NXLocalHandler.code == dps_err_resultTypeCheck ||
		NXLocalHandler.code == dps_err_resultTagCheck ||
		NXLocalHandler.code == dps_err_ps )
	    if( !ctxt->savedMsg ) {
		size = ctxt->bufEnd - ctxt->bufCurr;
		if( size )
		    if( ctxt->ooLineData )
			ctxt->savedMsg = ctxt->ooLineData;
		    else {
			MALLOC( ctxt->savedMsg, char, size );
			bcopy( ctxt->bufCurr, ctxt->savedMsg, size );
			ctxt->bufCurr = ctxt->savedMsg;
			ctxt->bufEnd = ctxt->savedMsg + size;
		    }
	    } else {
		if( ctxt->bufEnd == ctxt->bufCurr ) {	/* save buf used up */
		    FREE( ctxt->savedMsg );
		    ctxt->savedMsg = NULL;
		}
	    }
      /* else we have a serious connection error */
	RERAISE_ERROR( MACH_TO_STD(ctxt), &NXLocalHandler );
    NX_ENDHANDLER
    
    /* we can free out-of-line or saved data since we know a successful parse will consume the entire buffer. */
    freeMsgData(ctxt);
}


/* we can free out-of-line or saved data since we know a successful parse will consume the entire buffer.  Since saved data could also be out-of-line, first try to dealloc the out-of-line stuff.   */
static void freeMsgData(_DPSMachContext ctxt)
{
    if (ctxt->ooLineData) {
	vm_deallocate(task_self(), (vm_address_t)(ctxt->ooLineData), ctxt->ooLineSize);
	ctxt->ooLineData = NULL;
	ctxt->savedMsg = NULL;
    } else if (ctxt->savedMsg) {
	FREE(ctxt->savedMsg);
	ctxt->savedMsg = NULL;
    }
}


/* hooks up to the windowserver */
static port_t
getPSPort( const char *host, const char *name, port_t ourPort, int timeout )
{
    port_t bootstrap_port;
    port_t rendezVousPort = PORT_NULL;
    port_t wsPort = PORT_NULL;
    kern_return_t kret;

    if (!host && !name) {
	kret = task_get_bootstrap_port(task_self(), &bootstrap_port);
	if (kret == KERN_SUCCESS) {
	    kret = bootstrap_look_up(bootstrap_port, "WindowServer", &rendezVousPort);
	    if (kret == KERN_SUCCESS)
		wsPort = rendezVous(ourPort, rendezVousPort, timeout);
	}
    }
    if (!wsPort) {
	if (netname_look_up(name_server_port, (char *)(host ? host : ""),
				    (char *)(name ? name : DefaultServerName),
				    &rendezVousPort) != NETNAME_SUCCESS) {
	    return PORT_NULL;
	}
	wsPort = rendezVous (ourPort, rendezVousPort, timeout);
      /* maybe create a subset bootstrap port and pass this to our children? */
    }
    return wsPort;
}


/* trys to have rendez-Vous interaction with server, given his registered port */
static port_t
rendezVous( port_t ourPort, port_t theirPort, int timeout )
{
    typedef struct {	/* message used to rendez-vous with PS */
	    msg_header_t header;
	    msg_type_t type;
	    port_t port;
    } HookupMsg;
    const HookupMsg msgTemplate = {
	{0, FALSE, sizeof(HookupMsg), MSG_TYPE_NORMAL, PORT_NULL, PORT_NULL,
							PS_RENDEZVOUS_MSG_ID},
	{MSG_TYPE_PORT, 8 * sizeof(port_t), 1, TRUE, FALSE, FALSE},
	PORT_NULL
    };
    HookupMsg msg;

    msg = msgTemplate;
    msg.header.msg_local_port = ourPort;
    msg.header.msg_remote_port = theirPort;
    msg.port = ourPort;
    if( msg_rpc( (msg_header_t*)&msg, SEND_TIMEOUT|RCV_TIMEOUT, sizeof(HookupMsg),
					timeout, timeout ) != RPC_SUCCESS )
	return PORT_NULL;
    else
	return msg.port;
}


/* gets the right ports in localSet */
void
_DPSAssemblePorts( _DPSMachContext ctxt )
{
    _DPSAddNotifyToSet( ctxt->localSet );
    if( ctxt->flags.portsLocations != IN_LOCAL_SET ) {
	port_set_add( task_self(), ctxt->localSet, ctxt->inPort );
	port_set_add( task_self(), ctxt->localSet, ctxt->pingPort );
	ctxt->flags.portsLocations = IN_LOCAL_SET;
    }
}


msg_return_t
_DPSLocalSetMsgReceive( _DPSMachContext ctxt, _DPSMessage *msg,
			msg_option_t option, msg_timeout_t time )
{
    msg->header.msg_local_port = ctxt->localSet;
    msg->header.msg_size = _DPSMsgSize;
    return msg_receive((msg_header_t *)msg, option, time );
}

void DPSSetDeadKeysEnabled(DPSContext ctxtArg, int flag)
{
    if (ctxtArg->type == dps_machServer)
	STD_TO_SERVER(ctxtArg)->serverCtxtFlags.doDeadKeys = flag;
}
