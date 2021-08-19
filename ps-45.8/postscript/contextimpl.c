/*
  contextimpl.c

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

Original version: Ivor Durham: Tue Apr 19 21:09:27 1988
Edit History:
Larry Baer: Fri Nov 17 10:07:37 1989
Ivor Durham: Thu May 11 11:37:52 1989
Paul Rovner: Wednesday, May 11, 1988 8:41:07 AM
Leo Hourvitz: Wed Jul 12 04:00:00 1988
Ed Taft: Sun Dec 17 19:34:32 1989
Perry Caro: Tue Nov 15 12:05:35 1988
Jim Sandman: Fri Nov  3 16:28:54 1989
Joe Pasqua: Mon Feb  6 11:33:30 1989
Mark Francis: Mon Nov 13 18:25:00 1989
End Edit History.

Loose ends:
  put in more assertions
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include FONTS
#include GRAPHICS
#include LANGUAGE
#include ORPHANS
#include POSTSCRIPT
#include PSLIB
#include RECYCLER
#include SIZES
#include VM

extern procedure NewDevice();
  /* This procedure isn't exported from the graphics	*/
  /* package, so its redeclared here.			*/
extern PVM CreatePrivateVM();
  /* This should be imported through some interface	*/

typedef struct _t_Lock { /* concrete representation for a lock object */
  GenericBody header;
  PSContext waiters; /* The list of pscontexts waiting to acquire this lock */
  PSContext holder;  /* the PSContext holding this lock. NIL => lock is free */
  } Lock;

typedef struct _t_Condition { /* concrete representation for a condition object */
  GenericBody header;
  PSContext waiters; /* the list of pscontexts waiting on this condition */
  } Condition;

/* concrete definition for the opaque type PSSpace */
typedef struct _t_PSSpaceRec {
  PSSpace   next;	 /* link in list of spaces */
  SpaceID   id;
  PSContext contexts;    /* -> first in list of contexts in this space */
  PSContext exclusive;   /* non-NIL if this context is the only one that
                               can be run in this space */
  integer semaphore;	 /* exclusive access semaphore (counter) */
  PVM privateVM;
  Condition joinable;
      /* contexts of this space park here while awaiting some other to terminate */
  } PSSpaceRec;

 /* The concrete definition for the opaque type PSKernelContext.
    Each PSContext has one of these */
 typedef struct _t_PSKernelContextRec {
  ContextID    id;
  PSContext nextInSpace;/* next link in list of contexts in space */
  PCard8 staticData;	/* used by UnloadData, CreateData, LoadData */
  Object startup;	/* Passed to psExecute to start this context running */

  BitField detached: 1,	/* true => this context has been detached */
  	readyForJoin: 1,/* true if this context has returned from its startup
			   object and has not yet been detached or joined */
	joined: 1,	/* true between the time a readyForJoin context gets
			   joined and the time it next gets to run, when it will
			   terminate */
	terminating: 1,	/* set true by DoQuit while this context is terminating.
			   Disables notify processing */
	joinerWaiting: 1;/* true if another context is waiting to join this one */

  PStack opStk;		/* Valid if readyForJoin; to be copied by the joiner */
  cardinal opStkLn;	/* length of opStk to be copied by the joiner */
  Int32 disableCount;	/* disable count for interrupt and timeout */

  PLock lk;		/* non-NIL if reason == yield_wait and this context
			   is waiting to acquire this lock */
  PSContext nextPatron;	/* next link in the lk->waiters list.
			   Meaningful only if lk is non-NIL */

  PCondition waitee;	/* non-NIL if reason == yield_wait and this context
			   is waiting on this Condition */
  PSContext nextWaiter;	/* next link in the waitee->waiters list.
			   Meaningful only if waitee is non-NIL */
  } PSKernelContextRec;


#define initCtxTabSize 100
/* Support for the map: id <==> PSContext or PSSpace
   initCtxTabSize = the total number of active contexts + active spaces */
typedef struct { /* element of the map */
  GenericID timestamp;
  PSContext ctx;
  } CtxItem;


/* Private Data */

#if (OS != os_mpw)
/*-- BEGIN GLOBALS --*/

private PSSpace spaces;			 /* list of all spaces */
private PSContext exclusivePSContext;	/* Only context allowed to run */
private integer exclusiveContextSemaphore;
private NameObj monitorcmd;
  /* NameObj used in RgstMark call for "monitormark"	*/
  /* This object is shared by all contexts.		*/

private integer nextID, nextFreeID, gTimestamp;
private integer ctxTabSize;
private CtxItem *ctxTab;
private Object forkStartup; /* precompute invariant PostScript proc object */

/*-- END GLOBALS --*/
#else (OS != os_mpw)

