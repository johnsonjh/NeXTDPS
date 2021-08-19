#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
    queues.c

    This file has routines to maintain the event queue.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#include "dpsclient.h"
#include <string.h>
#include "defs.h"

static void condenseQ(void);

/* Puts current connection in or out of a mode where it coalesces either
   mouse-moves or mouse-dragged events in the client side Q to improve
   tracking performance.  Mouse up events are also combined with preceding
   mouse-dragged events.
 */
int DPSSetTracking(int flag)
{
    int old;

    if (flag && !_DPSTracking)
	condenseQ();
    old = _DPSTracking;
    _DPSTracking = flag;
    return old;
}


/* flushes the event Q of events that match the mask and server.
   We keep track of the largest event time of the flushed events for each
   context.  Then, for any context for which we flushed events, we send a
   Wait Cursor ack to the Window Server so the WC doesnt spin forever.
*/
void DPSDiscardEvents(DPSContext ctxt, int mask)
{
    _DPSQueue *q = &_DPSEventQ;
    _DPSQueueElement **prevNextPtr;
    _DPSQueueElement *matchingEvent;
    _DPSMachContext mc;
    
    ASSERT(ctxt, "NULL context in DPSDiscardEvents");
    if (ctxt == DPS_ALLCONTEXTS)
	ctxt = NULL;
    for (mc = (_DPSMachContext)(_DPSMachContexts.head); mc; mc = mc->nextMC)
	mc->WCFlushTime = 0;
    prevNextPtr = &(q->firstOut);
    while (*prevNextPtr)
	if ((NX_EVENTCODEMASK((*prevNextPtr)->ev.type) & mask) &&
			(!ctxt || (*prevNextPtr)->ev.ctxt == ctxt)) {
	    matchingEvent = *prevNextPtr;
	    *prevNextPtr = matchingEvent->next;
	    if (matchingEvent->ev.ctxt && matchingEvent->ev.ctxt->type == dps_machServer) {
		mc = STD_TO_MACH(matchingEvent->ev.ctxt);
		if (matchingEvent->ev.time > mc->WCFlushTime)
		    mc->WCFlushTime = matchingEvent->ev.time;
	    }
	    FREE(matchingEvent);
	} else
	    prevNextPtr = &((*prevNextPtr)->next);
    if (q->firstOut)			/* if some items left in Q */
      /* assumes next is the first field of _DPSQueueElement */
	q->lastIn = (_DPSQueueElement *)prevNextPtr;
    else
	q->lastIn = NULL;
    for (mc = (_DPSMachContext)(_DPSMachContexts.head); mc; mc = mc->nextMC)
	if (mc->WCFlushTime)
	    _DPSFlushStream(mc->outStream, FALSE, mc->WCFlushTime);
}


/* sets the procedure called when we see an event */
DPSEventFilterFunc DPSSetEventFunc(DPSContext ctxtArg, DPSEventFilterFunc routine)
{
    DPSEventFilterFunc temp;
    _DPSServerContext ctxt = STD_TO_SERVER(ctxtArg);

    temp = ctxt->eventFunc;
    ctxt->eventFunc = routine;
    return temp;
}


/* returns ptr to next element in a queue matching mask, or NULL if queue empty.  A NULL context will match no events.  */
NXEvent *_DPSGetQEntry(int mask, DPSContext ctxt)
{
    _DPSQueue *q = &_DPSEventQ;
    _DPSQueueElement *item;

    if (!ctxt)
	return NULL;
    if (ctxt == DPS_ALLCONTEXTS)
	ctxt = NULL;

    for (item = q->firstOut; item; item = item->next)
	if ((NX_EVENTCODEMASK(item->ev.type) & mask) &&
				(!ctxt || (item->ev.ctxt == ctxt)))
	   return &(item->ev);
    return NULL;
}


