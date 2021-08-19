/*
  gc.h

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

Original version: Pasqua: Tue Mar  1 14:58:41 1988
Edit History:
Joe Pasqua: Tue Jan 17 13:15:16 1989
Ivor Durham: Wed Jun 29 10:52:11 1988
Ed Taft: Mon Nov 13 17:45:21 1989
End Edit History.
*/

#ifndef	GC_H
#define	GC_H

#include	VM

/*	----- Types -----	*/

typedef	charptr RefAny;	/* This is a generic pointer type	*/

typedef RefAny GC_Info;
  /* This is pointer to an opaque structure whose contents is	*/
  /* known only to the collector. It is passed to callbacks so	*/
  /* they can pass it to various procs in this interface.	*/

typedef enum {privateVM, privateForShared, sharedVM} GC_CollectionType;

/*	----- CONSTANTs -----	*/

/*	----- DATA -----	*/

extern boolean *PrivTraceInfo;
  /* This pointer is exported only because it is needed by a	*/
  /* macro. Never read or change the ptr or its referent.	*/

/*	----- PROCEDURES -----	*/

extern GC_CollectionType GC_GetCollectionType(/* GC_Info info */);
  /* Return the type of collection that is in progress	*/

extern VMStructure *GC_GetSpace(/* GC_Info info */);
  /* Returns the space being collected at this moment	*/

extern PVMRoot GC_GetRoot(/* GC_Info info */);
  /* Returns a pointer to the root structure for the space being collected */

extern procedure GC_Push(/* GC_Info info, PObject value */);
  /* Push a PObject onto the GC stack. Used by the root	*/
  /* collecting procs to return data to the GC.		*/

extern procedure GC_CollectPrivate(/* VMStructure *space */);
  /* Causes the specified private VM to be collected	*/

extern procedure GC_CollectShared();
  /* Causes shared and all private VMs to be collected	*/

extern procedure GC_HandleIndex(
  /* cardinal index, cardinal indexType, GC_Info info */);
  /* Call this procedure to handle a name or cmd index	*/
  /* found while scanning a packed ary body. This proc	*/
  /* is called since there is no corresponding object	*/
  /* to push on the GC's pending stack in this case.	*/
  /* The indexType is 0 for names and 1 for commands.	*/

extern procedure GC_HandleStreamBody(
  /* PStmBody streamBody, GC_Info info */);
  /* Call this procedure to handle a streamBody being	*/
  /* cached in private or shared vm. This procedure	*/
  /* is called since there is no corresponding object	*/
  /* to push on the GC's pending stack in this case.	*/

extern procedure GC_RgstSharedRootsProc(
  /* procedure (*proc)(), RefAny clientData */);
  /* Registers a callback to get a pkgs shared roots.	*/
  /* Each package that contains shared roots should	*/
  /* register such a proc. The proc is defined as:	*/
  /* procedure proc(RefAny clientData, GC_Info info)	*/
  /* For each shared root in the package, proc should	*/
  /* push a ptr to the obj onto the spec'd. stack using	*/
  /* GC_Push(). Upon return all roots should be pushed.	*/

extern procedure GC_RgstGetRootsProc(
  /* procedure (*proc)(), RefAny clientData */);
  /* Registers a procedure to call to get roots from a	*/
  /* package. Each package that contains roots should	*/
  /* register such a proc. The proc is defined as:	*/
  /* procedure proc(RefAny clientData, GC_Info info)	*/
  /* For each root in the package, proc should push a	*/
  /* ptr to the object onto the spec'd. stack using	*/
  /* GC_Push(). Upon return all roots should be pushed.	*/

extern procedure GC_RegisterFinalizeProc(
  /* procedure (*proc)(), RefAny clientData */);
  /* Registers a proc to call when the trace is done,	*/
  /* but before any storage is reclaimed. This allows	*/
  /* a type manager to finalize any objects that were	*/
  /* freed. Used in conjunction with GC_WasCollected().	*/
  /* The type of the registered proc is:		*/
  /* procedure proc(RefAny clientData, GC_Info info);	*/
  
extern procedure GC_RgstPkdAryEnumerator(
  /* Card32 (*proc)() */);
  /* Registers a packed array enumerator with the GC.	*/
  /* Registration must occur before any collections.	*/
  /* The type of the registered proc is:		*/
  /*   Card32 proc(PObject pkdAry, GC_Info info);	*/
  /* When called, the proc should push all composite	*/
  /* objects and return the size in bytes of the body.	*/

extern procedure GC_RgstStmEnumerator(
  /* procedure (*proc)() */);
extern procedure GC_RgstGStateEnumerator(
  /* procedure (*proc)() */);
  /* Registers a Stm or GState enumerator (respectively)*/
  /* with the GC - must occur before any collections.	*/
  /* The type of the registered proc is:		*/
  /*   procedure proc(PObject obj, GC_Info info);	*/
  /* When called, the proc should push all composites.	*/

extern RefAny GC_MoveRecycleRange(
  /* RecycleRange *range, PObject *refs, integer nRefs */);
  /* Move the recycle range described by range into	*/
  /* free pool storage. refs is an ary of refs into the	*/
  /* range. All data in range must be reachable by	*/
  /* tracing from refs. Range must contain only arys &	*/
  /* strings. All internal self-refs are relocated as	*/
  /* are the argument refs. The new loc. of range is	*/
  /* returned. NIL is returned if range wasn't moved.	*/


extern boolean GC_WasCollected(/* PObject objectPtr, GCInfo info */);
  /* Returns a boolean indicating whether the object	*/
  /* was collected. NOTE: This proc returns accurate	*/
  /* results only for composite objects.		*/

extern boolean GC_WasNECollected(/* PNameEntry ne, GCInfo info */);
  /* Similar to GC_WasCollected, but accepts a PNameEntry */

/*	----- Inline Procedures -----	*/

#define GC_MarkAllocated(info, addr, size)	\
	GCInternal_MarkAllocated((info), (RefAny)(addr), (Card32)(size))
  /* The caller is saying that it placed an impure object in PS	*/
  /* VM. Its either not a PS object at all, or it has funny	*/
  /* semantics and shouldn't be traced by the GC. This proc	*/
  /* marks the space in VM as allocated and returns.		*/
extern procedure GCInternal_MarkAllocated();
  /* This proc is declared here only so that it may be used by	*/
  /* the above macro definition. Ignore it otherwise.		*/
	
#define	GC_PrivGCMustTrace(obj)	\
  ((PrivTraceInfo[((obj)->type == escObj) ? (obj)->length : (obj)->type]) \
  ? 1 : 0)
  /* Given an object, returns a boolean saying whether	*/
  /* a private collection would examine this obj. The	*/
  /* test is based solely on the type of the obj, not	*/
  /* this particular instance of the type.		*/

#define	GC_H
#endif	GC_H
