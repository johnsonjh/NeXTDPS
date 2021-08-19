/*
  makevm.c

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

Original version: Bill Bilodeau (from postscript.c)
Edit History:
Scott Byer: Wed May 17 09:28:15 1989
Bill Bilodeau	Wed Apr  5 18:05:59 PDT 1989
Ivor Durham: Sun May  7 12:04:47 1989
Ed Taft: Fri Jan  5 08:53:35 1990
Paul Rovner: Mon Aug 28 10:08:44 1989
Ross Thompson: Wed Nov 29 17:59:19 1989
End Edit History.
*/

#include PACKAGE_SPECS

#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include ORPHANS
#include PSLIB
#include SIZES
#include STREAM
#include UNIXSTREAM
#include VM

#include "saverestore.h"
#include "vm_space.h"
#include "vm_memory.h"
#include "vm_reverse.h"
#include "vm_relocate.h"
#include "vm_segment.h"

#ifndef CAN_WRITE_OBJ
#define CAN_WRITE_OBJ (OS==os_sun && STAGE==DEVELOP)
#endif CAN_WRITE_OBJ

#if CAN_WRITE_OBJ
#include "makeobjfile.h"
#endif CAN_WRITE_OBJ


/*-- BEGIN GLOBALS --*/

extern PRelocationEntry relocationTable;	/* Initially NIL */
extern int relocationTableSize, relocationTableLimit;
extern MarkObject *mark;
private procedure (*PackedArrayRelocator)();

/* Imported from language.h */
extern procedure ForceUnDef (), MakePName (), PopPString(), TopP(),
  SetDictAccess(), ResetNameCache();
extern integer PSPopInteger();

/*-- END GLOBALS --*/

/*
 * Forward declarations
 */

private procedure ScanPkdAry();
private procedure ScanFromAry();
private procedure ScanFromDict();


public procedure RgstPackedArrayRelocator (proc)
  procedure (*proc)();
{
  PackedArrayRelocator = proc;
}


public procedure NewRelocationEntry (address)
  PCard8 *address;
 /*
  * Add an entry to the relocation table for the given address and the
  * pointer stored at that address.
  */
{
  PCard8 value;

  DebugAssert (relocationTable != NIL);

#if	MINALIGN == 1
  value = *address;
#else	MINALIGN == 1
  os_bcopy (address, &value, sizeof (PCard8));
#endif	MINALIGN == 1

  if (value == 0)
    return;

  if (relocationTableSize >= relocationTableLimit) {
    /* Double table size */
    relocationTableLimit *= 2;
    relocationTable = (PRelocationEntry)os_realloc(
      (char *)relocationTable,
      (long int)(relocationTableLimit * sizeof(RelocationEntry)));
    Assert(relocationTable != NIL);
  }

  relocationTable[relocationTableSize].address.sa.unencoded =
    EncodeAddress((PCard8)address);
  relocationTable[relocationTableSize].value.sa.unencoded =
    EncodeAddress(value);

  relocationTableSize++;
}

#define	NoteRelocationEntry(x)	NewRelocationEntry ((PCard8 *)&(x))

private procedure Relocate_Shared_Root()
{
  rootShared = RootPointer(vmShared);

  NoteRelocationEntry (rootShared->trickyDicts.val.arrayval);
  NoteRelocationEntry (rootShared->nameMap.val.genericval);
  NoteRelocationEntry (rootShared->finalizeChain);

  NoteRelocationEntry (rootShared->vm.Shared.param.val.arrayval);
  NoteRelocationEntry (rootShared->vm.Shared.regNameArray.val.arrayval);
  NoteRelocationEntry (rootShared->vm.Shared.regOpNameArray.val.arrayval);
  NoteRelocationEntry (rootShared->vm.Shared.regOpIDArray.val.strval);
  NoteRelocationEntry (rootShared->vm.Shared.sysDict.val.dictval);
  NoteRelocationEntry (rootShared->vm.Shared.sharedDict.val.dictval);
  NoteRelocationEntry (rootShared->vm.Shared.internalDict.val.dictval);
  NoteRelocationEntry (rootShared->vm.Shared.sharedFontDirectory.val.dictval);
  NoteRelocationEntry (rootShared->vm.Shared.nameTable.val.genericval);
  NoteRelocationEntry (rootShared->vm.Shared.downloadChain);
}


