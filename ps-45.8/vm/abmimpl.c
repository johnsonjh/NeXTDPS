/*
				  abmimpl.c

	     Copyright (c) 1988, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE: All information  contained herein  is  the property of Adobe  Systems
Incorporated.   Many  of  the intellectual and   technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only  to Adobe licensees for  their internal use.  Any reproduction
or dissemination of this software  is strictly forbidden unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Pasqua: Thu Mar 17 11:52:21 1988
Edit History:
Scott Byer: Wed May 17 14:28:43 1989
Joe Pasqua: Mon Jun 11 10:11:02 1990
Ed Taft: Sun Dec 17 18:35:55 1989
Ivor Durham: Mon May 15 22:00:36 1989
Rick Saenz: Tuesday, July 5, 1988 10:25:57 AM
Paul Rovner: Wednesday, November 2, 1988 10:06:54 AM
Perry Caro: Fri Nov  4 14:19:36 1988
Jim Sandman: Tue Apr  4 17:28:06 1989
Mark Francis: Thu Nov  9 15:54:28 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT

#if (OS == os_mpw)

#include <Resources.h>
#include <OSUtils.h>
#include <Files.h>
#include <ToolUtils.h>
#include <Memory.h>
/* The redefinitions below avoid conflicts between the DPS head files
   and the Macintosh head files (sigh) */
#define Fixed MPWFixed
#define LineTo MPWLineTo
#define MoveTo MPWMoveTo

#endif (OS == os_mpw)

#include BASICTYPES
#include ERROR
#include EXCEPT
#include GC
#include ORPHANS
#include PSLIB
#include STREAM
#include VM
#include RECYCLER
#include "abm.h"
#include "gcinternal.h"
#include "vm_segment.h"

/*
IMPLEMENTATION NOTES:
o Not all of the bits in the last byte of the allocation bitmap
  are necessarily valid (the segment may not be a multiple of ByteSpan
  (64) in length). We needn't worry about this when scanning for free
  space since we scan only those bits which correspond to free pool
  space. More precisely, we scan up to, but not including the byte
  corresponding to the firstStackByte. So, unless the stack is a
  multiple of ByteSpan in length and completely full, we never look
  at the last byte of the ABM. If the segment is a multiple of ByteSpan
  in length and completely full, then the last byte of the ABM has
  all valid bits.
o Currently doesn't obey strict save level boundaries. It checks
  boundaries only at the segment level.
o The tag "TMP" is used in comments to mark things that should be fixed.
  The tag "NEED THIS" marks procedures that the collector needs.
  The tag "ARBITRARY" denotes arbitrary setting of parameters (tune these).

TO DO: (in roughly decreasing order of importance)
o Tune AllocInternal(). Depending on how ABM_SetAllocated turns out,
  it might be worth special casing it inside of AllocInternal().
  Look at dynamic frequency of various cases (particularly the
  single bit case).
o Tune ABM_SetAllocated. Look at dynamic frequency of various cases
  and modify appropriately. Think about the single bit set case.
o Obey strict save level boundaries.
o ABM_RelocateData() uses FindVMSegment() to get its work done.
  When we have the addr to segment mapping table, use it instead.
*/

/*----------------------------------------------*/
/*		CONSTANT DECLARATIONS		*/
/*----------------------------------------------*/

#define	LargeAllocThreshold 80	/* ARBITRARY	*/
#define	ByteSpan	(BitsPerByte * BitSpan)

/*----------------------------------------------*/
/*		LOCAL Inline Procedures		*/
/*----------------------------------------------*/

#define	AllocInternal(size)	\
  ((size < LargeAllocThreshold) ? \
    AllocSmall((Card32)size) : AllocLarge((Card32)size))
/* private RefAny AllocInternal(Card32 size)	*/
/* Find and allocate 'size' 8 byte chunks from	*/
/* the free pool. Returns a ptr to the alloc'd	*/
/* space, NIL if no room could be found.	*/


/*----------------------------------------------*/
/*		FORWARD DECLARATIONS		*/
/*----------------------------------------------*/

private procedure PSAllocBM();
private procedure PSTrashVM();
private procedure PSAllocVM();
private RefAny AllocSmall(), AllocLarge();
private PCard8 ResetSegFreePtr();

/*----------------------------------------------*/
/*		GLOBAL DECLARATIONS		*/
/*----------------------------------------------*/

#if (OS == os_mpw)

private Card8 *hiHole, *lowHole, *maxHole, *locOfMax;
#define hiHoleID 2104
#define lowHoleID 2105
#define maxHoleID 2106
#define locOfMaxID 2107

#else (OS == os_mpw)

private readonly Card8 hiHole[256] = {
  8, 7, 6, 6, 5, 5, 5, 5,	/*   0..  7	*/
  4, 4, 4, 4, 4, 4, 4, 4,	/*   8.. 15	*/
  3, 3, 3, 3, 3, 3, 3, 3,	/*  16..	*/
  3, 3, 3, 3, 3, 3, 3, 3,	/*    .. 31	*/
  2, 2, 2, 2, 2, 2, 2, 2,	/*  32..	*/
  2, 2, 2, 2, 2, 2, 2, 2,	/*    ..	*/
  2, 2, 2, 2, 2, 2, 2, 2,	/*    ..	*/
  2, 2, 2, 2, 2, 2, 2, 2,	/*    .. 63	*/
  1, 1, 1, 1, 1, 1, 1, 1,	/*  64..	*/
  1, 1, 1, 1, 1, 1, 1, 1,	/*    ..	*/
  1, 1, 1, 1, 1, 1, 1, 1,	/*    ..	*/
  1, 1, 1, 1, 1, 1, 1, 1,	/*    ..	*/
  1, 1, 1, 1, 1, 1, 1, 1,	/*    ..	*/
  1, 1, 1, 1, 1, 1, 1, 1,	/*    ..	*/
  1, 1, 1, 1, 1, 1, 1, 1,	/*    ..	*/
  1, 1, 1, 1, 1, 1, 1, 1,	/*    .. 127	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/* 128..	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*    ..	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*    ..	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*    ..	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*    ..	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*    ..	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*    ..	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*    ..	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*    ..	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*    ..	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*    ..	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*    ..	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*    ..	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*    ..	*/
  0, 0, 0, 0, 0, 0, 0, 0,	/*    ..	*/
  0, 0, 0, 0, 0, 0, 0, 0	/*    ..255	*/
  };

