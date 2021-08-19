/*
  binaryobject.c

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

Original version: Ed Taft, May 1988
Edit History:
Ed Taft: Fri Oct  6 13:35:21 1989
Ivor Durham: Tue May 16 09:30:18 1989
Perry Caro: Mon Nov  7 17:22:02 1988
Jim Sandman: Wed Apr 12 14:49:18 1989
Joe Pasqua: Fri Jan  6 14:43:13 1989
Leo Hourvitz: Fri Nov 2 23:59:59 1990
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include FP
#include LANGUAGE
#include PSLIB
#include STREAM
#include RECYCLER
#include VM

#include "exec.h"
#include "langdata.h"
#include "name.h"
#include "scanner.h"

#define MAXBINOBJSEQLENGTH 0x480000

/* the symbol below enables code in WriteObject that does special things
   when writing object to make NeXT's code work correctly.  */
#ifndef	NEXTSTMSTUFF
#define NEXTSTMSTUFF 1
#endif	NEXTSTMSTUFF

#define bo_immEvalName 6	/* same as stmObj */
#define bo_mark 10		/* same as fontObj */
#define bo_max 10

#if (OS != os_mpw)
/*-- BEGIN GLOBALS --*/

private Card8 mapLType[bo_max+1], mapXType[bo_max+1];
  /* maps incoming type code to tag/access/type byte of object */

/* Map from object format to token type for binary object sequence */
private Card8 formatToType[] = {
  0, bt_objSeqHiIEEE, bt_objSeqLoIEEE, bt_objSeqHiNative, bt_objSeqLoNative};

/*-- END GLOBALS --*/
#else (OS != os_mpw)

typedef struct {
 Card8 g_mapLType[bo_max+1], g_mapXType[bo_max+1];
  /* maps incoming type code to tag/access/type byte of object */

/* Map from object format to token type for binary object sequence */
 Card8 g_formatToType[5]; /*  = {
  0, bt_objSeqHiIEEE, bt_objSeqLoIEEE, bt_objSeqHiNative, bt_objSeqLoNative}; */
} GlobalsRec, *Globals;

private Globals globals;

#define mapLType globals->g_mapLType
#define mapXType globals->g_mapXType
#define formatToType globals->g_formatToType

#endif (OS != os_mpw)

private procedure InitBOSTypeMap(binObjType, pobj)
  Card8 binObjType; PObject pobj;
{
  UObject tempObj;
  DebugAssert(binObjType <= bo_max);
  tempObj.o = *pobj;
  tempObj.o.tag = Lobj;
  mapLType[binObjType] = tempObj.b.type;
  tempObj.o.tag = Xobj;
  mapXType[binObjType] = tempObj.b.type;
}


/* SWAP4(PCard32 pValue); -- swaps bytes of *pValue in place. */
#define SWAP4(pValue) { \
  register Card8 c0, c1; \
  c0 = ((PCard8) (pValue))[0]; c1 = ((PCard8) (pValue))[1]; \
  ((PCard8) (pValue))[0] = ((PCard8) (pValue))[3]; \
  ((PCard8) (pValue))[1] = ((PCard8) (pValue))[2]; \
  ((PCard8) (pValue))[2] = c1; ((PCard8) (pValue))[3] = c0; \
  }

/* SWAP2(PCard16 pValue); -- swaps bytes of *pValue in place. */
#define SWAP2(pValue) { \
  register Card8 c0; \
  c0 = ((PCard8) (pValue))[0]; \
  ((PCard8) (pValue))[0] = ((PCard8) (pValue))[1]; \
  ((PCard8) (pValue))[1] = c0; \
  }


private procedure SyntaxError(problem)
  char *problem;
{
  psERROR = syntaxerror;
  RAISE(PS_ERROR, problem);
}


