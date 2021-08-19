/*
  recycler.c

Copyright (c) 1987, '88, '89 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ivor Durham: Sat May  6 17:24:42 1989
Edit History:
Scott Byer: Fri Jun  2 09:14:13 1989
Ivor Durham: Sun May 14 15:25:19 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ORPHANS
#include RECYCLER
#include VM

#define	TRACE_RECYCLER	((STAGE == DEVELOP) && !INLINE_RECYCLER)

/* Exported Recycler Data */

public PRecycler privateRecycler, sharedRecycler;
public PCard8  recycleType, recyclerContextHandle;

#if	TRACE_RECYCLER

#if	STAGE == DEVELOP
#include UNIXSTREAM
#endif	STAGE == DEVELOP

static boolean	traceRecycler;
static Stm	recyclerStm;	/* Valid only when traceRecycler is true */

static char *RecyclerName (R)
  PRecycler R;
{
  return (R == sharedRecycler ? "Shared " : "Private");
}

#endif	TRACE_RECYCLER

#if	STAGE == DEVELOP
private procedure PSRecyclerStatus()
 /*
   Print status in simple form to be parsed by testing software:
	RR: ref exec length vm-free
  */
{
  PRecycler R = vmCurrent->recycler;

  os_printf ("RR: %d %d %d %d\n", R->refCount, R->execSemaphore,
	     (R->end - R->begin), vmCurrent->free);
  fflush (os_stdout);
}
#endif	STAGE == DEVELOP

#if	TRACE_RECYCLER
private procedure PSTraceRecycler()
 /*
  * tracerecycler --
  *
  * Toggle recycler tracing.  Append trace log to file in current directory.
  */
{
  if (!traceRecycler) {
    recyclerStm = os_fopen ("recycler.log", "a");

    if (recyclerStm == NIL)
      PSUndefFileName();
    else {
      os_fprintf (recyclerStm, "---- Begin Recycler Log ----\n");
      fflush (recyclerStm);
      os_printf ("Recycler tracing on.\n");
    }
  } else {
      os_fprintf (recyclerStm, "---- End Recycler Log ----\n");
      fflush (recyclerStm);
      fclose (recyclerStm);
      os_printf ("Recycler tracing off.\n");
  }

  traceRecycler = !traceRecycler;
}
#endif	TRACE_RECYCLER

#if	!INLINE_RECYCLER
public procedure ExtendRecycler(R, endOfVM)
  PRecycler R;
  PCard8 endOfVM;
{
#if	TRACE_RECYCLER
  if (traceRecycler) {
    os_fprintf (recyclerStm, "%s %08x: Extended [%08x,%08x] to %d bytes\n",
		RecyclerName (R), R, R->begin, endOfVM, endOfVM - R->begin);
    fflush (recyclerStm);
  }
#endif	TRACE_RECYCLER

  _ExtendRecycler (R, endOfVM);
}
#endif	!INLINE_RECYCLER

#if	!INLINE_RECYCLER
public procedure ReclaimRecyclableVM()
{
#if	TRACE_RECYCLER
  PRecycler R = vmCurrent->recycler;

  if (traceRecycler &&
      ((R->refCount | R->execSemaphore) == 0)) {
    int     size = R->end - R->begin;

    if (size != 0)		/* Use != in case failure and < 0! */
      os_fprintf (recyclerStm, "%s %08x: Reclaimed %d bytes\n",
		  RecyclerName (R), R, size);
    fflush (recyclerStm);
  }
#endif	TRACE_RECYCLER

  _ReclaimRecyclableVM ();
}
#endif	!INLINE_RECYCLER

#if	!INLINE_RECYCLER
public boolean Recyclable(pObject)
  PObject pObject;
{
  boolean result = _Recyclable (pObject);

#if	false && TRACE_RECYCLER	/* Too much log output with this */
  {
    PRecycler R = RecyclerForObject (pObject);

    if (traceRecycler) {
      os_fprintf (recyclerStm, "%s %08x: Recyclable? %08x %s\n",
		  RecyclerName (R), R,
		  RecyclerAddress (pObject), (result ? "yes" : "no"));
      fflush (recyclerStm);
    }
  }
#endif	TRACE_RECYCLER

  return (result);
}
#endif	!INLINE_RECYCLER

