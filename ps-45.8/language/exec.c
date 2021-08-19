/*
  exec.c

Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Chuck Geschke: February 13, 1983
Edit History:
Larry Baer: Tue Nov 14 11:14:12 1989
Scott Byer: Thu Jun  1 17:32:15 1989
Chuck Geschke: Fri Oct 11 18:01:20 1985
Doug Brotz: Fri Jun  6 09:46:55 1986
Ed Taft: Sun Dec 17 19:23:45 1989
John Gaffney: Thu Jan  3 16:13:01 1985
Ivor Durham: Fri May 12 08:31:02 1989
Leo Hourvitz 16Sep87 Added optimization code for pkdarys, used register
      variables for stacks in WOODPECKER
Linda Gass: Wed Aug  5 16:24:29 1987
Paul Rovner: Wednesday, October 7, 1987 2:00:25 PM
Jim Sandman: Mon Oct 16 11:09:43 1989
Perry Caro: Fri Nov 10 13:13:53 1989
Joe Pasqua: Thu Jun 29 17:27:53 1989
Jack Newlin: 8Apr90 added WannaYield proc
End Edit History.
*/

#include PACKAGE_SPECS
#include COPYRIGHT
#include BASICTYPES
#include ERROR
#include EXCEPT
#include FP
#include GC
#include LANGUAGE
#include ORPHANS
#include STREAM
#include RECYCLER
#include VM

#include "dict.h"
#include "array.h"
#include "packedarray.h"
#include "exec.h"
#include "opcodes.h"
#include "langdata.h"
#include "languagenames.h"
#include "name.h"
#include "stack.h"
#include "scanner.h"
#include "streampriv.h"

/* These procs should be declared in a header file	*/
extern procedure StringInit(/* InitReason reason */);
extern procedure TypeInit(/* InitReason reason */);
extern procedure MathInit(/* InitReason reason */);

extern procedure ReInitFontCache();
  /* This is a circular reference to the font package	*/


/*	----- Context for the language package -----	*/
public PLanguageData languageCtxt;
public PPubLangCtxt pubLangCtxt;
/*	----- End Context for language package -----	*/

public PNameObj languageNames;


#if (OS != os_mpw)

/*-- BEGIN GLOBALS --*/

private integer (*GetNotifyAbortCode)();
private PVoidProc MonitorExit;
private PVoidProc YieldByRequest;
private PVoidProc YieldTimeLimit;
private long int *yieldLoc1, *yieldLoc2;
private int checkingYield;

/* The following objects are not per-context data. They are	*/
/* shared by all contexts.					*/
private CmdObj execcmd, stoppedcmd, reptcmd, intforcmd, runcmd;
private CmdObj realforcmd, loopcmd, xitcmd;
private int *dummyMark;

/*-- END GLOBALS --*/

#else (OS != os_mpw)

typedef struct {
 integer (*g_GetNotifyAbortCode)();
 PVoidProc g_MonitorExit;
 PVoidProc g_YieldByRequest;
 PVoidProc g_YieldTimeLimit;
 long int *g_yieldLoc1, *g_yieldLoc2;
 int g_checkingYield;

 CmdObj g_execcmd, g_stoppedcmd, g_reptcmd, g_intforcmd;
 CmdObj g_realforcmd, g_loopcmd, g_xitcmd, g_runcmd;

 int *g_dummyMark;
} GlobalsRec, *Globals;

private Globals globals;

#define GetNotifyAbortCode globals->g_GetNotifyAbortCode
#define MonitorExit globals->g_MonitorExit
#define YieldByRequest globals->g_YieldByRequest
#define YieldTimeLimit globals->g_YieldTimeLimit
#define yieldLoc1 globals->g_yieldLoc1
#define yieldLoc2 globals->g_yieldLoc2
#define checkingYield globals->g_checkingYield
#define execcmd globals->g_execcmd
#define stoppedcmd globals->g_stoppedcmd
#define reptcmd globals->g_reptcmd
#define intforcmd globals->g_intforcmd
#define realforcmd globals->g_realforcmd
#define loopcmd globals->g_loopcmd
#define xitcmd globals->g_xitcmd
#define	runcmd globals->g_runcmd
#define dummyMark globals->g_dummyMark
#endif (OS == os_mpw)


/*
   Definitions to provide easy access to language ctxt fields used by
   this module.
 */

#define	execAbortPending	(languageCtxt->execData._execAbortPending)
#define	execAbort		(languageCtxt->execData._execAbort)
#define	superExec		(languageCtxt->execData._superExec)
#define	execLevel		(languageCtxt->execData._execLevel)

/*
  Time-slicing declarations.
 */

public	PCard32	pTimeSliceClock;	/* Assumed to be NIL initially */
private Card32	unitClock;		/* Operator units if no real clock */
private	Card32	clockLimit;		/* Current bound on *pTimeSliceClock */
private	boolean	useRealClock;		/* True if free-running clock in use */

public procedure SetTimeLimit (limit)
  Card32 limit;
 /*
   Set state for new time slice.
  */
{
  clockLimit = limit;
  unitClock = 0;			/* Redundant if useRealClock is false */
}

#define	Inline_IncrementTimeUsed(units) \
  if (!useRealClock) { \
    *pTimeSliceClock += (units);\
  }

public procedure IncrementTimeUsed (units)
  Card32 units;
 /*
   Add units to the count of operator count only if no real clock is in use.
  */
{
  Inline_IncrementTimeUsed (units);
}