#define	AddToMarkTable(loc)	AddMark((PCard8)(loc))
private int AddMark(loc)
  PCard8 loc;
 /*
  * Use hash function to find entry in mark table unless table has filled
  * up once, in which case we resort to a linear search because the hash
  * property is lost.
  */
{
  register PCard8 *TableEntry;

  /* See if this address has already been marked  */

  if (mark->hash) {
    register PCard8 *StartEntry;
    register longcardinal Index = (longcardinal) loc;
    boolean collision = false;

    Index *= 0x41c64e6d;		/* Borrowed from old dict.c hash */
    Index = (Index >> 16) & HASHMASK;

    TableEntry = &mark->markTable[Index];
    StartEntry = TableEntry;

    do {
      if (*TableEntry == loc) {
	if (collision)
	  mark->collisions++;
	return (false);
      } else if (*TableEntry == 0)
	break;
      else {
	collision = true;
	TableEntry++;
	if (TableEntry == mark->endTable)
	  TableEntry = &mark->markTable[0];
      }
    } while (TableEntry != StartEntry);

    if (collision)
      mark->collisions++;

   /* Reach here if (a) empty entry found or (b) loc not in full table */
  } else {
    register int i;

    for (i = 0; i < mark->tableEntries; i++)
      if (mark->markTable[i] == loc) {
	return (false);
      }

    TableEntry = &mark->markTable[mark->tableEntries];
  }

  /* Reach here when loc is to be added to the table */

  if (mark->tableEntries == mark->tableSize) {
    mark->tableSize *= 2;	/* Double the table size */
    mark->markTable = (PCard8 *)os_realloc (
      (char *)mark->markTable, (long int)(mark->tableSize * sizeof(PCard8)));
    Assert(mark->markTable != NIL);

    mark->hash = false;		/* Note mark->endTable is now invalid */

    os_printf ("Warning: Relocation continuing with very slow algorithm (%d entries)\n",
	       mark->tableSize);

    TableEntry = &mark->markTable[mark->tableEntries];
  }

  /* Place the new entry in the table and bump # of entries */

  *TableEntry = loc;
  mark->tableEntries++;

  return (true);
}


private procedure Relocate_Names ()
{
  integer i;
  PNameEntry Name;
  PNameEntry *P;

  P = &(rootShared->vm.Shared.nameTable.val.namearrayval)->nmEntry[0];

  for (i = (rootShared->vm.Shared.nameTable.val.namearrayval)->length; --i >= 0; P++) {
    NoteRelocationEntry (*P);
    Name = *P;

    for (; Name != NIL; Name = Name->link) {
      if (!AddToMarkTable (Name)) {
	os_printf ("Name already marked (0x%X)\n", Name);
	CantHappen ();
      }

      /*
       * We set the timestamp to 0 to indicate that the entry is not valid.
       * We must also invalidate the dict entry to make sure its not the same
       * as anything on the new dictstack. This happens for trickyDicts and
       * can also happen for regular dicts if you're extremely unlucky.  
       */

      Name->ts.stamp = 0;
      Name->dict = 0;
      Name->kvloc = 0;

      NoteRelocationEntry (Name->link);
      NoteRelocationEntry (Name->str);
    }
  }
}


private procedure ScanNameArray (NameArray)
  PNameArrayObj NameArray;
  
{
  integer i;
  PNameEntry *P;

  if (NameArray->type == nullObj)
    return;	/* Not allocated yet */

  Assert ((NameArray->type == escObj) && (NameArray->length == objNameArray));

  P = & (NameArray->val.namearrayval)->nmEntry[0];

  for (i = NameArray->val.namearrayval->length; --i >= 0; P++) {
    NoteRelocationEntry (*P);
  }
}


