/*
  vm_segment.c

Copyright (c) 1986, '87, '88 Adobe Systems Incorporated.
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
Larry Baer: Mon Nov 13 13:50:35 1989
Ivor Durham: Wed May 10 21:17:10 1989
Ed Taft: Sun Dec 17 18:43:30 1989
Linda Gass: Mon Jun  8 15:56:23 1987
Jim Sandman: Mon Oct 16 11:24:22 1989
Joe Pasqua: Tue Jan 17 13:15:46 1989
Perry Caro: Mon Nov  7 13:09:42 1988
End Edit History.
*/

/*
 * This module implements the  VM section abstraction, which is a sequence of
 * segments.  (See vm_memory.c for how these sequences of segments are used.)
 */

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include ORPHANS
#include SIZES
#include VM
#include RECYCLER
#include "abm.h"
#include "gcinternal.h"
#include "vm_segment.h"

public Pool segmentPool;

#if VMINIT
public boolean vVMSPLIT;
#endif VMINIT

private PVMSegment NewSegment (base, size)
  PCard8 base; integer size;
/* Allocates VMSegment header. If base is NIL, allocates VM data
   block of specified size, and also allocates allocBM.
   If base is non-NIL, it specifies the base of the block;
   allocBM is left NIL in this case.
 */
{
  PVMSegment newSegment = (PVMSegment) os_newelement (segmentPool);

  newSegment->first = newSegment->last = NIL;
  newSegment->allocBM = NIL;

  if (base != NIL)
    newSegment->first = base;
  else
    {
    DURING
      /*
       * TMP - we add 2 to the requested size so that we never end up with the
       * real data space of two segments being adjacent to one another. This
       * should go away sometime.
       */
      newSegment->first = NEW (1, size + 2);
      newSegment->allocBM = NEW (1, ABM_BytesForSize (size));
    HANDLER
      if (newSegment->first != NIL)
        os_free ((char *)newSegment->first);
      os_freeelement (segmentPool, (char *)newSegment);
      RERAISE;
    END_HANDLER;
    }

  newSegment->last = newSegment->first + size - 1;
  return (newSegment);
}

public procedure FreeSegment (vmSegment)
  PVMSegment vmSegment;
{
  Assert (vmSegment != NIL);

  FREE (vmSegment->first);
  if (vmSegment->allocBM != NIL) FREE (vmSegment->allocBM);
  os_freeelement (segmentPool, (char *)vmSegment);
}

#if	(STAGE == DEVELOP)
#include STREAM

private procedure Display_VM_Segment (segment, current)
  PVMSegment segment, current;
{
  os_fprintf (os_stderr, "    [0x%x]%c\t%d left of %d. Level %d.\tPred 0x%x, Succ 0x%x\n\t\tRange [0x%x, 0x%x]\tFree 0x%x\n",
	      segment, ((segment == current) ? '*' : ' '),
	      segment->last - segment->free + 1, VMSegmentSize (segment),
	      segment->level, segment->pred, segment->succ,
	      segment->first, segment->last, segment->free);
}

public procedure DisplayVMSection (vmStructure)
  PVM vmStructure;
{
  PVMSegment segment;

  if (vmStructure == NIL) {
    os_fprintf (os_stderr, "Error: Section pointer is NIL!\n");
  } else if (vmStructure->password != VMpassword) {
    os_fprintf (os_stderr, "Error: Section password (%d) incorrect.\n",
	     vmStructure->password);
  } else {
    vmStructure->current->free = vmStructure->free;

    os_fprintf (os_stderr, "Section 0x%x:\tExpansion size %d\n",
	     vmStructure, vmStructure->expansion_size);

    os_fprintf (os_stderr, "  Reserve (%s):\n", (vmStructure->current == vmStructure->reserve ? "in use" : "available"));
    Display_VM_Segment (vmStructure->reserve, vmStructure->current);

    os_fprintf (os_stderr, "  Segments:\n");

    segment = vmStructure->head;

    while (segment != NIL) {
      Display_VM_Segment (segment, vmStructure->current);
      segment = segment->succ;
    }
  }
}
#endif	(STAGE == DEVELOP)

