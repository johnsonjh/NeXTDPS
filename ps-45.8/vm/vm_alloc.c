/*
  vm_alloc.c

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

Original version: 
Edit History:
Ivor Durham: Mon May  8 09:20:01 1989
Ed Taft: Fri Dec  1 10:21:30 1989
Linda Gass: Mon Jun  8 15:51:14 1987
Joe Pasqua: Mon Jan  9 16:07:05 1989
Jim Sandman: Tue Oct 18 15:08:37 1988
Perry Caro: Mon Nov  7 11:14:27 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include EXCEPT
#include ORPHANS
#include PSLIB
#include RECYCLER
#include VM

#include "abm.h"
#include "saverestore.h"
#include "vm_memory.h"



public procedure AllocPName(length, pno)
  Card16 length; PNameObj pno;
{
  PNameEntry pne;
  boolean origShared = CurrentShared();
#if VMINIT
  Level origSegType = CurrentVMSegmentType();
#endif VMINIT

  /* Allocate both NameEntry and string in shared VM.
     The NameEntry goes in permanentRAM, the string in ROM */
  DURING
    SetShared(true);
#if VMINIT
    SetVMSegmentType(stPermanentRAM);
#endif VMINIT

    pne = (PNameEntry) AllocAligned ((integer) sizeof (NameEntry));
    ConditionalResetRecycler (sharedRecycler, (PCard8)pne);

    os_bzero((char *) pne, (integer) sizeof(NameEntry));
    pne->seen = !vmShared->valueForSeen;

#if VMINIT
    SetVMSegmentType(stROM);
#endif VMINIT

    pne->strLen = length;
    pne->str = (length == 0)? NIL : (charptr) AllocChars((integer) length);
    ConditionalResetRecycler (sharedRecycler, pne->str);

  HANDLER
    SetShared(origShared);
#if VMINIT
    SetVMSegmentType(origSegType);
#endif VMINIT
    RERAISE;
  END_HANDLER;

  SetShared(origShared);
#if VMINIT
  SetVMSegmentType(origSegType);
#endif VMINIT

  XNameObj(*pno, pne);
}

public procedure AllocPDict(size, pob)  cardinal size;  PObject pob;
/* size is the maximum number of entries that may be inserted into the
   returned dictionary (i.e., its maxlength). Ordinarily we allocate
   a dictionary 25% larger than size to improve hashing performance
   (especially for unsuccessful lookups). However, this is unnecessary
   for systemdict, since unsuccessful searches almost never occur
   (systemdict has its own private BitVector bit, so unsuccessful
   searches are usually avoided). */
{
  DictBody db;
  register PKeyVal kvp;
  register integer i;
  KeyVal  nkv;
  PDictBody vdp;
  PDictBody newDB;

  size = (size <= 10 || rootShared->vm.Shared.dictCount == 0) 
  				? size + 1 : size + size / 4;
  if ( (newDB = (PDictBody)ABM_Allocate((integer)sizeof(DictBody))) == NULL) {
    newDB = (PDictBody)AllocAligned ((integer) sizeof (DictBody));
    ConditionalResetRecycler (vmCurrent->recycler, (PCard8)newDB);
  }
  LDictObj (*pob, newDB);
  nkv.key = NOLL;
  nkv.value = NOLL;
  os_bzero ((char *)&db, (long int)sizeof(db));
  db.level = LEVEL;
  db.curlength = db.maxlength = db.curmax = 0;
  db.size = size;
  db.seen = !vmCurrent->valueForSeen;
  db.isfont = false;
  db.shared = pob->shared;
  db.tricky = false;
  i = size * sizeof (KeyVal);
  if ( (db.begin = (PKeyVal)ABM_Allocate(i)) == NULL) {
    db.begin = (PKeyVal)AllocAligned(i);
    ConditionalResetRecycler (vmCurrent->recycler, (PCard8)(&db.begin[0]));
  }
  db.end = &db.begin[size];
  kvp = db.begin;
  if ((i = size) != 0) {
    do {
      *(kvp++) = nkv;
    } while (--i != 0);
  }
  vdp = pob->val.dictval;
  *vdp = db;
  RecordFinalizableObject(dictObj, *pob);
}				/* end of AllocPDict */

public procedure AllocPString(length, pob)  cardinal length; PObject pob;
{
  LStrObj (*pob, length,
    (length == 0)? NIL : (charptr) AllocChars ((integer) length));
}				/* end of AllocPString */

