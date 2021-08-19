/*
  mtxvec.c

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
Ed Taft: Thu May  5 10:47:59 1988
Dick Sweet: Tue Mar 24 17:08:09 PST 1987
Don Andrews: Wed Jul  9 18:12:45 1986
Bill Paxton: Fri Jan 22 10:04:56 1988
Linda Gass: Wed Jul  8 13:51:13 1987
Ivor Durham: Sun Feb 14 23:50:24 1988
Jim Sandman: Wed Aug 12 10:20:14 PDT 1987
Perry Caro: Thu Nov  3 16:26:45 1988
Joe Pasqua: Mon Jan  9 10:31:10 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include EXCEPT
#include FP
#include PSLIB
#include "os_math.h"

#if USE_SIGNAL
#include <signal.h>
#endif USE_SIGNAL

#if	!FPCONSTANTS
public DPSFPGlobalsRec *dpsfpglobals;
#endif	!FPCONSTANTS

public procedure IdentityMtx(m)  PMtx m;
{m->a = m->d = fpOne;  m->b = m->c = m->tx = m->ty = fpZero;}


public procedure TlatMtx(tx, ty, m)  Preal tx, ty; PMtx m;
{m->a = m->d = fpOne;  m->b = m->c = fpZero;  m->tx = *tx;  m->ty = *ty;}


public procedure ScalMtx(xs, ys, m)  Preal xs, ys; PMtx m;
{m->a = *xs;  m->b = m->c = m->tx = m->ty = fpZero;  m->d = *ys;}


public procedure RtatMtx(a, m)  Preal a; PMtx m;
{
register real as, ac;
register integer ia;

ac = *a;
if (ac == fpZero)	{ as = fpZero; ac = fpOne;  }
else if (ac == fp90)	{ as = fpOne;  ac = fpZero; }
else if (ac == -fp90)	{ as = -fpOne; ac = fpZero; }
else { ac = RAD(ac);  as = os_sin(ac); ac = os_cos(ac); }
m->a = m->d = ac;
m->b = as;
m->c = -as;
m->tx = m->ty = fpZero;
} /* end of RtatMtx */


public procedure MtxCnct(m, n, r)  PMtx m, n, r;
{
Mtx t;
t.a = m->a * n->a + m->b * n->c;
t.b = m->a * n->b + m->b * n->d;
t.c = m->c * n->a + m->d * n->c;
t.d = m->c * n->b + m->d * n->d;
t.tx= m->tx * n->a + m->ty * n->c + n->tx;
t.ty= m->tx * n->b + m->ty * n->d + n->ty;
*r = t;
} /* end of MtxCnct */


public boolean MtxEqAlmost(m1, m2)  PMtx m1, m2;
{
double diff;
return (boolean)
  (    (diff = m1->a  - m2->a ) < fpp001 && diff > -fpp001
    && (diff = m1->b  - m2->b ) < fpp001 && diff > -fpp001
    && (diff = m1->c  - m2->c ) < fpp001 && diff > -fpp001
    && (diff = m1->d  - m2->d ) < fpp001 && diff > -fpp001
    && (diff = m1->tx - m2->tx) < fpp001 && diff > -fpp001
    && (diff = m1->ty - m2->ty) < fpp001 && diff > -fpp001);
} /* end of MtxEqAlmost */


private Mtx lastm, lastminv; /* Cached matrix and its inverse. */

#if IEEESOFT
/* Fast test for real equality. Note that this test is not correct
   in general because certain reals can be represented in multiple
   ways (e.g., +0 and -0). However, it is adequate for our purposes
   here since its only use is in determining whether we have hit the
   cached matrix. */
#define RealEq(a, b) (RtoILOOPHOLE(a) == RtoILOOPHOLE(b))
#else
#if OS == os_mach
#define RtoILOOPHOLE(r) (*(integer *)(&(r)))
#define RealEq(a, b) (RtoILOOPHOLE(a) == RtoILOOPHOLE(b))
#else
#define RealEq(a, b) ((a) == (b))
#endif
#endif