public PVMSegment FindVMSegment (vmStructure, o)
  PVM vmStructure;
  PCard8 o;
 /*
  * Search the given VM section for a segment containing the given pointer.
  *
  * NOTE: Allow address to be at most one byte beyond the last byte in the
  *	  data part of the segment to allow for finding the segment given
  *	  the free pointer when the segment is full and the free pointer is
  *	  just one byte beyond the end of the data (in the bitmap data).
  *
  * BEWARE: The hack for allowing the pointer to be one beyond the end of
  *	    the data becomes invalid if and when we move the bitmap out of
  *	    the segment proper.  One possible change to avoid this glitch
  *	    is to choose a different value for the free pointer (NIL) when
  *	    the segment is full.  However, this impacts the scanner which
  *	    looks at it.  So be careful!
  */
{
  PVMSegment segment;

  for (segment = vmStructure->tail; segment != NIL; segment = segment->pred) {
    if ((segment->first <= o) && (o <= (segment->last + 1)))
      break;
  }

  return (segment);
}

public boolean InVMSection (vmStructure, o)
  PVM vmStructure;
  PCard8 o;
 /*
  * post: RESULT = there is a segment in the section containing the offset.
  */
{
  return (FindVMSegment (vmStructure, o) != NIL);
}

private PCard8 FindFreeBytes (nBytes, Alignment, Allocate)
  integer nBytes;	/* Number of bytes to allocate */
  integer Alignment;	/* Boundary for allocation (1, 2, 4) */
  boolean Allocate;	/* true to take free bytes, false just to find them */
 /*
  * Look for nBytes of free space in the current VM section, looking in
  * all segments that are available at the current save level before
  * extending the section with a new segment.  The bytes are actually
  * allocated only of the Allocate parameter is true.
  *
  * pre:	vmCurrent is valid, nBytes > 0, and Alignment in {1, 2, 4}
  * post:	RESULT = address of (pre)allocated bytes
  * error:	An invocation of ExpandVMSection may cause an exception to
  *		be propagated.  (VMERROR)
  */
{
  PCard8 RESULT = (PCard8)(((Card32)vmCurrent->free + Alignment - 1) & -Alignment);

  DebugAssert ((nBytes > 0) &&
	       ((Alignment == 1) || (Alignment == 2) || (Alignment == 4)));

  while ((RESULT + nBytes - 1) > vmCurrent->last) {
    PVMSegment vmSegment = vmCurrent->current->pred;
    Level   CurrentLevel = vmCurrent->current->level;
    integer newSize = vmCurrent->expansion_size;

    /* Search previously allocated segments */

    while (vmSegment != NIL) {
      if (vmSegment->level == CurrentLevel) {
	RESULT = (PCard8)(((Card32)vmSegment->free + Alignment - 1) & -Alignment);

	if ((RESULT + nBytes - 1) <= vmSegment->last) {
	  if (Allocate) {
	    vmSegment->free = RESULT + nBytes;
	  } else
	    vmSegment->free = RESULT;

	  return (RESULT);
	} else
	  vmSegment = vmSegment->pred;
      } else
	break;			/* Abandon search for existing segment */
    }

    /* Give up and allocate a new segment */

    if (newSize < nBytes) {
      integer align = vPREFERREDALIGN;
      newSize = (nBytes + align - 1) & (-align);
    }

    ExpandVMSection(vmCurrent, (PCard8) NIL, newSize);
      /* Will exit via exception if it fails */

    RESULT = (PCard8)(((Card32)vmCurrent->free + Alignment - 1) & -Alignment);
  }

  /* Arrive here only if allocating from the current segment */

  if (Allocate) {
    vmCurrent->free = RESULT + nBytes;
    ExtendRecycler (vmCurrent->recycler, vmCurrent->free);
  } else
    vmCurrent->free = RESULT;

  return (RESULT);
}

