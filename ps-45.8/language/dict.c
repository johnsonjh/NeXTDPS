/*
  dict.c

Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Chuck Geschke: February 6, 1983
Edit History:
Larry Baer: Tue Nov 28 17:58:09 1989
Scott Byer: Thu Jun  1 16:40:41 1989
Chuck Geschke: Fri Oct 11 07:18:45 1985
Doug Brotz: Mon Jun  2 10:33:02 1986
Ed Taft: Sun Dec 17 18:56:24 1989
John Gaffney: Mon Jan 14 15:55:31 1985
Ivor Durham: Sun May 14 08:50:56 1989
Leo Hourvitz: Fri 03Oct86 Added BumpCETimeStamp
Linda Gass: Wed Aug  5 16:18:23 1987
Joe Pasqua: Wed Feb  8 13:36:15 1989
Jim Sandman: Fri Jul 28 16:23:06 1989
Paul Rovner: Fri Aug 25 22:37:21 1989
Perry Caro: Mon Nov  7 15:53:47 1988
Bill Bilodeau: Tue Jan 17 19:54:59 PST 1989
Jack 10Sep90 make shared FontDirectory growable in DictP, for bug # 7141
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include EXCEPT
#include PSLIB
#include FP
#include GC
#include LANGUAGE
#include VM
#include RECYCLER

#include "dict.h"
#include "exec.h"
#include "name.h"
#include "languagenames.h"
#include "stack.h"


/* The following items are global across all contexts	*/

#if (OS != os_mpw)
/*-- BEGIN GLOBALS --*/

private CmdObj dfacmd;
  /* Used in RgstMark() call for dictforall		*/
private int nPermDictEntries;
private boolean enableTrickyDictCopy;
  /* Set @ init time and same for all contexts		*/
  
public	GenericID timestamp;

#if (STAGE == DEVELOP)
private integer Nsearch, Nclash, Nmaxprobe;
#endif (STAGE == DEVELOP)

/*-- END GLOBALS --*/
#else (OS != os_mpw)

typedef struct {
 CmdObj g_dfacmd;
  /* Used in RgstMark() call for dictforall		*/
 int g_nPermDictEntries;
 boolean g_enableTrickyDictCopy;
  /* Set @ init time and same for all contexts		*/
#if STAGE==DEVELOP
 integer g_Nsearch, g_Nclash, g_Nmaxprobe;
#endif STAGE==DEVELOP
} GlobalsRec, *Globals;

private Globals globals;

#define dfacmd globals->g_dfacmd
#define nPermDictEntries globals->g_nPermDictEntries
#define enableTrickyDictCopy globals->g_enableTrickyDictCopy
#define Nsearch globals->g_Nsearch
#define Nclash globals->g_Nclash
#define Nmaxprobe globals->g_Nmaxprobe

#endif (OS != os_mpw)

/* some external declarations */
extern DictObj 	trickyUserDict;
extern DictObj	trickyFontDirectory;
extern DictObj	trickyStatusDict;

/*	----- Definitions for TrickyDicts -----
   A TrickyDict is a PostScript defined dictionary that
   is per space data and is named in systemdict. They
   are userdict, errordict, and FontDirectory. Since
   systemdict can not be updated (in ROM on printers)
   we put a TrickyDict object in systemdict that tells
   the implementation to substitute the current space's
   copy of that dictionary. These private copies are
   never seen outside the implementation (but they are
   put in the name cache). The other aspect of a
   TrickyDict is that a private copy of it isn't allocated
   until a write is performed; until then all spaces
   share a copy in shared vm. A TrickyDict is indicated
   by a non-zero value in the length field, which is normally
   zero for a dictionary; in this case, the dictval field
   gives the offset in the per-space Root structure at which
   the real dictionary can be found.

  FontDirectory trickydict substitution hack.

  When we look up FontDirectory in systemdict, the value that we get
  must depend on the VM allocation mode. When we are in private VM
  allocation mode, we should get the private font directory (which
  is actually a trickyDict with dictval tdFontDirectory). When we are
  in shared VM allocation mode, we should get the value of
  SharedFontDirectory. This substitution must be done at the moment
  of lookup, so that from the viewpoint of a PostScript program it
  appears that "setshared" actually changes the value of FontDirectory
  in systemdict.

  The actual value of FontDirectory in systemdict is always the
  private trickyDict; the substitution is done dynamically at
  lookup time. Also, we must prevent the timestamp for the name
  FontDirectory from ever being made current; this ensures that in-line
  lookups of FontDirectory will always fail and that the procedures in
  dict.c will be called.
 */

#define	MapFontDirectory(dictBody, pObj) \
  if (vmCurrent->shared && \
      (pObj)->length != 0 && \
      (pObj)->type == dictObj && \
      (Card32)((pObj)->val.dictval) == tdFontDirectory && \
      (dictBody) == rootShared->vm.Shared.sysDict.val.dictval) \
    *(pObj) = rootShared->vm.Shared.sharedFontDirectory;