/*
  End of time-slicing definitions.
 */

public procedure PSSetYieldLocations(loc1, loc2)
  long int *loc1, *loc2;
  {
  yieldLoc1 = loc1;
  yieldLoc2 = loc2;
  }

public int PSSetYieldChecking(on)
  int on;
  {
  int lastValue = checkingYield;

  if (on != 0 && on != 1)
    return(-1);
  if (on == 1 && (yieldLoc1 == NIL || yieldLoc2 == NIL))
    return(-2);
  checkingYield = on;
  return(lastValue);
  }

public procedure PSError(e) NameObj e;
{
psERROR = e;
RAISE(PS_ERROR, (char *)NULL);
}  /* end of PSError */

public procedure BindArray(ao)
AryObj ao;
{
  integer i;
  Object ob;
  
  /* if not writable, do nothing */
  if ((ao.access & wAccess) == 0) return;

  for (i=0; i < ao.length; i++)
    {
    AGetP(ao, (cardinal)i, &ob);
    if (ob.tag != Xobj) continue; /* don't process non-executable object */
    switch (ob.type)
      {
      case arrayObj:
	BindArray(ob);
	ob.access &= ~wAccess;
	APut(ao,(cardinal)i,ob);
	break;
      case pkdaryObj:
	BindPkdary(ob);
	break;
      case nameObj:
	if (Load(ob, &ob) && ob.type == cmdObj)
	  APut(ao, (cardinal)i, ob);
	break;
      default: ;
      }
    }
}  /* end of BindArray */

public procedure PSBind()
{
Object ob;
PopP(&ob);
switch (ob.type)
  {
  case arrayObj:
    BindArray(ob); break;
  case pkdaryObj:
    BindPkdary(ob); break;
  default:
    TypeCheck();}
PushP(&ob);
}  /* end of PSBind */

private procedure UnwindExecStk(pob)  register PObject pob;
/* Unwinds execution stack, executing cleanup actions as needed (there
 * are none at present) until a command object with non-null
 * mark type has been popped, and returns that command object in *pob. */
{
until (execStk->head == execStk->base)
  {
  EPopP(pob);
  if (pob->type == cmdObj && pob->access != mrkNone) return;
  }
CantHappen();
/*NOTREACHED*/
}  /* end of UnwindExecStk */

private CarefulPushP(pob)  PObject pob;
{
Object discard;
if (opStk->head == opStk->limit) PopP(&discard);
PushP(pob);
}  /* end of CarefulPushP */

private procedure HandleStackOverflow(stack, rob)  PStack stack;  PObject rob;
/* Snapshots the stack that overflowed; then restores a usable execution
 * environment by clearing or cutting back that stack. Returns the
 * appropriate error operator for stack in pob. */
{
AryObj a; Object ob;
AllocPArray(CountStack(stack, MAXcardinal), &a);
ArrayFromStack(&a, stack);
if (stack == execStk)
  {
  do UnwindExecStk(&ob);
    until (ob.access == mrkExec
	   || ob.access == mrkStopped
	   || ob.access == mrkMonitor
	   || ob.access == mrkRun);
  EPushP(&ob);
  *rob = estkoverflow;
  }
else if (stack == opStk)
  {ClearStack(stack); *rob = stackoverflow;}
else if (stack == dictStk)
  {ClearDictStack(); *rob = dstkoverflow;}
else if (stack == refStk)
  {ClearStack(stack); *rob = stackoverflow;}
else CantHappen();
CarefulPushP(&a);
}  /* end of HandleStackOverflow */

#if VMINIT
private procedure EndWholeClothInit()
/* Terminates "whole cloth" initialization mode if an error occurs during
   VM initialization. This is required to permit the error handler to
   work correctly; otherwise, it goes into an infinite loop because
   "setshared" and TrickyDict semantics are suppressed.
 */
{
  if (vmShared->wholeCloth)
    {
    os_eprintf("\007Error being raised; whole cloth initialization terminated.\n");
    vmShared->wholeCloth = false;
    SetShared(false);
    AllocPArray (rootShared->trickyDicts.length, &rootPrivate->trickyDicts);
    VMCopyArray (rootShared->trickyDicts, rootPrivate->trickyDicts);
    }
}
#endif VMINIT

private ExecPushP(pob)  register PObject pob;
{
switch(pob->type)
  {
  case pkdaryObj: case arrayObj: case strObj: case stmObj:
    if ((pob->access & (xAccess|rAccess)) == 0) InvlAccess(); break;
  case cmdObj:
    if (pob->access != mrkNone) TypeCheck(); break;
  endswitch
  }
IPush(execStk, *pob);
}  /* end of ExecPushP */

public boolean WannaYield () {
  return (*pTimeSliceClock >= clockLimit
       || (checkingYield && *yieldLoc1 != *yieldLoc2) );
}

/* UHANDLER is the same as HANDLER except that the usual exception state
   restoration code is omitted because it cannot be reached. */
#define UHANDLER } else {

