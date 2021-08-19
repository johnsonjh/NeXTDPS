/*
				  gstates.c

	  Copyright (c) 1987, '88, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE:  All information contained  herein  is the property  of Adobe Systems
Incorporated.  Many  of the  intellectual and technical    concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe  licensees for their  internal use.  Any reproduction
or dissemination of this software is  strictly forbidden unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: 
Edit History:
Scott Byer: Wed May 17 14:58:54 1989
Joe Pasqua: Tue Feb 21 15:08:23 1989
Ivor Durham: Sat May  6 14:54:57 1989
Ed Taft: Thu Jul 28 13:20:15 1988
Jim Sandman: Thu Apr 13 14:18:14 1989
Bill Paxton: Mon Sep 19 08:39:26 1988
Leo Hourvitz: Wed Jun 28 15:21:00 1989 call gStateExtProc in InitGraphics
Ted Cohn: Wed Dec 13 11:19:28 PST 1989 init pool for NeXT extensions;
	moved NextGStatesProc() here from nextmain.c
Jack Newlin 1Feb90 reorder setalpha in InitGraphics, Pasqua's fix to GSave
	   12Apr90 removed colortowhite
Ted Cohn: 4Apr90 fixed g->color to be g->color.color in new adobe release.
Ted Cohn: 5May90 initialize realalpha to 1.0 now in gs extension.
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include BINTREE
#include WINDOWDEVICE
#include ERROR
#include EXCEPT
#include FP
#include GC
#include DEVICE
#include GRAPHICS
#include LANGUAGE
#include PSLIB
#include VM
#include ORPHANS

#include "gstack.h"
#include "graphdata.h"
#include "graphicspriv.h"
#include "gray.h"
#include "graphicsnames.h"
#include "path.h"

extern BBoxCompareResult BBCompare( /* BBox figbb, clipbb; */ );

#define gstack          (graphicsStatics->gStates._gstack)
#define	savelevel	(graphicsStatics->gStates._savelevel)
#define	gsPrivate	(graphicsStatics->gStates._gs)

public PGState gs;

#define GSTATE_MAX (32)	/* Maximum gstates per stack */
#define GSTATE_GMAX 2000 /* Maximum global gstates */

/* private global variables. make change in both arms if changing variables */

#if (OS != os_mpw)

private Pool neStorage;     /* Pool for NeXT gstate extensions */
private Pool gstackStorage; /* Storage pool for gstacks */
private Pool gsStorage;     /* Pool for GStates; shared by all contexts.*/ 
private PVoidProc gStateExtProc;

#else (OS != os_mpw)

typedef struct {
    Pool g_gstackStorage;
    Pool g_gsStorage;
    PVoidProc g_gStateExtProc;
} GlobalsRec, *Globals;
  
private Globals globals;

#define gstackStorage globals->g_gstackStorage
#define gsStorage globals->g_gsStorage
#define gStateExtProc globals->g_gStateExtProc

#endif (OS != os_mpw)


public procedure SetGStateExtProc(PVoidProc proc)
{
    gStateExtProc = proc;
}

public procedure NewDevice(d)  PDevice d;
{
  PDevice old = gs->device;
  if (d != old) {
    PGState g;
    if (old != (PDevice)NIL) {
      if ((g = gs->previous) != NULL ) {
	if (g->previous == NULL && g->device == psNulDev) {
	  PGState oldGS = gs;
	  DURING
	    gs = g;
	    NewDevice(d);
	  HANDLER 
	    gs = oldGS;
	    RERAISE;
	  END_HANDLER;
	  gs = oldGS;
	  }
	}
      (*old->procs->Sleep)(old);
      if (--old->ref == 0) (*old->procs->GoAway)(old);
      }
    d->ref++;
    (*d->procs->Wakeup)(d);
    }
  gs->device = d;
  SetScal();
  SetDevColor(gs->color);
  InitMtx();
  InitClip();
}
  
public procedure SetMaskDevice(device)
    PDevice device;
{
  register PGState g = gs;
  real gray = fpZero;
  LAryObj(g->dashArray,0,NIL);
  if (g->tfrFcn) { RemTfrRef(g->tfrFcn); g->tfrFcn = NULL; }
  SetGray(&gray);
  g->noColorAllowed = true;
  NewDevice(device);
}

