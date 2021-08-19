/*
  fontcache.c

Copyright (c) 1984-1990 Adobe Systems Incorporated.
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
Ed Taft: Wed Jan 17 16:00:53 1990
Ivor Durham: Sun Aug 14 14:00:13 1988
Jim Sandman: Thu Mar 22 12:18:50 1990
Perry Caro: Wed Nov  9 14:16:42 1988
Joe Pasqua: Tue Feb 28 13:43:07 1989
Bill Bilodeau: Fri Jan 20 16:45:41 PST 1989
Rick Saenz: Mon Feb 12 20:20:02 1990
End Edit History.
*/

#include PACKAGE_SPECS

#include COPYRIGHT

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

#if STAGE==DEVELOP
public boolean fcdebug, fchange, fcCheck;
#endif STAGE==DEVELOP

public PMTItem MT, MTEnd;

public PNameObj fontsNames;


public procedure GetFontDirectory (pFontDict)
  PDictObj pFontDict;
{
  SysDictGetP (fontsNames[nm_FontDirectory], pFontDict);
  if (TrickyDict (pFontDict))
    *pFontDict = *XlatDictRef (pFontDict);
}

public procedure GetPrivFontDirectory (pFontDict)
  PDictObj pFontDict;
{
  *pFontDict = rootPrivate->trickyDicts.val.arrayval[tdFontDirectory];
}




private FID nextFID;

private boolean FindMaxFID(data, kvp) char *data; PKeyVal kvp; {
  FID ufid;
  Object fid;
  FID *maxFID = (FID *) data;
  if (kvp->value.type == dictObj) {
    DictGetP(kvp->value, fontsNames[nm_FID], &fid);
    if (TypeOfFID(fid.val.fontval) == ndcFID) {
      ufid = (fid.val.fontval & 0xffffff) + 1;
      *maxFID = MAX(*maxFID,ufid);
      }
    }
  return false;
  }

private procedure InitFID() {
  DictObj FD; 
  nextFID = 0;
#if (LANGUAGE_LEVEL >= level_2)
  if (vLANGUAGE_LEVEL >= level_2)
    (void) EnumerateDict(
      rootShared->vm.Shared.sharedFontDirectory, FindMaxFID, (char *)&nextFID);
#endif
  GetPrivFontDirectory(&FD);
  (void) EnumerateDict(FD, FindMaxFID, (char *)&nextFID);
  }  /* end of InitFID */

private procedure GenFID(type, val, pfo, adjust)
  integer type, val, adjust;  register PFontObj pfo;
{
if (type == ndcFID)  {val = nextFID; nextFID++; adjust = 0;}
LFontObj(*pfo,val);
pfo->val.ival |= (type|adjust)<<shiftFIDType;
}  /* end of GenFID */

typedef struct {
  PKeyVal *kvpArray;
  integer nLen;
  boolean matchingUID, sibling, uidknown;
  Object newuid;
  PDictBody ndp;
  PObject retobj;
  } FindSibData;
  
private boolean FindSibling(data, FDkvp) char *data; PKeyVal FDkvp; {
  Object uid;
  integer i;
  Object fontFD;
  PKeyVal kvp;
  register FindSibData *sd = (FindSibData *) data;
  fontFD = FDkvp->value;
  if (fontFD.type == dictObj) {
    if (!sd->matchingUID && sd->uidknown)
      {
      if (DictTestP(fontFD, fontsNames[nm_UniqueID], &uid, true))
        if (uid.val.ival == sd->newuid.val.ival) sd->matchingUID = true;
      }
    if (!(Known(fontFD, fontsNames[nm_FontName]))) return false;
    if (!(Known(fontFD, fontsNames[nm_FontInfo]))) return false;
    for (i = 0; i < sd->nLen; i++)
      {
      Object obj, temp2; /* to avoid CT compiler error */
      kvp = sd->kvpArray[i];
      temp2 = kvp->key;
      if ( DictTestP(fontFD, temp2, &obj, true) )
        {
        if (obj.type != kvp->value.type) return false;
        if (obj.val.ival != kvp->value.val.ival) return false;
        }
      /* only kludge to test whole 4-byte union element */
      else return false;
      }
    DictGetP(fontFD, fontsNames[nm_FID], sd->retobj);
    if (TypeOfFID(sd->retobj->val.fontval) != ndcFID)
      {sd->sibling = true; return true;}
    }
  return false;
  }