public boolean psExecute(obj)  Object obj;
  /* No Exceptions ever escape from this procedure. If an Exception is
   * pending, psExecute returns true and the Exception is left for the
   * next outer psExecute to handle (it is also available via GetAbort). */
  {
  Object ob, discard; register PObject obp;
  int mark = -1;
#if 	ANSI_C
  volatile int refStkMark = -1;
#else 	ANSI_C
  int refStkMark = -1;
#endif	ANSI_C
  int locOfErrObj = -1;
  StrObj rem; StmObj cstm;
  register PStack rExecStk = execStk, rOpStk = opStk;
  register cardinal code;
  boolean  errorOverflow; /* Needed during error handling */
  DURING
    if (execLevel >= MAXexecLevel) LimitCheck();
    IPush(rExecStk, execcmd);
    mark++;
    ExecPushP(&obj);
    mark = -1;
  HANDLER {
    if (mark != -1) UnwindExecStk(&obj);
    SetAbort((integer)Exception.Code); return true;}
  END_HANDLER;
  execLevel++;
  ChangeRecyclerExecLevel(execLevel, true);

  Assert ((YieldTimeLimit != NIL) && (pTimeSliceClock != NIL));

  while (true) { /* this outer loop allows Exceptions to be caught and
		    control to remain in the loop, while avoiding repeated
		    execution of DURING in normal (error-free) execution */
  DURING
  while (true) {
    if (execAbortPending) {
      integer abortCode = (*GetNotifyAbortCode)();
      if (abortCode == 0) {
        abortCode = execAbort; execAbort = 0;
        execAbortPending = 0;
	/* deal with the race: the above = 0 and async ++ */
	execAbort = (*GetNotifyAbortCode)();
	if (execAbort != 0)
	  execAbortPending++;
	}
      switch (abortCode) {
	case 0: break;
        case PS_INTERRUPT:
	  if (execAbort == 0)
	    psERROR = languageNames[nm_interrupt];

	  CrFile(&cstm);
	  if (EncryptedStream(cstm)) CloseFile(cstm, false);

          DictGetP(rootPrivate->trickyDicts.val.arrayval[tdErrorDict], psERROR, &ob);
          EPushP(&ob);
	  break;
        case PS_TIMEOUT:
	  CrFile(&cstm);
	  if (EncryptedStream(cstm)) CloseFile(cstm, false);
          DictGetP(rootPrivate->trickyDicts.val.arrayval[tdErrorDict],
	           languageNames[nm_timeout], &ob);
          EPushP(&ob);
	  break;
	case PS_YIELD:
          Assert (YieldByRequest != NIL);
	  (*YieldByRequest)();
	  break;
	case PS_EXIT:
          EPushP(&xitcmd);
	  break;
	case PS_DONE:
	  /* this is the normal termination of psExecute */
	  execLevel--;
	  ChangeRecyclerExecLevel(execLevel, false);
	  E_RETURN(false);
	default:
          RAISE((int)abortCode, (char *)NULL);}}

    while (*pTimeSliceClock >= clockLimit
       || (checkingYield && *yieldLoc1 != *yieldLoc2) )
      (*YieldTimeLimit)();

    if (vmCurrent->collectThisSpace)
      if (vmCurrent == vmShared)
        GC_CollectShared();
      else
        GC_CollectPrivate(vmPrivate);
        
    refStkMark = refStk->head - refStk->base;
    obp = rExecStk->head - 1;
    if (obp->tag == Lobj) {
#if 0
        PObject head = rOpStk->head;
	while (head < rOpStk->limit) { 
	    *head++ = *obp--;
	    if (obp->tag != Lobj) break;
	}
	rExecStk->head = obp + 1;
	rOpStk->head = head;
	if (obp->tag == Lobj) Overflow(rOpStk);
#else
	goto MoveToOpStack;
#endif
    }

    switch (obp->type)
      {
      case nullObj:
        StackPopDiscard(rExecStk); break;

      case strObj:
	if (! StrToken(*obp, &rem, &ob, true))
	  {EPopP(&discard); break;}
	if (rem.length != 0) *obp = rem;
	else EPopP(&discard);
	goto Xtoken;

      case stmObj:
	if (! StmToken(GetStream(*obp), &ob, true))
	  {
	  Stm stm;
          EPopP(&ob);
	  stm = GetStream(ob);
	  if (ferror(stm)) StreamError(stm);
	  CloseFile(ob, false);
	  break;
	  }
	/* The only objects normally returned by StmToken and StrToken
	   are executable name and array objects (however, other
	   executable objects can result from immediately evaluated
	   names). Executable array objects are returned only for
	   binary object sequences, which are subject to immediate
	   rather than deferred execution. */
      Xtoken:
        DebugAssert(ob.tag == Xobj);
	switch (ob.type)
	  {
	  case nameObj:
	    goto Xname;
	  case arrayObj:
	    IPush(rExecStk, ob);
	    continue;
	  default:
	    goto XXob;
	  }

      case pkdaryObj:
        {
	register Code *pCode = obp->val.pkdaryval;
	if (obp->length == 0)
	  { /* this happens only for an array that is empty to begin with */
	  EPopP(&discard);
	  break;
	  }
	while (true)
	  {
	  if (obp->length <= 1)
	    {
	    if (obp->length == 0) break;
	    IPopDiscard(rExecStk);
	    }
	  obp->length--;

	  switch (opType[code = *pCode++])
	    {
	    /* "break" means push object on opStk and go around again */
	    case ESCAPETYPE:
#if MINALIGN>1
	      if (((integer)pCode & (MINALIGN-1)) != 0)
	        os_bcopy((char *)pCode, (char *)&ob, sizeof(Object));
	      else
#endif MINALIGN>1
	        ob = *(PObject)pCode;
	      obp->val.pkdaryval += 1+sizeof(Object);
	      if (ob.tag != Lobj) goto Xob;
	      pCode += sizeof(Object);
	      break;
	    case LITNAMETYPE:
	      NameIndexObj(((code-LitNameBase) << 8) | *pCode++, &ob);
	      ob.tag = Lobj;
	      obp->val.pkdaryval += 2;
	      break;
	    case EXECNAMETYPE:
	      {
	      register PNameEntry pne;
	      code = ((code-ExecNameBase) << 8) | *pCode; /* name index */
	      obp->val.pkdaryval += 2;
	      for (pne = rootShared->vm.Shared.nameTable.val.namearrayval->
	                 nmEntry[code & rootShared->vm.Shared.hashindexmask];
		   pne != NIL; pne = pne->link)
		{
		if (code == pne->nameindex)
		  {
		  if (ILoadPNE(pne, &ob)) goto XnameVal;
		  NameIndexObj(code, &ob);
		  ob.tag = Xobj;
		  Undefined();
		  }
		}
	      CantHappen();
	      /* Can never get here, but the compiler doesn't know that.  The
		 following is to ensure that refStkMark isn't  in a register.
		 This  is important so  that if an   exception occurs and the
		 registers are restored, refStkMark won't revert to the value
		 it  had  when  the DURING was   executed.  We could  use the
		 'volatile' storage class if we had ANSI.  Don't combine this
		 code with   the piece  that  does the  same  thing   for the
		 variable 'mark'.   Some  compilers might  optimize away  the
		 effects of the first assignment in that case.		   */
#if	! ANSI_C
	      dummyMark = &refStkMark;
#endif	! ANSI_C
	      }
	    case CMDTYPE:
	      ob.length = ((code-CmdBase) << 8) | *pCode;
	      obp->val.pkdaryval += 2;
	      goto Xcmd;
	    case REALTYPE:
	      LRealObj(ob, encRealValues[code - RealBase]);
	      obp->val.pkdaryval += 1;
	      break;
	    case INTEGERTYPE:
	      LIntObj(ob, code - IntegerBase + MinInteger);
	      obp->val.pkdaryval += 1;
	      break;
	    case BOOLEANTYPE:
	      LBoolObj(ob, code - BooleanBase);
	      obp->val.pkdaryval += 1;
	      break;
#if false  /* "relative" packed arrays and strings disabled for now */
	    case RELSTRINGTYPE:
	      ob = *obp;  /* init object's fields to parent's values */
	      ob.tag = Lobj;
	      ob.type = strObj;
	      ob.access = aAccess;
	      goto relCommon;
	    case RELPKDARYTYPE:
	      ob = *obp;  /* init object's fields to parent's values */
	    relCommon:
#if MINALIGN==1
	      /***** incorrect if SWAPBITS *****/
	      code = *(PInt16)pCode;
	      pCode += sizeof (Int16);
#else MINALIGN==1
	      code = *pCode++;
	      code = (code << 8) | *pCode++;
#endif MINALIGN==1
	      ob.length = (code >> BitsForOffset) + MINArrayLength;
	      ob.val.pkdaryval +=
	        (((int)code + MINOffset) | (-1 << BitsForOffset));
	      obp->val.pkdaryval += 3;
	      break;  /* PS semantic glitch (see comment at Xob:) */
#endif false
	    default:
	      CantHappen();
	      /* Can never get here, but the compiler doesn't know that.
	         Following is to defeat C compiler's global flow analysis
	         incorrectly deducing that "mark" is never used. */
	      dummyMark = &mark;
	    }
	  IPush(rOpStk, ob);
	  }
	break;
	}

      case arrayObj:
	while (true)
	  {
	  if (obp->length > 1)
	    {
	    ob = *obp->val.arrayval;
	    obp->length--;
	    obp->val.arrayval++;
	    }
	  else
	    {
	    IPopDiscard(rExecStk);
	    if (obp->length == 0) break;
	    ob = *obp->val.arrayval;
	    goto Xob;
	    }
	  if (ob.tag != Lobj) goto XXob;
	  IPush(opStk, ob);
	  }
	break;

      Xob:
        if (ob.tag != Lobj)
	XXob:
	  switch (ob.type)
	    {
	    case nameObj: goto Xname;
	    case cmdObj: goto Xcmd;
	    case pkdaryObj: case arrayObj:
	      /* PostScript semantic glitch: executable arrays encountered
	         in arrays, streams, and strings are not executed but are
	         pushed onto the operand stack. This is where it happens! */
	      break;		/* that is, fall out bottom of switch */
	    case stmObj: case strObj:
	      if ((ob.access & (rAccess|xAccess)) == 0) InvlAccess();
	      /* fall through */
	    default:
	      IPush(rExecStk, ob); continue;
	    }
	IPush(rOpStk, ob);
	break;

      case nameObj:
	IPopNotEmpty(rExecStk, &ob);
      Xname:
        {
	register PNameEntry pne = ob.val.nmval;
	if (!ILoadPNE(pne, &ob)) Undefined();
	}
      XnameVal:
	if (ob.tag != Lobj)
	  switch(ob.type)
	    {
	    case cmdObj: goto Xcmd;
	    case strObj: case stmObj: case arrayObj: case pkdaryObj:
	      if ((ob.access & (xAccess|rAccess)) == 0) InvlAccess();
	      /* fall through */
	    default: IPush(rExecStk, ob); continue;
	    }
      PushOpStk:
	IPush(rOpStk, ob);
	break;

      case cmdObj:
	IPopNotEmpty(rExecStk, &ob);
      Xcmd:
	mark = rOpStk->head - rOpStk->base;
#if VMINIT
	currentCmd = ob.length;  /* for opdef */
#endif VMINIT
	(*cmds.cmds[ob.length])();
	mark = -1;
	refStk->head = refStk->base + refStkMark;
	Inline_IncrementTimeUsed (1);	/* No-op if using real clock */
	break;

      default:  /* other types non-executable, put on operand stack */
      MoveToOpStack:
        IPush(rOpStk, *obp); IPopDiscard(rExecStk);
      }
    }
  UHANDLER
    if (refStkMark != -1)
      refStk->head = refStk->base + refStkMark;

    switch (Exception.Code){
      case ecInvalidAccess:
        psERROR = invlaccess; Exception.Code = PS_ERROR; break;
      case ecInvalidFileAccess:
        psERROR = invlflaccess; Exception.Code = PS_ERROR; break;
      case ecIOError:
        psERROR = ioerror; Exception.Code = PS_ERROR; break;
      case ecLimitCheck:
        psERROR = limitcheck; Exception.Code = PS_ERROR; break;
      case ecRangeCheck:
        psERROR = rangecheck; Exception.Code = PS_ERROR; break;
      case ecUndef:
        psERROR = undefined; Exception.Code = PS_ERROR; break;
      case ecUndefFileName:
        psERROR = undeffilename; Exception.Code = PS_ERROR; break;
      case ecUndefResult:
        psERROR = undefresult; Exception.Code = PS_ERROR; break;
      case ecInvalidID:
	psERROR = invlid; Exception.Code = PS_ERROR; break;
    }

    switch (Exception.Code){
      case PS_REINITCACHE:{
	ReInitFontCache();
	psERROR = ioerror;}
	/* fall through as if this code had executed PSError(ioerror) */
      case PS_ERROR:{
	PKeyVal SDres; Object errorob;
	if (stackRstr){
	  if (mark != -1){
	    /* reconstitute command which might have come from packed array */
	    CmdIndexObj(ob.length, &ob);
	    RstrStack(rOpStk, rOpStk->base + mark);
	    mark = -1;
	    }
	  CarefulPushP(&ob);}  /* push error-causing object */
	stackRstr = true;
	superExec = 0;
	CrFile(&cstm);
	if (EncryptedStream(cstm)) CloseFile(cstm, false);
#if VMINIT
	EndWholeClothInit();
#endif VMINIT
	if (!SearchDict(rootPrivate->trickyDicts.val.arrayval[tdErrorDict].val.dictval, psERROR, &SDres)){
	  CarefulPushP(&psERROR);
	  if (!SearchDict(rootPrivate->trickyDicts.val.arrayval[tdErrorDict].val.dictval, undefined, &SDres))
	    CantHappen();}
	VMGetValue(&errorob, SDres);
	errorOverflow = false;
	if (rExecStk->head < rExecStk->limit) /* Definitely OK to push */
	  EPushP(&errorob);  /* push error */
	else /* must be cagey about how we push it */
	  {
	  DURING
	    EPushP(&errorob);
	  HANDLER
	    errorOverflow = true;
	  END_HANDLER
	  }
	if (!errorOverflow)
	  {
          execAbort = 0;
	  continue;
	  }
	/* else errorOverflow is true, so fall through to... */
	}
      case PS_STKOVRFLW:
#if VMINIT
	EndWholeClothInit();
#endif VMINIT
        HandleStackOverflow(psFULLSTACK, &psERROR);
	SetAbort((integer)PS_ERROR);
	mark = -1;
	continue;		/* around again to invoke the error op */
      case PS_STOP:
        while (true) {
          UnwindExecStk(&ob);
	  switch (ob.access) {
	    case mrkExec:
	      SetAbort((integer)PS_STOP); goto doneAbort;
	    case mrkMonitor: {
	      Assert (MonitorExit != NIL);
	      (*MonitorExit)();
	        /* pop a lock object off estk and release the lock */
	      continue;
	    }
	    case mrkStopped:
	      DURING
	        PushBoolean(true);
		execAbort = 0;
	      HANDLER
	        SetAbort((integer)Exception.Code);
	          /* leave it for main loop to see */
	      END_HANDLER;
	      break;
	    case mrkRun:
	      {
	        extern procedure CRun();	/* forward declaration */
	        CRun();				/* Clean up and continue */
	        continue;
	      }
	    default: continue;
	    }
	  break;
	  }
	break;
      case PS_EXIT:
        ETopP(&ob);
	if (ob.type == cmdObj && ob.access == mrkExec)
	  {  /* exit interpreter after exec stack already unwound */
	  EPopP(&ob);
	  SetAbort((integer)PS_EXIT);
	  goto doneAbort;
	  }
	else  /* process PS_EXIT reraised as exception */
	  SetAbort((integer)PS_EXIT);
	continue;
      case PS_INTERRUPT:
        SetAbort((integer)PS_INTERRUPT); continue;
      case PS_TIMEOUT:
        SetAbort((integer)PS_TIMEOUT); continue;
      case PS_DONE:
        CantHappen();		/* should never be raised as an exception */
      default:
        SetAbort((integer)Exception.Code); /* must have been a disk error */
	goto doneAbort;}
  END_HANDLER;}
doneAbort:
  execLevel--;
  ChangeRecyclerExecLevel(execLevel, false);
  return true;
  }

