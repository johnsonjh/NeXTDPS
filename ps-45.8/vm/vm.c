/*
				    vm.c

   Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE: All  information contained herein is  the property of   Adobe Systems
Incorporated.    Many of  the  intellectual and technical concepts  contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal  use.   Any reproduction
or dissemination of this software is strictly forbidden unless  prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Chuck Geschke: November 23, 1983
Edit History:
Scott Byer: Wed May 17 09:31:48 1989
Chuck Geschke: Sat Mar 29 15:05:06 1986
Doug Brotz: Thu Aug  7 17:33:36 1986
Ed Taft: Tue Nov 28 09:32:34 1989
Bill Paxton: Tue Jan 15 09:59:43 1985
John Gaffney: Mon Nov 11 16:32:57 1985
Peter Hibbard: Wed Dec  4 15:36:39 1985
Bill McCoy: Wed Aug 27 11:22:55 1986
Don Andrews: Wed Sep 17 14:56:13 1986
Ivor Durham: Sun May  7 13:54:03 1989
Joe Pasqua: Mon Jan  9 16:05:45 1989
Perry Caro: Mon Nov  7 14:02:13 1988
Jim Sandman: Thu Apr 13 15:44:40 1989
Paul Rovner: Mon Aug 28 10:01:09 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include COPYRIGHT
#include BASICTYPES
#include ERROR
#include GC
#include ORPHANS
#include VM
#include "abm.h"
#include "gcinternal.h"
#include "saverestore.h"
#include "vmnames.h"

public Switches switches;

/*	----> Garbage Collector Support Routines <----	*/
/* The following procedures are for use with the GC.	*/
/* They look thru the VM data structures for roots and	*/
/* other interesting uses of PS VM.			*/

private procedure ProcessFinalizeNodes(info, space)
/* This routine is responsible for marking the storage	*/
/* used by FinalizeNodes as alloc'd.			*/
GC_Info info;
register VMStructure *space;
{
  PVMRoot localRoot = (space->shared) ? rootShared : rootPrivate;
  register PFinalizeNode node;

  for (	/* FOR EACH node in the finalize chain DO	*/
    node = localRoot->finalizeChain; node != NIL;
    node = node->link)
    {
    GC_MarkAllocated(info, node, sizeof(FinalizeNode));
    }
}

private procedure ProcessSaveData(info)
/* This routine is responsible for marking the storage	*/
/* used by save records as alloc'd, as well as pushing	*/
/* any saved objects it finds on the way.		*/
GC_Info info;
{
  PSR saveRecordPtr;

  for (saveRecordPtr = rootPrivate->vm.Private.srList;
       saveRecordPtr != NIL;
       saveRecordPtr = saveRecordPtr->link)
    {
    GC_MarkAllocated(info, saveRecordPtr, sizeof(SR));
      /* Mark the storage occupied by the save record  */

    /*		----- Generic Bodies -----		*/
    {	/* Open scope for local variables		*/
    register PSRG savedGBPtr;

    for (savedGBPtr = saveRecordPtr->generics;
         savedGBPtr != NIL;
	 savedGBPtr = savedGBPtr->link)
      {
      GC_MarkAllocated(info, savedGBPtr,
      	sizeof(SRG) + savedGBPtr->header.length - sizeof (GenericBody));
      /* NOTE: No need to trace the elements pointed to	*/
      /* by this body. They'll be found (or not) by	*/
      /* other roots in VM. This save record only saves	*/
      /* the contents of the body itself.		*/
      }
    }

    /*		----- Saved Object list -----		*/
    {	/* Open scope for local variables		*/
    register PSRO savedObjectPtr;
  
    for (savedObjectPtr = saveRecordPtr->objs;
         savedObjectPtr != NIL;
	 savedObjectPtr = savedObjectPtr->link)
      {
      GC_MarkAllocated(info, savedObjectPtr, sizeof(SRO));
      GC_Push(info, &(savedObjectPtr->o));
      }
    }

    /*		----- Dictbody's -----			*/
    {	/* Open scope for local variables		*/
    register PSRD savedDBPtr;

    for (savedDBPtr = saveRecordPtr->dbs;
         savedDBPtr != NIL;
	 savedDBPtr = savedDBPtr->link)
      {
      GC_MarkAllocated(info, savedDBPtr, sizeof(SRD));
      GC_MarkAllocated(
        info, savedDBPtr->db.begin, savedDBPtr->db.size * sizeof(KeyVal));
      /* NOTE: No need to trace the elements pointed to	*/
      /* by this dictbody. They'll be found (or not) by	*/
      /* other roots in VM. This save record only saves	*/
      /* the contents of the dictbody itself. We mark	*/
      /* the key val array as allocated just in case we	*/
      /* expand the dict at a higher save level, then	*/
      /* restore it. In that case we want the old key	*/
      /* val array around.				*/
      }
    }
  }
}