private procedure FndFntSibling(newfnt, retobj, fidAdjust)
  DictObj newfnt;  PObject retobj; integer fidAdjust;
{
  /*	On the generating of FIDs for fonts:
	AdobeFont (i.e. FontType != 3)?
	  yes:
	    (top-level UID == private UID) && UID known?
	      yes:
		matches UID of some font already in FD?
		  yes:
		    sibling?
		      yes: give sibling's FID
		      no: give no-disk-cache FID
		  no: make Adobe FID from UID
	      no: is UID known?
		yes make user FID from UID
		no: give no-disk-cache FID (an "old" Adobe font)
	  no:
	    is UID known and NOT an ndcFID?
	      yes: make user FID from UID
	      no: make no-disk-cache FID
  */
PDictBody ndp;
register PKeyVal nkvp; Object obj;
DictObj FD; register integer i;
IntObj fnttype;
FindSibData sd;
DictGetP(newfnt, fontsNames[nm_FontType], &fnttype);
if (sd.uidknown = DictTestP(newfnt, fontsNames[nm_UniqueID], &sd.newuid, true))
  if (sd.newuid.type != intObj) sd.uidknown = false;
if (!sd.uidknown)
  {
  GenFID((integer)ndcFID, (integer)0, retobj, fidAdjust);
  return;
  }
if (fnttype.val.ival == 3) {
  GenFID((integer)userFID, sd.newuid.val.ival, retobj, fidAdjust); return;}
ForceGetP(newfnt, fontsNames[nm_Private], &obj);
if (ForceKnown(obj, fontsNames[nm_UniqueID]))
  ForceGetP(obj, fontsNames[nm_UniqueID], &obj);
else obj = NOLL;
if ((obj.type != intObj) || (obj.val.ival != sd.newuid.val.ival))
  {GenFID((integer)userFID, sd.newuid.val.ival, retobj, fidAdjust);  return;}
ndp = newfnt.val.dictval;
sd.kvpArray = (PKeyVal *)NEW((integer)ndp->curlength, sizeof(PKeyVal));
i = 0;
for (nkvp = ndp->begin; nkvp != ndp->end; nkvp++)
  {
  if (nkvp->key.type != nullObj)
    {
    if (nkvp->key.type == nameObj)
      {
      if (nkvp->key.val.nmval == fontsNames[nm_Encoding].val.nmval) continue;
      if (nkvp->key.val.nmval == fontsNames[nm_FontName].val.nmval) continue;
      if (nkvp->key.val.nmval == fontsNames[nm_FontMatrix].val.nmval) continue;
      if (nkvp->key.val.nmval == fontsNames[nm_FontInfo].val.nmval) continue;
      if (nkvp->key.val.nmval == fontsNames[nm_BitmapWidths].val.nmval) continue;
      if (nkvp->key.val.nmval == fontsNames[nm_ExactSize].val.nmval) continue;
      if (nkvp->key.val.nmval == fontsNames[nm_InBetweenSize].val.nmval) continue;
      if (nkvp->key.val.nmval == fontsNames[nm_TransformedChar].val.nmval) continue;
      }
    sd.kvpArray[i++] = nkvp;
    }
  }
sd.nLen = i;
sd.retobj = retobj;
sd.matchingUID = false;
sd.sibling = false;
#if (LANGUAGE_LEVEL >= level_2)
if (vLANGUAGE_LEVEL >= level_2)
  (void) EnumerateDict(
    rootShared->vm.Shared.sharedFontDirectory, FindSibling, (char *)&sd);
#endif
if (!sd.sibling) {
  GetPrivFontDirectory (&FD);
  (void) EnumerateDict(FD, FindSibling, (char *)&sd);
  }
FREE(sd.kvpArray);
if (sd.matchingUID)  {
  if (!sd.sibling) GenFID((integer)ndcFID, (integer)0, retobj, fidAdjust);
  else if (fidAdjust)
    GenFID((integer)adobeFID, retobj->val.ival, retobj, fidAdjust);
  }
else GenFID((integer)adobeFID, sd.newuid.val.ival, retobj, fidAdjust);
}  /* end of FndFntSibling */


private procedure PurgeFontRefs(dict)
  DictObj dict;