public PGState PopGState(obp,accessRequired)
    PObject obp;
    int accessRequired;
{
  Object ob;
  
  if (obp == NULL) obp = &ob;
  PopP(obp);
  if (obp->type == nullObj) return(gs); /* shorthand */
  if (obp->type != escObj || obp->length != objGState) TypeCheck();
  if ((obp->access & accessRequired) == 0) InvlAccess();
  return(obp->val.gstateval);
}

private procedure GSAddRefs(newGS)
    register PGState newGS;
{
  /* AddPathRef(&newGS->path); AddPathRef(&newGS->clip); */
  { register ReducedPath *rp;
    rp = newGS->path.rp;
    if (rp != NULL) rp->refcnt++;
    rp = newGS->clip.rp;
    if (rp != NULL) rp->refcnt++;
    }
  { register ListPath *lp;
    /* ListPath QuadPath & IntersectPath all start with refcnt */
    /* so do not bother to check which type it is */
    lp = newGS->path.ptr.lp;
    if (lp != NULL) lp->refcnt++;
    lp = newGS->clip.ptr.lp;
    if (lp != NULL) lp->refcnt++;
    }
  /* AddDevRef(newGS->device); */
  { register PDevice d = newGS->device;
    if (d) d->ref++;
    }
  /* AddScrRef(newGS->screen); */
  { register Screen screen = newGS->screen;
    if (screen != NULL) screen->ref++;
    }
  { register TfrFcn t = newGS->tfrFcn;
    if (t) t->refcnt++;
    }
  { register GamutTfr gt = newGS->gt;
    if (gt) gt->ref++;
    }
  { register Rendering r = newGS->rendering;
    if (r) r->ref++;
    }
  { register Color c = newGS->color;
    if (c) c->ref++;
    }
  if (gStateExtProc != NULL) (*gStateExtProc)(newGS, 1L);
}

public procedure GSave()
{
  register PGState newGS;
  if (gstack->gsCount++ >= GSTATE_MAX)
    {gstack->gsCount--; LimitCheck();}
  newGS = (PGState)os_newelement(gsStorage);
  *newGS = *gs;
  newGS->previous = gs;
  newGS->saveState = false;
  GSAddRefs(newGS);
  gstack->gss = gs = newGS;
}

private procedure GSRemRefs(oldGS)
    register PGState oldGS; 
{
  RemPathRef(&oldGS->path);
  RemPathRef(&oldGS->clip);
  /* RemScrRef(oldGS->screen); */
  { register Screen screen = oldGS->screen;
    if (screen != NULL) {
        if ((--screen->ref) == 0) {
          screen->ref++; RemScrRef(screen); oldGS->screen = NULL; }
      }
    }
  /* RemDevRef(oldGS->device); */
  { register PDevice d = oldGS->device;
    if (d != NULL) {
      if (--d->ref == 0) {
        (*d->procs->GoAway)(d); oldGS->device = NULL;}
      }
    }
  /* RemTfrRef(oldGS->tfrFcn); */
  { register TfrFcn t = oldGS->tfrFcn;
    if (t != NULL) {
      t->active = false;
      if ((--t->refcnt) == 0) {
        t->refcnt++; RemTfrRef(t); oldGS->tfrFcn = NULL; }
      }
    }
  { register GamutTfr gt = oldGS->gt;
    if (gt != NULL) {
      if ((--gt->ref) == 0) {
        gt->ref++; RemGTRef(gt, true); oldGS->gt = NULL; }
      }
    }
  { register Rendering r = oldGS->rendering;
    if (r != NULL) {
      if ((--r->ref) == 0) {
        r->ref++; RemRndrRef(r, true); oldGS->rendering = NULL; }
      }
    }
  { register Color c = oldGS->color;
    if (c != NULL) {
      if ((--c->ref) == 0) {
        c->ref++; RemColorRef(c, true); oldGS->color = NULL; }
      }
    }
  if (gStateExtProc != NULL) (*gStateExtProc)(oldGS, -1L);
}

