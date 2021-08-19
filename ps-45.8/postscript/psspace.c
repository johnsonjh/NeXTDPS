/*
  psspace.c

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

Original version: Ivor Durham, April 1988.
Edit History:
Ivor Durham: Tue May  9 00:02:47 1989
Ed Taft: Wed Nov 15 13:14:13 1989
Joe Pasqua: Fri Jan 13 15:31:44 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include EXCEPT
#include LANGUAGE
#include VM

public PVM CreatePrivateVM (spaceID)
  GenericID spaceID;
 /*
   Create a new private VM and return a pointer to it or NIL if it fails.
  */
{
  boolean originalShared = CurrentShared ();
  PVM     vmOld = vmPrivate;
  PVM     vmNew = NIL;

  DURING {
    vmNew = CreateVM (false, true);

    LoadVM (vmNew);		/* Set vmPrivate, vmCurrent, etc. */

    /* The root structure is *assumed* to be the first allocation in a VM */

    (void) AllocChars ((integer) sizeof (VMRoot));
    ResetRecycler (privateRecycler);

    rootPrivate->spaceID = spaceID;
    rootPrivate->version = rootShared->version;
    rootPrivate->shared = false;

    rootPrivate->vm.Private.level = 0;
    rootPrivate->vm.Private.srList = NIL;

    /* Initialise trickydicts from shared VM */

    if (vmShared->wholeCloth) {
      rootPrivate->trickyDicts = rootShared->trickyDicts;
    } else {
      AllocPArray (rootShared->trickyDicts.length, &rootPrivate->trickyDicts);
      ResetRecycler (RecyclerForObject (&rootPrivate->trickyDicts));
      VMCopyArray (rootShared->trickyDicts, rootPrivate->trickyDicts);
    }
  } HANDLER {
    if (vmNew != NIL) {
      DestroyVM (vmNew);
      vmNew = NIL;
    }
    if (vmOld != NIL)
      LoadVM (vmOld);

    SetShared (originalShared);	/* Guarantee original state */

    RERAISE;
  } END_HANDLER;

  if (vmOld != NIL)
    LoadVM (vmOld);

  SetShared (originalShared);	/* Guarantee original state */

  return (vmNew);
}

private boolean lastSetSharedforContext;

private procedure RstrSetShared (data, size)
  char *data;
  integer size;
{
  Assert (data == (char *)&lastSetSharedforContext);
  SetShared (lastSetSharedforContext);
}

public procedure PSSetShared ()
 /*
   Switch VMs and then switch FontDirectory with SharedFontDirectory.
   Disabled during VM construction.
  */
{
  PObject pTrickyArray;
  boolean shared = PopBoolean ();

  lastSetSharedforContext = CurrentShared ();

  if (vmCurrent->wholeCloth || (shared == lastSetSharedforContext))
    return;

  SetShared(shared);

  WriteContextParam (
    (char *)&lastSetSharedforContext, (char *)&shared,
    (integer)sizeof(shared), RstrSetShared);
}

public procedure PSCurrentShared ()
{
  PushBoolean (CurrentShared ());
}


public procedure PSSpaceInit (reason)
  InitReason reason;
{
  switch (reason) {
   case romreg:
    break;
  }
}
