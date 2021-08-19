/*
  hostdict.c

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

Original version:
Edit History:
Ivor Durham: Wed Sep 28 13:50:33 1988
Jim Sandman: Tue Nov  8 11:16:57 1988
Joe Pasqua: Tue Jan 10 14:34:52 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include PSLIB
#include EXCEPT

#define free(ptr) os_free(ptr)
#define strcpy(a, b) os_strcpy(a, b)
#define strcmp(a, b) os_strcmp(a, b)
#define strlen(a) os_strlen(a)

#include "hostdict.h"

int hashHostDict(/* HostDict *, char * */);
void hostDictPurge(/* HostDict * */);

static unsigned long timestamp;
#define TIME() ++timestamp;

HostDict *newHostDict(length, strings, copy, purge)
  int length;
  int strings;
  int copy;
  void (*purge)();
  {
  HostDict *hostDict;
  int i;
	
  hostDict = (HostDict *) os_sureMalloc(
    (integer)(sizeof(HostDict) + length * sizeof(HostDictEntry *)));
  hostDict->length = length;
  hostDict->items = 0;
  hostDict->strings = strings;
  hostDict->copy = copy;
  hostDict->purge = purge;
  for (i = 0; i < hostDict->length; i++)
    hostDict->entries[i] = 0L;

  return hostDict;
  }

void disposeHostDict(hostDict)
  HostDict *hostDict;
  {
  int i;
  HostDictEntry *item;
  HostDictEntry *next;
  
  for (i = 0; i < hostDict->length; i++)
    {
    for (item = hostDict->entries[i]; item;)
      {
      next = item->next;
      if (hostDict->strings && hostDict->copy)
        free((char *) item->key);
      free((char *) item);
      item = next;
      }
    }
  free((char *) hostDict);
  }

void defHostDict(hostDict, key, value)
  HostDict *hostDict;
  char *key;
  char *value;
  {
  int i;
  HostDictEntry *item;
  
  i = hashHostDict(hostDict, key);
  for (
    item = hostDict->entries[i];
    item && (hostDict->strings ? strcmp(item->key, key) : item->key != key);
    item = item->next
    )
    ;
  if (!item)
    {
    item = (HostDictEntry *) os_sureMalloc((integer)sizeof(HostDictEntry));
    if (hostDict->strings && hostDict->copy)
      {
      item->key = os_sureMalloc((integer)(strlen(key) + 1));
      strcpy(item->key, key);
      }
    else
      item->key = key;
    item->next = hostDict->entries[i];
    hostDict->entries[i] = item;
    hostDict->items++;
    }
  item->value = value;
  item->time = TIME();
  hostDictPurge(hostDict);
  }

char *getHostDict(hostDict, key)
  HostDict *hostDict;
  char *key;
  {
  int i;
  HostDictEntry *last;
  HostDictEntry *item;
  
  i = hashHostDict(hostDict, key);
  for (
    last = 0L, item = hostDict->entries[i];
    item && (hostDict->strings ? strcmp(item->key, key) : item->key != key);
    last = item, item = item->next
    )
    ;
  if (last && item)
    {
    last->next = item->next;
    item->next = hostDict->entries[i];
    hostDict->entries[i] = item;
    }
  if (item)
    item->time = TIME();
  return item ? item->value : 0L;
  }

void enumerateHostDict(hostDict, enumerate)
  HostDict *hostDict;
  void (*enumerate)();
  {
  int i;
  HostDictEntry *item;

  for (i = 0; i < hostDict->length; i++)
    for (item = hostDict->entries[i]; item; item = item->next)
      (*enumerate)(item->key, item->value);
  }

int hashHostDict(hostDict, key)
  HostDict *hostDict;
  char *key;
  {
  int i;
  
  if (hostDict->strings)
    {
    for (i = 0; *key; )
      i += *(unsigned char *)key++ + 1;
    return i % hostDict->length;
    }
  else
    return (unsigned long) key % hostDict->length;
  }

void hostDictPurge(hostDict)
  HostDict *hostDict;
  {
  int i;
  HostDictEntry *last;
  HostDictEntry *item;
  int leastI;
  HostDictEntry *leastLast;
  HostDictEntry *leastItem;
  
  for (leastItem = 0L; hostDict->purge && hostDict->items > hostDict->length; leastItem = 0L)
    {
    for (i = 0; i < hostDict->length; i++)
      {
      for (last = 0L, item = hostDict->entries[i]; item; last = item, item = item->next)
        {
        if (!leastItem || leastItem->time > item->time)
          {
          leastI = i;
          leastLast = last;
          leastItem = item;
          }
        }
      }
    if (leastLast)
      leastLast->next = leastItem->next;
    else
      hostDict->entries[leastI] = leastItem->next;
    (*hostDict->purge)(leastItem->value);
    if (hostDict->strings && hostDict->copy)
      free((char *) leastItem->key);
    free((char *) leastItem);
    hostDict->items--;
    }
  }
