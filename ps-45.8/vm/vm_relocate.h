/*
				vm_relocate.h

	     Copyright (c) 1988, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE:  All information contained herein  is  the  property of Adobe Systems
Incorporated.   Many  of the intellectual  and  technical  concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe  licensees for their  internal use.  Any reproduction
or dissemination of this software is strictly forbidden  unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version:
Edit History:
Scott Byer: Wed May 17 09:49:12 1989
Ivor Durham: Sat Oct 15 13:12:29 1988
Ed Taft: Mon Nov 20 18:09:30 1989
End Edit History.
*/

#ifndef	VM_RELOCATE_H
#define	VM_RELOCATE_H

#include ENVIRONMENT
#include PUBLICTYPES

/* Exported Procedures */

extern procedure BuildSegmentTable(/* PCard8 rombase, rambase */);
/* Builds segment table(s) for vmShared. If rombase and rambase are
   both NIL, builds a single table that can be used to relocate
   the VM in place. Otherwise, builds separate host and target tables
   that can be used to relocate the VM to target segments located
   at rombase (for stROM segment) and rambase (for stPermanentRAM).
   In the latter case, rounds all segments' free pointers up to
   vPREFERREDALIGN so that the segments can be concatenated safely.
 */

extern procedure FreeSegmentTable();
/* Frees the segment table(s) created by BuildSegmentTable. */

extern PCard8 EncodeAddress (/* PCard8 address */);
/* Translates real address to (segment, offset) encoded address,
   using the host segment table.
   BuildSegmentTable must have been called previously.
 */

extern boolean ReadRelocationTable(/* Stm vmStm */);
/* Reads the relocation table and sets up relocationTable.
   Returns true iff successful.

   pre: vmStm is positioned at the relocation table in the VM file.
   post: vmStm positioned beyond end of relocation table.
 */

extern procedure ApplyRelocation();
/* Relocates the VM according to relocationTable, which must have
   been set up previously; then frees relocationTable.
   BuildSegmentTable must have been called previously.
 */

/*
 * Relocation information is written to the VM file as a table of
 * address, value pairs.  Each element of the pair is written as
 * a (segment, offset) pair.  The table is preceded by a long int
 * that specifies the number of table entries.
 */

typedef struct _t_SegmentAddress {
  union {
    PCard8 unencoded;
    struct {
#if SWAPBITS
      BitField	offset:24;
      BitField	segment:8;
#else SWAPBITS
      BitField	segment:8;
      BitField	offset:24;
#endif SWAPBITS
    } encoded;
  } sa;
} SegmentAddress;

typedef struct _t_RelocationEntry {
  SegmentAddress	address,	/* Address of pointer */
  			value;		/* Value of pointer */
} RelocationEntry, *PRelocationEntry;

/*
 * Relocated addresses are recorded in a hash table so that they get
 * relocated only once.  If the hash table fills up, it is extended
 * and the search strategy switches to linear because the hash function
 * is no longer valid.
 */

#if (OS == os_vms)
#define HASHSIZE	(48 * 1024)
#else (OS == os_vms)
#define HASHSIZE	(1 << 14)
#endif (OS == os_vms)
#define	HASHMASK	(HASHSIZE - 1)

typedef struct _t_MarkObject {
  PCard8 *markTable;	/* Beginning to address recording table */
  PCard8 *endTable;	/* Entry beyond last one in original table */
  int     tableSize;	/* Current table size */
  int     tableEntries;	/* Number of entries in table */
  int     collisions;	/* Number of times an entry an entry already present */
  boolean hash;		/* True: use hash search, False: use linear search */
} MarkObject;

#endif	VM_RELOCATE_H
