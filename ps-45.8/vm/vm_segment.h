/*
  vm_segment.h

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
Ivor Durham: Sat Oct 15 10:50:35 1988
Joe Pasqua: Mon Jan  9 16:13:59 1989
Ed Taft: Fri Nov 24 10:29:10 1989
End Edit History.
*/

/*
 * A VM segment is a region of memory within which PostScript objects may be
 * allocated.
 */

#ifndef	VM_SEGMENT_H
#define	VM_SEGMENT_H

#include BASICTYPES
#include ENVIRONMENT
#include PSLIB
#include VM

/* Data Types */

#define VMpassword 91239
#define	DvmExpansion ((1024 * 8) - 10)
  /* We subtract 10 to leave malloc some room for double links	*/
  /* and two bytes for us until we learn why we need to make	*/
  /* sure that segments aren't adjacent to one another.		*/

/* Defaults */

typedef struct _t_VMSegment {
  struct _t_VMSegment *succ,	/* Next segment in sequence */
         *pred;			/* Preceding segment in sequence */
  PCard8  first,		/* First address in segment (actual handle) */
          last,			/* Last address in segment */
	  free;			/* Next free address in non-current segment */
  Level	  level;		/* Private: max save level for contents;
				   Shared: storage type (stXXX) */
  Card8	  Pad[3];		/* Pad to multiple of four bytes */
  PCard8  allocBM;		/* Pointer to allocation bitmap;
				   NIL => segment is permanent VM */
  PCard8  firstFreeInABM;	/* Ptr to 1st byte in abm that has free	*/
  				/* bits. This is only a hint!		*/
  PCard8  firstStackByte;	/* Ptr to 1st byte of stack area in seg	*/
} VMSegment;

#define	DvmReserve 1024

/* Segment storage types in VMSegment.level field. Values are distinct
   from valid save levels to permit identifying them without checking
   whether segment is private or shared.
 */
#define stROM (MAXlevel+1)	/* not traced, no allocation bitmap */
#define stPermanentRAM (MAXlevel+2) /* traced, no allocation bitmap */
#define stVolatileRAM (MAXlevel+3) /* traced, has allocation bitmap */

/* Exported Procedures */

extern	PCard8		AllocVMAligned(/* integer nBytes, boolean shared */);
extern	procedure	ContractVMSection(/* PVM vmStructure */);
extern	procedure	CreateSegmentPool ();

extern procedure ExpandVMSection(
  /* PVM vmStructure, PCard8 base, integer size */);
/* Attempts to expand the specified vmStructure by appending another
   segment of the specified size. If base is not NIL then it specifies
   the base of the storage to be used for the segment. Otherwise,
   new storage is allocated dynamically; if that fails, the reserve
   segment is used and a VMerror is raised. Sets vmStructure->free
   to the beginning of the new segment.
 */

extern	PVMSegment	FindVMSegment(/* PVM vmStructure, PCard8 o */);
extern	procedure	FreeSegment(/* PVMSegment */);
extern	boolean		InVMSection(/* PVM vmStructure, PCard8 o */);
extern	procedure	ResetVMSection(/* PVM, PCard8, Level */);
extern	procedure	VM_Usage(/* PVM, Int32, Int32 */);
extern	procedure	DisplayVMSection(/* PVM */);

#if VMINIT
extern procedure SetVMSegmentType(/* Level type */);
/* Switches shared VM allocation to the specified type of segment
   (one of stROM, stPermanentRAM, stVolatileRAM), creating a new
   segment of that type if necessary. Note: this procedure is a no-op
   if vVMSPLIT is false.
 */

extern Level CurrentVMSegmentType();
/* Returns the current segment type for shared VM allocation */

extern PVMRoot RootPointer(/* PVM vm */);
#else VMINIT
#define	RootPointer(vm) (PVMRoot)(vm->head->first)
#endif VMINIT

/* Inline Procedures */

#define VMSegmentSize(v)	((v)->last - (v)->first + 1)

#define	NoteLevel(v, n)		((v)->current->level = (n))

/* Exported Data */

extern Pool segmentPool;

#if VMINIT
extern boolean vVMSPLIT;
  /* VMSPLIT command line switch: enables building shared VM as
     separate ROM and RAM segments */
#endif VMINIT

#endif	VM_SEGMENT_H
