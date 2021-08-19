/*
  gcimpl.c

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Pasqua: Mon Feb 22 14:01:56 1988
Edit History:
Scott Byer: Wed May 17 14:45:05 1989
Joe Pasqua: Mon Jan  9 13:12:49 1989
Ivor Durham: Sun May 14 16:33:43 1989
Perry Caro: Fri Nov  4 15:05:47 1988
Ed Taft: Mon Nov 13 17:22:43 1989
Jack Newlin 22May90 added gc logging flagged by gcFd (for keyboard lights)
End Edit History.
*/

#include	PACKAGE_SPECS
#include	BASICTYPES
#include	ERROR
#include	EXCEPT
#include	GC
#include	VM
#include	RECYCLER
#include	"abm.h"
#include	"gcinternal.h"
#include	"saverestore.h"
#include	"vm_segment.h"
#import	"sys/syslog.h"
#import <nextdev/kmreg.h>

/*
IMPLEMENTATION NOTES:
o Names of procedures and types are prefixed by the name of the
  system they belong to (like an interface name). So a stack object
  defined for the garbage collector is GC_Stack. A pointer to the
  same is called GC_PStack.
o We use a pointer to a VMStructure structure to identify a PS VM.
  It is a VM Handle. Collections are relative to VM's, not threads,
  since there may be several threads per VM.
o About array objects: An array body is just a vector of objects, so
  there is not a separate place to store a seen bit for the body.
  Instead we mark every object in an array as seen as we scan the
  them. This avoids potential problems with subarrays also.
o Having the seen bit in name entries is not an optimization like it
  is for dictbodies and arrays. It is required for proper name cache
  finalization.
o The tag "TMP" is used in comments to mark things that should be fixed.
  The tag "NEED THIS" marks procedures that the collector needs.

TO DO: (in roughly decreasing order of importance)
o This version uses simple slow table based mechanism for mapping a
  reference to a segment. This needs to be replaced at some point.
*/

/*----------------------------------------------*/
/*		TYPE Declarations		*/
/*----------------------------------------------*/

/*----------------------------------------------*/
/*		CONSTANT Declarations		*/
/*----------------------------------------------*/

/*----------------------------------------------*/
/*	PUBLIC GLOBAL Data			*/
/*----------------------------------------------*/

/*----------------------------------------------*/
/*	LOCALLY EXPORTED GC Data		*/
/* Available for use by G.C. impl. modules.	*/
/*----------------------------------------------*/

/*----------------------------------------------*/
/*	PRIVATE INLINE Procedures		*/
/*----------------------------------------------*/

/*----------------------------------------------*/
/*		FORWARD Declarations		*/
/*----------------------------------------------*/

private procedure TracePrivateVM();

/*----------------------------------------------*/
/*		EXPORTED GC Procedures		*/
/*----------------------------------------------*/

procedure GC_CollectPrivate(space)
VMStructure *space;
{
  register CallBackItem *cur;
  GC_PData gcData = space->priv->gcData;
  NullObj dummy;
  boolean currentlyShared = CurrentShared();
  extern int gcFd;
  int localInt;

  if (gcFd > 0) {
    syslog(LOG_DAEMON|LOG_NOTICE, "collected Private VM for %9x", space);
    localInt = KM_LED_LEFT;
    ioctl(gcFd, KMIOSLEDSTATES, &localInt);
  }

  if (currentlyShared)
    SetShared(false);

  LNullObj (dummy);
  dummy.shared = false;
  InvalidateRecycler (&dummy, NIL);

  if (currentlyShared)
    SetShared(true);

  TracePrivateVM(space, gcData);

  /* Call all finalize procs...			*/
  for (cur = gcFinalizeProcs; cur != NIL; cur = cur->next)
    (*(cur->proc))(cur->clientData, gcData);

  space->valueForSeen = !space->valueForSeen;
    /* Change sense of seen bit			*/
  GCInternal_ResetFreePointer(vmPrivate);
  space->allocCounter = 0;
  space->collectThisSpace = false;
  if (gcFd > 0) {
    localInt = 0;
    ioctl(gcFd, KMIOSLEDSTATES, &localInt);
  }
}

/*----------------------------------------------*/
/*	LOCALLY EXPORTED GC Procedures		*/
/* For use by other vm package modules only.	*/ 
/*----------------------------------------------*/


/*----------------------------------------------*/
/*		Private GC Procedures		*/
/*----------------------------------------------*/

