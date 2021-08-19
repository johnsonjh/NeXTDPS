/*
				  gcmisc.c

	     Copyright (c) 1988, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE: All information contained  herein is  the property of   Adobe Systems
Incorporated.  Many of the    intellectual and technical concepts   contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their  internal  use.  Any reproduction
or dissemination of this software  is strictly forbidden unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Pasqua: Mon Feb 22 14:01:56 1988
Edit History:
Scott Byer: Wed May 17 14:48:01 1989
Ivor Durham: Sun May  7 12:30:23 1989
Joe Pasqua: Mon Jan  9 14:55:30 1989
Rick Saenz: Tuesday, July 5, 1988 10:23:02 AM
Ed Taft: Sun Dec 17 18:39:47 1989
Perry Caro: Fri Nov  4 15:59:32 1988
Jim Sandman: Tue Apr  4 17:28:24 1989
Mark Francis: Thu Nov  9 15:54:28 1989
Jack Newlin 22May90 added gc logging flagged by gcFd (for keyboard lights)
End Edit History.
*/

#include	PACKAGE_SPECS
#include	BASICTYPES
#include	ERROR
#include	EXCEPT
#include	GC
#include	ORPHANS
#include	PSLIB
#include	SIZES
#include	VM
#include	"abm.h"
#include	"gcinternal.h"
#include	"saverestore.h"
#include	"vm_segment.h"
#import	"sys/syslog.h"
#import <sys/file.h>

/*
IMPLEMENTATION NOTES:
o Names of procedures and types are prefixed by the name of the
  system they belong to (like an interface name). So a stack object
  defined for the garbage collector is GC_Stack. A pointer to the
  same is called GC_PStack.
o We use a pointer to a VMStructure structure to identify a PS VM.
  It is a VM Handle. Collections are relative to VM's, not threads,
  since there may be several threads per VM.
o The tag "TMP" is used in comments to mark things that should be fixed.
  The tag "NEED THIS" marks procedures that the collector needs.

TO DO: (in roughly decreasing order of importance)
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

/* #if (OS != os_mpw) */
/*-- BEGIN GLOBALS --*/
public VMSegment nilSegment;
  /* This VMSegment structure is basically used as a sentinel	*/
  /* in testing whether a ptr lies in a particular segment.	*/
public CallBackItem *getRootsProcs;
  /* Head of list of procs to call to enumerate roots. The list	*/
  /* is created at init time and used for all collections.	*/
public CallBackItem *sharedRootsProcs;
  /* Head of list of procs to call to enum shared roots. The	*/
  /* list is created at init time and used for all collections.	*/
public CallBackItem *gcFinalizeProcs;
  /* Head of list of procs to call to finalize objs. The list	*/
  /* is created at init time and used for all collections.	*/ 
public Card32 (*pushPkdAryComposites)();
public procedure (*pushStmItems)();
public procedure (*pushGStateItems)();
  /* Procedure to call to get the composites in an object's	*/
  /* value pushed on the pending stack, for objects of specific	*/
  /* types. These procedures are registered from other packages	*/
  /* at init time and is used for all collections.		*/

public Card32 lookups, cacheHits;
  /* These variables contains statistics about how many refs we	*/
  /* map into segments, and how often the cache is hit in that	*/
  /* process. These vars. are global across all collections.	*/

public PVMSegment segmentCache;
  /* One element cache of the last segment to contain a ref.	*/
  /* Data in the cache does not persist between collections.	*/

public boolean autoShared, autoPrivate;
  /* When true, automatic collections are enabled, when false,	*/
  /* automatic collections are disabled. This is not context	*/
  /* specific information (even the autoPrivate variable).	*/

public boolean *HasRefs, *PrivTraceInfo;
  /* This data allows the collector to determine whether or not	*/
  /* to trace an object based on its type. This is not context	*/
  /* specific information.					*/
/*-- END GLOBALS --*/
/* #endif (OS != os_mpw) */

public int gcFd;

/*----------------------------------------------*/
/*	PRIVATE INLINE Procedures		*/
/*----------------------------------------------*/

/*----------------------------------------------*/
/*		PRIVATE GC DATA			*/
/*----------------------------------------------*/

/*----------------------------------------------*/
/*		FORWARD Declarations		*/
/*----------------------------------------------*/

extern procedure PSCollect();
extern procedure PSSetThresh();
private procedure PSGCStats();
private procedure AddProcToList();
private procedure InitRefArrays();
private procedure PSSetGCLog();