private procedure RelocateObject(obj, doRelocation)
  PObject obj;
  boolean doRelocation;
 /*
  * Update reference to VM in object and then relocate objects contained in
  * the value of the object.
  */
{
  switch (obj->type == escObj ? obj->length : obj->type) {
   case dictObj:
    if (!TrickyDict (obj)) {
      if (doRelocation)
	NoteRelocationEntry (obj->val.dictval);
      ScanFromDict ((PDictObj) obj);
    }
    break;
   case arrayObj:
    if (doRelocation)
      NoteRelocationEntry (obj->val.arrayval);
    ScanFromAry ((PAryObj) obj);
    break;
   case pkdaryObj:
    if (doRelocation)
      NoteRelocationEntry (obj->val.pkdaryval);
    ScanPkdAry ((PPkdaryObj) obj);
    break;
   case strObj:
    if (doRelocation)
      NoteRelocationEntry (obj->val.strval);
    AddToMarkTable (obj->val.strval);
    break;
   case nameObj:
   case cmdObj:
   case objCond:
   case objLock:
    if (doRelocation)
      NoteRelocationEntry (obj->val.genericval);
    break;
   case objNameArray:
    if (doRelocation)
      NoteRelocationEntry (obj->val.namearrayval);
    ScanNameArray ((PNameArrayObj) obj);
    break;
   case nullObj:
   case intObj:
   case realObj:
   case boolObj:
   case fontObj:
   case objMark:
  /* No action */
    break;
   case objSave:
   case objGState:
   case stmObj:
    /* Fall through to CantHappen () */
   default:
    CantHappen ();
  }
}


private procedure ScanPkdAry(ary)
  PPkdaryObj ary;
{
  charptr pa = ary->val.pkdaryval;
  PkdaryObj copy,
          obj;
  Card16  length;
  cardinal i;

  if (!AddToMarkTable ((PCard8) pa))
    return;

  /*
   * If AddToMarkTable returns false, that means the ary is already in the
   * table, so we need trace no further.	 
   */

  copy = *ary;			/* Copy the packed array object	 */
  length = copy.length;		/* Stash the number of elements */

  for (i = 0; i < length; i++) {

    /*
     * PackedArrayRelocator decodes the first element of copy into obj it then
     * updates copy so that it describes the rest of the pkdary. Calling it
     * repeatedly enumerates the array. The returned object is unrelocated
     * although the packed version is. 
     */

    (*PackedArrayRelocator) (&copy, &obj);

    switch (obj.type) {
     case dictObj:
     case arrayObj:
     case pkdaryObj:
      RelocateObject (&obj, false);
      break;
     default:
      break;
    }
  }
}


private procedure ScanFromAry(ary)
  PAryObj ary;
{
  PObject obj = ary->val.arrayval;
  cardinal i;

  /* Every entry is a pointer. Count them */

  for (i = 0; i < ary->length; i++, obj++) {
    if (AddToMarkTable (obj))
      RelocateObject (obj, true);
  }
}


private procedure ScanFromDict(dict)
  PDictObj dict;
{
  PDictBody db;
  cardinal length;
  PKeyVal kv;
  PObject obj;

  Assert(! TrickyDict(dict));

  db = dict->val.dictval;
  length = db->curlength;

  if (!AddToMarkTable ((PCard8) db))
    return;

  /* If AddToMarkTable returns false, that means the dict is	 */
  /* already in the table, so we need trace no further.	 */

  NoteRelocationEntry (db->begin);
  NoteRelocationEntry (db->end);

  if (!AddToMarkTable (db->begin))
    return;

  /*
   * Handle each entry in the dictionary. 
   */

  for (kv = db->begin; kv < db->end; kv++) {
    RelocateObject (&(kv->key), true);
    RelocateObject (&(kv->value), true);
  }
}


private procedure RelocateFinalizeChain(pNode)
  PFinalizeNode pNode;  /* Address of head FinalizeNode */
{
  for ( ; pNode != NIL; pNode = pNode->link) {
    RelocateObject (&pNode->obj, true);
    NoteRelocationEntry (pNode->link);
  }
}


