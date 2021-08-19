/*
  stack.c

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Chuck Geschke: February 1, 1983
Edit History:
Scott Byer: Thu Jun  1 15:53:22 1989
Chuck Geschke: Fri Oct 11 07:18:49 1985
Doug Brotz: Mon Jun  2 10:33:52 1986
Ed Taft: Sun Dec 17 14:02:03 1989
Joe Pasqua: Thu Feb 23 13:30:17 1989
Ivor Durham: Mon May 15 23:23:07 1989
Hourvitz: 19Jun87 Rewrite for array based stacks
Jim Sandman: Thu Mar 10 14:35:31 1988
Jack Newlin  7Jun90 fix Roll per Bilodeau instructions (Adobe bug #25swift)
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include GC
#include LANGUAGE
#include PSLIB
#include RECYCLER
#include "array.h"
#include "exec.h"
#include "name.h"
#include "stack.h"

#define initOpstkSize 500
#define initExecstkSize 250
#define initDictstkSize 20
#define initRefstkSize 5

/*
 * Exported Data
 */

public	PStack	opStk;
public	PStack	execStk;
public	PStack	dictStk;
public	PStack	refStk;

private procedure MakeNullObjects();	/* Forward declaration for Overflow */

/*	----- Error handling  -----	*/

/*ARGSUSED*/
public procedure Underflow(stack) PStack stack; {PSError(stackunderflow);}

public procedure Overflow(stack) register Stack *stack;
/* Overflow is called when the stack is full (head == limit)	*/
/* and somebody wants to push something on the stack.  If	*/
/* stacks can dynamically grow, Overflow will grow the stack	*/
/* by STKGROWINC Objects and return.  Otherwise, it will	*/
/* just raise PS_STKOVRFLW.					*/
  {
  PObject newStack;
  int newSize = (stack->limit - stack->base) + STKGROWINC;
  
  if (newSize >= curStackLimit)	/* Too Big! Give it up...	*/
    {
    psFULLSTACK = stack;
    RAISE(PS_STKOVRFLW, (char *)NIL);
    }
  newStack = (PObject)os_realloc(
    (char *)stack->base, (long int)newSize * sizeof(Object));
  if (newStack == NULL)
    LimitCheck();
  stack->head = newStack + (stack->head - stack->base);
  stack->limit = newStack + newSize;
  stack->base = newStack;
  MakeNullObjects(stack, stack->head);
  }

public procedure RstrStack(stack, mark)
PStack stack;
PObject mark;
/* Try to restore the stack so that mark is the first	*/
/* free object. First, be sure marked node is in stack	*/
{
  register PObject po;

  if ((mark < stack->base)||(mark > stack->limit + 1)) return;
  for (po = stack->head; po < mark; po++) RecyclerPush(po);
  stack->head = mark;
}  /* end of RstrStack */

/*  ----- Stack Allocation -----	*/

private procedure MakeNullObjects(stack, objPtr)
Stack *stack;
register PObject objPtr;
{
  Object template;
  register PObject limit = stack->limit;
  
  LNullObj(template);
  for(; objPtr < limit; objPtr++) *objPtr = template;
} /* MakeNullObjects */

private PStack NewStack(n)
cardinal n;
/* NewStack: allocates a new stack of given length */
{
  register PStack stack;

  stack = (Stack *)NEW(1, sizeof(Stack));
  n = STKINITSIZE;
  stack->base = (Object *)os_malloc((long int)(n * sizeof(Object)));
    /* Use malloc here so we can realloc later...	*/
  if (stack->base == NULL)
    LimitCheck();
  stack->head = stack->base;
  stack->limit = stack->head + n;
  MakeNullObjects(stack, stack->base);
  return(stack);
} /* NewStack */

/*	----- Stack Operations -----	*/

public procedure CreateStacks ()
{
  opStk = NewStack (initOpstkSize);
  execStk = NewStack (initExecstkSize);
  dictStk = NewStack (initDictstkSize);
  refStk = NewStack (initRefstkSize);
    /* This is a place to put refs to objects that	*/
    /* aren't garbage, but aren't otherwise reachable	*/
}

public procedure StackPushP(pob, stack)
register Object *pob;
register Stack *stack;
{
  IPush(stack, (*pob));
}  /* end of StackPushP */

StackPopP(pob, stack)
register Object *pob;
register Stack *stack;
{
  IPop (stack, pob);
} /* StackPopP */