public procedure MtxInvert(m, im)  register PMtx m, im;
{
  real q;
  boolean matched;
  Mtx tm;
  if (RealEq(m->a, lastm.a) && RealEq(m->b, lastm.b) &&
      RealEq(m->c, lastm.c) && RealEq(m->d, lastm.d)) {
    tm = lastminv; /* with possible problems in tx and ty */
    if (RealEq(m->tx, lastm.tx) && RealEq(m->ty, lastm.ty))
      goto returntm;  /* exact match, just return cached inverse */
    matched = true;
    }
  else {
    matched = false;
    q = m->b * m->c - m->a * m->d;
    if (q == fpZero) RAISE(ecUndefResult, (char *)NIL);
    q = fpOne / q;
    tm.a = -m->d * q;
    tm.b = m->b * q;
    tm.c = m->c * q;
    tm.d = -m->a * q;
    }
  /* We now test for translation components that are zero. This serves
     two purposes. First, it saves unnecessary multiplications.
     Second, if the cached matrix matched except for the translation
     components and those are both zero, we are most likely performing
     an idtransform using the CTM; in that case, we don't want to
     disturb the cached inverse of the CTM. */
  if (RealEq0(m->tx))
    if (RealEq0(m->ty)) {
      tm.tx = fpZero; tm.ty = fpZero;
      if (matched) goto returntm;
      }
    else {
      tm.tx = -tm.c*m->ty;
      tm.ty = -tm.d*m->ty;
      }
  else 
    if (RealEq0(m->ty)) {
      tm.tx = -tm.a*m->tx;
      tm.ty = -tm.b*m->tx;
      }
    else {
      tm.tx = -(m->tx * tm.a + m->ty * tm.c);
      tm.ty = -(m->tx * tm.b + m->ty * tm.d);
      }
  lastm = *m; lastminv = tm; 
returntm:
  *im = tm;
  } /* end of MtxInvert */


public procedure TfmPCd(c, m, ct)  Cd c;  register PMtx m;  Cd *ct;
{
register real s;
if (RealEq0(m->a))
  s = c.y * m->c;
else {
  s = c.x * m->a;
  if (!RealEq0(m->c))
    s += c.y * m->c;
  }
ct->x = s + m->tx;
if (RealEq0(m->b))
  s = c.y * m->d;
else {
  s = c.x * m->b;
  if (!RealEq0(m->d))
    s += c.y * m->d;
  }
ct->y = s + m->ty;
} /* end of TfmPCd */


public Cd TfmCd(c, m)  Cd c; PMtx m;
{
Cd ct;
TfmPCd(c, m, &ct);
return ct;
} /* end of TfmCd */


public procedure DTfmPCd(c, m, ct)  Cd c;  PMtx m;  Cd *ct;
{
ct->x = c.x * m->a + c.y * m->c;
ct->y = c.x * m->b + c.y * m->d;
} /* end of DTfmPCd */


public Cd DTfmCd(c, m)  Cd c; PMtx m;
{
Cd ct;
DTfmPCd(c, m, &ct);
return ct;
} /* end of DTfmCd */


public procedure ITfmPCd(c, m, rc)  Cd c; PMtx m;  Cd *rc; {
  Mtx minv;
  MtxInvert(m, &minv);
  TfmPCd(c, &minv, rc);
  } /* end of ITfmPCd */


public Cd ITfmCd(c, m)  Cd c; PMtx m;
{
Cd rc;
ITfmPCd(c, m, &rc);
return rc;
} /* end of ITfmCd */


public procedure IDTfmPCd(c, m, rc)  Cd c; PMtx m; Cd *rc;
{
  Mtx m0, minv;
  m0 = *m; m0.tx = fpZero; m0.ty = fpZero;
  MtxInvert(&m0, &minv);
  TfmPCd(c, &minv, rc);
  } /* end of IDTfmPCd */


public Cd IDTfmCd(c, m)  Cd c; PMtx m;
{
Cd rc;
IDTfmPCd(c, m, &rc);
return rc;
} /* end of IDTfmCd */

/*
 * Vector implementation
 */

public procedure VecAdd(v1, v2, v3) Cd v1, v2;  PCd v3;
{v3->x = v1.x + v2.x;  v3->y = v1.y + v2.y;}

public procedure VecSub(v1, v2, v3) Cd v1, v2;  PCd v3;
{v3->x = v1.x - v2.x;  v3->y = v1.y - v2.y;}

public procedure VecMul(v, pr, v2) Cd v; Preal pr; PCd v2;
{v2->x = v.x * *pr;  v2->y = v.y * *pr;}

