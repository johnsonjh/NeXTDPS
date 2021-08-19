/*
  corouxfer.c

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
Ed Taft: Tue May  3 12:59:03 1988
Ivor Durham: Thu Aug 11 08:51:59 1988
Paul Rovner: Monday, June 6, 1988 11:21:34 AM
Terry Donahue: Monday, July 20, 1990
End Edit History.

This coroutine implementation makes use of a non-standard "xfer" primitive,
whose implementation is machine-dependent and must be provided separately.
There is another implementation, coroujmp.c, that uses the more standard
"setjmp" and "longjmp" primitives.

The xfer primitives manipulate a "machine state" (XferState), which
at the C level is simply an array of long integers. An XferState
represents an execution context that, at minimum, includes program
counter, stack pointer, and registers. The xfer primitive stores
the current machine state in one XferState and reloads the machine
state from another XferState. xfer is the basic building block for
implementing coroutines and lightweight processes. The xfer primitives
are described below.

The length of the XferState array is (in principle) machine-dependent
and is defined by the macro STATELENGTH; in practice, we make
STATELENGTH as large as is needed for any existing xfer implementation.
Current xfer implementations save most registers on the stack instead of
in the XferState, so the required length is quite small.

This implementation allocates stacks by means of malloc; it assumes that
stacks grow downward in memory. The original machine stack present at
InitCoroutine time remains for exclusive use by the "main" coroutine.
The stackSize argument passed to InitCoroutine is ignored; the procedures
CurStackUnused and MinStackUnused return MAXinteger when applied to
the "main" coroutine.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include COROUTINE
#include ENVIRONMENT
#include EXCEPT
#include PSLIB

#define STATELENGTH 4
typedef integer XferState[STATELENGTH];

extern procedure initxfer(
  /* XferState *state; char *stack; integer stackSize;
     procedure (*proc)(char *arg) */);
/* Initializes an XferState, using as its stack the region of memory
   starting at *stack and of length stackSize bytes. A subsequent
   call to xfer that specifies this XferState as destination causes
   (*proc)(arg) to be called, running in the new stack; proc must not
   ever return. */

extern char *xfer(/* XferState *source, *destination; char *arg */);
/* Stores the current machine state in the source XferState and
   loads the machine state from the destination XferState; in other
   words, transfers control from the source context to the destination
   context. If the destination XferState was most recently initialized
   by initxfer, the procedure "proc" passed to initxfer is called
   with arg as its argument. Otherwise, the destination context must
   most recently have executed an xfer with the destination XferState
   as its source; control passes to xfer in the destination context,
   which returns arg as its result. arg is an uninterpreted argument
   that may be cast to any pointer type. */


typedef struct _t_CoroutineRec {/* Coroutine concrete representation */
  XferState state;		/* saved machine state for this coroutine */
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

/* The following variable is public for the mac implementation of xfer */
public Coroutine rootCoroutine;

#define FOOTPRINT 0xBA987654
private readonly integer footprint = FOOTPRINT;


private ApplyFootprints(coroutine)
  Coroutine coroutine;
/* This initializes the stack of a coroutine after it is created
   and before it is executed. Caution: do not call this from within
   the coroutine itself. */
  {
  register integer count, *p;
  p = coroutine->stackLimit;
  count = coroutine->stackSize / sizeof(integer);
  do *p++ = FOOTPRINT; while (--count > 0);
  }

private CoroutineRootProc(arg)
  char *arg;
/* This is the root procedure passed to initxfer; arg is the "source"
   Coroutine passed to xfer by CoReturn. */
  {
  (*currentCoroutine->proc)((Coroutine) arg, currentCoroutine->info);
  CantHappen();  /* proc must never return */
  /*NOTREACHED*/
  }

/*ARGSUSED*/
public Coroutine InitCoroutine(stackSize, extraStack, check)
  integer stackSize, extraStack; boolean check;
  {
  Assert(currentCoroutine == NIL);
  coroutineExtraStack = extraStack;
  coroutineCheckFlag = check;
  currentCoroutine = rootCoroutine = (Coroutine)os_sureCalloc(sizeof(CoroutineRec), 1);
  currentCoroutine->stackLimit = &footprint;
  return currentCoroutine;
  }

public Coroutine CreateCoroutine(proc, info, stackSize)
  procedure (*proc)(/* Coroutine source; char *info */);
  char *info; register integer stackSize;
  {
  Coroutine coroutine;
  integer *stack;
  
  /* We are placing the coroutine structure and the stack in the same
   * malloc area, arranging things so that the coroutine structure is
   * at the end of the malloc and the stack grows from there down to the
   * beginning of the malloc'ed area.  This insures that for small stacks
   * we only touch one page of the stack.
   */
  stackSize += coroutineExtraStack;
  stackSize = ((stackSize + 3) / 4) * 4; /* stackSize multiple of 4 bytes */
  stack = (integer *) os_malloc(sizeof(CoroutineRec) + stackSize + 4);
  if (stack) {
    coroutine = (Coroutine) (((int) stack) + stackSize + 4);
    coroutine->info = info;
    coroutine->proc = proc;
    coroutine->stackSize = stackSize;
    coroutine->stackLimit = stack;
    coroutine->_Exc_Header = NIL;
    if (coroutineCheckFlag) ApplyFootprints(coroutine);
    initxfer(coroutine->state, coroutine->stackLimit, stackSize,
      CoroutineRootProc);
    }
  return coroutine;
  }

public DestroyCoroutine(coroutine)
  Coroutine coroutine;
  {
  Assert(coroutine != currentCoroutine);

  /* The malloc'ed pointer is in the stackLimit field since the stack comes
   * first in memory.
   */
  os_free((char *) coroutine->stackLimit);
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
  source = currentCoroutine;
  currentCoroutine = destination;
  source->_Exc_Header = _Exc_Header;
  _Exc_Header = destination->_Exc_Header;
  return (Coroutine) xfer(source->state, destination->state, (char *) source);
  }

#if (OS != os_mach)
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
  integer here;
  return (currentCoroutine->stackLimit == &footprint)?
    MAXinteger : (char *) &here - (char *) currentCoroutine->stackLimit;
  }

public integer MinStackUnused(coroutine)
  Coroutine coroutine;
  {
  register integer *p = coroutine->stackLimit;
  if (p == &footprint) return MAXinteger;
  while (*p++ == FOOTPRINT) {};
  return (char *) --p - (char *) coroutine->stackLimit;
  }
#endif
