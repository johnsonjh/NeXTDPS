/*
  saverestore.c

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

Original version: 
Edit History:
Terry Donahue: Sat Apr 28 04:34:11 1990
Scott Byer: Fri Jun  2 09:34:11 1989
Ivor Durham: Mon May  8 15:34:22 1989
Ed Taft: Sun Dec 17 14:09:37 1989
Joe Pasqua: Wed Jul  5 16:18:32 1989
Jim Sandman: Tue Mar 15 14:36:18 1988
Perry Caro: Tue Nov 15 11:46:59 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include GC
#include ORPHANS
#include PSLIB
#include RECYCLER
#include VM

#include "abm.h"
#include "saverestore.h"
#include "vm_memory.h"
#include "vm_segment.h"

/* The following procedures are from the language pkg.	*/
/* language.h can't be imported since that would cause	*/
/* circular dependencies.				*/
extern procedure DecodeObj(), PushP(), PopP();

/* Data Structures */

typedef struct paramrec {
  struct paramrec *next;
  char *param;
  integer size;
  PVoidProc proc;
  Level saveLevel;
} Param, *PParam;

typedef struct {
  PVMRoot   rootPrivate;
  PParam    parameterList;
} SaveRestore_Data, *PSaveRestore_Data;

typedef struct {
  procedure (*proc) ( /* level */ );
} SRQitem;

/* Constants */

#define MAXnSRQProcs 10

/* Inline procedures */

#define VMObjPtr(o) ((PObject)(o))

#define SavObjMacro(o) { \
  if (VMObjPtr(o)->level != level) saveobj(o);}

#define CheckSharable(o) { \
  if (! (o).shared) FInvlAccess();}

/*
 * Main PS State
 */

public	PVMRoot	rootShared, rootPrivate;

/*
 * Save/Restore State 
 */

PSaveRestore_Data saveRestoreData;

#define	paramList (saveRestoreData->parameterList)

public	Level	 level;

public Card16 finalizeReasons[nObTypes];
public FinalizeProc finalizeProcs[nObTypes];

#if (OS != os_mpw)
/*-- BEGIN GLOBALS --*/

private SRQitem SaveProcs[MAXnSRQProcs], RstrProcs[MAXnSRQProcs];
private boolean (*StackChecker)();
private PVoidProc SaveSemaphore;
private integer nSProcs, nRProcs;
private boolean inRestore;

/*-- END GLOBALS --*/
#else (OS != os_mpw)

typedef struct {
 SRQitem g_SaveProcs[MAXnSRQProcs], g_RstrProcs[MAXnSRQProcs];
 boolean (*g_StackChecker)();
 PVoidProc g_SaveSemaphore;

 integer g_nSProcs, g_nRProcs;
 boolean g_inRestore;
} GlobalsRec, *Globals;

private Globals globals;

#define SaveProcs globals->g_SaveProcs
#define RstrProcs globals->g_RstrProcs
#define StackChecker globals->g_StackChecker
#define SaveSemaphore globals->g_SaveSemaphore
#define nSProcs globals->g_nSProcs
#define nRProcs globals->g_nRProcs
#define inRestore globals->g_inRestore

#endif (OS != os_mpw)

public procedure VMCarCdr(pao, pobj)
 PAnyAryObj pao;
  PObject pobj;
{
  switch (pao->type) {
   case arrayObj:
    *pobj = *pao->val.arrayval;
    if (--pao->length == 0)
      pao->val.arrayval = (PObject)NIL;
    else pao->val.arrayval++;
    return;
   case pkdaryObj:
    DecodeObj (pao, pobj);
    return;
   default:
    CantHappen ();
  }
/*NOTREACHED*/
}				/* end of VMCarCdr */

private saveobj(o)
  PObject o;
{
  PSRO    srop;

  Assert (o->level <= level);
  if ( (srop = (PSRO)ABM_AllocateVM((integer)sizeof(SRO), false)) == NULL) {
    srop = (PSRO) AllocVMAligned ((integer) sizeof (SRO), false);
    ConditionalResetRecycler (privateRecycler, (PCard8)srop);
  }

  srop->link = rootPrivate->vm.Private.srList->objs;
  rootPrivate->vm.Private.srList->objs = srop;
  srop->pointer = o;
  srop->o = *o;
}				/* end of saveobj */