#define IsFontDirectory(name) \
  (languageNames != NIL && \
   (name).val.nmval == languageNames[nm_FontDirectory].val.nmval)
   

private procedure BumpCETimeStamp()
 /*
   Bump generation field of per-context timestamp.  No-op during early
   initialisation.
  */
{
  if (timestamp.id.generation == MAXGenericIDGeneration) {
    ResetNameCache ((integer)3);	/* Zap all timestamps for cur ctx */
    timestamp.id.generation = 1;	/* Start at 1 so stamp never zero */
  } else
    timestamp.id.generation++;
}				/* BumpCETimeStamp */

public procedure SetCETimeStamp(index)
  Card32 index;
 /*
   Initialise the timestamp, starting with generation one so that the stamp
   variant can never be zero.
  */
{
  Assert (index <= MAXGenericIDIndex);
  timestamp.id.index = index;
  timestamp.id.generation = 1;
} /* SetCETimeStamp */

public GenericID GetCETimeStamp() { return(timestamp); }

private cardinal HashObject(key, length)
	Object key; cardinal length;
{
  register longcardinal lh;
  switch (key.type) {
    case intObj: {lh = os_labs(key.val.ival); break;}
    case realObj: {lh = os_floor(os_fabs(key.val.rval)); break;}
    case boolObj: return (cardinal)(key.val.bval);
    case nameObj: {lh = key.val.nmval->nameindex; break;}
    case strObj: {CantHappen(); break;}   /* strings are converted to names */
    case stmObj: case cmdObj: {lh = (longcardinal)key.length; break;}
    default: return 0;}
  lh = lh * 0x41c64e6d;
  lh = (lh ^ (lh>>16)) & 0177777;
  lh *= length;
  return ((lh>>16) & 0177777);
}

#define KeyName(k,n) {if ((k).type==strObj) StrToName(k,n); else *(n) = (k);}

#if STAGE==DEVELOP
private procedure PSProbeStats()
 {
  PushInteger(Nsearch);
  PushInteger(Nclash);
  PushInteger(Nmaxprobe);
}
#endif STAGE==DEVELOP

public boolean SearchDict(db, key, pResult)
  register PDictBody db; Object key; PKeyVal *pResult;
{
  register PKeyVal kvp = &db->begin[HashObject(key,db->size)];
  register integer i;
#if STAGE==DEVELOP
  Nsearch++;
#endif STAGE==DEVELOP
  if (key.type == nullObj) TypeCheck();
  for (i=0; i < db->size; i++){
    if ((kvp->key.type == nameObj) && (key.type == nameObj)){
      if (kvp->key.val.nmval == key.val.nmval)
      	goto success;
    } else {
      if (Equal(kvp->key,key)) 
      	goto success;
      }
    if (kvp->key.type == nullObj){
#if STAGE==DEVELOP
      if (i != 0){Nclash += i; Nmaxprobe = MAX(Nmaxprobe,i+1);}
#endif STAGE==DEVELOP
      *pResult = (db->curlength < db->curmax) ? kvp : NIL;
      return false;
    }
    if (++kvp >= db->end)
    	kvp = db->begin;
  }
  CantHappen();
  success:
#if STAGE==DEVELOP
    if (i != 0){Nclash += i; Nmaxprobe = MAX(Nmaxprobe,i+1);}
#endif STAGE==DEVELOP
    *pResult = kvp;
    return true;
}

typedef struct _t_TTLRec {PKeyVal kvl; PDictBody d;} TTLRec, *PTTLRec;

