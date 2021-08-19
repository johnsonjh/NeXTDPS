/*
  type.c

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

Original version: Chuck Geschke: February 9, 1983
Edit History:
Larry Baer: Wed Nov 15 18:01:06 1989
Scott Byer: Thu Jun  1 16:05:13 1989
Chuck Geschke: Thu Oct 10 06:30:13 1985
Doug Brotz: Fri Sep 19 13:39:59 1986
Ed Taft: Sun Dec 17 19:18:43 1989
John Gaffney: Fri Jan  4 17:10:42 1985
Don Andrews: Fri Jul 11 18:01:47 1986
Ivor Durham: Mon May  8 11:13:42 1989
Linda Gass: Thu Aug  6 09:43:10 1987
Joe Pasqua: Wed Feb  8 14:22:03 1989
Jim Sandman: Mon Oct 16 11:15:54 1989
Paul Rovner: Thursday, October 6, 1988 4:54:19 PM
Perry Caro: Mon Nov  7 17:56:21 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include COPYRIGHT
#include BASICTYPES
#include ERROR
#include EXCEPT
#include FP
#include LANGUAGE
#include RECYCLER
#include VM

#include "languagenames.h"
#include "array.h"
#include "name.h"
#include "packedarray.h"
#include "scanner.h"
#include "stack.h"

#if (OS != os_mpw)
#define extended longreal
#endif

#define StringMatch(a,b) ((a.length==b.length)?(boolean)(StringCompare(a,b)==0):false)

public boolean Equal(a,b)  Object a,b;
{
switch (a.type)
  {
  case nullObj: return (boolean)(b.type == nullObj);
  case intObj:
    switch (b.type)
      {
      case intObj: return (boolean)(a.val.ival == b.val.ival);
      case realObj: return (boolean)((extended)a.val.ival == (extended)b.val.rval);
      default: return false;
      }
  case realObj:
    switch (b.type)
      {
      case realObj: return (boolean)(a.val.rval == b.val.rval);
      case intObj: return (boolean)((extended)a.val.rval == (extended)b.val.ival);
      default: return false;
      }
  case boolObj:
    return ((b.type == boolObj) ? (boolean)(a.val.bval==b.val.bval) : false);
  case nameObj:
    switch (b.type)
      {
      case nameObj: return (boolean)(a.val.nmval == b.val.nmval);
      case strObj:
        {
        if ((b.access & rAccess) == 0) InvlAccess();
	NameToPString(a, &a);
	return (StringMatch(a,b));
        }
      default: return false;
      }
  case strObj:
    if ((a.access & rAccess) == 0) InvlAccess();
    switch (b.type)
      {
      case strObj:
        {
        if ((b.access & rAccess) == 0) InvlAccess();
	return (StringMatch(a,b));
        }
      case nameObj: {NameToPString(b, &b); return (StringMatch(a,b));}
      default: return false;
      }
  case stmObj:
    return ((b.type == stmObj)
            ? (boolean)((a.length==b.length) && (a.val.stmval==b.val.stmval))
            : false);
  case cmdObj:
    return ((b.type == cmdObj) ? (boolean)(a.length == b.length) : false);
  case dictObj:
    return (boolean) (b.type==dictObj &&
		      a.val.dictval==b.val.dictval && a.length==b.length);
  case arrayObj:
    return((b.type==arrayObj)
	   ?(boolean)((a.length==b.length)&&(a.val.arrayval==b.val.arrayval))
	   :false);
  case pkdaryObj:
    return ((b.type == pkdaryObj)
          ? (boolean)((a.length==b.length)&&(a.val.pkdaryval==b.val.pkdaryval))
          : false);
  case fontObj:
    return ((b.type == fontObj)
          ? (boolean)(a.val.fontval==b.val.fontval) : false);
  case escObj:
    if (b.type != escObj || a.length != b.length) return false;
    switch (a.length)
      {
      case objMark: return true;
      case objSave:
	return  (boolean)(a.val.saveval==b.val.saveval);
      case objGState:
	return  (boolean)(a.val.gstateval==b.val.gstateval);
      case objLock:
	return  (boolean)(a.val.lockval==b.val.lockval);
      case objCond:
	return  (boolean)(a.val.condval==b.val.condval);
      }
  }
return false;
}

/* type specific stack operations */

