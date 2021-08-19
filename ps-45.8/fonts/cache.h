/*
  cache.h

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

Original version: Mark Francis: Tue Sep 26 12:41:24 1989
Edit History:
End Edit History.
*/

/*--------------------------------------------------------------------

  These notes describe the use of the generic  cache mechanism.
Primitives  provided  by  the  mechanism  include lookup entry, add
entry, remove single entry, and remove multiple entries.   Entries
within  the cache  are organized by hash chains, and all entries occupy
some position on an LRU chain.  The cache implementor declares an
instance  of  the  cache  header object,  specifying  the  number  of
hash buckets, the maximum cache size, and policy procedures to
implement hash index generation, cache tag matching, cache limit
checking,   entry   displacement,   fill   after  miss,  and  selective
invalidation. Procedures  to  implement  these  policies  are
optional;  if  a particular policy is not specified, the mechanism
provides a default policy.

  Whenever a cache entry is found via the CacheLookup primitive, it is
moved to the  head  or  "most  recently  used"  position  of  the
cache's  LRU   chain.  Infrequently used entries are subject to
displacement when the cache size limit is exceeded.

  The mechanism allows the implementor to control the size of the cache
in  any manner  desired.  Examples are a simple count of current
cache entries compared to some maximum number, or the total size (in
whatever units) of  the  contents of  all  cache  entries compared to
some maximum size.  When the addition of an entry surpasses the
expressed maximum, one or more entries  will  be  displaced according
to  the  LRU policy. An entry whose 'size' exceeds the maximum cache
limit will not be added to the cache.

  When an entry is removed from the cache  by  the  LRU  mechanism,
the  cache implementation  will be given a chance to cleanup any data
associated with that entry via the CPFreeEntry callback procedure.

  The primitives provided are:

   - PCache CacheCreate(string name;  PCacheProcs  proc;  int  limit; int
     buckets)  -  create  a  cache structure with the given name and
     procs (policy procedures). The number of hash buckets and  the
     size  limit are also specified.

   - void CacheDestroy(PCache cache) - not yet implemented (no need for it
     in fontrun). It should remove each entry from  the  specified
     cache, calling the policy procedure CPFreeEntry for each one. The
     hash table memory and the actual cache header structure are
     discarded.

   - boolean CacheLookup(PCache cache; CaTag tag; CaData *data) - look for
     an  entry  with  the  specified  tag, returning the contents for
     that entry in *data. If the lookup is  successful,  the  function
     returns true.  If  a CPResolveMiss policy procedure exists, it
     will be called in the event of a cache miss to attempt to add an
     entry to the  cache that  matches  the requested tag.  Thus, the
     cache may be implemented so an  unsuccessful  CacheLookup  call
     means  a  serious  error  has occurred.

   - boolean  CacheAddEntry(PCache  cache; CaTag tag; CaData contents; int
     size) - allocate a new cache entry with the given tag  and
     contents.  An  exception  will  occur  if  an  entry  with the
     given tag already exists. If the 'size' of the entry by itself
     exceeds the cache limit, no entry will be made and the function
     will return false.

   - void CacheReuseEntry(PCache cache; int needed; PCacheEntHdr handle) -
     called only from the PCResolveMiss  policy  procedure,  this
     routine attempts  to  return a handle to an existing entry that
     satisfies the specified 'needed' parameter. As a  side  effect,
     it  will  displace existing  entries  if the cache cannot contain
     the new entry (of size 'needed') without exceeding the  cache
     limit.  The  intent  of  this routine  is  to  allow  the cache
     implementor to reuse memory already allocated rather than
     continually freeing and reallocating chunks  of memory when
     entries are displaced.

   - void  CacheRemoveEntry(PCache  cache;  CaTag  tag) - remove any entry
     with the given tag from  the  cache,  deallocating  its  cache
     entry structure. It is ok if no such entry exists.

   - void  CacheInvalidate(PCache  cache;  CaTag  tag;  CaData selector) -
     remove any entries that meet the selection criteria  implied  by
     tag and  selector;  see  the description of the CPInvalidateSelect
     policy procedure for more information.

  The Cache Policy procedures are:

   - int CPGenerateHashId(CaTag tag) - return a hash index  based  on the
     given tag. If no such procedure is provided, the tag is assumed to
     be without underlying structure and a hash index is generated by
     taking "tag MOD hash table size".

   - boolean  CPTagMatch(CaTag target, candidate) - compare the target tag
     to the  candidate  tag,  returning  true  if  they  match  and
     false otherwise.  If no such procedure is provided, the tags are
     treated as ints and a simple comparison is performed.

   - boolean  CPResolveMiss(CaTag  tag;  CaData  *contents;   int *size;
     PCacheEntHdr *handle) - called when CacheLookup fails. Given the
     tag, this procedure should attempt to supply the contents
     associated  with that  tag, returning true if successful.  The
     size of the entry and a handle for a reused  entry  must  be
     returned.    If  no  reuse  was possible, a NULL handle should be
     returned.  If successful, the cache mechanism will add an entry to
     the cache with this tag and  contents.  If  no  such  procedure
     is  provided,  the  cache mechanism will not attempt to
     automatically create an entry to satisfy  an  unsuccessful cache
     lookup.

   - void  CPFreeEntry(CaData  contents;  boolean  dispose)  - provides a
     chance to perform cleanup actions on the contents of an entry that
     is about to be removed from the cache or reused with different
     contents.  If dispose is true, the entry will be removed and the
     policy  routine should  dispose  of  any dynamically allocated
     data pointed to by the entry. If dispose is false, it means the
     cache  mechanism  wants  to reuse  all  memory  associated  with
     the  entry.    For instance, if 'contents' is a pointer to some
     structure and dispose  is  true,  the structure  may be freed.  If
     no policy is provided here, the contents of a cache entry is
     assumed to be simply an integer.

   - boolean  CPInvalidateSelect(CaTag  target;  CaData  selector; CaTag
     candidate;  CaData  contents)  - this function allows the cache to
     be purged of all entries that  meet  certain  criteria.
     CacheInvalidate enumerates  all  entries, calling this policy
     procedure for each one.  The target tag and selector can be used
     in any means to evaluate  the entry  with  tag 'candidate' and the
     specified contents. If the entry should be purged from the cache,
     this function should return true. If no  such  procedure  is
     provided, the CacheInvalidate primitive is a noop.

---------------------------------------------------------------------*/