public procedure StackTopP(pob, stack)
PObject pob;
PStack stack;
{
  if (stack->head <= stack->base) Underflow(stack);
  *pob = *(stack->head-1);
} /* StackTopP */

/* These should all be macros on the relevant routines */
public procedure PopP(pob)
  PObject pob;
  {
  StackPopP(pob, opStk);
  IPushSimple(refStk, *pob);	/* For garbage collector...	*/
  }
public procedure PushP(pob)  PObject pob; {StackPushP(pob, opStk);}
public procedure EPushP(pob)  PObject pob; {StackPushP(pob, execStk);}
public procedure EPopP(pob) PObject pob; {StackPopP(pob, execStk);}
public procedure TopP(pob)  PObject pob; {StackTopP(pob, opStk);}
public procedure DTopP(pob)  PObject pob; {StackTopP(pob, dictStk);}
public procedure ETopP(pob)  PObject pob; {StackTopP(pob, execStk);}

/* StackPushP & PopP do recycling, don't want that on dictStk	*/
public procedure DPushP(pob) PObject pob; {IPushSimple(dictStk, *pob);}
public procedure DPopP(pob) PObject pob; {IPopSimple(dictStk, pob);}

KillStack(stack)
PStack stack;
{
  FREE(stack->base);
  FREE(stack);
} /* KillStack */

public procedure CopyStack(src, dst, n)
  register PStack src, dst;
  register cardinal n;
{
  Object *po;

  if (n == 0)
    return;

  if (CountStack (src, n) < n)
    Underflow (src);

  while (n > (dst->limit - dst->head))
    Overflow (dst);		/* handler? */

  for (po = src->head - n; n > 0; n--, po++) {
    *dst->head++ = *po;
    RecyclerPush (po);
  }
}

public procedure Copy(stack, n)
register PStack stack;
register cardinal n;
{
  Object *po;

  if (n == 0)
    return;
  if (CountStack (stack, n) < n)
    Underflow (stack);
  while (n > (stack->limit - stack->head))
    Overflow (stack);
  for (po = stack->head - n; n > 0; n--, po++) {
    *stack->head++ = *po;
    RecyclerPush (po);
  }
}				/* Copy */

private Roll(stack, n, k)
register PStack stack;
cardinal n, k;
/* Roll the top n entries by k places (in the "Pop" direction)	*/
{
  register PObject po, po2;
  register int i;

  if (CountStack(stack, n) < n) Underflow(stack);
  if ((n == 0) || ((k = k % n) == 0)) return;
  /* NOTE: By having two cases in the code below,	*/
  /* we could bound k to [-n/2, n/2] instead of [0, n]	*/
  /* Ensure there's enough space to buffer k elements	*/
  while(k > (stack->limit - stack->head)) Overflow(stack);
  /* Copy everybody up by k spaces			*/
  for (po = stack->head+k, po2 = stack->head, i=n; i > 0; i--)
    *--po = *--po2;
  /* Copy the k elements above stack head down into the	*/
  /* empty spots. po points at the correct destination.	*/
  for (po2 = stack->head + k; k > 0; k--) *--po = *--po2;
} /* Roll */

public cardinal CountStack(stack, max)
register Stack *stack;
register cardinal max;
{
  register integer i = stack->head - stack->base;
  return( i > max ? max : i);
}

public procedure ClearStack(stack)  register PStack stack;
{
  register PObject po;
  for (po = stack->head - 1; po >= stack->base; po--)
    RecyclerPop(po);
  stack->head = stack->base;
}  /* end of ClearStack */

public cardinal CountToMark(stack)  PStack stack;
{
  register integer n = 0;
  register PObject po;

  for (po = stack->head-1; po >= stack->base; po--)
    {
    if (po->type == escObj && po->length == objMark) return(n);
    n++;
    }
  PSError(unmatchedmark);
}  /* end of CountToMark */

public procedure ArrayFromStack(pao, stack)
register PAryObj pao;
PStack stack;
{
  register cardinal i;
  cardinal size = CountStack(stack, MAXcardinal);
  register Object *po = stack->base;
  
  if (size > pao->length) RangeCheck();
  if (size != 0)
    {
    for (i = 0; i < size; i++) APut(*pao, i, *po++);
    pao->length = size;
    }
}  /* end of ArrayFromStack */

public procedure ArrayToStack(pao, stack)
register PAryObj pao;
PStack stack;
{
  cardinal size, i;
  Object notMuch;
  
  if ((size = pao->length) != 0)
    {
    for (i = 0; i < size; i++)
      {
      AGetP(*pao, i, &notMuch);
      StackPushP(&notMuch, stack);
      }
    }
}  /* end of ArrayToStack */