/* Purges all makefont MIDs that refer to the specified font dict */
{
  register MID m, fm; register PMTItem mp; register MID *map;
  forallMM(map)
    {
    m = *map;
    while (m != MIDNULL)
      {
      mp = &MT[m];
      DebugAssert(mp->mfmid);
      if (dict.val.dictval == mp->values.mf.origdict.val.dictval ||
          dict.val.dictval == mp->values.mf.mfdict.val.dictval)
        {
	fm = m;
	m = mp->mlink;
	PurgeMID(fm);
	}
      else m = mp->mlink;
      }
    }
}


public procedure PSUnDefineFont()
{
  DictObj oldfdict, FD; Object fname;
  GetFontDirectory(&FD);
  PopP(&fname);
  if (fname.type == strObj) StrToName(fname, &fname);
  if (DictTestP(FD, fname, &oldfdict, true))
    {
    PurgeFontRefs(oldfdict);
    PurgeSFForKey(fname);  /* this is overkill, but simplest */
    }
  ForceUnDef(FD, fname);
}  /* end of PSUnDefineFont */


private procedure DictGetPType (d, n, po, t)
  DictObj d; NameObj n; PObject po; integer t;
{
DictGetP (d, n, po);
if ((*po).type != t) TypeCheck ();
}

private boolean DictTestPType (d, n, po, t)
  DictObj d; NameObj n; PObject po; integer t;
{
if (!DictTestP(d, n, po, true)) return false;
if ((*po).type != t) TypeCheck ();
return true;
}

private integer GetBuildingOption(fdict, name, dflt)
  DictObj fdict; integer name, dflt; {
  Object obj;
  if (DictTestPType(fdict, fontsNames[name], &obj, intObj))
    {
    if ((obj.val.ival > 2) || (obj.val.ival < 0)) InvlFont();
    return obj.val.ival;
    }
  else return dflt;
  }

private integer CheckPrebuiltOptions(fdict) DictObj fdict; {
  integer fidAdjust = 0;
  Object obj;
  fidAdjust = GetBuildingOption(
    fdict, (integer)nm_ExactSize, (integer)PREBUILT_CLOSEST) *
    exactSizeFIDFactor;
  fidAdjust += GetBuildingOption(
    fdict, (integer)nm_InBetweenSize, (integer)PREBUILT_CLOSEST) *
    inBetweenSizeFIDFactor;
  fidAdjust += GetBuildingOption(
    fdict, (integer)nm_TransformedChar, (integer)PREBUILT_OUTLINE) *
    transformedCharFIDFactor;
  fidAdjust <<= transformFIDShift;
  if (DictTestP(fdict, fontsNames[nm_BitmapWidths], &obj, true))
    {
    if (obj.type != boolObj)
      InvlFont();
    fidAdjust |= useBitmapWidthsFID;
    }
  return fidAdjust;
  }

private integer FDNestedDepth (fdict)
  DictObj fdict;
{
  IntObj fntype;
  AryObj fvec;
  integer i, max, temp;
  max = 0;
  DictGetP (fdict, fontsNames[nm_FontType], &fntype);
  if (fntype.val.ival == COMPOSEDtype)
  {
    DictGetP (fdict, fontsNames[nm_FDepVector], &fvec);
    for (i = 0; i < fvec.length; i++)
    {
      temp = FDNestedDepth (VMGetElem (fvec, i));
      if (temp > max) max = temp;
    }
  }
  return (max + 1);
}

private procedure AllocRAMStr(nBytes, shared, pobj)
  integer nBytes; boolean shared; PStrObj pobj; {
  boolean origShared = CurrentShared();
  PCard8 p;
  SetShared(shared);
  DURING
    p = AllocAligned(nBytes);
    LStrObj(*pobj, nBytes, p);
  HANDLER
    SetShared(origShared);
    RERAISE;
  END_HANDLER;
  SetShared(origShared);
  }

private procedure AllocRAMArray(nItems, shared, pobj)
  integer nItems; boolean shared; PStrObj pobj; {
  boolean origShared = CurrentShared();
  PCard8 p;
  SetShared(shared);
  DURING
    AllocPArray(nItems, pobj);
  HANDLER
    SetShared(origShared);
    RERAISE;
  END_HANDLER;
  SetShared(origShared);
  }