public integer GetAbort()
{
  return execAbort;	/* Not one line def to help debugger distinguish */
}

public procedure SetAbort(reason) integer reason; {
  execAbort = reason;
  if (reason != 0) execAbortPending++;
  }

public procedure NotifyAbort ()
 /*
   Can be called asynchronously (see NotifyPSContext) on current context.
  */
{
  execAbortPending++;
}

public procedure PSStop()
{
  execAbort = PS_STOP;
  execAbortPending++;
}

private procedure PSStopped()
{
Object ob; PopP(&ob);
EPushP(&stoppedcmd);
ExecPushP(&ob);
}  /* end of Stopped */

private procedure CStopped() {PushBoolean(false);}

public procedure PSExec()
{
Object ob; PopP(&ob);
ExecPushP(&ob);
}  /* end of Exec */

private procedure CExec() {execAbort = PS_DONE; execAbortPending++;}

public procedure PSIf()
{
AryObj ob;
PopPArray(&ob);
if (PopBoolean()) ExecPushP(&ob);
}  /* end of If */

public procedure PSIfElse()
{
AryObj obF,obT;
PopPArray(&obF);
PopPArray(&obT);
ExecPushP((PopBoolean()) ? &obT : &obF);
}  /* end of IfElse */

public procedure PSRepeat()
{
AryObj ob; integer i;
PopPArray(&ob);
if ((i = PopInteger()) < 0) RangeCheck();
ExecPushP(&ob);
EPushInteger(i);
EPushP(&reptcmd);
}  /* end of Repeat */