private readonly Card8 lowHole[256] = {
  8, 0, 1, 0, 2, 0, 1, 0,	/*   0..  7	*/
  3, 0, 1, 0, 2, 0, 1, 0,	/*   8.. 15	*/
  4, 0, 1, 0, 2, 0, 1, 0,	/*  16.. 23	*/
  3, 0, 1, 0, 2, 0, 1, 0,	/*  24.. 31	*/
  5, 0, 1, 0, 2, 0, 1, 0,	/*  32.. 39	*/
  3, 0, 1, 0, 2, 0, 1, 0,	/*  40.. 47	*/
  4, 0, 1, 0, 2, 0, 1, 0,	/*  48.. 55	*/
  3, 0, 1, 0, 2, 0, 1, 0,	/*  56.. 63	*/
  6, 0, 1, 0, 2, 0, 1, 0,	/*  64.. 71	*/
  3, 0, 1, 0, 2, 0, 1, 0,	/*  72.. 79	*/
  4, 0, 1, 0, 2, 0, 1, 0,	/*  80.. 87	*/
  3, 0, 1, 0, 2, 0, 1, 0,	/*  88.. 95	*/
  5, 0, 1, 0, 2, 0, 1, 0,	/*  96..103	*/
  3, 0, 1, 0, 2, 0, 1, 0,	/* 104..111	*/
  4, 0, 1, 0, 2, 0, 1, 0,	/* 112..119	*/
  3, 0, 1, 0, 2, 0, 1, 0,	/* 120..127	*/
  7, 0, 1, 0, 2, 0, 1, 0,	/* 128..135	*/
  3, 0, 1, 0, 2, 0, 1, 0,	/* 136..143	*/
  4, 0, 1, 0, 2, 0, 1, 0,	/* 144..151	*/
  3, 0, 1, 0, 2, 0, 1, 0,	/* 152..159	*/
  5, 0, 1, 0, 2, 0, 1, 0,	/* 160..167	*/
  3, 0, 1, 0, 2, 0, 1, 0,	/* 168..175	*/
  4, 0, 1, 0, 2, 0, 1, 0,	/* 176..183	*/
  3, 0, 1, 0, 2, 0, 1, 0,	/* 184..191	*/
  6, 0, 1, 0, 2, 0, 1, 0,	/* 192..199	*/
  3, 0, 1, 0, 2, 0, 1, 0,	/* 200..207	*/
  4, 0, 1, 0, 2, 0, 1, 0,	/* 208..215	*/
  3, 0, 1, 0, 2, 0, 1, 0,	/* 216..223	*/
  5, 0, 1, 0, 2, 0, 1, 0,	/* 224..231	*/
  3, 0, 1, 0, 2, 0, 1, 0,	/* 232..239	*/
  4, 0, 1, 0, 2, 0, 1, 0,	/* 240..247	*/
  3, 0, 1, 0, 2, 0, 1, 0	/* 248..255	*/
  };

private readonly Card8 maxHole[256] = {
  8, 7, 6, 6, 5, 5, 5, 5,	/*   0..  7	*/
  4, 4, 4, 4, 4, 4, 4, 4,	/*   8.. 15	*/
  4, 3, 3, 3, 3, 3, 3, 3,	/*  16.. 23	*/
  3, 3, 3, 3, 3, 3, 3, 3,	/*  24.. 31	*/
  5, 4, 3, 3, 2, 2, 2, 2,	/*  32.. 39	*/
  3, 2, 2, 2, 2, 2, 2, 2,	/*  40.. 47	*/
  4, 3, 2, 2, 2, 2, 2, 2,	/*  48.. 55	*/
  3, 2, 2, 2, 2, 2, 2, 2,	/*  56.. 63	*/
  6, 5, 4, 4, 3, 3, 3, 3,	/*  64.. 71	*/
  3, 2, 2, 2, 2, 2, 2, 2,	/*  72.. 79	*/
  4, 3, 2, 2, 2, 1, 1, 1,	/*  80.. 87	*/
  3, 2, 1, 1, 2, 1, 1, 1,	/*  88.. 95	*/
  5, 4, 3, 3, 2, 2, 2, 2,	/*  96..103	*/
  3, 2, 1, 1, 2, 1, 1, 1,	/* 104..111	*/
  4, 3, 2, 2, 2, 1, 1, 1,	/* 112..119	*/
  3, 2, 1, 1, 2, 1, 1, 1,	/* 120..127	*/
  7, 6, 5, 5, 4, 4, 4, 4,	/* 128..135	*/
  3, 3, 3, 3, 3, 3, 3, 3,	/* 136..143	*/
  4, 3, 2, 2, 2, 2, 2, 2,	/* 144..151	*/
  3, 2, 2, 2, 2, 2, 2, 2,	/* 152..159	*/
  5, 4, 3, 3, 2, 2, 2, 2,	/* 160..167	*/
  3, 2, 1, 1, 2, 1, 1, 1,	/* 168..175	*/
  4, 3, 2, 2, 2, 1, 1, 1,	/* 176..183	*/
  3, 2, 1, 1, 2, 1, 1, 1,	/* 184..191	*/
  6, 5, 4, 4, 3, 3, 3, 3,	/* 192..199	*/
  3, 2, 2, 2, 2, 2, 2, 2,	/* 200..207	*/
  4, 3, 2, 2, 2, 1, 1, 1,	/* 208..215	*/
  3, 2, 1, 1, 2, 1, 1, 1,	/* 216..223	*/
  5, 4, 3, 3, 2, 2, 2, 2,	/* 224..231	*/
  3, 2, 1, 1, 2, 1, 1, 1,	/* 232..239	*/
  4, 3, 2, 2, 2, 1, 1, 1,	/* 240..247	*/
  3, 2, 1, 1, 2, 1, 1, 0	/* 248..255	*/
  };

