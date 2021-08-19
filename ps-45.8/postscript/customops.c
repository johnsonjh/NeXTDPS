/*
  customops.c

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

Edit History:
Scott Byer: Fri May 19 10:39:08 1989
Perry Caro: Wed Nov  9 17:22:01 1988
Ivor Durham: Tue May  9 00:02:46 1989
Jim Sandman: Tue Nov  7 15:39:39 1989
Joe Pasqua: Tue Feb  7 14:25:13 1989
Paul Rovner: Tue Aug 22 10:04:34 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include CUSTOMOPS
#include DEVICE
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include GC
#include GRAPHICS
#include PSLIB
#include PUBLICTYPES
#include LANGUAGE
#include ORPHANS
#include STREAM
#include VM

/* support for objs */

typedef struct { /* element of the map */
  Object obj;
  PVM homeVM;
  integer ref;
  } ManagedItem;

#define ITEMSPERCHUNK 10

typedef struct _t_ItemChunk {
  struct _t_ItemChunk *next;
  ManagedItem items[ITEMSPERCHUNK];
  } ItemChunk;

extern  DictObj trickyStatusDict;

#if (OS != os_mpw)
/*-- BEGIN GLOBALS --*/

private Pool chunkPool;
private ItemChunk *chunks;
private ManagedItem *miFree;

/*-- END GLOBALS --*/
#else (OS != os_mpw)

typedef struct {
  Pool g_chunkPool;
  ItemChunk *g_chunks;
  ManagedItem *g_miFree;
  } GlobalsRec, *Globals;
  
#define miFree globals->g_miFree
#define chunkPool globals->g_chunkPool
#define chunks globals->g_chunks

private Globals globals;

#endif (OS != os_mpw)
public void PSRegister(opName, proc) char *opName; void (*proc)(); {
  RgstExplicit(opName, proc);
  }

public void PSRegisterStatusDict(opName, proc) char *opName; void (*proc)(); {
  Begin(trickyStatusDict);
  RgstExplicit(opName, proc);
  End();
  }

public void PSRgstOps(p) PRgOpEntry p; {
  while (p->name != NIL) {
    RgstExplicit(p->name, p->proc);
    p++;
    }
  }

private PSOperandType TypeOfObj(pobj) PObject pobj; {
  integer t;
  PSOperandType ans;
  t = pobj->type;
  if (t == escObj) t = pobj->length;
  switch (t) {
    case nullObj: ans = dpsNullObj; break;
    case intObj: ans = dpsIntObj; break;
    case realObj: ans = dpsRealObj; break;
    case nameObj: ans = dpsNameObj; break;
    case boolObj: ans = dpsBoolObj; break;
    case strObj: ans = dpsStrObj; break;
    case stmObj: ans = dpsStmObj; break;
    case cmdObj: ans = dpsCmdObj; break;
    case dictObj: ans = dpsDictObj; break;
    case arrayObj: ans = dpsArrayObj; break;
    case pkdaryObj: ans = dpsPkdaryObj; break;
    case objGState: ans = dpsGStateObj; break;
    case objCond: ans = dpsCondObj; break;
    case objLock: ans = dpsLockObj; break;
    default: ans = dpsOtherObj; break;
    }
  return ans;
  }

public PSOperandType PSGetOperandType() {
  if (opStk->head <= opStk->base) Underflow(opStk);
  return TypeOfObj(opStk->head-1);
  }
  
public long int PSStringLength() {
  if (opStk->head <= opStk->base) Underflow(opStk);
  if ((opStk->head-1)->type != strObj) TypeCheck();
  return (opStk->head-1)->length;
  }

public void PSPopString(sP, nChars) char *sP; long int nChars; {
  StrObj str;
  if (nChars < (opStk->head-1)->length+1) RangeCheck();
  PopPRString(&str);
  VMGetText(str, (string)sP);
  }

public real PSPopReal() {real r; PSPopPReal(&r); return r;}

public procedure PSPushReal(r) real r; {PSPushPReal(&r);}

public boolean PSSharedObject(pobj) PObject pobj; {return pobj->shared;}

public procedure PSInvalidID() {RAISE(ecInvalidID, (char *)NIL);}

public void PSPushString(sP) char *sP; {
  StrObj str;
  str = MakeXStr((string)sP);
  PushP(&str);
  }

public Stm PSPopStream() {
  StmObj s;
  PopPStream(&s);
  return GetStream(s);
  }

public void PSPushStream(stm, executable) Stm stm; long int executable; {
  Object obj;
  MakePStm(stm, (cardinal)executable, &obj);
  PushP(&obj);
  }

/* support for objs */

public void PSPopTempObject(type, pobj) PSOperandType type; PObject pobj; {
  PSOperandType objType = PSGetOperandType();
  if (objType == dpsOtherObj || (type != dpsAnyObj && type != objType))
    TypeCheck();
  PopP(pobj);
  }

public void PSPushObject(pobj) PObject pobj; {
  PushP(pobj);
  }