private procedure CheckCompositeFont(fdict, enc, fvec)
  DictObj fdict; AryObj enc; PAryObj fvec; {
  IntObj fmap, fint; AryObj pfe, ensupp;
  DictObj fdsupp;
  StrObj midarray, so;
  UniqueMID *SPtr;
  integer i; Mtx m;
  PCard8 mtpe;
  if (!DictTestPType (fdict, fontsNames[nm_FMapType], &fmap, intObj) ||
      !DictTestPType (fdict, fontsNames[nm_FDepVector], fvec, arrayObj))
     InvlFont();
  if (!DictTestPType (fdict, fontsNames[nm_PrefEnc], &pfe, arrayObj))
    {
    pfe = iLAryObj;
    pfe.shared = fdict.shared;
    ForcePut (fdict, fontsNames[nm_PrefEnc], pfe);
    }
  if (enc.length == 0) {
    midarray = enc; /* get a zero length array */
    }
  else {
    for (i = 0; i < enc.length; i++)
      {
      fint = VMGetElem (enc, i);
      if (fint.type != intObj) TypeCheck ();
      if ((fint.val.ival < 0) || (fint.val.ival >= fvec->length)) RangeCheck ();
      }
    AllocRAMStr(
      fvec->length * sizeof (UniqueMID) + BytesForMTPE(fvec->length),
      fdict.shared,
       &midarray);
    midarray.access = nAccess;
    os_bzero( (char *)midarray.val.strval, (long int)midarray.length);
    mtpe = midarray.val.strval;
    mtpe += fvec->length * sizeof (UniqueMID);
    for (i = 0; i < fvec->length; i++)
      {
      integer mtpeVal = 0;
      fdsupp = fvec->val.arrayval[i];
      if (fdsupp.type != dictObj) TypeCheck ();
      if (!fdsupp.val.dictval->isfont) InvlFont ();
	  /* also breaks any possible infinite recursion in show and makefont */
      DictGetP(fdsupp, fontsNames[nm_FontType], &fint);
      if (fint.val.ival == COMPOSEDtype)
	{
	DictGetP(fdsupp, fontsNames[nm_FMapType], &fint);
	if ((fint.val.ival == FMapEscape) && (fmap.val.ival != FMapEscape))
	  InvlFont();
	mtpeVal = fint.val.ival & MTMASK;
	}
      DictGetP (fdsupp, fontsNames[nm_Encoding], &ensupp);
      if ((ensupp.val.arrayval == pfe.val.arrayval) && (ensupp.length == pfe.length))
	mtpeVal |= PEMASK;
      SetMTPE(mtpe, i, mtpeVal);
      }
    }
  ForcePut(fdict, fontsNames[nm_MIDVector], midarray);
  AllocRAMStr(sizeof(UniqueMID), fdict.shared, &so);
  so.access = nAccess;
  ((UniqueMID*)so.val.strval)->stamp = 0;
  ForcePut(fdict, fontsNames[nm_CurMID], so);
  switch (fmap.val.ival)
    {
    case FMap17:
      break;
    case FMap88:
      break;
    case FMap97:
      break;
    case FMapEscape:
      if (DictTestPType (fdict, fontsNames[nm_EscChar], &fint, intObj))
	{
	if ((fint.val.ival < 0) || (fint.val.ival > 255)) RangeCheck ();
	}
      else
	{
	LIntObj (fint, 255);
	ForcePut (fdict, fontsNames[nm_EscChar], fint);
	}
      break;
    case FMapGen:
      if (!DictTestPType (fdict, fontsNames[nm_SubsVector], &so, strObj))
         InvlFont();
      if (so.length == 0) RangeCheck ();
      if (*so.val.strval > 3) LimitCheck ();
      if (1+(enc.length-1)*(*so.val.strval+1) != so.length)
        RangeCheck ();
      break;
    default:
      RangeCheck ();
    }
  if (FDNestedDepth (fdict) >= MaxSSS) LimitCheck ();
  }

private char * EncCharNameProc(index, nameLength, enc)
  integer index, *nameLength; PObject enc; {
  Object obj;
  if (index < 0 || enc->type != arrayObj || index >= enc->length) {
    *nameLength = 0; return NULL;
    }
  obj = VMGetElem(*enc, index);
  if (obj.type != nameObj ||
    obj.val.nmval == fontsNames[nm_Encoding].val.nmval) {
    *nameLength = 0; return NULL;
    }
  *nameLength = obj.val.nmval->strLen;
  return (char *)obj.val.nmval->str;
  }