private readonly Card8 locOfMax[256] = {
  0, 1, 2, 2, 3, 3, 3, 3,	/*   0..  7	*/
  4, 4, 4, 4, 4, 4, 4, 4,	/*   8.. 15	*/
  0, 1, 5, 5, 5, 5, 5, 5,	/*  16.. 23	*/
  0, 5, 5, 5, 5, 5, 5, 5,	/*  24.. 31	*/
  0, 1, 2, 2, 0, 3, 3, 3,	/*  32.. 39	*/
  0, 1, 6, 6, 0, 6, 6, 6,	/*  40.. 47	*/
  0, 1, 2, 2, 0, 6, 6, 6,	/*  48.. 55	*/
  0, 1, 6, 6, 0, 6, 6, 6,	/*  56.. 63	*/
  0, 1, 2, 2, 3, 3, 3, 3,	/*  64.. 71	*/
  0, 1, 4, 4, 0, 4, 4, 4,	/*  72.. 79	*/
  0, 1, 2, 2, 0, 1, 0, 3,	/*  80.. 87	*/
  0, 1, 0, 2, 0, 1, 0, 5,	/*  88.. 95	*/
  0, 1, 2, 2, 0, 3, 3, 3,	/*  96..103	*/
  0, 1, 0, 2, 0, 1, 0, 4,	/* 104..111	*/
  0, 1, 2, 2, 0, 1, 0, 3,	/* 112..119	*/
  0, 1, 0, 2, 0, 1, 0, 7,	/* 120..127	*/
  0, 1, 2, 2, 3, 3, 3, 3,	/* 128..135	*/
  0, 4, 4, 4, 4, 4, 4, 4,	/* 136..143	*/
  0, 1, 2, 2, 0, 5, 5, 5,	/* 144..151	*/
  0, 1, 5, 5, 0, 5, 5, 5,	/* 152..159	*/
  0, 1, 2, 2, 0, 3, 3, 3,	/* 160..167	*/
  0, 1, 0, 2, 0, 1, 0, 4,	/* 168..175	*/
  0, 1, 2, 2, 0, 1, 0, 3,	/* 176..183	*/
  0, 1, 0, 2, 0, 1, 0, 6,	/* 184..191	*/
  0, 1, 2, 2, 3, 3, 3, 3,	/* 192..199	*/
  0, 1, 4, 4, 0, 4, 4, 4,	/* 200..207	*/
  0, 1, 2, 2, 0, 1, 0, 3,	/* 208..215	*/
  0, 1, 0, 2, 0, 1, 0, 5,	/* 216..223	*/
  0, 1, 2, 2, 0, 3, 3, 3,	/* 224..231	*/
  0, 1, 0, 2, 0, 1, 0, 4,	/* 232..239	*/
  0, 1, 2, 2, 0, 1, 0, 3,	/* 240..247	*/
  0, 1, 0, 2, 0, 1, 0, 0	/* 248..255	*/
  };

#endif (OS == os_mpw)

/*----------------------------------------------*/
/*	EXPORTED GCInternal Procedures		*/
/* 		Used only in VM pkg		*/ 
/*----------------------------------------------*/

procedure GCInternal_ResetFreePointer(vm)
  /* Starting at the current segment, look at each seg	*/
  /* and determine whether the stack free ptr can	*/
  /* be moved backward over heap space. If so, reset	*/
  /* stack ptr. If we were able to reset the stack ptr	*/
  /* all the way to the beginning of the segment, then	*/
  /* try it again in the previous segment. Keep doing	*/
  /* this until either the stack free ptr can't be	*/
  /* reset to the beginning of a segment, or the next	*/
  /* segment to look at is at a different save level.	*/
  /* When as many segs as possible have been updated,	*/
  /* call ResetVMSection to make the last updated seg	*/
  /* be the current one.				*/
  PVM vm;
  {
  PCard8 newFree;
  Level currentLevel;
  register PVMSegment seg;
  register boolean isCurrent;

  seg = vm->current;
  isCurrent = true;
  currentLevel = seg->level;
  while (1)	/* FOR each segment in this space DO	*/
    {
    newFree = ResetSegFreePtr(seg, isCurrent ? vm->free : seg->free);
    if (isCurrent) vm->free = newFree;
    seg->free = newFree;
    seg->firstStackByte = newFree;
    if (seg->first == seg->free
    && seg->pred != NIL && seg->pred->level == currentLevel)
      {
      seg = seg->pred;
      isCurrent = false;
      }
    else break;	/* NOTE EXIT FROM LOOP			*/
    }
  if (!isCurrent)
    ResetVMSection(vm, seg->free, seg->level);
  else
    InitRecycler(vm->recycler, vm->free);
  }