public PCard8 PreallocChars (nBytes)
  integer nBytes;
 /*
  * Find the nBytes of free space in the current VM section that would be
  * allocated if AllocChars were invoked immediately upon return from this
  * operation.
  */
{
  vmCurrent->current->free = vmCurrent->free;
  return (FindFreeBytes (nBytes, (integer)1, false));
}

public procedure ClaimPreallocChars (vmAddress, nBytes)
  PCard8 vmAddress;
  integer nBytes;
 /*
  * pre:  vmAddress = PreallocChars (n), n >= nBytes with no intervening
  *	  allocations.
  * post: Allocates the bytes from the segment identified by vmAddress.
  */
{
  PVMSegment segment;

  segment = FindVMSegment (vmCurrent, vmAddress);

  Assert (segment != NIL &&
  	  (segment->free == vmAddress) &&
	  ((vmAddress + nBytes - 1) <= segment->last));

  segment->free += nBytes;

  if (segment == vmCurrent->current) {
    vmCurrent->free = segment->free;
    ExtendRecycler (vmCurrent->recycler, vmCurrent->free);
  }
}

public PCard8 AllocChars (nBytes)
  integer nBytes;
 /*
  * Allocate nBytes of VM from the current section.
  */
{
  PCard8 RESULT = FindFreeBytes (nBytes, (integer)1, true);
  return (RESULT);	/* Separate line for debugging */
}

public PCard8 AllocAligned(nBytes)
  integer nBytes;
/*
 * pre:  vmCurrent is valid && (vPREFERREDALIGN == 2 || vPREFERREDALIGN == 4)
 *
 * post: result is address of allocated sequence of nBytes bytes on a
 *	 vPREFERREDALIGN byte boundary.
 */
{
  PCard8 RESULT = FindFreeBytes (nBytes, (integer)vPREFERREDALIGN, true);
  return (RESULT);	/* Separate line for debugging */
}


public PCard8 AllocVMAligned (nBytes, shared)
  integer nBytes;
  boolean shared;
{
  boolean originalShared = vmCurrent->shared;
  PCard8 RESULT;

  SetShared (shared);

  DURING {
    RESULT = AllocAligned (nBytes);
  } HANDLER {
    SetShared (originalShared);
    RERAISE;
  } END_HANDLER;

  SetShared (originalShared);

  return (RESULT);
}

private procedure Use_Reserve (vmStructure)
  PVM vmStructure;
 /*
  * Append the reserve segment for the section to the sequence of segments.
  */
{
  Assert ((vmStructure->reserve != NIL) &&
          (vmStructure->tail != NIL) &&
	  (vmStructure->tail != vmStructure->reserve));

  vmStructure->reserve->pred = vmStructure->tail;
  vmStructure->tail->succ = vmStructure->reserve;
  vmStructure->tail = vmStructure->reserve;

  ResetVMSection(vmStructure, vmStructure->reserve->free, vmStructure->current->level);
}

private procedure Reclaim_Reserve (vmStructure)
  PVM vmStructure;
 /*
  * Unlink reserve segment from section if it is in use.
  *
  * pre: true
  * post: vmStructure'->current != vmStructure'->reserve =>
  *		vmStructure-reserve is empty and not in the segment sequence
  */
{
  if ((vmStructure->current != NIL) &&
      (vmStructure->tail == vmStructure->reserve) &&
      (vmStructure->current != vmStructure->tail)) {
    vmStructure->tail = vmStructure->tail->pred;
    vmStructure->tail->succ = NIL;
    vmStructure->reserve->pred = NIL;
    vmStructure->reserve->free = vmStructure->reserve->first;
  }
}