#if	!INLINE_RECYCLER
public procedure RecyclerPop(pObject)
  PObject pObject;
{
#if	TRACE_RECYCLER
  PRecycler R = RecyclerForObject (pObject);
  int refCount = R->refCount;
#endif	TRACE_RECYCLER

  _RecyclerPop (pObject);

#if	TRACE_RECYCLER
  if (traceRecycler && (R->refCount < refCount)) {
    os_fprintf (recyclerStm, "%s %08x: Pop %08x %d\n", RecyclerName(R), R, RecyclerAddress (pObject), R->refCount);
    fflush (recyclerStm);
  }
#endif	TRACE_RECYCLER
}
#endif	!INLINE_RECYCLER

#if	!INLINE_RECYCLER
public procedure RecyclerPush(pObject)
  PObject pObject;
{
#if	TRACE_RECYCLER
  PRecycler R = RecyclerForObject (pObject);
  int refCount = R->refCount;
#endif	TRACE_RECYCLER

  _RecyclerPush (pObject);

#if	TRACE_RECYCLER
  if (traceRecycler && (R->refCount > refCount)) {
    os_fprintf (recyclerStm, "%s %08x: Push %08x %d\n", RecyclerName(R), R, RecyclerAddress (pObject), R->refCount);
    fflush (recyclerStm);
  }
#endif	TRACE_RECYCLER
}
#endif	!INLINE_RECYCLER

#if	!INLINE_RECYCLER
public procedure ResetRecycler(R)
  PRecycler R;
{
#if	TRACE_RECYCLER
  if (traceRecycler) {
    os_fprintf (recyclerStm, "%s %08x: Reset %08x\n",  RecyclerName (R), R, R->end);
    fflush (recyclerStm);
  }
#endif	TRACE_RECYCLER

  _ResetRecycler (R);
}
#endif	!INLINE_RECYCLER

#if	!INLINE_RECYCLER
public procedure ChangeRecyclerExecLevel(newLevel, deeper)
  int newLevel;
  boolean deeper;
{
  _ChangeRecyclerExecLevel (newLevel, deeper);

#if	TRACE_RECYCLER
  if (traceRecycler) {
    os_fprintf (recyclerStm, "Shared  %08x: Exec semaphore %d\n", sharedRecycler, sharedRecycler->execSemaphore);
    os_fprintf (recyclerStm, "Private %08x: Exec semaphore %d\n", privateRecycler, privateRecycler->execSemaphore);
    fflush (recyclerStm);
  }
#endif	TRACE_RECYCLER
}
#endif	!INLINE_RECYCLER

#if	TRACE_RECYCLER
private procedure TraceInvalidation (movedOK, R)
  boolean movedOK;
  PRecycler R;
{
  if (traceRecycler) {
    int     size = R->end - R->begin;

    if (movedOK)
      os_fprintf (recyclerStm, "%s %08x: Moved %d bytes\n", RecyclerName (R), R, size);
    else
      os_fprintf (recyclerStm, "%s %08x: Invalidated %d bytes\n", RecyclerName (R), R, size);
    fflush (recyclerStm);
  }
}
#endif	TRACE_RECYCLER

/*
  (void) ReclaimMovedVM(movedOK, R)
    boolean movedOK;
    PRecycler R;

  If movedOK==true, recyclable data has been moved elsewhere and so it may
  be reclaimed.  If not, the recycler must be reset to preserve the data.
 */
#define ReclaimMovedVM(movedOK, R)				\
{								\
  if (movedOK) {		/* Moved, so reclaim VM */	\
    if (R == sharedRecycler)					\
      vmShared->free = R->begin;				\
    else							\
      vmPrivate->free = R->begin;				\
    InitRecycler (R, R->begin);					\
  } else			/* Failed to move, so reset */	\
    ResetRecycler (R);						\
}