public cardinal PopCardinal()
{
Object ob;
integer i;
IPopSimple(opStk, &ob);
switch (ob.type)
  {
  case intObj:
    i = ob.val.ival;
    if ((i<0) || (i>MAXcardinal)) RangeCheck();
    goto ret;
  default: {RecyclerPop(&ob); TypeCheck();}
  }
ret: return (cardinal)i;
}  /* end of PopCardinal */

public Card16 PopLimitCard()
{
  integer i = PopInteger();
  if (i < 0) RangeCheck();
  if (i > MAXCard16) LimitCheck();
  return (Card16) i;
}

private cardinal CardFromOb(ob)  Object ob;
{
integer i;
if (ob.type != intObj) TypeCheck();
if ((i = ob.val.ival)<0 || i>MAXcardinal) RangeCheck();
return (cardinal)i;
}  /* end of CardFromOb */

public integer PSPopInteger()
{
Object ob;
IPopSimple(opStk, &ob);
if (ob.type != intObj) {RecyclerPop (&ob); TypeCheck();}
return ob.val.ival;
}  /* end of PopInterger */

public integer EPopInteger()
{
Object ob;
IPopSimple(execStk, &ob);
if (ob.type != intObj) {RecyclerPop (&ob); TypeCheck();}
return ob.val.ival;
}  /* end of EPopInteger */

public procedure PopPDict(pob)  PDictObj pob;
{

IPopOp(pob);
if (pob->type != dictObj) TypeCheck();
}  /* end of PopPDict */

public procedure PopPStream(pob)  PStmObj pob;
{
IPopOp(pob);
if (pob->type != stmObj) TypeCheck();
}  /* end of PopPStream */

public procedure PSPopPReal(pr)  Preal pr;
{
Object ob;
IPopSimple(opStk, &ob);
switch (ob.type)
  {
  case intObj: *pr = ob.val.ival;  break;
  case realObj: *pr = ob.val.rval; break;
  default: {RecyclerPop (&ob); TypeCheck();}
  }
}  /* end of PSPopPReal */

public procedure EPopPReal(pr)  Preal pr;
{
Object ob;
IPopSimple(execStk, &ob);
switch (ob.type)
  {
  case intObj: *pr = ob.val.ival;  break;
  case realObj: *pr = ob.val.rval; break;
  default: {RecyclerPop(&ob); TypeCheck();}
  }
}  /* end of EPopPReal */

public boolean PSPopBoolean()
{
Object ob;
IPopSimple(opStk, &ob);
if (ob.type != boolObj) {RecyclerPop(&ob); TypeCheck();}
return ob.val.bval;
}  /* end of PSPopBoolean */

public procedure PopPString(pob) PStrObj pob;
{
IPopOp(pob);
if (pob->type != strObj) TypeCheck();
}  /* end of PopPString */

public procedure PopPRString(pob)  PStrObj pob;
{
PopPString(pob);
if ((pob->access & rAccess) == 0) InvlAccess();
}  /* end of PopPRString */

public procedure PopPArray(pob)  PObject pob;
{
IPopOp(pob);
switch (pob->type)
  {
  case pkdaryObj:
  case arrayObj: return;
  default: TypeCheck();
  }
}  /* end of PopPArray */

public procedure PushCardinal(c)  cardinal c;
{
Object ob;
LIntObj(ob,(integer)c);
IPushSimple(opStk,ob);
}  /* end of PushCardinal */