private procedure CRepeat()
{
  integer i;
  i = EPopInteger();
  if (i > 0) {
    int placeHolder = execStk->head - execStk->base - 1;
    EPushInteger(i-1);
    IPush(execStk, reptcmd);
    IPush(execStk, *(execStk->base + placeHolder));}
  else IPopDiscard(execStk);
}

private procedure PopPNumber(pob)  PObject pob;
{
PopP(pob);
switch (pob->type)
  {
  case intObj:
  case realObj: return;
  default: TypeCheck();
  }
}  /* end of PopPNumber */

public procedure PSFor()
{
AryObj ob; Object iob,job,kob;
PopPArray(&ob);
PopPNumber(&kob);
PopPNumber(&job);
PopPNumber(&iob);
ExecPushP(&ob);
if ((iob.type == realObj) || (job.type == realObj))
  {
  real i,j,k;
/*
 * Type conversion problems in MPW require expanded form below. (id)
 */

  if (kob.type == realObj) k = kob.val.rval; else k = kob.val.ival;
  if (job.type == realObj) j = job.val.rval; else j = job.val.ival;
  if (iob.type == realObj) i = iob.val.rval; else i = iob.val.ival;
  EPushPReal(&k);
  EPushPReal(&j);
  EPushPReal(&i);
  EPushP(&realforcmd);
  }
else
  {
  integer k;
  if (kob.type == realObj) k = kob.val.rval;
  else k = kob.val.ival;
  EPushInteger(k);
  EPushInteger(job.val.ival);
  EPushInteger(iob.val.ival);
  EPushP(&intforcmd);
  }
}  /* end of For */

