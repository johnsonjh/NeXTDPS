/*
  name.c

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

Original version: Chuck Geschke: February 10, 1983
Edit History:
Scott Byer: Thu May 25 17:46:18 1989
Chuck Geschke: Fri Mar 28 08:49:40 1986
Doug Brotz: Fri May 30 17:47:32 1986
Ed Taft: Sun Dec 17 14:00:05 1989
John Gaffney: Tue Jan 15 10:43:05 1985
Don Andrews: Wed Jul 16 10:52:37 1986
Ivor Durham: Mon May  8 11:13:32 1989
Linda Gass: Wed Aug  5 16:47:42 1987
Joe Pasqua: Fri Jan  6 14:56:44 1989
Jim Sandman: Wed Mar  9 16:53:30 1988
Perry Caro: Mon Nov  7 16:39:24 1988
End Edit History.
*/


#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include GC
#include LANGUAGE
#include PSLIB
#include RECYCLER
#include VM

#include "name.h"

extern procedure PurgeCI(/* PNameEntry pne */);
  /* This is a circular reference to the fonts package	*/

public procedure NameToPString(name, pob)
  NameObj name;  register PObject pob;
{
register PNameEntry nep = name.val.nmval;
LStrObj(*pob, nep->strLen, nep->str);
pob->level = 0; /* since name strings are always in shared VM */
pob->tag = name.tag;
pob->access = rAccess;
}  /* end of NameToPString */

#if STAGE==DEVELOP
private integer Nsearch, Ncomp, Maxprobe;
  /* These statistics are gathered across all contexts.	*/

private procedure PSNProbeStats()
{
  integer Nused = 0,i; PNameEntry *ppne;
  PushInteger(Nsearch);
  for (ppne = & rootShared->vm.Shared.nameTable.val.namearrayval->nmEntry[0],
       i = rootShared->vm.Shared.nameTable.val.namearrayval->length; --i >= 0;)
    if (*ppne++ != NIL) Nused++;
  PushInteger(Nused);
  PushInteger(Ncomp);
  PushInteger(Maxprobe);
}
#endif STAGE==DEVELOP

public procedure NameIndexObj(nameindex, pnobj)
  register cardinal nameindex;		/* name object's index */
  register PNameObj pnobj;		/* returned name object */
/*
 *	This function returns the name object corresponding to 
 *	the argument name index.
 */
{
  register PNameEntry pne;
  for (pne = rootShared->vm.Shared.nameTable.val.namearrayval->
       nmEntry[nameindex & rootShared->vm.Shared.hashindexmask];
       pne != NIL; pne = pne->link)
    {
    if (pne->nameindex == nameindex)
      {LNameObj(*pnobj, pne); return;}
    }

CantHappen();
/*NOTREACHED*/
}  /* end of NameIndexObj */

private procedure newstn(str, len, h, pnobj)
string str;
cardinal len;
longcardinal h;
register PNameObj pnobj;
{
register PNameEntry newpne, pne;
PNameEntry *pPrevLink;
longcardinal nameindex;

if (len > MAXnameLength) LimitCheck();
AllocPName(len, pnobj);
newpne = pnobj->val.nmval;
os_bcopy((char *) str, (char *) newpne->str, (integer) len);
DebugAssert(newpne->kvloc == NIL && newpne->ts.stamp == 0 &&
            newpne->vec == 0 && newpne->dict == NIL && newpne->ncilink == 0);

/* Find smallest unused nameindex in hash chain and insert new entry so as
   to maintain ascending order of nameindex values. Maintaining them in
   order is solely for ease of finding unused ones. */
nameindex = h;
pPrevLink = & rootShared->vm.Shared.nameTable.val.namearrayval->nmEntry[h];
for (pne = *pPrevLink; pne != NIL; pne = pne->link)
  {
  if (nameindex < pne->nameindex) break;
  nameindex += 1 << rootShared->vm.Shared.hashindexfield;
  pPrevLink = & pne->link;
  }
*pPrevLink = pnobj->val.nmval;
newpne->link = pne;
if (nameindex > MAXCard16)
  /* If we run out of nameindex values, assign the largest possible one.
     This is too large to represent in a packedarray, so it will never
     be referenced. */
  nameindex = (~ rootShared->vm.Shared.hashindexmask) + h;
newpne->nameindex = nameindex;
}  /* end of newstn */

