#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
    namecode.c

    This file has routines to manage the getting of name codes
    from the server.  It keeps a hash table, hashing on the
    text of the names.  Of each entry in the table is a
    linked-list of structs that hashed there.  In the struct
    is a name and its code.
    There is also another array of pointers to these structs
    that is ordered by bytecode number.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#include "dpsclient.h"
#include "defs.h"
#include "wraps.h"
#include <string.h>
#include <objc/hashtable.h>
#include <stdio.h>

#include "nameTable.c"

static unsigned nameHash(const void *info, const void *data);
static int nameIsEqual(const void *info, const void *data1, const void *data2);

static NXHashTable *NameTable = NULL;
    /* The use of the hashtable is a little weird.  I store all the names in the NameList array, and the position in this array is the username index.  Instead of giving the hashtable package the addresses of these strings, I give it their indices in NameList.  This means when I look up a string, I get back its index if its already hashed.  I can also determine the string from the index fast when printing.   By using indices I can realloc the NameList array without screwing the hashtable up.  */

static const char **NameList = NULL;
    /* zero slot unused, first slot reserved for hashtable temp lookup key */
static int NameListSize = 0;

static LastUserObjectIndex = 0;


/* This is called with some object on the top of the operand stack.  It
   maps the object to the next available user index.  It returns the index
   that the object was assigned.
*/
int DPSDefineUserObject(int index)
{
    if (DPSGetCurrentContext()) {
	if(!index)
	    index = ++LastUserObjectIndex;
	PSsendint(index);
	PSexch();
	PSdefineuserobject();
	return index;
    } else
	return 0;
}


/* undefines a user object.  For now, we just tell PS about it.  One day
   maybe we'll reuse the index.
 */
void DPSUndefineUserObject(int index)
{
    if (DPSGetCurrentContext())
	PSundefineuserobject(index);
}


/* Hands out codes for usernames.  After this routine is called,
   the context's names will be in sync.  If the context is in sync when
   this routine is called, then any new names are sent in the lookup loop
   with defineusernames.  If its not in sync, we just get the whole thing
   up to speed after we lookup the names.  There can be no gaps in the 
   username code list to be filled in later.
*/
void DPSMapNames(DPSContext ctxt, unsigned int numNames,
		const char * const *nameArray,
		long int * const *numPtrArray )
{
    const char *name;
    long int *code;
    int lastCode;		/* last code handed out */
    int mapUpdateNeeded;
    const NXHashTablePrototype proto = {&nameHash, &nameIsEqual, &NXNoEffectFree, 0};

    if (!NameList) {
	MALLOC(NameList, const char *, 8);
	NameListSize = 8;
    }
    if (!NameTable)
	NameTable = NXCreateHashTable(proto, 8, NULL);

    mapUpdateNeeded = (ctxt->space->lastNameIndex < _DPSLastNameIndex);
    for ( ; numNames--; nameArray++, numPtrArray++) {
	name = *nameArray;
	code = *numPtrArray;
	if (name) {
	    NameList[1] = name;
	    lastCode = (int)NXHashGet(NameTable, (void *)1);
	    if (!lastCode) {		/* if not previously used */
		lastCode = ++_DPSLastNameIndex;
		if (_DPSLastNameIndex == NameListSize) {
		    NameListSize *= 2;
		    REALLOC(NameList, const char *, NameListSize);
		}
		NameList[_DPSLastNameIndex] = name;
		(void)NXHashInsert(NameTable, (void *)_DPSLastNameIndex);
		if (!mapUpdateNeeded && ctxt->nameEncoding == dps_indexed)
		    DPSPrintf(ctxt, "%d /%s defineusername\n", lastCode, name);
	    }
	}
	*code = lastCode;		/* take the previous name's value */
    }
    if (mapUpdateNeeded)
	DPSUpdateNameMap(ctxt);
    else
	ctxt->space->lastNameIndex = _DPSLastNameIndex;
}


/* looks up a name given a code */
const char *DPSNameFromIndex(int namecode)
{
    return DPSNameFromTypeAndIndex(-1, namecode);
}

/* looks up a name given a code and group */
const char *DPSNameFromTypeAndIndex(short namegroup, int namecode)
{
    if (namegroup < 0)		/* if pre-wired name */
	return _DPSNameTable + _DPSNameOffsetTable[-namegroup][namecode];
    else		/* else a user name defined at runtime */
	return NameList ? NameList[namecode] : NULL;
}


static unsigned nameHash(const void *info, const void *data)
{
    return NXStrHash(NULL, NameList[(int)data]);
}


static int nameIsEqual(const void *info, const void *data1, const void *data2)
{
    return NXStrIsEqual(NULL, NameList[(int)data1], NameList[(int)data2]);
}