private procedure GStackRestore(force)
    boolean force;
{
  /* when a context goes away, call with force true to discard last gs */
  /* for normal grestore, call with force false */
  boolean wasSave;
  register PGState g;
  PDevice oldDevice = gs->device;
  g = gs;
  if (!force && g->previous == NULL) return;
  wasSave = g->saveState && !force;
  if (wasSave && savelevel == 0) return;
  if (!g->previous || (oldDevice != g->previous->device))
     (*oldDevice->procs->Sleep)(oldDevice);
  GSRemRefs(g);
  gstack->gss = gs = g->previous;
  os_freeelement(gsStorage, (char *)g);
  g = gs;
  gstack->gsCount--;
  if (g == NULL) { Assert(force); return; }
  if (wasSave) { GSave(); g = gs; g->saveState = true; }
#if DPSXA
  {
	DevLBounds bounds;
	extern SetXABounds();
	(*gs->device->procs->DefaultBounds)(gs->device, &bounds);
	SetXABounds(&bounds);
  }
#endif /* DPSXA */
  if (oldDevice != g->device)
    (*g->device->procs->Wakeup)(g->device);
}

public procedure GRstr()
{
    GStackRestore(false);
}

public procedure GRstrAll()
{
  while (gs->saveState == false)
    GStackRestore(false);
  GStackRestore(false);
}

private procedure GSaveProc(lev)
    Level lev;
{
  if ((short)++savelevel != (short)lev) CantHappen();
  GSave();
  gs->saveState = true;
}

public procedure GStackClear()
{ /* called when context is going away */
  while (gs)
    GStackRestore(true);
  os_freeelement(gstackStorage, (char *)gstack);
  gstack = NULL;
}

public procedure InitGraphics()
{
  PNextGSExt ep;
  real gray;
  
  InitMtx();
  NewPath();
  InitClip();
  gs->lineWidth = fpOne;
  gs->lineCap = buttCap;
  gs->lineJoin = miterJoin;
  gs->miterlimit = fp10;
  LAryObj(gs->dashArray,0,NIL);
  gs->dashOffset = fpZero;
  gray = fpZero;
  /* Extension MUST be allocated */
  ep = *((PNextGSExt *)PSGetGStateExt(NULL));
  ep->alpha = OPAQUE; /* Set alpha opaque before SetGray */
  ep->realalpha = 1.0;
  ep->instancing = 0;
  SetGray(&gray);
  /* do not change gs->strokeAdjust */
} /* InitGraphics */
  
private procedure InitGS() {
  register PGState g = gs;
  DictObj fdict;
  if (graphicsNames != NIL && ! vmShared->wholeCloth) {
    DictGetP(rootShared->vm.Shared.internalDict,
	 graphicsNames[nm_InvalidFont], &fdict);
    SetFont(fdict);
  }
  InitPath(&g->path);
  InitPath(&g->clip);
  g->flatEps = fpOne;
  g->screenphase.x = g->screenphase.y = 0;
  g->device = NIL;
  g->screen = NIL;
  g->isCharPath = false;
  g->noColorAllowed = false;
  g->strokeAdjust = true;
  g->circleAdjust = true;
  g->tfrFcn = NIL;
  g->device = psNulDev;
  InitGraphics();
} /* InitGS */

public PGStack CreateGStack()
{
  register PGStack gstk;
  gstk = (PGStack)os_newelement(gstackStorage);
  gstk->gss = gs = (PGState)os_newelement(gsStorage);
  os_bzero((char *)gs, (long int)sizeof(GState));
  gstk->gsCount = 1;
  /* Create gs->extension and initialize it */
  if (gStateExtProc != NULL) (*gStateExtProc)(gs, 0);
  InitGS();
  return gstk;
}

public procedure GStackCopy (original)
    PGState original;
{
  PGState gState, prevGState;

  while (gs)
    GStackRestore(true);

  while (original != NIL) {
    gState = (PGState)os_newelement(gsStorage);
    *gState = *original;
    if (gs == NIL)
      gs = gState;
    else
      prevGState->previous = gState;

    prevGState = gState;
    gstack->gsCount++;
    GSAddRefs(gState);
    original = original->previous;
  }
  gstack->gss = gs;
}

private procedure GRestoreProc(lev)
    Level lev;
{
  while(savelevel>lev) {
    GRstrAll();
    gs->saveState = false;
    GStackRestore(false);
    savelevel--;
  }
}

private procedure GSFinalize(obj, reason)
  GStateObj obj;
  FinalizeReason reason;
{
  PGState g = obj.val.gstateval;

  switch (reason)
    {
    case fr_restore:
    case fr_privateReclaim:
    case fr_sharedReclaim:
    case fr_destroyVM:
    case fr_overwrite:
      GSRemRefs(g);
      break;

    case fr_copy:
      GSAddRefs(g);
      break;

    default:
      CantHappen();
    }
}

