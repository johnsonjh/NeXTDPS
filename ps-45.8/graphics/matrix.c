/*
  matrix.c

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

Original version: John Warnock, Mar 29, 1983
Edit History:
John Warnock: Mon Apr 11 15:39:50 1983
Doug Brotz: Wed Jun  4 17:37:57 1986
Chuck Geschke: Fri Oct 11 07:18:47 1985
Ed Taft: Thu Jul 28 16:38:28 1988
Don Andrews: Wed Jul  9 18:12:45 1986
Bill Paxton: Fri Jan 22 10:04:56 1988
Linda Gass: Wed Jul  8 13:51:13 1987
Ivor Durham: Mon May  8 15:54:26 1989
Jim Sandman: Mon Nov  7 15:09:13 1988
Joe Pasqua: Thu Jan 12 12:52:19 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include FP
#include GRAPHICS
#include LANGUAGE
#include RECYCLER
#include VM

public procedure PRealValue(ob, r)  Object ob;  Preal r;
{
switch (ob.type)
  {
  case intObj: *r = (real)ob.val.ival;  break;
  case realObj: *r = ob.val.rval;  break;
  default: TypeCheck(); /*NOTREACHED*/
  }
} /* end of PRealValue */


public procedure PAryToMtx(pao, m)  register PAryObj pao; PMtx m;
{
Object ob;  AryObj ao;
if ((pao->type != arrayObj && pao->type != pkdaryObj)) TypeCheck();
if (pao->length != 6) RangeCheck();
ao = *pao;  pao = &ao;  /* so that we don't modify caller's array object */
VMCarCdr(pao, &ob); PRealValue(ob, &m->a);
VMCarCdr(pao, &ob); PRealValue(ob, &m->b);
VMCarCdr(pao, &ob); PRealValue(ob, &m->c);
VMCarCdr(pao, &ob); PRealValue(ob, &m->d);
VMCarCdr(pao, &ob); PRealValue(ob, &m->tx);
VMCarCdr(pao, &ob); PRealValue(ob, &m->ty);
} /* end of PAryToMtx */


public procedure MtxToPAry(m, pao)  PMtx m; register PAryObj pao;
{
Object ro;
LRealObj(ro, 0);
if (pao->type != arrayObj) TypeCheck();
if (pao->length != 6) RangeCheck();
ro.val.rval = m->a; VMPutElem(*pao, (cardinal)0, ro);
ro.val.rval = m->b; VMPutElem(*pao, (cardinal)1, ro);
ro.val.rval = m->c; VMPutElem(*pao, (cardinal)2, ro);
ro.val.rval = m->d; VMPutElem(*pao, (cardinal)3, ro);
ro.val.rval = m->tx; VMPutElem(*pao, (cardinal)4, ro);
ro.val.rval = m->ty; VMPutElem(*pao, (cardinal)5, ro);
} /* end of MtxToPAry */


public procedure PopMtx(m)  PMtx m;
{AryObj ao;  PopPArray(&ao);  PAryToMtx(&ao, m);}


public procedure PushPMtx(pao, m)  PAryObj pao; PMtx m;
{MtxToPAry(m, pao);  PushP(pao);}


public procedure PSMtx()
{
Mtx m;
AryObj a;
ReclaimRecyclableVM ();
AllocPArray(6, &a);
IdentityMtx(&m);
PushPMtx(&a, &m);
} /* end of PSMtx */


public procedure PSIdentMtx()
{
Mtx m;
AryObj a;
PopPArray(&a);
IdentityMtx(&m);
PushPMtx(&a, &m);
} /* end of PSIdentMtx */


public procedure PSCnctMtx()
{
Mtx m1, m2, m3;
AryObj a3;
PopPArray(&a3);
PopMtx(&m2);
PopMtx(&m1);
MtxCnct(&m1, &m2, &m3);
PushPMtx(&a3, &m3);
} /* end of PSCnctMtx */


public procedure PSInvertMtx()
{
Mtx m1, m2;
AryObj a2;
PopPArray(&a2);
PopMtx(&m1);
MtxInvert(&m1, &m2);
PushPMtx(&a2, &m2);
} /* PSInvertMtx */


public procedure PSTlat()
{
Cd tc;
Mtx t;
Object a;
TopP(&a);
switch (a.type) {
  case arrayObj:
    PopP(&a);
    PopPCd(&tc);
    TlatMtx(&tc.x, &tc.y, &t);
    PushPMtx(&a, &t);
    break;
  default:
    PopPCd(&tc);
    Tlat(tc);
    break;
  }
} /* end of PSTlat */


public procedure PSScal()
{
Cd sc;
Mtx s;
Object a;
TopP(&a);
switch (a.type) {
  case arrayObj:
    PopP(&a);
    PopPCd(&sc);
    ScalMtx(&sc.x, &sc.y, &s);
    PushPMtx(&a, &s);
    break;
  default:
    PopPCd(&sc);
    Scal(sc);
    break;
  }
} /* end of PSScal */


public procedure PSRtat()
{
real ang;
Mtx m;
Object a;
TopP(&a);
switch (a.type) {
  case arrayObj:
    PopP(&a);
    PopPReal(&ang);
    RtatMtx(&ang, &m);
    PushPMtx(&a, &m);
    break;
  default:
    PopPReal(&ang);
    Rtat(&ang);
    break;
  }
} /* end of PSRtat */


public procedure PSTfm()
{
Mtx m;
Object ob;
Cd cd;  register PCd pcd;
pcd = &cd;
TopP(&ob);
switch (ob.type) {
  case pkdaryObj: case arrayObj:
    PopMtx(&m);
    PopPCd(pcd);
    TfmPCd(*pcd, &m, pcd);
    break;
  default:
    PopPCd(pcd);
    TfmP(*pcd, pcd);
    break;
  }
PushPCd(pcd);
} /* end of PSTfm */


public procedure PSDTfm()
{
Mtx m;
Object ob;
Cd cd;  register PCd pcd;
pcd = &cd;
TopP(&ob);
switch (ob.type) {
  case pkdaryObj: case arrayObj:
    PopMtx(&m);
    PopPCd(pcd);
    DTfmPCd(*pcd, &m, pcd);
    break;
  default:
    PopPCd(pcd);
    DTfmP(*pcd, pcd);
    break;
  }
PushPCd(pcd);
} /* end of PSDTfm */


public procedure PSITfm()
{
Mtx m;
Object ob;
Cd cd;  register PCd pcd;
pcd = &cd;
TopP(&ob);
switch (ob.type) {
  case pkdaryObj: case arrayObj:
    PopMtx(&m);
    PopPCd(pcd);
    ITfmPCd(*pcd, &m, pcd);
    break;
  default:
    PopPCd(pcd);
    ITfmP(*pcd, pcd);
    break;
  }
PushPCd(pcd);
} /* end of PSITfm */


public procedure PSIDTfm()
{
Mtx m;
Object ob;
Cd cd;  register PCd pcd;
pcd = &cd;
TopP(&ob);
switch (ob.type) {
  case pkdaryObj: case arrayObj:
    PopMtx(&m);
    PopPCd(pcd);
    IDTfmPCd(*pcd, &m, pcd);
    break;
  default:
    PopPCd(pcd);
    IDTfmP(*pcd, pcd);
    break;
  }
PushPCd(pcd);
} /* end of PSIDTfm */