private void TracePrivateVM(space, gcData)
/* This is the guts of a private VM garbage collection.	*/
/* It is basically a big loop that iterates through the	*/
/* roots pushing new roots as they are found.		*/
VMStructure *space;
GC_PData gcData;
{
  boolean seen = space->valueForSeen;
  GC_Stack *pending = GCInternal_AllocStack(700);
  Object *obj;
  PVMSegment sH;

  DebugAssert(pushPkdAryComposites != NIL &&
              pushStmItems != NIL &&
	      pushGStateItems != NIL);

  gcData->type = privateVM;
  gcData->stack = pending;

  Ctxt_StopAllSiblings();

  HasRefs[nameObj] = false;
  HasRefs[cmdObj] = false;
  HasRefs[objNameArray] = false;
    /* During Private collections, these guys need no	*/
    /* further tracing performed on them.		*/

  segmentCache = &nilSegment;
    /* Prepare to use GCInternal_GetSegHnd procedure.	*/

  ABM_ClearAll(space);	/* Clear all allocation bitmaps	*/
  
  GCInternal_PushNoTest(pending, (PObject)NIL);
    /* This is a sentinal item so we know when we have	*/
    /* looked at all the valid items on the GC stack.	*/

  {	/* Gather the roots of the collection...	*/
  register CallBackItem *cur;
  GenericID me, curCtxt;

  /* For each ctxt, call the getRootsProc's...		*/
  me = Ctxt_GetCurrentCtxt();
  for (
    curCtxt = Ctxt_GetNextCtxt(space, Ctxt_NIL);
    curCtxt.stamp != Ctxt_NIL.stamp;
    curCtxt = Ctxt_GetNextCtxt(space, curCtxt))
    {
    Ctxt_SetCurrentCtxt(curCtxt);
    for (cur = getRootsProcs; cur != NIL; cur = cur->next)
      (*(cur->proc))(cur->clientData, gcData);
    }
  Ctxt_SetCurrentCtxt(me);
  }

  /* This is the main loop of the trace algorithm. It pops an	*/
  /* object ptr off the stack and processes it. PObjects on the	*/
  /* GC stack have the following characteristics:		*/
  /*   1. They don't necessarilly point into PS VM. They could	*/
  /*      point into C data space.				*/
  /*   2. If a ptr does point to PS VM, it points to this space	*/
  /*   3. The objects are always composites.			*/
  /*   4. The objects have not been processed yet. An obj may	*/
  /*      be marked as seen, but it is not a recursive ref. It	*/
  /*      happens for the first element in array bodies or if a	*/
  /*      getRootsProc pushes something more than once.		*/
  while ( (obj = GCInternal_Pop(pending)) != NIL )
    {
    obj->seen = seen;
#ifdef	GATHERSTATS
    gcData->maxStackDepth = os_max(
      (long int)gcData->maxStackDepth,
      (long int)(pending->nextFree - pending->base));
#endif	GATHERSTATS

    switch (obj->type) {
      case nullObj: case intObj: case realObj:
      case boolObj: case fontObj: case nameObj: case cmdObj:
        CantHappen();
        break;
      case escObj:
        switch (obj->length) {
          case objMark: case objSave:
          case objNameArray:
            CantHappen();
            break;
	    
	  case objCond: case objLock:
	    {
            PGenericBody body = obj->val.genericval;
    
	    /* Check to see if the body is in shared VM. If so,	*/
	    /* no need to go on. If not, check if its been seen	*/
	    /* before. If it has we're done, else deal with it.	*/
	    if ((sH = GCInternal_GetSegHnd(body, space)) == NIL) break;
	    if (body->seen == seen) break;
	    else
	      {
	      body->seen = seen;
	      ABM_SetAllocated((RefAny)body, sH, (Card32)body->length);
	      }
	    break;
            }
	    
          case objGState:
            {
            PGenericBody body = obj->val.genericval;
    
	    /* Check to see if the body is in shared VM. If so,	*/
	    /* no need to go on. If not, check if its been seen	*/
	    /* before. If it has we're done, else deal with it.	*/
	    if (body->seen == seen)
	      break;
	    if ((sH = GCInternal_GetSegHnd(body, space)) == NIL)
	      break;
	    else
	      {
	      body->seen = seen;
	      ABM_SetAllocated((RefAny)body, sH, (Card32)body->length);
	      (*pushGStateItems)(obj, CAST(gcData, GC_Info));
	      }
	    break;
	    }
          }
        break;

      case stmObj:
        {
        register PStmBody sb = obj->val.stmval;
        if (sb->seen == seen ||
          (sH = GCInternal_GetSegHnd(sb, space)) == NIL)
	  break;
	sb->seen = seen;
	ABM_SetAllocated((RefAny)sb, sH, (Card32)sizeof(StmBody));
	(*pushStmItems)(obj, CAST(gcData, GC_Info));
	break;
        }		/* END OF stmObj CASE			*/

      case strObj:
	{
	register charptr strBody = obj->val.strval;

	if (strBody == NIL)
	  break;
	if ( (sH = GCInternal_GetSegHnd(strBody, space)) == NIL)
	  break;
	ABM_SetAllocated((RefAny)strBody, sH, (Card32)obj->length);
	break;
	}		/* END OF strObj CASE			*/

      case dictObj:
	{
	PDictBody db;
	register PKeyVal kv, last;

        obj = XlatDictRef(obj);
          /* If its a dict, it might be a trickyDict, Xlat it	*/
        db = obj->val.dictval;

	/* Check to see if the dictbody is in shared VM. If so,	*/
	/* no need to go on. If not, check if it has been seen	*/
	/* before. If it has we're done, else deal with it.	*/
	if (db->seen == seen || (sH=GCInternal_GetSegHnd(db, space)) == NIL)
	  break;
	else
	  {
	  db->seen = seen;
	  ABM_SetAllocated((RefAny)db, sH, (Card32)sizeof(DictBody));
	  kv = db->begin;
	  if ((sH = GCInternal_GetSegHnd(kv, space)) == NIL)
	    break;
	  ABM_SetAllocated(
	    (RefAny)kv, sH, (Card32)(db->size * sizeof(KeyVal)));
	  last = db->end;
#ifdef	GATHERSTATS
          gcData->dicts++;
#endif	GATHERSTATS
	  }

	/* Process the dictionary elements.  Process the keys first,	*/
	/* checking the boolean of the dictionary body to see if we	*/
	/* even have to look.  This boolean is maintained by dict.c	*/
	/* in the language package.					*/
	if (db->privTraceKeys)
	  {
	  /* There are composites in the key part of the dictionary,	*/
	  /* scan both the key and value part.				*/
	  for (; kv < last; kv++)
	    {
	    /* First process the key for the entry...		*/
	    if (ContainsRefs(&(kv->key)))
	      {GCInternal_PushNoTest(pending, &(kv->key));}
	    
	    /* Now process the value for the entry...		*/
	    if (ContainsRefs(&(kv->value)))
	      {GCInternal_PushNoTest(pending, &(kv->value));}
	    }
	  }
	else
	  {
	  /* The key part of the dictionary contains nothing we	*/
	  /* care about, scan only the values.			*/
	  for (; kv < last; kv++)
	    if (ContainsRefs(&(kv->value)))
	      {GCInternal_PushNoTest(pending, &(kv->value));}
	  }
	break;
	}		/* END OF dictObj CASE			*/

      case arrayObj:
	/* SEE NOTES ABOUT ARRAY OBJECTS AT HEAD OF MODULE	*/
	{
	register cardinal len = obj->length;
	register PObject arrayEntry = obj->val.arrayval;

        if (len == 0)
          break;	/* Not a very interesting array	*/
	if ((sH = GCInternal_GetSegHnd(arrayEntry, space)) == NIL)
	  break;	/* Not in this space, don't bother	*/
	ABM_SetAllocated(
	  (RefAny)arrayEntry, sH, (Card32)(len * sizeof(Object)));
	for (; len > 0; len--, arrayEntry++)
	  {
	  if (ContainsRefs(arrayEntry) && arrayEntry->seen != seen)
	    {
	    GCInternal_PushNoTest(pending, arrayEntry);
	    }
	  arrayEntry->seen = seen;
	    /* Must set the seen bit on every entry, not just	*/
	    /* the ones that have refs since PrivateForShared	*/
	    /* trace considers different types as having refs.	*/
	    /* We could do another test to see if we really	*/
	    /* need to set the bit, but its not worth it.	*/
	  }
#ifdef	GATHERSTATS
        gcData->arrays++;
#endif	GATHERSTATS
	break;
	}		/* END OF arrayObj CASE			*/

      case pkdaryObj:
	{
	register Card32 paSize;
	register PCard8 aryBody = obj->val.pkdaryval;

        if (obj->length == 0)	/* This is easy...		*/
	  break;
	/* Check to see if the array body is in shared VM. If	*/
	/* so, no need to go on. If not, we must go on since	*/
	/* there's no way to tell if we've seen a pkdary body.	*/
	if ( (sH = GCInternal_GetSegHnd(aryBody, space)) == NIL)
	  break;
#ifdef	GATHERSTATS
        gcData->pkdArys++;
#endif	GATHERSTATS

	paSize = (*pushPkdAryComposites)(obj, CAST(gcData, GC_Info));
	ABM_SetAllocated((RefAny)aryBody, sH, (Card32)paSize);
	break;
	}		/* END OF arrayObj CASE			*/

      default:
        CantHappen();
      }	/* switch on object type				*/
    }	/* while there are more roots to process...		*/

  GCInternal_FreeStack(pending);
  Ctxt_RestartAllSiblings();
}	/* END OF TracePrivateVM	*/

/*----------------------------------------------*/
/*	Internal GC Support Procedures		*/
/*----------------------------------------------*/