private procedure BuildRelocationTable ()
 /*
   Builds table of (address, value) pairs for relocation, returning pointer
   to the table in output parameter table.  The number of entries in the
   table is returned in the output parameter entries.  The table is allocated
   dynamically.  It is the responsibility of the caller to deallocate the
   table; also to build the segment table beforehand and free it afterward.
  */
{
  int size;

  Assert (PackedArrayRelocator != NIL &&
          mark == NIL &&
	  relocationTable == NIL);

  mark = (MarkObject *) NEW (1, sizeof (MarkObject));

  mark->tableSize = HASHSIZE;
  mark->tableEntries = 0;
  mark->collisions = 0;
  mark->hash = true;

  size = mark->tableSize * sizeof (PCard8);

  mark->markTable = (PCard8 *) os_sureCalloc((long int)1, (long int)size);
  
  if (mark->markTable == NIL) 
    RAISE (ecLimitCheck, (char *)NIL);

  mark->endTable = &mark->markTable[mark->tableSize];

  relocationTableSize = 0;
  relocationTableLimit = 1024;
  relocationTable = (PRelocationEntry)os_sureMalloc(
    (long int)(relocationTableLimit * sizeof(RelocationEntry))); 

  os_printf ("Relocating ... ");
  fflush (os_stdout);

  Relocate_Shared_Root ();

  Relocate_Names ();

  ScanNameArray (&rootShared->nameMap);

  /* Scan from dictionaries in root */

  ScanFromDict (&(rootShared->vm.Shared.sysDict));
  ScanFromDict (&(rootShared->vm.Shared.sharedDict));
  ScanFromDict (&(rootShared->vm.Shared.internalDict));
  ScanFromDict (&(rootShared->vm.Shared.sharedFontDirectory));
  ScanFromAry (&rootShared->trickyDicts);

  /* Handle srLoffset, params, downloadchain */

  ScanFromAry (&(rootShared->vm.Shared.param));
  ScanFromAry (&(rootShared->vm.Shared.regNameArray));
  ScanFromAry (&(rootShared->vm.Shared.regOpNameArray));

  RelocateFinalizeChain (rootShared->finalizeChain);

  os_free((char *)mark->markTable);
  os_free((char *)mark);
  mark = NIL;
}

private boolean WriteRelocationTable (vmStm)
  Stm vmStm;
 /*
   Write a relocation table for the current VM to the given stream.
   Note: reversal of the table is done here if necessary, and the
   table is rendered unusable.
  */
{
  boolean ok;
  integer size;

  Assert (relocationTable != NIL && (size = relocationTableSize) > 0);

#if CANREVERSEVM
  if (vSWAPBITS != SWAPBITS)
    {
    ReverseInteger(&size);
    ReverseRelocationTable(true);
    }
#endif CANREVERSEVM

  ok = (fwrite (&size, 1, sizeof (size), vmStm) == sizeof (size));

  if (ok) {
    size = relocationTableSize * sizeof (RelocationEntry);
    ok = (fwrite (relocationTable, 1, size, vmStm) == size);
  }

  return (ok);
}

private boolean WriteVMSection(vmTarget, vmFile)
  PVM vmTarget;
  Stm vmFile;
 /*
  * Write a VM section to the specified file.  Write the header first
  * and then each of the segments, verbatim.  Translation of addresses
  * is dealt with when the world is read in again.
  */
{
  VMStructure tempVM;
  PVMSegment Segment;
  VMSegment tempVMS;
  boolean ok;
  integer Size;

  ContractVMSection (vmTarget);

  if (vmTarget->tail == vmTarget->reserve) {
    os_printf ("Warning: Emergency reserve segment in use when VM saved!\n");
  }

  vmTarget->current->free = vmTarget->free;

  tempVM = *vmTarget;
  tempVM.free = EncodeAddress (tempVM.free);

#if CANREVERSEVM
  if (vSWAPBITS != SWAPBITS) ReverseStructure(&tempVM);
#endif CANREVERSEVM

  Size = sizeof (VMStructure);
  ok = (fwrite (&tempVM, 1, Size, vmFile) == Size);

  for (Segment = vmTarget->head; Segment != NIL && ok; Segment = Segment->succ) {
    Size = sizeof (VMSegment);
    tempVMS = *Segment;
#if CANREVERSEVM
    if (vSWAPBITS != SWAPBITS) ReverseSegment(&tempVMS);
#endif CANREVERSEVM
    ok = (fwrite (&tempVMS, 1, Size, vmFile) == Size);
    if (ok) {
      Size = Segment->free - Segment->first;
      ok = (fwrite (Segment->first, 1, Size, vmFile) == Size);
    }
  }

  return (ok);
}

private boolean WriteVMToFile(name)
  string name;
 /*
  * pre:	VMPutRoot(root) called and SaveFC(name) called.
  * post:	VM segments are written to file "name".
  */
{
  boolean ok;
  Stm vmFile;

  vmFile = os_fopen ((char *) name, "w");

  if (vmFile == NULL) {
    os_printf ("Cannot open %s for write -- VM not saved!\n", name);
    return false;
  }

  os_printf ("\nSaving VM to file %s ... ", name);
  fflush (os_stdout);

  BuildSegmentTable((PCard8) NIL, (PCard8) NIL);

  BuildRelocationTable ();

#if CANREVERSEVM
  if (vSWAPBITS != SWAPBITS)
    {
    os_printf("Reversing ... ");
    fflush (os_stdout);
    ReverseVM();
    }
#endif CANREVERSEVM

  ok = WriteVMSection (vmShared, vmFile);

  if (ok) {
    ok = WriteRelocationTable (vmFile);
  }

  FreeSegmentTable();

  if (ok) {
    ok = (fwrite (&switches, 1, sizeof (Switches), vmFile) == sizeof (Switches));
  }

  ok = (fclose (vmFile) == 0) && ok;

  if (ok)
    os_printf ("Done.\n");
  else
    os_printf ("Error attempting to write %s -- VM not saved properly!\n", name);
  return ok;
} /* WriteVMToFile */