public procedure FastName(str, strlen, pnobj)
string str;
cardinal strlen;
register PNameObj pnobj;
{
register string s, s2;
register integer len;
register longcardinal h;
register PNameEntry pne;
#if STAGE==DEVELOP
integer i = 0, sc = 0;
Nsearch++;
#endif STAGE==DEVELOP
/* Hash the string. This implementation is tuned for speed and good
   probe distribution assuming that characters are alphabetic (0x41 to
   0x72) and that the hash table is ~ 2**10 long. However, if these
   assumptions are violated, it will still work, just more slowly. */
s = str;
len = strlen;
switch (len)
  {
  case 0:
    h = 0; break;
  case 1:
    h = *s << 4;
    break;
  case 2:
    h = *s++; h <<= 2;
    h ^= *s;
    break;
  case 3:
    h = *s++; h <<= 2;
    h ^= *s++; h <<= 2;
    h ^= *s;
    break;
  case 4:
    h = *s++; h <<= 2;
    h ^= *s++; h <<= 2;
    h ^= *s++; h <<= 2;
    h ^= *s;
    break;
  case 5:
    h = *s++; h <<= 2;
    h ^= *s++; h <<= 2;
    h ^= *s++; h <<= 2;
    h ^= *s++; h <<= 2;
    h ^= *s;
    break;
  default:
    h = *s++; h <<= 2;
    h ^= *s++; h <<= 2;
    h ^= *s++; h <<= 2;
    s += len-5;
    h ^= *s++; h <<= 2;
    h ^= *s;
    h ^= len;
    break;
  }
h = ((h >> 10) ^ h) & rootShared->vm.Shared.hashindexmask;
len = strlen;
for (pne = rootShared->vm.Shared.nameTable.val.namearrayval->nmEntry[h];
     pne != NIL;
     pne = pne->link)
  {
#if STAGE==DEVELOP
  i++;
#endif STAGE==DEVELOP
  if (pne->strLen == len)
    {
#if STAGE==DEVELOP
    sc++;
#endif STAGE==DEVELOP
    s = str;
    s2 = pne->str;
    if (len != 0)
      {
      do {if (*(s++) != *(s2++)) {len = strlen; goto cont;}}
	while (--len != 0);
      }
    /* matched; construct and return name object */
    XNameObj(*pnobj, pne);
    goto ret;
    }
 cont:;
  }

/* not found; create new name */
newstn(str, strlen, h, pnobj);
ret: ;
#if STAGE==DEVELOP
if (i != 0) {Ncomp += sc; Maxprobe = MAX(Maxprobe,i+1);}
#endif STAGE==DEVELOP
}  /* end of FastName */

public procedure StrToName(so, pno)  StrObj so;  PObject pno;
{
if ((so.access & rAccess) == 0) InvlAccess();
FastName(so.val.strval, so.length, pno);
pno->tag = so.tag;
}  /* end of StrToName */

public procedure MakePName(str, pnobj)  char *str;  PNameObj pnobj;
{FastName((string)str, (cardinal)StrLen(str), pnobj);}


public procedure AllocPNameArray(length, pObj)
  Card16 length; PNameArrayObj pObj;
{
  AllocGenericObject (objNameArray,
		sizeof (NameArrayBody) + (length - 1) * sizeof (PNameEntry),
		      pObj);
  pObj->val.namearrayval->length = length;
  os_bzero(
    (char *)&(pObj->val.namearrayval->nmEntry[0]),
    (long int)(length * sizeof(PNameEntry)));
}

