/*
  abm.h

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

Original version: Pasqua: Thu Mar 17 13:25:18 1988
Edit History:
Joe Pasqua: Tue Oct 25 14:06:47 1988
Ivor Durham: Fri Sep 23 10:21:56 1988
End Edit History.
*/

#ifndef	ABM_H
#define	ABM_H

#include	"gcinternal.h"

/*	----- Types -----	*/


/*	----- Constants -----	*/

#define	ABM_EffectivelyNoSpace	64	/* ARBITRARY - TUNE	*/
  /* When an allocation of this size or less can not be	*/
  /* satisfied from a particular segment's free pool,	*/
  /* consider that segment effectively out of space.	*/
#define	ABM_MaxSegsToExamine	2	/* ARBITRARY - TUNE	*/
  /* When allocating from the free pool, only scan this	*/
  /* number of segments back from the current segment.	*/
  /* This is done in attempt to avoid looking at every	*/
  /* seg in a failed try at alloc from the free pool.	*/

#define	BitsPerByte	8
#define	BitSpan		8
  /* Number of bytes in VM represented by one	*/
  /* bit in the allocation bitmap.		*/

/*	----- Procedure Declarations -----	*/

extern RefAny ABM_Allocate(/* integer size */);
  /* Allocate size bytes from the free pool of	*/
  /* the current vm (private or shared). If	*/
  /* size bytes can not be found, return NIL.	*/
  /* Otherwise return a pointer to the alloc'd	*/
  /* space. The space will be 8 byte aligned.	*/

extern RefAny ABM_AllocateVM(/* integer size, boolean shared */);
  /* Allocate size bytes from the free pool of	*/
  /* the shared VM if shared is true, or from	*/
  /* the current private VM otherwise. If	*/
  /* size bytes can not be found, return NIL.	*/
  /* Otherwise return a pointer to the alloc'd	*/
  /* space. The space will be 8 byte aligned.	*/

extern procedure ABM_ClearAll(/* VMStructure *space */);
  /* Zero out the alloc bitmap associated with	*/
  /* each segments in the specified space.	*/

extern procedure ABM_Init(/* InitReason reason */);
  /* Initialize the allocation bitmap mechanism	*/

extern procedure ABM_SetAllocated(
  /* RefAny ref, VMSegment *sH, Card32 len */);
  /* The range [ref..ref+len) is an area in sH.	*/
  /* Set the bits associated with this range.	*/
  /* len is given in bytes.			*/

/*	----- Inline Procedures -----	*/

#define	ABM_BitsForBitmap(size)	(((size) + BitSpan - 1)/BitSpan)
  /* Number of bits required to represent a	*/
  /* given size of VM. size is in bytes.	*/

#define	ABM_BytesForSize(size)	\
	((ABM_BitsForBitmap((size)) + BitsPerByte - 1)/BitsPerByte)
  /* # of bytes of bitmap required to represent	*/
  /* a given size of VM. size is in bytes.	*/

#define	ABM_BytesForBitmap(v)	ABM_BytesForSize(VMSegmentSize( (v) )) 
  /* # of bytes of bitmap required to represent	*/
  /* a given vmSegment. v is a VMSegment.	*/

#endif	ABM_H