/* The following procedures are in the language	*/
/* package, but can not be imported due to the	*/
/* interface circularity they would cause.	*/
extern procedure NameIndexObj();
extern integer PSPopInteger();

/*----------------------------------------------*/
/*		EXPORTED GC Procedures		*/
/*----------------------------------------------*/

GC_CollectionType GC_GetCollectionType(info)
GC_Info info;
{
  return(GCDATA(info)->type);
}

VMStructure *GC_GetSpace(info)
GC_Info info;
{
  return(GCDATA(info)->space);
}

PVMRoot GC_GetRoot(info)
GC_Info info;
{
  return(RootPointer(GCDATA(info)->space));
}

procedure GCInternal_MarkAllocated(info, addr, size)
GC_Info info;
RefAny addr;
Card32 size;
{
  PVMSegment sH;
  
  sH = GCInternal_GetSegHnd(addr, GCDATA(info)->space);
  if (sH == NIL)
    CantHappen();
  ABM_SetAllocated(addr, sH, size);
}

procedure GC_Push(info, value)
/* Push an element on the GC's pending stack if	*/
/* it contains a ref. and its appr for the type	*/
/* of collection that is in progress.		*/
register GC_Info info;
register PObject value;
{
  register GC_PStack stack = GCDATA(info)->stack;
  register PObject origPtr = value;
  Object tmp;	/* Must be declared in this scope, its address	*/
  		/* is taken & used outside the local scope.	*/

#if	(MINALIGN != 1)
  if ( ((integer)value & (MINALIGN-1)) != 0)	/* It's unaligned	*/
    {
    value = &tmp;
    os_bcopy((char *)origPtr, (char *)value, sizeof(Object));
    }
#endif	(MINALIGN != 1)
  if (!ContainsRefs(value) ||
    value->shared && GCDATA(info)->type == privateVM)
    return;
  if (stack->nextFree == stack->limit) GCInternal_GrowStack(stack);
  *(stack->nextFree++) = origPtr;
}

procedure GC_RgstGetRootsProc(proc, clientData)
procedure (*proc)();
RefAny clientData;
{
  AddProcToList(&getRootsProcs, proc, clientData);
}

procedure GC_RgstSharedRootsProc(proc, clientData)
procedure (*proc)();
RefAny clientData;
{
  AddProcToList(&sharedRootsProcs, proc, clientData);
}

procedure GC_RegisterFinalizeProc(proc, clientData)
procedure (*proc)();
RefAny clientData;
{
  AddProcToList(&gcFinalizeProcs, proc, clientData);
}

procedure GC_RgstPkdAryEnumerator(proc)
Card32 (*proc)();
{
  pushPkdAryComposites = proc;
}

procedure GC_RgstStmEnumerator(proc)
procedure (*proc)();
{
  pushStmItems = proc;
}

procedure GC_RgstGStateEnumerator(proc)
procedure (*proc)();
{
  pushGStateItems = proc;
}

boolean GC_WasCollected(obj, info)
PObject obj;
GC_Info info;
{
  boolean collected = false;
  boolean seen = GCDATA(info)->space->valueForSeen;
  PVMSegment sH;
  
  if (ContainsRefs(obj))
    {
    switch (obj->type) {
      case stmObj:
        collected = (obj->val.stmval->seen != seen);
        break;		/* END OF stmObj CASE			*/
      case nameObj:
      case cmdObj:
        collected = (obj->val.nmval->seen != seen);
        break;		/* END OF nameObj, cmdObj CASE		*/
      case escObj:
        switch (obj->length) {
          case objNameArray:
            collected = (obj->val.namearrayval->header.seen != seen);
            break;	/* END OF objNameArray CASE		*/
          case objGState:
            collected = (obj->val.genericval->seen != seen);
            break;	/* END OF objGState CASE		*/
          }		/* END OF switch on escape type		*/
        break;		/* END OF escObj CASE			*/
      case strObj:
        collected = false;	/* Can't tell really!		*/
	break;		/* END OF strObj CASE			*/
      case dictObj:
	collected = (XlatDictRef(obj)->val.dictval->seen != seen);
	break;		/* END OF dictObj CASE			*/
      case arrayObj:
	collected = (obj->val.arrayval->seen != seen);
	break;		/* END OF arrayObj CASE			*/
      case pkdaryObj:
        collected = false;	/* Can't tell really!		*/
	break;		/* END OF arrayObj CASE			*/
      default: CantHappen();
      }	/* END OF switch on object type				*/

    /* Ensure that objects in permanent segments are never collected */
    if (collected && GCDATA(info)->space->shared)
      {
      sH = GCInternal_GetSegHnd(obj->val.strval, GCDATA(info)->space);
      Assert(sH != NIL)
      if (sH->allocBM == NIL) collected = false;
      }
    }

  return(collected);
}