private procedure FinalizeNames(clientData, info)
RefAny clientData;
GC_Info info;
/* A garbage collection trace has completed. We must go	*/
/* thru the name cache, find any name entries that have	*/
/* been freed, and remove them from the cache. This may	*/
/* cause some finalization on the font cache (ci items)	*/
{
  integer i;
  PNameArrayBody body;
  register PNameEntry cur, *ptrToPNameEntry, *prev;

  if (GC_GetCollectionType(info) != sharedVM) return;
  
  body = rootShared->vm.Shared.nameTable.val.namearrayval;

  for (/* FOR each item in the name table DO	*/
    i = body->length - 1, ptrToPNameEntry = body->nmEntry;
    i >= 0; i--, ptrToPNameEntry++)
    {
    for (/* FOR each name entry in the chain DO	*/
      cur = *ptrToPNameEntry, prev = ptrToPNameEntry;
      cur != NIL; cur = cur->link)
      {
      if (GC_WasNECollected(cur, info))
        {
        *prev = cur->link;
        PurgeCI(cur);
        }
      else
        prev = &(cur->link);
      }	/* FOR each name entry in the chain DO	*/
    }	/* FOR each item in the name table DO	*/
}

#if STAGE==DEVELOP
public procedure PSDumpNames()
/*
 *	This procedure dumps the name table to the standard output,
 *	including all name entries linked to each hash table entry.
 */
{ integer i,j;				/* loop variables */
  integer entrycount;			/* number of entries linked */
  PNameEntry pne;			/* ptr to name entry */
  StrObj so;
#define MAXEntries 10			/* record [0...10] entries */
  integer histogram[MAXEntries+1];	/* entry count histogram */

  for ( i = 0; i <= MAXEntries; i++ )	/* clear histogram */
    histogram[i] = 0;
  os_printf("\nName Table Dump:\n\n");
  for ( i = 0; i < rootShared->vm.Shared.nameTable.val.namearrayval->length; i++ )
  { entrycount = 0;			/* clear counter */
    os_printf("%d:",i);			/* print hash table entry number */
    for (pne = rootShared->vm.Shared.nameTable.val.namearrayval->nmEntry[i];
         pne != NIL; pne = pne->link)
    { 
      os_printf(" ");			/* blank separator */
      LStrObj(so, pne->strLen, pne->str);
      PrintSOP(&so);			/* print string object (name) */      
      entrycount++;			/* bump entry count */
    } 
    histogram[MIN(entrycount,MAXEntries)]++;    /* record links */
    os_printf("\n");			/* end of line */
  }
  os_printf("\n\nEntry Count Histogram\n\n");	/* follow up with histogram */
  for ( i = 0; i <= MAXEntries; i++ )
  { os_printf("%3d: %4d ",i,histogram[i]);		/* print histogram */
    for ( j = 0; j < histogram[i]; j += 10 )	/* print stars (1 per 10) */
      os_printf("*");
    os_printf("\n");
  }
}
#endif STAGE==DEVELOP


/* System and User Name Tables */

private procedure PutInNameMap(pNAObj, index, name, expand)
  PNameArrayObj pNAObj; Card16 index; NameObj name; boolean expand;
{
  PNameEntry *ppne;
  PNameArrayBody newpna;
  integer oldLen, newLen, size;

  oldLen = (pNAObj->type == nullObj)? 0 : pNAObj->val.namearrayval->length;
  if (oldLen <= index)
    {
    if (!expand) RangeCheck();
    if (index >= maxNameTableLength) LimitCheck();
    newLen = oldLen * 2;
    if (newLen <= index) newLen = index * 2;
    if (newLen < minNameTableLength) newLen = minNameTableLength;
    if (newLen > maxNameTableLength) newLen = maxNameTableLength;
    size = sizeof(NameArrayBody) + (newLen-1)*sizeof(PNameEntry);

    if (oldLen == 0)
      {
      newpna = (PNameArrayBody) os_malloc(size);
      if (newpna == NIL) LimitCheck();
      os_bzero((char *) newpna, size);
      newpna->header.type = objNameArray;
      }
    else
      {
      newpna = (PNameArrayBody) os_realloc(
        (char *)(pNAObj->val.namearrayval), size);
      if (newpna == NIL) LimitCheck();
      os_bzero(
        (char *)newpna + newpna->header.length,
        (long int)(size - newpna->header.length));
      }

    newpna->header.length = size;
    newpna->length = newLen;
    LGenericObj(*pNAObj, objNameArray, (PGenericBody)newpna);
    }

  ppne = & (pNAObj->val.namearrayval->nmEntry[index]);
  if (*ppne != NIL && *ppne != name.val.nmval) InvlAccess();
  *ppne = name.val.nmval;
}

