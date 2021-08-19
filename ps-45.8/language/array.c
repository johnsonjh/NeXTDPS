/*
  array.c

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

Original version: Chuck Geschke: February 11, 1983
Edit History:
Chuck Geschke: Fri Oct 11 07:18:44 1985
Doug Brotz: Mon Jun  2 09:58:37 1986
Ed Taft: Sun Dec 17 13:53:54 1989
John Gaffney: Mon Dec 17 13:28:20 1984
Ivor Durham: Sun May 14 11:28:43 1989
Linda Gass: Wed Aug  5 14:55:25 1987
Joe Pasqua: Thu Jan  5 11:46:07 1989
Paul Rovner: Wednesday, May 4, 1988 12:54:53 PM
Perry Caro: Mon Nov  7 15:43:24 1988
End Edit History.
*/


#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include GC
#include LANGUAGE
#include VM
#include RECYCLER
#include "array.h"
#include "packedarray.h"
#include "stack.h"

private NameObj arraycmd;
  /* NameObj used in RgstMark call for "arrayforall"	*/
  /* This object is shared by all contexts.		*/

private procedure AHead(a, n, ahead)
  AryObj a; cardinal n; register PAryObj ahead;
{*ahead = a;  if (n < ahead->length) ahead->length = n;}

private procedure ATail(a, n, atail)
  AryObj a; cardinal n; register PAryObj atail;
{
Object discard;
*atail = a;
if (n > atail->length) n = atail->length;
switch (atail->type)
  {
  case arrayObj:
    atail->length -= n;
    atail->val.arrayval += n;
    return;
  case pkdaryObj:
    while (n-- != 0) DecodeObj(atail, &discard);
    return;
  default: CantHappen();
  }
/*NOTREACHED*/
}  /* end of ATail */

public procedure SubPArray(a, beg, len, pao)
  AryObj a; cardinal beg, len;  register PAnyAryObj pao;
  /* Returned array object has all of a's attributes except
   * length and arrayval. */
{ATail(a, beg, pao);  AHead(*pao, len, pao);}

public procedure PutArray(from, beg, into)
  AnyAryObj from, into; cardinal beg;
  {
  Object ob;

  ATail(into, beg, &ob);
  VMCopyArray(from, ob);
  }

public procedure APut(a, index, ob)
  AryObj a;
  cardinal index;
  Object ob;
{
  if (index < a.length) {
    VMPutElem (a, index, ob);	/* Recycler check in VMPutElem */
  } else
    RangeCheck ();
}				/* end of APut */

public procedure AGetP(a,i,pob)  AnyAryObj a; cardinal i; PObject pob;
{
if (i >= a.length) RangeCheck();
if ((a.access & rAccess) == 0) InvlAccess();
switch (a.type)
  {
  case arrayObj:
    *pob = (VMGetElem(a,i));
    return;
  case pkdaryObj:
    do DecodeObj(&a, pob); while (i-- != 0);
    return;
  default:
    TypeCheck();
  }
}  /* end of AGetP */

public procedure ForceAGetP(a,i,pob)  AnyAryObj a; cardinal i; PObject pob;
{
a.access = rAccess;
AGetP(a, i, pob);
}  /* end of ForceAGetP */

public procedure AStore(a)
  AryObj          a;
  {
  integer i;

  if (CountStack(opStk, a.length) < a.length)
    Underflow(opStk);

    i = a.length;
    while(--i >= 0) {
      Object ob;

      IPopNotEmpty(opStk, &ob);
      VMPutElem(a, (cardinal) i, ob);	/* Recycler check in VMPutElem */
      }
  } /*  AStore */

public procedure PSArray()
{
  cardinal n;
  AryObj  ao;

  n = PopLimitCard ();
  ReclaimRecyclableVM ();
  AllocPArray (n, &ao);
  PushP (&ao);
}				/* end of PSArray */

public procedure PSAStore()
{
AryObj ao;
PopPArray(&ao);
AStore(ao);
PushP(&ao);
}  /* end of PSAStore */

public procedure PSALoad()
{
AryObj ao, aorig;
Object ob;
PopPArray(&ao);  aorig = ao;
if ((ao.access & rAccess) == 0) InvlAccess();
while (ao.length != 0) {VMCarCdr(&ao, &ob);  PushP(&ob);}
PushP(&aorig);
}  /* end of PSALoad */

public procedure AryForAll(aryOb, procOb)
  AryObj aryOb, procOb;
{
if ((aryOb.access & rAccess) == 0) InvlAccess();
EPushP(&procOb);
EPushP(&aryOb);
EPushP(&arraycmd);
}

private procedure AFAProc()
{
AryObj ao; Object ob,elem;
EPopP(&ao);
switch (ao.type)
  {
  case arrayObj:
  case pkdaryObj:
    ETopP(&ob);
    if (ao.length != 0)
      {
      VMCarCdr(&ao, &elem);
      EPushP(&ao);
      EPushP(&arraycmd);
      PushP(&elem);
      EPushP(&ob);
      }
    else EPopP(&ob);
    break;
  default:
    TypeCheck();
  }
}  /* end of AFAProc */

#if 0
procedure Array(int len, PObject pobj)
{
    PObject op, head;
    Object ob;
    int lev,seen;
    
    LAryObj (*pobj, len, len ? (PObject)AllocAligned(len*sizeof(Object)):NIL);

    if (len) {
	if (opStk->head - opStk->base < len)
	    Underflow(opStk);
	op = pobj->val.arrayval + len;
	seen = !vmCurrent->valueForSeen;
	head = opStk->head;
	lev = (pobj->shared) ? 0 : level;
	do {
	    ob = *(--head);
	    if (recycleType[ob.type]) {
 		PRecycler R = (ob.shared ? sharedRecycler : privateRecycler);
		if (AddressInRecyclerRange(R, ((PCard8)(ob.val.nullval)))) {
		    R->refCount--;
		    Assert (R->refCount >= 0);
		}
	    }
	    if (pobj->shared && !ob.shared) FInvlAccess();
	    ob.level = lev;
	    ob.seen = seen;
	    *(--op) = ob;
	} while (--len != 0);
	opStk->head = head;
    }
}
#endif
public procedure ArrayInit(reason)  InitReason reason;
{
switch (reason)
  {
  case init: break;
  case romreg:
    RgstMark("@arrayforall", AFAProc, (integer)(mrk2Args), &arraycmd);
    break;
  endswitch
  }
}  /* end of ArrayInit */