typedef struct {
 PSSpace g_spaces;			 /* list of all spaces */
 PSContext g_exclusivePSContext;	/* Only context allowed to run */
 integer g_exclusiveContextSemaphore;

 NameObj g_monitorcmd;
  /* NameObj used in RgstMark call for "monitormark"	*/
  /* This object is shared by all contexts.		*/
  
/* support for context and space IDs */
 integer g_nextID, g_nextFreeID, g_timestamp;
 integer g_ctxTabSize;
 CtxItem *g_ctxTab;

 Object g_forkStartup; /* precompute invariant PostScript proc object */
} GlobalsRec, *Globals;

private Globals globals;

#define spaces globals->g_spaces
#define exclusivePSContext globals->g_exclusivePSContext
#define exclusiveContextSemaphore globals->g_exclusiveContextSemaphore
#define monitorcmd globals->g_monitorcmd
#define nextID globals->g_nextID
#define nextFreeID globals->g_nextFreeID
#define gTimestamp globals->g_timestamp
#define ctxTabSize globals->g_ctxTabSize
#define ctxTab globals->g_ctxTab
#define forkStartup globals->g_forkStartup

#endif (OS != os_mpw)

/* EXTERNs (XXX others too) */
extern DictObj trickyUserDict;
extern procedure NotifyAbort();

private procedure InvalidContext ()
{
  PSError (invlcontext);
}

/* ***************************************************** */
/* Procedures for managing context and space ID's */

private procedure RecycleID(id) GenericID id; {
  integer i = id.id.index;
  ctxTab[i].ctx = NIL;
  ctxTab[i].timestamp.stamp = nextFreeID;
  nextFreeID = i;
  gTimestamp++;
  }

/* modification to remove hard limits on # contexts */
#define NEXT_MOD 1

private procedure NewContextID(ctx) PSContext ctx; {
  integer i = nextFreeID;
  if (i == 0) { /* get a new id */
    i = nextID;
    if (i >= ctxTabSize) {
#if NEXT_MOD
	/* grow ctxTab, zeroing out the new data*/
	ctxTab = realloc(ctxTab, 2*ctxTabSize*sizeof(CtxItem));
	os_bzero((char *)ctxTab + ctxTabSize*sizeof(CtxItem),
		 ctxTabSize*sizeof(CtxItem));
	ctxTabSize += ctxTabSize; /* exponentially grow size */
#else
      LimitCheck();
#endif
      }
    nextID++;
    }
  else { /* get a recycled id */
    nextFreeID = ctxTab[i].timestamp.stamp;
    }

  ctx->kernel->id.id.index = i;
  ctx->kernel->id.id.generation = gTimestamp;
  ctxTab[i].timestamp = ctx->kernel->id;

  Assert(ctxTab[i].ctx == NIL);
  ctxTab[i].ctx = ctx;
  }
  
/* KLUDGE */
private procedure NewSpaceID(s) PSSpace s; {
  integer i = nextFreeID;
  if (i == 0) { /* get a new id */
    i = nextID;
    if (i >= ctxTabSize) {
#if NEXT_MOD
	/* grow ctxTab, zeroing out the new data*/
	ctxTab = realloc(ctxTab, 2*ctxTabSize*sizeof(CtxItem));
	os_bzero((char *)ctxTab + ctxTabSize*sizeof(CtxItem),
		 ctxTabSize*sizeof(CtxItem));
	ctxTabSize += ctxTabSize; /* exponentially grow size */
#else
      LimitCheck();
#endif
      }
    nextID++;
    }
  else { /* get a recycled id */
    nextFreeID = ctxTab[i].timestamp.stamp;
    }

  s->id.id.index = i;
  s->id.id.generation = gTimestamp;
  ctxTab[i].timestamp = s->id;

  Assert(ctxTab[i].ctx == NIL);
  ctxTab[i].ctx = (PSContext)s;
  }
  
/* ***************************************************** */
/* Utility procedures */

private procedure BreakLooseCV(ctx) PSContext ctx; {
  PSContext c, prev = NIL;
  for (c = ctx->kernel->waitee->waiters; c != NIL; c = c->kernel->nextWaiter) {
    if (c == ctx) {
      if (prev == NIL) ctx->kernel->waitee->waiters = c->kernel->nextWaiter;
      else prev->kernel->nextWaiter = c->kernel->nextWaiter;
      return;
      }
    else prev = c;
    }
  }

private procedure BreakLooseLK(ctx) PSContext ctx; {
  PSContext c, prev = NIL;
  for (c = ctx->kernel->lk->waiters; c != NIL; c = c->kernel->nextPatron) {
    if (c == ctx) {
      if (prev == NIL) ctx->kernel->lk->waiters = c->kernel->nextPatron;
      else prev->kernel->nextPatron = c->kernel->nextPatron;
      return;
      }
    else prev = c;
    }
  }