private procedure CRFor()
{
real i,j,k; Object ob;
EPopPReal(&i);
EPopPReal(&j);
EPopPReal(&k);
ETopP(&ob);
if ((RealGt0(j)) ? (i > k) : (i < k)) EPopP(&ob);
else
  {
  EPushPReal(&k);
  EPushPReal(&j);
  j += i;
  EPushPReal(&j);
  EPushP(&realforcmd);
  PushPReal(&i);
  EPushP(&ob);
  }
}  /* end of CRFor */

private procedure CIFor()
{
integer i,j,k; Object ob;
i = EPopInteger();
j = EPopInteger();
k = EPopInteger();
ETopP(&ob);
if ((j > 0) ? (i > k) : (i < k)) EPopP(&ob);
else
  {
  EPushInteger(k);
  EPushInteger(j);
  EPushInteger(i+j);
  EPushP(&intforcmd);
  PushInteger(i);
  EPushP(&ob);
  }
}  /* end of CIFor */

public procedure PSLoop()
{
AryObj ob;
PopPArray(&ob);
ExecPushP(&ob);
EPushP(&loopcmd);
}  /* end of Loop */

private procedure CLoop()
{
  int placeHolder = execStk->head - execStk->base - 1;
  IPush(execStk, loopcmd);
  IPush(execStk, *(execStk->base + placeHolder));
}

public procedure PSRun()
 /*
   Open file and push onto the execution stack.  Push the stream both
   above the @run mark and below it.  The one above is used for closing
   the file when we come to execute @run.
  */
{
  StrObj  str;
  StmObj  stm;
  boolean shared;

  PopPRString (&str);
  shared = CurrentShared ();
  DURING {
    SetShared (false);
    CreateFileStream (str, (string) "r", &stm);
  }
  HANDLER {
    SetShared (shared);
    RERAISE;
  }
  END_HANDLER;
  SetShared (shared);
  stm.tag = Xobj;

  EPushP(&stm);		/* Saved for closing when exit executed in error */
  EPushP(&runcmd);	/* End of "(filename) run" */
  EPushP(&stm);		/* Stream to be executed now */
}

/*private*/ procedure CRun()	/* Omit "private" for earlier forward decl. */
{
  Object ob;
  EPopP (&ob);
  if (ob.type != nullObj)  /* may have been zapped by restore */
    {
    DebugAssert (ob.type == stmObj);
    CloseFile (ob, false);
    }
}