private boolean GSHasPrivateStuff(g)
    register PGState g;
{
  TfrFcn t;
  if (!g->encAry.shared
   || !g->fontDict.shared
   || !g->dashArray.shared) return true;
  if (g->tfrFcn == NULL) return false;
  t = g->tfrFcn;
  return (!t->tfrGryFunc.shared
   || !t->tfrRedFunc.shared
   || !t->tfrGrnFunc.shared
   || !t->tfrBluFunc.shared
   || !t->tfrUCRFunc.shared
   || !t->tfrBGFunc.shared
   );
}

public procedure PSGState()
{
  /* create a new gstate object body and initialize from current gs */
  Object ob;
  PGState g;
  GenericBody savedBody;
  if (gs->noColorAllowed) Undefined(); /* not allowed in mask device */
  if (CurrentShared () && GSHasPrivateStuff(gs))
    FInvlAccess ();
  AllocGenericObject(objGState, sizeof(GState), &ob);
  g = ob.val.gstateval;
  savedBody = g->header;	/* Preserve header across next assignment */
  *g = *gs;
  g->header = savedBody;
  GSAddRefs(g);
  PushP(&ob);
}

public procedure PSCurrentGState()
{
  /* fill in gstate object body with current gstate */
  Object ob;
  register PGState g = gs;
  if (PopGState(&ob,wAccess) == g)
    TypeCheck ();
  if (g->noColorAllowed) Undefined(); /* not allowed in mask device */
  if (ob.shared && GSHasPrivateStuff(g))
    FInvlAccess();
  VMPutGeneric(ob, ((PCard8)g + sizeof(GenericBody)));
    /* this will call back to GStateHandler to decrement refcnts */
  GSAddRefs(g);
  PushP(&ob);
}

public procedure PSSetGState()
{
  /* replace current gstate with copy of gstate object body */
  PGState prev;
  register PGState g = gs;
  PDevice oldDevice = g->device;
  boolean wasSave;
  if (g->noColorAllowed) Undefined(); /* not allowed in mask device */
  wasSave = g->saveState;
  prev = g->previous;
  g = PopGState((PObject)NULL,rAccess);
  if (g == gs)
    TypeCheck ();
  if (g->device != oldDevice) /* put old device to sleep */
    (*oldDevice->procs->Sleep)(oldDevice);
  GSRemRefs(gs);
  *gs = *g; g = gs;
  g->saveState = wasSave;
  g->previous = prev;
  GSAddRefs(g);
  if (g->device != oldDevice) /* wake up the new device */
    (*g->device->procs->Wakeup)(g->device);
}

public procedure GStateDataHandler (code)
    StaticEvent code;
{
  switch (code) {
   case staticsCreate:
    savelevel = -1;
    gstack = CreateGStack ();
    GSaveProc (0);
    gsPrivate = gs;
    {
      /* Piggy back ucache.c handler on gstates.c handler for this case */
      extern procedure UCacheDataHandler ();
      UCacheDataHandler (code);
    }
    break;
   case staticsDestroy:
    if (gsPrivate != NIL) GStackClear ();
    break;
   case staticsLoad:
    gs = gsPrivate;
    if (gs != NIL && gs->device != NIL)
      (*gs->device->procs->Wakeup)(gs->device);
    break;
   case staticsUnload:
    gsPrivate = gs;
    if (gs != NIL) {
      if (gs->tfrFcn != NIL)
        gs->tfrFcn->active = false;
      if (gs->device != NIL)
	(*gs->device->procs->Sleep)(gs->device);
    }
    break;
  }
}

/*	----> Garbage Collector Support <----	*/
/* The following routines are supplied for use	*/
/* by the garbage collector. They enumerate &	*/
/* push items in a GState onto the GC stack.	*/

procedure TracePath(path, info)
  register PPath path;
  GC_Info info;
{
  while (true) {
    if ((PathType)path->type == strokePth)
      {
      StrkPath *sp = path->ptr.sp;
      GC_Push(info, &(sp->dashArray));
      path = &sp->path;
      }
    else if ((PathType)path->type == intersectPth)
      {
      IntersectPath *ip = path->ptr.ip;
      TracePath(&ip->path, info);
      path = &ip->clip;
      }
    else break;
    }
}