public procedure VMCopyArray(src, dst)
AryObj src,dst;
{
  register integer i,
          count = MIN (src.length, dst.length);
  register PObject sp = src.val.arrayval;
  register PObject dp = dst.val.arrayval;
  PRecycler  recycler = RecyclerForObject (&dst);
  boolean dstRecyclable = IsRecyclable (&dst, recycler);
  
  if (dp == sp) return;
  if (((src.access & rAccess) == 0) || ((dst.access & wAccess) == 0))
    InvlAccess ();
  if (dp < sp) {
    for (i = 0; i < count; i++) {
      if (dst.shared) {CheckSharable(*sp);}
      else {SavObjMacro (dp);}
      recycler = RecyclerForObject (sp);
      if (!dstRecyclable && IsRecyclable (sp, recycler))
        ConditionalResetRecycler (recycler, RecyclerAddress (sp));
      *dp = *(sp++);
      (dp++)->level = (dst.shared)? 0 : level;
    }
    return;
  }
  sp += count;
  dp += count;
  for (i = count; i > 0; i--) {
    sp--;
    dp--;
    if (dst.shared) {CheckSharable(*sp);}
    else {SavObjMacro (dp);}
    recycler = RecyclerForObject (sp);
    if (!dstRecyclable && IsRecyclable (sp, recycler))
      ConditionalResetRecycler (recycler, RecyclerAddress (sp));
    *dp = *sp;
    dp->level = (dst.shared)? 0 : level;
  }
}				/* end of VMCopyArray */

public procedure VMCopyString(src,dst)  StrObj src,dst;
{
/* callers of this proc ensure that dst is large enough */

  if (((src.access & rAccess) == 0) || ((dst.access & wAccess) == 0))
    InvlAccess ();
  if (src.length != 0)
    os_bcopy ((char *) src.val.strval, (char *) dst.val.strval,
              (long int) src.length);
}				/* end of VMCopyString */

public procedure VMGetText(so, str)  StrObj so; string str;
{
  register integer i;
  register charptr sp = so.val.strval;

  if ((so.access & rAccess) == 0)
    InvlAccess ();
  str[(i = so.length)] = NUL;
  if (i == 0)
    return;
  if (str == sp)
    return;
  do {
    *(str++) = *(sp++);
  } while (--i != 0);
}				/* end of VMGetText */

public procedure VMPutNChars(so, str, count)
  StrObj so; string str; integer count;
{
  string dp = so.val.strval;

  if ((so.access & wAccess) == 0)
    InvlAccess ();
  if (dp != str) os_bcopy((char *)str, (char *)dp, count);
}

public procedure VMPutText(so, str)  StrObj so; string str;
{
  VMPutNChars(so, str, (integer)StrLen(str));
}				/* end of VMPutText */

public procedure VMObjForPString(s, pstrob)
  string s;  register PStrObj pstrob;
{
  LStrObj (*pstrob, StrLen (s), s);
#if VMINIT
  Assert(pstrob->val.strval == s);
#endif VMINIT
}				/* end of VMObjForPString */

public procedure VMPutDict(d, dp)
  DictObj d;
  PDictBody dp;
{
  PSRD    srdp;
  PDictBody vdp = d.val.dictval;

  if (! d.shared && vdp->level != level) {
    Assert (vdp->level <= level);
    if ((srdp = (PSRD)(ABM_AllocateVM((integer)sizeof(SRD), false))) == NULL) {
      srdp = (PSRD) AllocVMAligned ((integer) sizeof (SRD), false);
      ConditionalResetRecycler (privateRecycler, (PCard8)srdp);
    }
    srdp->link = rootPrivate->vm.Private.srList->dbs;
    rootPrivate->vm.Private.srList->dbs = srdp;
    srdp->pointer = vdp;
    srdp->db = *vdp;
  }
  *vdp = *dp;
  vdp->level = (d.shared)? 0 : level;
}				/* end of VMPutDict */

