/*
			       gcsharedimpl.c

	     Copyright (c) 1988, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE: All  information contained herein is the  property  of  Adobe Systems
Incorporated.  Many  of  the intellectual   and technical  concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only  to Adobe licensees for their  internal use.  Any reproduction
or dissemination of this software  is strictly forbidden unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Pasqua: Fri Feb 19 17:15:48 1988
Edit History:
Scott Byer: Wed May 17 14:48:52 1989
Ivor Durham: Sun May 14 16:41:58 1989
Joe Pasqua: Thu Jul  6 14:48:47 1989
Perry Caro: Mon Nov  7 13:42:08 1988
Ed Taft: Tue Nov 28 11:07:21 1989
End Edit History.
Jack Newlin 22May90 added gc logging flagged by gcFd (for keyboard lights)
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
o See also notes in gcimpl.c.
o The tag "TMP" is used in comments to mark things that should be fixed.
  The tag "NEED THIS" marks procedures that the collector needs.
o About array objects: An array body is just a vector of objects, so
  there is not a separate place to store a seen bit for the body.
  Instead we mark every object in an array as seen as we scan the
  them. This avoids potential problems with subarrays also.
  
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
/*	PRIVATE INLINE Procedures		*/
/*----------------------------------------------*/

/*----------------------------------------------*/
/*		FORWARD Declarations		*/
/*----------------------------------------------*/

private procedure TracePrivateForShared();
private procedure TraceSharedVM();

/*----------------------------------------------*/
/*	LOCALLY EXPORTED GC Procedures		*/
/* Available for use by G.C. impl. modules.	*/
/*----------------------------------------------*/

procedure GC_CollectShared()
{
  register CallBackItem *cur;
  boolean currentlyShared = CurrentShared();
  NullObj dummy;
  extern int gcFd;
  int localInt;

  if (gcFd > 0) {
    syslog(LOG_DAEMON|LOG_NOTICE, "collected Shared VM");
    localInt = KM_LED_LEFT;
    ioctl(gcFd, KMIOSLEDSTATES, &localInt);
  }

  Ctxt_StopAllCtxts();	/* STOP THE WORLD	*/

  if (!currentlyShared)
    SetShared(true);

  LNullObj (dummy);
  dummy.shared = true;

  InvalidateRecycler(&dummy, NIL);

  if (!currentlyShared)
    SetShared(false);

  TraceSharedVM(vmShared->priv->gcData);

  /* Call all finalize procs...			*/
  for (cur = gcFinalizeProcs; cur != NIL; cur = cur->next)
    (*(cur->proc))(cur->clientData, vmShared->priv->gcData);

  vmShared->valueForSeen = !vmShared->valueForSeen;
    /* Change sense of seen bit			*/
  GCInternal_ResetFreePointer(vmShared);
  vmShared->allocCounter = 0;
  vmShared->collectThisSpace = false;
  Ctxt_RestartAllCtxts();	/* RESTART THE WORLD	*/
  if (gcFd > 0) {
    localInt = 0;
    ioctl(gcFd, KMIOSLEDSTATES, &localInt);
  }
}

/*----------------------------------------------*/
/*		Private GC Procedures		*/
/*----------------------------------------------*/

