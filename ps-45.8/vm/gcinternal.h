/*
  gcinternal.h

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

Original version: 
Edit History:
Joe Pasqua: Mon Jan  9 13:05:54 1989
Ivor Durham: Sun Oct 30 13:14:26 1988
Perry Caro: Fri Nov  4 15:26:46 1988
Ed Taft: Tue Nov 28 09:53:16 1989
End Edit History.
*/

#ifndef	GCI_H
#define	GCI_H

#include PACKAGE_SPECS
#include GC
#include "vm_memory.h"
#include "vm_segment.h"

/*	----> TYPEs <----	*/

typedef struct {	/* Stack of Object Pointers...		*/
  PObject *base;	/* Pointer to base of stack		*/
  PObject *nextFree;	/* Next free spot: 1 past top of stack	*/
  PObject *limit;	/* Pointer to end of the stack space	*/
  } GC_Stack, *GC_PStack;

typedef struct _t_GC_Data {/* Garbage collector per space data	*/
  GC_CollectionType type;	/* Type of trace going on now	*/
  VMStructure *space;	/* Name of space being collected now	*/
  GC_PStack stack;	/* Name of stack being used now		*/
  Card16 maxStackDepth;	/* Saves high water mark of pending stk	*/
  Card16 arrays;	/* Total number of arrays processed	*/
  Card16 dicts;		/* Total number of dicts processed	*/
  Card16 pkdArys;	/* Total number of pkdArys processed	*/
  Card16 names;		/* Total number of names processed	*/
  } GC_Data;

typedef struct callBackItem {	/* Callback proc list	*/
  RefAny clientData;		/* Client supplied data	*/
  procedure (*proc)();		/* The callback proc	*/
  struct callBackItem *next;	/* The next list item	*/
  } CallBackItem;

typedef enum {creating, deleting} VMChangeType;

/*	----> CONSTANTS <----	*/
#if	(STAGE == DEVELOP)
#define	GATHERSTATS	1
#endif	(STAGE == DEVELOP)

/* Default values for items in the size interface	*/
#define	OEM_DefaultThreshold	40000	/* ARBITRARY	*/
#define	OEM_MinThreshold	8192	/* ARBITRARY	*/
#define	OEM_MaxThreshold	500000	/* ARBITRARY	*/
#define	OEM_SharedThreshold	80000	/* ARBITRARY	*/
#define	OEM_OversizeThreshold	500000	/* ARBITRARY	*/

/*	----> PROCEDURES <----	*/

extern PVMSegment GCInternal_LookupSegment(
  /* RefAny ref, PVM space	*/);
  /* Returns the segment in space containing ref. If	*/
  /* ref is not in the given space, NIL is returned.	*/

extern procedure GCInternal_ResetFreePointer(/* PVM vm */);
  /* Called when a collection is complete to move back	*/
  /* the current segment's free pointer if possible.	*/

extern procedure GCInternal_Init(/* InitReason reason */);
  /* Called to initialize the Garbage Collector. Must be called	*/
  /* before any collections are requested.			*/

extern procedure GCInternal_VMChange(
  /* VMStructure *space, VMChangeType changeType */);
  /* Called by the vm pkg when it creates a new VM or deletes	*/
  /* an old one. This gives the collector a chance to deal with	*/
  /* (allocate, initialize, free) its per vm data.		*/

extern GC_Stack *GCInternal_AllocStack(/* Card16 initSize */);
  /* Alloc a stack obj and a stack of the spec'd	*/
  /* initial size (size in entries, not bytes).		*/

extern procedure GCInternal_GrowStack(/* GC_PStack stack */);
  /* Called to extend an existing stack			*/

extern GC_Stack *GCInternal_FreeStack(/* GC_Stack *stack */);
  /* Free the storage occupied by a GC_Stack. Always	*/
  /* return NIL.					*/

#if	((STAGE==DEVELOP) || (MINALIGN != 1))
extern PObject GCInternal_Pop(/* GC_PStack */);
  /* Pop an Object pointer from the GC stack.		*/
#endif	((STAGE==DEVELOP) || (MINALIGN != 1))

#if	STAGE==DEVELOP
extern procedure GCInternal_PushNoTest( /* GC_PStack stk, PObject val */);
  /* Push an object on the spec'd stack without doing	*/
  /* any tests for suitability on the object.		*/ 
#endif	STAGE==DEVELOP

extern procedure GCInternal_TraceROMDict(/* GC_Info info, PDictObj obj */);
/* Trace a shared dictionary whose value might be in ROM (e.g., systemdict).
   This procedure enumerates the contents of the dictionary, which
   would otherwise be ignored due to being in ROM.
   Note: the DictObj itself is assumed to be in RAM.
 */

/*	----> EXPORTed DATA <----	*/
/* Exported between modules of the	*/
/* garbage collector implementation.	*/

