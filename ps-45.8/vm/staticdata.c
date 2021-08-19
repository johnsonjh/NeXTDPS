/*
  staticdata.c

Copyright (c) 1987, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ivor Durham: Mon Nov 23 20:38:22 1987
Edit History:
Ivor Durham: Tue Jan 31 17:13:09 1989
End Edit History.
*/

/*
  This module implements an abstraction for managing the PostScript static
  data that needs to be private to individual PS contexts.  The model is that
  packages register their static data requirements in terms of a pointer
  variable and a size.  A specific static data context is established by
  setting the registered pointers to a particular set of static data records.

  During the PostScript "init" initialisation stage, the pointer variables
  and the size of the record to which they are to refer are registered in
  this module.  A monolithic data block is used for the storage for a
  particular (context's) set of static data records and the pointers refer
  to sub-blocks of the required size.  The creation of a new static data
  context then involves only allocating a single record and copying the
  initial record into it.

  A static data context swap is accomplished by changing a relatively small
  number of pointers instead of wholesale data copying.
 */

#include PACKAGE_SPECS
#include BASICTYPES
#include EXCEPT
#include ORPHANS
#include PSLIB
#include VM

/*
 * Data Registration Data Types
 */

#define	BLUNDERKEY	((integer)&staticData)	/* To check record type */
#define	BLUNDEROVERHEAD	sizeof(integer)

typedef struct _DataRecord {
  PCard8 *Record;
  integer Size;
  procedure (*Procedure)(/* StaticEvent event */);
  integer Mask;
  struct _DataRecord *Next;
} DataRecord;

typedef struct _StaticData {
  DataRecord *Head,	/* First registered record */
         *Tail;		/* Last registered record */
  integer Size;		/* Size of static data block */
  PCard8  CurrentData;	/* Current static data block */
} StaticData;

/*
 * All information maintained by this module hangs off a single pointer,
 * staticData.
 */

StaticData *staticData;

public procedure CallDataProcedures (Code)
  StaticEvent Code;
 /*
  * Invoke registered procedures for which Code is enabled in the mask.
  *
  * pre:  Static data pointers have been loaded.
  * post: Each registered procedure that is enabled for Code has been
  *       invoked once.
  */
{
  DataRecord *Pointer = staticData->Head;
  integer Mask = STATICEVENTFLAG (Code);

  if ((staticData->CurrentData != NIL) || (Code == staticsSpaceDestroy)) {
    while (Pointer != NIL) {
      if ((Pointer->Mask & Mask) != 0)
	(*Pointer->Procedure) (Code);
      Pointer = Pointer->Next;
    }
  }
}

public procedure LoadPointers (To_StaticData)
  PCard8 To_StaticData;
 /*
  * Load registered data pointers with addresses of records in the data
  * block addressed by To_StaticData.
  */
{
  DataRecord *Pointer = staticData->Head;
  integer staticOffset = BLUNDEROVERHEAD;

  while (Pointer != NIL) {
    if (Pointer->Record != NIL) {
      *Pointer->Record = (PCard8)(To_StaticData + staticOffset);
      staticOffset += Pointer->Size;
    }

    Pointer = Pointer->Next;
  }
}

public procedure RegisterData (PointerVariable, Size, Procedure, Mask)
  PCard8 *PointerVariable;	/* Address of pointer to static record */
  integer Size;			/* Required size of static record */
  procedure (*Procedure)();	/* Initialisation procedure */
  integer Mask;			/* Enabling mask for StaticEvents */
 /*
  * Register a static data record pointer along with a procedure to invoke
  * for the various static data events and a mask to indicate which of the
  * events the procedure is interested in.
  */
{
  DataRecord *NewData = (DataRecord *) NEW(1, sizeof(DataRecord));

  Assert ((NewData != NIL) && (staticData != NIL) &&
	  ((Size > 0) == (PointerVariable != NIL)));

  Size = ((Size + 3) / 4) * 4;

  NewData->Record = PointerVariable;
  NewData->Size = Size;
  NewData->Procedure = Procedure;
  NewData->Mask = Mask;
  NewData->Next = NIL;

  if (PointerVariable != NIL)
    *PointerVariable = NIL;	/* Ensure pointer unusable until loaded */

  staticData->Size += Size;	/* Add required space to total needed */

  if (staticData->Head == NIL) {
    staticData->Head = staticData->Tail = NewData;
  } else {
    staticData->Tail->Next = NewData;
    staticData->Tail = NewData;
  }
}

public PCard8 CreateData ()
 /*
  * Generate and load new initial data block and invoke initialisation
  * procedures for which the staticCreate event is enabled.
  *
  * Much the same implementation as LoadData, but does not call the
  * static procedures for staticsLoad before calling them for staticsCreate!
  *
  * pre: No pointers loaded.
  * post: (RESULT == address of new data block && new block loaded) ||
  *	  RESULT == NIL if a new block cannot be allocated.
  */
{
  PCard8  New_Data = NEW (1, staticData->Size);

  if (New_Data != NIL) {
    *(integer *) New_Data = BLUNDERKEY;

    if (staticData->CurrentData != NIL)
      CallDataProcedures (staticsUnload);

    LoadPointers (New_Data);

    staticData->CurrentData = New_Data;

    CallDataProcedures (staticsCreate);
  }
  return (New_Data);
}

public DestroyData ()
 /*
  * Calls the static procedures with the staticsDestroy option and then
  * frees the storage.
  *
  * May want to modify the implementation to keep a pool of data blocks
  * around to minimise context creation cost.
  */
{
  if (staticData->CurrentData != NIL) {
    CallDataProcedures (staticsDestroy);
    FREE (staticData->CurrentData);
    staticData->CurrentData = NIL;
  }
}

public procedure LoadData (To_StaticData)
  PCard8 To_StaticData;
 /*
  * Load static data pointers with addresses of records in the To_StaticData
  * block and invoke the procedures enabled for the staticLoad event.
  *
  * pre:  Data pointers unloaded.
  * post: Data pointers loaded with addresses inside To_StaticData.
  */
{
  Assert (*(integer *)To_StaticData == BLUNDERKEY);

  if (staticData->CurrentData != NIL)
    CallDataProcedures (staticsUnload);

  LoadPointers (To_StaticData);

  staticData->CurrentData = To_StaticData;

  CallDataProcedures (staticsLoad);
}

public PCard8 UnloadData ()
 /*
  * Invokes the static data procedures with staticsUnload.
  *
  * pre:  true
  * post: RESULT == address of unloaded data block or NIL if there was none.
  */
{
  PCard8 From_StaticData = staticData->CurrentData;

  if (staticData->CurrentData != NIL)
    CallDataProcedures (staticsUnload);

  staticData->CurrentData = NIL;

  return (From_StaticData);
}

public procedure Init_StaticData (reason)
  InitReason reason;
 /*
  * During the init stage, the list of registered records is initialised
  * and an initial block of memory allocated for the first batch.  This
  * block may be too large or it may need to grow.  At "romreg" time,
  * the block is trimmed back to the size that is actually used.
  */
{
  switch (reason) {
   case init:
    staticData = (StaticData *) NEW (1, sizeof (StaticData));
    staticData->Head = NIL;
    staticData->Tail = NIL;
    staticData->Size = BLUNDEROVERHEAD;
    staticData->CurrentData = NIL;
    break;
  }
}
