/*
  vm_reverse.c

Copyright (c) 1987, '88 Adobe Systems Incorporated.
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
Ivor Durham: Wed Jul 20 17:25:05 1988
Joe Pasqua: Thu Dec  3 16:46:53 1987
Jim Sandman: Tue Mar 15 11:31:00 1988
Ed Taft: Fri Jan  5 09:56:14 1990
Ross Thompson: Tue Nov 28 18:33:11 1989
End Edit History.

This module exports a single procedure ReverseVM that reverses the
endianness of all objects in the VM. It alters the VM in place.
This enables the VM to be written out and subsequently bound into a
system being cross-compiled to a target machine whose endianness is
opposite that of the development machine. Of course, this assumes that
the development and target machine's data structures (structs, floating
point numbers, etc.) are compatible except for bit order.

Since the granularity of items that have to be reversed varies according
to the type of object, this operation requires tracing the entire VM,
starting at the Root. We keep an auxiliary data structure to ensure that
each object is visited only once, since we are reversing them in place.
The tracing algorithm is derived from vm_relocate.c in the Display
PostScript system (vm/v011/v001).

ReverseVM renders the contents of the VM unusable in the host environment,
so the host system must exit immediately thereafter. ReverseVM is
currently called only as part of the makevm operator.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include PSLIB
#include STREAM
#include VM

#include "vm_space.h"
#include "vm_segment.h"
#include "saverestore.h"
#include "vm_relocate.h"
#include "vm_reverse.h"

#if (! CANREVERSEVM)
ConfigurationError("vm_reverse.c compiled for unsupported configuration");
#endif

#if UNSIGNEDCHARS
ConfigurationError("Doesn't work when char type is unsigned");
#endif

/*
 * Addresses of objects are recorded in a hash table so that they get
 * reversed only once.  If the hash table fills up, it is extended
 * and the search strategy switches to linear because the hash function
 * is no longer valid.
 */

#define	REVHASHSIZE	(1 << 13)
#define	REVHASHMASK	(REVHASHSIZE - 1)

typedef char *Pchar;

typedef struct {
  PCard8 *markTable;	/* Beginning to address recording table */
  PCard8 *endTable;	/* Entry beyond last one in original table */
  int     tableSize;	/* Current table size */
  int     tableEntries;	/* Number of entries in table */
  int     collisions;	/* Number of times an entry an entry already present */
  boolean hash;		/* True: use hash search, False: use linear search */

}       revMarkObject;

static revMarkObject *mark;

/* SWAP4(PCard32 pValue); -- swaps bytes of *pValue in place. */
#define SWAP4(pValue) { \
  register char c0, c1; \
  c0 = ((Pchar) (pValue))[0]; c1 = ((Pchar) (pValue))[1]; \
  ((Pchar) (pValue))[0] = ((Pchar) (pValue))[3]; \
  ((Pchar) (pValue))[1] = ((Pchar) (pValue))[2]; \
  ((Pchar) (pValue))[2] = c1; ((Pchar) (pValue))[3] = c0; \
  }

/* SWAP2(PCard16 pValue); -- swaps bytes of *pValue in place. */
#define SWAP2(pValue) { \
  register char c0; \
  c0 = ((Pchar) (pValue))[0]; \
  ((Pchar) (pValue))[0] = ((Pchar) (pValue))[1]; \
  ((Pchar) (pValue))[1] = c0; \
  }

/*
 * Forward declarations
 */

private procedure ScanPkdAry();
private procedure ScanFromAry();
private procedure ScanFromDict();
private procedure ScanNameArray();