private boolean trytoload(key,p)
  Object key; PTTLRec p;
{
  Object name;
  PKeyVal SDres;
  PDictBody dict;
  register PDictObj obj, xobj;
  register BitVector bv;
  register PNameEntry pne;
  register PDictBody dp;
  
  KeyName(key, &name);
  if (name.type == nameObj) {
    pne = name.val.nmval;
    if (pne->ts.stamp == timestamp.stamp){  /* found in cache */
#if STAGE==DEVELOP
      Assert(pne->kvloc != NIL);
#endif STAGE==DEVELOP
      /* no need to check access: noaccess dict entries aren't timestamped */
      p->kvl = pne->kvloc;
      p->d = pne->dict;
      return true;
      }
    bv = pne->vec;
    dict = pne->dict;
    }
  else {bv = MAXunsignedinteger; dict = NIL;}
	/* if the key is not a name, must inspect each dictionary. setting bv
	   to all ones and dict to NIL ensures that each dict is searched. */
 
  for (obj = dictStk->head-1; obj >= dictStk->base; obj--)
    {
#if STAGE==DEVELOP
    Assert(obj->type == dictObj);
#endif STAGE==DEVELOP
    xobj = XlatDictRef(obj);
    dp = xobj->val.dictval;
    if ((dp->bitvector & bv) != 0)
      { /* this dict may contain name */
      if (dict == xobj->val.dictval)
        {
	/* name is here but ts is not current */
#if STAGE==DEVELOP
	Assert((pne->kvloc >= dp->begin)&&(pne->kvloc <= dp->end));
#endif STAGE==DEVELOP
	if ((dp->access & rAccess) == 0) InvlAccess();
	else if (! IsFontDirectory(name)) pne->ts = timestamp;
	p->kvl = pne->kvloc;
	p->d = xobj->val.dictval;
	return true;
	}
      if (SearchDict(dp, name, &SDres))
        {
	if ((dp->access & rAccess) == 0) InvlAccess();
	if (name.type == nameObj) {
	  pne->kvloc = SDres;
	  pne->dict = xobj->val.dictval;
	  pne->ts.stamp =
	    ((dp->access & rAccess) != 0 && ! IsFontDirectory(name))?
	    timestamp.stamp : 0;
	  }
	p->kvl = SDres;
	p->d = xobj->val.dictval;
	return true;
	}
      }
    }
  p->kvl = NIL;
  return false;
}

public boolean Load(key, pVal)
	Object key; PObject pVal;
{
  boolean known;
  TTLRec ttlval;
  known = trytoload(key, &ttlval);
  if (known) {
    VMGetValue(pVal, ttlval.kvl);
    MapFontDirectory(ttlval.d, pVal);
  }
  return known;
}

public boolean LoadName(key, pVal)
  PNameEntry key; PObject pVal;
{
  boolean known;
  TTLRec ttlval;
  NameObj no;
  LNameObj(no, key);
  known = trytoload(no, &ttlval);
  if (known) {
    VMGetValue(pVal, ttlval.kvl);
    MapFontDirectory(ttlval.d, pVal);
  }
  return known;
}


private procedure copydict(alloc, fd, td, checkacc, maxlength, realloc)
  boolean alloc; DictObj fd, *td; boolean checkacc; cardinal maxlength;
  boolean realloc;
{
  PDictObj reffd, reftd;
  DictBody db;
  PDictBody fdp;
  register PKeyVal kvp, endkvp;
  integer t;

  if (!alloc)
    if (fd.val.dictval == td->val.dictval) return;
  
  reffd = XlatDictRef(&fd);
  fdp = reffd->val.dictval;
  if (checkacc && (fdp->access & rAccess) == 0)
    InvlAccess();

  if (alloc)
    { /* Allocate new dictionary. Give it the same BitVector bit instead
         of assigning a new one, since the new dictionary will have the
	 same set of keys as the original, at least initially. */
    t = rootShared->vm.Shared.dictCount;
    DictP (maxlength, td);
    rootShared->vm.Shared.dictCount = t;
    td->val.dictval->bitvector = fdp->bitvector;
    }

  reftd = XlatDictRef(td);
  VMGetDict (&db, *reftd);
  if (realloc) {db.access = wAccess; db.curlength = 0;}
  if (!alloc)
    {
    if ((db.access & wAccess) == 0) InvlAccess ();
    if ((db.curlength != 0) || (db.maxlength < fdp->curlength))
      RangeCheck ();
    }

  db.access = fdp->access;
  VMPutDict (*reftd, &db);

  endkvp = fdp->end;
  for (kvp = fdp->begin; kvp < endkvp; kvp++)
    if (kvp->key.type != nullObj)
      ForcePut (*td, kvp->key, kvp->value);
}


public procedure CopyDict(from, pto)  DictObj from;  PDictObj pto;
{
  cardinal maxlength;
  maxlength = (cardinal)(XlatDictRef(&from)->val.dictval->curlength);
  copydict (false, from, pto, true, maxlength, false);
}


public procedure AllocCopyDict(d, increment, pdict)
  DictObj d; integer increment;  PDictObj pdict;
{
  cardinal maxlength;
  maxlength = (cardinal)(XlatDictRef(&d)->val.dictval->curlength
			 + increment);
  copydict(
    true, d, pdict, false, maxlength, false);
}


public procedure ReallocDict(d, pdict)
  DictObj d; 
  PDictObj pdict;
  {
   copydict(false, d, pdict, false, 0, true);
  }


private procedure CopyTrickyDict(pRealDict)
  PDictObj pRealDict;
{
  DictObj newDict;
  boolean originalShared = CurrentShared ();
  cardinal i;

  SetShared (false);
  DURING {
    copydict(
      true, *pRealDict, &newDict, false,
      (cardinal)pRealDict->val.dictval->maxlength, false);
  } HANDLER {
    SetShared (originalShared);
    RERAISE;
  } END_HANDLER;
  SetShared (originalShared);
  newDict.val.dictval->tricky = true;
  /* Fix reference in trickyDict array for copied dictionary. */
  for (i = 0; i < rootPrivate->trickyDicts.length; i++) {
    if (rootPrivate->trickyDicts.val.arrayval[i].val.dictval == pRealDict->val.dictval) {
      VMPutElem(rootPrivate->trickyDicts, i, newDict);
    }
  }
  *pRealDict = newDict;
  BumpCETimeStamp(); /* we might have changed what is on the dict stack */
}