public procedure VMPutDKeyVal(dp, vmkvp, kvp)
  PDictBody dp;
  PKeyVal vmkvp, kvp;
{
  if (dp->shared) {CheckSharable(kvp->key); CheckSharable(kvp->value);}
  else {SavObjMacro (&(vmkvp->key)); SavObjMacro (&(vmkvp->value));}
  *vmkvp = *kvp;
  vmkvp->key.level = vmkvp->value.level = (dp->shared)? 0 : level;
}				/* end of VMPutDKeyVal */

public procedure VMPutDValue(dp, kvp, obp)
  PDictBody dp;
  PKeyVal kvp;
  PObject obp;
{
  if (dp->shared) {CheckSharable(*obp);}
  else {SavObjMacro (&(kvp->value));}
  kvp->value = *obp;
  kvp->value.level = (dp->shared)? 0 :level;
}				/* end of VMPutDValue */

public procedure VMPutElem(ao, i, ob)
  AryObj ao;
  cardinal i;
  Object ob;
{
  PObject  op = &(ao.val.arrayval)[i];
  PRecycler recycler;

  if ((ao.access & wAccess) == 0)
    InvlAccess ();

  recycler = RecyclerForObject (&ao);
  if (!IsRecyclable (&ao, recycler)) {
      recycler = RecyclerForObject (&ob);
      ConditionalResetRecycler(recycler, RecyclerAddress (&ob));
  }
  if (ao.shared) {CheckSharable(ob);}
  else {SavObjMacro (op);}
  *op = ob;
  if (ao.shared)
    {
    op->level = 0;
    op->seen = !(vmShared->valueForSeen);
    }
  else
    {
    op->level = level;
    op->seen = !(vmPrivate->valueForSeen);
    }
}				/* end of VMPutElem */

public procedure VMPutChar(so, i, c)
  StrObj so;
  integer i;
  character c;
{
  charptr  s = so.val.strval;

  if ((so.access & wAccess) == 0)
    InvlAccess ();
  s[i] = c;
}				/* end of VMPutChar */

public procedure VMPutGeneric (gObject, newBody)
  Object gObject;
  PCard8 newBody;
 /*
   See interface for specification.

   This implementation assumes absolutely addressed VM that does not move when
   it is expanded.
  */
{
  PGenericBody pBody = gObject.val.genericval;
  PSRG    pSaveObject;

#if	STAGE == DEVELOP
  Assert ((gObject.type == escObj) && (gObject.length == pBody->type));
#endif	STAGE == DEVELOP

  if ((gObject.access & wAccess) == 0)
    InvlAccess ();

  if (! gObject.shared && pBody->level != level)
    {
    Assert (pBody->level <= level);
    pSaveObject = (PSRG)ABM_AllocateVM(
      (integer)(sizeof (SRG) + pBody->length - sizeof(GenericBody)), false);
    if (pSaveObject == NULL) {
      pSaveObject = (PSRG)AllocVMAligned(
        (integer)(sizeof (SRG) + pBody->length - sizeof(GenericBody)), false);
      ConditionalResetRecycler (privateRecycler, (PCard8)pSaveObject);
    }
    os_bcopy(
      (char *)pBody, (char *)&pSaveObject->header, (long int)pBody->length);
    pSaveObject->link = rootPrivate->vm.Private.srList->generics;
    pSaveObject->pointer = pBody;
    rootPrivate->vm.Private.srList->generics = pSaveObject;
    }
  else
    CallFinalizeProc(pBody->type, gObject, fr_overwrite);

  os_bcopy(
    (char *)newBody, (char *)&pBody[1], 
    (long int)(pBody->length - sizeof(GenericBody)));
  pBody->level = (gObject.shared) ? 0 : level;
}

