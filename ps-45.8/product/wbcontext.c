/*****************************************************************************

    wbcontext.c
    Procedures for Windowbitmap contexts.
	
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Dave 16Feb89
	
    Modified:
    
    10Nov89 Terry CustomOps conversion
    04Dec89 Ted   Integratathon!
    28Mar90 Terry Removed references to layer from WBDeviceInfo
    07Jun90 Dave  Upgraded to new NS call format.
	
******************************************************************************/

#import <sys/message.h>
#import <servers/netname.h>
#import <sys/notify.h>
#import PACKAGE_SPECS
#import CUSTOMOPS
#import EXCEPT
#import "ipcscheduler.h"
#import "wbcontext.h"

WBContext *currentWBContext = NULL;
WBContext *WBCList = NULL;

static void WBNotifyNewShmem(WBContext *);

static msg_return_t WBMsg_send(msg_header_t *header, msg_option_t option,
    msg_timeout_t timeout)
{
    msg_return_t mrval;
    
    if ((mrval = msg_send(header, SEND_NOTIFY|SEND_TIMEOUT|option, timeout))
	== SEND_WILL_NOTIFY)
	ContextYield(yield_stdout, header->msg_remote_port);

    return mrval;
}

static void WBError(int etype)
{
    msg_header_t ereply;
    
    ereply.msg_simple = true;
    ereply.msg_size = sizeof(ereply);
    ereply.msg_type = MSG_TYPE_NORMAL;
    ereply.msg_local_port = currentWBContext->scheduler->inputPort;
    ereply.msg_remote_port = currentWBContext->scheduler->remotePort;
    ereply.msg_id = etype;
    WBMsg_send(&ereply, MSG_OPTION_NONE, 0);
}

static void WBCoProc()
{
    YieldReason reason;
    NXWBMessage *msg;
    int err;

    /* just loop around forever, ContextYielding all the time */

    /* for ContextYield(yield_other, &msg) -- initialize our input port */
    msg = (NXWBMessage *)currentWBContext->inPortCache;

    /* initialises the context's ports */
    reason = yield_other;
    while (1) {
	/*  If we need to update the shmem as a result of going through this
	 *  loop, then do so now.  If the shmem needs to be updated after
	 *  the next ContextYield, then take care of it then.  It is possible
	 *  for a wb context to run without any messages waiting for it;
	 *  in this case it must need to update shmem.  It is not possible
	 *  for a wb context to need running because of a pending message
	 *  and dirty shmem.  Once the dirty shmem is handled, we
	 *  ContextYield(yield_other) to put our message pointers back in
	 *  order, since we can reach this stage without all the proper 
	 *  ContextYielding that would otherwise be done.  For example, 
	 *  you can get the MsgPtrP in the context pointing to random 
	 *  memory (not filled in by reading a message) 
	 *  before you ContextYield(yield_stdin); this causes port_restrict 
	 *  on the real port, and an unrestrict on garbage.
	 */
	if (currentWBContext->dirtyShmem) {
	    WBNotifyNewShmem(currentWBContext);
	    msg = (NXWBMessage *)currentWBContext->scheduler->inputPort;
	    reason = yield_other;
	}

	ContextYield(reason, &msg);
	reason = yield_stdin;
	
	if (currentWBContext->dirtyShmem) {
	    WBNotifyNewShmem(currentWBContext);
	    msg = (NXWBMessage *)currentWBContext->scheduler->inputPort;
	    reason = yield_other;
	} else {
	    if (currentWBContext->deathKnell)
		break;
    
	    /* now, gobble up the message that's waiting for us */
	    switch (msg->header.msg_id & NXWB_FUNCTIONS) {
		case NXWB_OPEN:
		    if ((err = WBOpenBitmap((NXWBOpenMsg *)msg)) == 
				NXWB_NOERR)
			WBMsg_send((msg_header_t *)msg, MSG_OPTION_NONE, 0);
		    else
			WBError(err);
		    break;
		    
		case NXWB_PUT:
		    WBMarkBitmap(msg);
		    if (msg->header.msg_id & NXWB_FLUSH)
			WBFlushBitmap(msg);
		    if (msg->header.msg_id & NXWB_SYNCFLUSH)
			/* send a sync message */
			WBError(NXWB_NOERR);
		    break;
		    
		case NXWB_GET:
		    if ((err = WBGetBitmap(msg)) == NXWB_NOERR)
			WBMsg_send((msg_header_t *)msg, MSG_OPTION_NONE, 0);
		    else
			WBError(err);
		    break;
		    
		case NXWB_CLOSE:
		    WBCloseBitmap(msg->wid);
		    break;
	    }
	}
    }
    ContextYield(yield_terminate, 0);
    CantHappen();
}