public procedure ResetVMSection (vmStructure, newFree, newLevel)
  PVM vmStructure;
  PCard8 newFree;
  Level newLevel;
 /*
  * Reset the free pointer in the given VM section.
  */
{
  PVMSegment newCurrent;

  newCurrent = FindVMSegment (vmStructure, newFree);

  Assert (newCurrent != NIL);

  if (newCurrent != vmStructure->current) {
    if (vmStructure->current != NIL)
      vmStructure->current->free = vmStructure->free;

    vmStructure->current = newCurrent;
    vmStructure->last = newCurrent->last;

    Reclaim_Reserve (vmStructure);

    /* Discard all but one spare segment */

    if (vmStructure->current->succ != NIL) {
      vmStructure->current = vmStructure->current->succ;
      vmStructure->current->free = vmStructure->current->first;
      ContractVMSection (vmStructure);
      vmStructure->current = vmStructure->current->pred;
    }
  }

  vmStructure->free = vmStructure->current->free = newFree;
  vmStructure->current->level = newLevel;

  if (newFree < vmStructure->current->firstStackByte)
    vmStructure->current->firstStackByte = newFree;

  InitRecycler (vmStructure->recycler, vmStructure->free);
}

public procedure ContractVMSection(vmStructure)
  PVM vmStructure;
 /*
  * Remove all segments above current segment
  */
{
  PVMSegment discardedSegment;

  Reclaim_Reserve (vmStructure);

  while (vmStructure->tail != vmStructure->current) {
    discardedSegment = vmStructure->tail;

    Assert (discardedSegment != NIL);

    vmStructure->tail = vmStructure->tail->pred;
    vmStructure->tail->succ = NIL;

    FreeSegment (discardedSegment);
  }
}

public procedure ExpandVMSection (vmStructure, base, newSize)
  PVM vmStructure; PCard8 base; integer newSize;
 /*
  * Move the current allocation to a new segment of VM appended to the
  * "current" segment.
  *
  * post: New current segment is successor to previous current segment and
  *       free space in previous current segment is copied into new current
  *	  segment.
  *
  * Caveat: Once the reserve segment is in use, it is not possible to expand
  *	    the section again until the reserve segment can be reclaimed.
  */
{
  PVMSegment newSegment = NIL,	/* Pointer to new segment in section */
         discardedSegment = NIL;/* Pointer to segment to be discarded */
  integer copySize = 0,		/* Default for first segment in section */
	  tempSize;
  Level	  CurrentMaxLevel = 0;	/* Highest save level in current segment */

  if (vmStructure->current != NIL) {
    Assert (vmStructure->current != vmStructure->reserve);

    CurrentMaxLevel = vmStructure->current->level;

    /*
     * Compute expansion size.  Must be at least 150% of the free space in
     * the current segment or the required expansion size, whichever is
     * greater. 
     */

    copySize = (vmStructure->free <= vmStructure->last) ? vmStructure->last - vmStructure->free + 1 : 0;

    if (copySize > 0) {
      tempSize = (copySize * 3) / 2;

      if (tempSize > newSize)
	newSize = tempSize;
    }

    /* Round expansion to multiple of original default expansion size */

    tempSize = ps_getsize (SIZE_VM_EXPANSION, DvmExpansion);
    newSize = ((newSize + tempSize - 1) / tempSize) * tempSize;

    if (vmStructure->free == vmStructure->current->first) {

      /*
       * "Empty" segment ... may be scanner with a large string. Discard
       * other available segments and unlink current segment. Save handle on
       * current segment for later copying. 
       */

      ContractVMSection (vmStructure);

      Assert (vmStructure->tail != NIL);

      discardedSegment = vmStructure->tail;
      vmStructure->tail = vmStructure->tail->pred;
      vmStructure->tail->succ = NIL;
      /* vmStructure->current is invalid at this point */
    } else if (vmStructure->current->succ != NIL) {
      if (VMSegmentSize (vmStructure->current->succ) >= newSize) {
	newSegment = vmStructure->current->succ;
      } else
	ContractVMSection (vmStructure);
    }
  }

  if (newSegment == NIL) {
    DURING
      newSegment = NewSegment (base, newSize);
    HANDLER {
      Assert (vmStructure->reserve != NIL);
      Use_Reserve (vmStructure);/* Append reserve segment */
      VMERROR();		/* Now we have memory to report failure */
    } END_HANDLER;

    Assert (newSegment != NIL);

    newSegment->succ = NIL;
    newSegment->pred = vmStructure->tail;
    newSegment->firstFreeInABM = newSegment->allocBM;
    newSegment->firstStackByte = newSegment->first;

    if (vmStructure->head == NIL) {
      vmStructure->head = newSegment;
    } else {
      vmStructure->tail->succ = newSegment;
    }

    vmStructure->tail = newSegment;
  }

  /*
   * Copy free space from current segment to new segment because this may be
   * a call from the scanner which has written into the free space without
   * actually allocating anything.   
   */

  if (copySize > 0) os_bcopy(
    (char *)vmStructure->free, (char *)newSegment->first, (long int)copySize);

  /* Finish setting up new segment */

  ResetVMSection (vmStructure, newSegment->first, CurrentMaxLevel);

  /* Clean up */

  if (discardedSegment != NIL) {
    FreeSegment (discardedSegment);
  }
}