private procedure WaitOnJoinable(target) PSContext target; {
  /* Park here, waiting to join with target */
  /* called only from PSJoin */
  PSContext ctx = currentPSContext;
  /* wait on ctx->space->joinable */
  ctx->kernel->waitee = &ctx->space->joinable;
  ctx->kernel->nextWaiter = ctx->space->joinable.waiters;
  ctx->space->joinable.waiters = ctx;
  target->kernel->joinerWaiting = true;
  DURING {
    PSYield(yield_wait, (char *)NIL);	/* May raise exception */
  } HANDLER {
    Assert(ctx == currentPSContext);
    if (ctx->kernel->waitee != NIL) BreakLooseCV(ctx);
    RERAISE;
  } END_HANDLER;
  Assert(ctx == currentPSContext);
  if (ctx->kernel->waitee != NIL) BreakLooseCV(ctx);
  }
  
private procedure InnerNotify (c) PCondition c; {
  PSContext ctx = c->waiters;
  c->waiters = NIL;
  for (; ctx != NIL; ctx = ctx->kernel->nextWaiter) {
    ctx->kernel->waitee = NIL;
    PSMakeRunnable(ctx);
    }
  }

private procedure DoQuit() {
  PSContext ctx = currentPSContext;
  ctx->kernel->terminating = true; /* disable notify processing */
  ctx->notified = false;
  
  /* let prospective joiners run */
  if (ctx->kernel->joinerWaiting) InnerNotify(&ctx->space->joinable);

  PSYield(yield_terminate, (char *)NIL);
  CantHappen();
  }

private procedure PSCoProc() { /* passed as an arg to CreateSchedulerContext */
  PSContext ctx = currentPSContext;

  os_stdin = ctx->in;
  os_stdout = ctx->out;

  if (psExecute(ctx->kernel->startup)) {
    Assert(GetAbort() == PS_TERMINATE);
    StmCtxDestroy();
    }
  else if (ctx->kernel->detached) {
    StmCtxDestroy();
    }
  else {
    Assert(!ctx->kernel->joined);
    ctx->kernel->readyForJoin = true; /* this disables CheckForPSNotify */
    /* Remember opStk and its length */
    ctx->kernel->opStk = opStk;
    ctx->kernel->opStkLn = CountStack (opStk, (cardinal)MAXCard16);
    StmCtxDestroy();
    do {
      if (ctx->kernel->joinerWaiting) InnerNotify(&ctx->space->joinable);

      if (level > 0) {
	/* If context is exiting with unrestored save pending, treat
	   this as an error. Don't wait for a join - fall out of this
	   loop and call DoQuit as if an error had occurred. This 
	   will cause any contexts currently waiting for a join to
	   get an invalidcontext error. */
        ctx->kernel->readyForJoin = false;
        ctx->kernel->joined = true; /* force it to quit */
	}
      else
        PSYield(yield_wait, (char *)NIL); Assert(ctx == currentPSContext);

      if (ctx->notified && (ctx->nReason == notify_terminate)) {
        ctx->kernel->readyForJoin = false;
        ctx->kernel->joined = true; /* force it to quit */
	} /* otherwise ignore notification */
      } while (!ctx->kernel->detached && !ctx->kernel->joined);
    }
  DoQuit(); /* never returns */
  }

private procedure UnloadContext ()
{
  if (currentPSContext != NIL) {
    UnloadData (currentPSContext->kernel->staticData);
    vmPrivate = NIL;	/* Safety: Ensure VM Unusable. */
    vmCurrent = NIL;	/* ditto */
    currentPSContext = NIL;
  }
}

private procedure AcquireLock(pl) PLock pl; {
  PSContext ctx = currentPSContext, c;
  if (ctx == pl->holder) InvalidContext();
  while (pl->holder != NIL) { /* the lock is not free */
    PSContext prev = NIL;
    if ((level > 0) && (pl->holder->space == ctx->space)) InvalidContext();
	/* add ctx to list waiting on the lock */
    ctx->kernel->lk = pl;
    ctx->kernel->nextPatron = NIL;
    
    for (c = pl->waiters; c != NIL; c = c->kernel->nextPatron) {
      prev = c;
      }
    if (prev == NIL) pl->waiters = ctx; else prev->kernel->nextPatron = ctx;

    DURING {
      PSYield(yield_wait, (char *)NIL);
    } HANDLER {
      Assert(ctx == currentPSContext);
      if (ctx->kernel->lk != NIL) BreakLooseLK(ctx);
      RERAISE;
    } END_HANDLER; 
    Assert(ctx == currentPSContext);
    if (ctx->kernel->lk != NIL) BreakLooseLK(ctx);
    }
  pl->holder = ctx;
  }