PSSchedulerContext NSCreateWB(NXWBConnectMsg *msg, NSContextType *type)
{
    WBContext *wbc;
    PSSchedulerContext psc;
    extern void NSSwitchinWB();
    
    wbc = (WBContextP)malloc(sizeof(WBContext));
    bzero((char *)wbc, sizeof(WBContext));
    psc = (PSSchedulerContext) CreateTypedContext(WBCoProc, wbc, type);
    wbc->scheduler = psc;
    wbc->next = WBCList;
    WBCList = wbc;

    /*  Set up the I/O ports for this WB context.  The input port
     *  (how we get data from the client) has to be created here,
     *  and sent back.  The output port (how we send data to the
     *  client) is sent to us by the client in the notify_port field
     *  of our message.
     */

    /* Create a new input port for accepting the WB Context's data */
    if (port_allocate(task_self(), &(wbc->inPortCache)) != KERN_SUCCESS) {
	free((char *)wbc);
	PSLimitCheck();
	return;
    }

    /* The output port was sent to us */
    SetRemotePort(psc, msg->port /*_header.msg_remote_port*/);
    
    /* It's all created so send back the reply message */
    msg->port = wbc->inPortCache;
    WBMsg_send((msg_header_t *)msg, SEND_TIMEOUT, 0);
    return psc;
}

void NSTermWB(WBContext *ctxt)
{
    ctxt->deathKnell = true;
}

void NSDestroyWB(WBContext *wbc)
{
    WBContext **pw, *w;
    
    for (pw = &WBCList, w = WBCList; w; pw = &w->next, w = w->next)
	if (w == wbc) {
	    *pw = w->next;
	    break;
	}
    
    if (w == NULL) return;	/* no such context */

    if (wbc->shmemfd > 0)
	WBCloseBitmap(wbc->wid);

    free((char *)(wbc));
    if (WBCList == NULL)
	currentWBContext = NULL;
}

PSSchedulerContext NSLoadWB(PSSchedulerContext psc, WBContext *ctxt)
{
    currentWBContext = ctxt;
    return psc;
}

void WBNotifyNewShmem(WBContext *wbc)
{
    NXWBOpenMsg msg;

    /* notify the client, so it can adjust it's shmem */
    msg.header.msg_simple = 1;
    msg.header.msg_size = sizeof(msg);
    msg.header.msg_local_port = wbc->scheduler->inputPort;
    msg.header.msg_remote_port = wbc->scheduler->remotePort;
    msg.header.msg_type = MSG_TYPE_NORMAL;
    msg.header.msg_id = NXWB_NOERR;
    msg.wid_type.msg_type_name = MSG_TYPE_INTEGER_32;
    msg.wid_type.msg_type_size = 8 * sizeof(int);
    msg.wid_type.msg_type_number = 4;
    msg.wid_type.msg_type_inline = TRUE;
    msg.wid_type.msg_type_longform = FALSE;
    msg.wid_type.msg_type_deallocate = FALSE;
    msg.wid = wbc->wid;
    msg.roi_type.msg_type_name = MSG_TYPE_REAL;
    msg.roi_type.msg_type_size = 8 * sizeof(float);
    msg.roi_type.msg_type_number = 4;
    msg.roi_type.msg_type_inline = TRUE;
    msg.roi_type.msg_type_longform = FALSE;
    msg.roi_type.msg_type_deallocate = FALSE;
    WBGetDeviceInfo(&msg, wbc->wid);
    msg.path_type.msg_type_name = MSG_TYPE_CHAR;
    msg.path_type.msg_type_size = 8 * sizeof(char);
    msg.path_type.msg_type_number = WBSHPATHLEN;
    msg.path_type.msg_type_inline = TRUE;
    msg.path_type.msg_type_longform = FALSE;
    msg.path_type.msg_type_deallocate = FALSE;
    strncpy(msg.path, wbc->shmemPath, WBSHPATHLEN);

    WBMsg_send((msg_header_t *)&msg, MSG_OPTION_NONE, 0);  /* don't wait */
    wbc->dirtyShmem = false;
}

/* This routine gets called by the dynamic loader to initialize this package. 
 * A return value of 0 means all went well.
 */
int WBmain(NSContextType *myEntry)
{
    myEntry->ncids = 1;
    myEntry->cid = (int *)NXWB_CONTEXTID;
    myEntry->createProc		= NSCreateWB;
    myEntry->termProc		= NSTermWB;
    myEntry->destroyProc	= NSDestroyWB;
    myEntry->loadProc		= NSLoadWB;
    myEntry->unloadProc		= NULL;
    myEntry->timeoutProc	= NULL;
    myEntry->notifyProc		= NULL;
    myEntry->checkNotifyProc	= NULL;
    myEntry->current = (NSContext *)(&currentWBContext);
    myEntry->usesIPCStms = 0;
    myEntry->filler = 0;
    return 0;
}



