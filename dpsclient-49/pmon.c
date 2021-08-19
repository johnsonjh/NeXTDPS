#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
    pmon.c

    This file has some utility routines for using the pmon performance
    event library.

    Copyright (c) 1990 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#if defined(DEBUG) && defined(PMON)

#import "dpsclient.h"
#import "defs.h"
#import <stdarg.h>
#import <stdio.h>
#import <string.h>
#import "/Net/harlie/mk/osdev/PMON/pmon/include/pmon/pmon_ipc.h"
#import "/Net/harlie/mk/osdev/PMON/pmon/include/pmon/pmon.h"
#import <mach_error.h>

extern char **NXArgv;
 
typedef struct _pmonSrcData {	/* data per pmon event source */
	int src;
	int mask;
	struct _pmonSrcData *next;
} pmonSrcData;

static struct mutex pmonDataLock;
static pmonSrcData *pmonDataHead = NULL;

static port_t signaturePort = PORT_NULL;
static port_t eventPort = PORT_NULL;
static port_t controlPort = PORT_NULL;
static port_t serverPort = PORT_NULL;

static pmon_event_msg_t eventMsg;

static cthread_t pmonThread = 0;

static any_t pmonListener(any_t arg);
static pmonSrcData *getSrcData(int src);

void _NXInitPmon(void)
{
    char *appName;
    char targetName[1024];
    kern_return_t ret;

  /* makes sure the shlib pointer has been initted, which only happens if
     the app thats running gets relinked.
   */
    if (!pmonThread) {
        mutex_init(&pmonDataLock);
	appName = rindex(NXArgv[0], '/');
	if (appName)
	    appName++;
	else
	    appName = NXArgv[0];
	strcpy(targetName, appName);
	strcat(targetName, ".pmon");
	ret = pmon_target_init(&signaturePort, &eventPort, &controlPort,
								targetName);
	if (!ret) {
	    if (pmonThread = cthread_fork(pmonListener, NULL))
		cthread_detach(pmonThread);
	    else
		fprintf(stderr, "dpsclient PMON: couldnt launch thread\n");
	} else
	    pmon_error("dpsclient PMON", ret);
	if(!pmonThread)
	    pmonThread = (cthread_t)-1;   /* make sure we dont try twice */
    }
}

void _NXSendPmonEvent(void *srcDataArg, int eventType, int d1, int d2, int d3)
{
    pmonSrcData *pd;
    struct tsval time;
    struct pmon_event event;
    struct {
	int src;
	void *data;
    } *srcData = srcDataArg;

    if (serverPort && !eventMsg) {
	eventMsg = malloc(PMON_EMSG_SIZE);
	pmon_build_emsg(eventMsg, eventPort, serverPort);
    }
    if (!srcData->data)
	srcData->data = getSrcData(srcData->src);
    pd = srcData->data;
    if (pd->mask & eventType) {
	event.source = pd->src;
	event.event_type = eventType;
	kern_timestamp(&event.time);
	event.data1 = d1;
	event.data2 = d2;
	event.data3 = d3;
	if (pmon_add_event(eventMsg, &event, PM_SEND_IFFULL) < 0)
	    fprintf(stderr, "dpsclient PMON: adding an event failed\n");
    }
}

void _NXFlushPmonEvents(void)
{
    kern_return_t ret;

    if (eventMsg) {
	ret = pmon_send_emsg(eventMsg);
	if (ret)
	    pmon_error("dpsclient PMON", ret);
    }
}

static pmonSrcData *getSrcData(int src)
{
    pmonSrcData *pd;

    for (pd = pmonDataHead; pd; pd = pd->next)
	if (pd->src == src)
	    break;
    if (!pd) {
	pd = calloc(1, sizeof(pmonSrcData));
	pd->src = src;
	pd->next = pmonDataHead;
	mutex_lock(&pmonDataLock);
	pmonDataHead = pd;
	mutex_unlock(&pmonDataLock);
    }
    return pd;
}

static any_t pmonListener(any_t arg)
{
    kern_return_t ret;
    struct pmon_cntrl_msg controlMsgSpace;
    pmon_cntrl_msg_t controlMsg = &controlMsgSpace;
    pmonSrcData *pd;
    
    controlMsg->header.msg_local_port = controlPort;
    controlMsg->header.msg_size = PMON_CMSG_SIZE;

    while(1) {
	ret = msg_receive(controlMsg, MSG_OPTION_NONE, 0);
	if(ret) {
	    mach_error("cntrl_msg_handler: msg_receive", ret);
	    continue;
	}
	pd = getSrcData(controlMsg->source);
	switch(controlMsg->header.msg_id) {
	    case PM_CTL_ENABLE:
		serverPort = controlMsg->event_port;
		pd->mask |= controlMsg->event_type;
		controlMsg->rtn_status = PMR_SUCCESS;
		break;
	    case PM_CTL_DISABLE:
		pd->mask &= ~controlMsg->event_type;
		controlMsg->rtn_status = PMR_SUCCESS;
		break;
	    default:
		fprintf(stderr, "dpsclient PMON: bogus msg_id in control message (%d)\n", controlMsg->header.msg_id);
		controlMsg->rtn_status = PMR_BADMSG;
		break;
	}
	ret = msg_send(&controlMsg->header, SEND_TIMEOUT, 0);
	if (ret)
	    fprintf(stderr, "dpsclient PMON: couldn't reply to control message (%d)\n", ret);
    }
    return 0;
}


#import "pmon/pmon_error.c"
#import "pmon/pmon_target_lib.c"
#import "pmon/pmon_targetu_lib.c"

#endif defined(DEBUG) && defined(PMON)



