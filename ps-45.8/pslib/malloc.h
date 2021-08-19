/*
  malloc.h

Copyright (c) 1986, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Edit History:
Ivor Durham: Thu Aug 28 18:06:01 1986
Ed Taft: Sun Jan 31 17:53:49 1988
Paul Rovner: Tue Sep  5 17:08:05 1989
End Edit History.
*/

#ifndef MALLOC_H
#define MALLOC_H

#include ENVIRONMENT

#ifndef MALLOC_DEBUG
#define MALLOC_DEBUG (STAGE == DEVELOP)
#endif

#if MALLOC_DEBUG
#define MASSERT(cond,aboutp) \
if(!(cond)) \
  if (allocDebugLevel) \
    malloc_botch("cond",aboutp); \
  else \
    ; \
else 

extern int allocDebugLevel;
#else MALLOC_DEBUG
#define MASSERT(p,q)
#endif MALLOC_DEBUG

/*	avoid break bug */
#ifdef pdp11
#define GRANULE 64
#else
#define GRANULE 0
#endif

/*	C storage allocator
 *	circular first-fit strategy
 *	works with noncontiguous, but monotonically linked, arena
 *	each block is preceded by a ptr to the (pointer of) 
 *	the next following block
 *	blocks are exact number of words long 
 *	aligned to the data type requirements of ALIGN
 *	pointers to blocks must have BUSY bit 0
 *	bit in ptr is 1 for busy, 0 for idle
 *	gaps in arena are merely noted as busy blocks
 *	last block of arena (pointed to by alloct) is empty and
 *	has a pointer to first
 *	idle blocks are coalesced during space search
 *
 *	a different implementation may need to redefine
 *	ALIGN, NALIGN, BLOCK, BUSY, INT
 *	where INT is integer type to which a pointer can be cast
*/
#define INT unsigned long int
#define ALIGN unsigned long int
#define NALIGN 1
#define WORD sizeof(union store)
#define BLOCK 1024	/* a multiple of WORD*/
#define BUSY 1
#define NULL 0
#define testbusy(p) ((INT)(p)&BUSY)
#define setbusy(p) (union store *)((INT)(p)|BUSY)
#define clearbusy(p) (union store *)((INT)(p)&~BUSY)

union store { union store *ptr;
	      ALIGN dummy[NALIGN];
	      int calloc;	/*calloc clears an array of integers*/
};

#if MALLOC_DEBUG
extern union store allocs[2], *allocp, *alloct, *allocx;
extern procedure os_eprintf();
#endif MALLOC_DEBUG
#endif MALLOC_H