public procedure ReadBinObjSeq(stm, hdr, ret)
  Stm stm; PBOSHeader hdr; PObject ret;
{
  register PUObject pobj, pAryMax;
  Card32 bsize;
  register boolean swap;
  register Card8 type;
  PCard8 pStrMin;
  integer nTop, headerSize;
  PUObject pbos;
#if (! IEEEFLOAT)
  procedure (*ConvReal)(/* FloatRep *from, real *to */) = NIL;
#endif (! IEEEFLOAT)
  UObject tempObj;
  Card8 simplePad, compositePad;

  switch (hdr->type)
    {
    case bt_objSeqHiIEEE:
#if (! IEEEFLOAT)
      ConvReal = IEEEHighToNative;
#endif (! IEEEFLOAT)
      /* fall through */

    case bt_objSeqHiNative:
      swap = SWAPBITS;
      break;

    case bt_objSeqLoIEEE:
#if (! IEEEFLOAT)
      ConvReal = IEEELowToNative;
#endif (! IEEEFLOAT)
      /* fall through */

    case bt_objSeqLoNative:
      swap = ! SWAPBITS;
      break;

    default:
      CantHappen();
    }

  if (swap) SWAP2(&hdr->size);
  if ((nTop = hdr->lenTopArray) == 0) {
    nTop = hdr->size;
    if (fread((char *) &bsize, 1, sizeof(Int32), stm) != sizeof(Int32))
      SyntaxError("premature end");
    if (swap) SWAP4(&bsize);
    headerSize = sizeof(BOSHeader) + sizeof(Int32);
    }
  else {
    bsize = hdr->size;
    headerSize = sizeof(BOSHeader);
    }
  if (bsize < (headerSize+sizeof(Object)))
    SyntaxError("sequence too short");
  bsize -= headerSize;
  if (nTop * sizeof(Object) > bsize)
    SyntaxError("array too long");
  if (bsize > MAXBINOBJSEQLENGTH)
    SyntaxError("bin obj seq length too long");
  XAryObj(*ret, nTop, (PObject)AllocAligned(bsize));
  /* Because the XAryObj bypasses the normal AllocPArray way of creating
     an array object, it does not pick up all the attributes of an array.
     In particular, it misses the seen bit being set correctly.  So, we
     have to simulate what AllocPArray would do here.
     Leovitch 02Nov90 */
  ret->seen = !vmCurrent->valueForSeen;
  pbos = (PUObject) ret->val.arrayval;
  if (fread((char *) pbos, 1, bsize, stm) != bsize)
    SyntaxError("premature end");

  compositePad = ((PBObject) ret)->pad;
  tempObj.b.pad = compositePad;
  tempObj.o.shared = true;
  simplePad = tempObj.b.pad;

  pAryMax = pbos + nTop;
  pStrMin = (PCard8) pbos + bsize;
  for (pobj = pbos; pobj < pAryMax; pobj++)
    {
    type = pobj->b.type;
    if (type >= 128) {type -= 128; pobj->b.type = mapXType[type];}
    else pobj->b.type = mapLType[type];
    if (pobj->b.pad != 0) goto NonZeroUnused;

    switch (type)
      {
      case nullObj:
        if (pobj->b.length != 0 || pobj->b.value != 0) goto NonZeroUnused;
        pobj->b.pad = simplePad;
	break;

      case intObj:
	if (pobj->b.length != 0) goto NonZeroUnused;
        pobj->b.pad = simplePad;
	if (swap) SWAP4(&pobj->b.value);
	break;

      case realObj:
        pobj->b.pad = simplePad;
	if (pobj->b.length != 0)
	  { /* fixed number; convert to float */
	  if (swap)
	    {
	    SWAP2(&pobj->b.length);
	    SWAP4(&pobj->b.value);
	    }
	  if (pobj->b.length > 31) SyntaxError("bad fixed scale");
	  pobj->o.val.rval = os_ldexp((double) pobj->o.val.ival,
	  			      - (integer) pobj->b.length);
	  break;
	  }
	/* float to begin with, but representation may need to be adjusted */
#if IEEEFLOAT
	if (swap) SWAP4(&pobj->b.value);
#else IEEEFLOAT
	if (ConvReal != NIL)
	  {
	  tempObj.b.value = pobj->b.value;
	  (*ConvReal)(&tempObj.b.value, &pobj->b.value);
	    /* this can cause an ecUndefResult exception */
	  }
	else
#endif IEEEFLOAT
	if (! IsValidReal(&pobj->o.val.rval)) UndefResult();
	break;

      case boolObj:
	if (pobj->b.length != 0) goto NonZeroUnused;
        pobj->b.pad = simplePad;
	if (swap) SWAP4(&pobj->b.value);
	if ((Card32) pobj->b.value > 1) SyntaxError("bad boolean value");
	break;

      case bo_mark:
        if (pobj->b.length != 0 || pobj->b.value != 0) goto NonZeroUnused;
        pobj->b.length = objMark;
        pobj->b.pad = simplePad;
	break;

      case strObj:
        pobj->b.pad = compositePad;
        if (swap)
	  {
	  SWAP2(&pobj->b.length);
	  SWAP4(&pobj->b.value);
	  }
	if (pobj->b.length == 0) pobj->b.value = NIL;
	else
	  {
	  register PCard8 ptr = (PCard8) pbos + pobj->b.value;
	  if (pobj->b.value + pobj->b.length > bsize ||
	      ptr < (PCard8) pAryMax)
	    SyntaxError("string out of bounds");
	  if (ptr < pStrMin) pStrMin = ptr;
	  pobj->b.value = (Card32) ptr;
	  }
	break;

      case arrayObj:
        pobj->b.pad = compositePad;
        if (swap)
	  {
	  SWAP2(&pobj->b.length);
	  SWAP4(&pobj->b.value);
	  }
	if (pobj->b.length == 0) pobj->b.value = NIL;
	else
	  {
	  register PUObject ptr;
	  ptr = (PUObject) ((PCard8) pbos + pobj->b.value);
	  if (ptr < pbos ||
	      (pobj->b.value & (sizeof(Object) - 1)) != 0)
	    SyntaxError("bad array offset");
	  pobj->b.value = (Card32) ptr;
	  ptr += pobj->b.length;
	  if ((PCard8) ptr > pStrMin)
	    SyntaxError("array out of bounds");
	  if (ptr > pAryMax) pAryMax = ptr;
	  }
	break;

      case nameObj:
      case bo_immEvalName:
        pobj->b.pad = simplePad;  /* names are always shared */
        if (swap)
	  {
	  SWAP2(&pobj->b.length);
	  SWAP4(&pobj->b.value);
	  }
	if ((Int16) pobj->b.length <= 0)
	  {
	  register PNameArrayBody pna;
	  PNameEntry no;
	  switch ((Int16) pobj->b.length)
	    {
	    case 0:
	      pna = rootPrivate->nameMap.val.namearrayval;
	      break;
	    case -1:
	      pna = rootShared->nameMap.val.namearrayval;
	      break;
	    default:
	      SyntaxError("bad name length");
	    }
	  if (pna == NIL ||
	      (Card32) pobj->b.value >= pna->length ||
	      (no = pna->nmEntry[pobj->b.value]) == NIL)
	    UndefNameIndex(
	      (pobj->b.length == 0)? "user" : "system",
	      (integer)pobj->b.value);
	  pobj->o.length = 0;
	  pobj->o.val.nmval = no;
	  }
	else
	  {
	  register PCard8 ptr;
	  if (pobj->b.length > MAXnameLength) LimitCheck();
	  ptr = (PCard8) pbos + pobj->b.value;
	  if (pobj->b.value + pobj->b.length > bsize ||
	      ptr < (PCard8) pAryMax)
	    SyntaxError("name string out of bounds");
	  if (ptr < pStrMin) pStrMin = ptr;
	  FastName(ptr, pobj->b.length, &tempObj.o);
	  pobj->o.length = 0;
	  pobj->o.val.nmval = tempObj.o.val.nmval;
	  }
	if (type == bo_immEvalName &&
	    ! LoadName(pobj->o.val.nmval, (PObject)pobj))
	  {
	  PushP((PObject)pobj);
	  stackRstr = false;
	  Undefined();
	  }
	break;

      default:
        SyntaxError("bad type");

      NonZeroUnused:
        SyntaxError("non-zero unused field");
      }
    }
}