/* posts an event in the Q */
int _DPSPostQElement(NXEvent *event, int postAtStart)
{
    _DPSQueueElement *new;
    _DPSQueue *q = &_DPSEventQ;

    MALLOC(new, _DPSQueueElement, 1);
    new->ev = *event;
    if (postAtStart) {
	new->next = q->firstOut;
	q->firstOut = new;
	if (!q->lastIn)
	    q->lastIn = new;
    } else {
	new->next = NULL;
	if (q->lastIn)
	    q->lastIn->next = new;
	q->lastIn = new;
	if (!q->firstOut)
	    q->firstOut = new;
    }
    return 0;
}


/* removes an element from the Q */
void _DPSRemoveQEntry(NXEvent *event)
{
    _DPSQueue *q = &_DPSEventQ;
    _DPSQueueElement **prevNextPtr;
    _DPSQueueElement *matchingEvent;
    
    ASSERT(q->firstOut, "_DPSRemoveQEntry: can't retract from empty queue" );
    prevNextPtr = &(q->firstOut);
    while (*prevNextPtr)
	if (&((*prevNextPtr)->ev) == event) {
	    matchingEvent = *prevNextPtr;
	    *prevNextPtr = matchingEvent->next;
	    FREE(matchingEvent);
	    if (q->lastIn == matchingEvent)
		if (q->firstOut)		/* if some items left in Q */
		  /* assumes next is the first field of _DPSQueueElement */
		    q->lastIn = (_DPSQueueElement *)prevNextPtr;
		else
		    q->lastIn = NULL;
	    return;
	} else
	    prevNextPtr = &((*prevNextPtr)->next);
    ASSERT(FALSE, "_DPSRemoveQEntry: event not found");
}


/* returns a pointer to the next available event to be filled in.  */
NXEvent *_DPSGetNextAvail(void)
{
    _DPSQueueElement *new;
    _DPSQueue *q = &_DPSEventQ;
    NXEvent dummyEvent;

    _DPSPostQElement(&dummyEvent, FALSE);
    return &(q->lastIn->ev);
}


/*  coalesces events in the given Q.  Adjoining mouse-moved events are combined into one event.  Adjoining mouse-dragged events are combined, and if the sequence is followed by a mouse-up event, then it replaces the whole series of mouse-dragged events.  */
static void condenseQ(void)
{
    _DPSQueue *q = &_DPSEventQ;
    _DPSQueueElement **firstEvPrevNextPtr;
    _DPSQueueElement *secondEv;
    _DPSQueueElement *deadEvent;
    int firstType, secondMask;

    if (q->firstOut == q->lastIn)	/* if 0 or 1 elements in Q */
	return;
    firstEvPrevNextPtr = &(q->firstOut);
    secondEv = q->firstOut->next;
    while (secondEv) {
	firstType = (*firstEvPrevNextPtr)->ev.type;
	secondMask = NX_EVENTCODEMASK(secondEv->ev.type);
	if(((firstType == NX_LMOUSEDRAGGED && 
		    (secondMask & (NX_LMOUSEDRAGGEDMASK | NX_LMOUSEUPMASK))) ||
	   (firstType == NX_RMOUSEDRAGGED && 
		    (secondMask & (NX_RMOUSEDRAGGEDMASK | NX_RMOUSEUPMASK))) ||
	   (firstType == NX_MOUSEMOVED && secondMask == NX_MOUSEMOVEDMASK)) &&
	   ((*firstEvPrevNextPtr)->ev.ctxt == secondEv->ev.ctxt)) {
	    deadEvent = *firstEvPrevNextPtr;
	    *firstEvPrevNextPtr = deadEvent->next;
	    FREE(deadEvent);
	} else
	    firstEvPrevNextPtr = &((*firstEvPrevNextPtr)->next);
	secondEv = secondEv->next;
    }
  /* Since we never coalesce the last event, we dont have to worry about fixing q->lastIn.  */
}


#if defined(DEBUG) && defined(TRACE_Q_SIZE)
void _DPSTraceReportQSize(_DPSQueue *q)
{
    int size;

    if (q->firstOut == -1)
	size = 0;	/* queue empty */
    else {
	size = q->lastIn - q->firstOut + 1;
	if (q->firstOut > q->lastIn)
	    size += MAX_SIZE;
    }
    if (size >= 5)
	fprintf(stderr, "Q size = %2d\n", size);
}
#endif