private ManagedItem *ValidatePObj(pobj) PObject pobj; {
  ManagedItem *mi = (ManagedItem *)pobj;
  ItemChunk *chunk = chunks;
  while (chunk != NULL) {
    ManagedItem *miBase = &chunk->items[0];
    if (mi >= miBase && mi < miBase + ITEMSPERCHUNK) {
      if (((PCard8)mi - (PCard8)miBase) % sizeof(ManagedItem) != 0)
        Undefined();
      return mi;
      }
    chunk = chunk->next;
    }
  Undefined();
  }

private procedure RecycleMI(mi) ManagedItem *mi; {
  mi->obj = iLNullObj;
  mi->obj.length = 1;
  mi->homeVM = NIL;
  mi->obj.val.ival = (integer)miFree;
  miFree = mi;
  }

private procedure myTerminateSpace(e) StaticEvent e; {
  integer i;
  ManagedItem *mi;
  ItemChunk *chunk = chunks;
  Assert(e == staticsSpaceDestroy);
  while (chunk != NULL) {
    for (i = 0; i < ITEMSPERCHUNK; i++) {
      mi = &chunk->items[i];
      if ( mi->homeVM == vmPrivate ) RecycleMI(mi);
      }
    chunk = chunk->next;
    }
  }

private procedure myGetRoots(clientData, info)
  RefAny clientData; GC_Info info; {
  GC_CollectionType ct = GC_GetCollectionType(info);
  integer i;
  ManagedItem *mi;
  ItemChunk *chunk = chunks;
  while (chunk != NULL) {
    for (i = 0; i < ITEMSPERCHUNK; i++) {
      mi = &chunk->items[i];
      if ( mi->homeVM != NIL ) {
	if (ct == sharedVM) {
	  if ( mi->obj.shared )
	    GC_Push(info, &mi->obj);
	  }
	else {
	  if ( mi->homeVM == vmPrivate )
	    GC_Push(info, &mi->obj);
	  }
	}
      }
    chunk = chunk->next;
    }
  }

  
public PPSObject PSPopManagedObject(type) PSOperandType type; {
  PSOperandType objType = PSGetOperandType();
  integer i;
  ManagedItem *mi = miFree;
  if (objType == dpsOtherObj || (type != dpsAnyObj && type != objType))
    TypeCheck();
  if (mi == NULL) { /* get a new chunk */
    ItemChunk *new = (ItemChunk *)os_newelement(chunkPool);
    new->next = chunks;
    chunks = new;
    for (i = 0; i < ITEMSPERCHUNK; i++) {
      RecycleMI(&new->items[i]);
      }
    mi = miFree;
    }
  miFree =  (ManagedItem *)mi->obj.val.ival;
  DebugAssert(mi->obj.type == nullObj);
  PopP(&mi->obj);
  ConditionalInvalidateRecycler (&mi->obj);
  mi->ref = 1;
  if (mi->obj.shared) mi->homeVM = vmShared;
  else mi->homeVM = vmPrivate;
  return (PPSObject) &mi->obj;
  }

public void PSReleaseManagedObject(pobj) PObject pobj; {
  ManagedItem *mi = ValidatePObj(pobj);
  if (--mi->ref == 0)
    RecycleMI(mi);
  }

public PSOperandType PSGetObjectType(pobj) PObject pobj; {
  integer type;
  return TypeOfObj(pobj);
  }


public void PSPopPMtx(m) PMtx m; {
  AryObj ao;  PopPArray(&ao);  PAryToMtx(&ao, m);
  }
  
public void PSPushPMtx(pobj, m) PObject pobj; PMtx m; {
  MtxToPAry(m, pobj);
  PushP(pobj);
  }
  
public procedure PSExecuteOperator(index) integer index; {
  PNameArrayBody pna = rootShared->nameMap.val.namearrayval;
  Object obj;
  PNameEntry pne;
  if (index < 0 || index >= pna->length || (pne = pna->nmEntry[index]) == NIL)
    PSUndefined();
  LNameObj(obj, pne);
  DictGetP(rootShared->vm.Shared.sysDict, obj, &obj);
  if (obj.type != cmdObj || obj.tag == Lobj) {
    if (psExecute(obj))
      RAISE((int)GetAbort(), (char *)NIL);
    }
  else
    (*cmds.cmds[obj.length])();
  }

public boolean PSExecuteObject(pobj) PObject pobj; {
  return psExecute(*pobj);
  }


public boolean PSExecuteString(sP) char *sP; {
  return psExecute(MakeXStr((string)sP));
  }

public procedure PSHandleExecError() {
  if (GetAbort() != 0)
    RAISE((int)GetAbort(), (char *)NIL);
  }


public PPSGState PSPopGState() {
  Object obj;
  PopP(&obj);
  if (obj.type == nullObj)
    return (PPSGState)gs;
  if (obj.type != escObj || obj.length != objGState) TypeCheck();
  if ((obj.access & rAccess) == 0) InvlAccess();
  return (PPSGState) obj.val.gstateval;
  }