public procedure ReadNumAry(stm, hdr, ret)
  Stm stm; PHNAHeader hdr; PObject ret;
{
  register PObject pobj;
  register PInt16 pint16;
  register PInt32 pint32;
  register Int32 length;
  register Card16 rep;
  Int32 scale, bsize;
#if (! IEEEFLOAT)
  procedure (*ConvReal)(/* FloatRep *from, real *to */) = NIL;
#endif (! IEEEFLOAT)
  Object tempObj;

  DebugAssert(hdr->type == bt_numArray);
  scale = hdr->rep;
  if (scale < 128) rep = SWAPBITS;  /* 1 if need to swap, 0 if not */
  else {rep = ! SWAPBITS; scale -= 128;}
  if (rep) SWAP2(&hdr->length)
  length = hdr->length;
  if (scale < 32)
    {
    bsize = length * 4;
    rep += (scale == 0)? 0 : 4;
    }
  else if (scale < 48)
    {
    bsize = length * 2;
    scale -= 32;
    rep += (scale == 0)? 2 : 6;
    }
  else if (scale == 48)
    {
    bsize = length * 4;
#if IEEEFLOAT
    rep += 8;
#else IEEEFLOAT
    if (hdr->rep < 128)		/* () ? : caused error for VAX cc */
      ConvReal = IEEEHighToNative;
    else
      ConvReal = IEEELowToNative;
    rep = 9;
#endif IEEEFLOAT
    }
  else if (scale == 49)
    {
    bsize = length * 4;
    rep = 8;
    }
  else SyntaxError("bad representation");

  LAryObj(*ret, length, (PObject) AllocAligned(length * sizeof(Object)));
  pobj = ret->val.arrayval;

  /* read body into the end of the allocated array */
  pint32 = (PInt32) ((char *) pobj + (length * sizeof(Object)) - bsize);
  pint16 = (PInt16) pint32;
  if (fread((char *) pint32, 1, bsize, stm) != bsize)
    SyntaxError("premature end");

  if (rep < 4) {LIntObj(tempObj, 0);}
  else {LRealObj(tempObj, 0);}
  tempObj.seen = ! vmCurrent->valueForSeen;

  for ( ; --length >= 0; pobj++)
    {
    switch (rep)
      {
      case 1:	/* 32-bit integer, swapped */
	SWAP4(pint32);
	/* fall through */

      case 0:	/* 32-bit integer */
	tempObj.val.ival = *pint32++;
	break;

      case 3:	/* 16-bit integer, swapped */
        SWAP2(pint16);
	/* fall through */

      case 2:	/* 16-bit integer */
        tempObj.val.ival = *pint16++;
	break;

      case 5:	/* 32-bit fixed (real), swapped */
	SWAP4(pint32);
	/* fall through */

      case 4:	/* 32-bit fixed (real) */
	tempObj.val.rval = os_ldexp((double) *pint32++, -scale);
	break;

      case 7:	/* 16-bit fixed (real), swapped */
        SWAP2(pint16);
	/* fall through */

      case 6:	/* 16-bit fixed (real) */
        tempObj.val.rval = os_ldexp((double) *pint16++, -scale);
	break;

      case 9:	/* real, swapped or converted */
#if IEEEFLOAT
	SWAP4(pint32);
	/* fall through */
#else IEEEFLOAT
	(*ConvReal)(pint32++, &tempObj.val.rval);
	break;
#endif IEEEFLOAT

      case 8:	/* real (native) */
        tempObj.val.ival = *pint32++;
	if (! IsValidReal(&tempObj.val.rval)) UndefResult();
	break;
      }
    *pobj = tempObj;
    }
}