public procedure InvalidateRecycler (pObject1, pObject2)
  PObject pObject1, pObject2;
 /*
   pre: (pObject1->type == nullObj || Recyclable (pObject1)) &&
	(pObject2 == NIL ||
	 (pObject1->type != nullObj && Recyclable (pObject2))

   The most common invocation will have pObject2 == NIL, so invalidating
   the one range is straightforward.  If pObject1 is a nullObj, we are
   being called by the garbage collector to invalidate a range before a
   collection.  If pObject2 != NIL we need to check whether both private
   and shared recyclers need to be invalidated.
  */
{
  PRecycler R;
  boolean movedOK;

  DebugAssert ((pObject1->type == nullObj) || Recyclable (pObject1));

  R = RecyclerForObject (pObject1);

  if (pObject2 == NIL) {
    if (R->moveDisabled)
      movedOK = false;
    else {
      if (pObject1->type == nullObj) {
	movedOK = (GC_MoveRecycleRange (R, (PObject *) NIL, 0) != NIL);
      } else {
	movedOK = (GC_MoveRecycleRange (R, &pObject1, 1) != NIL);
      }
    }

#if	TRACE_RECYCLER
    TraceInvalidation (movedOK, R);
#endif	TRACE_RECYCLER

    ReclaimMovedVM (movedOK, R);
  } else {
    DebugAssert (Recyclable (pObject2));

    if (pObject1->shared == pObject2->shared) {
      if (R->moveDisabled)
	movedOK = false;
      else {
	PObject refs[2];

	refs[0] = pObject1;
	refs[1] = pObject2;
	movedOK = (GC_MoveRecycleRange (R, refs, 2) != NIL);
      }

#if	TRACE_RECYCLER
      TraceInvalidation (movedOK, R);
#endif	TRACE_RECYCLER

      ReclaimMovedVM (movedOK, R);
    } else {			/* pObject1, pObject2 in different VMs */
      if (R->moveDisabled)
	movedOK = false;
      else
	movedOK = (GC_MoveRecycleRange (R, &pObject1, 1) != NIL);

#if	TRACE_RECYCLER
      TraceInvalidation (movedOK, R);
#endif	TRACE_RECYCLER

      ReclaimMovedVM (movedOK, R);

      R = RecyclerForObject (pObject2);

      if (R->moveDisabled)
	movedOK = false;
      else
	movedOK = (GC_MoveRecycleRange (R, &pObject2, 1) != NIL);

#if	TRACE_RECYCLER
      TraceInvalidation (movedOK, R);
#endif	TRACE_RECYCLER

      ReclaimMovedVM (movedOK, R);
    }
  }
}

public PRecycler NewRecycler ()
 /*
  * Allocate new recycler record.  Do not bother to initialise begin and
  * end fields because that will occur when the initial expansion of VM is
  * done and the recycler reset to the end of the first segment.
  */
{
  PRecycler newRecycler;

  newRecycler = (PRecycler) NEW (1, sizeof (Recycler));

  /* All fields initially zero, which is correct */

  return (newRecycler);
}

#define	DATA_HANDLER_FLAGS \
	STATICEVENTFLAG(staticsCreate) | \
	STATICEVENTFLAG(staticsLoad)

private procedure RecyclerDataHandler (code)
  StaticEvent code;
 /*
  * The only state to change on context switch is the pointer to the private
  * recycler.
  */
{
  switch (code) {
   case staticsCreate:
    /* Fall through */
   case staticsLoad:
    privateRecycler = vmPrivate->recycler;
    break;
  }
}

public procedure Init_Recycler(reason)
  InitReason reason;
{
  switch (reason) {
   case init:
    /* Register dummy pointer for use as recyclerContextHandle */
    RegisterData ((PCard8)&recyclerContextHandle, 4, RecyclerDataHandler, (integer) DATA_HANDLER_FLAGS);
#if	TRACE_RECYCLER
    traceRecycler = false;
#endif	TRACE_RECYCLER

    recycleType = (PCard8) os_sureCalloc (1, nObTypes);
    recycleType[strObj] = true;
    recycleType[arrayObj] = true;
    /* recycleType[pkdaryObj] = true; QAR 184 - disable for now */
    /* Handle dictionaries later, but beware of GC restriction on moving. */
    break;
   case romreg:
#if	(STAGE == DEVELOP)
    if (vSTAGE == DEVELOP) {
#if	TRACE_RECYCLER
      RgstExplicit ("tracerecycler", PSTraceRecycler);
#endif	TRACE_RECYCLER
      RgstExplicit ("recyclerstatus", PSRecyclerStatus); }
#endif	(STAGE == DEVELOP)
    break;
  }
}