/*----------------------------------------------*/
/*		EXPORTED ABM Procedures		*/
/* Exported to abm.h -- Used only in the VM pkg	*/ 
/*----------------------------------------------*/

RefAny ABM_AllocateVM(size, shared)
  integer size;
  boolean shared;
  {
  RefAny retVal;
  boolean originalShared = vmCurrent->shared;

  SetShared(shared);
  /* Deal with automatic collection issues...		*/
  vmCurrent->allocCounter += size;
  if (vmCurrent->allocCounter > vmCurrent->allocThreshold)
    vmCurrent->collectThisSpace = shared ? autoShared : autoPrivate;

  DURING
    {
    retVal = AllocInternal((size + BitSpan - 1)/BitSpan);
    }
  HANDLER
    {
    SetShared(originalShared);
    RERAISE;
    }
  END_HANDLER;
  SetShared(originalShared);
  return (retVal);
  }

RefAny ABM_Allocate(size)
  integer size;
  {
  /* Deal with automatic collection issues...		*/
  vmCurrent->allocCounter += size;
  if (vmCurrent->allocCounter > vmCurrent->allocThreshold)
    vmCurrent->collectThisSpace = CurrentShared() ? autoShared : autoPrivate;
  return(AllocInternal((size + BitSpan - 1)/BitSpan));
  }

procedure ABM_ClearAll(space)
/* Zero out the alloc bitmap associated with	*/
/* each segments in the specified space.	*/
VMStructure *space;
{
  register PVMSegment cur;
  
  for (cur = space->head; cur != NIL; cur = cur->succ)
    {
    register PCard8 free = (cur == space->current) ?
      space->free : cur->free;
    cur->firstStackByte = free;
    cur->firstFreeInABM = cur->allocBM;
    if (cur->allocBM != NIL)
    	os_bzero((char *)cur->allocBM, (long int)ABM_BytesForBitmap(cur));
      /* We really only need to zero the bitmap bytes	*/
      /* that correspond to the data bytes before the	*/
      /* free pointer. The bitmap allocator won't look	*/
      /* beyond that point anyway.			*/
    }
}

procedure ABM_SetAllocated(ref, sH, len)
/* The range [ref..ref+len) is an area in the segment	*/
/* sH. Set the bits associated with this range.		*/
RefAny ref;
PVMSegment sH;
Card32 len;
{
  Card32 bitInByte;
  register Card32 nBits;
  register PCard8 curByte;

  {	/* 	----- DECLARATION BLOCK -----		*/
  Card32 offset;
  Card32 startBit;

  if (sH->allocBM == NIL)
     {
     DebugAssert(sH->level == stROM || sH->level == stPermanentRAM);
     return;  /* permanent segment; nothing to do */
     }
 
  offset = CAST(ref - sH->first, Card32);
    /* Offset of ref from beginning of data in segment	*/
  startBit = offset/BitSpan;
    /* Bit # of first bit to mark for this reference	*/
  nBits = (offset + len - 1)/BitSpan - startBit + 1;
    /* Number of bits to set for this reference & len	*/
  curByte = sH->allocBM + startBit/BitsPerByte;
    /* Byte in the allocBM that contains startBit	*/
  bitInByte = startBit % BitsPerByte;
    /* Bit in curByte that corresponds to ref		*/
  }

  /* Check for case where start & stop are in same byte	*/
  if (bitInByte + nBits <= BitsPerByte)
    *curByte |= ((0xFF << bitInByte) & ~(0xFF << nBits + bitInByte));
  else
    {
    register int i;

    *curByte++ |= 0xFF << bitInByte;	/* Handle first byte	*/
    nBits -= (BitsPerByte - bitInByte);
    /* Now handle the middle bytes...				*/
    while (nBits > BitsPerByte)
      {
      *curByte++ = 0xFF;
      nBits -= 8;
      }
    *curByte |= ~(0xFF << nBits);	/* Handle last byte	*/
      /* Technically we should test to see if nBits is nonzero	*/
      /* before touching the last byte, but we know we won't	*/
      /* fault or screw up data, so its faster this way.	*/
    }
}

#if (OS == os_mpw)
private short int OpenPSVMResource() {
  SysEnvRec sysEnvRec;
  short int resFile;
  char *PSVMNAME = "PostScript.VM";

  SysEnvirons(1, &sysEnvRec);
#if MPW3
  return openrfperm(PSVMNAME, sysEnvRec.sysVRefNum, fsRdWrPerm);
#else /* MPW3 */  
  return OpenRFPerm(PSVMNAME, sysEnvRec.sysVRefNum, fsRdWrPerm);
#endif /* MPW3 */
  }
#endif (OS == os_mpw)