public procedure EnumStack(stack, callBackProc, clientData)
/* Enumerates the elements on the spec'd stack from tos	*/
/* down. Calls callBackProc with each element and with	*/
/* the supplied clientData. The enumeration continues	*/
/* as long as the callBackProc returns true.		*/
PStack stack;
boolean (*callBackProc)(/* PObject, clientData */);
charptr clientData;
{
  register PObject obj;
  
  for (obj = stack->head - 1; obj >= stack->base; obj--)
    if ( (*callBackProc)(obj, clientData) == false ) return;
}  /* End of EnumStack */


/*
  The following procs and data structures ultimately implement the
  CheckAllStacks procedure. CheckStack uses EnumStack.  The enumerator
  calls CheckStackElement for each stack element.  CheckStackStruct is
  used to communicate between CheckStack and the callback.	
*/

struct CheckStackStruct {
  PStack stack;
  Level n;
  boolean problem;
  };

private boolean CheckStackElement(obj, data)
PObject obj;
charptr data;
{
  struct CheckStackStruct *css = (struct CheckStackStruct *) data;

  /* Following test uses obj->level only as a hint that the object's
     value MIGHT be at too deep a save level. Note that the shared bit is
     set in all atomic objects as well as composite objects in shared VM. */
  if (obj->shared || obj->level <= css->n)
    return true;  /* object is valid; continue enumeration */

  /* Must inspect value of object more carefully */
  switch ((obj->type == escObj)? obj->length : obj->type)
    {
    case objSave:
      /* value of save object is level that it restores to */
      if (obj->val.saveval > css->n)
        {css->problem = true; return false;}
      break;

    case stmObj:
      if (! AddressValidAtLevel(obj->val.strval, css->n))
        {
	if (css->stack == execStk)
	  {
	  /* Don't generate invalidrestore for a StmObj on the execution
	     stack, since the restore will close the file anyway.
	     Instead, replace the stack entry with a benign value. */
	  LNullObj(*obj);
	  obj->tag = Xobj;
	  return true;
	  }
        css->problem = true;
	return false;
	}
      break;

    default:
      /* assert: val is a pointer into VM */
      if (! AddressValidAtLevel(obj->val.strval, css->n))
        {css->problem = true; return false;}
      break;
    }

  /* Object's value is still valid. Force the object's save level
     down to the level being restored */
  obj->level = css->n;
  return true;
}

private boolean CheckStack(stack, n)
PStack stack;
Level n;
{
  struct CheckStackStruct css;

  css.stack = stack;
  css.n = n;
  css.problem = false;
  EnumStack (stack, CheckStackElement, (charptr) & css);
  return (css.problem);
}				/* end of CheckStack */

private boolean CheckAllStacks(n)
  Level n;
{
  return( CheckStack (opStk, n) ||
	  CheckStack (dictStk, n) ||
	  CheckStack (execStk, n) );
} /* End of CheckAllStacks */

/*	----- Stack Intrinsics -----	*/

public procedure StackPopDiscard(stack)
register PStack stack;
{
  if (stack->head <= stack->base)
    Underflow (stack);
  stack->head--;
  RecyclerPop (stack->head);
}				/* StackPopDiscard */

public procedure PSPop() {StackPopDiscard(opStk);}

public procedure PSDup()
{
  if (opStk->head <= opStk->base) Underflow(opStk);
  if (opStk->head >= opStk->limit) Overflow(opStk);
  *opStk->head = *(opStk->head-1);
  RecyclerPush(opStk->head);
  opStk->head++;
} /* end of PSDup */

public procedure PSExch()
{
  register Object *po;
  Object ob;

  po = opStk->head-1;
  if (po <= opStk->base) Underflow(opStk);
  ob = *po;
  *po = *(po - 1); 
  *(po-1) = ob;
} /* end of PSExch */

public procedure PSRoll()
{
  integer kk = PopInteger();
  integer nn = PopInteger();
  if (nn < 0) RangeCheck();
  nn = MIN(nn, MAXcardinal);
  kk = (nn==0)?0:kk%nn;
  if (kk < 0) kk += nn;
  Roll(opStk, (cardinal)nn, (cardinal)kk);
}
  
