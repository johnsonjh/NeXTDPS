#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
    scanner.c

    This file holds the definition for the lexical scanner that
    parses data coming from the server and fills the queues with
    the elements it identifies.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#include "dpsclient.h"
#include "defs.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "deadKeyTable.c"

#define MAXUSHORT  0x8000

/* code to trace peaks in event queue size */
#if defined(DEBUG) && defined(TRACE_Q_SIZE)
#define MARK_Q_GROWING(ctxt)	((ctxt)->serverCtxtFlags.evQGrowing = TRUE)
#else
#define MARK_Q_GROWING(ctxt)
#endif

/* codes found in the "level" field of a binary object */
#define	DPS_EVENTCODE		255	/* event object type */
#define DPS_ERRORCODE		250	/* error object type */
#define DPS_MAXTOKENCODE	249	/* largest possible value for */
		/* a "normal" object (int, real, boolean, string,...) */

/* PostScript's events are smaller since they dont have the connection id */
#define PSEVENT_SIZE	(sizeof(NXEvent) - sizeof(DPSContext))


static void doError(_DPSServerContext cc, DPSBinObjSeqRec *seq);
static int doEvent(int mask, _DPSServerContext cc, int *deadEvTime);
static void doAscii(_DPSServerContext cc);
static void doBinToken(_DPSServerContext cc, DPSBinObjSeqRec *seqStart);
static void tokenOOSync(_DPSServerContext cc, int errCode, DPSBinObjSeqRec *seq, DPSBinObjRec *obj);
static int localRead(int totalChars, char *to, _DPSServerContext cc, int blockFlag);
static char *tryPartialItem(_DPSServerContext cc, char *buf, unsigned char *length, unsigned char totalLength);
static int filterAndQueueEvent(int mask, _DPSServerContext cc, NXEvent *evFrom);
static int doDeadKey(NXEvent *newEvent, NXEvent *oldEvent,
			int mask, _DPSServerContext cc, int *maskSuccess);

/*  This is the entry point into this file.  It reads events, binary tokens,
    errors, and chunks of ascii from the PS connection.  Events are put
    in the event queue, possibly with coalescing.  Tokens are put in the
    results table of the current context, if they match, otherwise cause
    a call on the error proc.  Errors also cause the error proc to be
    called.  Ascii is gobbled until the start of the next binary object
    sequence, and then the text proc is called.

    This routine requires very precise coordination with its caller, and
    the fillBuf proc in the context it is passed.  When the routine is
    entered, if the buffer appears empty (bufCurr==bufEnd), fillBuf will
    be called;  otherwise, it parses what is there.  During parsing,
    errors or out of sync tokens may cause exceptions to be raised, so
    routines up the stack should protect themselves.  Tokens and errors
    that cross buffer boundaries cause fillBuf to be called to complete
    the item.  Events that cross are handled by this routine.  Ascii
    chunks are always terminated at bufEnd.

    It returns TRUE if it found what it was looking for, FALSE if not.  */