private PutInDict(kvp,d,top, checkacc)
/* callers of this proc are expected to convert	*/
/* the key to a name object by calling KeyName	*/
  PKeyVal kvp;
  DictObj d;
  boolean top,	/* this defines the topmost binding on the dict stack */
    checkacc;	/* true to check write access, false to bypass it */
{
  PKeyVal SDres;
  register PDictBody dp;
  register PNameEntry pne;
  
  /*
    Invalidate the appropriate recycler range if either the key or the value
    is recyclable.  Because names are not recyclable, the most common keys
    will not be recyclable.  It is therefore most likely that only the value
    object will be recyclable.  The frequency of both key and value being
    recyclable is low.
   */

  {
    integer flag = 0;	/* 1 bit for key, 1 bit for value */

    if (Recyclable (&(kvp->value)))
      flag += 1;

    if (Recyclable (&(kvp->key)))
      flag += 2;

    switch (flag) {
     case 1:	/* Value only (most likely) */
      InvalidateRecycler (&(kvp->value), NIL);
      break;

     case 2:	/* Key only (unlikely:) */
      InvalidateRecycler (&(kvp->key), NIL);
      break;

     case 3:	/* Both key and value (least likely) */
      InvalidateRecycler (&(kvp->key), &(kvp->value));
      break;

     case 0:
      break;

     default:
      CantHappen();
    }
  }

  d = *XlatDictRef(&d);
  dp = d.val.dictval;

  if (dp->shared && dp->tricky && enableTrickyDictCopy) {
    CopyTrickyDict(&d);
    dp = d.val.dictval;
#if	VMINIT
    Assert (dp->tricky && !dp->shared);
#endif	VMINIT
  }

  if (checkacc && ((dp->access & wAccess) == 0)) InvlAccess();
  

  if (kvp->key.type == nameObj)
    { /* Since it's a name object, we can try the name cache... */
    pne = kvp->key.val.nmval;
    if (d.val.dictval == pne->dict)	/* found in cache... */
      {
      VMPutDValue(dp, pne->kvloc, &(kvp->value));	/* update value	*/
      if (top && (dp->access & rAccess) != 0
#if	VMINIT
	  && ! IsFontDirectory(kvp->key)
#endif	VMINIT
	 )
        pne->ts = timestamp;	/* make current */
      return;
      }
    }

  /* Couldn't get it thru the name cache. Do it the hard way. */
  if (SearchDict(dp, kvp->key, &SDres))
    VMPutDValue(dp, SDres, &(kvp->value));
  else {
    DictBody db;
    VMGetDict(&db, d);
    if (SDres == NIL)  {	/* dictionary is full */
    	if(db.curlength < db.maxlength) {
    		VMExpandDict(d, SearchDict);	/* expand the dictionary */
    		VMGetDict(&db, d);
    		SearchDict(dp, kvp->key, &SDres);
   	} else {
       		PSError(dictfull);
       	}
    }
    /* If the trace keys indicator hasn't been set and the key	*/
    /* is interesting to the GC, set the indicator. This only	*/
    /* needs to happen here, when inserting a new kv pair.	*/
    if (!db.privTraceKeys)
      db.privTraceKeys = GC_PrivGCMustTrace(&(kvp->key));
    VMPutDKeyVal(&db, SDres, kvp);
    db.curlength++;
    VMPutDict(d, &db);
  }

  if (kvp->key.type == nameObj) {
    pne->kvloc = SDres;
    pne->dict = d.val.dictval;
    pne->vec |= dp->bitvector;
    pne->ts.stamp = (top && ((dp->access & rAccess) != 0)
#if	VMINIT
		     && ! IsFontDirectory(kvp->key)
#endif	VMINIT
		    )? timestamp.stamp : 0;
      /* validate timestamp only if we know this is the topmost binding
         on the dict stack and the dictionary has at least read access */
  }
  return;
}

public procedure Def(key, value)  Object key, value;
{
Object ob;
KeyVal kv;
KeyName(key, &kv.key);
kv.value = value;
DTopP(&ob);
PutInDict(&kv,ob,true,true);
}  /* end of Def */

public procedure Begin(dict)  DictObj dict;
{
PDictObj ref = XlatDictRef(&dict);
if (ref->val.dictval->access == nAccess) InvlAccess();
BumpCETimeStamp();
DPushP(&dict);
}  /* end of Begin */