public procedure PSPushInteger(i)  integer i;
{
Object ob;
LIntObj(ob,i);
IPushSimple(opStk,ob);
}  /* end of PushInteger */

public procedure EPushInteger(i)  integer i;
{
Object ob;
LIntObj(ob,i);
IPushSimple(execStk,ob);
}  /* end of EPushInteger */

public procedure PSPushBoolean(b)  boolean b;
{
Object ob;
LBoolObj(ob,b);
IPushSimple(opStk,ob);
}  /* end of PushBoolean */

public procedure PSPopPCd(pcd)  register PCd pcd;
{
register PObject x, y;

if (opStk->head - 2 >= opStk->base)
  {
  y = opStk->head - 1;
  x = opStk->head - 2;
  switch (x->type)
    {
    case realObj: pcd->x = x->val.rval; break;
    case intObj:  pcd->x = x->val.ival; break;
    default: goto problem;
    }
  switch (y->type)
    {
    case realObj: pcd->y = y->val.rval; break;
    case intObj:  pcd->y = y->val.ival; break;
    default: goto problem;
    }
  opStk->head -= 2;
  return;
  }
problem:
PopPReal(&pcd->y);
PopPReal(&pcd->x);
}  /* end of PopPCd */

public procedure PSPushPCd(pcd) PCd pcd;
{PushPReal(&pcd->x);  PushPReal(&pcd->y);}

public procedure PSPushPReal(pr)  Preal pr;
{Object ob;  LRealObj(ob, *pr);  IPushSimple(opStk, ob);}

public procedure PushReal(r)  real r;
{real realr; realr = r; PushPReal(&realr);}

public procedure EPushPReal(pr)  Preal pr;
{Object ob;  LRealObj(ob, *pr);  IPushSimple(execStk, ob);}

public procedure EPushReal(r)  real r;
{real realr;  realr = r;  EPushPReal(&realr);}

/* intrinsics */

public procedure PSLength()
{
Object ob; cardinal len;
IPopOp(&ob);
switch (ob.type)
  {
  case strObj: case arrayObj: case pkdaryObj:
    if ((ob.access & rAccess) == 0) InvlAccess();
    len = ob.length;
    break;
  case dictObj:
    len = DictLength(ob);
    break;
  case nameObj:
    NameToPString(ob, &ob);
    len = ob.length;
    break;
  default: TypeCheck();
  }
PushInteger((integer)len);
}  /* end of PSLength */

public procedure PSXCheck()
{Object ob;  IPop(opStk, &ob);  PushBoolean((boolean)(ob.tag == Xobj));}

public procedure PSRCheck()
  {
  Object ob;
  boolean v;

  IPop(opStk, &ob);
  switch (ob.type)
    {
    case arrayObj: case pkdaryObj: case strObj: case stmObj:
      v = (ob.access & rAccess) ? true : false;
      break;
    case dictObj:
      {
      PDictObj ref = XlatDictRef(&ob);
      v = (ref->val.dictval->access & rAccess)? true : false;
      break;
      }
    case escObj:
      {
      if (ob.length == objGState) {
	v = (ob.access & rAccess) ? true : false;
	}
      else TypeCheck();
      break;
      }
    default: TypeCheck();
    }
  PushBoolean(v);
  }  /* end of PSRCheck */

public procedure PSWCheck()
  {
  Object ob;
  boolean v;
  
  IPop(opStk, &ob);
  switch (ob.type)
    {
    case arrayObj: case pkdaryObj: case strObj: case stmObj:
      v = (ob.access & wAccess) ? true : false;
      break;
    case dictObj:
      {
      PDictObj ref = XlatDictRef(&ob);
      PDictBody pd = ref->val.dictval;
      v = (pd->access & wAccess) ? true : false;
      break;
      }
    case escObj:
      {
      if (ob.length == objGState) {
	v = (ob.access & wAccess) ? true : false;
	}
      else TypeCheck();
      break;
      }
    default: TypeCheck();
    }
  PushBoolean(v);
  }  /* end of PSWCheck */