public procedure VMCopyGeneric (src, dst)
  Object src, dst;
 /*
    See interface for specification.

    NOTE: This is not yet a pure generic implementation.  It is for
    gstateObjects for now.
  */
{
  PGenericBody pSrc = src.val.genericval,
	       pDst = dst.val.genericval;

#if	(STAGE == DEVELOP)
  Assert ((src.type == escObj) && (src.length == pSrc->type) &&
    (dst.type == escObj) && (dst.length == pDst->type)
    && (pSrc->length == pDst->length));
#endif	(STAGE == DEVELOP)

  if (pSrc->type != pDst->type)
    TypeCheck ();

  if (((src.access & rAccess) == 0) || ((dst.access & wAccess) == 0))
    InvlAccess ();

  if (pSrc == pDst) {
    return;
  }

  VMPutGeneric (dst, (PCard8) &pSrc[1]);

  CallFinalizeProc(pDst->type, dst, fr_copy);
}

private procedure RstrGenericObjects (srp, n)
  PSR srp;
  Level n;
{
  PSRG    psrg = srp->generics;
  GenericObj gob;

  until (psrg == NIL) {
    if (psrg->header.level <= n) {
      PGenericBody pBody = psrg->pointer;

      if (WantToFinalize(pBody->type, fr_overwrite)) {
	LGenericObj(gob, pBody->type, psrg->pointer);
	(*finalizeProcs[pBody->type])(gob, fr_overwrite);
      }

      os_bcopy((char *)&psrg->header, (char *)pBody, (long int)pBody->length);
      pBody->seen = !vmPrivate->valueForSeen;
    } else
      if (WantToFinalize(psrg->header.type, fr_restore)) {
	LGenericObj(gob, psrg->header.type, &psrg->header);
	(*finalizeProcs[psrg->header.type])(gob, fr_restore);
      }

    psrg = psrg->link;
  }
}

public procedure WriteContextParam (param, newValue, size, proc)
  char *param, *newValue;
  integer size;
  PVoidProc proc;
 /*
   See vm.h for specification.

   Search chain of saved parameters for this parameter at the current
   save level.  If it is not found, a new entry is added to the front
   of the paramList containing a copy of the original value of param.

   The data pointed at by param is overwritten with the data pointed
   at by newValue.
  */
{
  PParam parameter = paramList;
  boolean saveValue = (level > 0);

  while (parameter != NIL) {
    if (parameter->param == param) {
      saveValue = (parameter->saveLevel != level);
      break;
    } else if (parameter->saveLevel < level)
      break;
    else
      parameter = parameter->next;
  }

  if (saveValue) {
    /* Save original value on list of changed parameters */

    parameter = (PParam) NEW (1, (sizeof (Param) + size));

    parameter->param = param;
    parameter->size = size;
    parameter->proc = proc;
    parameter->saveLevel = level;

    os_bcopy (param, (char *)&parameter[1], size);

    parameter->next = paramList;
    paramList = parameter;
  }

  os_bcopy (newValue, param, size);
}

private procedure RestoreContextParams (level)
  Level level;
  /*
   Restore parameters from the list back to the specified level.
  */
{
  PParam parameter = paramList;

  while (parameter != NIL) {
    if (parameter->saveLevel <= level)
      break;
    else {
      /* Restore value */
      os_bcopy ((char *)&parameter[1], parameter->param, parameter->size);

      if (parameter->proc != NIL)
        (*parameter->proc)(parameter->param, parameter->size);

      paramList = parameter->next;
      os_free ((char *)parameter);
      parameter = paramList;
    }
  }
}

public procedure VMRgstFinalize(type, proc, reasonSet)
  Card8 type; FinalizeProc proc; FinalizeReasonSet reasonSet;
{
#if	(STAGE == DEVELOP)
  Assert (type < nObTypes);
#endif	(STAGE == DEVELOP)
  finalizeReasons[type] = reasonSet;
  finalizeProcs[type] = proc;
}

public procedure _RecordFinalizableObject(obj)
  Object obj;
{
  PFinalizeNode pNode;
  PVMRoot root = ((&obj)->shared) ? rootShared : rootPrivate;

#if (STAGE==DEVELOP)
  Assert(obj.shared || obj.level == level);
  Assert(root->finalizeChain == NIL ||
     obj.level >= root->finalizeChain->obj.level);
  Assert(finalizeReasons[(obj.type == escObj)? obj.length : obj.type] != 0);
#endif (STAGE==DEVELOP)
  pNode = (PFinalizeNode)ABM_AllocateVM((integer)sizeof(FinalizeNode), obj.shared);
  if (pNode == NULL) {
    PRecycler recycler = RecyclerForObject (&obj);
    pNode = (PFinalizeNode) AllocVMAligned(
      (integer) sizeof(FinalizeNode), obj.shared);
    ConditionalResetRecycler (recycler, (PCard8)pNode);
  }
  pNode->obj = obj;
  pNode->link = root->finalizeChain;
  root->finalizeChain = pNode;
}