public PMtx PSGetMatrix(p) PPSGState p; {
  PGState g = (p != NULL) ? (PGState) p : gs;
  return &g->matrix;
  }

public PDevice PSGetDevice(p) PPSGState p; {
  PGState g = (p != NULL) ? (PGState) p : gs;
  return g->device;
  }

public void PSGetMarkInfo(p, info) PPSGState p; DevMarkInfo *info; {
  PGState g = (p != NULL) ? (PGState) p : gs;
  info->offset.x = 0;
  info->offset.y = 0;
  info->color = g->color->color;
  info->halftone = (g->screen == NULL) ? NULL : g->screen->halftone;
  info->screenphase = g->screenphase;
  info->priv = (unsigned char *) &g->extension;
  }

/* PSGetTfrFcn:  used by nextimage to build its DevImage
   in level 2 probably will need to be exapnded to get gamut transfer
   and color rendering as well
   */
public DevTfrFcn *PSGetTfrFcn()
{
    /* code swiped from first few lines of ImageInternal in graphics/image.c */
    if (gs->tfrFcn != NULL) {
	if (!gs->tfrFcn->active) ActivateTfr(gs->tfrFcn);
	return(gs->tfrFcn->devTfr);
    }
    return(0);
}

public char *PSGetGStateExt(p) PPSGState p; {
  PGState g = (p != NULL) ? (PGState) p : gs;
  return (char *)&g->extension;
  }


public boolean PSBuildMasks() {return gs->noColorAllowed;}

public boolean PSGetClip(clip) DevPrim **clip; {
  *clip = GetDevClipPrim();
  return DevClipIsRect();
  }

public boolean PSReduceRect(x, y, w, h, mtx, dp)
  real x, y, w, h; PMtx mtx; DevPrim *dp; {
  Cd pt;
  boolean rect;
  QuadPath qp;
  BBoxRec bbox;
  pt.x = x;
  pt.y = y;
  if (mtx == NULL)
    mtx = &gs->matrix;
  rect = ReduceQuadPath(
    pt, w, h, mtx, dp, &qp, &bbox, (Path *)NULL);
  return rect;
  }

public boolean PSClipInfo(ll, ur) Cd *ll, *ur; {
  *ll = gs->clip.bbox.bl;
  *ur = gs->clip.bbox.tr;
  return gs->clip.isRect;
  }

public Object PSLNullObj() {
  Object obj;
  LNullObj(obj);
  return obj;
  }
	
public Object PSLIntObj(value) long int value; {
  Object obj;
  LIntObj(obj, value);
  return obj;
  }

public procedure PSPutArray(a, index, elem)
  PObject a, elem; integer index; {
  if (a->type != arrayObj)
    PSTypeCheck();
  if (index < 0 || index >= a->length)
    PSRangeCheck();
  ConditionalInvalidateRecycler(a);
  VMPutElem(*a, index, *elem);
  return;
  }

public procedure PSSetRealClockAddress(clockaddr)
  PCard32 clockaddr; {
  SetRealClockAddress(clockaddr);
  }

/* new operator added by pgraff, 24 march 90 */

public procedure PSDictGetPObj(dict, str, type, val)
PObject dict,val;
char *str;
PSOperandType type;
{
    PSOperandType objType;
    NameObj name;

    if(dict->type != dictObj)
	PSTypeCheck();
    MakePName(str, &name);
    DictGetP(*dict, name, val);		/* will raise undefined error */
    objType = TypeOfObj(val);
    if (objType == dpsOtherObj || (type != dpsAnyObj && type != objType))
	TypeCheck();
}
/* return whether key was found or not (does not raise undefined error) */
public boolean PSDictGetTestPObj(dict, str, type, val)
PObject dict,val;
char *str;
PSOperandType type;
{
    PSOperandType objType;
    NameObj name;

    if(dict->type != dictObj)
	PSTypeCheck();
    MakePName(str, &name);
    if(!DictTestP(*dict, name, val, true))
	return false;
    DictGetP(*dict, name, val);	
    objType = TypeOfObj(val);
    if (objType == dpsOtherObj || (type != dpsAnyObj && type != objType))
	TypeCheck();
    return true;
}

public procedure CustomOpsInit (reason) InitReason reason; {
  integer i;
  switch (reason) {
   case init:
#if OS==os_mpw
    globals = (Globals)os_sureCalloc(sizeof(GlobalsRec),1);
#endif OS==os_mpw
    miFree = NULL;
    chunks = NULL;
    chunkPool = os_newpool(sizeof(ItemChunk), 5, 0);
    GC_RgstGetRootsProc(myGetRoots, (RefAny)NIL);
    GC_RgstSharedRootsProc(myGetRoots, (RefAny)NIL);
    RegisterData(
      (PCard8 *)NIL, (integer)0, myTerminateSpace, 
      (integer)STATICEVENTFLAG(staticsSpaceDestroy));
    break;
    }
  }