public procedure PSSCheck ()
  { 
  Object ob;
  IPop(opStk, &ob);
  PushBoolean(((ob.type == dictObj) && TrickyDict(&ob)) ? false : ob.shared);
  }

public procedure PSXctOnly()
  {
  Object ob;
  IPop(opStk, &ob);
  switch (ob.type)
    {
    case arrayObj: case pkdaryObj: case strObj: case stmObj:
      if ((ob.access & (rAccess|xAccess)) == 0) InvlAccess();
      ob.access = xAccess;
      break;
    default: TypeCheck();
    }
  PushP(&ob);
  }  /* end of PSXctOnly */

public procedure PSReadOnly()
  {
  Object ob;
  IPop(opStk, &ob);
  switch (ob.type)
    {
    case arrayObj: case pkdaryObj: case strObj: case stmObj:
      if ((ob.access & rAccess) == 0) InvlAccess();
      ob.access = rAccess;
      break;
    case dictObj:
      SetDictAccess(ob, rAccess);
      break;
    case escObj:
      {
      if (ob.length == objGState) {
	if ((ob.access & rAccess) == 0) InvlAccess();
	ob.access = rAccess;
	}
      else TypeCheck();
      break;
      }
    default: TypeCheck();
    }
  IPush(opStk, ob);
}  /* end of PSReadOnly */

public procedure PSNoAccess()
  {
  Object ob;
  IPop(opStk, &ob);
  switch (ob.type)
    {
    case arrayObj: case pkdaryObj: case strObj: case stmObj:
      ob.access = nAccess;
      break;
    case dictObj:
      SetDictAccess(ob, nAccess);
      break;
    case escObj:
      {
      if (ob.length == objGState) {
	ob.access = nAccess;
	}
      else TypeCheck();
      break;
      }
    default: TypeCheck();
    }
  PushP(&ob);
}  /* end of PSNoAccess */

public procedure PSType()
  {
  Object ob;
  integer type;
  IPop(opStk, &ob);
  type = (ob.type == escObj)? ob.length : ob.type;
  PushP(&languageNames[nm_nulltype + type]);
  }  /* end of PSType */

public procedure PSCvLit()
  {
  if (opStk->head == opStk->base) Underflow(opStk);
  (opStk->head-1)->tag = Lobj;
  }  /* end of PSCvLit */

public procedure PSCvX()
  {
  if (opStk->head == opStk->base) Underflow(opStk);
  (opStk->head-1)->tag = Xobj;
  }  /* end of PSCvX */

public procedure PSCvI()
{
  Object ob, rem; integer i;
  IPopOp(&ob);
  while (true) {
    switch (ob.type){
      case intObj:
        i = ob.val.ival; goto ret;
      case realObj:
        if ((extended)ob.val.rval > (extended)MAXinteger ||
	  (extended)ob.val.rval < (extended)MINinteger) RangeCheck();
	i = (integer)ob.val.rval; goto ret;
      case strObj:
        if ((ob.access & rAccess) == 0) InvlAccess();
	if (! StrToken(ob, &rem, &ob, false)) PSError(syntaxerror);
	break; /* go around again */
      default:
        TypeCheck();}}
ret:
  PushInteger(i);
}

public procedure PSCvR()
{
Object ob, rem; real r;
IPopOp(&ob);
while (true)
  {
  switch (ob.type)
    {
    case intObj:
      r = ob.val.ival; goto ret;
    case realObj:
      r = ob.val.rval; goto ret;
    case strObj:
      if ((ob.access & rAccess) == 0) InvlAccess();
      if (! StrToken(ob, &rem, &ob, false)) PSError(syntaxerror);
      break; /* go around again */
    default:
      TypeCheck();
    }
  }
ret:
PushPReal(&r);
}  /* end of PSCvR */