boolean GC_WasNECollected(ne, info)
PNameEntry ne;
GC_Info info;
{
  PVMSegment sH;
  if (ne->seen == GCDATA(info)->space->valueForSeen) return false;
  DebugAssert(GCDATA(info)->space->shared); /* names are always shared */

  /* Ensure that objects in permanent segments are never collected */
  sH = GCInternal_GetSegHnd(ne, GCDATA(info)->space);
  Assert(sH != NIL);
  return (sH->allocBM != NIL);
}

procedure GC_HandleIndex(index, indexType, info)
cardinal index;
cardinal indexType;
GC_Info info;
{
  PVMSegment sH;
  register PNameEntry ne;
  register charptr strBody;

  if (GCDATA(info)->type == privateVM)
    return;

  /* Reconstitute an object from the index spec'd. Get	*/
  /* the assoc'd NameEntry & if we've seen it, return.	*/
  {
  Object obj;
  if (indexType == 0)
    NameIndexObj(index, &obj);
  else
    CmdIndexObj(index, &obj);
  ne = obj.val.nmval;
  if (ne->seen == vmShared->valueForSeen) return;
  }

  /* Set NameEntry's seen bit and mark as allocated...	*/
  sH = GCInternal_GetSegHnd(ne, vmShared);
  Assert(sH != NIL);
  ne->seen = vmShared->valueForSeen;
  ABM_SetAllocated((RefAny)ne, sH, (Card32)sizeof(NameEntry));

  /* Mark the string body as allocated...		*/
  strBody = ne->str;
  sH = GCInternal_GetSegHnd(strBody, vmShared);
  Assert(sH != NIL);
  ABM_SetAllocated((RefAny)strBody, sH, (Card32)ne->strLen);
  
#ifdef	GATHERSTATS
  GCDATA(info)->names++;
#endif	GATHERSTATS
}

/*----------------------------------------------*/
/*	LOCALLY EXPORTED GC Procedures		*/
/* For use by other vm package modules only.	*/ 
/*----------------------------------------------*/

procedure GCInternal_Init(reason)
InitReason reason;
{
  switch (reason) {
    case init:		/* FIRST initialization step	*/
      nilSegment.first = nilSegment.last = NIL;
      pushPkdAryComposites = NIL;
      pushGStateItems = NIL;
      getRootsProcs = NIL;
      sharedRootsProcs = NIL;
      gcFinalizeProcs = NIL;
      lookups = cacheHits = 0;
      autoShared = false;
      autoPrivate = false;
      gcFd = -1;
      InitRefArrays();
      break;
    case romreg:	/* SECOND initialization step	*/
#if	STAGE==DEVELOP
      if (vSTAGE==DEVELOP) {
	RgstExplicit ("gcstats", PSGCStats);
      }
#endif	STAGE==DEVELOP
      RgstExplicit ("setgclog", PSSetGCLog);
      break;
    case ramreg:	/* THIRD initialization step	*/
      break;
    default:
      CantHappen();
    }
}

procedure GCInternal_VMChange(space, changeType)
/* This procedure is called by the VM package	*/
/* any time a space is created or deleted. This	*/
/* gives the collector a chance to handle its	*/
/* associated data structures.			*/
VMStructure *space;
VMChangeType changeType;
{
  GC_PData gcData;

  switch (changeType) {
    case creating:
      DURING
        {
        space->collectThisSpace = false;
        space->allocCounter = 0;
        space->allocThreshold = ps_getsize(
          SIZE_GC_DEF_THRESHOLD, OEM_DefaultThreshold);
        gcData = CAST(NEW(1, sizeof(GC_Data)), GC_PData);
        space->priv->gcData = gcData;
	gcData->space = space;
        gcData->maxStackDepth = 0;
        gcData->arrays = 0;
        gcData->dicts = 0;
        gcData->pkdArys = 0;
        gcData->names = 0;
        }
      HANDLER
        {
	VMERROR();
	}
      END_HANDLER;
      break;
    case deleting:
      FREE(space->priv->gcData);
      break;
    default:
      CantHappen();
    }
}