private integer RgstPrebuiltInfo(fdict, fid) DictObj fdict; integer fid; {
  Object obj, stdEnc, isoEnc;
  char * name;
  integer nameLength;
  boolean stdEncoding;
  if (DictTestP(fdict, fontsNames[nm_FontName], &obj, true))
    {
    switch (obj.type) {
      case strObj: {
	name = (char *)obj.val.strval;
	nameLength = obj.length;
	break;
	}
      case nameObj: {
	name = (char *)obj.val.nmval->str;
	nameLength = obj.val.nmval->strLen;
	break;
	}
      default: {
	name = NIL;
	nameLength = 0;
	break;
	}
      }
    }
  else {
    name = (char *)NIL;
    nameLength = 0;
    }
  DictGetP(fdict, fontsNames[nm_Encoding], &obj);
  DictGetP(
    rootShared->vm.Shared.sysDict,
    fontsNames[nm_StandardEncoding], &stdEnc);
  DictGetP(
    rootShared->vm.Shared.sysDict,
    fontsNames[nm_ISOLatin1Encoding], &isoEnc);
  stdEncoding = Equal(obj, stdEnc) || Equal(obj, isoEnc);
  return DevRgstPrebuiltFontInfo(
    fid & prebuiltFIDMask, name, nameLength, stdEncoding,
    EncCharNameProc, &obj);
  }

private procedure PSDefineFont()
{
  DictObj fdict, oldfdict, FD; Object fname, uid; FontObj fid;
  integer fidAdjust, fidClient;
  AryObj enc, ao1;
  IntObj iObj, fntype; AryObj fvec;
  PopPDict(&fdict);
  PopP(&fname);
  if (Known(fdict, fontsNames[nm_FID]) ||
      ! Known(fdict, fontsNames[nm_Encoding]) ||
      ! Known(fdict, fontsNames[nm_FontMatrix]) ||
      ! Known(fdict, fontsNames[nm_FontType]) ||
      TrickyDict(&fdict))
    InvlFont();
  if (DictTestPType(fdict, fontsNames[nm_WMode], &iObj, intObj))
    if ((iObj.val.ival > maxwrtmodes) || (iObj.val.ival < 0)) RangeCheck();
  DictGetPType (fdict, fontsNames[nm_FontType], &fntype, intObj);
  DictGetPType (fdict, fontsNames[nm_Encoding], &enc, arrayObj);
  if (fntype.val.ival == COMPOSEDtype)
    CheckCompositeFont(fdict, enc, &fvec);
  DictGetPType (fdict, fontsNames[nm_FontMatrix], &ao1, arrayObj);
  if (ao1.length != 6) RangeCheck ();
  if (DictTestPType(fdict, fontsNames[nm_UniqueID], &uid, intObj))
    if ((uid.val.ival > 0xffffff) || (uid.val.ival < 0)) InvlFont();
  fidAdjust = CheckPrebuiltOptions(fdict);
  GetFontDirectory (&FD);
  if (fname.type == strObj) StrToName(fname, &fname);
  PurgeSFForKey(fname);  /* this is overkill, but simplest */
  if (DictTestP(FD, fname, &oldfdict, true))
    PurgeFontRefs(oldfdict);
  FndFntSibling(fdict, &fid, fidAdjust);
  fidClient = RgstPrebuiltInfo(
    fdict, (TypeOfFID(fid.val.fontval) != ndcFID) ? fid.val.fontval : 0);
  if (fidClient != 0 && TypeOfFID(fid.val.fontval) != adobeFID) {
    GenFID((integer)clientFID, fidClient, &fid, fidAdjust);
    }
  ForcePut(fdict, fontsNames[nm_FID], fid);
  fdict.val.dictval->isfont = true;
  SetDictAccess(fdict, rAccess);
  ConditionalInvalidateRecycler (&fdict);   /* Preclude move by ForcePut */
  if (fntype.val.ival == COMPOSEDtype)
    {
    DictObj fdsupp;
    integer i; Mtx m;
    AryObj newfv;
    PAryToMtx (&ao1, &m);
    if ((m.a != 1.0) || (m.d != 1.0) || (m.b != 0.0) || (m.c != 0.0))
      {
      for (i = 0; i < fvec.length; i++)
	{
	fdsupp = VMGetElem (fvec, i);
	DictGetPType (fdsupp, fontsNames[nm_FontType], &fntype, intObj);
	if (fntype.val.ival == COMPOSEDtype) goto makedesc;
	}
      goto nodesc;
    makedesc:
      AllocRAMArray ((int)fvec.length, fdict.shared, &newfv);
      for (i = 0; i < fvec.length; i++)
	{
	fdsupp = VMGetElem (fvec, i);
	DictGetP (fdsupp, fontsNames[nm_FontType], &fntype);
	if (fntype.val.ival == COMPOSEDtype) MakeFont (fdsupp, &m, &fdsupp);
	VMPutElem (newfv, i, fdsupp);
	}
      newfv.access = fvec.access;
      ForcePut (fdict, fontsNames[nm_FDepVector], newfv);
      }
    nodesc: ;
    }
  ForcePut(FD,fname,fdict);
  PushP(&fdict);
}  /* end of PSDefineFont */

