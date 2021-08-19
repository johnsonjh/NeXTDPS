/*
  coroujmp.c

Copyright (c) 1987, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Thu Sep 17 09:05:48 1987
Edit History:
Ed Taft: Tue May  3 13:03:06 1988
Ivor Durham: Thu Aug 11 08:52:05 1988
End Edit History.

This is a demonstration of a coroutine implementation written entirely in C.
It serves mainly as a model and is unlikely to be used in any real system.
It depends on the following assumptions, which may not be valid in
some C implementations:

  1. The library procedures setjmp and longjmp are available.
  2. The stack is allocated linearly in memory at decreasing addresses.
  3. Stack space below the current stack pointer is not automatically
     reclaimed.
  4. It is OK to perform a longjmp to a frame that is below the current
     stack pointer (assuming, of course, that the frame is still intact).

This implementation also assumes that the coroutines it creates have
a small number of discrete stack sizes; it makes no attempt to split
or coalesce stacks.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include COROUTINE
#include EXCEPT		/* includes setjmp.h or some substitute */
#include ENVIRONMENT

/* Since we do not switch coroutines from within signal handlers,
   we can use the flavors of setjmp and longjmp that do not fiddle
   with the signal mask, assuming those are available. This avoids
   a kernel call and may be substantially more efficient.
 */

#ifndef SETJMP
#define SETJMP _setjmp
#endif

#ifndef LONGJMP
#define LONGJMP _longjmp
#endif

typedef struct _t_CoroutineRec {/* Coroutine concrete representation */
  Coroutine next;		/* -> next coroutine on free list;
				   this must be the first field */
  struct {jmp_buf buf;}		/* wrap with struct to allow assignment */
    initialState,		/* state at moment of coroutine creation */
    state;			/* state as of last CoReturn */
  procedure (*proc)(/* Coroutine source; char *info */); /* initial proc */
  char *info;			/* uninterpreted info for proc */
  integer stackSize;		/* requested size of stack in bytes */
  integer *stackLimit;		/* where the stack ends */
  _Exc_Buf *_Exc_Header;	/* head of exception handler chain */
  } CoroutineRec;

/* The following variable is exported through the interface */
public Coroutine currentCoroutine;

/* The following variables are public for debugging purposes */
public boolean coroutineCheckFlag;
public integer coroutineExtraStack;

private Coroutine freeCoroutine, allocatorCoroutine;
private CoroutineRec initialCoroutineRec;

#define FOOTPRINT 0xBA987654
private readonly integer footprint = FOOTPRINT;


/* Coroutine stack allocation

   At any given moment, the deepest procedure call in progress is to
   the procedure CoroutineRootProc, which is owned by the allocatorCoroutine.
   Successive instances of the allocatorCoroutine are always
   deeper on the real machine stack. There is no provision to reclaim
   machine stack space when coroutine contexts are destroyed; however,
   destroyed coroutines are recycled by CreateCoroutine.

   The flow of control when allocating a new coroutine is as follows:

   1. CreateCoroutine does a CoReturn to the allocatorCoroutine, which
   transfers control into CoroutineRootProc at the statement after
   its first CoReturn.

   2. CoroutineRootProc saves its current state, then calls CoroutineStackProc
   to allocate a stack for the new coroutine. CoroutineStackProc in turn calls
   CoroutineRootProc recursively.

   3. The recursive CoroutineRootProc establishes itself as the new
   allocatorCoroutine. It then does a CoReturn back to the original
   allocatorCoroutine, which is about to become the newly allocated
   coroutine.

   4. The original allocatorCoroutine does a CoReturn back to the
   coroutine that called CreateCoroutine, thereby passing itself
   (the newly allocated coroutine) as the result.

   5. CreateCoroutine saves the coroutine's state as its initialState;
   then it returns the coroutine as its result.

   6. The first time anyone does a CoReturn to the coroutine, control
   returns to CoroutineRootProc at the statement after its second
   CoReturn. It then calls the proc that was passed to CreateCoroutine.
   The coroutine is now off and running.

   To recycle a coroutine, CreateCoroutine resets its state to its
   initialState and sets its proc and info appropriately. The next
   CoReturn to that coroutine reenters CoroutineRootProc at step 6.
*/