#if CAN_WRITE_OBJ
private procedure AppendSegmentData(type)
  Level type;  /* one of stROM or stPermanentRAM */
{
  PVMSegment seg;

  for (seg = vmShared->head; seg != NIL; seg = seg->succ)
    if (seg->level == type)
      ObjPutText(seg->first, seg->free - seg->first);
}

private integer AppendSegmentHeader(type, base)
  Level type; PCard8 base;
{
  PVMSegment seg;
  VMSegment tempSegment;
  integer totalSize = 0;

  os_bzero((char *) &tempSegment, (long int) sizeof(VMSegment));
  tempSegment.first = base;
  tempSegment.level = type;
  tempSegment.succ = &tempSegment;  /* anything non-NIL */

  for (seg = vmShared->head; seg != NIL; seg = seg->succ)
    if (seg->level == type)
      totalSize += seg->free - seg->first;

  tempSegment.free = base + totalSize;
  tempSegment.last = tempSegment.free - 1;

#if CANREVERSEVM
  if (vSWAPBITS != SWAPBITS) ReverseSegment(&tempSegment);
#endif CANREVERSEVM

  ObjPutText((char *) &tempSegment, (long int) sizeof(VMSegment));

  return totalSize;
}

private boolean WriteRelocatedVMToFile(name, rombase, rambase)
  string name;
  PCard8 rombase, rambase;
{
  boolean ok;
  integer sizeROM, sizePermanentRAM;
  VMStructure tempStructure;
  VMSegment tempSegment;

  Assert(vVMSPLIT);
  if (rombase == NIL && rambase == NIL)
    os_printf("Warning: setting both ROM and RAM bases to zero disables relocation.\n");

  /* Start building the object file */
  ok = ObjFileBegin(name);
  if (! ok)
    {
    os_printf("Cannot open %s for write -- VM not saved!\n", name);
    return false;
    }

  os_printf ("Saving VM to file %s ... ", name);
  fflush (os_stdout);

  /* Tidy up the VM in preparation for saving it */
  SetDictAccess(rootShared->vm.Shared.sysDict, rAccess);
  ResetNameCache((integer) 1);  /* flush all bindings */

  /* Relocate the VM in-place */
  BuildSegmentTable(rombase, rambase);
  BuildRelocationTable();
  Assert ((relocationTable != NIL) && (relocationTableSize > 0));
  vmShared->free = EncodeAddress(vmShared->free);

#if CANREVERSEVM
  if (vSWAPBITS != SWAPBITS)
    {
    os_printf("Reversing ... ");
    fflush (os_stdout);
    ReverseVM();
    }
#endif CANREVERSEVM

  ApplyRelocation();
  FreeSegmentTable();

  /* Write block of ROM data */
  ObjPutTextSym("_vmROM");
  AppendSegmentData(stROM);

  /* Write block containing VM file that ReadVMFromFile will read */
  /*** Change this to write via a filtered file that compresses the data ***/
  ObjPutTextSym("_vmFileData");
  tempStructure = *vmShared;
#if CANREVERSEVM
  if (vSWAPBITS != SWAPBITS) ReverseStructure(&tempStructure);
#endif CANREVERSEVM
  ObjPutText((char *) &tempStructure, (long int) sizeof(VMStructure));
  sizePermanentRAM = AppendSegmentHeader(stPermanentRAM, rambase);
  AppendSegmentData(stPermanentRAM);
  sizeROM = AppendSegmentHeader(stROM, rombase);
  Assert(vmShared->current->level == stVolatileRAM &&
         vmShared->current->free == vmShared->current->first);
  tempSegment = *vmShared->current;
#if CANREVERSEVM
  if (vSWAPBITS != SWAPBITS) ReverseSegment(&tempSegment);
#endif CANREVERSEVM
  ObjPutText((char *) &tempSegment, (long int) sizeof(VMSegment));
  ObjPutText((char *) &switches, (long int) sizeof(Switches));

  /* Write directive to allocate static storage for permanentRAM segment */
  ObjPutBSSSym("_vmPermanentRAM", sizePermanentRAM);

  os_printf("\nvmROM:          base = 0x%X, size = %D\n",
            (integer) rombase, sizeROM);
  os_printf("vmPermanentRAM: base = 0x%X, size = %D\n",
            (integer) rambase, sizePermanentRAM);

  /* Close out the object file */
  ok = ObjFileEnd() && ok;
  if (ok)
    os_printf ("Done.\n");
  else
    os_printf ("Error attempting to write %s -- VM not saved properly!\n", name);
  return ok;
}
#endif CAN_WRITE_OBJ

