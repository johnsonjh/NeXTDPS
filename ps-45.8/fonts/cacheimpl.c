/*
  cacheimpl.c

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

Original version: Mark Francis: Wed Sep 13 15:41:24 1989
Edit History:
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include ORPHANS
#include PSLIB
#include LANGUAGE

#include "cache.h"
#include "cacheimpl.h"

/* Globals */

private PCache cacheChain;

/* Private procedures */

private PCacheEntHdr AllocHdr(cache)
  PCache cache;
  {
  PCacheEntHdr hdr;

  if (cache->availHdrs != NULL) {
    hdr = cache->availHdrs;
    cache->availHdrs = (PCacheEntHdr) hdr->hashChain.next;
    hdr->hashChain.next = NULL;
    }
  else
      hdr = (PCacheEntHdr) os_sureCalloc(1L, (long int) sizeof(CacheEntHdr));
  return(hdr);
  }

private procedure FreeHdr(cache, hdr)
  PCache cache;
  PCacheEntHdr hdr;
  {
  hdr->hashChain.next = (Links *) cache->availHdrs;
  cache->availHdrs = hdr;
  }

private procedure UnlinkEntry(cache, hdr)
  PCache cache;
  PCacheEntHdr hdr;
  {
  RemoveLink((Links *) &hdr->hashChain);
  RemoveLink((Links *) &hdr->LRUChain);
  }

private procedure DisposeEntry(cache, hdr)
  PCache cache;
  PCacheEntHdr hdr;
  {
  if (cache->procs->CPFreeEntry != NULL)
    (*cache->procs->CPFreeEntry)(hdr->data, true);
  FreeHdr(cache, hdr);
  }

private PCacheEntHdr Lookup(cache, tag, id)
  register PCache cache;
  register CaTag tag;
  int *id;
  {
  int (*hashIdProc)() = cache->procs->CPGenerateHashId;
  register boolean (*matchProc)() = cache->procs->CPTagsMatch;
  register PCacheEntHdr hdr, end;

  *id = (hashIdProc ? (*hashIdProc)(tag) : (int) tag) % cache->hashTblSize;
  for (hdr = (PCacheEntHdr) cache->hashTbl[*id].next,
	 end = (PCacheEntHdr) &cache->hashTbl[*id];
       (hdr != end) && 
	 !(matchProc ? (*matchProc)(tag, hdr->tag) : tag == hdr->tag);
       hdr = (PCacheEntHdr) hdr->hashChain.next);
  return(hdr != end ? hdr : NULL);
  }

private PCacheEntHdr DisplaceEntries(cache, needed)
  PCache cache;
  int *needed;
  {
#define PHdr(l) ((PCacheEntHdr) ((char*)((l).prev)-sizeof(Links)))
  PCacheEntHdr end, next, first;
  register PCacheEntHdr cur;
  register int tot;

  if ((cur = cache->tooBig) != NULL) {
    /* If entry that wouldn't fit is left over from prior lookup,
       see if it fits size requirement. If not, dispose of it now */
    cache->tooBig = NULL;
    if (cur->size >= *needed) {
      *needed = cur->size;
      return(cur);
      }
    else
      DisposeEntry(cache, cur);
    }

  if (*needed + cache->curSize <= cache->limit)
    return(NULL);

  end = (PCacheEntHdr) ((char*)&cache->LRUChain - sizeof(Links));
  for (first = cur = PHdr(cache->LRUChain), tot = 0;
       (cur != end) && (tot < *needed);
       tot += cur->size, cur = PHdr(cur->LRUChain))
    if (cur->size >= *needed) {
      CacheStat(cache->stats.displacements++);
      UnlinkEntry(cache, cur);
      cache->curSize -= cur->size;
      CacheStat( cache->stats.entries-- );
      *needed = cur->size;
      return(cur);
      }

  /* Couldn't find a big enuf entry - free the entries we just traversed */

  for (end = cur, cur = first; cur != end; cur = next) {
    CacheStat(cache->stats.displacements++);
    next = PHdr(cur->LRUChain);
    CacheStat( cache->stats.entries-- );
    UnlinkEntry(cache, cur);
    DisposeEntry(cache, cur);
    }

  cache->curSize -= tot;
  return(NULL);
  }