public procedure PSExit()
{
Object ob, discard;
integer i;

while (true) {
  UnwindExecStk(&ob);
  switch (ob.access)
    {
    case mrkExec:
      EPushP(&ob); RAISE(PS_EXIT, (char *)NULL); /* never returns */
    case mrkStopped:
      EPushP(&ob); PSError(invlexit); /* never returns */
    case mrkMonitor:
      Assert (MonitorExit != NIL);
      (*MonitorExit)();
        /* pop a lock object off estk and release the lock */
      break;
    case mrkRun:
      CRun();		/* Handle normal cleanup before reporting error */
      PSError(invlexit);/* never returns */
    case mrk1Arg:
      EPopP(&discard);
      return;
    case mrk2Args:
      EPopP(&discard);
      EPopP(&discard);
      return;
    case mrk4Args:
      for (i = 4; i > 0; i--)
        EPopP(&discard);
      return;
    default:
      CantHappen();
    }
  }
}  /* end of Exit */

private procedure ClearExecStack()
{
  Object  ob,
          discard;
  integer i;

  until (execStk->head == execStk->base) {
    EPopP (&ob);
    if (ob.type == cmdObj && ob.access != mrkNone) {
      switch (ob.access) {
       case mrkMonitor:
	Assert (MonitorExit != NIL);
	(*MonitorExit) ();
	/* pop a lock object off estk and release the lock */
	break;
       case mrkRun:
	CRun();	/* Clean up */
	break;
       case mrkExec:
       case mrkStopped:
       case mrk1Arg:
       case mrk2Args:
       case mrk4Args:
         break;
       default:
	CantHappen();
      }
    }
  }
}				/* end of ClearExecStack */

private procedure PSSuperExec()
{
Object ob;
PopP(&ob);
superExec++;
(void) psExecute(ob);
if (--superExec < 0) superExec = 0;
}

public procedure InvlAccess() {if (superExec == 0) PSError(invlaccess);}

public procedure RgstContextProcs (monitorProc, yieldRequestProc, yieldTimeLimitProc, getNotifyCodeProc)
  PVoidProc monitorProc, yieldRequestProc, yieldTimeLimitProc;
  integer (*getNotifyCodeProc)();
{
  MonitorExit = monitorProc;
  YieldByRequest = yieldRequestProc;
  YieldTimeLimit = yieldTimeLimitProc;
  GetNotifyAbortCode = getNotifyCodeProc;
}

public procedure ExecInit(reason)  InitReason reason;
{
switch (reason)
  {
  case init:
    useRealClock = (pTimeSliceClock != NIL);
    if (!useRealClock)
      pTimeSliceClock = &unitClock;
    break;
  case romreg:
    RgstExplicit("stopped", PSStopped);  /* needed to get off the ground */
    RgstInternal("@exit", PSExit, &xitcmd);
    RgstMark("@exec", CExec, (integer)mrkExec, &execcmd);
    RgstMark("@stopped", CStopped, (integer)mrkStopped, &stoppedcmd);
    RgstMark("@loop", CLoop, (integer)(mrk1Arg), &loopcmd);
    RgstMark("@repeat", CRepeat, (integer)(mrk2Args), &reptcmd);
    RgstMark("@ifor", CIFor, (integer)(mrk4Args), &intforcmd);
    RgstMark("@rfor", CRFor, (integer)(mrk4Args), &realforcmd);
    RgstMark("@run", CRun, (integer)mrkRun, &runcmd);
    Begin(rootShared->trickyDicts.val.arrayval[tdErrorDict]);
    End();
    Begin(rootShared->vm.Shared.internalDict);
    RgstExplicit("superexec", PSSuperExec);
    End();
    break;
  endswitch
  }
}  /* end of ExecInit */

#define	LANGUAGESTATICEVENTS \
  STATICEVENTFLAG(staticsCreate) | \
  STATICEVENTFLAG(staticsLoad) | \
  STATICEVENTFLAG(staticsUnload) | \
  STATICEVENTFLAG(staticsDestroy) | \
  STATICEVENTFLAG(staticsSpaceDestroy)

private procedure LanguageDataHandler (code)
  StaticEvent code;
 /*
   Handle creation, destruction, loading, and destruction of static data.
  */
{
  switch (code) {
   case staticsCreate:
    stackRstr = true;			/* for exec.c */
    curStackLimit = STKSIZELIMIT;	/* for stack.c */
    echo = (OS == os_ps);		/* for stodevedit.c */
    StmCtxCreate ();			/* for stream.c */
    randx = 1;				/* for math.c */
    objectFormat = (IEEEFLOAT) ?	/* for binaryobject.c */
      ((SWAPBITS) ? of_lowIEEE : of_highIEEE) :
      ((SWAPBITS) ? of_lowNative : of_highNative);
    /* Stacks created separately; initialisation sequencing issues */
    languageCtxt->stackData._opStk = opStk;
    languageCtxt->stackData._execStk = execStk;
    languageCtxt->stackData._dictStk = dictStk;
    languageCtxt->stackData._refStk = refStk;
    languageCtxt->dictData._timestamp = timestamp;
    break;

   case staticsLoad:
    opStk = languageCtxt->stackData._opStk;
    execStk = languageCtxt->stackData._execStk;
    dictStk = languageCtxt->stackData._dictStk;
    refStk = languageCtxt->stackData._refStk;
    timestamp = languageCtxt->dictData._timestamp;
    break;

   case staticsUnload:
    languageCtxt->dictData._timestamp = timestamp;
    break;

   case staticsDestroy:
    /* StmCtxDestroy is called by DoQuit because staticDestroy not executed
       by context coroutine and we must avoid yielding after terminating. */
    DictCtxDestroy();

    if (opStk != NIL) {		/* Check in case context create failed */
      ClearStack (opStk);
      KillStack (opStk);
    }
    if (dictStk != NIL) {
      ClearStack (dictStk);
      KillStack (dictStk);
    }
    if (refStk != NIL) {
      ClearStack (refStk);
      KillStack (refStk);
    }
    if (execStk != NIL) {
      ClearExecStack (execStk);
      KillStack (execStk);
    }
    break;

   case staticsSpaceDestroy:
    DestroyNameMap ();
    break;
  }
}