public procedure PSBegin()
{
DictObj ob;
PopPDict(&ob);
Begin(ob);
}  /* end of PSBegin */

public procedure End()
{
DictObj d;
DPopP(&d);
if (d.type != dictObj) TypeCheck();
BumpCETimeStamp();
}  /* end of End */

public procedure PSEnd()
{
  if (dictStk->head - dictStk->base == nPermDictEntries)
    PSError(dstkunderflow);
  End();
}

public procedure ClearDictStack()
{
  register PObject dictStkPermTop = dictStk->base + nPermDictEntries;
  while (dictStk->head != dictStkPermTop) End();
}

public procedure PSClearDictStack()
{
  ClearDictStack();
}

public procedure PSDef()
{
Object temp; KeyVal kv;
PopP(&kv.value);
PopP(&temp);
KeyName(temp, &kv.key);
DTopP(&temp);
PutInDict(&kv, temp, true, true);
}  /* end of PSDef */

public procedure PSLoad()
{
Object key, val;
register PNameEntry pne;
PopP(&key);
if (key.type == nameObj)
  {
  pne = key.val.nmval;
  if (ILoadPNE(pne, &val)) goto success;
  }
else if (Load(key, &val)) goto success;
KeyName(key, &key);
PushP(&key); stackRstr = false; Undefined();
success:
PushP(&val);
}  /* end of PSLoad */

public procedure DictP(maxlength,  pdobj)
cardinal maxlength;
register PDictObj pdobj;
{
  cardinal ds, curmax;
  DictBody db;

  if((pdobj == &trickyUserDict)
  	|| (pdobj == &trickyFontDirectory)
  	|| (pdobj == &rootShared->vm.Shared.sharedFontDirectory)
  	|| (pdobj == &trickyStatusDict))
  		curmax = INITSIZE;
  else
  		curmax = maxlength;
  AllocPDict(curmax, pdobj);
  VMGetDict(&db, *pdobj);
  db.maxlength = maxlength;
  db.curmax = curmax;
  db.curlength = 0;
  db.privTraceKeys = false;
  db.access = wAccess | rAccess;
  db.shared = CurrentShared();
  ds = rootShared->vm.Shared.dictCount % nBitVectorBits;
  if (ds == 0)
    { /* position 0 is reserved for sysDict */
    if (rootShared->vm.Shared.dictCount != 0)
      { /* control creates sysDict first */
      rootShared->vm.Shared.dictCount++; ds = 1;
      }
    }
  db.bitvector = 1<<ds;
  if (++rootShared->vm.Shared.dictCount == 0) rootShared->vm.Shared.dictCount++;
  VMPutDict(*pdobj, &db);
} /* end of DictP */

#if VMINIT
public procedure TrickyDictP(maxlength, pdobj)
  cardinal maxlength;
  PDictObj pdobj;
{
  cardinal i;
  DictObj dict;

  if (! CurrentShared()) FInvlAccess();

  /* Do not use entry 0 in trickyDicts--algorithm simplification */
  for (i = 1; i < rootShared->trickyDicts.length; i++)
    if (rootShared->trickyDicts.val.arrayval[i].type == nullObj)
      {
      DictP(maxlength, &dict);
      dict.val.dictval->tricky = true;
      VMPutElem(rootPrivate->trickyDicts, i, dict);
      VMPutElem(rootShared->trickyDicts, i, dict);
      LDictObj(*pdobj, (PDictBody)i);
      pdobj->length = 1;
      pdobj->shared = true;
      return;
      }
  LimitCheck();  /* too many tricky dicts defined */
}

private procedure PSTrickyDict()
{
  DictObj dobj;
  TrickyDictP(PopLimitCard(), &dobj);
  PushP(&dobj);
}
#endif VMINIT

public cardinal DictLength(d)  DictObj d;
{
PDictObj ref = XlatDictRef(&d);
PDictBody dp = ref->val.dictval;
if ((dp->access & rAccess) == 0) InvlAccess();
return dp->curlength;
}  /* end of DictLength */

private boolean dknown(d, name, checkacc)
	DictObj d; NameObj name; boolean checkacc;
{
  PKeyVal discard;
  PDictObj ref = XlatDictRef(&d);
  
  if (checkacc && ref->val.dictval->access == nAccess)
    InvlAccess();
  /* Try to find it in the name cache...	*/
  KeyName(name, &name);
  if ((name.type == nameObj)
    && (ref->val.dictval == name.val.nmval->dict)) {return(true);}
  /* Couldn't find it in cache, do full search	*/
  return(SearchDict(ref->val.dictval, name, &discard));
}

public boolean Known(d,name)
	DictObj d; NameObj name;
{
  return dknown(d, name, true);
}

public boolean ForceKnown(d,name)
	DictObj d; NameObj name;
{
  return dknown(d, name, false);
}