private int AddToMarkTable(loc)
  PCard8 loc;
 /*
  * Use hash function to find entry in mark table unless table has filled
  * up once, in which case we resort to a linear search because the hash
  * property is lost.
  */
{
  register PCard8 *TableEntry;

  /* See if this address has already been marked  */

  if (mark->hash) {
    register PCard8 *StartEntry;
    register longcardinal Index = (longcardinal) loc;

    Index *= 0x41c64e6d;		/* Borrowed from old dict.c hash */
    Index = (Index >> 16) & REVHASHMASK;

    TableEntry = &mark->markTable[Index];
    StartEntry = TableEntry;

    do {
      if (*TableEntry == loc) {
	return (false);
      } else if (*TableEntry == 0)
	break;
      else {
	mark->collisions++;
	TableEntry++;
	if (TableEntry == mark->endTable)
	  TableEntry = &mark->markTable[0];
      }
    } while (TableEntry != StartEntry);

   /* Reach here if (a) empty entry found or (b) loc not in full table */
  } else {
    register int i;

    for (i = 0; i < mark->tableEntries; i++)
      if (mark->markTable[i] == loc) {
	return (false);
      }

    TableEntry = &mark->markTable[mark->tableEntries];
  }

  /* Reach here when loc is to be added to the table */

  if (mark->tableEntries == mark->tableSize) {
    mark->tableSize *= 2;	/* Double the table size */
    os_eprintf("ReverseVM: expanding mark table to %D\n",
            mark->tableEntries);
    fflush(os_stderr);
    mark->markTable = (PCard8 *) EXPAND (
		       mark->markTable, mark->tableSize, sizeof (PCard8));
    mark->hash = false;		/* Note mark->endTable is now invalid */

    TableEntry = &mark->markTable[mark->tableEntries];
  }

  /* Place the new entry in the table and bump # of entries */

  *TableEntry = loc;
  mark->tableEntries++;

  return (true);
}

public procedure ReverseFields(ptr, fields)
  Pchar ptr, fields;
{
  register Int32 field, align, bits;
  register Card32 temp, destData;
  register Pchar bptr, eptr;

  align = 0;
  while ((field = *fields++) != 0) {
    if (((align | field) & 7) == 0) {
      if (field > 0) {
         field >>= 3;
         bptr = ptr;
         ptr += field;
         if (field > 1) {
            eptr = ptr;
            do {temp = *bptr; *bptr++ = *--eptr; *eptr = temp;}
            while (bptr < eptr); }}
      else ptr += (-field) >> 3; }
    else {
#if SWAPBITS
      CantHappen();  /* little-to-big-endian conversion not implemented */
#else SWAPBITS
      /* Unaligned field; do complicated shuffle. First, extract the
         source field into into a simple 32-bit variable (as if we
	 had simply referenced the field in a C program). */
      Assert(field > 0);
      temp = 0;
      bptr = ptr;
      bits = align + field;
      Assert(bits <= 32);
      do {temp = (temp << 8) | *bptr++;}
      while ((bits -= 8) > 0);
      temp >>= (-bits);

      /* Field value is right-justified in temp. Now align it with
         the destination and merge low-order bits with the data (if any)
	 left over from a previous field. */
      temp <<= align;
      destData &= (1 << align) - 1;
      destData |= temp;
      temp >>= 8;
      field -= 8 - align;

      /* Store any bytes that are completely filled */
      while (field >= 0)
        {
	*ptr++ = destData;
	destData = temp;
	temp >>= 8;
	field -= 8;
	}

      /* Compute ending alignment. If it is nonzero, the rightmost "align"
         bits of destData are left over for the next iteration. */
      align = field & 7;  /* field mod 8 */
#endif SWAPBITS
      }
    }

  Assert(align == 0);
}