private procedure TraceSharedVM(gcData)
/* This procedure is the guts of a shared VM collection	*/
/* while a shared collection is in progress, no context	*/
/* can run. All private VM's are collected as a side	*/
/* effect of a shared VM collection.			*/
GC_PData gcData;
{
  Object *obj;
  PVMSegment sH;
  boolean seen = vmShared->valueForSeen;
  GC_Stack *pending = GCInternal_AllocStack(700);

  DebugAssert(pushPkdAryComposites != NIL &&
              pushStmItems != NIL &&
	      pushGStateItems != NIL);

  gcData->type = sharedVM;
  gcData->stack = pending;

  
  GCInternal_PushNoTest(pending, (PObject)NIL);
    /* This is a sentinal item so we know when we have	*/
    /* looked at all the valid items on the GC stack.	*/

  /* Prepare to do a shared collection by setting up	*/
  /* appropriate state and collecting the shared roots.	*/
  /* We'll need to reset some of this state later since	*/
  /* we're going to do some private gc's in a bit. We	*/
  /* must collect roots now since we promise to do this	*/
  /* at the very beginning of a collection.		*/
  HasRefs[cmdObj] = true;
  HasRefs[nameObj] = true;
  HasRefs[objNameArray] = true;
    /* We look at these for the shared case.		*/
  segmentCache = &nilSegment;
    /* Prepare to use GCInternal_GetSegHnd procedure.	*/
  ABM_ClearAll(vmShared);
    /* Clear the alloc bitmaps for shared VM		*/
  {	/* Gather the shared roots of the collection...	*/
  register CallBackItem *cur;
  
  for (cur = sharedRootsProcs; cur != NIL; cur = cur->next)
    (*(cur->proc))(cur->clientData, CAST(gcData, GC_Info));
  }

  /*	----> Collect each of the private VM's <----	*/
  /* Perform a collection of each of the private vm's.	*/
  /* As well as doing a normal collection this pass	*/
  /* pushes shared roots found in private vm's.		*/
  {
  PVM space;
  GC_Stack *privStack = GCInternal_AllocStack(700);

  /* We leave the HasRefs array alone even though we're	*/
  /* tracing private vm since we want to find all refs	*/
  /* into shared VM so that they can be marked.		*/
  for (	/* FOR each space DO				*/
    space = Ctxt_GetNextSpace((PVM)NIL);
    space != NIL;
    space = Ctxt_GetNextSpace(space))
    {
    register CallBackItem *cur;

    {
      boolean currentlyShared = CurrentShared();
      NullObj dummy;
      PRecycler oldPrivateRecycler = privateRecycler;

      if (currentlyShared)
        SetShared(false);

      LNullObj (dummy);
      dummy.shared = false;
      InvalidateRecycler (&dummy, NIL);

      if (currentlyShared)
        SetShared(true);

      privateRecycler = oldPrivateRecycler;
    }
    TracePrivateForShared(space, pending, privStack);
    space->valueForSeen = !space->valueForSeen;
    GCInternal_ResetFreePointer(space);
    space->allocCounter = 0;
    space->collectThisSpace = false;
    }
  GCInternal_FreeStack(privStack);
  }
  /*	----> END private VM collections <----		*/

  /* Yes, we did the following earlier, but we have to	*/
  /* do it again since we did private gc's in between.	*/
  HasRefs[objNameArray] = true;
    /* We look at these for the shared case.		*/
  segmentCache = &nilSegment;
    /* Prepare to use GCInternal_GetSegHnd procedure.	*/
  
  /* This is the main loop of the trace algorithm. It pops an	*/
  /* object ptr off the stack and processes it. PObjects on the	*/
  /* GC stack have the following characteristics:		*/
  /*   1. They don't necessarilly point into PS VM. They could	*/
  /*      point into C data space.				*/
  /*   2. Either the object itself or its body is in shared VM	*/
  /*      NameMap bodies can be in malloc storage, but the name	*/
  /*      entries they point to are in shared VM.		*/
  /*   3. The objects are always composites.			*/
  /*   4. The objects have not been processed yet. An obj may	*/
  /*      be marked as seen, but it is not a recursive ref. It	*/
  /*      happens for the first element in array bodies or if a	*/
  /*      getRootsProc pushes something more than once.		*/
  /*   5. TrickyDicts on the pending stack didn't come from a	*/
  /*	  private VM (which is good because you wouldn't know	*/
  /*	  which VM if they did). Since TrickyDicts refer to	*/
  /*	  private dicts, they can be ignored for tracing.	*/
  while ( (obj = GCInternal_Pop(pending)) != NIL )
    {
    if ((sH = GCInternal_GetSegHnd(obj, vmShared)) != NIL &&
        ! IsROMSegment(sH))
      obj->seen = seen;
      /* Set seen bit iff the object is in shared vm and not in a
         ROM segment. Otherwise we'll screw up future collections
	 in the private vm that the object does live in. */
#ifdef	GATHERSTATS
    gcData->maxStackDepth = os_max(
      (long int)gcData->maxStackDepth,
      (long int)(pending->nextFree - pending->base));
#endif	GATHERSTATS

    switch (obj->type) {
      case nullObj: 
      	if (obj->length == 1) break;
      case intObj: case realObj:
      case boolObj: case fontObj:
        CantHappen();
        break;
      case escObj:
        switch (obj->length) {
          case objMark: case objSave:
            CantHappen();
            break;

          case objCond: case objLock:
            {
            PGenericBody body = obj->val.genericval;
    
	    if (body->seen == seen)
	      break;
	    sH = GCInternal_GetSegHnd(body, vmShared);
	    Assert(sH != NIL);
	    body->seen = seen;
	    ABM_SetAllocated((RefAny)body, sH, (Card32)body->length);
	    break;
	    }	/* END OF objCond, objLock CASE			*/


          case objGState:
            {
            PGenericBody body = obj->val.genericval;
    
	    if (body->seen == seen)
	      break;
	    sH = GCInternal_GetSegHnd(body, vmShared);
	    Assert(sH != NIL);
	    if (IsROMSegment(sH)) break;
	    body->seen = seen;
	    ABM_SetAllocated((RefAny)body, sH, (Card32)body->length);
	    (*pushGStateItems)(obj, CAST(gcData, GC_Info));
	    break;
	    }	/* END OF objGState CASE			*/

	  case objNameArray:
	    {
	    register int length;
	    register PNameEntry *cur;
	    register PNameEntry ne;
	    register charptr strBody;
	    PNameArrayBody body = obj->val.namearrayval;

	    if (body->header.seen == seen)
	      break;
	    sH = GCInternal_GetSegHnd(body, vmShared);
	    
	    /* Only mark the name map if its in shared VM. It	*/
	    /* may be the name map of a private VM, meaning its	*/
	    /* in malloc storage.				*/
	    if (sH != NIL)
	      {
	      if (IsROMSegment(sH)) break;
	      ABM_SetAllocated((RefAny)body, sH, (Card32)body->header.length);
	      }
	    body->header.seen = seen;

	    for ( /* FOR EACH offset in name array DO		*/
	      cur = body->nmEntry, length = body->length;
	      length > 0; length--, cur++)
	      {
	      ne = *cur;
	      if (ne == NIL)
	        continue;
	      if (ne->seen == seen)
	        continue;
	      sH = GCInternal_GetSegHnd(ne, vmShared);
	      Assert(sH != NIL);
	      ne->seen = seen;
	      ABM_SetAllocated((RefAny)ne, sH, (Card32)sizeof(NameEntry));
	      strBody = ne->str;
	      sH = GCInternal_GetSegHnd(strBody, vmShared);
	      Assert(sH != NIL);
	      ABM_SetAllocated((RefAny)strBody, sH, (Card32)ne->strLen);
	      }	/* FOR EACH offset in name array DO		*/
	    break;
	    }	/* END OF objNameArray CASE			*/

	  default: CantHappen();
          }	/* END OF switch on escape type			*/
        break;	/* END OF escObj CASE				*/

      case nameObj:
      case cmdObj:
        /* NOTE: Since both cmd and name vals are PNameEntrys	*/
        /* we just pick up the value for either using nmval.	*/
        /* NOTE: The NameEntry seen bit isn't just an optim-	*/
        /* zation. It's needed for name cache finalization.	*/
        {
        register PNameEntry ne;
	register charptr strBody;

	ne = obj->val.nmval;
	if (ne->seen == seen)
	  break;
	sH = GCInternal_GetSegHnd(ne, vmShared);
	Assert(sH != NIL &&
	       ! IsROMSegment(sH)); /* NameEntries are always in RAM */
	ne->seen = seen;
	ABM_SetAllocated((RefAny)ne, sH, (Card32)(sizeof(NameEntry)));
	strBody = ne->str;
	sH = GCInternal_GetSegHnd(strBody, vmShared);
	Assert(sH != NIL);
	if (! IsROMSegment(sH))
	  ABM_SetAllocated((RefAny)strBody, sH, (Card32)(ne->strLen));
#ifdef	GATHERSTATS
	gcData->names++;
#endif	GATHERSTATS
        break;
        }	/* END OF nameObj, cmdObj CASE			*/

      case stmObj:
        {
        register PStmBody sb = obj->val.stmval;
        if (sb->seen == seen)
	  break;
        sH = GCInternal_GetSegHnd(sb, vmShared);
        Assert(sH != NIL &&
	       ! IsROMSegment(sH)); /* StmBodies are always in RAM */
	sb->seen = seen;
	ABM_SetAllocated((RefAny)sb, sH, (Card32)(sizeof(StmBody)));
	(*pushStmItems)(obj, CAST(gcData, GC_Info));
	break;
        }		/* END OF stmObj CASE			*/

      case strObj:
	{
	register charptr strBody = obj->val.strval;

	if (strBody == NIL)
	  break;
	sH = GCInternal_GetSegHnd(strBody, vmShared);
	Assert(sH != NIL);
	if (! IsROMSegment(sH))
	  ABM_SetAllocated((RefAny)strBody, sH, (Card32)(obj->length));
	break;
	}	/* END OF strObj CASE				*/

      case dictObj:
	{
	PDictBody db;
	register PKeyVal kv, last;
	
	if (TrickyDict(obj))
	  break;
	db = obj->val.dictval;
	/* Handle Dictbody. If it has been seen before, no need	*/
	/* to go on. If not, mark it seen and allocated.	*/
	if (db->seen == seen)
	  break;
	sH = GCInternal_GetSegHnd(db, vmShared);
	Assert(sH != NIL);
	if (IsROMSegment(sH)) break;
	db->seen = seen;
	ABM_SetAllocated((RefAny)db, sH, (Card32)(sizeof(DictBody)));

	/* Find the segment containing the entries and mark the	*/
	/* space that they use as allocated.			*/
	kv = db->begin;
	last = db->end;
	sH = GCInternal_GetSegHnd(kv, vmShared);
	Assert(sH != NIL &&
	       ! IsROMSegment(sH)); /* KeyVal array cannot be in ROM if
				       DictBody was in RAM */
	ABM_SetAllocated((RefAny)kv, sH, (Card32)(db->size * sizeof(KeyVal)));

	/* Process the dictionary elements themselves...	*/
	for (; kv < last; kv++)
	  {
	  /* First process the key for the entry...		*/
	  if (ContainsRefs(&(kv->key)))
	    {GCInternal_PushNoTest(pending, &(kv->key));}

	  /* Now process the value for the entry...		*/
	  if (ContainsRefs(&(kv->value)))
	    {GCInternal_PushNoTest(pending, &(kv->value));}
	  }
#ifdef	GATHERSTATS
	gcData->dicts++;
#endif	GATHERSTATS
	break;
	}	/* END OF dictObj CASE				*/

      case arrayObj:
	/* SEE NOTES ABOUT ARRAY OBJECTS AT HEAD OF MODULE	*/
	{
	register cardinal len = obj->length;
	register PObject arrayEntry = obj->val.arrayval;

        if (len == 0)
          break;	/* That was certainly easy		*/
	sH = GCInternal_GetSegHnd(arrayEntry, vmShared);
	Assert(sH != NIL);
	if (IsROMSegment(sH)) break;
	ABM_SetAllocated(
	  (RefAny)arrayEntry, sH, (Card32)(len * sizeof(Object)));
	for (; len > 0; len--, arrayEntry++)
	  {
	  if (ContainsRefs(arrayEntry) && arrayEntry->seen != seen)
	    {
	    GCInternal_PushNoTest(pending, arrayEntry);
	    arrayEntry->seen = seen;
	    }
	  }
#ifdef	GATHERSTATS
        gcData->arrays++;
#endif	GATHERSTATS
	break;
	}	/* END OF arrayObj CASE				*/

      case pkdaryObj:
	{
	register Card32 paSize;
	register PCard8 aryBody = obj->val.pkdaryval;

	if (obj->length == 0)	/* This is easy...		*/
	  break;
	/* There's no way to tell if we've seen a pkd ary body	*/
	/* before, so just get the segment handle and proceed.	*/
	sH = GCInternal_GetSegHnd(aryBody, vmShared);
	Assert(sH != NIL);
	if (IsROMSegment(sH)) break;
	paSize = (*pushPkdAryComposites)(obj, CAST(gcData, GC_Info));
	ABM_SetAllocated((RefAny)aryBody, sH, (Card32)(paSize));
#ifdef	GATHERSTATS
        gcData->pkdArys++;
#endif	GATHERSTATS
	break;
	}	/* END OF pkdaryObj CASE			*/

      default: CantHappen();
      }	/* END OF switch on object type				*/
    }	/* END OF while there are more roots to process...	*/
  GCInternal_FreeStack(pending);
}	/* END OF TraceSharedVM	*/