public procedure PSKnown()
{
Object name; DictObj d;
PopP(&name);
PopPDict(&d);
PushBoolean(Known(d,name));
}  /* end of PSKnown */

public procedure PSWhere()
{
TTLRec ttlval;  Object ob;
PopP(&ob);
if (trytoload(ob, &ttlval))
  {
  DictObj d;
  PDictObj pObj;
  LDictObj(d,ttlval.d);
  d.shared = d.val.dictval->shared;
  if (d.val.dictval->tricky) {
    for (pObj = dictStk->head-1; pObj >= dictStk->base; pObj--)
      if (TrickyDict(pObj) && DoDictXlat(pObj)->val.dictval == d.val.dictval)
        {d = *pObj; break;}
#if STAGE==DEVELOP
    Assert(TrickyDict(&d));
#endif STAGE==DEVELOP
    }
  PushP(&d);
  PushBoolean(true);
  }
else PushBoolean(false);
}  /* end of PSWhere */

public boolean DictTestP(d, key, pval, checkacc)
  DictObj d; Object key; boolean checkacc; PObject pval;
{
  PKeyVal SDres; Object name; PNameEntry pne; PDictBody dp;
  PDictObj ref = XlatDictRef(&d);
  dp = ref->val.dictval;
  KeyName(key, &name);
  if (checkacc && ((dp->access & rAccess) == 0)) InvlAccess();
  if (name.type == nameObj &&
      (pne = name.val.nmval)->dict == ref->val.dictval)
    VMGetValue(pval, pne->kvloc);
  else if (SearchDict(dp, name, &SDres))
    VMGetValue(pval, SDres);
  else return false;
  MapFontDirectory(d.val.dictval, pval);
  return true;
}

public procedure DictGetP(d, key, pval)  DictObj d; Object key; PObject pval;
{if(!DictTestP(d,key,pval,true)) {PushP(&key); stackRstr=false; Undefined();}}

public procedure ForceGetP(d, key, pval)  DictObj d; Object key; PObject pval;
{if(!DictTestP(d,key,pval,false)) {PushP(&key); stackRstr=false; Undefined();}}

public procedure DictPut(d,key,value)  DictObj d; Object key, value;
{
KeyVal kv;
kv.value = value;
KeyName(key, &kv.key);
PutInDict(&kv,d,false,true);
}  /* end of DictPut */

public procedure ForcePut(d,key,value)  DictObj d; Object key,value;
{
KeyVal kv;
kv.value = value;
KeyName(key, &kv.key);
PutInDict(&kv,d,false,false);
}  /* end of ForcePut */

public procedure PSDict()
{
DictObj dobj;
DictP(PopLimitCard(), &dobj);
PushP(&dobj);
}  /* end of PSDict */

public procedure PSMaxLength()
{
PDictBody dp; DictObj d, *ref;
PopPDict(&d);
ref = XlatDictRef(&d);
dp = ref->val.dictval;
if ((dp->access & rAccess) == 0) InvlAccess();
PushCardinal(dp->maxlength);
}  /* end of PSMaxLength */

public procedure PSStore()
{
Object temp; KeyVal kv; TTLRec ttlval; DictObj d;
PopP(&kv.value);
PopP(&temp);
KeyName(temp, &kv.key);
if (trytoload(kv.key,&ttlval))
  {LDictObj(d,ttlval.d);
   d.shared = d.val.dictval->shared;}
else DTopP(&d);
PutInDict(&kv,d,true,true);
}  /* end of PSStore */

private procedure UnDef(dict, key, checkAccess)
  DictObj dict; Object key; boolean checkAccess;
{
  PDictBody dp;
  PKeyVal kvloc;
  register PKeyVal kvp, jkvloc, rkvloc;
  DictBody db;
  KeyVal freekv;
  register PNameEntry pne;

  dict = *XlatDictRef(&dict);
  dp = dict.val.dictval;

  if (checkAccess && (dp->access & wAccess) == 0) InvlAccess();

  if (dp->shared && dp->tricky && enableTrickyDictCopy) {
    CopyTrickyDict(&dict);
    dp = dict.val.dictval;
#if	VMINIT
    Assert (dp->tricky && !dp->shared);
#endif	VMINIT
  }

  KeyName(key, &key);
  if (key.type == nameObj &&
      (pne = key.val.nmval)->dict == dict.val.dictval) {
    kvloc = pne->kvloc;
    pne->kvloc = NIL;
    pne->dict = NIL;
    pne->ts.stamp = 0;
    }
  else if (! SearchDict(dp, key, &kvloc))
    return;  /* undef of nonexistent key is a no-op */

  /* Mark entry free; then rehash any existing entries whose reprobe
     sequence would hit the deleted entry.
     (Knuth, vol. 3, section 6.4, algorithm R) */
  LNullObj(freekv.key);
  LNullObj(freekv.value);
  while (true) {
    VMPutDKeyVal(dp, kvloc, &freekv);
    jkvloc = kvloc;
    while (true) {
      if (++kvloc >= dp->end) kvloc = dp->begin;
      kvp = kvloc;
      if (kvp->key.type == nullObj) goto done;
      rkvloc = &dp->begin[HashObject(kvp->key, dp->size)];
      if ((kvloc < jkvloc)?
          rkvloc > kvloc && rkvloc <= jkvloc :
	  rkvloc <= jkvloc || rkvloc > kvloc)
        break;
      }
    VMPutDKeyVal(dp, jkvloc, kvp);  /* rehash */
    if (kvp->key.type == nameObj &&
        (pne = kvp->key.val.nmval)->dict == dict.val.dictval)
      pne->kvloc = jkvloc;
    }
done:
  VMGetDict(&db, dict);
  db.curlength--;
  VMPutDict(dict, &db);
}