private procedure ReleaseLock(pl) PLock pl; {
  PSContext waiter, ctx = currentPSContext;

  if (pl->holder != ctx) {
    if (ctx->kernel->terminating)
      /* May have been inside "wait" and not owner of lock when terminated. */
      return;
    else
      InvalidContext();
  }

  pl->holder = NIL; /* mark pl free */
  waiter = pl->waiters; /* reschedule one waiter */
  if (waiter != NIL) {
    pl->waiters = waiter->kernel->nextPatron;
    waiter->kernel->lk = NIL;
    PSMakeRunnable(waiter);
    }
  }

private procedure NewLock (pob) PObject pob; {
  /* create and initialize a new lock object body */
  PLock lk;
  AllocGenericObject(objLock, sizeof(Lock), pob);
  lk = pob->val.lockval;
  lk->holder = NIL;
  lk->waiters = NIL;
  }

private procedure NewCondition (pob) PObject pob; {
  /* create and initialize a new condition object body */
  PCondition c;
  AllocGenericObject(objCond, sizeof(Condition), pob);
  c = pob->val.condval;
  c->waiters = NIL;
  }

private procedure MonitorExit() { /* called back from exec.c */
  Object ob;

  EPopP(&ob);
  if (!(ob.type == escObj && ob.length == objLock)) TypeCheck();
  ReleaseLock(ob.val.lockval);
  }
  
private integer GetNotifyAbortCode() { /* called from psExecute */
  PSContext ctx = currentPSContext;
  if (ctx->notified) {
    Assert(!ctx->kernel->terminating);
    ctx->notified = false;
    switch (ctx->nReason) {
      case notify_request_yield:
        return PS_YIELD;
      case notify_timeout:
        if (ctx->kernel->disableCount == 0) return PS_TIMEOUT;
	else {ctx->notified = true; return 0;}  /* leave it pending */
      case notify_terminate:
        return PS_TERMINATE;
      case notify_interrupt:
        if (ctx->kernel->disableCount == 0) return PS_INTERRUPT;
	else {ctx->notified = true; return 0;}  /* leave it pending */
      default:
        CantHappen ();
      }
    }
  return 0;
  }

private procedure YieldByRequest ()
{
  PSYield (yield_by_request, (char *)NIL);
}

private procedure YieldTimeLimit ()
{
  PSYield (yield_time_limit, (char *)NIL);
}

public PSContext CreateContext (space, in, out, startup)
  PSSpace space; Stm in, out; Object startup;
 /* Imported for postscript initialisation only */
 {
  PSContext ctx = NIL;

  UnloadContext (); /* save away the current kernel goodies, if any */

  DURING {
    ctx = (PSContext) NEW(1, sizeof(PSContextRec));
    ctx->kernel = (PSKernelContext) NEW(1, sizeof(PSKernelContextRec));
    ctx->space = space;	/* Assigned here in case DestroyPSContext called. */

    NewContextID(ctx);

    os_stdin = ctx->in = in;
    os_stdout = ctx->out = out;

    /* initialize a new set of kernel goodies */

    LoadVM (space->privateVM);

    ConditionalInvalidateRecycler (&startup);

    opStk = execStk = dictStk = NIL;	/* For error handling */
    CreateStacks ();	/* Stacks must be created before invoking CreateData */

    SetCETimeStamp ((Card32)ctx->kernel->id.id.index);
    ctx->kernel->staticData = CreateData ();
  } HANDLER {
    if (ctx != NIL) {
      if (opStk != NIL) { /* Supposed to be safe with or without statics!?! */
	currentPSContext = ctx;	/* Precondition for DestroyPSContext */
        DestroyPSContext ();
      } else {
	if (ctx->kernel != NIL) {
	  if (ctx->kernel->id.stamp != 0)
	    RecycleID (ctx->kernel->id);
	  FREE (ctx->kernel);
	}

        FREE (ctx);
      }
    }
    RERAISE;
  } END_HANDLER;

  ctx->kernel->startup = startup;

  ctx->kernel->nextInSpace = space->contexts;
  space->contexts = ctx;

  currentPSContext = ctx;

  if (startup.type != nullObj) {
    /* Do not create scheduler context during wholecloth initialization. */

    ctx->scheduler = CreateSchedulerContext(PSCoProc, ctx);

    if (ctx->scheduler == NIL) {
      DestroyPSContext ();	/* currentPSContext already valid here */
      LimitCheck ();
    }
  }

  /* leave the new one installed */
  return ctx;
  }


/* ********************************** */
/* The implementation of postscript.h */
/* ********************************** */

public PSContext currentPSContext;
/* The PSContext that the PostScript interpreter is or was most recently
   executing. This is changed only by calling SwitchPSContext.
   If this variable is zero, no PSContext has yet run or the most recent
   PSContext has been destroyed. */