public procedure AllocPStream(pob)
  PStmObj pob;
{
  PStmBody sb;
#if VMINIT
  /* StmBodies must always be allocated in RAM */
  Level origSegType = CurrentVMSegmentType();
  SetVMSegmentType(stPermanentRAM);
  DURING
#endif
    sb = (PStmBody) AllocAligned ((integer) sizeof(StmBody));
#if VMINIT
  HANDLER
    SetVMSegmentType(origSegType);
    RERAISE;
  END_HANDLER;
  SetVMSegmentType(origSegType);
#endif VMINIT

  ConditionalResetRecycler (vmCurrent->recycler, (PCard8)sb);

  LStmObj (*pob, 0, sb);
  os_bzero((char *)sb, (integer) sizeof(StmBody));
  sb->shared = pob->shared;
  sb->seen = !vmCurrent->valueForSeen;
  RecordFinalizableObject(stmObj, *pob);
}				/* end of AllocPStream */

public procedure AllocPArray(len, paob)
  cardinal len;
  PObject paob;
{
  register integer i;
  register PObject op;

  LAryObj (*paob, len, (len == 0) ? NIL : (PObject) AllocAligned ((integer) (len * sizeof (Object))));

  op = paob->val.arrayval;
  if ((i = len) != 0) {
    Object ob;
    ob = NOLL;
    ob.seen = !vmCurrent->valueForSeen;
    do { *(op++) = ob; } while (--i != 0);
  }
}				/* end of AllocPArray */

public procedure AllocGenericObject (Type, Size, pObject)
  cardinal Size;
  Card8 Type;
  PObject pObject;
 /*
   See interface for specification.
  */
{
  PGenericBody pBody;
#if	(STAGE == DEVELOP)
  Assert ((Size >= sizeof (GenericBody)) && (Type >= nBaseObTypes) && (Type < nObTypes));
#endif	(STAGE == DEVELOP)

  pBody = (PGenericBody) AllocAligned((integer)Size);
  ConditionalResetRecycler (vmCurrent->recycler, (PCard8)pBody);

  pBody->type = Type;
  pBody->level = LEVEL;
  pBody->seen = !vmCurrent->valueForSeen;
  pBody->length = Size;

  LGenericObj(*pObject, Type, pBody);
  RecordFinalizableObject(Type, *pObject);
}


VMExpandDict(d,SearchFunc)		/* expand dictionary */
DictObj	d;
boolean	(*SearchFunc)();
{
register PKeyVal	kvp;
register PNameEntry	pne;
register integer	i,maxsize;
PKeyVal		loc,old_begin;
DictBody	db;
KeyVal		nkv;
Card16		old_size;
PVM		savevm;
Level		savelevel;

/* allocate a new dictionary table */


  VMGetDict(&db, d);
  old_size = db.size;
  old_begin = db.begin;
  savevm = vmCurrent;
  savelevel = NOLL.level;
  if (db.shared) {
	vmCurrent = vmShared;
	NOLL.level = 0;
  } else
  	vmCurrent = vmPrivate;
  	
  maxsize = db.maxlength + db.maxlength/4;
  db.size = (maxsize < (db.size * 2)) 
  				? maxsize : db.size * 2;
  db.curmax = (db.maxlength < (db.curmax * 2)) 
  				? db.maxlength : db.curmax * 2;  
  i = db.size * sizeof (KeyVal);
  if ( (db.begin = (PKeyVal)ABM_Allocate(i)) == NULL) {
    db.begin = (PKeyVal)AllocAligned(i);
    ConditionalResetRecycler (vmCurrent->recycler, (PCard8)db.begin);
  }

  vmCurrent = savevm;
  NOLL.level = savelevel;

  db.end = &db.begin[db.size];
 
  nkv.key = NOLL;
  nkv.value = NOLL;
  kvp = db.begin;
  for (i=0; i<db.size; i++)
	*(kvp++) = nkv;

/* copy the old contents into the new table */
  kvp = old_begin;
  for (i=0; i<old_size; i++) {
  	if(kvp->key.type != nullObj){
		if((*SearchFunc)(&db, kvp->key, &loc)) {   /* get location */
			os_fprintf(os_stderr, "Unable to expand dictionary\n");
			CantHappen();	/* table can't be full yet */
		} 
		VMPutDKeyVal(&db, loc, kvp); 	/* copy kvp */
		if(kvp->key.type == nameObj) {	/* validate name cache */
			pne = kvp->key.val.nmval;
			if(pne->dict == d.val.dictval)
				pne->kvloc = loc;
		}
	}
  kvp++;
  }

  VMPutDict(d, &db);
}		