#if VMINIT
public procedure SetVMSegmentType(type)
  Level type;
{
  register PVMSegment seg, ssHead, ssTail;

  if (! vVMSPLIT) return;

  if ((seg = vmShared->current) != NIL)
    {
    DebugAssert(seg->level == vmShared->tail->level);
    if (seg->level == type) return;  /* nothing to do */
    seg->free = vmShared->free;
    }

  /* Find subsequence of segments of specified type */
  ssHead = NIL;
  ssTail = NIL;
  for (seg = vmShared->head; seg != NIL; seg = seg->succ)
    if (seg->level == type)
      {if (ssHead == NIL) ssHead = seg;}
    else
      {if (ssHead != NIL) {ssTail = seg->pred; break;}}

  if (ssHead == NIL)
    { /* No segment of that type; create one */
    vmShared->current = NIL;
    ExpandVMSection(vmShared, (PCard8) NIL, vmShared->expansion_size);
    vmShared->current->level = type;
    return;
    }

  /* Subsequence does not include tail (else it would have been current) */
  DebugAssert(ssTail != NIL && ssTail->succ != NIL &&
              ssTail != vmShared->tail && vmShared->tail != NIL);

  /* Unlink subsequence from chain */
  if (ssHead->pred == NIL) vmShared->head = ssTail->succ;
  else ssHead->pred->succ = ssTail->succ;
  ssTail->succ->pred = ssHead->pred;

  /* Relink subsequence at end of chain */
  ssHead->pred = vmShared->tail;
  vmShared->tail->succ = ssHead;
  ssTail->succ = NIL;
  vmShared->tail = ssTail;

  /* Set current position to last non-empty segment */
  for (seg = ssTail; seg != ssHead; seg = seg->pred)
    if (seg->free != seg->first) break;
  ResetVMSection(vmShared, seg->free, seg->level);
}

public Level CurrentVMSegmentType()
{
  return (vmShared->current != NIL)? vmShared->current->level :
         (vVMSPLIT)? stPermanentRAM : stVolatileRAM;
}

public PVMRoot RootPointer(vm)
  PVM vm;
{
  PVMSegment seg = vm->head;
  if (vm->shared && vVMSPLIT)
    /* find first permanentRAM segment (there must be one) */
    while (seg->level != stPermanentRAM)
      if ((seg = seg->succ) == NIL) CantHappen();
  return (PVMRoot)(seg->first);
}
#endif VMINIT