private procedure HandleGCFinalize(clientData, info)
/* Performs finalization for all finalizable objects in the VM being
   collected. Objects that are NOT reclaimed are "finalized" with
   reason fr_privateEnum or fr_sharedEnum. */
RefAny clientData;	/* UNUSED	*/
GC_Info info;		/* GC State	*/
{
  register PFinalizeNode pNode, prev;
  register Card8 type;
  FinalizeReason thisReason, reclaimReason, enumReason;
  PVMRoot root;

  switch (GC_GetCollectionType(info)) {
    case sharedVM:
      reclaimReason = fr_sharedReclaim;
      enumReason = fr_sharedEnum;
      root = rootShared;
      break;
    case privateVM:
      reclaimReason = fr_privateReclaim;
      enumReason = fr_privateEnum;
      root = RootPointer(GC_GetSpace(info));
      break;
    case privateForShared:
      reclaimReason = fr_privateReclaim;
      enumReason = fr_sharedEnum;
      root = RootPointer(GC_GetSpace(info));
      break;
    default: CantHappen();
    }

  prev = NIL;
  for (	/* FOR EACH node in the finalize chain DO	*/
    prev = NIL, pNode = root->finalizeChain;
    pNode != NIL; pNode = pNode->link)
    {
    if (GC_WasCollected(&pNode->obj, info))
      {
      if (prev == NIL) root->finalizeChain = pNode->link;
      else prev->link = pNode->link;
      thisReason = reclaimReason;
      }
    else	/* It wasn't reclaimed so we want to enum it	*/
      {
      prev = pNode;
      thisReason = enumReason;
      }

    if ((type = pNode->obj.type) == escObj) type = pNode->obj.length;
    CallFinalizeProc(type, pNode->obj, thisReason);
    }
}

private procedure PerformFinalization(root, reason, level)
  PVMRoot root; FinalizeReason reason; Level level;
/* Performs finalization for all finalizable objects in the VM specified
   by root and the specified reason. If reason is fr_restore, level is the
   save level being discarded (in a multi-level restore, this procedure
   is called once per level discarded).  */
{
  register PFinalizeNode pNode, prev;
  register Card8 type;
  boolean reclaim;

  prev = NIL;
  for (pNode = root->finalizeChain; pNode != NIL; pNode = pNode->link)
    {
    switch (reason)
      {
      case fr_restore:
        if (pNode->obj.level < level) return;
#if (STAGE==DEVELOP)
	Assert(pNode->obj.level == level);
#endif (STAGE==DEVELOP)
	reclaim = true;
	break;
      case fr_destroyVM:
        reclaim = true;
	break;
      default:
        reclaim = false;
      }

    if (reclaim)
      {
      if (prev == NIL) root->finalizeChain = pNode->link;
      else prev->link = pNode->link;
      }
    else prev = pNode;

    if ((type = pNode->obj.type) == escObj) type = pNode->obj.length;
    CallFinalizeProc(type, pNode->obj, reason);
    }
}

private procedure RstrObj(srp, n)
  PSR srp;
  Level n;
{
  register PSRO srop = srp->objs;
  register PObject op;
  register PNameEntry pne;

  until (srop == NIL) {
    if (srop->o.level <= n) {
      op = srop->pointer;
      if (op->type == nameObj) {
	/* If we are replacing a name object, we may be removing a key
	   from a dictionary; update name cache entry if necessary */
	pne = op->val.nmval;
	if (op == &(pne->kvloc->key))
	  {
	  pne->kvloc = NULL;
	  pne->dict = NULL;
	  pne->ts.stamp = 0;
	  }
        }
      *op = srop->o;
      op->seen = !vmPrivate->valueForSeen;
    }
    srop = srop->link;
  }
}				/* end of RstrObj */