public procedure PSCopy()
{
Object ob1, ob2;
PopP(&ob2);
if (ob2.type == intObj) {Copy(opStk, CardFromOb(ob2));  return;}
PopP(&ob1);				/* grab source */
if (ob1.type == pkdaryObj && ob2.type == arrayObj) /* packed array to array */
  {
  cardinal i;				/* loop index */
  Object ob;				/* scratch object */
  if (ob1.length > ob2.length) RangeCheck();
  for ( i = 0; ob1.length != 0; i++ )	/* for all source objects */
    {
    DecodeObj(&ob1, &ob);		/* get next object */
    VMPutElem(ob2,i,ob);		/* plant in destination array */
    }
  }
else
  {
  if (ob1.type != ob2.type) TypeCheck();
  switch (ob1.type)
    {
    case arrayObj:
      if (ob1.length > ob2.length) RangeCheck();
      VMCopyArray(ob1, ob2);
      ob2.length = ob1.length;
      break;
    case dictObj:
      CopyDict(ob1, &ob2);
      break;
    case strObj:
      if (ob1.length > ob2.length) RangeCheck();
      VMCopyString(ob1, ob2);
      ob2.length = ob1.length;
      break;
    case pkdaryObj:
      InvlAccess(); /* just for consistency in error reporting */
      /* fall through */
    case escObj:
      if (ob1.length != ob2.length) TypeCheck();
      if (ob1.length != objGState) TypeCheck();
      VMCopyGeneric(ob1, ob2);
      break;
    default:
      TypeCheck();
    }
  }
PushP(&ob2);
}  /* end of PSCopy */

public procedure PSGet()
{
Object aggregOb, indexOb, ob;
cardinal i;
IPopOp(&indexOb);
IPopOp(&aggregOb);
switch(aggregOb.type)
  {
  case arrayObj:
  case pkdaryObj:
    AGetP(aggregOb, CardFromOb(indexOb), &ob);
    IPush(opStk, ob);
    break;
  case dictObj:
    DictGetP(aggregOb, indexOb, &ob);
    IPush(opStk, ob);
    break;
  case strObj:
    if ((aggregOb.access & rAccess) == 0) InvlAccess();
    i = CardFromOb(indexOb);
    if (i >= aggregOb.length) RangeCheck();
    PushInteger((integer)(VMGetChar(aggregOb, i)));
    break;
  default: TypeCheck();
  }
}  /* end of PSGet */

public procedure PSPut()
  {
  Object aggregOb, indexOb, valueOb;
  cardinal i, item;
  IPopOp(&valueOb);
  IPopOp(&indexOb);
  IPopOp(&aggregOb);
  switch(aggregOb.type) {
    case arrayObj:
      APut(aggregOb, CardFromOb(indexOb), valueOb);
      break;
    case dictObj:
      DictPut(aggregOb, indexOb, valueOb);
      break;
    case strObj:
      i = CardFromOb(indexOb);
      item = CardFromOb(valueOb);
      if (i >= aggregOb.length || item > 255) RangeCheck();
      VMPutChar(aggregOb, (integer)i, (character)item);
      break;
    case pkdaryObj:
      InvlAccess(); /* just for consistency in error reporting */
      /* fall through */
    default:
      TypeCheck();
    }
  }

public procedure PSGetInterval()
  {
   Object aggregOb, resultOb;
   cardinal beg, len;
   len = PopCardinal();
   beg = PopCardinal();
   IPopOp(&aggregOb);
   if ((aggregOb.type == strObj) ||
       (aggregOb.type == arrayObj) ||
       (aggregOb.type == pkdaryObj)) {
      if ((aggregOb.access & rAccess) == 0) InvlAccess();
      if (beg > aggregOb.length || len > (aggregOb.length-beg)) RangeCheck();
      switch (aggregOb.type)
	{
	case arrayObj:
	case pkdaryObj:
	 SubPArray(aggregOb, beg, len, &resultOb);
	 if (aggregOb.type != pkdaryObj)
	   resultOb.val.arrayval->seen = aggregOb.val.arrayval->seen;
	 /* Set the seen bit of the new array body			   */
	 break;
	case strObj:
	 SubPString(aggregOb, beg, len, &resultOb);
	 break;
	default: TypeCheck();	/* Should never be reached.		   */
	}
      IPush(opStk, resultOb);
     } else TypeCheck();	/* if type not str, array or pkdarray.	   */
  }				/* end of PSGetInterval			   */