public PVM  CreateVM (Shared, createSegment)
  boolean Shared, createSegment;
 /*
  * (see description in vm.h)
  *
  * If createSegment is false, no initial segment is created,
  * because the VM will be constructed by expanding it
  * in exact size segments read from the VM stream.
  *
  * If Shared, set the storage type (level field) of the current segment
  * to the appropriate value, which is stPermanentRAM if vVMSPLIT is true,
  * stVolatileRAM otherwise.
  */
{
  PVM     vmNew;
  extern PRecycler NewRecycler ();

  vmNew = (PVM) NEW (1, sizeof (VMStructure));

  vmNew->password = VMpassword;
  vmNew->shared = Shared;
  vmNew->valueForSeen = 1;
  vmNew->expansion_size = ps_getsize (SIZE_VM_EXPANSION, DvmExpansion);

  DURING
    vmNew->priv = (PVMPrivateData) NEW (1, sizeof (VMPrivateData));
    vmNew->recycler = NewRecycler ();

    if (Shared)		/* BEWARE: This is a side-effect */
      sharedRecycler = vmNew->recycler;

    /* Create reserve segment as ordinary segment, then unhook it. */

    ExpandVMSection (vmNew, (PCard8) NIL, DvmReserve);

    vmNew->reserve = vmNew->head;
    vmNew->head = NIL;
    vmNew->tail = NIL;
    vmNew->current = NIL;
    if (Shared) vmNew->reserve->level = stVolatileRAM;

    /* Create first segment unless requested not to */

    if (createSegment)
      {
#if VMINIT
      if (Shared)
        ExpandVMSection (vmNew, (PCard8) NIL,
	                 NumCArg('o', 10, vmNew->expansion_size));
      else
#endif VMINIT
        ExpandVMSection (vmNew, (PCard8) NIL, vmNew->expansion_size);
      DebugAssert (vmNew->current != NIL && vmNew->current == vmNew->head);
#if VMINIT
      if (Shared)
        if (vVMSPLIT)
	  {
	  /* We just created the ROM segment. Also create the
	     permanentRAM segment and make it current */
	  vmNew->current->level = stROM;
	  vmNew->current = NIL;
	  ExpandVMSection(vmNew, (PCard8) NIL,
	                  NumCArg('a', 10, vmNew->expansion_size));
	  vmNew->current->level = stPermanentRAM;
	  }
	else vmNew->current->level = stVolatileRAM;
#endif VMINIT
      }
  HANDLER
    if (vmNew->priv != NIL) FREE (vmNew->priv);
    if (vmNew->recycler != NIL)	FREE (vmNew->recycler);
    vmNew->current = NIL;  /* force contraction to empty */
    ContractVMSection (vmNew);
    if (vmNew->reserve != NIL) FreeSegment (vmNew->reserve);
    DebugAssert(vmNew->head == NIL);
    FREE (vmNew);
    RERAISE;			/* Continue error report */
  END_HANDLER;

  vmNew->wholeCloth = (Shared && createSegment);

  GCInternal_VMChange (vmNew, creating);/* Inform collector of a new VM */

  return (vmNew);
}

public procedure DestroyVM (vmOld)
  PVM vmOld;
 /*
  * Deallocate entire VM section.
  *
  * pre:  vmOld points to a valid VM section.
  * post: vmOld is invalid.
  */
{
  if (vmOld != NIL) {
    PVM oldPrivate = vmPrivate;
    LoadVM (vmOld); /* This proc in saverestore.c! Fix when vm/root merge */
    GCInternal_VMChange(vmOld, deleting);
      /* Give the collector time to clean up its data	*/
    CallDataProcedures (staticsSpaceDestroy);

    if (oldPrivate != vmOld  && oldPrivate != NIL)
      LoadVM (oldPrivate);
    else {
      vmPrivate = NIL;
      rootPrivate = NIL;	/* This is saverestores domain, but ... */
    }

    vmOld->current = vmOld->head;
    ContractVMSection (vmOld);

    FreeSegment(vmOld->head);
    FreeSegment(vmOld->reserve);

    FREE(vmOld->priv);
    FREE(vmOld->recycler);
    FREE(vmOld);
  }
}