int _DPSTokenProc( register int mask, register _DPSServerContext cc )
{
    register int success = FALSE;	/* Did we get the right item? */
    int err;
    DPSBinObjSeqRec *seq;
    int deadEvTime = 0;			/* Timestamp of last dead key */
#if defined(DEBUG) && defined(PMON)
    int numEvents = 0;
    int numTokens = 0;
#endif
/* sizes of pieces of the start of a sequence */
#define HEADERSZ	4
#define TOKPIECESZ	(sizeof(DPSBinObjRec)-sizeof(seq->objects[0].val))
#define WHOLESZ		(HEADERSZ+TOKPIECESZ)

  /* if we dont have any data, get some fresh stuff */
    if( cc->bufCurr == cc->bufEnd )
	(*cc->fillBuf)( SERVER_TO_STD(cc) );
    if( cc->partialEventChars )
      /* finish a half-baked event */
	success |= doEvent(mask, cc, &deadEvTime);
    while( cc->bufCurr < cc->bufEnd )
      /* if start of seq or we've got a half-baked start from before */
	if( (unsigned char)*(cc->bufCurr) == DPS_DEF_TOKENTYPE ||
					    cc->partialHeaderChars ) {
	    if( !(seq = (DPSBinObjSeqRec *)tryPartialItem( cc,
				(char *)&cc->header,
				&cc->partialHeaderChars, WHOLESZ )))
		break;			/* header didnt fit */
	  /* At this point, we have 8 valid chars of the sequence, and
	     bufCurr points to the data of the first token.
	   */
	    if( seq->objects[0].tag == DPS_EVENTCODE ) {
		ASSERT( seq->length == PSEVENT_SIZE+WHOLESZ,
				    "Bad length for event object");
		success |= doEvent(mask, cc, &deadEvTime);
#if defined(DEBUG) && defined(PMON)
		numEvents++;
#endif
	    } else if( seq->objects[0].tag <= DPS_MAXTOKENCODE ) {
		ASSERT( seq->nTopElements == 1,
				"Composite binary object sequence found");
		ASSERT( seq->length == HEADERSZ+sizeof(DPSBinObjRec) +
			((seq->objects[0].attributedType & 0x7f) ==
				DPS_STRING ? seq->objects[0].length : 0),
			"Bad length for token object");
		doBinToken(cc, seq);
#if defined(DEBUG) && defined(PMON)
		numTokens++;
#endif
	    } else if( seq->objects[0].tag == DPS_ERRORCODE )
		doError( cc, seq );
	    else {
		ASSERT( FALSE, "Unknown binary object sequence" );
	      /* skip unknown sequence */
		localRead( seq->length - WHOLESZ, NULL, cc, TRUE );
	    }
	} else	/* its an ascii token of some sort */
	    doAscii(cc);
    _NXSendPmonEvent(_DPSPmonSrcData, pmon_parsed_data,
				numEvents, numTokens, 0);

    if (deadEvTime && SERVER_TO_STD(cc)->type == dps_machServer)
	_DPSFlushStream(cc->outStream, FALSE, deadEvTime);

 /* if we got here, the buffer is empty */
    return success;
}


/*  Handles an error message from PS. It should be called with bufCurr
    at the start of the first object of the error. */
static void doError(_DPSServerContext cc, DPSBinObjSeqRec *seq)
{
    char *buf;

    NXAllocErrorData( seq->length, &(void *)buf );
    bcopy( (char *)seq, buf, WHOLESZ );
    localRead( seq->length - WHOLESZ, buf + WHOLESZ, cc, TRUE );
    RAISE_ERROR( SERVER_TO_STD(cc), dps_err_ps, buf, (void *)seq->length );
}


/* Filters out dead keys.  Returns whether to keep processing event. */
static int doDeadKey(NXEvent *newEvent, NXEvent *oldEvent,
			int mask, _DPSServerContext cc, int *maskSuccess)
{
    const unsigned char *cInfo;
    int canBeDead;

  /* chars that are not in the ascii char set, or have bucky bits on cant play in the dead key game */
    canBeDead = (newEvent->data.key.charSet == NX_ASCIISET) &&
		!(newEvent->flags & (NX_CONTROLMASK | NX_COMMANDMASK));
    *maskSuccess = FALSE;
    if (!oldEvent->data.key.charCode) {		/* if !previous dead key */
	if (canBeDead && DiacritcalInfoOffsets[newEvent->data.key.charCode]) {
	    *oldEvent = *newEvent;		/* its a dead, stash it */
	    return FALSE;
	}
    } else {
	if (canBeDead) {
	    cInfo = DiacritcalInfo +
			    DiacritcalInfoOffsets[oldEvent->data.key.charCode];
	    while (*cInfo) {
		if (*cInfo++ == newEvent->data.key.charCode)
		    break;
		cInfo++;			/* skip result char of non-match */
	    }
	} else
	    cInfo = NULL;
	if (cInfo && *cInfo)
	    newEvent->data.key.charCode = *cInfo;
	else
	    *maskSuccess = filterAndQueueEvent(mask, cc, oldEvent);
	oldEvent->data.key.charCode = '\0';
    }
    return TRUE;
}