private RstrDB(srp, n)
  PSR srp;
 Level n;
{
  register PSRD srdp = srp->dbs;
  register PKeyVal	kvp;
  register PNameEntry	pne;
  register int		i;

  until (srdp == NIL) {
    if (srdp->db.level <= n) {
      if(srdp->db.begin != srdp->pointer->begin) {
      	kvp = srdp->pointer->begin;
      	for(i=0; i < srdp->pointer->size; i++) {
      		if(kvp->key.type == nameObj) {	/* invalidate name cache */
			pne = kvp->key.val.nmval;
			pne->kvloc = NULL;
			pne->dict = NULL;
			pne->ts.stamp = 0;
			

		}
	kvp++;
      	} /* end for */
      }
      *(srdp->pointer) = srdp->db;
      srdp->pointer->seen = !vmPrivate->valueForSeen;
    }
    srdp = srdp->link;
  }
}				/* end of RstrDB */

private procedure ForAllSProcs(l)
  Level l;
{
  cardinal i;

  for (i = 0; i < nSProcs; (*SaveProcs[i++].proc) (l));
}				/* end of ForAllSProcs */

private ForAllRProcs(l)
  Level l;
{
  integer i;

  for (i = nRProcs; i > 0; (*RstrProcs[--i].proc) (l));
}				/* end of ForAllRProcs */

public procedure RgstSaveProc(proc)
  procedure (*proc)();
{
  if ((nSProcs + 1) >= MAXnSRQProcs)
    CantHappen ();
  SaveProcs[nSProcs++].proc = proc;
}				/* end of RgstSaveProc */
  
public procedure RgstRstrProc(proc)
  procedure (*proc)();
{
  if ((nRProcs + 1) >= MAXnSRQProcs)
    CantHappen ();
  RstrProcs[nRProcs++].proc = proc;
}				/* end of RgstRstrProc */

public procedure RgstStackChecker (proc)
  boolean (*proc)();
{
  StackChecker = proc;
}

public boolean AddressValidAtLevel(address, lvl)
  PCard8 address; Level lvl;
{
  register PVMSegment segment;
  register PSR srp;

  if (address == NIL) return true;  /* NIL is always valid */

  /* Find the segment containing address. If the segment's max level
     is no greater than the requested level then all addresses in
     that segment are valid, so return true without further ado. */
  segment = FindVMSegment(vmPrivate, address);
  Assert(segment != NIL);
  if (lvl >= segment->level) return true;

  /* At least some addresses in the segment are not valid at the requested
     level. Find the save/restore object for level lvl + 1, which contains
     the free pointer that will result from a restore to level lvl. */
  for (srp = rootPrivate->vm.Private.srList; ; srp = srp->link)
    {
    DebugAssert(srp != NIL);
    if (srp->level == lvl + 1) break;
    }

  /* The address is valid if it is in the same segment as the free pointer
     and is less than the free pointer. */
  return (FindVMSegment(vmPrivate, srp->free) == segment &&
          address < srp->free);
}

public procedure RgstSaveSemaphoreProc (proc)
  PVoidProc proc /*( int level )*/;
 /*
   The specified procedure is called when 
   greater than zero.
  */
{
  SaveSemaphore = proc;
}

private procedure ResetAllocCounter(space, newFree)
  PVM space;
  PCard8 newFree;
  {
  register long int cur;
  register PVMSegment seg, lastSeg;
  
  cur = space->allocCounter;
  lastSeg = FindVMSegment(space, newFree);
  for (seg = FindVMSegment(space, space->free); cur > 0; seg = seg->pred)
    {
    register PCard8 thisFree;

    thisFree = (seg == space->current) ? space->free : seg->free;
    if (seg == lastSeg)
      {
      cur -= thisFree - newFree;
      break;	/* NOTE the break	*/
      }
    cur -= thisFree - seg->first;
    }
  space->allocCounter = (cur > 0) ? cur : 0;
  } /* ResetAllocCounter */