#if NEXTSTMSTUFF
private int StmZero() { return(0); }
#endif NEXTSTMSTUFF

private procedure WriteObject(stm)
  Stm stm;
{
  register boolean swap;
  register PObject pobj, endobj;
  register integer size;
  integer tag;
  Object obj;
  BOSHeader hdr;
#if (! IEEEFLOAT)
  procedure (*ConvReal)(/* real *from, FloatRep *to */) = NIL;
#endif (! IEEEFLOAT)
#if NEXTSTMSTUFF
  static readonly StmProcs wStrStmProcs = {
  StmErr, StmZero, StmErrLong, StmFWrite, StmUnGetc, StmZero,
  StmErr, StmZero, StmErr, StmZero, StmErr, StmErrLong,
  "Writable String"};
  StmRec writableStrStm;
#endif NEXTSTMSTUFF

  switch (objectFormat)
    {
    case of_disable:
      Undefined();

    case of_highIEEE:
#if (! IEEEFLOAT)
      ConvReal = NativeToIEEEHigh;
#endif (! IEEEFLOAT)
      /* fall through */

    case of_highNative:
      swap = SWAPBITS;
      break;

    case of_lowIEEE:
#if (! IEEEFLOAT)
      ConvReal = NativeToIEEELow;
#endif (! IEEEFLOAT)
      /* fall through */

    case of_lowNative:
      swap = ! SWAPBITS;
      break;
    }

  hdr.type = formatToType[objectFormat];
  hdr.lenTopArray = 1;

  tag = PopCardinal();
  if (tag > MAXCard8) RangeCheck();
  PopP(&obj);
  if (stm == NIL)
    {
    StmObj so;
    PopPStream(&so);
    stm = GetStream(so);
    }

  /* Assuming obj is an array, this operation requires 3 passes over it:
     1. Check type and access of elements; accumulate total size
     2. Emit array elements
     3. Emit values of array elements that are composite
   */

  size = sizeof(BOSHeader);


  for (pobj = &obj, endobj = pobj + 1; pobj < endobj; pobj++)
    {
    size += sizeof(Object);
    switch (pobj->type)
      {
      case nullObj:
      case intObj:
      case realObj:
      case boolObj:
        break;

      case escObj:
        if (obj.length != objMark) TypeCheck();
        break;

      case nameObj:
        size += pobj->val.nmval->strLen;
        break;

      case strObj:
	if ((pobj->access & rAccess) == 0) InvlAccess();
	size += pobj->length;
	break;

      case arrayObj:
        if (pobj != &obj) LimitCheck();
	if ((pobj->access & rAccess) == 0) InvlAccess();
	endobj = pobj->val.arrayval + pobj->length;
	pobj = pobj->val.arrayval - 1;  /* "for" will increment it */
	break;

      default:
        TypeCheck();
      }
    }

  if (size > MAXCard16) LimitCheck();
  hdr.size = size;
  if (swap) SWAP2(&hdr.size);
#if NEXTSTMSTUFF
  /* Well, in this world we need to ensure that IF we are going
     to cause a flush of the underlying stream, we do only one
     single fwrite call, enabling the stream mechanism to maintain
     token integrity in the fact of the fact that it is multiplexing
     several contexts onto one stream.
     
     In order to do that, we do some checking right here.  If we can
     make it so that the object we are about to write will fit in
     one bufferful, we do so and do our writes normally, secure in the
     fact that we will copy all the data into the buffer before
     any flushes happen.  If we can't guarantee that, we allocate some
     side storage in wStr that is large enough to hold the object,
     and cons up a dummy writableStringStream on that buffer.  Then
     the normal  WriteObject code writes the object into that stream.
     At the bottom, when it's all done, we will then do one large fwrite
     of that buffer to the stream, again ensuring we never break
     single tokens across calls to fflush.  Then it frees the buffer.
   */
   writableStrStm.base = NULL;
   if (size > stm->cnt)
   {
	RecyclerPush (&obj);	/* Protect obj across yield */
   	fflush(stm);
	RecyclerPop (&obj);
	if (size > stm->cnt)
	{ /* OK, we have to do the side buffer */
	    char *p = (char *)os_malloc(size+1);
	    if (p == NULL) LimitCheck();
	    writableStrStm.base = writableStrStm.ptr = p;
	    writableStrStm.cnt = writableStrStm.data.b = size;
	      /* JP - Removed +1	*/
	    writableStrStm.procs = &wStrStmProcs;
	    writableStrStm.data.a = (int)stm;
	    stm = &writableStrStm;
	}
   }
#endif NEXTSTMSTUFF
  RecyclerPush (&obj);
  fwrite((char *) &hdr, 1, sizeof(BOSHeader), stm);
  size = sizeof(Object);  /* start of string values if no array */

  for (pobj = &obj, endobj = pobj + 1; pobj < endobj; pobj++)
    {
    UObject uobj;
    Card8 type;
    uobj.o = *pobj;
    type = pobj->type;
    uobj.b.type = (pobj->tag != Lobj)? type + 128 : type;
    uobj.b.pad = tag;
    tag = 0;

    switch (type)
      {
      case nullObj:
        break;

      case intObj:
      case boolObj:
        goto SwapV;

      case realObj:
#if IEEEFLOAT
	goto SwapV;
#else IEEEFLOAT
	if (ConvReal != NIL) (*ConvReal)(&pobj->val.rval, &uobj.b.value);
	break;
#endif IEEEFLOAT

      case escObj:
        uobj.b.type += bo_mark - escObj;
	uobj.b.length = 0;
        break;

      case nameObj:
	uobj.b.length = pobj->val.nmval->strLen;
        uobj.b.value = size;
	size += uobj.b.length;
	goto SwapLV;

      case strObj:
        uobj.b.value = size;
	size += pobj->length;
	goto SwapLV;

      case arrayObj:
        uobj.b.value = sizeof(Object);  /* value immediately follows object */
	size += pobj->length * sizeof(Object); /* start of string values */
	endobj = pobj->val.arrayval + pobj->length;
	pobj = pobj->val.arrayval - 1;  /* "for" will increment it */
	goto SwapLV;

      default:
        CantHappen();

      SwapLV:
        if (! swap) break;
        SWAP2(&uobj.b.length);
      SwapV:
        if (! swap) break;
        SWAP4(&uobj.b.value);
      }

    if (stm->cnt >= sizeof(Object))
      {
#if (MINALIGN > 1)
      if ((((Card32) stm->ptr) & (MINALIGN - 1)) != 0)
        os_bcopy((char *) &uobj.b, stm->ptr, sizeof(Object));
      else
#endif (MINALIGN > 1)
	*(PBObject) stm->ptr = uobj.b;
      stm->ptr += sizeof(Object);
      stm->cnt -= sizeof(Object);
      }
    else fwrite((char *) &uobj.b, 1, sizeof(Object), stm);
    }

  for (pobj = &obj, endobj = pobj + 1; pobj < endobj; pobj++)
    {
    switch (pobj->type)
      {
      case nullObj:
      case intObj:
      case realObj:
      case boolObj:
      case escObj:
        break;

      case nameObj:
        {
        PNameEntry pne = pobj->val.nmval;
        fwrite(pne->str, 1, pne->strLen, stm);
        break;
	}

      case strObj:
	if (pobj->length != 0)
	  fwrite(pobj->val.strval, 1, pobj->length, stm);
	break;

      case arrayObj:
	endobj = pobj->val.arrayval + pobj->length;
	pobj = pobj->val.arrayval - 1;  /* "for" will increment it */
	break;

      default:
        CantHappen();
      }
    }

#if NEXTSTMSTUFF
  if (writableStrStm.base)
  { /* OK, now write the accumulated string to the real stream */
    stm = (Stm)writableStrStm.data.a;
    fwrite(writableStrStm.base,1,writableStrStm.data.b,stm);
    os_free(writableStrStm.base);
  }
#endif NEXTSTMSTUFF

  RecyclerPop (&obj);

  if (ferror(stm) || feof(stm)) StreamError(stm);
}


