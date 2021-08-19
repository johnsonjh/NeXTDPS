/*
  vm_garbage.c

Copyright (c) 1987, '88, '89 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Linda Gass and Andy Shore April 21, 1987
Edit History:
Linda Gass: Thu Jun 11 11:08:50 1987
Ivor Durham: Sun May  7 14:55:02 1989
Jim Sandman: Mon Mar 14 08:40:46 1988
Joe Pasqua: Mon Jan  9 16:09:33 1989
Perry Caro: Mon Nov  7 11:35:37 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include EXCEPT
#include GC

/*
  Garbage Collector Callback Procedures
 */

PVoidProc gcSpaceSemaphore, gcTotalSemaphore;
PCard8 (*gcSetCurrentCtxt)();
GenericID (*gcGetCurrentCtxt)(), (*gcGetNextCtxt)();
PVM	  (*gcGetNextSpace)();

public procedure RgstGCContextProcs (spaceSemaphore, totalSemaphore,
	    getCurrentCtxt, setCurrentCtxt, getNextCtxt, getNextSpace)
  PVoidProc spaceSemaphore, totalSemaphore;
  PCard8 (*setCurrentCtxt)();
  GenericID (*getCurrentCtxt)(), (*getNextCtxt)();
  PVM (*getNextSpace)();
{
  gcSpaceSemaphore = spaceSemaphore;
  gcTotalSemaphore = totalSemaphore;
  gcGetCurrentCtxt = getCurrentCtxt;
  gcSetCurrentCtxt = setCurrentCtxt;
  gcGetNextCtxt = getNextCtxt;
  gcGetNextSpace = getNextSpace;
}

public procedure Ctxt_StopAllSiblings ()
{
  Assert (gcSpaceSemaphore != NIL);
  (*gcSpaceSemaphore) (1);
}

public procedure Ctxt_RestartAllSiblings ()
{
  Assert (gcSpaceSemaphore != NIL);
  (*gcSpaceSemaphore) (-1);
}

public procedure Ctxt_StopAllCtxts ()
{
  Assert (gcTotalSemaphore != NIL);
  (*gcTotalSemaphore) (1);
}

public procedure Ctxt_RestartAllCtxts ()
{
  Assert (gcTotalSemaphore != NIL);
  (*gcTotalSemaphore) (-1);
}

public GenericID Ctxt_GetCurrentCtxt ()
{
  Assert (gcGetCurrentCtxt != NIL);
  return ((*gcGetCurrentCtxt) ());
}

public procedure Ctxt_SetCurrentCtxt (context)
  GenericID context;
 /*
   Acquire pointer to static data block via callback and then load pointers
   to data block.
  */
{
  extern procedure LoadPointers (); /*+?+ shouldn't this be LoadData? PAC */

  Assert (gcSetCurrentCtxt != NIL);
  LoadData ((*gcSetCurrentCtxt) (context));
}

public GenericID Ctxt_GetNextCtxt (space, prev)
  PVM space;
  GenericID prev;
{
  Assert (gcGetNextCtxt != NIL);
  return ((*gcGetNextCtxt) (space, prev));
}

public PVM Ctxt_GetNextSpace (space)
  PVM space;
{
  Assert (gcGetNextSpace != NIL);
  return ((*gcGetNextSpace) (space));
}

public GenericID Ctxt_NIL;	/* NIL value for a Ctxt ID	*/

public procedure Init_VM_Garbage(reason)
  InitReason reason;
{
  switch (reason) {
   case init:
    Ctxt_NIL.stamp = 0;
    break;
  }
}
