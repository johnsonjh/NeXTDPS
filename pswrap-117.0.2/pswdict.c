/*
  pswdict.c

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Paul Rovner: Thursday, May 12, 1988 4:02:55 PM
Edit History:
Andrew Shore: Wed Jul 13 16:39:45 1988
End Edit History.
*/

/***********/
/* Imports */
/***********/

#include "pswtypes.h"
#include "pswdict.h"
#include <string.h>

extern char *psw_calloc();
extern char *psw_malloc();

/********************/
/* Types */
/********************/

typedef struct _t_EntryRec {
  struct _t_EntryRec *next;
  char *name;
  PSWDictValue value;
  } EntryRec, *Entry;

 /* The concrete definition for a dictionary */
 typedef struct _t_PSWDictRec {
  int nEntries;
  Entry *entries;
  } PSWDictRec;

PSWDict atoms;

/**************************/
/* Procedure Declarations */
/**************************/

/* Creates and returns a new dictionary. nEntries is a hint. */
PSWDict CreatePSWDict(nEntries) int nEntries; {
  PSWDict d = (PSWDict)psw_calloc(sizeof(PSWDictRec), 1);
  d->nEntries = nEntries;
  d->entries = (Entry *)psw_calloc(sizeof(EntryRec), d->nEntries);
  return d;
  }

/* Destroys a dictionary */
void DestroyPSWDict(dict) PSWDict dict; {
  free(dict->entries);
  free(dict);
  }

static int Hash(name, nEntries) char *name; int nEntries; {
  register int val = 0;
  while (*name) val += *name++;
  if (val < 0) val = -val;
  return (val % nEntries);
  }

static Entry Probe(d, x, name) PSWDict d; int x; char *name; {
  register Entry e;
  for (e = (d->entries)[x]; e; e = e->next) {
    if (strcmp(name, e->name) == 0) break;
    }
  return e;
  }

static Entry PrevProbe(prev, d, x, name)
  Entry *prev; PSWDict d; int x; char *name;
  {
  register Entry e;
  *prev = NULL;
  for (e = (d->entries)[x]; e; e = e->next) {
    if (strcmp(name, e->name) == 0) break;
    *prev = e;
    }
  return e;
  }

/* -1 => not found */
PSWDictValue PSWDictLookup(dict, name) PSWDict dict; char *name; {
  Entry e;
  e = Probe(dict, Hash(name, dict->nEntries), name);
  if (e == NULL) return -1;
  return e->value;
  }

/*  0 => normal return (not found)
   -1 => found. If found, value is replaced. */
PSWDictValue PSWDictEnter(dict, name, value)
  PSWDict dict; char *name; PSWDictValue value;
  {
  Entry e;
  int x = Hash(name, dict->nEntries);
  e = Probe(dict, x, name);
  if (e) {
    e->value = value;
    return -1;
    }
  e = (Entry)psw_calloc(sizeof(EntryRec), 1);
  e->next = (dict->entries)[x]; (dict->entries)[x] = e;
  e->value = value;
  e->name = MakeAtom(name);
  return 0;
  }

/* -1 => not found. If found, value is returned. */
PSWDictValue PSWDictRemove(dict, name) PSWDict dict; char *name; {
  Entry e, prev;
  PSWDictValue value;
  int x = Hash(name, dict->nEntries);

  e = PrevProbe(&prev, dict, x, name);
  if (e == NULL) return -1;
  value = e->value;
  if (prev == NULL) (dict->entries)[x] = e->next; else prev->next = e->next;
  free(e);
  return value;
  }

PSWAtom MakeAtom(name) char *name; {
  Entry e;
  int x = Hash(name, 511);
  char *newname;

  if (atoms == NULL) atoms = CreatePSWDict(511);
  e = Probe(atoms, x, name);
  if (e == NULL) {
    e = (Entry)psw_calloc(sizeof(EntryRec), 1);
    e->next = (atoms->entries)[x]; (atoms->entries)[x] = e;
    e->value = 0;
    newname = psw_malloc(strlen(name)+1);
    strcpy(newname, name);
    e->name = newname;
    }
  return e->name;
  }