/* zeroBits[i] == number of 0's in binary representation of (unsigned char)i */
private readonly Card8 zeroBits[256] = {
  8, 7, 7, 6, 7, 6, 6, 5,	/*   0..  7	*/
  7, 6, 6, 5, 6, 5, 5, 4,	/*   8.. 15	*/
  7, 6, 6, 5, 6, 5, 5, 4,	/*  16.. 23	*/
  6, 5, 5, 4, 5, 4, 4, 3,	/*  24.. 31	*/
  7, 6, 6, 5, 6, 5, 5, 4,	/*  32.. 39	*/
  6, 5, 5, 4, 5, 4, 4, 3,	/*  40.. 47	*/
  6, 5, 5, 4, 5, 4, 4, 3,	/*  48.. 55	*/
  5, 4, 4, 3, 4, 3, 3, 2,	/*  56.. 63	*/
  7, 6, 6, 5, 6, 5, 5, 4,	/*  64.. 71	*/
  6, 5, 5, 4, 5, 4, 4, 3,	/*  72.. 79	*/
  6, 5, 5, 4, 5, 4, 4, 3,	/*  80.. 87	*/
  5, 4, 4, 3, 4, 3, 3, 2,	/*  88.. 95	*/
  6, 5, 5, 4, 5, 4, 4, 3,	/*  96..103	*/
  5, 4, 4, 3, 4, 3, 3, 2,	/* 104..111	*/
  5, 4, 4, 3, 4, 3, 3, 2,	/* 112..119	*/
  4, 3, 3, 2, 3, 2, 2, 1,	/* 120..127	*/
  7, 6, 6, 5, 6, 5, 5, 4,	/* 128..135	*/
  6, 5, 5, 4, 5, 4, 4, 3,	/* 136..143	*/
  6, 5, 5, 4, 5, 4, 4, 3,	/* 144..151	*/
  5, 4, 4, 3, 4, 3, 3, 2,	/* 152..159	*/
  6, 5, 5, 4, 5, 4, 4, 3,	/* 160..167	*/
  5, 4, 4, 3, 4, 3, 3, 2,	/* 168..175	*/
  5, 4, 4, 3, 4, 3, 3, 2,	/* 176..183	*/
  4, 3, 3, 2, 3, 2, 2, 1,	/* 184..191	*/
  6, 5, 5, 4, 5, 4, 4, 3,	/* 192..199	*/
  5, 4, 4, 3, 4, 3, 3, 2,	/* 200..207	*/
  5, 4, 4, 3, 4, 3, 3, 2,	/* 208..215	*/
  4, 3, 3, 2, 3, 2, 2, 1,	/* 216..223	*/
  5, 4, 4, 3, 4, 3, 3, 2,	/* 224..231	*/
  4, 3, 3, 2, 3, 2, 2, 1,	/* 232..239	*/
  4, 3, 3, 2, 3, 2, 2, 1,	/* 240..247	*/
  3, 2, 2, 1, 2, 1, 1, 0	/* 248..255	*/
  };

/* return the number of bytes available behind the segment's firstStackByte. */
private Int32 FreeBytesBehindStack(seg)
  PVMSegment seg;
{
  register PCard8 curByte;
  register PCard8 stackByte;  /* byte in abm that seg->firstStackByte maps to */
  register Int32  zeroCount;  /* cumulative sum of 0 bits found in abm */

  curByte = seg->allocBM;     /* -> first byte in segment allocation bitmap */

  /* the last byte we want to examine in the allocation bitmap is the
   * predecessor of the byte that contains the bit for the firstStackByte
   * pointer
   */
  stackByte = curByte + (seg->firstStackByte - seg->first)/(BitsPerByte*BitSpan);
  DebugAssert(   curByte   <= stackByte
              && stackByte <= (curByte + ABM_BytesForBitmap(seg) - 1));

  zeroCount = 0;
  while (curByte < stackByte)
    zeroCount += zeroBits[*curByte++];

  return (zeroCount*BitSpan);
}

public procedure VM_Usage (vmStructure, Used, Size)
 PVM vmStructure;
 Int32 *Used,
       *Size;
 /*
  * Count bytes used in the given vmStructure and return it in variable Used.
  * Return space left in current segment in Free.
  */
{
  register PVMSegment segment = vmStructure->head;
  Int32   U = 0,
          S = 0;

  vmStructure->current->free = vmStructure->free;

  while (true) {
    S += segment->last - segment->first + 1;

    /* First add to U the amount of space behind the free pointer */
    U += segment->free - segment->first;		/* used */

    /* Now subtract from U that amount of free space behind */
    /* the free pointer that is available to be allocated   */
    U -= FreeBytesBehindStack(segment);

    if (segment == vmStructure->current)
      break;

    segment = segment->succ;
  }

  *Used = U;
  *Size = S;
}

public procedure CreateSegmentPool ()
{
  segmentPool = os_newpool (sizeof(VMSegment), 8, 0);
}