procedure ABM_Init(reason)
/* Initialize the Allocation Bitmap machinery	*/
InitReason reason;
{
  switch (reason) {
    case init:		/* FIRST initialization step	*/
#if (OS == os_mpw)
	  { Handle h;
	    char *bf, *b2;
		integer i;
		short bytecount;
	    
        short int resFile0 = CurResFile();
        short int resFile = OpenPSVMResource();

        Assert(resFile != -1);
        UseResFile(resFile);

	    h = GetResource('BYTV',hiHoleID);
		if (h == NULL) CantHappen();
		LoadResource(h);
		MoveHHi(h);
		bytecount = *(short *)*h;
		hiHole = (Card8 *)os_sureCalloc(sizeof(Card8),bytecount);
		b2 = hiHole; bf = ((Card8 *) *h) + 2;
		for (i = 0; i < bytecount; i++) *b2++ = *bf++;
		ReleaseResource(h);
		
	    h = GetResource('BYTV',lowHoleID);
		if (h == NULL) CantHappen();
		LoadResource(h);
		MoveHHi(h);
		bytecount = *(short *)*h;
		lowHole = (Card8 *)os_sureCalloc(sizeof(Card8),bytecount);
		b2 = lowHole; bf = ((Card8 *) *h) + 2;
		for (i = 0; i < bytecount; i++) *b2++ = *bf++;
		ReleaseResource(h);
		
	    h = GetResource('BYTV',maxHoleID);
		if (h == NULL) CantHappen();
		LoadResource(h);
		MoveHHi(h);
		bytecount = *(short *)*h;
		maxHole = (Card8 *)os_sureCalloc(sizeof(Card8),bytecount);
		b2 = maxHole; bf = ((Card8 *) *h) + 2;
		for (i = 0; i < bytecount; i++) *b2++ = *bf++;
		ReleaseResource(h);
		
	    h = GetResource('BYTV',locOfMaxID);
		if (h == NULL) CantHappen();
		LoadResource(h);
		MoveHHi(h);
		bytecount = *(short *)*h;
		locOfMax = (Card8 *)os_sureCalloc(sizeof(Card8),bytecount);
		b2 = locOfMax; bf = ((Card8 *) *h) + 2;
		for (i = 0; i < bytecount; i++) *b2++ = *bf++;
		ReleaseResource(h);
		
        CloseResFile(resFile);
        UseResFile(resFile0);
		}
		
#endif (OS == os_mpw)
      break;
    case romreg:	/* SECOND initialization step	*/
#if	STAGE==DEVELOP
      if (vSTAGE==DEVELOP)
        {
	RgstExplicit ("allocvm", PSAllocVM);
	RgstExplicit ("allocbm", PSAllocBM);
	RgstExplicit ("trashvm", PSTrashVM);
	}
#endif	STAGE==DEVELOP
      break;
    case ramreg:	/* THIRD initialization step	*/
      break;
    default: CantHappen();
    }
}

/*----------------------------------------------*/
/*		Private ABM Procedures		*/
/*----------------------------------------------*/

private PCard8 ResetSegFreePtr(seg, oldFree)
  PVMSegment seg;
  PCard8 oldFree;
/*
  DESCRIPTION:
  This routine attempts to move the segment's free pointer backward into
  the heap space as far as possible. It does this by examining the
  allocation bitmap searching for 0 bits that are contiguous to the bit
  that corresponds to the free pointer. The previous free pointer is
  passed in by the caller (remember the free pointer in the segment
  header isn't always correct for the current segment). This routine
  returns the value that the free pointer can be set to. It does
  not actually set anything to this value.
  
  CONDITIONS OF OPERATION:
  1. This operation is invoked only immedeately after a garbage
     collection, before any new allocations in the space have occured.
  2. The collector must invalidate the recycler range before beginning the
     collection and move it if possible.
  
  ALGORITHM:
  Find the bit corresponding to the segment's free pointer. It represents
  8 bytes of memory, some of which may be past where the free pointer
  points and some of which may be before. This bit is either set or clear
  depending on whether any of the eight bytes contain useful data. The
  bytes from the free pointer forward are guaranteed to be empty, so they
  can not cause the bit to be set.  The bytes before the free pointer
  will only be set if there is really useful data in them. Remember, the
  Collector invalidated and moved any active recycler range.
  
  Test this initial bit. If it is set, return. If it is clear, continue
  scanning backward in the allocation bitmap until either the entire
  bitmap has been scanned or a set bit is found. Return a free pointer
  that corresponds with the last clear bit encountered.
*/
  {
  int h, curBit;
  PCard8 curByte, lastByteToScan, newFree;
  register int thisBit, curVal;
 
   if (seg->allocBM == NIL)
     {
     DebugAssert(seg->level == stROM || seg->level == stPermanentRAM);
     return oldFree;
     }

  curByte = seg->allocBM + (oldFree - seg->first) / ByteSpan;
    /* This is a ptr into the allocBM. It refers to	*/
    /* the byte whose bits represent the data space	*/
    /* containing the 1st stack byte. 			*/
  curBit = ( (oldFree - seg->first) / BitSpan ) % BitsPerByte;
  if (lowHole[curVal = *curByte] < curBit + 1)
    {
    /* Are there any clear bits next to the free ptr?	*/
    for (thisBit = curBit; thisBit >= 0; thisBit--)
      if ( ((1 << thisBit) & curVal) != 0 )
        break;
    if (thisBit == curBit)
      return(oldFree);	/* Absolutely no luck!		*/
    newFree =
      seg->first + (curByte - seg->allocBM) * ByteSpan +
      (thisBit + 1) * BitSpan;
    return(newFree);
    }
  curByte--;

  for (
    lastByteToScan = seg->allocBM;
    curByte >= lastByteToScan && *curByte == 0;
    curByte--);
  
  /* At this point, curByte is the 1st byte we hit with	*/
  /* 1 bits in it. The new free ptr will point at the	*/
  /* the first byte of the 8 byte range associated with	*/
  /* the last clear bit in the byte.			*/
  h = (curByte < lastByteToScan) ? 0 : hiHole[*curByte];
  newFree =
    seg->first + (curByte - seg->allocBM) * ByteSpan +
    (BitsPerByte - h) * BitSpan;
  return(newFree);
  }