public RstrToLevel(n, startup)  Level n;  boolean startup;
{
  PSR srp = rootPrivate->vm.Private.srList;

  Assert (n <= level);
  if (!startup) {
    if ((*StackChecker) (n))
      PSError (invlrestore);
  }
  inRestore = true;
  DURING {
#if (STAGE==DEVELOP)
    until (srp == NIL) {
      if (srp->level == n)
	break;
      Assert (srp->level > n);
      srp = srp->link;
    }
#endif (STAGE==DEVELOP)
    ForAllRProcs (n);
    srp = rootPrivate->vm.Private.srList;
    until (srp == NIL) {
      if (srp->level == n)
	break;
      Assert (srp->level > n);
      RstrObj (srp, n);
      RstrDB (srp, n);
      RstrGenericObjects (srp, n);
      RestoreContextParams (n);
      PerformFinalization(rootPrivate, fr_restore, srp->level);
      ResetAllocCounter(vmPrivate, srp->free);
      ResetVMSection (vmPrivate, srp->free, n);
      srp = srp->link;
    }
  }
  HANDLER
  {
    inRestore = false;
    RERAISE;
  }
  END_HANDLER;
  inRestore = false;

  if (SaveSemaphore != NIL)
    (*SaveSemaphore) (n - level);

  NOLL.level = level = n;
  rootPrivate->vm.Private.level = level;
  rootPrivate->vm.Private.srList = srp;
}				/* end of RstrToLevel */

private Save()
{
  PCard8  free = vmPrivate->free;
  PSR     srp;

  if (level >= MAXlevel)
    LimitCheck ();
  /* Do the AllocVMAligned first in case of LimitCheck */
  if ( (srp = (PSR)ABM_AllocateVM((integer)sizeof(SR), false)) == NULL) {
    srp = (PSR) AllocVMAligned ((integer) sizeof (SR), false);
    ConditionalResetRecycler (privateRecycler, (PCard8)srp);
  }

  /* Now update level etc. */
  NOLL.level = ++level;
  NoteLevel(vmPrivate, level);
  srp->link = rootPrivate->vm.Private.srList;
  rootPrivate->vm.Private.srList = srp;
  srp->free = free;
  srp->level = level;
  srp->objs = NIL;
  srp->dbs = NIL;
  srp->generics = NIL;

  rootPrivate->vm.Private.level = level;
  ForAllSProcs (level);

  if (SaveSemaphore != NIL)
    (*SaveSemaphore) (1);
}				/* end of Save */

public procedure PSSave()
{
  SaveObj sv;
  Level   oldlevel = level;

  Save ();
  LSaveObj (sv, oldlevel);	/* init here to get old value of level for
				 * value but new value of level of save
				 * object */
  PushP (&sv);
}				/* end of PSSave */

public procedure PSRstr()
{
  Object  sv;

  PopP (&sv);
  if (sv.type != escObj || sv.length != objSave)
    TypeCheck ();
  Assert (sv.val.saveval < level);
  RstrToLevel ((Level) sv.val.saveval, false);
}				/* end of PSRstr */

public NullObj NOLL;	/* maintained at current save level */

public readonly NullObj	iLNullObj = {Lobj, 0, nullObj, true, 0, 0, 0, 0};
public readonly IntObj	iLIntObj = {Lobj, 0, intObj, true, 0, 0, 0, 0};
public readonly RealObj	iLRealObj = {Lobj, 0, realObj, true, 0, 0, 0, 0};
public readonly NameObj
  iLNameObj = {Lobj, 0, nameObj, true, 0, 0, 0, 0},
  iXNameObj = {Xobj, 0, nameObj, true, 0, 0, 0, 0};
public readonly BoolObj	iLBoolObj = {Lobj, 0, boolObj, true, 0, 0, 0, 0};
public readonly StrObj
  iLStrObj = {Lobj, aAccess, strObj, false, 0, 0, 0, 0},
  iXStrObj = {Xobj, aAccess, strObj, false, 0, 0, 0, 0};