public PSSpace CreatePSSpace() {
  PSSpace s = NIL;

  DURING {
    s = (PSSpace) NEW(1, sizeof(PSSpaceRec));
    NewSpaceID(s);
    s->privateVM = CreatePrivateVM(s->id);
  } HANDLER {
    if (s != NIL) {
      if (s->id.stamp != 0)
        RecycleID (s->id);

      Assert (s->privateVM == NIL);	/* Should have raised an exception */

      FREE (s);
    }

    return (NIL);
  } END_HANDLER;

  s->next = spaces; spaces = s;
  return s;
  }
  
public PSContext IDToPSContext (ID) ContextID ID; {
  /* This can be called asynchronously */
  PSContext ctx;
  if ((ID.id.index > nextID)
      || (ID.id.generation != ctxTab[ID.id.index].timestamp.id.generation))
    return NIL;
  ctx = ctxTab[ID.id.index].ctx;
  if (ctx == NIL || ctx->kernel->id.stamp != ID.stamp) return NIL; /* KLUDGE */
  return ctx;
  }

/* KLUDGE */
public PSSpace IDToPSSpace (S) SpaceID S; {
  PSSpace s;
  if ((S.id.index > nextID)
      || (S.id.generation != ctxTab[S.id.index].timestamp.id.generation))
    return NIL;
  s = (PSSpace) ctxTab[S.id.index].ctx;
  if (s == NIL || s->id.stamp != S.stamp) return NIL; /* KLUDGE */
  return s;
  }

public ContextID PSContextToID(context) PSContext context; {
  return context->kernel->id;
  }

public SpaceID PSSpaceToID(space) PSSpace space; {
  return space->id;
  }

public procedure CheckForPSNotify() {
  /* Check for abort value and ifso invoke interpreter to handle it. */

  /* if the current context is terminating it better not think
     it has been notified */

  if (currentPSContext->notified) {

    Assert(! currentPSContext->kernel->terminating);

    if (currentPSContext->kernel->joined
        || currentPSContext->kernel->readyForJoin) return;
  
    switch (currentPSContext->nReason) {
      case notify_request_yield:
        currentPSContext->notified = false;
        /* NOP */;
        break;
      case notify_interrupt:
      case notify_timeout:
        if (currentPSContext->kernel->disableCount > 0) return;
	/* fall through */
      case notify_terminate:
        {
        Object  Dummy;

	NotifyAbort();
        LNullObj (Dummy);
        Dummy.tag = Xobj;
        if (psExecute (Dummy)) RAISE ((int)GetAbort(), (char *)NIL);
        break;
	}
      default:
        CantHappen ();
      }
    }
  }

public PSContext CreatePSContext(space, in, out, device, startup)
  PSSpace space;
  Stm in, out;
  PDevice device;
  char *startup; {
  PSContext ctx, originalCtx = currentPSContext;

  /* Establish new private VM because MakeXStr allocates in VM! */

  UnloadContext ();
  LoadVM (space->privateVM);

  /* Now safe to execute MakeXStr */

  DURING
    ctx = CreateContext(space, in, out, MakeXStr((string)startup));
  HANDLER
    if (originalCtx != NIL)
      SwitchPSContext (originalCtx);

    return (NIL);
  END_HANDLER

  NewDevice (device);

#if VMINIT
  if (vmShared->wholeCloth || MAKEVM)
    Begin (rootShared->vm.Shared.internalDict);
#endif VMINIT

  Begin (rootShared->vm.Shared.sysDict);
  Begin (rootShared->vm.Shared.sharedDict);
  Begin (trickyUserDict);

  PSMakeRunnable(ctx);

  if (originalCtx != NIL)
    SwitchPSContext (originalCtx);

  return ctx;
  }

public PSContext ExclusivePSContext(space) PSSpace space; {
  return ((exclusivePSContext != NIL) ? exclusivePSContext : space->exclusive);
  }

private procedure SpaceExclusionSemaphore (count)
  integer count;
 /*
   The context exclusion mechanism is handled like a semaphore. This procedure
   is registered with the save/restore machinery.

   When a save is executed, the procedure is called with a +1 count.  When a
   restore is executed, the count is the negative of the number of save levels
   restored so that the ref. count is decremented for the corresponding number
   of saves.

   Similarly the garbage collector calls with +/- 1 to obtain exclusive access
   to the space.
  */
{
  Assert (currentPSContext != NIL);

  currentPSContext->space->semaphore += count;

  if (currentPSContext->space->semaphore == 0)
    currentPSContext->space->exclusive = NIL;
  else
    currentPSContext->space->exclusive = currentPSContext;
}

private procedure TotalExclusionSemaphore (count)
  integer count;
 /*
   This semaphore prevents all other contexts from running.  It is registered
   with the stroke machinery (and later with the Garbage Collector).
  */
{
  Assert (currentPSContext != NIL);

  exclusiveContextSemaphore += count;

  if (exclusiveContextSemaphore == 0)
    exclusivePSContext = NIL;
  else
    exclusivePSContext = currentPSContext;
}