private RefAny AllocSmall(size)
/* Find and allocate 'size' 8 byte chunks from	*/
/* the free pool. Returns a ptr to the alloc'd	*/
/* space, NIL if no room could be found.	*/
Card32 size;
{
  Level currentLevel;
  PCard8 lastByte;
  register PVMSegment seg;
  int segsToExamine;
  
  /* Determine the # of segs to examine by figuring out	*/
  /* how many segments it would take to hold an		*/
  /* allocThreshold's worth of data. Add a segment to	*/
  /* deal with fragmentation and segments with a non-	*/
  /* trivial amount of remaining stack alloc space (eg	*/
  /* vmCurrent->current).				*/
  segsToExamine = ((vmCurrent->allocThreshold + vmCurrent->expansion_size - 1)
                  / vmCurrent->expansion_size) + 1;

  for (	/* FOR each segment in this space DO		*/
    seg = vmCurrent->current, currentLevel = seg->level;
    seg != NIL && seg->level == currentLevel && segsToExamine > 0;
    seg = seg->pred, segsToExamine--)
    {
    int hiSize;
    RefAny retVal;
    int carryIn = 0;
    PCard8 startOfHole;
    register Card8 curVal;
    register PCard8 curByte;

	DebugAssert(seg->allocBM != NIL);

    lastByte = seg->allocBM + (seg->firstStackByte - seg->first) / ByteSpan;
      /* This is a ptr into the allocBM. It refers to	*/
      /* the byte whose bits represent the data space	*/
      /* containing the 1st stack byte. Since we can't	*/
      /* alloc in stk space, we can only look at BM	*/
      /* bytes up to, but not including, this byte.	*/

    for ( /* FOR each byte in alloc bitmap DO	*/
      startOfHole = curByte = seg->firstFreeInABM;
      curByte < lastByte;
      curByte++)
      {
      curVal = *curByte;
      if (lowHole[curVal] + carryIn >= size)
        {
        retVal = CAST(
          seg->first + (startOfHole - seg->allocBM) * ByteSpan,
          RefAny);
        if ((carryIn &= 0x07) != 0)
          retVal += (BitsPerByte - carryIn) * BitSpan;
        ABM_SetAllocated(retVal, seg, size * BitSpan);
        return(retVal);
        }
      if (maxHole[curVal] >= size)
        {
        retVal = CAST(
          seg->first +
            (((curByte - seg->allocBM) * BitsPerByte) + locOfMax[curVal])
            * BitSpan,
          RefAny);
        ABM_SetAllocated(retVal, seg, size * BitSpan);
        return(retVal);
        }
      switch (hiSize = hiHole[curVal]) {
        case 0:
          /* No hi order bits to carry along - move start	*/
	  carryIn = 0;
	  startOfHole = curByte + 1;
	  break;
	case BitsPerByte:
	  /* An empty byte, carry it along, don't move start	*/
	  carryIn += BitsPerByte;
	  break;
	default:
	  /* Some high bits. Carry along, make this new start	*/
	  carryIn = hiSize;
          startOfHole = curByte;
          break;
        } /* SWITCH on hiSize	*/
      } /* FOR each byte in alloc bitmap DO	*/
    /* The following test says basically the following:		*/
    /* If we have scanned this segment's ABM and couldn't find	*/
    /* some small amount of free space, don't bother scanning	*/
    /* this segment's ABM again. We implement this by setting	*/
    /* the hint about the first free space in the abm to the	*/
    /* effective end of the ABM.				*/
    if (size < ABM_EffectivelyNoSpace)
      seg->firstFreeInABM = lastByte;
    } /* FOR each seg in this space DO	*/

  return(NIL);	/* Not enough space anywhere	*/
} /* AllocSmall */

private RefAny AllocLarge(size)
Card32 size;
{
  Level currentLevel;
  register PVMSegment seg;

  for (	/* FOR each segment in this space DO		*/
    seg = vmCurrent->current, currentLevel = seg->level;
    seg != NIL && seg->level == currentLevel;
    seg = seg->pred)
    {
    RefAny retVal;
    PCard8 lastAllocBMByteToCheck;
    register PCard8 lastByteToMatch;
    register PCard8 curByte;
    
    DebugAssert(seg->allocBM != NIL);

    lastAllocBMByteToCheck = seg->allocBM +
      (seg->firstStackByte - seg->first) / ByteSpan;
      /* This is a ptr into the allocBM. It refers to	*/
      /* the byte whose bits represent the data space	*/
      /* containing the 1st stack byte. Since we can't	*/
      /* alloc in stk space, we can only look at BM	*/
      /* bytes up to, but not including, this byte.	*/

    lastByteToMatch = seg->firstFreeInABM;
    curByte = lastByteToMatch + (size - 1)/BitsPerByte;
    while (curByte < lastAllocBMByteToCheck)
      {	/* WHILE more bytes to check DO...		*/
      if (*curByte == 0)	/* Found a hole		*/
        {
        if (curByte == lastByteToMatch)
          {
          /* Compute data space addr assoc'd w/ curByte	*/
          /* Mark space as alloc'd. Return ptr to it.	*/
          retVal = CAST(
            seg->first + (curByte - seg->allocBM) * ByteSpan,
            RefAny);
          ABM_SetAllocated(retVal, seg, size * BitSpan);
          return(retVal);	/* NOTE return		*/
          }
        else	/* We're not done matching yet...	*/
          curByte--;
        }
      else	/* curByte is not a hole		*/
        {
        lastByteToMatch = curByte + 1;
        curByte = lastByteToMatch + (size - 1)/BitsPerByte;
        }
      }	/* WHILE more bytes to check DO...		*/
    } /* FOR each seg in this space DO	*/

  return(NIL);	/* Not enough space anywhere	*/
} /* AllocLarge	*/