private procedure AddEntry(cache, hashId, tag, data, size, pHdr)
  PCache cache;
  int hashId;
  CaTag tag;
  CaData data;
  int size;
  PCacheEntHdr *pHdr;
  {
  register PCacheEntHdr hdr = *pHdr;
  PCacheEntHdr displaced;
  boolean itFits = true;

  if (hdr == NULL)
    /* Allocate a header if caller didn't provide one */
    *pHdr = hdr = AllocHdr(cache);

  hdr->tag = tag;
  hdr->data = data;
  hdr->size = size;

  /* If entry will fit, add it to head of its hash chain and LRU chain. If not,
     displace enough entries to make room for it. Special case: if
     entry is itself larger than the cache limit, put it on the 
     'tooBig' chain */

  while ((cache->curSize + size) > cache->limit) {
    if (size > cache->limit) {
      cache->tooBig = hdr;
      itFits = false;
      break;
    } else {
      int tmp = size;
      if ((displaced = DisplaceEntries(cache, &tmp)) != NULL)
	DisposeEntry(displaced);
      }
    } 

  if (itFits) {
    InsertLink((Links*) &cache->hashTbl[hashId], (Links*) &hdr->hashChain);
    InsertLink((Links*) &cache->LRUChain, (Links*) &hdr->LRUChain);
    hdr->hashId = hashId;
    cache->curSize += size;
    CacheStat( cache->stats.entries++ );
    }
  }

/* globals */

public boolean CacheLookup(cache, tag, data)
  PCache cache;
  CaTag tag;
  CaData *data;
  {
  PCacheEntHdr hdr;
  int id;
  longcardinal start;
  int size;

  CacheStat(cache->stats.lookups++);
  CacheStat(start = (longcardinal) os_clock());
  if ((hdr = Lookup(cache, tag, &id)) != NULL) {
    /* Found - return data, put on head of LRU list */
    *data = hdr->data;
    CacheStat(cache->stats.hits++);
    if (cache->LRUChain.next != &hdr->LRUChain) {
      /* Only relink if not already at head of chain */
      RemoveLink((Links *) &hdr->LRUChain);
      InsertLink((Links*) &cache->LRUChain, (Links*) &hdr->LRUChain);
      }
    CacheStat(cache->stats.totService += (longcardinal) os_clock() - start);
  } else {
    boolean (*resolveProc)() = cache->procs->CPResolveMiss;

    if (resolveProc ? (*resolveProc)(&tag, data, &size, &hdr) : false) {
      CacheStat( cache->stats.reuse += (hdr != NULL) ? 1 : 0 );
      AddEntry(cache, id, tag, *data, size, &hdr);
      CacheStat(cache->stats.totService += os_clock() - start);
      }
    }

  return(hdr != NULL);
  }

public boolean CacheAddEntry(cache, tag, data, size)
  PCache cache;
  CaTag tag;
  CaData data;
  int size;
  {
  PCacheEntHdr hdr;
  int id;

  if ((hdr = Lookup(cache, tag, &id)) == NULL) {
    AddEntry(cache, id, tag, data, size, &hdr);
    if (cache->tooBig) {
      /* If entry not added (too big), tell the caller */
      FreeHdr(cache->tooBig);
      cache->tooBig = NULL;
      return(false);
      }
  } else 
    RAISE(-1, (char *)NULL);

  return(true);
  }

public procedure CacheRemoveEntry(cache, tag)
  PCache cache;
  CaTag tag;
  {
  PCacheEntHdr hdr;
  int id;

  if ((hdr = Lookup(cache, tag, &id)) != NULL) {
    UnlinkEntry(cache, hdr); 
    DisposeEntry(cache, hdr);
    }
  }

public CaData CacheReuseEntry(cache, needed, pHandle)
  PCache cache;
  int *needed;
  PCacheEntHdr *pHandle;
  {
  CaData data = NULL;

  if ((*pHandle = DisplaceEntries(cache, needed)) != NULL) {
    /* Got one to reuse - let implementation cleanup up the
       entry but not dispose of it */
    if (cache->procs->CPFreeEntry != NULL)
      (*cache->procs->CPFreeEntry)((*pHandle)->data, false);
    data = (*pHandle)->data;
    }
  return(data);
  }