/*  Does the real work for an event.  Calls filter proc and puts it in the Q */
static int filterAndQueueEvent(int mask, _DPSServerContext cc, NXEvent *evFrom)
{
    register _DPSQueue *q = &_DPSEventQ;
    register NXEvent *evTo;
    register int lastType;

    if( cc->eventFunc ) {
	evFrom->ctxt = SERVER_TO_STD(cc);
	if( !(*cc->eventFunc)(evFrom) )
	    return FALSE;	/* proc consumed the event */
    }

    lastType = _DPS_QUEUEEMPTY(q) ? NX_NULLEVENT : _DPS_LASTQELEMENT(q)->type;
    MARK_Q_GROWING(cc);
  /* if we need to combine events */
    if( lastType &&	/* 0 if Q empty */
       ((evFrom->type == NX_LMOUSEDRAGGED && lastType == NX_LMOUSEDRAGGED) ||
        (evFrom->type == NX_RMOUSEDRAGGED && lastType == NX_RMOUSEDRAGGED) ||
        (evFrom->type == NX_MOUSEMOVED && lastType == NX_MOUSEMOVED)) &&
							_DPSTracking ) {
     /* copy new stuff to the matching element, thus coalescing it out */
	evTo = _DPS_LASTQELEMENT(q);
	ASSERT( NX_EVENTCODEMASK(evTo->type) & (NX_LMOUSEDRAGGEDMASK |
		NX_RMOUSEDRAGGEDMASK | NX_MOUSEMOVEDMASK),
		"wrong type of event coalesced");
    } else { 
      /* copy new stuff to a new element in the Q */
	evTo = _DPSGetNextAvail();
	if( !evTo ) {
	    ASSERT( FALSE, "Queue full" );
	  /* pitch the first in Q event if Q full */
	    _DPSRemoveQEntry(_DPS_FIRSTQELEMENT(q));
	    evTo = _DPSGetNextAvail();
	}
    }
    bcopy( (char *)evFrom, (char *)evTo, PSEVENT_SIZE );   /* put it in Q */
    evTo->ctxt = SERVER_TO_STD(cc);
    if( cc->serverCtxtFlags.traceEvents ) {
	fprintf(stderr, "Receiving: ");
	_DPSPrintEvent(stderr, evTo);
    }
    if( NX_EVENTCODEMASK(evFrom->type) & mask ) {
	if( !_DPSLastEvent )
	    _DPSLastEvent = evTo;
	return TRUE;
    } else
	return FALSE;
}


/*  This routine is called when an event code has been scanned.  Does dead key processing of event and the calls the proc that filters and queues the event. */
static int doEvent(int mask, _DPSServerContext cc, int *deadEvTime)
{
    register NXEvent *evFrom;
    int firstSuccess, secondSuccess;

    if( !(evFrom = (NXEvent *)tryPartialItem( cc, (char *)&cc->event,
				&cc->partialEventChars, PSEVENT_SIZE )))
	return 0;
    ASSERT(evFrom->type >= NX_FIRSTEVENT && evFrom->type <= NX_LASTEVENT, 
			    "Invalid type of event found in doEvent" );

    if (cc->serverCtxtFlags.doDeadKeys && evFrom->type == NX_KEYDOWN) {
	if (!doDeadKey(evFrom, &cc->deadDownEvent, mask, cc, &firstSuccess)) {
	    *deadEvTime = evFrom->time;
	    return FALSE;
	}
    } else if (cc->serverCtxtFlags.doDeadKeys && evFrom->type == NX_KEYUP) {
	if (!doDeadKey(evFrom, &cc->deadUpEvent, mask, cc, &firstSuccess)) {
	    *deadEvTime = evFrom->time;
	    return FALSE;
	}
    } else {
	*deadEvTime = 0;
	firstSuccess = FALSE;
    }
    secondSuccess = filterAndQueueEvent(mask, cc, evFrom);
    return firstSuccess || secondSuccess;
}


/* finish up any whitespace separated token.  Returns TRUE if a token
   is now available.
*/
static void doAscii(_DPSServerContext cc)
{
    char *buf = cc->bufCurr;

    while( cc->bufCurr < cc->bufEnd &&
		    (unsigned char)*(cc->bufCurr) != DPS_DEF_TOKENTYPE )
	cc->bufCurr++;
    (*(cc)->c.textProc)( SERVER_TO_STD(cc), buf, cc->bufCurr - buf );
}