/*----------------------------------------------*/
/*	DEVELOP ONLY ABM Private Procedures	*/
/*----------------------------------------------*/

#if	STAGE==DEVELOP

extern integer PSPopInteger(); /* Cant include language.h (circular) */
private procedure PSAllocVM()
/* Pop an integer from the stack and alloc that many	*/
/* bytes of VM using the bitmap allocator.		*/
{
  RefAny retVal;
  PVMSegment sH;
  integer size = PSPopInteger();
  
  if (size <= 0)
    {
    os_eprintf("funny size = %d\n", size);
    return;
    }
  retVal = ABM_Allocate(size);
  if (retVal == NIL)
    {
    os_eprintf("Allocator returned NIL\n");
    }
  else
    {
    sH = GCInternal_GetSegHnd(retVal, vmCurrent);
    os_eprintf("Address returned: %x\n", retVal);
    os_eprintf("This is in segment %x\n", sH);
    os_eprintf("Its offset is %x\n", retVal - sH->first);
    }
}

private procedure PSAllocBM()
/* Display the allocation bitmap for the current space	*/
{
  register PVMSegment cur;
  register PCard8 curByte;
  charptr lastByte;
  int i, j, count;
  Card8 lastVal;

  for (cur = vmCurrent->head; cur != NIL; cur = cur->succ)
    /* FOR each segment DO				*/
    {
    lastVal = 0; count = 0;
    curByte = cur->allocBM;
    lastByte = (curByte == NIL)? NIL : curByte + ABM_BytesForBitmap(cur) - 1;
    while (curByte < lastByte)
      /* WHILE more bytes to process DO			*/
      /* Don't worry about the last byte. This is only	*/
      /* a test piece of code, so don't sweat it...	*/
      {
      Card8 tmp = *curByte++;
      if (tmp == lastVal) count++;
      else
        {
        if (count < 10)
          for (j = 0; j < count; j++)
            os_eprintf("%02x", lastVal);
        else
          os_eprintf(" (%d)%02x ", count, lastVal);
        lastVal = tmp; count = 1;
        }
      }	/* WHILE more bytes to process DO		*/
    if (count < 10)
      {
      for (j = 0; j < count; j++)
	os_eprintf("%02x", lastVal);
      putchar('\n');
      }
    else
      os_eprintf(" (%d)%02x\n", count, lastVal);
    }	/* FOR each segment DO				*/
}

private procedure PSTrashVM()
/* Sets all freed locations to 0xFF.			*/
{
  register PVMSegment cur;
  register PCard8 curByte;
  register PCard32 dataPtr;
  charptr lastByte;
  int i;

  os_eprintf("Trashing the garbage...\n");
  for (cur = vmCurrent->head; cur != NIL; cur = cur->succ)
    /* FOR each segment DO				*/
    {	
    dataPtr = CAST(cur->first, PCard32);
    curByte = cur->allocBM;
    lastByte = (curByte == NIL)? NIL : curByte + ABM_BytesForBitmap(cur) - 1;
    while (curByte < lastByte)
      /* WHILE more bytes to process DO			*/
      /* Don't worry about the last byte. This is only	*/
      /* a test piece of code, so don't sweat it...	*/
      {
      Card8 tmp = *curByte++;
      for (i = 0; i < 8; i++)
        /* FOR each bit in the byte DO			*/
        {
        if ((tmp & 1) == 0)
          {*dataPtr++ = 0xFFFFFFFF; *dataPtr++ = 0xFFFFFFFF;}
        else dataPtr += 2;
        tmp = tmp >> 1;
        }	/* FOR each bit in the byte DO		*/
      }	/* WHILE more bytes to process DO		*/
    }	/* FOR each segment DO				*/
  os_eprintf("Done trashing the garbage...\n");
}
#endif	STAGE==DEVELOP

/*----------------------------------------------*/
/*		PUBLIC GC Procedures		*/
/*----------------------------------------------*/