public readonly StmObj	iLStmObj = {Lobj, aAccess, stmObj, false, 0, 0, 0, 0};
public readonly CmdObj	iXCmdObj = {Xobj, 0, cmdObj, true, 0, 0, 0, 0};
public readonly DictObj	iLDictObj = {Lobj, 0, dictObj, false, 0, 0, 0, 0};
public readonly AryObj
  iLAryObj = {Lobj, aAccess, arrayObj, false, 0, 0, 0, 0},
  iXAryObj = {Xobj, aAccess, arrayObj, false, 0, 0, 0, 0};
public readonly PkdaryObj
  iLPkdaryObj = {Lobj, (rAccess | xAccess), pkdaryObj, false, 0, 0, 0, 0},
  iXPkdaryObj = {Xobj, (rAccess | xAccess), pkdaryObj, false, 0, 0, 0, 0};
public readonly MarkObj
  iLMarkObj = {Lobj, 0, escObj, true, 0, 0, 0, objMark};
public readonly SaveObj
  iLSaveObj = {Lobj, 0, escObj, false, 0, 0, 0, objSave};
public readonly FontObj
  iLFontObj = {Lobj, 0, fontObj, true, 0, 0, 0, 0};
public readonly GStateObj
  iLGStateObj = {Lobj, (rAccess | wAccess), escObj, false, 0, 0, 0, objGState};
public readonly LockObj
  iLLockObj = {Lobj, 0, escObj, false, 0, 0, 0, objLock};
public readonly CondObj
  iLCondObj = {Lobj, 0, escObj, false, 0, 0, 0, objCond};
public readonly GenericObj
  iLGenericObj = {Lobj, (rAccess | wAccess), escObj, false, 0, 0, 0, 0};

public procedure LoadVM (newVM)
  PVM newVM;
 /*
   LoadVM ensures that the VM environment is loaded properly when creating
   a new space or context.  It's work is partially duplicated by the static
   handler below, but this operation is needed for manipulating spaces
   independent of any context.

   Allocation is set to the new private VM.
  */
{
  vmPrivate = newVM;
  privateRecycler = vmPrivate->recycler;
  rootPrivate = RootPointer(vmPrivate);
  NOLL.level = level = rootPrivate->vm.Private.level;
  SetShared (false);
}

#define	SAVERESTOREEVENTS \
  STATICEVENTFLAG(staticsCreate) | \
  STATICEVENTFLAG(staticsDestroy) | \
  STATICEVENTFLAG(staticsLoad) | \
  STATICEVENTFLAG(staticsUnload) | \
  STATICEVENTFLAG(staticsSpaceDestroy)

private procedure SaveRestore_Data_Handler (code)
  StaticEvent code;
{
  switch (code) {
   case staticsCreate:
    rootPrivate = RootPointer(vmPrivate);
    NOLL.level = level = rootPrivate->vm.Private.level;
    saveRestoreData->rootPrivate = rootPrivate;
    break;
   case staticsDestroy:
    if (level > 0)
      RstrToLevel (0, true);
    break;
   case staticsLoad:
    rootPrivate = saveRestoreData->rootPrivate;
    NOLL.level = level = rootPrivate->vm.Private.level;
    break;
   case staticsUnload:
    saveRestoreData->rootPrivate = rootPrivate;
    rootPrivate = NIL;	/* For safety */
    break;
   case staticsSpaceDestroy:
    rootPrivate = RootPointer(vmPrivate);
    PerformFinalization(rootPrivate, fr_destroyVM, 0);
    break;
  }
}

public procedure Init_SaveRestore(reason)
  InitReason reason;
{
  Object ob;
  switch (reason) {
   case init:
#if (OS == os_mpw)
    globals = (Globals)os_sureCalloc(sizeof(GlobalsRec),1);
#endif
    RegisterData(
      (PCard8 *)&saveRestoreData, (integer)sizeof(SaveRestore_Data),
      SaveRestore_Data_Handler, (integer)SAVERESTOREEVENTS);
    inRestore = false;	/* Static? */
    LNullObj (NOLL); 	/* Static? */
    break;
   case romreg:
    break;
   case ramreg:
    GC_RegisterFinalizeProc(HandleGCFinalize, (RefAny)NIL);
    break;
   endswitch
  }
}