private procedure PushGStateItems(gState, info)
    GState *gState;
    GC_Info info;
{
  GC_Push(info, &(gState->encAry));
  GC_Push(info, &(gState->pfontDict));       /* replace fontDict with pfontDict */
  GC_Push(info, &(gState->dashArray));
  if (gState->tfrFcn != NIL)
    {
    GC_Push(info, &(gState->tfrFcn->tfrGryFunc));
    GC_Push(info, &(gState->tfrFcn->tfrRedFunc));
    GC_Push(info, &(gState->tfrFcn->tfrGrnFunc));
    GC_Push(info, &(gState->tfrFcn->tfrBluFunc));
    GC_Push(info, &(gState->tfrFcn->tfrUCRFunc));
    GC_Push(info, &(gState->tfrFcn->tfrBGFunc));
    }
  if (gState->screen != NIL)
    {
    Screen s = gState->screen;
    GC_Push(info, &(s->dict));
    if (s->dict.type == nullObj) {
      switch (s->type) {
        case 2:
          GC_Push(info, &(s->fcns[3].ob));
          GC_Push(info, &(s->fcns[2].ob));
          GC_Push(info, &(s->fcns[1].ob));
        case 1:
          GC_Push(info, &(s->fcns[0].ob));
          break;
        }
      }
    }
  TracePath(&gState->path, info);
  TracePath(&gState->clip, info);
}

private procedure GStateEnumerator(obj, info)
    PObject obj;
    GC_Info info;
{
  PushGStateItems(obj->val.gstateval, info);
}

private procedure PushGrfxRoots(clientData, info)
/* This proc is responsible for pushing all	*/
/* roots that are held by the graphics package.	*/
    RefAny clientData;
    GC_Info info;
{
  register GState *gState;

  for (gState = gstack->gss; gState != NIL; gState = gState->previous)
    PushGStateItems(gState, info);
}

/*	----- End GC Support Routines -----	*/

public procedure IniGStates(reason)  InitReason reason;
{
switch (reason)
  {
  case init:
#if (OS == os_mpw)
    globals = (Globals)os_sureCalloc(sizeof(GlobalsRec),1);
#endif (OS == os_mpw)
    gstackStorage = os_newpool (sizeof(GStack),5,0);
    gsStorage = os_newpool(sizeof(GState),10,GSTATE_GMAX);
    neStorage = os_newpool(sizeof(NextGSExt),10,-1);
    GC_RgstGetRootsProc(PushGrfxRoots, (RefAny)NIL);
    GC_RgstGStateEnumerator(GStateEnumerator);
    break;
  case romreg:
    RgstSaveProc(GSaveProc);
    RgstRstrProc(GRestoreProc);
    VMRgstFinalize(objGState, GSFinalize, frset_reclaim | frset_overwriteCopy);
    break;
  }
}

/*****************************************************************************
    NeXT uses the gtate extension field to store NeXT related items per
    gstate.  NextGStatesProc is the gStateExtProc called to maintain that
    field.  This procedure is meant to be used  to maintain a reference
    count, the action taken depends on the verb.

     	 0: Allocate new extension and initialize it.
	 1: increment reference count by 1.
	-1: decrement reference count by 1

    Rather than maintaining a reference count, this implementation actually
    allocates and deallocates the extention structure, maintaining a 1:1
    corespondence between gstates and extension structs.
	
    In the NeXT extension we store the user's input alpha, instancing state,
    window device coordinate state and patterning state.

******************************************************************************/

public void NextGStatesProc(PGState g, int verb)
{
    PNextGSExt ep;

    switch(verb) {
    case 0:
	DebugAssert(g->extension == NULL);
	(PNextGSExt)g->extension = ep = (PNextGSExt)os_newelement(neStorage);
	ep->alpha = OPAQUE;
	ep->realalpha = 1.0;
	ep->instancing = 0;
	ep->realscale = 0;
	ep->patternpending = 0;
	ep->graypatstate = 0;
	break;
    case 1:
	ep = (PNextGSExt)os_newelement(neStorage);
	*ep = *((PNextGSExt)g->extension);
	g->extension = (char *)ep;
	break;
    case -1:
	os_freeelement(neStorage, g->extension);
	g->extension = NULL;
	break;
    default:
        CantHappen();
	break;
    }
}