/*	----> GC Support Routines <----		*/
/* The following procs are required for correct	*/
/* operation of the Garbage Collector.		*/

private procedure PushLangRoots(clientData, info)
/* This proc is responsible for pushing all	*/
/* roots that are held by the language package.	*/
RefAny clientData;
GC_Info info;
{
  if (GC_GetCollectionType(info) != sharedVM)
    {
    PushStackRoots(info);
    PrivateStreamRoots(info);
    }
}

/*	-----------------------------------	*/
/*	----> END GC Support Routines <----	*/
/*	-----------------------------------	*/

/*	----> UserObject Routines <----		*/

private procedure PSDefUserObj() {
  Object ob, uObj;
  integer index;
  cardinal newSize = 0;
  PopP(&ob);
  index = PopInteger();
  if (index < 0 || index >= MAXarrayLength) RangeCheck();
  if (!DictTestP(rootPrivate->trickyDicts.val.arrayval[tdUserDict],
   		 languageNames[nm_UserObjects], &uObj, true)) {
    uObj.length = 0;
    newSize = (index > UOBJINITSIZE) ? index + 1 : UOBJINITSIZE;
    }
  else {
    if (uObj.length <= index) {
      unsigned long l = uObj.length * 2L;
      newSize = (l <= MAXarrayLength && l > index) ? l : index + 1;
      }
    }
  if (newSize != 0) {
    Object newUObj;
    Object tmpobj;
    cardinal i;  
    boolean origShared = CurrentShared();
    SetShared(false);
    DURING
      AllocPArray(newSize, &newUObj);
    HANDLER
      SetShared(origShared); RERAISE;
    END_HANDLER;
    SetShared(origShared);
    for (i = 0; i < uObj.length; i++) {
		tmpobj = VMGetElem(uObj,i);
    	APut(newUObj, i, tmpobj);
    }

    ConditionalInvalidateRecycler (&newUObj);	/* Preclude move by DictPut */

    DictPut(
      rootPrivate->trickyDicts.val.arrayval[tdUserDict],
      languageNames[nm_UserObjects],
      newUObj);
    uObj = newUObj;
    }
  APut(uObj, (cardinal)index, ob);    
  return;
  }

public procedure PSUndefUserObj() {
  Object ob, uObj;
  integer index = PopInteger();
  if (!DictTestP(
      rootPrivate->trickyDicts.val.arrayval[tdUserDict],
      languageNames[nm_UserObjects], &uObj, true) ||
      index < 0 || index >= uObj.length)
    RangeCheck();
  LNullObj(ob);
  APut(uObj, (cardinal)index, ob);
  }
 
public procedure PSExecUserObj() {
  Object ob, uObj;
  integer index = PopInteger();
  if (!DictTestP(
      rootPrivate->trickyDicts.val.arrayval[tdUserDict],
      languageNames[nm_UserObjects], &uObj, true) ||
      index < 0 || index >= uObj.length)
    RangeCheck();
  LNullObj(ob);
  ob = VMGetElem(uObj, index);
  if(ob.tag == Xobj)
    ExecPushP(&ob);
  else
    PushP(&ob);
  return;
  }
 
/*	-----------------------------------	*/
/*	----> END UserObject Routines <----	*/
/*	-----------------------------------	*/


public procedure LanguageInit(reason)
  InitReason reason;
{
  switch (reason) {
   case init:
#if OS==os_mpw
    globals = (Globals)os_sureCalloc(sizeof(GlobalsRec),1);
#endif OS==os_mpw
    checkingYield = 0; yieldLoc1 = yieldLoc2 = NIL;
    RegisterData(
      (PCard8 *)&languageCtxt, (integer)sizeof(LanguageData),
      LanguageDataHandler, (integer)LANGUAGESTATICEVENTS);
    RegisterData(
      (PCard8 *)&pubLangCtxt, (integer)sizeof(PubLangCtxt),
      (PVoidProc)NIL, (integer)0);
    GC_RgstGetRootsProc(PushLangRoots, (RefAny)NIL);
    GC_RgstSharedRootsProc(PushLangRoots, (RefAny)NIL);
    break;
   case romreg:
      languageNames = RgstPackageNames((integer)PACKAGE_INDEX, (integer)NUM_PACKAGE_NAMES);
#include "ops_languageDPS.c"
    RgstExplicit("run", PSRun);  /* needed to get off the ground */
    break;
  }
  NameInit (reason);
  DictInit (reason);
  ScannerInit (reason);
  BinObjInit (reason);
  ArrayInit (reason);
  PkdaryInit (reason);
  ExecInit (reason);
  StackInit (reason);
  StreamInit (reason);
  StringInit (reason);
  TypeInit (reason);
  MathInit (reason);
}

