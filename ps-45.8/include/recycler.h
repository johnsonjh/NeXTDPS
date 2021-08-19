/*
  recycler.h

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

Original version: Linda Gass and Andy Shore April 21, 1987
Edit History:
Ivor Durham: Sun May 14 15:14:04 1989
End Edit History.
*/

#ifndef	RECYCLER_H
#define	RECYCLER_H

#include BASICTYPES
#include EXCEPT
#include VM

#ifndef INLINE_RECYCLER
#define INLINE_RECYCLER 1
#endif

/*
 * Data types
 */

typedef struct _t_Recycler {
  PCard8  begin;		/* Pointer into VM of first recyclable item */
  PCard8  end;			/* Pointer to first free byte at end of VM */
  integer refCount;		/* Count of references to recyclable items */
  integer execSemaphore;	/* 0 when execLevel==0 for contexts in VM */
  PCard8  contextHandle;	/* Unique context identifier */
  boolean moveDisabled;		/* True iff multiple contexts used range */
} Recycler;

/*
 * Inline procedures
 */

/*
  boolean IsRecyclableType(pObject)
    PObject pobject;
 
  Returns true if the object type is one that can be recycled.
  [id: Restore conditional if any recyclable new escape-type objects defined.]
 */

#define IsRecyclableType(pObject)	\
 recycleType[/*(pObject)->type==escObj ? (pObject)->length :*/ (pObject)->type]

/*
  PRecycler RecyclerForObject(pObject)
    PObject pObject;
 
  Examine shared bit of object and pick the appropriate recycler.
 */

#define RecyclerForObject(pObject)	\
  ((pObject)->shared ? sharedRecycler : privateRecycler)

/*
  PCard8 RecyclerAddress(pObject)
    PObject pObject;
 
  Cast object to null type and take null value as the address.  This allows
  use of value field that points into VM without individual object type
  checking.
 */

#define RecyclerAddress(pObject)	\
  ((PCard8)(((PNullObj)(pObject))->val.nullval))

/*
  boolean AddressInRecyclerRange(R, address)
   PRecycler R;
   PCard8 address;
 
  Returns true if address in [begin,end] range for the recycler.
 */

#define AddressInRecyclerRange(R, address)	\
  (((R)->begin <= (address)) && ((address) < (R)->end))

/*
  boolean ObjectInRecyclerRange(R, address)
   PRecycler R;
   PCard8 address;
 
  Returns true if object begins in [begin,end] range for the recycler.
  If it begins in the range, it is contained within the range.
 */

#define	ObjectInRecyclerRange(pObject)	\
  AddressInRecyclerRange(RecyclerForObject(pObject), RecyclerAddress(pObject))


/*
  InitRecycler(R, address)
    PRecycler R;
    PCard8 address;

  Initialise recycler with empty range beginning at given address.
 */

#define InitRecycler(R, address)	\
{					\
  R->end = (address);			\
  ResetRecycler (R);			\
}

/*
  ConditionalResetRecycler(R, address)
    PRecycler R;
    PCard8 address;

  if the given address is in the recycler range, reset the recycler.
 */

#define ConditionalResetRecycler(R, address)	\
{						\
  if (AddressInRecyclerRange (R, (address)))	\
    ResetRecycler (R);				\
}

/*
  ConditionalInvalidateRecycler (pObject)
    PObject pObject;

  Invalidate if object is recyclable.
 */

#define ConditionalInvalidateRecycler(pObject)	\
{						\
  if (Recyclable (pObject))			\
    InvalidateRecycler (pObject, NIL);		\
}

/*
  _ExtendRecycler(R, endOfVM)
    PRecycler R;
    PCard8 endOfVM;
 
  Move up end of recycler range.
 */

#define	_ExtendRecycler(R, endOfVM)	\
{					\
  (R)->end = (endOfVM);			\
}

/*
  _ReclaimRecyclableVM()

  Resets the recycler and VM pointer if there are no references and the
  exec level semaphore is clear.
 */

#define _ReclaimRecyclableVM()						\
{									\
  PRecycler R = vmCurrent->recycler;					\
									\
  if ((R->refCount | R->execSemaphore) == 0) {				\
    Assert ((R->begin <= R->end) && (R->end == vmCurrent->free));	\
    vmCurrent->free = R->begin;						\
    InitRecycler (R, R->begin);						\
  }									\
}

/*
  boolean _Recyclable(pObject)
    PObject pObject;
 
  If the object is of a type that can be recycled and it is in the current
  collection of recyclable objects, return true and false otherwise.
 */