extern boolean *HasRefs;
  /* This array is indexed by PS object type. The value of	*/
  /* an entry says whether that type needs further tracing.	*/
extern CallBackItem *getRootsProcs;
  /* Head of list of procs to call to enumerate roots. The list	*/
  /* is created at init time and used for all collections.	*/
extern CallBackItem *sharedRootsProcs;
  /* Head of list of procs to call to enum shared roots. The	*/
  /* list is created at init time and used for all collections.	*/
extern CallBackItem *gcFinalizeProcs;
  /* Head of list of procs to call to finalize objs. The list	*/
  /* is created at init time and used for all collections.	*/ 
extern Card32 (*pushPkdAryComposites)();
extern procedure (*pushStmItems)();
extern procedure (*pushGStateItems)();
  /* Procedure to call to get the composites in an object's	*/
  /* value pushed on the pending stack, for objects of specific	*/
  /* types. These procedures are registered from other packages	*/
  /* at init time and is used for all collections.		*/

extern boolean autoShared, autoPrivate;
  /* When true, automatic collections are enabled, when false,	*/
  /* automatic collections are disabled. This is not context	*/
  /* specific information (even the autoPrivate variable).	*/

extern Card32 lookups, cacheHits;
  /* These variables contains statistics about how many refs we	*/
  /* map into segments, and how often the cache is hit in that	*/
  /* process. These vars. are global across all collections.	*/

extern PVMSegment segmentCache;
  /* One element cache of the last segment to contain a ref.	*/
  /* Data in the cache does not persist between collections.	*/

extern VMSegment nilSegment;
  /* This VMSegment structure is basically used as a sentinel	*/
  /* in testing whether a ptr lies in a particular segment.	*/

/*	----- Inline Procedures -----	*/

#define	CAST(value, type)	((type)(value))

#define	GCDATA(info)	(CAST(info, GC_PData))

#define ContainsRefs(obj)       \
  (HasRefs[((obj)->type == escObj) ? (obj)->length : (obj)->type])

#define	InSegment(ref, seg)				\
  (CAST(ref, RefAny) >= (seg)->first && CAST(ref, RefAny) <= (seg)->last)

#define IsROMSegment(seg) \
  ((seg)->level == stROM && (seg)->allocBM == NIL)
  
#ifdef	GATHERSTATS
#define	GCInternal_GetSegHnd(ref, space)		\
  ( (lookups++, InSegment((ref), segmentCache)) ? 	\
    (cacheHits++, segmentCache) : 			\
    GCInternal_LookupSegment((RefAny)(ref), (space)))
#else	GATHERSTATS
#define	GCInternal_GetSegHnd(ref, space)		\
  (InSegment((ref), segmentCache) ? 			\
    segmentCache : 					\
    GCInternal_LookupSegment((ref), (space)))
#endif	GATHERSTATS

#if	(STAGE!=DEVELOP && MINALIGN == 1)
#define	GCInternal_Pop(stack)	(*(--(stack)->nextFree))
#endif	(STAGE!=DEVELOP && MINALIGN == 1)

#if	STAGE!=DEVELOP
#define	GCInternal_PushNoTest(stack, value)	\
  if ((stack)->nextFree == (stack)->limit)	\
    GCInternal_GrowStack((stack));		\
  *((stack)->nextFree++) = value;
#endif	STAGE!=DEVELOP

/*	----> Ctxt PROCEDURES <-----		*/
/* These procedures are used by the collector	*/
/* and actually impl'd by the context machinery	*/

extern procedure Ctxt_StopAllSiblings();
  /* Prevent any siblings of the calling context from running	*/

extern procedure Ctxt_RestartAllSiblings();
  /* Make all siblings of the calling context ready to run	*/

extern procedure Ctxt_StopAllCtxts();
  /* Prevent any other context in the system from running	*/

extern procedure Ctxt_RestartAllCtxts();
  /* Make all contexts in the system ready to run		*/

extern GenericID Ctxt_GetCurrentCtxt();
  /* Return the ID of the currently executing context		*/

extern procedure Ctxt_SetCurrentCtxt(/* GenericID context */);
  /* Load package data for the specified context		*/

extern GenericID Ctxt_GetNextCtxt(/* PVM space, GenericID prev */);
  /* Stateless enumerator for the contexts in a given space.	*/
  /* The caller passes in the previous context and this proc.	*/
  /* returns the next context (start with NIL). The proc will	*/
  /* return NIL when all contexts have been enumerated.		*/

extern PVM Ctxt_GetNextSpace(/* PVM space */);
  /* Stateless enumerator for all of the spaces in the world.	*/
  /* The caller passes in the previous space and this proc.	*/
  /* returns the next space (start with NIL). The proc will	*/
  /* return NIL when all spaces have been enumerated.		*/

/*		----> Ctxt DATA <-----		*/

extern GenericID Ctxt_NIL;
  /* NIL value for a context identifier				*/

#endif	GCI_H
