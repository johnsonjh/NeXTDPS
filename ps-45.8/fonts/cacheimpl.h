/*
  cacheimpl.h

Copyright (c) 1989 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Mark Francis: Tue Sep 19 12:41:24 1989
Edit History:
End Edit History.
*/

#ifndef	CACHEIMPL_H
#define	CACHEIMPL_H

typedef Links CacheHashTbl;

/* Structure used to maintain cache usage statistics */

typedef struct {
  integer	entries;
  integer	lookups;
  integer	hits;
  integer	displacements;
  integer	reuse;
  longcardinal	totService;
  } CacheStats;

/* Definition of header for each entry in a cache. */

typedef struct _t_CacheEntHdr {
  Links		hashChain;	/* must be first item in struct */
  Links		LRUChain;	/* links to LRU chain */
  int		size;		/* size of entry (units unknown) */
  int		hashId;		/* id of hash entry entry is on */
  CaTag		tag;		/* uninterpreted entry tag */
  CaData	data;		/* uninterpreted data */
  } CacheEntHdr;

/* Definition of cache header structure. */

typedef struct _t_Cache {
  PCache	next;		/* next in list of all cache headers */
  string	name;		/* a descriptive name */
  PCacheProcs	procs;		/* policy procedures */
  Card32	limit;		/* max 'size' of cache */
  Card32	curSize;	/* current size of cache */
  int		hashTblSize;	/* # buckets in hash table */
  CacheHashTbl	*hashTbl;	/* ptr to hash table */
  Links		LRUChain;	/* LRU list of all entries */
  PCacheEntHdr	tooBig;		/* ptr to single entry that won't fit */
  PCacheEntHdr	availHdrs;	/* list of available cache headers */
#if (STAGE==DEVELOP)
  CacheStats	stats;		/* usage statistics */
#endif (STAGE==DEVELOP)
  } Cache;

/* Enable collection of cache statistics only in DEVELOP versions */

#if (STAGE==DEVELOP)
#define CacheStat(code)	{ code; }
#else
#define CacheStat(code)
#endif (STAGE==DEVELOP)

#endif CACHEIMPL_H