public procedure DictUnDef(dict, key)
  DictObj dict; Object key;
{
  UnDef(dict, key, true);
}

public procedure ForceUnDef(dict, key)
  DictObj dict; Object key;
{
  UnDef(dict, key, false);
}

public procedure PSUnDef()
{
  DictObj dict;
  Object key;
  PopP(&key);
  PopPDict(&dict);
  UnDef(dict, key, true);
}

public procedure PSCrDict()
{Object ob;  DTopP(&ob);  PushP(&ob);}

typedef struct {
	Object key,value; 
	cardinal new; 
	AryObj kv;
} DFArec, *DFArecPtr;

private NextKeyVal(d,old,dap)
  DictObj d; cardinal old; DFArecPtr dap;
 /*
    increments old to location of next non-null keyval and returns new index
    (via dfarec) with the key and value of the keyval.  Initial call should
    have old=0. Returns new=0 if no more keyvals
  */
{
  PDictObj ref = XlatDictRef (&d);
  PDictBody dp = ref->val.dictval;
  PKeyVal	kvloc;
  KeyVal keyval;
  PNameEntry pne;
  PKeyVal SDres;
  int found,size;
 
  if (old == 0) {
    dap->new = 0;
    dap->kv.type = arrayObj;
    dap->kv.val.arrayval = (PObject) dp->begin;
    dap->kv.length = 2 * dp->size;
    dap->kv.level = level;    
  }
   
  size = (dap->kv.length)/2;
  kvloc = (PKeyVal) &dap->kv.val.arrayval[2 * dap->new];
  
  while (dap->new < size) {
    VMGetKeyVal (&keyval, kvloc);
    dap->new++;
    if (keyval.key.type != nullObj) {
    	if (dap->kv.val.arrayval != (PObject) dp->begin) { /* dict expanded */
	    found = false;
	    if (keyval.key.type == nameObj) {
		   /* Since it's a name object, we can try the name cache... */
		    pne = keyval.key.val.nmval;
    		    if (d.val.dictval == pne->dict) {   /* found in cache... */
			VMGetKeyVal(&keyval, pne->kvloc);
			found = true;
	           }
	    } 
	    if (!found) { /* Couldn't get it thru the name cache. */
	    	if (SearchDict(dp, keyval.key, &SDres))
	    		VMGetKeyVal (&keyval, SDres);
		else
			CantHappen();
	    }
	}    
	dap->key = keyval.key;
	dap->value = keyval.value;
	return;
    }
    kvloc++;
  }
  Assert (kvloc == (PKeyVal) &dap->kv.val.arrayval[dap->kv.length]);
  dap->new = 0;
  return;
}

private procedure DFAProc()
{
integer old = EPopInteger();
Object ob; DictObj d; DFArec dr;
if ((old < 0) || (old > MAXcardinal)) TypeCheck();
EPopP(&dr.kv);
EPopP(&ob);
ETopP(&d);
if (d.type != dictObj) TypeCheck();
dr.new = old;
NextKeyVal(d, (cardinal)old, &dr);
if (dr.new == 0)
  {
  EPopP(&d);
  if (d.type != dictObj) TypeCheck();
  return;
  }
PushP(&dr.key);
PushP(&dr.value);
EPushP(&ob);
EPushP(&dr.kv);
EPushInteger((integer)dr.new);
EPushP(&dfacmd);
EPushP(&ob);
}  /* end of DFAProc */

public procedure DictForAll(dictOb, procOb)
  DictObj dictOb; AryObj procOb;
{
Object dummy;
PDictObj ref = XlatDictRef(&dictOb);
if ((ref->val.dictval->access & rAccess) == 0)
  InvlAccess();
EPushP(&dictOb);
EPushP(&procOb);
EPushP(&dummy);
EPushInteger((integer)0);
DFAProc();
}  /* end of DictForAll */