#if 0
private procedure PSFlushCache() {
  MID m, *map;
  forallMS(map) {
    m = *map;
    while (m != MIDNULL) {
      if (FlushMID(m)) {
	PushBoolean(false);
	return;
      }
      m = MT[m].mlink;
    }
  }
  PushBoolean(true);
}
#endif

private boolean RgstInitialFont(data, kvp) char *data; PKeyVal kvp; {
  Object fid;
  if (kvp->value.type == dictObj) {
    DictGetP(kvp->value, fontsNames[nm_FID], &fid);
    if (TypeOfFID(fid.val.fontval) != ndcFID) {
      RgstPrebuiltInfo(kvp->value, fid.val.fontval);
      }
    }
  return false;
  }

#if STAGE==DEVELOP
private procedure PSFCDebug()
{
  fcdebug = PopBoolean();
}

private procedure PSFCCheck()
{
  fcCheck = PopBoolean();
}

#endif STAGE==DEVELOP

public procedure FontCacheInit(reason)
	InitReason reason;
{
  switch (reason){
    case init: {
#if STAGE==DEVELOP
      fcdebug = false; fchange = false; fcCheck = true;
#endif STAGE==DEVELOP
      InitSFCache();
      GC_RegisterFinalizeProc(PurgeOnGC, (RefAny)NIL);
      break;}
    case romreg: {
      fontsNames = RgstPackageNames((integer)PACKAGE_INDEX, (integer)NUM_PACKAGE_NAMES);

#include "ops_fonts1.c"
#include "ops_fontsInternal.c"
#if (LANGUAGE_LEVEL >= level_2)
      if (vLANGUAGE_LEVEL >= level_2)
#include "ops_fonts2.c"
#endif

#if STAGE==DEVELOP
      if (vSTAGE==DEVELOP) RgstExplicit("fcdebug",PSFCDebug);
      if (vSTAGE==DEVELOP) RgstExplicit("fcCheck",PSFCCheck);
#endif STAGE==DEVELOP

      Begin(rootShared->vm.Shared.internalDict);

#if 0
      RgstExplicit("flushcache", PSFlushCache);
#endif

#if VMINIT
      /* If undefinefont is not in the language level being supported, register
         it in internaldict because it's needed while building the VM. */
      if (vmShared->wholeCloth && vLANGUAGE_LEVEL < level_2)
        RgstExplicit("undefinefont", PSUnDefineFont);
#endif VMINIT
      End();
      RgstRstrProc(PurgeOnRstr);
      InitFID();
      break;}
    case ramreg: {
#if (LANGUAGE_LEVEL >= level_2)
      if (vLANGUAGE_LEVEL >= level_2)
        (void) EnumerateDict(
	  rootShared->vm.Shared.sharedFontDirectory,
	  RgstInitialFont, (char *)NIL);
#endif
      break;}
    endswitch}
}


private procedure FontDataHandler (code)
  StaticEvent code;
{
  extern procedure FontShwDataHandler (), FontBldDataHandler ();
  if (code == staticsSpaceDestroy)
    PurgeOnRstr(-1); /* restore to "level" -1 destroys all MIDs for space */
  FontShwDataHandler (code);
  FontBldDataHandler (code);
}


public PFontCtx fontCtx;

extern procedure FontDiskInit(), FontBuildInit(), FontShowInit();

public procedure FontsInit(reason)
  InitReason reason;
{
  switch (reason)
    {
    case init:
      RegisterData (
        (PCard8 *)&fontCtx, (integer)sizeof (FontCtx), FontDataHandler,
        (STATICEVENTFLAG (staticsCreate) |
	 STATICEVENTFLAG (staticsDestroy) |
	 STATICEVENTFLAG (staticsSpaceDestroy)));
      break;
    }

  FontCacheInit(reason);
  FontShowInit(reason);
  FontBuildInit(reason);
  FontDiskInit(reason);
}