private procedure TracePrivateForShared(space, sharedStack, localStack)
/* This is almost the same as a normal private VM trace	*/
/* except that when we find a ref to shared VM,	instead	*/
/* of ignoring it, we push it on the shared stack.	*/
VMStructure *space;
GC_PStack sharedStack;
GC_PStack localStack;
{
  register PVMSegment sH;
  register Object *obj;
  boolean seen = space->valueForSeen;
  GC_PData gcData = space->priv->gcData;
  GenericID me, curCtxt;
  CallBackItem *cur;

  gcData->type = privateForShared;
  gcData->stack = localStack;

  segmentCache = &nilSegment;
    /* Prepare for use of the GetSegHnd procedure.	*/

  ABM_ClearAll(space);	/* Clear all allocation bitmaps	*/
  
  GCInternal_PushNoTest(localStack, (PObject)NIL);
    /* This is a sentinal item so we know when we have	*/
    /* looked at all the valid items on the GC stack.	*/

  /* Gather the roots of the collection...		*/
  /* For each ctxt, call the getRootsProc's...		*/
  me = Ctxt_GetCurrentCtxt();
  for (
    curCtxt = Ctxt_GetNextCtxt(space, Ctxt_NIL);
    curCtxt.stamp != Ctxt_NIL.stamp;
    curCtxt = Ctxt_GetNextCtxt(space, curCtxt))
    {
    Ctxt_SetCurrentCtxt(curCtxt);
    for (cur = getRootsProcs; cur != NIL; cur = cur->next)
      (*(cur->proc))(cur->clientData, CAST(gcData, GC_Info));
    }
  /* We want to perform this trace as if we were some ctxt in	*/
  /* this space. Since we're already set to the "last" ctxt,	*/
  /* we'll use it. At the end of the proc, we'll reset to the	*/
  /* real context (it's stored in the variable 'me').		*/

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
  while ( (obj = GCInternal_Pop(localStack)) != NIL )
    {
    obj->seen = seen;
#ifdef	GATHERSTATS
    gcData->maxStackDepth = os_max(
      (long int)gcData->maxStackDepth,
      (long int)(localStack->nextFree - localStack->base));
#endif	GATHERSTATS

    switch (obj->type) {
      case nullObj: case intObj: case realObj:
      case boolObj: case fontObj:
        CantHappen();
        break;
      case escObj:
        switch (obj->length) {
          case objMark: case objSave:
            CantHappen();
            break;

          case objCond: case objLock:
            {
            PGenericBody body = obj->val.genericval;
    
	    if ((sH = GCInternal_GetSegHnd(body, space)) == NIL)
	      {GCInternal_PushNoTest(sharedStack, obj);}
	    else
	      {
	      if (body->seen == seen)
	        break;
	      body->seen = seen;
	      ABM_SetAllocated((RefAny)body, sH, (Card32)(body->length));
	      }
	    break;
	    }	/* END OF objCond, objLock CASE			*/

	  case objNameArray:
	    {
	    GCInternal_PushNoTest(sharedStack, obj);
	      /* The body is in malloc storage, so it needn't	*/
	      /* be marked, but the items in the name map must	*/
	      /* be traced as part of a shared collection.	*/
	    break;
	    }	/* END OF objNameArray CASE			*/

          case objGState:
            {
            PGenericBody body = obj->val.genericval;
    
	    if ((sH = GCInternal_GetSegHnd(body, space)) == NIL)
	      {GCInternal_PushNoTest(sharedStack, obj);}
	    else
	      {
	      if (body->seen == seen)
	        break;
	      body->seen = seen;
	      ABM_SetAllocated((RefAny)body, sH, (Card32)(body->length));
	      (*pushGStateItems)(obj, CAST(gcData, GC_Info));
	      }
	    break;
	    }	/* END OF objGState CASE			*/

          }	/* END OF switch on escape type			*/
        break;	/* END OF escObj CASE				*/

      case nameObj:
      case cmdObj:
        /* Since we know that name objs point into shared vm we	*/
        /* just push this obj on the sharedStack and move on.	*/
        /* NOTE: Since both cmd and name vals are PNameEntrys	*/
        /* we just pick up the value for either using nmval.	*/
        {
        GCInternal_PushNoTest(sharedStack, obj);
#ifdef	GATHERSTATS
	gcData->names++;
#endif	GATHERSTATS
	break;
        }	/* END OF nameObj, cmdObj CASE			*/

      case stmObj:
        {
        register PStmBody sb = obj->val.stmval;
        if ((sH = GCInternal_GetSegHnd(sb, space)) == NIL)
	  {GCInternal_PushNoTest(sharedStack, obj);}
	else
	  {
          if (sb->seen == seen)
            break;
	  sb->seen = seen;
	  ABM_SetAllocated((RefAny)sb, sH, (Card32)(sizeof(StmBody)));
	  (*pushStmItems)(obj, CAST(gcData, GC_Info));
	  }
	break;
        }		/* END OF stmObj CASE			*/

      case strObj:
	{
	register charptr strBody = obj->val.strval;

	if (strBody == NIL)
	  break;
	if ( (sH = GCInternal_GetSegHnd(strBody, space)) == NIL)
	  {GCInternal_PushNoTest(sharedStack, obj);}
	else
	  ABM_SetAllocated((RefAny)strBody, sH, (Card32)(obj->length));
	break;
	}	/* END OF strObj CASE				*/

      case dictObj:
	{
	PDictBody db;
	register PKeyVal kv, last;

        obj = XlatDictRef(obj);
          /* If its a dict, it might be a trickyDict, Xlat it	*/
        db = obj->val.dictval;

	/* If the dictbody is in shared VM, push the object and	*/
	/* move along. Otherwise, process the body and entries.	*/
	if ((sH = GCInternal_GetSegHnd(db, space)) == NIL)
	  {
	  GCInternal_PushNoTest(sharedStack, obj);
	  break;
	  }

	if (db->seen == seen)	/* Already seen so move along	*/
	  break;
	/* Mark the dictbody seen and allocated...		*/
	db->seen = seen;
	ABM_SetAllocated((RefAny)db, sH, (Card32)(sizeof(DictBody)));

	/* Mark the space used by the entries as allocated...	*/
	kv = db->begin;
	last = db->end;
	sH = GCInternal_GetSegHnd(kv, space);
	ABM_SetAllocated((RefAny)kv, sH, (Card32)(db->size * sizeof(KeyVal)));
	
	/* Process the dictionary elements...			*/
	for (; kv < last; kv++)
	  {
	  /* First process the key for the entry...		*/
	  if (ContainsRefs(&(kv->key)))
	    {GCInternal_PushNoTest(localStack, &(kv->key));}
	    
	  /* Now process the value for the entry...		*/
	  if (ContainsRefs(&(kv->value)))
	    {GCInternal_PushNoTest(localStack, &(kv->value));}
	  }

#ifdef	GATHERSTATS
        gcData->dicts++;
#endif	GATHERSTATS
	break;
	}	/* END OF dictObj CASE				*/

      case arrayObj:
	/* SEE NOTES ABOUT ARRAY OBJECTS AT HEAD OF MODULE	*/
	{
	register cardinal len = obj->length;
	register PObject arrayEntry = obj->val.arrayval;

	if (len == 0)
	  break;
	if ((sH = GCInternal_GetSegHnd(arrayEntry, space)) == NIL)
	  {
	  GCInternal_PushNoTest(sharedStack, obj);
	  break;
	  }
	ABM_SetAllocated(
	  (RefAny)arrayEntry, sH, (Card32)(len * sizeof(Object)));
	/* Process the array entries themselves...		*/
	for (; len > 0; len--, arrayEntry++)
	  {
	  if (ContainsRefs(arrayEntry) && arrayEntry->seen != seen)
	    {
	    GCInternal_PushNoTest(localStack, arrayEntry);
	    arrayEntry->seen = seen;
	    }
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
	if ( (sH = GCInternal_GetSegHnd(aryBody, space)) == NIL)
	  {
	  GCInternal_PushNoTest(sharedStack, obj);
	  break;
	  }

	paSize = (*pushPkdAryComposites)(obj, CAST(gcData, GC_Info));
	ABM_SetAllocated((RefAny)aryBody, sH, (Card32)(paSize));
#ifdef	GATHERSTATS
	gcData->pkdArys++;
#endif	GATHERSTATS
	break;
	}	/* END OF arrayObj CASE				*/

      default: CantHappen();
      }	/* END OF switch on object type				*/
    }	/* END OF while there are more roots to process...	*/

  for (cur = gcFinalizeProcs; cur != NIL; cur = cur->next)
    (*(cur->proc))(cur->clientData, space->priv->gcData);

  Ctxt_SetCurrentCtxt(me);	/* Reset to real context	*/
}	/* END OF TracePrivateForShared	*/

public procedure GCInternal_TraceROMDict(info, obj)
  GC_Info info;
  PDictObj obj;
{
  PDictBody db;
  PVMSegment sH;
  register PKeyVal kv, last;

  Assert(obj->type == dictObj && obj->shared && ! TrickyDict(obj) &&
         GCDATA(info)->space == vmShared);
  db = obj->val.dictval;
  sH = GCInternal_GetSegHnd(db, vmShared);
  Assert(sH != NIL);

  /* assume that DictBody is in same type of segment as KeyVal array */
  if (IsROMSegment(sH))
    {
    /* set seen flag in DictObj only, not in DictBody or KeyVal array */
    obj->seen = vmShared->valueForSeen;

    /* trace only values; keys in ROM dict are permanent */
    for (kv = db->begin, last = db->end; kv < last; kv++)
      if (ContainsRefs(&(kv->value)))
        {GCInternal_PushNoTest(GCDATA(info)->stack, &(kv->value));}
    }
  else
    GC_Push(info, obj);
}