public PKeyVal EnumerateDict(d, proc, data)
  DictObj d; PDictEnumProc proc; char *data; {
  PDictObj xobj;
  PKeyVal kvp;
  PDictBody dp;
  xobj = XlatDictRef(&d);
  dp = xobj->val.dictval;
  for (kvp = dp->begin; kvp < dp->end; kvp++) {
    if (kvp->key.type != nullObj && (*proc)(data, kvp)) {
      return kvp;}
    }
  return NIL;
}

private procedure DictFinalize(); /* forward declaration */

public procedure SetDictAccess(d, access)  DictObj d; Access access;
{
  DictBody db;
  PDictObj ref = XlatDictRef(&d);
  VMGetDict(&db, *ref);
  if ((db.access & ~access) == 0) return;
  if ((db.access & wAccess) == 0) InvlAccess();
  db.access &= access;
  VMPutDict(*ref, &db);
  if ((access & rAccess) == 0) DictFinalize(d, fr_restore);
    /* discard name bindings for this dict, since it is no longer readable
       (this is overkill, but simplest) */
}  /* end of SetDictAccess */


public procedure ResetNameCache(action)
  register integer action;
    /* 0 => zap all timestamps
       1 => flush all bindings
       2 => zap bitvectors in preparation for rebuilding them
       3 => zap timestamps for current context only
    */
{
  register PNameEntry *pno, pne;
  register integer i;

  ForAllNames(pno, pne, i)
    {
    switch (action) {
      case 1:
	pne->kvloc = NIL;
	pne->dict = NIL;
	/* fall through */
      case 0:
	pne->ts.stamp = 0;
	break;
      case 2:
	pne->vec = 0;
	break;
      case 3:
        if (pne->ts.id.index == timestamp.id.index) pne->ts.stamp = 0;
	break;
      }
    }
}


private procedure AboutToCollectShared(clientData, info)
RefAny clientData;
GC_Info info;
/* Called at the beginning of a shared collection while shared roots are
   being registered. We do not have any shared roots to register;
   the purpose of this procedure is to reset all bitvectors in the
   name cache in preparation for rebuilding them (see DictFinalize). */
{
  ResetNameCache((integer)2);
}


private procedure DictFinalize(obj, reason)
  Object obj; register FinalizeReason reason;
/* This is called when a dictionary is reclaimed for any reason.
   It is also called at the end of a shared VM collection for each
   surviving dictionary, and from SetDictAccess (above). */
{
  PDictBody dp;
  register PNameEntry pne;
  register PKeyVal pkv, pkvend;

  /* If dict is being reclaimed, invalidate cache entries for keys.
     If it is not being reclaimed, enumerate keys and update bitvectors
     in name cache. */
  dp = obj.val.dictval;
  pkvend = dp->end;
  for (pkv = dp->begin; pkv < pkvend; pkv++)
    if (pkv->key.type == nameObj)
      {
      pne = pkv->key.val.nmval;
      switch (reason)
        {
	case fr_restore:
	case fr_privateReclaim:
	case fr_sharedReclaim:
	case fr_destroyVM:
	  if (pne->dict == obj.val.dictval)
	    {pne->kvloc = NIL; pne->dict = NIL; pne->ts.stamp = 0;}
	  break;

	case fr_sharedEnum:
          pne->vec |= dp->bitvector;
	  break;

	default:
	  CantHappen();
	}
      }
}

public procedure DictCtxDestroy()
/* Called when a context is destroyed */
{
  /* Invalidate name bindings for current context only */
  ResetNameCache((integer) 3);

  /* Enable copy-on-write semantics for tricky dicts when initialization
     context is destroyed, unless wholeCloth init is to continue. */
  enableTrickyDictCopy = ! vmShared->wholeCloth;
}


public procedure DictInit(reason)  InitReason reason;
{
switch (reason)
  {
  case init:
#if (OS == os_mpw)
    globals = (Globals)os_sureCalloc(sizeof(GlobalsRec),1);
#endif (OS == os_mpw)
    GC_RgstSharedRootsProc(AboutToCollectShared, (RefAny)NIL);
    VMRgstFinalize(dictObj, DictFinalize, frset_reclaim | 1<<fr_sharedEnum);
    break;
  case romreg:
    nPermDictEntries = dictStk->head - dictStk->base;
    RgstMark("@dictforall", DFAProc, (integer)(mrk4Args), &dfacmd);
#if VMINIT
    if (vmShared->wholeCloth) {
      Begin(rootShared->vm.Shared.internalDict);
      RgstExplicit("trickydict", PSTrickyDict);
      End();
    }
#endif VMINIT
#if STAGE==DEVELOP
    if (vSTAGE==DEVELOP) RgstExplicit("probestats", PSProbeStats);
#endif STAGE==DEVELOP
    break;
  case ramreg:
  case restart:
    nPermDictEntries = dictStk->head - dictStk->base;
    break;
  endswitch
  }
}  /* end of DictInit */

