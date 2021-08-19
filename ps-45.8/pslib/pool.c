/*
  pool.c

Copyright (c) 1987 Adobe Systems Incorporated.
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
Ivor Durham: Tue Aug 16 10:25:45 1988
Paul Rovner: Tuesday, December 29, 1987 12:36:48 PM
Joe Pasqua: Mon Jan  9 16:36:34 1989
End Edit History.

	Pools -- storage pools for fixed-size objects
	Copyright 1987 -- NeXT, Inc.

	This module defines a simple mechanism for
	fixed-size objects which are frequently
	allocated and freed.  A Pool is a buffer for
	free objects.  One of these free objects
	can be obtained by calling NewElement; an
	unused object can be returned by calling
	FreeElement.  The elements are allocated in
	chunks whose size is called the hystersis
	of the pool; this implementation never frees
	any storage until FreePool is called, which
	destroys the entire storage pool (even any
	currently allocated elements!).  An absolute
	maximum for the number of elements can be
	set in NewPool; no more chunks than the
	number needed to store that many elements
	will ever be allocated.  If an attempt is
	made to allocate more, _LimitCheck will be
	called.

	Created: Leo Hourvitz 20Apr87
	Modified: 21Sep87 Added absolute max
	Paul Rovner: Tuesday, December 29, 1987 12:36:48 PM
	  check for malloc returning NULL in NewPool

	GrowPool
	allocates another pool chunk into the pool chunk list
	of the given pool, makes the pool chunk be a terminated
	list of null elements, and returns a pointer to the head
	of that list.  pool->free shold be NULL whenever GrowPool
	is called.
	NewPool
	allocates a new, minimum-length pool.
	   with this pool.  So, free all the chunks in
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include EXCEPT
#include PSLIB

typedef struct _t_PoolElement {
  struct _t_PoolElement *next;
} PoolElement;

typedef struct _t_PoolChunk {
  struct _t_PoolChunk *next;
} PoolChunk;

typedef struct _t_Pool {
  int     elementSize;		/* Size of individual elements */
  int     chunkSize;		/* Number of elements per chunk */
  int     numChunks;		/* Number of chunks so far */
  int     maxChunks;		/* Absolute maximum on chunks */
  PoolChunk *chunks;		/* Linked list of all chunks */
  PoolElement *free;		/* Linked list of free elements */
} PoolRec;

private PoolElement *GrowPool(pool)
  Pool pool;
 /*
   GrowPool allocates another pool chunk into the pool chunk list
   of the given pool, makes the pool chunk be a terminated
   list of null elements, and returns a pointer to the head
   of that list.  pool->free shold be NULL whenever GrowPool
   is called.
 */
{
  PoolElement *element,
         *lastElement;
  PoolChunk *chunk;
  int     i;

  if (pool->numChunks++ >= pool->maxChunks)
    PSLimitCheck();

  DebugAssert ((pool->chunkSize) && (pool->elementSize));

  chunk = (PoolChunk *) os_calloc(
    (long int)(sizeof(PoolChunk) + (pool->elementSize * pool->chunkSize)),
    (long int)1);

  if (chunk == NIL)
    PSLimitCheck();

  /* Insert chunk into pool's lists */

  chunk->next = pool->chunks;
  pool->chunks = chunk;

  /* Insert new elements into free list */

  element = (PoolElement *) (chunk + 1);
  lastElement = pool->free;

  for (i = 0; i < pool->chunkSize; i++) {
    element->next = lastElement;
    lastElement = element;
    element = (PoolElement *) (((char *) element) + pool->elementSize);
  }

  return (lastElement);
}				/* GrowPool */

public Pool os_newpool(elementSize, elementsPerChunk, maxElements)
  int	elementSize;		/* in bytes */
  int	elementsPerChunk;	/* -1 or 0 means default */
  int	maxElements; 		/* -1 or 0 is no limit */
 /*
   NewPool allocates a new, minimum-length pool.
  */
{
#define	DEFAULTHYST	10
  register Pool pool;

  pool = (Pool) os_malloc((long int)sizeof(PoolRec));

  if (pool == NIL)
    PSLimitCheck();

  pool->elementSize =
    (elementSize >= sizeof (PoolElement) ? elementSize : sizeof (PoolElement));

  if ((elementsPerChunk == 0) || (elementsPerChunk == -1))
    elementsPerChunk = DEFAULTHYST;
  else if (elementsPerChunk < 0)
    PSRangeCheck();

  if ((maxElements == -1) || (maxElements == 0))
    pool->maxChunks = MAXInt32;
  else if (maxElements > 0)
    pool->maxChunks = 1 + (maxElements / elementsPerChunk);
  else
    PSRangeCheck();

  pool->chunkSize = elementsPerChunk;
  pool->free = NIL;
  pool->chunks = NIL;
  pool->numChunks = 0;
  pool->free = GrowPool (pool);

  return (pool);
}				/* os_newpool */

public procedure os_freepool (pool)
  Pool	pool;
{
  PoolChunk *chunk;

  /*
   * The idea is to free all the storge associated with this pool.  So, free
   * all the chunks in the chunk list
   */

  while (pool->chunks != NIL) {
    chunk = pool->chunks;
    pool->chunks = chunk->next;
    os_free((char *)chunk);
  }

  os_free((char *)pool);	/* And now the pool itself */
}				/* os_freepool */

public char *os_newelement(pool)
  register Pool	pool;
{
  register PoolElement *el;

  DebugAssert ((pool->chunkSize != 0) && (pool->elementSize != 0));

  if ((el = pool->free) == NIL)
    el = GrowPool (pool);

  pool->free = el->next;

  return ((char *) el);
}				/* os_newelement */

public procedure os_freeelement(pool,el)
  register Pool	pool;
  register char	*el;
{
  DebugAssert ((pool->chunkSize != 0) && (pool->elementSize != 0));
  ((PoolElement *)el)->next = pool->free;
  pool->free = ((PoolElement *)el);
}				/* os_freeelement */
		