public boolean SwitchPSContext(context)
  PSContext context; {
  PSContext exclusive = ExclusivePSContext(context->space);
  
  if ((exclusive != NIL) && (context != exclusive)) return (false);
  
  if (currentPSContext != context) {
    UnloadContext ();
    os_stdin = context->in;
    os_stdout = context->out;
    LoadData (context->kernel->staticData);    
    currentPSContext = context;
    Assert (currentPSContext->space->privateVM == vmPrivate);
    }
  return (true);
  }

public procedure DestroyPSContext()
  /* this is called only from the scheduler's coroutine */
  {
  PSContext context;
  PSSpace s, ns, prevs;
  PSContext c, prev;
  
  context = currentPSContext;

  Assert((context != NIL) && (context->kernel != NIL));

  s = context->space;

  DestroyData ();

  currentPSContext = NIL;	/* Must be valid prior to DestroyData call */

  RecycleID(context->kernel->id);
  
  /* remove it from its space.  (May not be there if CreateContext fails.)  */
  prev = NIL;

  for (c = s->contexts; c != NIL; c = c->kernel->nextInSpace) {
    if (c == context) {
      if (prev == NIL) s->contexts = c->kernel->nextInSpace;
      else prev->kernel->nextInSpace = c->kernel->nextInSpace;
      break;
      }
    else prev = c;
    }

  FREE (context->kernel);
  FREE (context);
  }

public procedure NotifyPSContext(context, reason)
  /* may be called asynchronously */
  PSContext context; NotifyReason reason;
  {
  if (context == NIL
      || context->kernel->terminating
      || context->nReason == notify_terminate)
    return;
  
  if (context->notified) {
    if (reason == notify_terminate || reason == notify_interrupt)
	  context->nReason = reason;
    }
  else {
    context->nReason = reason;
    context->notified = true;
    }

  if (context == currentPSContext) NotifyAbort();
  }

public procedure TerminatePSSpace(space) PSSpace space; {
  PSContext ctx;
  for (ctx = space->contexts; ctx != NIL; ctx = ctx->kernel->nextInSpace) {
    NotifyPSContext(ctx, notify_terminate);
    PSMakeRunnable(ctx);
    }
  }
  
public boolean DestroyPSSpace(space) PSSpace space; {
  PSSpace prevs, ns;

  if (space->contexts != NIL) return false;
  
  DestroyVM(space->privateVM);
  RecycleID(space->id);

  /* remove space from spaces */
  prevs = NIL;
  for (ns = spaces ; ns != NIL; ns = ns->next) {
    if (ns == space) {
      if (prevs == NIL) spaces = space->next;
      else prevs->next = space->next;
      break;
      }
    else prevs = ns;
    }

  FREE (space);
  return true;
  }
  
public procedure PSSetTimeLimit (limit)
  Card32 limit;
 /*
   Set execution time limit for current context.  (Units depend on system
   initialisation: Either operator units or actual future value of free-
   running clock.)
  */
{
  SetTimeLimit (limit);		/* Call to language package */
}

public procedure PSSetDevice (device, erase)
  PDevice device; boolean erase;
 /*
   Extablish new device for the current context.

   pre: currentPSContext != NIL
  */
{
  Assert (currentPSContext != NIL);
  NewDevice (device);
  if (erase)
    PSErasePage();
}

/* ***************************************************** */
/* Procedures implementing the context operators (in alphabetic order) */

public procedure PSCondition () {
  Object ob;
  NewCondition(&ob);
  PushP(&ob);
  }

public procedure PSCurrentContext () {
  PushInteger((integer)currentPSContext->kernel->id.stamp);
  }

public procedure PSDetach () {
  ContextID id;
  PSContext ctx;
  id.stamp = PopInteger();
  ctx = IDToPSContext(id);
  if ((ctx == NIL) || (ctx->kernel->detached) || (ctx->kernel->joined))
    InvalidContext();
  ctx->kernel->detached = true;

  /* let ctx and any prospective joiners run */
  if (ctx->kernel->readyForJoin) PSMakeRunnable(ctx);
  if (ctx->kernel->joinerWaiting) InnerNotify(&ctx->space->joinable);
  }