private procedure ReverseObject(obj, trace)
  register PObject obj; boolean trace;
 /*
  * If trace is true, traces all objects referenced from the value of *obj.
  * Then reverses the endianness of the object itself. The caller is
  * responsible for ensuring that the object has not been visited before.
  */
{
  static char fieldsObject[] = {
    1, 3, 4,            /* tag, access, type */
    1, 1, 2, 4,         /* shared, seen, (unused), level */
    16, 32,             /* length, val */
    0};
  static char fieldsGenericBody[] = {
    8, 4, 1, 3, 16};	/* type, level, seen, pad, length */

  switch (obj->type)
    {
    case dictObj:
      if (trace) ScanFromDict ((PDictObj) obj);
      break;
    case arrayObj:
      if (trace) ScanFromAry ((PAryObj) obj);
      break;
    case pkdaryObj:
      if (trace) ScanPkdAry ((PPkdaryObj) obj);
      break;
    case escObj:
      switch(obj->length) {
          case objCond:
          case objLock:
	    /* Treat as GenericBody only; do not attempt to reverse value */
	    if (AddToMarkTable((PCard8) obj->val.genericval))
	      ReverseFields((Pchar) obj->val.genericval, fieldsGenericBody);
	    break;
          case objSave:
          case objGState:
	    /* illegal for these to appear in a saved VM */
            CantHappen();
            break;
          case objNameArray:
              if (trace) ScanNameArray((PNameArrayObj) obj, false);
            break;
          case objMark:
             /* No action. */
             break;
          default: CantHappen();
      }
      break;
    case nameObj: /* nothing to do; name table is traced separately */
    case cmdObj: /* ditto */
    case nullObj:
    case intObj:
    case realObj:
    case boolObj:
    case strObj: /* nothing to do; string bodies do not have endianness */
    case stmObj:
    case fontObj:
      /* No action */
      break;
    default:
      CantHappen ();
    }

  ReverseFields((Pchar) obj, fieldsObject);
}

private procedure ScanPkdAry(ary)
  PPkdaryObj ary;
{
  PkdaryObj rest;
  Object obj, robj;
  register PCard8 pa;

  rest = *ary;			/* Copy the packed array object	 */
  while (rest.length != 0)
    {
    /* DecodeObj() decodes the first element of rest into obj; it then
       updates rest so that it describes the rest of the pkdary. Calling it
       repeatedly enumerates the array. Only "escape" objects
       (i.e., full-blown embedded Objects) need to be reversed,
       though others must be traced. We detect that an escape object
       has been encountered by observing when the address advances
       by sizeof(Object) + 1.
     */

    pa = rest.val.pkdaryval + 1;
    DecodeObj(&rest, &obj);

    if (rest.val.pkdaryval == pa + sizeof(Object))
      {
      if (! AddToMarkTable(pa))
        continue;  /* already visited this object; skip it */

      /* To reverse an escape object, reverse a copy of the decoded
         object and then do a byte copy to store it back to the
         packed array. This avoids problems with misaligned data. */

      robj = obj;
      ReverseObject(&robj, false);
      os_bcopy((char *) &robj, (char *) pa, sizeof(Object));
      }

    /* Now trace the returned object. Note that we do not save its
       address in the mark table unless it was an escape object; thus,
       we may visit it more than once. This does no harm for simple
       objects. Dictionary bodies and nested array elements are marked
       independently. Nested packed arrays are not marked (except
       for escape elements); however, this cannot cause an infinite
       recursion because any circular chain of packed arrays is
       guaranteed to include at least one escape object. */

    switch (obj.type)
      {
      case dictObj:
        ScanFromDict((PDictObj) &obj);
	break;
      case arrayObj:
        ScanFromAry((PAryObj) &obj);
	break;
      case pkdaryObj:
        ScanPkdAry((PPkdaryObj) &obj);
        break;
      default:
	break;
      }
    }
}

private procedure ScanFromAry(ary)
  PAryObj ary;
{
  register PObject pObj = ary->val.arrayval;
  register integer i;

  /* Every entry is a pointer. Count them */

  for (i = ary->length; --i >= 0; pObj++)
    {
    if (AddToMarkTable ((PCard8) pObj))
      ReverseObject (pObj, true);
    }
}