public procedure PSClear() {ClearStack(opStk);}
public procedure PSCount() {PushCardinal(CountStack(opStk, MAXcardinal));}
public procedure PSCntToMark() {PushCardinal(CountToMark(opStk));}

public procedure PSCntDictStack()
{PushCardinal(CountStack(dictStk, MAXcardinal));}

public procedure PSCntExecStack()
{PushCardinal(CountStack(execStk, MAXcardinal));}


public procedure PSClrToMrk()
{
  Object ob;
  until (opStk->head <= opStk->base)
    {
    StackPopP(&ob, opStk);
    if (ob.type == escObj && ob.length == objMark) return;
    }
  PSError(unmatchedmark);
}  /* end of PSClrToMark */

public procedure PSMark()
{
  Object ob;
  LMarkObj(ob);
  PushP(&ob);
}  /* end of PSMark */

public procedure PSIndex()
{
  register Object *po;

  po = opStk->head - 2;
  po -= PopCardinal ();

  if (po < opStk->base) Underflow(opStk);
  PushP(po);
}  /* end of PSIndex */

public procedure PSExecStack()
{
  AryObj ao;
  PopPArray(&ao);
  ArrayFromStack(&ao, execStk);
  PushP(&ao);
}  /* end of PSExecStack */

public procedure PSDictStack()
{
  AryObj ao;
  PopPArray(&ao);
  ArrayFromStack(&ao, dictStk);
  PushP(&ao);
}  /* end of PSDictStack */

#if	(STAGE == DEVELOP)
private procedure PSSetStackLimit()
{
  cardinal i = PopCardinal();
  
  PushCardinal(curStackLimit);
  if (i < curStackLimit || i > UPPERSTKLIMIT)
    {PushBoolean(false); return;}

  curStackLimit = i;
  PushBoolean(true);
}

private procedure PSStackStats()
  {
  os_printf("Current stack limit: %d\n", curStackLimit);
  os_printf("\nOperand Stack:\n");
  os_printf("Current Size: %d\n", opStk->limit - opStk->base);
  os_printf("Current Elements: %d\n", opStk->head - opStk->base);
  os_printf("\nDict Stack:\n");
  os_printf("Current Size: %d\n", dictStk->limit - dictStk->base);
  os_printf("Current Elements: %d\n", dictStk->head - dictStk->base);
  os_printf("\nExec Stack:\n");
  os_printf("Current Size: %d\n", execStk->limit - execStk->base);
  os_printf("Current Elements: %d\n", execStk->head - execStk->base);
  os_printf("\nRef Stack:\n");
  os_printf("Current Size: %d\n", refStk->limit - refStk->base);
  os_printf("Current Elements: %d\n", refStk->head - refStk->base);
  }
#endif	(STAGE == DEVELOP)

/*	----> Collector Support Routines <----	*/
/* The following procs are required for correct	*/
/* operation of the Garbage Collector.		*/

procedure PushStackRoots(info)
/* This proc pushes all roots found on the	*/
/* dictStk, opStk, & refStk onto the GC's stk.	*/
GC_Info info;
{
  register PObject obj, last;
  
  for (	/* FOR each object on the opStk DO...	*/
    obj = opStk->head - 1, last = opStk->base;
    obj >= last; obj--)
    GC_Push(info, obj);

  for (	/* FOR each object on the execStk DO...	*/
    obj = execStk->head - 1, last = execStk->base;
    obj >= last; obj--)
    GC_Push(info, obj);

  for (	/* FOR each object on the refStk DO...	*/
    obj = refStk->head - 1, last = refStk->base;
    obj >= last; obj--)
    GC_Push(info, obj);

  /* We really needn't push the permanent dicts	*/
  /* since they'll be found by other means. But	*/
  /* it doesn't hurt to push them. So we will.	*/
  for (	/* FOR each object on the dictStk DO...	*/
    obj = dictStk->head - 1, last = dictStk->base;
    obj >= last; obj--)
    GC_Push(info, obj);
}

/*	-----------------------------------	*/
/*	----> END GC Support Routines <----	*/
/*	-----------------------------------	*/

public procedure StackInit(reason)
InitReason reason;
{
  switch (reason) {
    case init:
      RgstStackChecker(CheckAllStacks);
      break; 
    case romreg:
#if	STAGE==DEVELOP
      if (vSTAGE == DEVELOP) {
        RgstExplicit ("setstacklimit", PSSetStackLimit);
        RgstExplicit ("stackstats", PSStackStats);
      }
#endif	STAGE==DEVELOP
      break;
    endswitch
    }
}
