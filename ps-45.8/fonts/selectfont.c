/*
  selectfont.c

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
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
Ed Taft: Fri Jul 29 09:36:07 1988
Ivor Durham: Sun Aug 14 14:00:13 1988
Jim Sandman: Mon Apr 10 10:33:33 1989
Perry Caro: Wed Nov  9 14:16:42 1988
Joe Pasqua: Tue Feb 28 13:43:07 1989
Bill Bilodeau: Fri Jan 20 16:45:41 PST 1989
End Edit History.
*/

#include PACKAGE_SPECS

#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include DEVICE
#include GC
#include GRAPHICS
#include LANGUAGE
#include ORPHANS
#include PSLIB
#include STREAM
#include VM

#include "fontcache.h"
#include "fontshow.h"
#include "fontdata.h"
#include "fontsnames.h"

/* globals */

#define MaxMCount 20

public PSFCache sfCache;
public PSFCEntry sfcFreeList,	/* -> head of free list */
  sfcRover;			/* -> next entry to displace */

#define HashSFC(key, mtx) \
  ((Card32) ((key).val.nmval + *(PCard32) &(mtx)->a + \
	     rootPrivate->spaceID.stamp) % szSelectFontHash)


public boolean SearchSelectFont(key, mtx, isScaleFont, pDict)
  Object key;
  register PMtx mtx;
  register boolean isScaleFont;
  PDictObj pDict;
{
  register PSFCEntry psf;
  register PMTItem mp;
  PSFCEntry *phead, *pprev;

  if (key.type != nameObj) return false;
#if STAGE==DEVELOP
  sfCache->searches++;
#endif STAGE==DEVELOP
  phead = pprev = &sfCache->hashTable[HashSFC(key, mtx)];
  for (psf = *pprev; psf != NIL; pprev = &psf->link, psf = psf->link)
    if (key.val.nmval == psf->key &&
	mtx->a == psf->mtx.a &&
	((isScaleFont)? psf->isScaleFont :
	 mtx->b == psf->mtx.b &&
	 mtx->c == psf->mtx.c && mtx->d == psf->mtx.d &&
	 mtx->tx == psf->mtx.tx && mtx->ty == psf->mtx.ty) &&
	(psf->shared || ! vmCurrent->shared))
      {
      mp = &MT[psf->mid];
      DebugAssert(mp->mfmid && mp->type != freemem);
      if (mp->values.mf.dictSpace.stamp == rootPrivate->spaceID.stamp)
        {
#if STAGE==DEVELOP
        sfCache->hits++;
#endif STAGE==DEVELOP
        if (pprev != phead)
          {
#if STAGE==DEVELOP
	  sfCache->reorders++;
#endif STAGE==DEVELOP
	  *pprev = psf->link;
	  psf->link = *phead;
	  *phead = psf;
	  }
        *pDict = mp->values.mf.mfdict;
	DecrSetMIDAge(psf->mid);
        return true;
	}
      }
  return false;
}


private procedure FreeSelectFont(psf)
  register PSFCEntry psf;
{
  register PSFCEntry psfd, *pprev;

  pprev = &sfCache->hashTable[psf->hash];
  for (psfd = *pprev; psfd != psf; pprev = &psfd->link, psfd = psfd->link)
    if (psfd == NIL) CantHappen();
  *pprev = psf->link;
  psf->link = sfcFreeList;
  sfcFreeList = psf;
}


public procedure InsertSelectFont(key, mtx, shared, mid)
  Object key;
  register PMtx mtx;
  boolean shared;
  MID mid;
{
  register PSFCEntry psf;
  PSFCEntry *pprev;
  PMTItem mp = &MT[mid];

  if (key.type != nameObj) return;
  Assert(mp->mfmid && mp->type != freemem);
  if ((psf = sfcFreeList) == NIL)
    {
    psf = sfcRover;
    if (--sfcRover < &sfCache->entries[0]) sfcRover += szSelectFontCache;
    FreeSelectFont(psf);
    DebugAssert(psf == sfcFreeList);
    }
  sfcFreeList = psf->link;

  psf->key = key.val.nmval;
  psf->mid = mid;
  psf->shared = shared;
  psf->mtx = *mtx;
  psf->isScaleFont = psf->mtx.d == psf->mtx.a &&
    psf->mtx.b == 0 && psf->mtx.c == 0 &&
    psf->mtx.tx == 0 && psf->mtx.ty == 0;
  psf->hash = HashSFC(key, mtx);
  pprev = &sfCache->hashTable[psf->hash];
  psf->link = *pprev;
  *pprev = psf;
}


public procedure PurgeSFForKey(key)
  Object key;
/* Removes selectfont cache entries for specified key in current space */
{
  register PSFCEntry psf;
  register PMTItem mp;
  register PSFCEntry *phead;
  PSFCEntry psfd;
  PVMRoot root = (CurrentShared() ? rootShared : rootPrivate);

  if (key.type != nameObj) return;
  for (phead = &sfCache->hashTable[0];
       phead < &sfCache->hashTable[szSelectFontHash]; phead++)
    for (psf = *phead; psf != NIL; )
      {
      mp = &MT[psf->mid];
      DebugAssert(mp->mfmid && mp->type != freemem);
      if (psf->key == key.val.nmval &&
          mp->values.mf.dictSpace.stamp == root->spaceID.stamp)
	{
	psfd = psf;
	psf = psf->link;
	FreeSelectFont(psfd);
	}
      else psf = psf->link;
      }
}

public procedure PurgeSFForMID(mid)
  register MID mid;
/* Removes selectfont cache entries for specified MID */
{
  register PSFCEntry psf;
  PSFCEntry *phead;
  PSFCEntry psfd;

  for (phead = &sfCache->hashTable[0];
       phead < &sfCache->hashTable[szSelectFontHash]; phead++)
    for (psf = *phead; psf != NIL; )
      if (psf->mid == mid)
	{
	psfd = psf;
	psf = psf->link;
	FreeSelectFont(psfd);
	}
      else psf = psf->link;
}


public procedure InitSFCache(reason) InitReason reason;
{
  PSFCEntry psf;
  sfCache = (PSFCache) NEW(1, sizeof(SFCache));
  sfcFreeList = NIL;
  for (psf = &sfCache->entries[0];
       psf < &sfCache->entries[szSelectFontCache]; psf++)
    {psf->link = sfcFreeList; sfcFreeList = psf;}
  sfcRover = sfcFreeList;
}