private procedure ScanFromDict(dict)
  PDictObj dict;
{
  register PDictBody db;
  register PDictObj xobj;
  register PKeyVal kv;
  static char fieldsDictBody[] = {
    32, 32, 32,		/* bitvector, begin, end */
    16, 16, 16, 16,	/* curlength, maxlength, curmax, size */
    1, 1, 1, 1,         /* shared, seen, isfont, tricky */
    4, 1, 4, 3,         /* level, privTraceKeys, (unused), access */
    0};

  xobj = XlatDictRef(dict);
  if (xobj != dict) {
      return; }
  else {
      db = xobj->val.dictval;
      if (!AddToMarkTable (db)) {
            return; }

  /* If AddToMarkTable returns false, that means the dict is	 */
  /* already in the table, so we need trace no further.	 */

      if (AddToMarkTable (db->begin))
        {
    /* Handle each entry in the dictionary. */
        for (kv = db->begin; kv < db->end; kv++)
          {
          ReverseObject (&(kv->key), true);
          ReverseObject (&(kv->value), true);
          }
        }
  }

  ReverseFields((Pchar) db, fieldsDictBody);
}

private procedure ScanNameArray(na, flag)
    PNameArrayObj na;
{
    static char fieldsNameEntry[] = {
      32, 32,			/* link, str */
      32, 10, 22,		/* vec, ts (2 flds) */
      32, 32,			/* kvloc, dict */
      16, 16,			/* nameindex, ncilink */
      8, 1, 7,			/* strlen seen, (unused)  */
      0};
    static char fieldsNABody[] = {
      8, 4, 1, 3, 16,		/* header */
      16, 16,			/* length, (unused) */
      0};
    PNameEntry *t0, t1, t2;
    PNameArrayBody nb;
    int i;

    nb = na->val.namearrayval;
    if (!(AddToMarkTable(nb))) return;

    for (t0 = &nb->nmEntry[0], i = nb->length; i--; t0++) {
        if (flag) 
            for (t1 = *t0; t1; t1 = t2) {
                t2 = t1->link;
                if (!AddToMarkTable (t1)) CantHappen();
                ReverseFields((Pchar) t1, fieldsNameEntry); }
        SWAP4((Pchar) t0); }
    
    ReverseFields((Pchar) nb, fieldsNABody);
}

private procedure RevTableCreate()
/* Builds the temporary data structures used by ReverseVM. */
{
  register integer i;

  mark = (revMarkObject *) NEW (1, sizeof (revMarkObject));

  mark->tableSize = REVHASHSIZE;
  mark->tableEntries = 0;
  mark->collisions = 0;
  mark->hash = true;

  mark->markTable = (PCard8 *) NEW (mark->tableSize, sizeof(PCard8));
  mark->endTable = &mark->markTable[mark->tableSize];
}

private procedure RevTableDestroy()
{

  FREE (mark->markTable);
  FREE (mark);
  mark = NULL;
}

private procedure Trace_Finalize_Chain(root)
   PVMRoot root;
{
    PFinalizeNode t1, t2;

    for (t1 = root->finalizeChain; t1; t1 = t2) {
        t2 = t1->link;
        if (!AddToMarkTable(&t1->obj)) CantHappen();
        ReverseObject(&t1->obj, true);
        SWAP4((Pchar) &t1->link); }
}

private procedure Trace_Stream_List(root)
    PVMRoot root;
{
    static char fieldsStmBody[] = {
      32, 32,           /* link, stm */
      16, 8, 1, 1, 6,	/* generation, level, shared, seen, unused */
      0};
    PStmBody t1, t2;

    for (t1 = root->stms; t1; t1 = t2) {
        t2 = t1->link;
        if (t1->stm && AddToMarkTable(t1->stm))
            ReverseObject(t1->stm, true);
        ReverseFields((Pchar) t1, fieldsStmBody); }
}

private procedure ReverseIDArray(arr, lim)
   charptr arr;
   register Card16 lim;
{
   /* This is the one string in all of VM that has to be 
      reversed.  The reason is that it is being treated as
      an array of 16 bit quantities by everything except
      the type mechanism. */

  register PCard16 tmp;
  register Card16 i;

   for (tmp = (PCard16) arr, i = 0; i < lim ; i++, tmp++)
        SWAP2(tmp);
}