public procedure PSPutInterval()
{
  Object  fromOb,
          intoOb;
  cardinal beg;

  PopP (&fromOb);
  beg = PopCardinal ();
  PopP (&intoOb);
  /*
   * The following test (after some boolean algebra) says that the types
   * of the composite operands must be EITHER the same and one of array,
   * packed array or string OR if they are different they must be one of
   * the two array types.  If not, a typecheck error is signalled.
   */
  if (((fromOb.type != intoOb.type) ||
       (fromOb.type != arrayObj &&
        fromOb.type != pkdaryObj &&
	fromOb.type != strObj)) &&
      (fromOb.type != pkdaryObj || intoOb.type != arrayObj) &&
      (fromOb.type != arrayObj || intoOb.type != pkdaryObj))
      TypeCheck ();       /* TypeCheck must precede length test below */
  if (beg > intoOb.length || fromOb.length > (intoOb.length - beg))
    RangeCheck ();
  switch (fromOb.type) {
   case arrayObj:
    PutArray (fromOb, beg, intoOb);
    break;
   case strObj:
    PutString (fromOb, beg, intoOb);
    break;
   case pkdaryObj:
    if (intoOb.type == arrayObj) {
      Int16   i;                /* loop index */
      Object  ob;               /* scratch object */
 
      for (i = 0; fromOb.length != 0; i++) {    /* for all source objects */
        DecodeObj (&fromOb, &ob);               /* grab next object */
        VMPutElem (intoOb, (beg + i), ob);      /* plant in destination */
      }
      break;
    } else
      InvlAccess ();            /* just for consistency in error reporting */
    /* fall through */
   default:
    CantHappen ();              /* Checked above */
  }
}                               /* end of PSPutInterval */

public procedure PSForAll()
{
Object aggregOb; AryObj procOb;
PopPArray(&procOb);
PopP(&aggregOb);
switch (aggregOb.type)
  {
  case arrayObj:
  case pkdaryObj:
    AryForAll(aggregOb, procOb); break;
  case dictObj:
    DictForAll(aggregOb, procOb); break;
  case strObj:
    StrForAll(aggregOb, procOb); break;
  default: TypeCheck();
  }
}  /* end of PSForAll */

public procedure RRoundP(x, y) Preal x, y;
{*y = (real)((integer)((RealLt0(*x)) ? (*x - fpHalf) : (*x + fpHalf)));}

public procedure FixCd(c, d)  Cd c;  PDevCd d;
{d->x = pflttofix(&c.x); d->y = pflttofix(&c.y);}

public procedure UnFixCd(d, c) DevCd d;  PCd c;
{fixtopflt(d.x, &c->x); fixtopflt(d.y, &c->y);}

public procedure TypeInit(reason)
  InitReason reason;
{
  switch (reason) {
   case init:
    /* verify assumption made by PopLimitCard procedure */
    DebugAssert(MAXarrayLength == MAXcardinal &&
                MAXdctCount == MAXcardinal &&
		MAXstringLength == MAXcardinal);
    break;
   case romreg:
#if VMINIT
    {
      Object  ob;

      LBoolObj (ob, true);
      RgstObject ("true", ob);
      ob.val.bval = false;
      RgstObject ("false", ob);
      LNullObj (ob);
      RgstObject ("null", ob);
    }
#endif VMINIT
    break;
   endswitch
  }
} /* end of TypeInit */