public integer VecTurn(v1, v2) Cd v1, v2;
{ /* returns 1 if v1 followed by v2 makes a left turn, 0 if straight or
     u-turn, -1 if right turn. */
real dir = v1.x * v2.y - v2.x * v1.y;
return (RealEq0(dir)) ? 0 : (RealLt0(dir)) ? -1 : 1;
} /* end of VecTurn */


/*
 * FP package initialization and misc procedures
 */

#if (OS == os_mach)
/* Leo 06Jun89
   There is a significant problem doing the NumOverflow
   support on the NextStep window server.  Because we cannot
   afford to make a system call on every setjmp, we use the
   _setjmp and _longjmp routines to implement the exception
   scheme.  However, this means we cannot longjmp from a signal
   handler back to the mainline.  In order to make the NumOverflow
   handler work, we have it jam the pc in the sigcontext to be
   the RaiseHack routine.  Thus, after the signal handler
   (NumOverflow) returns, control is transferred to the 
   RaiseHack routine, which raise the desired condition, 
   causing a longjmp out to the handler.
   */
private void RaiseHack()
{
  RAISE(ecUndefResult, NIL);
  /*NOTREACHED*/
} /* RaiseHack */

NumOverflow(sig, code, scp)
int sig, code;
struct sigcontext *scp;
{
    scp->sc_pc = (int)RaiseHack;
}
#else (OS == os_mach)
#if USE_SIGNAL
private int NumOverflow(sig)
  int sig;
{
  signal(sig, NumOverflow);
#if (OS==os_vaxeln)
  sigsetmask(0);  /* workaround for VAXELN C library bug */
#endif (OS==os_vaxeln)
  RAISE(ecUndefResult, NIL);
  /*NOTREACHED*/
}
#endif USE_SIGNAL
#endif (OS == os_mach)

public procedure ReportErrno(n)
  int n;
{
  switch (n)
    {
    case EDOM:
      n = ecRangeCheck; break;
    case 0:
      CantHappen();
    default:
      n = ecUndefResult; break;
    }

  RAISE(n, (char *)NIL);
  /*NOTREACHED*/
}

public procedure FPInit()
{
#if	!FPCONSTANTS
  integer xz = 0, x1 = 1, x3 = 3, x10 = 10, x16k = 16000,
          x1k = 1000, x65k = 65536;

  dpsfpglobals = (DPSFPGlobalsRec *)os_sureMalloc(sizeof(DPSFPGlobalsRec));

  fp1073741824 = 1073741824.0;
  fp1p5707963268 = 1.5707963268;
  fpp015 = 0.015;
  fpp11 = 0.11;
  fpp3364 = 0.3364;
  fpp552 = 0.552;
  fpp59 = 0.59;

  fp65536 = x65k;
  fpZero = xz;
  fpOne = x1;
  fp1p3333333 = fpOne + (fpOne / x3);
  fpTwo = fpOne + fpOne;
  fp3 = fpOne + fpTwo;
  fp3p1415926535 = fp1p5707963268 + fp1p5707963268;
  fp4 = fpTwo + fpTwo;
  fp5 = fp3 + fpTwo;
  fp6 = fp3 + fp3;
  fp6p2831853071 = fp3p1415926535 + fp3p1415926535;
  fp8 = fp4 + fp4;
  fp10 = fp5 + fp5;
  fp50 = fp10 + fp10 + fp10 + fp10 + fp10;
  fp72 = fp50 + fp10 + fp10 + fpTwo;
  fp100 = fp50 + fp50;
  fp90 = fp100 - fp10;
  fp180 = fp90 + fp90;
  fp270 = fp180 + fp90;
  fp360 = fp180 + fp180;
  fp1024 = fp360 + fp360 + fp360 - fp50 - fp6;
  fp16k = x16k;
  fpp001 = fpOne / x1k;
  fpp03 = fpp015 + fpp015;
  fpp1 = fpOne / x10;
  fpp2 = fpp1 + fpp1;
  fpp25 = fpOne / fp4;
  fpp3 = fpp1 + fpp2;
  fpp45 = fpp25 + fpp2;
  fpHalf = fpp3 + fpp2;
  fpp515 = fpHalf + fpp015;
  fpp53 = fpHalf + fpp03;
  fpp7 = fpp25 + fpp45;
  fpp9 = fpp7 + fpp2;
#endif	!FPCONSTANTS

  IdentityMtx(&lastm);
  IdentityMtx(&lastminv);

#if USE_SIGNAL
  signal(SIGFPE, (void (*)())NumOverflow);
#endif USE_SIGNAL

}