GC_Stack *GCInternal_AllocStack(initSize)
/* Alloc a stack obj and a stack of the spec'd	*/
/* initial size (size in entries, not bytes).	*/
Card16 initSize;
{
  GC_PStack retVal = NIL;
  
  /* NOTE: We do 2 allocs here for a reason. If	*/
  /* we alloc'd the stack object and its space	*/
  /* with one allocation, we couldn't grow the	*/
  /* stack space independently (which we must).	*/
  DURING
    {
    retVal = CAST(NEW(1, sizeof(GC_Stack)), GC_PStack);
    retVal->base = CAST(NEW(1, initSize * sizeof(PObject)), PObject *);
    retVal->nextFree = retVal->base;
    retVal->limit = retVal->base + initSize;
    }
  HANDLER
    {
    if (retVal != NIL) FREE(retVal);
    VMERROR();
    }
  END_HANDLER;
  return(retVal);
} 

#if	(STAGE==DEVELOP || MINALIGN != 1)
private Object popTemporary;
PObject GCInternal_Pop(stack)
/* Pop an element from the spec'd GC_Stack.	*/
/* Does not check for underflow! Assumes that	*/
/* the first item pushed has some distinguished	*/
/* val used to denote no more entries, e.g. NIL	*/
GC_PStack stack;
{
#if	(MINALIGN != 1)
  register PObject stackPtr = *(--stack->nextFree);
  
  if ( ((integer)stackPtr & (MINALIGN-1)) != 0)
    {
    os_bcopy((char *)stackPtr, (char *)&popTemporary, sizeof(Object));
    return(&popTemporary);
    }
  else
    return(stackPtr);
#else	(MINALIGN != 1)
  return(*(--stack->nextFree));
#endif	(MINALIGN != 1)
}
#endif	(STAGE==DEVELOP || MINALIGN != 1)

#if	(STAGE==DEVELOP)
procedure GCInternal_PushNoTest(stack, value)
/* Push an element on the GC's pending stack.	*/
/* Assume it hasn't been seen yet. Don't Assert	*/
/* this since the first obj in an array body is	*/
/* marked as seen before it is pushed (see the	*/
/* array case in TracePrivateVM for details).	*/
GC_PStack stack;
PObject value;
{
  if (stack->nextFree == stack->limit) GCInternal_GrowStack(stack);
  *(stack->nextFree++) = value;
}
#endif	STAGE==DEVELOP

GC_Stack *GCInternal_FreeStack(stack)
/* Free the storage occupied by a GC_Stack	*/
GC_Stack *stack;
{
  FREE(stack->base);
  FREE(stack);
  return(NIL);
}


/*----------------------------------------------*/
/*		Private GC Procedures		*/
/*----------------------------------------------*/

private procedure PSSetGCLog()
{
  if (PSPopBoolean()) { /* turn on if not */
    if (gcFd < 0)
      gcFd = open("/dev/console", O_WRONLY, 0);
      openlog("WindowServer", 0, LOG_DAEMON);
  } else if (gcFd > 0) {/* turn off if on */
      close(gcFd);
      closelog();
      gcFd = -1;
  }
}

public procedure PSSetThresh()
  {
  integer new = PSPopInteger();
  int maxVal, minVal, defVal;

  if (CurrentShared())
    return;
  defVal = ps_getsize(SIZE_GC_DEF_THRESHOLD, OEM_DefaultThreshold);
  minVal = ps_getsize(SIZE_GC_MIN_THRESHOLD, OEM_MinThreshold);
  maxVal = ps_getsize(SIZE_GC_MAX_THRESHOLD, OEM_MaxThreshold);
  if (new == -1)
    {
    vmCurrent->allocThreshold = defVal;
    return;
    }
  if (new < 0) RangeCheck();
  else if (new < minVal) new = minVal;
  else if (new > maxVal) new = maxVal;
  vmCurrent->allocThreshold = new;
  }

public procedure PSCollect()
{
  integer operation = PSPopInteger();
  
  switch (operation) {
    case -2:	/* Disable all automatic collections	*/
      autoShared = false;
      autoPrivate = false;
      break;
    case -1:	/* Disable automatic priv. collections	*/
      autoPrivate = false;
      break;
    case  0:	/* Enable automatic collection		*/
      autoShared = true;
      autoPrivate = true;
      break;
    case  1:	/* perform immediate private collection	*/
      GC_CollectPrivate(vmPrivate);
      break;
    case  2:	/* Perform immediate shared collection	*/
      GC_CollectShared();
      break;
    default:
      RangeCheck();
    }
}