private procedure ReverseRoot(root, privp)
   PVMRoot root;
   boolean privp;
{
  static char fieldsRoot1[] = {
    10, 22,             /*spaceID */
    -64, 32,             /* trickyDicts, finalizeChain */
    -64, 32,             /* nameMap, stms */
    8, 1, (32-8-1),     /* version, shared, (unused) */
    0};
  static char fieldsRoot2[] = {
    32,                 /* stamp */
    -64, -64,           /* sysDict, sharedDict */
    -64, -64,           /* internalDict, sharedFontDirectory */
    -64, -64, -64,      /* param, regNameArray, regOpNameArray */
    -64, 16, 16,        /* regOpIDArray, cmdCnt, dictCount */
    -64,                /* nameTable */
    16, 16,             /* hashindexmask, hashindexfield */
    32,                 /* downloadChain */
    0};

  ReverseObject(&root->trickyDicts, true);
  Trace_Stream_List(root);
  Trace_Finalize_Chain(root);
  ReverseObject(&root->nameMap, true);
  ReverseFields((Pchar) root, fieldsRoot1);

  if (privp) {
      Assert(root->vm.Private.srList == 0 &&
             root->vm.Private.level == 0); }
  else {
      ReverseObject(&root->vm.Shared.sysDict, true);
      ReverseObject(&root->vm.Shared.sharedDict, true);
      ReverseObject(&root->vm.Shared.internalDict, true);
      ReverseObject(&root->vm.Shared.sharedFontDirectory, true);

      ReverseObject(&root->vm.Shared.param, true);
      ReverseObject(&root->vm.Shared.regNameArray, true);
      ReverseObject(&root->vm.Shared.regOpNameArray, true);
      ReverseIDArray(root->vm.Shared.regOpIDArray.val.strval,
              root->vm.Shared.regOpIDArray.length / sizeof(Card16));
      ReverseObject(&root->vm.Shared.regOpIDArray, false);

  /* Follow up all the name chains and reverse the NameEntries; then
     reverse the name array object itself (without tracing).
     Do this AFTER reversing all other objects, because this renders
     the names unusable; tracing packed arrays requires being able
     to look up names. */

      ScanNameArray((PNameArrayObj) &root->vm.Shared.nameTable,
					true);
      ReverseObject(&root->vm.Shared.nameTable, false);
      ReverseFields((Pchar) &root->vm.Shared, fieldsRoot2);     
  }
}

public procedure ReverseVM ()
{
  RevTableCreate();

  ReverseRoot(rootShared, false);

  RevTableDestroy();
}

public procedure ReverseSegment(segment)
    PVMSegment segment;
{
  static char FieldsVmSeg[] = {
    32, 32,     /* succ, pred */
    32, 32, 32, /* first, last, free */
    8, 24,      /* level, (unused) */
    32, 32, 32, /* allocBM, firstFreeInABM, firstStackByte */
    0};

    ReverseFields((Pchar) segment, FieldsVmSeg);
}

public procedure ReverseStructure(vms)
   PVM vms;
{
    static char fieldsVMStructure[] = {
      32,               /* password */
      1, 1, 1,          /* shared, wholeCloth, valueForSeen  */
      1, (32-4),        /* collectThisSpace, (unused) */
      32, 32,		/* free, last */
      32, 32, 32, 32,	/* head, tail, current, reserve */
      32, 32, 32,	/* expansion_size, recycler, priv */
      32, 32,		/* allocCounter, allocThreshold */
      0};

    ReverseFields((Pchar) vms, fieldsVMStructure);
}

public procedure ReverseRelocationTable(addrp)
    boolean addrp;
{
    extern PRelocationEntry relocationTable;
    extern int relocationTableSize;
    PRelocationEntry t1;
    integer i;

    for (t1 = relocationTable, i = relocationTableSize;
                i--; t1++) {
	SWAP4((Pchar) &t1->value);
        if (addrp) { SWAP4((Pchar) &t1->address); }}
}

public procedure ReverseInteger(pInt)
   integer *pInt;
{
    SWAP4((Pchar) pInt);
}