public PCache CacheCreate(name, procs, limit, buckets)
  string name;
  PCacheProcs procs;
  int limit;
  int buckets;
  {
  PCache cache;
  int b;

  cache = (PCache) os_sureCalloc(1L, (long) sizeof(Cache));
  cache->next = cacheChain;
  cacheChain = cache;
  cache->name = name;
  cache->procs = procs;
  cache->limit = limit;
  InitLink(&cache->LRUChain);
  cache->hashTblSize = buckets;
  cache->hashTbl = (CacheHashTbl *) os_sureCalloc((long) buckets, 
    (long) sizeof(CacheHashTbl));
  for (b = 0; b < buckets; b++)
    InitLink(&cache->hashTbl[b]);
  return(cache);
  }

public procedure CacheInvalidate(cache, tag, selector)
  PCache cache;
  CaTag tag;
  string selector;
  {
#define NHdr(l) ((PCacheEntHdr) ((char*)((l).next)-sizeof(Links)))
  register PCacheEntHdr hdr, next, end;
  boolean (*selectProc)() = cache->procs->CPInvalidateSelect;

  end = (PCacheEntHdr) ((char*)&cache->LRUChain - sizeof(Links));
  for (hdr = NHdr(cache->LRUChain); hdr != end; hdr = next) {
    next = NHdr(hdr->LRUChain);
    if (selectProc ? 
	(*selectProc)(tag, selector, hdr->tag, hdr->data) : 
	true) 
      {
      cache->curSize -= hdr->size;
      CacheStat( cache->stats.entries-- );
      UnlinkEntry(cache, hdr);
      DisposeEntry(cache, hdr);
      }
    }
  }

#if (STAGE==DEVELOP)
public procedure PSCacheStatistics()
  {
  StrObj name, curName;
  IntObj num;
  boolean all;
  int i = 0;
  PCache p;

  PopPRString(&name);
  all = (name.length == 0);

  for (p = cacheChain; p != NULL; p = p->next) {
    curName = MakeStr(p->name);
    if (all || (StringCompare(curName, name) == 0)) {
      i++;
      LIntObj(num, p->limit); IPush(opStk, num);
      LIntObj(num, p->curSize); IPush(opStk, num);
      LIntObj(num, p->stats.entries); IPush(opStk, num);
      LIntObj(num, p->stats.reuse); IPush(opStk, num);
      LIntObj(num, p->stats.displacements); IPush(opStk, num);
      LIntObj(num, p->stats.totService); IPush(opStk, num);
      LIntObj(num, p->stats.hits); IPush(opStk, num);
      LIntObj(num, p->stats.lookups); IPush(opStk, num);
      IPush(opStk, curName);
      }
    }

  /* Return # of caches we looked at */

  LIntObj(num, i); IPush(opStk, num);
  }

public procedure PSCacheFlush()
  {
  StrObj name, curName;
  boolean all;
  PCache p;

  PopPRString(&name);
  all = (name.length == 0);

  for (p = cacheChain; p != NULL; p = p->next) {
    curName = MakeStr(p->name);
    if (all || (StringCompare(curName, name) == 0)) {
      CacheInvalidate(p, (CaTag) NULL, NULL);
      os_bzero(&p->stats, (long int) sizeof(CacheStats));
      }
    }

  }

public procedure PSCacheInit()
  {
  StrObj name, curName;
  integer limit;
  boolean all;
  PCache p;

  limit = PSPopInteger();
  PopPRString(&name);
  all = (name.length == 0);

  for (p = cacheChain; p != NULL; p = p->next) {
    curName = MakeStr(p->name);
    if (all || (StringCompare(curName, name) == 0)) {
      CacheInvalidate(p, (CaTag) NULL, NULL);
      os_bzero(&p->stats, (long int) sizeof(CacheStats));
      p->limit = limit;
      }
    }

  }
#endif (STAGE==DEVELOP)