public procedure PSPrObject()
{
  WriteObject(os_stdout);
}


public procedure PSWrObject()
{
  WriteObject((Stm)NIL);
}


public procedure SetObjFormat(f) integer f; {
  WriteContextParam(
    (char *)&objectFormat, (char *)&f, (integer)sizeof(objectFormat),
    (PVoidProc)NIL);
  };

public procedure PSStObjFormat()
{
  Card16 f = PopCardinal();
  if (f > of_max) RangeCheck();
  SetObjFormat(f);
}


public integer GetObjFormat() {return (integer)objectFormat;}

public procedure PSCrObjFormat()
{
  PushInteger((integer)objectFormat);
}


public procedure BinObjInit(reason)  InitReason reason;
{
Object ob;
string s;
switch (reason)
  {
  case init:
#if (OS == os_mpw)
    globals = (Globals)os_sureCalloc(sizeof(GlobalsRec),1);
    formatToType[0] = 0;
    formatToType[1] = bt_objSeqHiIEEE;
    formatToType[2] = bt_objSeqLoIEEE;
    formatToType[3] = bt_objSeqHiNative;
    formatToType[4] = bt_objSeqLoNative;
#endif (OS == os_mpw)

    InitBOSTypeMap(nullObj, &iLNullObj);
    InitBOSTypeMap(intObj, &iLIntObj);
    InitBOSTypeMap(realObj, &iLRealObj);
    InitBOSTypeMap(nameObj, &iLNameObj);
    InitBOSTypeMap(boolObj, &iLBoolObj);
    InitBOSTypeMap(strObj, &iLStrObj);
    InitBOSTypeMap(bo_immEvalName, &iLNameObj);
    InitBOSTypeMap(arrayObj, &iLAryObj);
    InitBOSTypeMap(bo_mark, &iLGenericObj);
#if (STAGE==DEVELOP)
    Assert(sizeof(Object) == sizeof(BObject));
    Assert(sizeof(Object) == sizeof(UObject));
#endif (STAGE==DEVELOP)
    break;
  case romreg:
#if VMINIT
    if (vmShared->wholeCloth)
      {
      LBoolObj(ob, SWAPBITS);
      Begin (rootPrivate->trickyDicts.val.arrayval[tdStatusDict]);
      RgstObject("byteorder", ob);
#if IEEEFLOAT
      s = (string) "IEEE";
#else IEEEFLOAT
      if ((s = GetCArg('R')) == NIL)
        {
        os_eprintf("realformat definition missing; use -R format\n");
        s = (string) "unknown";
        }
#endif IEEEFLOAT
      ob = MakeStr(s);
      ob.access = rAccess;
      RgstObject("realformat", ob);
      End ();
      }
#endif VMINIT
    break;
  case ramreg:
    break;
  endswitch
  }
}