#define _Recyclable(pObject) \
  (IsRecyclableType(pObject) && ObjectInRecyclerRange(pObject))

/*
  boolean IsRecyclable(pObject,R)
    PObject pObject;
    PRecycler R;

  pre: R == RecyclerForObject(pObject)
 
  This is the key change for working around the compiler limitation in
  common subexpression optimization.
 */
 
#define IsRecyclable(pObject,R) \
   (IsRecyclableType(pObject) && AddressInRecyclerRange(R, RecyclerAddress(pObject)))
 
/*
  _RecyclerPop(pObject)
 
  Decrement reference count for the appropriate recycler for the object if
  the object is in that recycler range.
 */

#define	_RecyclerPop(pObject)				\
{							\
  register PRecycler R = RecyclerForObject(pObject);	\
  if (IsRecyclable(pObject,R)) {			\
    R->refCount--;					\
    Assert (R->refCount >= 0);				\
  }							\
}

/*
  _RecyclerPush(pObject)
 
  Increment reference count for the appropriate recycler for the object if
  the object is in that recycler range.
 */

#define	_RecyclerPush(pObject)				\
{							\
  PRecycler R = RecyclerForObject(pObject);		\
  if (IsRecyclable(pObject,R)) {			\
    R->refCount++;					\
    if (R->contextHandle != recyclerContextHandle) {	\
      if (R->contextHandle == NIL)			\
        R->contextHandle = recyclerContextHandle;	\
      else						\
        R->moveDisabled = true;				\
    }							\
  }							\
}

/*
  _ResetRecycler (R)
 
  Reset recycler range to end of range and clear reference count.
  Do not touch the execSemaphore because that is independent of the range.
 */

#define	_ResetRecycler(R)	\
{				\
  (R)->begin = (R)->end;	\
  (R)->refCount = 0;		\
  (R)->contextHandle = NIL;	\
  (R)->moveDisabled = false;	\
}

/*
  _ChangeRecyclerExecLevel(newLevel, deeper)
    int newLevel;
    boolean deeper;

  pre: sharedRecycler and privateRecycler are both valid.

  Account for transitions up from level 1 and back in execSemaphore.
  Transition from 0 to 1 and back is entering and leaving top level.

  "deeper" should be a constant so only one arm of the top-level conditional
  should be compiled.
 */

#define _ChangeRecyclerExecLevel(newLevel, deeper)	\
{							\
  if (deeper) {						\
    if (newLevel == 2) {				\
      privateRecycler->execSemaphore++;			\
      sharedRecycler->execSemaphore++;			\
    }							\
  } else {						\
    if (newLevel == 1) {				\
      privateRecycler->execSemaphore--;			\
      sharedRecycler->execSemaphore--;			\
    }							\
  }							\
}

/*
 * Exported Procedures
 */

extern procedure InvalidateRecycler(/* PObject pObject1, pObject2 */);
 /*
   Move the recycle range(s) containing the objects somewhere else
   and invalidate the range(s).  pObject2 may be NIL.
  */

#if INLINE_RECYCLER
#define	ExtendRecycler(R, endOfVM)	_ExtendRecycler(R, endOfVM)
#define	ReclaimRecyclableVM()		_ReclaimRecyclableVM()
#define	Recyclable(pObject)		_Recyclable(pObject)
#define	RecyclerPop(pObject)		_RecyclerPop(pObject)
#define	RecyclerPush(pObject)		_RecyclerPush(pObject)
#define	ResetRecycler(R)		_ResetRecycler (R)
#define	ChangeRecyclerExecLevel(L,d)	_ChangeRecyclerExecLevel(L,d)
#else INLINE_RECYCLER
extern procedure	ExtendRecycler(/* PRecycler R, PCard8 endOfVM*/);
extern procedure	ReclaimRecyclableVM();
extern boolean		Recyclable(/* PObject pObject */);
extern procedure	RecyclerPop(/* PObject pObject */);
extern procedure	RecyclerPush(/* PObject pObject */);
extern procedure	ResetRecycler(/* PRecycler R */);
extern procedure	ChangeRecyclerExecLevel(/* int newLevel, bool deeper */);
#endif INLINE_RECYCLER

/*
 * Exported Data
 */

extern PRecycler privateRecycler, sharedRecycler;
extern PCard8 recycleType, recyclerContextHandle;

#endif	RECYCLER_H