#define STACKPROCOVERHEAD 12  /* size of arg, pc, link */

private CoroutineStackProc1000(stackSize)
  integer stackSize;
  {
  integer stack[(1000 - STACKPROCOVERHEAD) / sizeof(integer)];
  currentCoroutine->stackLimit = &stack[0];
  if (stackSize >= 1000) CoroutineStackProc1000(stackSize - 1000);
  else if (stackSize > 0) CoroutineStackProc100(stackSize - 100);
  else CoroutineRootProc();
  }

private CoroutineStackProc100(stackSize)
  integer stackSize;
  {
  integer stack[(100 - STACKPROCOVERHEAD) / sizeof(integer)];
  currentCoroutine->stackLimit = &stack[0];
  if (stackSize > 0) CoroutineStackProc100(stackSize - 100);
  else CoroutineRootProc();
  }

private ApplyFootprints(coroutine)
  Coroutine coroutine;
/* This initializes the stack of a coroutine after it is created
   and before it is executed. Caution: do not call this from within
   the coroutine itself. */
  {
  register integer count, *p;
  p = coroutine->stackLimit;
  count = (coroutine->stackSize - STACKPROCOVERHEAD) / sizeof(integer);
  do *p++ = FOOTPRINT; while (--count > 0);
  }

private CoroutineRootProc()
/* Called recursively to create a new allocatorCoroutine (step 2 above) */
  {
  CoroutineRec myCoroutineRec;
  Coroutine other;
  other = allocatorCoroutine;
  allocatorCoroutine = currentCoroutine = &myCoroutineRec;

  /* currentCoroutine == this invocation of CoroutineRootProc, which is
       to become the new allocatorCoroutine.
     other == the next higher invocation of CoroutineRootProc, which is
       the coroutine to be given to CreateCoroutine.
   */
  if (coroutineCheckFlag) {
    ApplyFootprints(other);
    myCoroutineRec.stackLimit = &footprint;
    }
  other = CoReturn(other); /* Back to coroutine being allocated (step 3) */

  /* Somebody has called CreateCoroutine, which has done a CoReturn (step 1).
     other == somebody invoking CreateCoroutine.
     currentCoroutine == allocatorCoroutine -> myCoroutineRec:
       the coroutine to be given to CreateCoroutine.
     currentCoroutine->stackSize has been set by caller.
   */
  if (SETJMP(myCoroutineRec.state.buf) == NIL)
    {/* Now create new stack and allocatorCoroutine by invoking
        CoroutineStackProc and CoroutineRootProc recursively (step 2). */
    if (myCoroutineRec.stackSize >= 1000)
      CoroutineStackProc1000(myCoroutineRec.stackSize - 1000);
    else CoroutineStackProc100(myCoroutineRec.stackSize - 100);
    /*NOTREACHED*/
    }

  /* The recursive CoroutineRootProc has done a CoReturn to here.
     other == the caller invoking CreateCoroutine.
     currentCoroutine -> myCoroutineRec: this invocation of CoroutineRootProc.
     Now simply send control back to CreateCoroutine, giving it
     currentCoroutine as the result (step 4). This has to be done
     by explicit setjmp/longjmp instead of CoReturn because
     this coroutine may be restarted here repeatedly, whenever it is
     destroyed and recycled.
   */
  currentCoroutine = other;
  if ((other = (Coroutine) SETJMP(myCoroutineRec.state.buf)) == NIL) {
    LONGJMP(currentCoroutine->state.buf, &myCoroutineRec);
    /*NOTREACHED*/
    }

  /* other == somebody who has done a CoReturn to this coroutine
     for the first time (step 6).
     currentCoroutine -> myCoroutineRec: this invocation of CoroutineRootProc.
   */
  (*myCoroutineRec.proc)(other, myCoroutineRec.info);
  CantHappen();  /* proc must never return */
  /*NOTREACHED*/
  }