public procedure PSFork () {
  PSContext ctx = NIL, old = currentPSContext;
  Stm in, out;
  PStack old_opStk = opStk, old_dictStk = dictStk;
  PGState old_gs = gs;
  boolean b;
  integer objFormat = GetObjFormat();
  integer old_opStkSize = CountToMark (old_opStk);
  Object tobj;
  
  DebugAssert (currentPSContext != NIL);

  if (level != 0) InvalidContext();
  /* PAC - fix bug 218: mark fork bombs */
  if (old_opStkSize < 1) Underflow(opStk);
  TopP(&tobj);
  if (tobj.type != arrayObj && tobj.type != pkdaryObj) TypeCheck();
  if ((tobj.access & xAccess) == 0) InvlAccess();
  if (!PSNewContextStms(&in, &out)) LimitCheck();
  
  DURING {
    ctx = CreateContext(old->space, in, out, forkStartup);
      /* This unloads the old context and leaves the new context loaded */
  
    /* Copy the contents of the dict and graphic state stacks, and
       objects on the opstack up to the mark. */

    CopyStack (old_opStk, opStk, (cardinal)old_opStkSize);
    CopyStack (
      old_dictStk, dictStk, CountStack (old_dictStk, (cardinal)MAXCard16));
    GStackCopy (old_gs);
    SetObjFormat(objFormat);
  } HANDLER {
    if (ctx != NIL) {
      /*
        Cause new context to go away immediately.  Cannot destroy it directly
        because scheduler already has a record of it (CreateContext terminated
	properly).
       */
      NotifyPSContext (ctx, notify_terminate);
      PSMakeRunnable (ctx);
    }

    b = SwitchPSContext (old);
    Assert (b);
    RERAISE;	/* Assert safe because Exception.{code,message} local */
  } END_HANDLER;

  b = SwitchPSContext(old); /* restore the old context */
  Assert(b);
  
  PSClrToMrk();

  PushInteger((integer)ctx->kernel->id.stamp);
  PSMakeRunnable(ctx);
  }

public procedure PSJoin () {
  ContextID id;
  
  id.stamp = PopInteger();

  while (true) {
    PSContext ctx = IDToPSContext(id);
    if ((ctx == NIL) || (ctx == currentPSContext)
        || (ctx->kernel->detached) || (ctx->kernel->joined)
        || (ctx->space != currentPSContext->space)
        || (level != 0)
       ) InvalidContext();
    else {
      if (ctx->kernel->readyForJoin) { /* do the actual join */
        ctx->kernel->readyForJoin = false;
        ctx->kernel->joined = true;
        /* push a mark, pull off results and push them */
	PushP (&iLMarkObj);
	CopyStack (ctx->kernel->opStk, opStk, ctx->kernel->opStkLn);

	/* let ctx and others waiting to join with it run */
        PSMakeRunnable(ctx);
	if (ctx->kernel->joinerWaiting) InnerNotify(&ctx->space->joinable);
        return;
        }
      else { /* ctx not yet ready for join */
        WaitOnJoinable(ctx);
        }
      }
    }
  }

public procedure PSLock () {
  Object ob;
  NewLock(&ob);
  PushP(&ob);
  }

private procedure MonExitProc() {
  Object ob;

  EPopP(&ob);
  if (!(ob.type == escObj && ob.length == objLock)) TypeCheck();
  
  ReleaseLock(ob.val.lockval);
  }

public procedure PSMonitor () {
  Object ob, proc;
  PCondition c;
  PLock pl;
  PSContext ctx = currentPSContext;
  
  PopP(&proc);
  if (proc.type != arrayObj && proc.type != pkdaryObj) TypeCheck();
  if ((proc.access & xAccess) == 0) InvlAccess();

  PopP(&ob);
  if (!(ob.type == escObj && ob.length == objLock)) TypeCheck();

  AcquireLock(ob.val.lockval);

  EPushP(&ob);
  EPushP(&monitorcmd);
  EPushP(&proc);
  }

public procedure PSAcquireLock(pobj) PObject pobj;
{
  if (!(pobj->type == escObj && pobj->length == objLock)) TypeCheck();
  AcquireLock(pobj->val.lockval);
}

public procedure PSReleaseLock(pobj) PObject pobj;
{
  if (!(pobj->type == escObj && pobj->length == objLock)) TypeCheck();
  ReleaseLock(pobj->val.lockval);
}

public procedure PSNotify () {
  Object ob;
  
  PopP(&ob);
  if (!(ob.type == escObj && ob.length == objCond)) TypeCheck();

  InnerNotify(ob.val.condval);
  }

public procedure PSQuit()
{
  NotifyPSContext(currentPSContext, notify_terminate);
}

public procedure PSWait () {
  Object ob;
  PCondition c;
  PLock pl;
  PSContext ctx, waiters;
  boolean cInSharedVM, plInSharedVM;
  
  PopP(&ob);
  if (!(ob.type == escObj && ob.length == objCond)) TypeCheck();
  c = ob.val.condval;
  cInSharedVM = ob.shared;

  PopP(&ob);
  if (!(ob.type == escObj && ob.length == objLock)) TypeCheck();

  pl = ob.val.lockval;
  plInSharedVM = ob.shared;
  
  ctx = currentPSContext;
  if ((level > 0) && !(plInSharedVM && cInSharedVM)) {
    InvalidContext ();
    }

  ReleaseLock(pl);

  /* wait on c */
  ctx->kernel->waitee = c;
  ctx->kernel->nextWaiter = c->waiters;
  c->waiters = ctx;
  DURING {
    PSYield(yield_wait, (char *)NIL);
  } HANDLER {
    Assert(ctx == currentPSContext);
    if (ctx->kernel->waitee != NIL) BreakLooseCV(ctx);
    RERAISE;
  } END_HANDLER;
  Assert(ctx == currentPSContext);
  if (ctx->kernel->waitee != NIL) BreakLooseCV(ctx);
  
  AcquireLock(pl);
  }

