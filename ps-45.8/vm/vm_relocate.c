/*
  vm_relocate.c

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

Original version: 
Edit History:
Scott Byer: Wed May 17 09:45:06 1989
Ivor Durham: Sat Nov  5 11:28:42 1988
Joe Pasqua: Mon Jan  9 16:25:14 1989
Jim Sandman: Tue Mar 15 11:31:00 1988
Ed Taft: Sun Dec 17 15:52:34 1989
Perry Caro: Fri Nov 11 15:54:49 1988
End Edit History.
*/

/*
 * This module is responsible for relocating all VM references after a new
 * VM has been read from file. It is also capable of producing a VM
 * that has already been relocated to fixed addresses for a target machine.
 */

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include EXCEPT
#include PSLIB
#include STREAM
#include VM

#include "saverestore.h"
#include "vm_memory.h"
#include "vm_segment.h"
#include "vm_relocate.h"
#include "vm_reverse.h"

/*
 * Keep pointers to pseudo sections used in address translation as statics
 * to avoid using parameters to RelocateAddress.
 */

typedef struct _t_SegmentTable {
  integer length;		/* number of segments */

  /* Arrays of PCard8 indexed by segment number */
  struct {			/* segment table for this (host) machine */
    PCard8 *first,		/* VMSegment.first */
           *last;		/* VMSegment.last + 1 */
  } host;

  struct {			/* segment table for target machine */
    PCard8 *first;              /* VMSegment.first */
  } target;
} SegmentTable, *PSegmentTable;
 
/*-- BEGIN GLOBALS --*/

private PSegmentTable segmentTable;

public PRelocationEntry relocationTable;	/* Initially NIL */
public int relocationTableSize, relocationTableLimit;

public MarkObject *mark;


/*-- END GLOBALS --*/


public procedure BuildSegmentTable(rombase, rambase)
  PCard8 rombase, rambase;
{
  PVMSegment seg;
  int i;
#if VMINIT
  boolean doTarget = rombase != NIL || rambase != NIL;
#else VMINIT
#define doTarget false
#endif VMINIT

  DebugAssert(segmentTable == NIL);
  DebugAssert (vmShared != NIL);

  for (i = 0, seg = vmShared->head; seg != NIL; seg = seg->succ)
    i++;

  DebugAssert (i > 0);

  segmentTable = (PSegmentTable) os_sureMalloc(
    (long int) sizeof(SegmentTable) +
    ((doTarget)? 3 : 2) * i * sizeof(PCard8));

  segmentTable->length = i;
  segmentTable->host.first = (PCard8 *) (segmentTable + 1);
  segmentTable->host.last = segmentTable->host.first + i;
  segmentTable->target.first = segmentTable->host.first;
  if (doTarget) segmentTable->target.first += 2 * i;

  for (i = 0, seg = vmShared->head;
       i < segmentTable->length;
       i++, seg = seg->succ)
    {
    segmentTable->host.first[i] = seg->first;
    segmentTable->host.last[i] = seg->last + 1;
    if (doTarget)
      {
      seg->free = (PCard8) (((Card32) seg->free + vPREFERREDALIGN - 1)
                            & -vPREFERREDALIGN);
      switch(seg->level)
        {
	case stROM:
	  segmentTable->target.first[i] = rombase;
	  rombase += seg->free - seg->first;
	  break;
	case stPermanentRAM:
	  segmentTable->target.first[i] = rambase;
	  rambase += seg->free - seg->first;
	  break;
	case stVolatileRAM:
	  segmentTable->target.first[i] = seg->first;
	  break;
	default:
	  CantHappen();
	}
      }
    }
}

public procedure FreeSegmentTable()
{
  DebugAssert(segmentTable != NIL);
  os_free((char *) segmentTable);
  segmentTable = NIL;
}

private PCard8 DecodeAddress (segmentAddress)
  PCard8 segmentAddress;
/* Translates (index, offset) encoded address into a real address
   using the target segment table.
 */
{
  SegmentAddress address;

  DebugAssert(segmentTable != NIL);

  if (segmentAddress == NIL) return (NIL);

  address.sa.unencoded = segmentAddress;

  Assert (address.sa.encoded.segment < segmentTable->length);

  return (segmentTable->target.first[address.sa.encoded.segment] + 
	  address.sa.encoded.offset);
}

public PCard8 EncodeAddress (address)
  PCard8 address;
{
  SegmentAddress segmentAddress;
  int i;

  DebugAssert(segmentTable != NIL);

  if (address == NIL) return (NIL);

  for (i = 0; i < segmentTable->length; i++)
    if (segmentTable->host.first[i] <= address &&
        address <= segmentTable->host.last[i])
      {
      segmentAddress.sa.encoded.segment = i;
      segmentAddress.sa.encoded.offset = address - segmentTable->host.first[i];
      return (segmentAddress.sa.unencoded);
      }

  CantHappen ();
}

public boolean ReadRelocationTable (vmStm)
  Stm vmStm;
{
  boolean ok;

  Assert (relocationTable == NIL);

  ok = (fread (&relocationTableSize, 1, sizeof (relocationTableSize), vmStm) 
	== sizeof (relocationTableSize));

  if (ok) {
    int     size = relocationTableSize * sizeof (RelocationEntry);

    relocationTable = (PRelocationEntry) os_malloc ((long int)size);

    if (relocationTable == NIL)
      RAISE (ecLimitCheck, (char *)NIL);

    ok = (fread (relocationTable, 1, size, vmStm) == size);
  }

  return ok;
}

public procedure ApplyRelocation()
{
  register PRelocationEntry relocationEntry;
  int i, index;
  PCard8 *address,
         value;

  Assert(relocationTable != NIL);

  for (i = 0, relocationEntry = relocationTable;
       i < relocationTableSize; i++, relocationEntry++)
    {
    /* decode the address in the host VM */
    index = relocationEntry->address.sa.encoded.segment;
    Assert (index < segmentTable->length);
    address = (PCard8 *) (segmentTable->host.first[index] +
			  relocationEntry->address.sa.encoded.offset);

    /* decode the value in the target VM */
    index = relocationEntry->value.sa.encoded.segment;
    Assert (index < segmentTable->length);
    value = (segmentTable->target.first[index] +
	     relocationEntry->value.sa.encoded.offset);

#if CANREVERSEVM
    if (vSWAPBITS != SWAPBITS) ReverseInteger(&value);
#endif CANREVERSEVM

#if MINALIGN == 1
    *address = value;
#else MINALIGN == 1
    os_bcopy (&value, address, sizeof (PCard8));
#endif MINALIGN == 1
    }

  /* Following fixups are valid only for host, not target */

  rootShared = RootPointer (vmShared);
  vmShared->free = DecodeAddress(vmShared->free);
  ResetVMSection (vmShared, vmShared->free, vmShared->current->level);

  os_free ((char *)relocationTable);
  relocationTable = NIL;
  relocationTableSize = 0;
}