#if	STAGE==DEVELOP
private procedure PSGCStats()
{
  register GC_PData gcData = vmCurrent->priv->gcData;
  
  os_printf("\nGarbage Collection Statistics:\n");
  os_printf("Max Stack Depth: %d\n", gcData->maxStackDepth);
  os_printf("Total number of arrays processed: %d\n", gcData->arrays);
  os_printf("Total number of dicts processed: %d\n", gcData->dicts);
  os_printf("Total number of pkdArys processed: %d\n", gcData->pkdArys);
  os_printf("Total number of names processed: %d\n", gcData->names);
  os_printf("Total number of lookups: %d\n", lookups);
  os_printf("Total number of cache hits: %d\n", cacheHits);
}
#endif	STAGE==DEVELOP


/*----------------------------------------------*/
/*	Internal GC Support Procedures		*/
/*----------------------------------------------*/

#define	StackGrowInc	50

procedure GCInternal_GrowStack(stack)
/* Handle stack overflow by allocating a larger stack	*/
/* area, copying the data and adjusting the stack obj.	*/
GC_PStack stack;
{
  PObject *oldBase = stack->base;
  Card32 offsetOfNextFree = stack->nextFree - stack->base;
  int newSize = stack->limit - stack->base + StackGrowInc;
  
  DURING
    {
    stack->base = CAST(NEW(1, newSize * sizeof(PObject)), PObject *);
    }
  HANDLER
    {
    VMERROR();
    }
  END_HANDLER;
  os_bcopy(
    (char *)oldBase, (char *)stack->base,
    (long int)(offsetOfNextFree * sizeof(PObject *)));
  stack->limit = stack->base + newSize;
  stack->nextFree = stack->base + offsetOfNextFree;
  FREE(oldBase);
}

public PVMSegment GCInternal_LookupSegment(ref, space)
  register RefAny ref;
  VMStructure *space;
  /* Return the segment in space that contains the addr	*/
  /* ref. If ref is not within the space, return NIL.	*/
  {
  register PVMSegment segment;

  for (segment = space->current; segment != NIL; segment = segment->pred)
    {
    if (InSegment(ref, segment))
      return(segmentCache = segment);
    }

  for (
    segment = space->current->succ;
    segment != NIL; segment = segment->succ)
    {
    if (InSegment(ref, segment))
      return(segmentCache = segment);
    }

  return(NIL);
  }

private procedure AddProcToList(list, proc, clientData)
/* Add a CallBackItem to the spec'd list of callbacks.	*/
CallBackItem **list;
procedure (*proc)();
RefAny clientData;
{
  register CallBackItem *new;
  register PVMSegment seg;

  DURING
    {
    new = CAST(NEW(1, sizeof(CallBackItem)), CallBackItem *);
    new->proc = proc;
    new->clientData = clientData;
    new->next = *list;
    *list = new;
    }
  HANDLER
    {
    VMERROR();
    }
  END_HANDLER;
}

private procedure InitRefArrays()
  {
  HasRefs = (boolean *)os_malloc((long int)(sizeof(boolean) * nObTypes * 2));
  if (HasRefs == NULL) CantHappen();
  PrivTraceInfo = HasRefs + nObTypes;
  HasRefs[nullObj] = PrivTraceInfo[nullObj] = false;
  HasRefs[intObj] = PrivTraceInfo[intObj] = false;
  HasRefs[realObj] = PrivTraceInfo[realObj] = false;
  HasRefs[nameObj] = PrivTraceInfo[nameObj] = false;
  HasRefs[boolObj] = PrivTraceInfo[boolObj] = false;
  HasRefs[strObj] = PrivTraceInfo[strObj] = true;
  HasRefs[stmObj] = PrivTraceInfo[stmObj] = true;
  HasRefs[cmdObj] = PrivTraceInfo[cmdObj] = true;
  HasRefs[dictObj] = PrivTraceInfo[dictObj] = true;
  HasRefs[arrayObj] = PrivTraceInfo[arrayObj] = true;
  HasRefs[fontObj] = PrivTraceInfo[fontObj] = false;
  HasRefs[pkdaryObj] = PrivTraceInfo[pkdaryObj] = true;
  HasRefs[objMark] = PrivTraceInfo[objMark] = false;
  HasRefs[objSave] = PrivTraceInfo[objSave] = false;
  HasRefs[objGState] = PrivTraceInfo[objGState] = true;
  HasRefs[objCond] = PrivTraceInfo[objCond] = true;	/*JP*/
  HasRefs[objLock] = PrivTraceInfo[objLock] = true;	/*JP*/
  HasRefs[objNameArray] = PrivTraceInfo[objNameArray] = false;
  }