RefAny GC_MoveRecycleRange(recycler, refs, nRefs)
PRecycler recycler;
PObject *refs;
integer nRefs;
{
  /* IMPLEMENTATION NOTES: This is a mini trace algorithm that	*/
  /* uses the opStk and the refs param as roots and traces thru	*/
  /* the given recycler. Anything found to point into the	*/
  /* range is updated to point at the range's new home. The 1st	*/
  /* thing we do is try and alloc the new space for the range.	*/
  /* We use that space as the pending stack for the trace and	*/
  /* copy the range itself into the new storage at the very end	*/
  /* of the process (when we're done with the pending stack).	*/
  /* We note that an object has been seen before by checking	*/
  /* whether or not it points into the range. An object in the	*/
  /* range thats been seen before will have had its value field	*/
  /* updated to point into the new range and has therefore been	*/
  /* seen. We can't use the seen bit in objects since we need	*/
  /* to preserve their existing values.				*/
  register PObject obj;
  integer count;
  integer size = recycler->end - recycler->begin;
  Card16 offset;
  PCard8 start = recycler->begin;
  RefAny newStorage;
  boolean firstTime = true;
  GC_Stack stkHeader;
  boolean oldShared = CurrentShared();
  register GC_PStack pending = &stkHeader;
  /* TMP: TOTAL KLUDGE FOLLOWS!!!!!!!!!!!!!!!!!!!!!!!!!	*/
  extern struct {	/* From language.h	*/
    PObject head;
    PObject base;
    PObject limit;
  } *opStk, *execStk;

#define	CreateStack(stk, mem, size)			\
  {							\
  (stk).nextFree = (stk).base = CAST((mem), PObject *);	\
  (stk).limit = (stk).base + (size)/sizeof(PObject);	\
  GCInternal_PushNoTest(&(stk), (PObject)NIL); firstTime=false;	\
  }

#define	AdjustRef(obj, old, new)	\
  (obj)->val.strval = (new) + ((obj)->val.strval - old)

#define InRange(o) AddressInRecyclerRange(recycler, RecyclerAddress(o))
#define CouldRecycle(o) IsRecyclableType(o) && InRange(o)

  if (size == 0) return(NIL);

  SetShared(recycler == sharedRecycler);
    /* Make sure to do this SetShared before doing the	*/
    /* test on vmCurrent->allocCounter (see below).	*/
    
  /* Deal with automatic collection issues...		*/
  vmCurrent->allocCounter += size;
  if (vmCurrent->allocCounter > vmCurrent->allocThreshold)
    vmCurrent->collectThisSpace = CurrentShared() ? autoShared : autoPrivate;

  /* If any item on execStk points into range then we	*/
  /* cant relocate. The interpreter loop may be holding	*/
  /* a pointer to the object that won't get relocated.	*/
  if (recycler->refCount > 0)
    for (obj = execStk->base; obj < execStk->head; obj++)
      {  /* FOR each execStk item DO	*/
      if (InRange(obj))
        {
        SetShared(oldShared);
	return(NULL);
        }
      }

  /* Get the new storage...				*/
  offset = ((long unsigned int)start) & (vPREFERREDALIGN - 1);
  newStorage = AllocInternal(((size+offset) + BitSpan - 1)/BitSpan);
  if (newStorage == NIL)
    {
    SetShared(oldShared);
    return(NIL);
    }
  else newStorage += offset;

  /* Now the tedious part, we must trace the recycle	*/
  /* range using the opStk and the argument ref(s) as	*/
  /* potential roots. Any item found to refer to the	*/
  /* recycle range must be updated to reflect the move.	*/

  /* Handle the refs that the caller has popped off the	*/
  /* stack. When we hit an array, process it in line.	*/
  /* E.g. adjust ref to strings and push array objs.	*/
  while (nRefs > 0)
    {  /* WHILE there is another ref DO	*/
    obj = refs[--nRefs];
    if (CouldRecycle(obj))
      {  /* Handle reference into range	*/
      if ( obj->type == arrayObj )
        {  /* Handle array object	*/
        register int i;
        register PObject item = obj->val.arrayval;
        
        for (i = obj->length; i > 0; i--, item++)
          {  /* FOR each array item DO	*/
          if (CouldRecycle(item))
            {
            if (item->type == arrayObj)
              {
              if (firstTime)
                CreateStack(stkHeader, newStorage - offset, size);
              GCInternal_PushNoTest(pending, item);
              }
            else	/* Its a string	*/
              AdjustRef(item, start, newStorage);
            }
          }  /* FOR each array item DO	*/
        }  /* Handle array object	*/
      AdjustRef(obj, start, newStorage);
      }  /* Handle reference into range	*/
    }  /* WHILE there is another ref DO	*/

  /* We now scan the opStk looking for items that point	*/
  /* into the recycleRange. We know there are exactly	*/
  /* range->refcnt items that point into the range, so	*/
  /* we scan until we've found that many.		*/
  count = recycler->refCount;
  for (obj = opStk->base; count > 0; obj++)
    {  /* FOR each opStk item DO	*/
    Assert(obj < opStk->head);
    if (CouldRecycle(obj))
      {  /* Handle reference into range	*/
      if ( obj->type == arrayObj )
        {  /* Handle array object	*/
        register int i;
        register PObject item = obj->val.arrayval;
        
        for (i = obj->length; i > 0; i--, item++)
          {  /* FOR each array item DO	*/
          if (CouldRecycle(item))
            {
            if (item->type == arrayObj)
              {
              if (firstTime)
                CreateStack(stkHeader, newStorage - offset, size);
              GCInternal_PushNoTest(pending, item);
              }
            else	/* Its a string	*/
              AdjustRef(item, start, newStorage);
            }
          }  /* FOR each array item DO	*/
        }  /* Handle array object	*/
      AdjustRef(obj, start, newStorage);
      count--;
      }  /* Handle reference into range	*/
    }  /* FOR each opStk item DO	*/

  /* At this point we may have pushed a bunch of array	*/
  /* objects onto the pending stack. We trace from each	*/
  /* after checking whether its been seen before.	*/
  if (!firstTime)
    while ( (obj = GCInternal_Pop(pending)) != NIL )
      {  /* WHILE more items on stack DO	*/
      register int i;
      register PObject item;
  
      Assert(obj->type == arrayObj);
      item = obj->val.arrayval;
      AdjustRef(obj, start, newStorage);
      for (i = obj->length; i > 0; i--, item++)
	{  /* FOR each array item DO	*/
	if (CouldRecycle(item))
	  {
	  if (item->type == arrayObj)
	    {GCInternal_PushNoTest(pending, item);}
	  else	/* Its a string		*/
	    AdjustRef(item, start, newStorage);
	  }
	}  /* FOR each array item DO	*/
      }  /* WHILE items on stack DO	*/
  
  os_bcopy((char *)recycler->begin, (char *)newStorage, size);
  SetShared(oldShared);
  return(newStorage);
}