public Coroutine InitCoroutine(stackSize, extraStack, check)
  integer stackSize, extraStack; boolean check;
  {
  Assert(currentCoroutine == NIL);
  coroutineExtraStack = extraStack;
  stackSize += extraStack;
  coroutineCheckFlag = check;
  allocatorCoroutine = currentCoroutine = &initialCoroutineRec;
  if (SETJMP(currentCoroutine->state.buf) == NIL) {
    currentCoroutine->initialState = currentCoroutine->state;
    currentCoroutine->stackSize = stackSize;
    CoroutineStackProc1000(stackSize - 1000);
    /*NOTREACHED*/
    }
  return currentCoroutine;
  }

public Coroutine CreateCoroutine(proc, info, stackSize)
  procedure (*proc)(/* Coroutine source; char *info */);
  char *info; register integer stackSize;
  {
  register Coroutine coroutine, pred;
  stackSize += coroutineExtraStack;
  pred = (Coroutine) &freeCoroutine;
  while ((coroutine = pred->next) != NIL) {
    if (coroutine->stackSize == stackSize) break;
    pred = coroutine;
    }
  if (coroutine != NIL) {
    /* Recycle an existing coroutine */
    pred->next = coroutine->next;
    coroutine->state = coroutine->initialState;
    if (coroutineCheckFlag) ApplyFootprints(coroutine);
    }
  else {
    /* Create a new coroutine and stack from scratch */
    allocatorCoroutine->stackSize = stackSize;
    coroutine = CoReturn(allocatorCoroutine);
    coroutine->initialState = coroutine->state;
    }
  coroutine->next = NIL;
  coroutine->proc = proc;
  coroutine->info = info;
  coroutine->_Exc_Header = NIL;
  return coroutine;
  }

public DestroyCoroutine(coroutine)
  Coroutine coroutine;
  {
  Assert(coroutine != currentCoroutine && coroutine->next == NIL);
  coroutine->state = coroutine->initialState;
  coroutine->proc = CantHappen;
  coroutine->next = freeCoroutine;
  freeCoroutine = coroutine;
  }

public Coroutine CoReturn(destination)
  Coroutine destination;
  {
  Coroutine source;
  if (coroutineCheckFlag)
    Assert(currentCoroutine != NIL &&
      *currentCoroutine->stackLimit == FOOTPRINT &&
      destination != NIL &&
      *destination->stackLimit == FOOTPRINT);
  source = (Coroutine) SETJMP(currentCoroutine->state.buf);
  if (source == NIL) {
    source = currentCoroutine;
    currentCoroutine = destination;
    source->_Exc_Header = _Exc_Header;
    _Exc_Header = destination->_Exc_Header;
    LONGJMP(destination->state.buf, source);
    /*NOTREACHED*/
    }
  return source;
  }

public char *GetCoroutineInfo(coroutine)
  Coroutine coroutine;
  {
  return coroutine->info;
  }

typedef struct {
  integer (*proc)(/* char *arg */);
  char *arg;
  integer result;
  } TempCoroutineParam;

#define TEMPCOROUTINEOVERHEAD 20 /* size of 1 local, 2 args, pc, link */

private TempCoroutineProc(source, info)
  Coroutine source; char *info;
  {
  TempCoroutineParam *param = (TempCoroutineParam *) info;
  currentCoroutine->info = source->info;
  param->result = (*param->proc)(param->arg);
  (void) CoReturn(source);
  /*NOTREACHED*/
  }

public integer CallAsCoroutine(proc, arg, stackSize)
  integer (*proc)(/* char *arg */); char *arg; integer stackSize;
  {
  Coroutine coroutine;
  TempCoroutineParam param;
  param.proc = proc;
  param.arg = arg;
  DestroyCoroutine(
    CoReturn(
      CreateCoroutine(TempCoroutineProc, (char *) &param,
        stackSize + TEMPCOROUTINEOVERHEAD)));
  return param.result;
  }

public integer CurStackUnused()
  {
  integer result;
  result = (char *) &result - (char *) currentCoroutine->stackLimit;
  return result;
  }

public integer MinStackUnused(coroutine)
  Coroutine coroutine;
  {
  register integer *p = coroutine->stackLimit;
  while (*p++ == FOOTPRINT) {};
  return (char *) --p - (char *) coroutine->stackLimit;
  }