/* raises an error for objects out of sync */
static void tokenOOSync(_DPSServerContext cc,
			int errCode,
			DPSBinObjSeqRec *seq,	/* first 8 bytes of sequence */
			DPSBinObjRec *obj)
{
    DPSBinObjSeqRec *buf;

    NXAllocErrorData( seq->length, &(void *)buf );
    bcopy( (char *)seq, buf, WHOLESZ );
    buf->objects[0].val.integerVal = obj->val.integerVal;
    if( (obj->attributedType & 0x7f) == DPS_STRING )
	localRead( seq->length - sizeof(DPSBinObjSeqRec), (char *)(buf + 1), cc, TRUE );
    RAISE_ERROR( SERVER_TO_STD(cc), errCode, buf, (void *)seq->length );
}


/* process binary objects from PostScript. */
static void doBinToken(_DPSServerContext cc,
			DPSBinObjSeqRec *seqStart)   /* 1st 8 bytes of seq */
{
    DPSResults res;
    DPSBinObjRec firstObj;
    DPSBinObjRec *obj = &firstObj;
    int objType;
    int dataSize;
    unsigned short stringBytesRead = 0;		/* init'ed to shut up -W */

  /* copy first half of first object from seqStart we were passed */
    obj->attributedType = seqStart->objects[0].attributedType;
    obj->tag = seqStart->objects[0].tag;
    obj->length = seqStart->objects[0].length;
  /* get second half from the input stream */
    localRead( sizeof(long int), (char *)&(obj->val), cc, TRUE );

    objType = obj->attributedType & 0x7f;	/* get rid of lit/exec bit */
    if( !cc->c.resultTable || obj->tag > cc->c.resultTableLength )
	tokenOOSync( cc, dps_err_resultTagCheck, seqStart, obj );
    else if( obj->tag == cc->c.resultTableLength )
	cc->c.resultTable = NULL;		/* last token in list */
    else {
	res = cc->c.resultTable + obj->tag;
	if( res->count != 0 ) {
	    switch( res->type ) {
	    case dps_tChar:
	    case dps_tUChar:
		if( objType == DPS_STRING ) {
		    ASSERT( obj->val.stringVal == sizeof(DPSBinObjRec),
					    "string data out of place" );
		    if( res->count == -1 )
			stringBytesRead = obj->length;
		    else
			stringBytesRead = MIN(res->count, obj->length);
		    localRead( stringBytesRead, res->value, cc, TRUE );
		    if( res->count == -1 || res->count > obj->length )
			res->value[obj->length] = '\0';
		    localRead( seqStart->length - sizeof(DPSBinObjSeqRec) -
				    stringBytesRead, NULL, cc, TRUE );
		} else
		    tokenOOSync( cc, dps_err_resultTypeCheck, seqStart, obj );
		break;
	    case dps_tBoolean:
		{
		    int *val;
		    
		    val = (int *)(res->value);
		    dataSize = sizeof(int);
		    if( objType == DPS_BOOL )
			*val = obj->val.booleanVal != 0;
		    else
			tokenOOSync( cc, dps_err_resultTypeCheck,
							seqStart, obj );
		}
		break;
	    case dps_tFloat:
		{
		    float *val;
		    
		    val = (float *)(res->value);
		    dataSize = sizeof(float);
		    if( objType == DPS_REAL )
			*val = obj->val.realVal;
		    else if( objType == DPS_INT )
			*val = obj->val.integerVal;
		    else
			tokenOOSync( cc, dps_err_resultTypeCheck, 
						    seqStart, obj );
		}
		break;
	    case dps_tDouble:
		{
		    double *val;
		    
		    val = (double *)(res->value);
		    dataSize = sizeof(double);
		    if( objType == DPS_REAL )
			*val = obj->val.realVal;
		    else if( objType == DPS_INT )
			*val = obj->val.integerVal;
		    else
			tokenOOSync( cc, dps_err_resultTypeCheck, 
						    seqStart, obj );
		}
		break;
	    case dps_tShort:
	    case dps_tUShort:
		{
		    short *val;
		    
		    val = (short *)(res->value);
		    dataSize = sizeof(short);
		    if( objType == DPS_REAL )
			*val = obj->val.realVal;
		    else if( objType == DPS_INT )
			*val = obj->val.integerVal;
		    else
			tokenOOSync( cc, dps_err_resultTypeCheck, 
						    seqStart, obj );
		}
		break;
	    case dps_tInt:
	    case dps_tUInt:
	    case dps_tLong:
	    case dps_tULong:
		ASSERT( sizeof(int) == sizeof(long), "ints arent longs");
		{
		    int *val;
		    
		    val = (int *)(res->value);
		    dataSize = sizeof(int);
		    if( objType == DPS_REAL )
			*val = obj->val.realVal;
		    else if( objType == DPS_INT )
			*val = obj->val.integerVal;
		    else
			tokenOOSync( cc, dps_err_resultTypeCheck, 
						    seqStart, obj );
		}
		break;
	    default:
		ASSERT( FALSE, "Bad result type in DPSAwaitReturnValues" );
	    }
	    if( cc->traceCtxt && STD_TO_OUTPUT(cc->traceCtxt)->debugging) {
		NXStream *st = STD_TO_OUTPUT(cc->traceCtxt)->outStream;

		NXPrintf( st, "%% value returned ==> " );
		if( objType == DPS_STRING )
		    if( obj->length > 150 )
			NXPrintf( st, "( <%d byte string> )", obj->length );
		    else
			NXWrite( st, res->value, stringBytesRead );
		else if( objType == DPS_ARRAY )
		    NXPrintf( st, "<array>" );
		else if( objType == DPS_NAME )
		    NXPrintf( st, "<name>" );
		else
		    _DPSConvertBinArray( st, obj, 1, 0, seqStart,
						dps_ascii, dps_strings, TRUE );
		NXWrite( st, "\n", 1 );
	    }
	    if( res->type == dps_tChar || res->type == dps_tUChar)
		res->count = 0;		/* mark this string as done */
	    else if( res->count != -1 ) {
		res->count--;
		res->value += dataSize;
	    }
	} else {	/* else extra array objects */
	    if( objType == DPS_STRING )		/* read rest of object */
		localRead( seqStart->length - sizeof(DPSBinObjSeqRec),
							NULL, cc, TRUE );
	}
    }
}