public procedure PSDefUserName() /* index name defineusername => -- */
{
  NameObj name;
  Card16 index;

  PopP(&name);
  if (name.type != nameObj) TypeCheck();
  index = PopCardinal();
  PutInNameMap(& rootPrivate->nameMap, index, name, true);
}

#if VMINIT
private procedure PSSysNameMap() /* int systemnamemap => -- */
{
  if (rootShared->nameMap.type != nullObj) Undefined();
  if (! CurrentShared()) InvlAccess();
  AllocPNameArray(PopCardinal(), &rootShared->nameMap);
}

private procedure PSDefSysName() /* index name definesystemname => -- */
{
  NameObj name;
  Card16 index;

  PopP(&name);
  if (name.type != nameObj) TypeCheck();
  index = PopCardinal();
  PutInNameMap(& rootShared->nameMap, index, name, false);
}
#endif VMINIT

/*	----> Garbage Collector Support <----	*/

#define ChunkSize 20	/* ARBITRARY	*/
typedef struct _NameChunk {
  NameObj names[ChunkSize];
  int firstFree;
  struct _NameChunk *next;
  } NameChunk;

private NameChunk *firstChunk;

private procedure AddToGCTable(namePtr)
PNameObj namePtr;
{
  register NameChunk *curChunk = firstChunk;
  
  while (curChunk->firstFree == ChunkSize)
    {
    if (curChunk->next == NIL)
      {
      curChunk->next = (NameChunk *)os_sureMalloc(
        (long int)sizeof(NameChunk));
      curChunk = curChunk->next;
      curChunk->firstFree = 0;
      curChunk->next = NIL;
      break;
      }
    else curChunk = curChunk->next;
    }
  curChunk->names[curChunk->firstFree++] = *namePtr;
}

private procedure PushSharedNames(clientData, info)
RefAny clientData;
GC_Info info;
{
  register int i;
  register NameChunk *curChunk = firstChunk;
  
  for (curChunk = firstChunk; curChunk != NIL; curChunk = curChunk->next)
    {
    for (i = curChunk->firstFree - 1; i >= 0; i--)
      GC_Push(info, &(curChunk->names[i]));
    }
}

public procedure MakeStaticPName(str, pnobj)
/* Like MakePName except the resulting name is kept in	*/
/* a table and passed to the GC for shared collections.	*/
char *str;
PNameObj pnobj;
{
  FastName((string)str, (cardinal)StrLen(str), pnobj);
  AddToGCTable(pnobj);
}

/*	----> End Garbage Collector Support <----	*/

public procedure DestroyNameMap ()
{
  if (rootPrivate->nameMap.val.namearrayval != NIL)
    os_free((char *)rootPrivate->nameMap.val.namearrayval);
  LNullObj(rootPrivate->nameMap);
}

public procedure NameInit(reason)  InitReason reason;
{
switch (reason)
  {
  case init:
    firstChunk = (NameChunk *)os_sureMalloc((long int)sizeof(NameChunk));
    firstChunk->firstFree = 0;
    firstChunk->next = NIL;
    GC_RegisterFinalizeProc(FinalizeNames, (RefAny)NIL);
    GC_RgstSharedRootsProc(PushSharedNames, (RefAny)NIL);
    break;
  case romreg:
#if VMINIT
    if (vmShared->wholeCloth) {
      Begin(rootShared->vm.Shared.internalDict);
      RgstExplicit("systemnamemap", PSSysNameMap);
      RgstExplicit("definesystemname", PSDefSysName);
      End();
    }
#endif VMINIT
#if STAGE==DEVELOP
    if (vSTAGE==DEVELOP)
      {
      RgstExplicit("nprobestats", PSNProbeStats);
      RgstExplicit("dumpnames", PSDumpNames);
      }
#endif STAGE==DEVELOP
    break;
  }
}