public procedure YieldOp () {
  PSYield(yield_by_request, (char *)NIL);
  }

/*
 Internaldict operators:

   disableinterrupt	defers notification of interrupts and timeouts
   enableinterrupt	reenables and performs deferred interrupt/timeout
   clearinterrupt	reenables and discards deferred interrupt/timeout

 Note: these operators do not affect notification of conditions other
 than interrupt and timeout.
 */

public procedure DisableCC()
{
  currentPSContext->kernel->disableCount++;
}

public procedure EnableCC()
{
  if (currentPSContext->kernel->disableCount > 0 &&
      --currentPSContext->kernel->disableCount == 0 &&
      currentPSContext->notified)
    NotifyAbort();
}

public procedure PSClrInt()
{
  currentPSContext->kernel->disableCount = 0;
  if (currentPSContext->notified)
    {
    currentPSContext->notified = false;
    switch (currentPSContext->nReason)
      {
      case notify_interrupt:
      case notify_timeout:
        break; /* discard pending interrupt or timeout */
      default:
        currentPSContext->notified = true;  /* leave others pending */
	break;
      }
    }
}


/*
  Garbage Collector Procedures (except semaphores)
 */

private GenericID GetCurrentContext ()
{
  return (currentPSContext->kernel->id);
}

private PCard8 SetCurrentContext (id)
  GenericID id;
{
  PSContext ctx = IDToPSContext (id);
  Assert (ctx != NIL);
  return (ctx->kernel->staticData);	/* This needs to be rethought */
}

private GenericID GetNextContext (privateVM, id)
  PVM privateVM;
  GenericID id;
 /*
   Search spaces for the VM and then for the id.
  */
{
  PSSpace S;
  PSContext C;
  GenericID result;

  Assert (spaces != NIL);

  S = spaces;

  while (S != NIL) {
    if (S->privateVM == privateVM)
      break;
    else
      S = S->next;
  }

  Assert (S != NIL);

  C = S->contexts;

  if (id.stamp != 0) {
    while (C != NIL) {
      boolean done = (C->kernel->id.stamp == id.stamp);
      C = C->kernel->nextInSpace;
      if (done)
	break;
    }
  }

 if (C == NIL)
   result.stamp = 0;
  else
   result = C->kernel->id;

  return (result);
}

private PVM GetNextSpace (privateVM)
  PVM privateVM;
{
  PSSpace S;

  Assert (spaces != NIL);

  if (privateVM == NIL)
    return (spaces->privateVM);
  else {
    S = spaces;

    while (S != NIL) {
      if (S->privateVM == privateVM)
	break;
      else
        S = S->next;
    }

    if ((S == NIL) || (S->next == NIL))
      return (NIL);
    else
      return (S->next->privateVM);
  }
}

/* ***************************************************** */
/* Initialization */

public procedure ContextOpsInit (reason)
  InitReason reason;
{
  boolean oldShared;

  switch (reason) {
   case init:
#if (OS == os_mpw)
     globals = (Globals)os_sureCalloc(sizeof(GlobalsRec),1);
#endif
     nextID = 1; nextFreeID = 0; gTimestamp = 1;
     ctxTabSize = ps_getsize (SIZE_ID_SPACE, initCtxTabSize);
     Assert (ctxTabSize <= (MAXGenericIDIndex + 1));
     ctxTab = (CtxItem *)os_sureCalloc((long int)sizeof(CtxItem), ctxTabSize);
     currentPSContext = NIL;
     RgstContextProcs (MonitorExit, YieldByRequest, YieldTimeLimit, GetNotifyAbortCode);
     RgstFontsSemaphoreProc (SpaceExclusionSemaphore);
     RgstSaveSemaphoreProc (SpaceExclusionSemaphore);
     RgstStrokeSemaphoreProc (TotalExclusionSemaphore);
     RgstGCContextProcs(SpaceExclusionSemaphore, TotalExclusionSemaphore,
			GetCurrentContext, SetCurrentContext,
			GetNextContext, GetNextSpace);
     break;
   case romreg:
    oldShared = CurrentShared();
    SetShared (true);
    forkStartup = MakeXStr((string)"stopped {/handleerror load stopped pop systemdict /quit get exec} if");
    ConditionalInvalidateRecycler (&forkStartup);
    SetShared (oldShared);
    RgstMark("@monitormark", MonExitProc, (integer)(mrkMonitor), &monitorcmd);
    break;
  }
}