/*  This routine fetches data from the connection.  It refills the local
    buffer when the buffer is empty.  It will block.  If to is NULL, then
    it just skips data in the stream without copying it anywhere.
    
    WARNING:  This reads into the same message buffer that previous data
    was stored in.  Any pointers into the stream are invalidated by this call. */
static int localRead(int totalChars, char *to, _DPSServerContext cc, int blockFlag)
{
    register char *bufCurr;
    register int charsAvail;
    register int charsToRead;
    register int totalLeftToRead;

    totalLeftToRead = totalChars;
    while (1) {
	bufCurr = cc->bufCurr;
	charsAvail = cc->bufEnd - bufCurr;
	charsToRead = MIN( charsAvail, totalLeftToRead );
	if( to ) {
	    bcopy( bufCurr, to, charsToRead );
	    to += charsToRead;
	}
	cc->bufCurr += charsToRead;
	totalLeftToRead -= charsToRead;
	if( totalLeftToRead && blockFlag )	/* if we need more */
	    (*cc->fillBuf)( SERVER_TO_STD(cc) );
	else
	    break;
    }
    return totalChars - totalLeftToRead;
}


/* tries to get the rest of an item that crossed a previous buffer boundary.
   Returns a pointer to the complete data or NULL if there wasnt enough
   to complete the object. */
static char *tryPartialItem(_DPSServerContext cc, char *buf, unsigned char *length, unsigned char totalLength)
{
    *length += localRead( totalLength - *length, buf + *length, cc, FALSE );
    if( totalLength - *length )
	return NULL;
    else {
	*length = 0;
	return buf;
    }
}