private procedure PSMakeVM()
{
  PDictBody db;
  integer n;
  StrObj  s;
  string  name;
  Stm     vmStm;
  NameObj nmObj;
  Object ob;
  boolean relocate, ok;
  PCard8 rombase, rambase;
  integer SharedSize,
          PrivateSize,
          Dummy;

#if CAN_WRITE_OBJ
  TopP(&ob);
  switch (ob.type)
    {
    case strObj:
      relocate = false;
      break;
    case intObj:
      if (! vVMSPLIT) TypeCheck();
      rambase = (PCard8) PSPopInteger();
      rombase = (PCard8) PSPopInteger();
      relocate = true;
      break;
    default:
      TypeCheck();
    }
#endif CAN_WRITE_OBJ

  PopPString (&s);
  name = NEW (1, s.length + 1);
  VMGetText (s, name);

  SetShared (false);

#if VMINIT
  /* If we are building a split VM, ensure that the VM segments are
     in the correct order, which is: stPermanentRAM, stROM, stVolatileRAM */
  VMSetROMAlloc();
  SetVMSegmentType(stVolatileRAM);  /* this may allocate a new segment */
#endif VMINIT

  VM_Usage (vmShared, &Dummy, &SharedSize);
  VM_Usage (vmPrivate, &Dummy, &PrivateSize);
  os_printf ("Shared VM size = %D, Private VM size = %D\n", SharedSize, PrivateSize);

  db = rootShared->vm.Shared.sysDict.val.dictval;
  os_printf ("systemdict used %d entries out of %d\n",
	  db->curlength, db->maxlength);
  db = rootShared->trickyDicts.val.arrayval[tdStatusDict].val.dictval;
  os_printf ("statusdict used %d entries out of %d\n",
	  db->curlength, db->maxlength);
  if (db->maxlength - db->curlength < 10)
    os_printf ("**WARNING** number of unused entries may be insufficient\n");
  db = rootShared->vm.Shared.internalDict.val.dictval;
  os_printf ("internaldict used %d entries out of %d\n",
	  db->curlength, db->maxlength);
  if (db->maxlength - db->curlength < 10)
    os_printf ("**WARNING** number of unused entries may be insufficient\n");
  MakePName("makevm",&nmObj);
  ForceUnDef(rootShared->vm.Shared.sysDict,nmObj);

#if CAN_WRITE_OBJ
  if (relocate) ok = WriteRelocatedVMToFile(name, rombase, rambase);
  else
#endif CAN_WRITE_OBJ
    ok = WriteVMToFile (name);

  os_printf ("Exiting PS\n");
  fflush (os_stdout);
  os_cleanup();
  os_exit ((ok)? 0 : 1);
}				/* end of PSMakeVM */

#if	(STAGE == DEVELOP)
private procedure PSdummymakevm ()
 /*
   Special for MAC II development where creation of VM files is deemed
   too expensive for convenient debugging.
  */
{
  SetShared (false);
  vmShared->wholeCloth = false;
  AllocPArray (rootShared->trickyDicts.length, &rootPrivate->trickyDicts);
  VMCopyArray (rootShared->trickyDicts, rootPrivate->trickyDicts);
}
#endif	(STAGE == DEVELOP)

public procedure InitMakeVM()
{ 
      Begin (rootShared->vm.Shared.sysDict);
      RgstExplicit ("makevm", PSMakeVM);
#if	(STAGE == DEVELOP)
      if (vSTAGE == DEVELOP) RgstExplicit ("dummymakevm", PSdummymakevm);
#endif	(STAGE == DEVELOP)
      End ();
}