private procedure PushSharedVMRoots(clientData, info)
RefAny clientData;
GC_Info info;
{
  GC_Push(info, &(rootShared->vm.Shared.param));
  GC_Push(info, &(rootShared->vm.Shared.regNameArray));
  GC_Push(info, &(rootShared->vm.Shared.regOpNameArray));
  GC_Push(info, &(rootShared->vm.Shared.regOpIDArray));
  GC_Push(info, &(rootShared->trickyDicts));
  ProcessFinalizeNodes(info, vmShared);
  GC_Push(info, &rootShared->nameMap);
  /* handle systemdict specially; its contents must be treated as roots
     even if the DictBody is in ROM */
  GCInternal_TraceROMDict(info, &rootShared->vm.Shared.sysDict);
  GC_Push(info, &rootShared->vm.Shared.sharedDict);
  GC_Push(info, &rootShared->vm.Shared.internalDict);
  GC_Push(info, &rootShared->vm.Shared.sharedFontDirectory);
  {
  PNameArrayBody body = rootShared->vm.Shared.nameTable.val.namearrayval;
  GC_MarkAllocated(info, body, body->header.length);
  }
  GC_MarkAllocated(info, rootShared, sizeof(VMRoot));
}

private procedure PushVMRoots(clientData, info)
/* This procedure is responsible for pushing	*/
/* all private roots held by the vm package.	*/
RefAny clientData;
GC_Info info;
{
  GC_Push(info, &(rootPrivate->trickyDicts));
  ProcessFinalizeNodes(info, vmPrivate);
  /* For private spaces, NameMap is not in VM	*/
  /* so it needn't be marked. It only has names	*/
  /* in it, so only trace it if we're tracing	*/
  /* private VM as part of a shared collection.	*/
  if (GC_GetCollectionType(info) == privateForShared)
  	GC_Push(info, &(rootPrivate->nameMap));
  ProcessSaveData(info);
  GC_MarkAllocated(info, rootPrivate, sizeof(VMRoot));
}

/*	-----------------------------------	*/
/*	----> END GC Support Routines <----	*/
/*	-----------------------------------	*/


/* Implementation of ERROR interface */

public PNameObj errorNames;

/* InvlAccess is defined in exec.c */

public procedure FInvlAccess() {PSError(invlaccess);}
public procedure InvlFont() {PSError(invlfont);}
public procedure PSLimitCheck() {PSError(limitcheck);}
public procedure NoCurrentPoint() {PSError(nocurrentpoint);}
public procedure PSRangeCheck() {PSError(rangecheck);}
public procedure PSTypeCheck() {PSError(typecheck);}
public procedure PSUndefFileName() {PSError(undeffilename);}
public procedure PSUndefined() {PSError(undefined);}
public procedure PSUndefResult() {PSError(undefresult);}
public procedure VMERROR() {PSError(VMerror);}

extern procedure Init_Cmds(), Init_StaticData(), Init_SaveRestore(),
		 Init_VM_Garbage(), Init_Recycler();

public procedure VMInit(reason)
  InitReason reason;
  {
   Init_Cmds (reason);		/* this must be first */
   Init_VM_Space (reason);
   Init_StaticData (reason);	/* Must precede allocation of any statics */
   Init_VM_Memory (reason);	/* Must precede saverestore initialisation */
   Init_SaveRestore (reason);
   Init_VM_Garbage (reason);	/* Must follow saverestore for rootPrivate */
   Init_Recycler (reason);
   ABM_Init (reason);
   GCInternal_Init (reason);

   switch (reason) {
     case romreg:
      errorNames = RgstPackageNames ((integer) PACKAGE_INDEX, (integer) NUM_PACKAGE_NAMES) + nm_dictfull;
      break;
     case ramreg:
      GC_RgstGetRootsProc (PushVMRoots, (RefAny) NIL);
      GC_RgstSharedRootsProc (PushSharedVMRoots, (RefAny) NIL);
      break;
     }
  }				/* end of VMInit */