#ifndef	CACHE_H
#define	CACHE_H

/* Opaque pointers to implementor specified cache tags and
   cache data structures. Uninterpreted by the cache mechanism. */

typedef char *CaTag;
typedef char *CaData;

/* Opaque pointers to the data structures representing the cache
   and cache entry headers. Real definitions in cacheimpl.h. Cache
   implementors don't need to know what's in these structures. */

typedef struct _t_Cache *PCache;
typedef struct _t_CacheEntHdr *PCacheEntHdr;

/* Definition of structure containing the set of policy procedures 
   supplied by the implementor. See discussion above for details and
   parameters. */

typedef struct _t_CacheProcs {
  int		(*CPGenerateHashId)();
  boolean	(*CPTagsMatch)();
  boolean	(*CPResolveMiss)();
  void		(*CPFreeEntry)();
  boolean	(*CPInvalidateSelect)();
  } CacheProcs, *PCacheProcs;


/* Exported procedures - see discussion above for details and
   parameters. */

#define _cache extern

_cache PCache		CacheCreate();
_cache procedure	CacheDestroy();
_cache boolean		CacheLookup();
_cache boolean		CacheAddEntry();
_cache procedure	CacheRemoveEntry();
_cache procedure	CacheInvalidate();
_cache CaData		CacheReuseEntry();
#if (STAGE==DEVELOP)

/* These procedure implement operators present only in DEVELOP
   versions */

_cache procedure	PSCacheStatistics();
_cache procedure	PSCacheFlush();
_cache procedure	PSCacheInit();
#endif

#endif CACHE_H
