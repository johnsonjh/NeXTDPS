/*
  math.c

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

Original version: Tom Boynton: Thu Feb 17 18:22:24 1983
Edit History:
Scott Byer: Thu Jun  1 16:45:03 1989
Tom Boynton: Mon Mar 21 16:37:43 1983
Chuck Geschke: Thu Oct 10 06:36:35 1985
Doug Brotz: Fri Sep 19 10:33:22 1986
Ed Taft: Thu Jul 28 09:25:43 1988
Peter Hibbard: Sat Nov 16 22:32:30 1985
Ivor Durham: Fri May  6 16:19:39 1988
Joe Pasqua: Tue Feb 28 11:06:29 1989
Paul Rovner: Thursday, October 6, 1988 12:48:55 PM
Perry Caro: Mon Nov  7 17:55:53 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT

#include BASICTYPES
#include ERROR
#include FP
#include LANGUAGE
#include "langdata.h"

#if (OS != os_mpw)
#define extended longreal
#endif

public procedure PSAdd() {
  Object a, b; real r;
  PopP(&b);
  PopP(&a);
  if (a.type == intObj) {
    integer ai = a.val.ival;
    if (b.type == intObj) {
      integer bi = b.val.ival;
      integer ir = ai+bi;
      if ((ai<0) != (bi<0) || (ai<0) == (ir<0)) {
        PushInteger(ir);
	return;
	}
      else {
        extended ar = (extended)ai;
        extended br = (extended)bi;
        r = ar + br;
	}
      }
    else if (b.type == realObj) {
      extended br = (extended)b.val.rval;
      r = (extended)ai + br;
      }
    else TypeCheck();
    }
  else if (a.type == realObj) {
    extended ar = (extended)a.val.rval;
    if (b.type == intObj) {
      integer bi = b.val.ival;
      r = ar + (extended)bi;
      }
    else if (b.type == realObj) {
      extended br = (extended)b.val.rval;
      r = ar + br;
      }
    else TypeCheck();
    }
  else TypeCheck();
  PushPReal(&r);
  } /* end of PSAdd */

public procedure PSSub() {
  Object a, b; real r;
  PopP(&b);
  PopP(&a);
  if (a.type == intObj) {
    integer ai = a.val.ival;
    if (b.type == intObj) {
      integer bi = b.val.ival;
      integer ir = ai-bi;
      if ((ai<0) != (bi>0) || (ai<0) == (ir<0)) {
        PushInteger(ir);
	return;
	}
      else {
        extended ar = (extended)ai;
        extended br = (extended)bi;
        r = ar - br;
	}
      }
    else if (b.type == realObj) {
      extended br = (extended)b.val.rval;
      r = (extended)ai - br;
      }
    else TypeCheck();
    }
  else if (a.type == realObj) {
    extended ar = (extended)a.val.rval;
    if (b.type == intObj) {
      integer bi = b.val.ival;
      r = ar - (extended)bi;
      }
    else if (b.type == realObj) {
      extended br = (extended)b.val.rval;
      r = ar - br;
      }
    else TypeCheck();
    }
  else TypeCheck();
  PushPReal(&r);
  } /* end of PSSub */

public procedure PSMul() {
  Object a, b; real r;
  PopP(&b);
  PopP(&a);
  if (a.type == intObj) {
    integer ai = a.val.ival;
    if (b.type == intObj) {
      extended x = (extended)ai;
      integer bi = b.val.ival;
      x *= (extended)bi;
      if ((x > (extended)MINinteger) && (x < (extended)MAXinteger)) {
        PushInteger(ai*bi);
	return;
	}
      else {
        r = x;
	}
      }
    else if (b.type == realObj) {
      extended br = (extended)b.val.rval;
      r = (extended)ai * br;
      }
    else TypeCheck();
    }
  else if (a.type == realObj) {
    extended ar = (extended)a.val.rval;
    if (b.type == intObj) {
      integer bi = b.val.ival;
      r = ar * (extended)bi;
      }
    else if (b.type == realObj) {
      extended br = (extended)b.val.rval;
      r = ar * br;
      }
    else TypeCheck();
    }
  else TypeCheck();
  PushPReal(&r);
  } /* end of PSMul */

public procedure PSDiv() {
  Object a, b; extended ra, rb;  real r;
  PopP(&b);
  PopP(&a);

  if (b.type == intObj) {
    integer i = b.val.ival;
    if (i == 0) UndefResult();
    rb = (extended) i;
    }
  else if (b.type == realObj) {
    real x = b.val.rval;
    if (RealEq0(x)) UndefResult();
    rb = (extended) x;
    }
  else TypeCheck();

  if (a.type == intObj) {
    integer i = a.val.ival;
    ra = (extended) i;
    }
  else if (a.type == realObj) {
    real x = a.val.rval;
    ra = (extended) x;
    }
  else TypeCheck();

  r = (real) (ra / rb);
  PushPReal(&r);
  }  /* end of PSDiv */

public procedure PSIDiv()
{
integer i, j;
j = PopInteger();
i = PopInteger();
if (i == MINinteger && j == -1) UndefResult();
PushInteger(i/j);
}  /* end of PSIDiv */


public procedure PSMod()
{
Object a, b;
PopP(&b);
PopP(&a);
if (a.type != intObj || b.type != intObj) TypeCheck();
if (b.val.ival == 0) UndefResult();
PushInteger(a.val.ival%b.val.ival);
}  /* end of PSMod */


public procedure PSNeg()
{
Object a;  real r;
PopP(&a);
switch (a.type)
  {
  case intObj:
    if (a.val.ival == MINinteger) {r = -((real)(a.val.ival)); goto realret;}
    PushInteger(-a.val.ival);
    return;
  case realObj: r = -a.val.rval; goto realret;
  }
TypeCheck();
realret: PushPReal(&r);
}  /* end of PSNeg */


public procedure PSAbs()
{
Object a; real r;
PopP(&a);
switch (a.type)
  {
  case intObj:
    if (a.val.ival == MINinteger) {r = a.val.ival; r = -r; PushPReal(&r);}
    else PushInteger(os_labs(a.val.ival));
    return;
  case realObj:
    r = os_fabs(a.val.rval);
    PushPReal(&r);
    return;
  }
TypeCheck();
}  /* end of PSAbs */


public procedure PSRound()
{
Object a; real r;
register PObject pa;
pa = &a;
PopP(pa);
if (pa->type == intObj) PushP(pa);
else if (pa->type == realObj) {
  extended r1, r2 = (extended) pa->val.rval;
  r1 = os_ceil(r2);
  if ((r1 - r2) > fpHalf) r1 = os_floor(r2);
  r = (real) r1;
  PushPReal(&r);
  return;
  }
else TypeCheck();
}  /* end of PSRound */


public procedure PSFloor()
{
Object a;  real r;
register PObject pa;
pa = &a;
PopP(pa);
switch (pa->type)
  {
  case intObj: PushP(pa); return;
  case realObj: r = os_floor(pa->val.rval);  PushPReal(&r);  return;
  }
TypeCheck();
} /* end of PSFloor */


public procedure PSCeiling()
{
Object a;  real r;
register PObject pa;
pa = &a;
PopP(pa);
switch (pa->type)
  {
  case intObj: PushP(pa); return;
  case realObj: r = os_ceil(pa->val.rval);  PushPReal(&r);  return;
  }
TypeCheck();
}  /* end of PSCeiling */


public procedure PSTruncate()
{
Object a;  real r;
register PObject pa;
pa = &a;
PopP(pa);
switch (pa->type)
  {
  case intObj: PushP(pa); return;
  case realObj: {
    extended rr = (extended) pa->val.rval;
    r = (RealLt0(rr)) ? os_ceil(rr) : os_floor(rr);
    PushPReal(&r);
    return;
    }
  }
TypeCheck();
}  /* end of PSTruncate */

private boolean LGt(a, b) Object a, b; { /* Gt(a,b) == LGt(b,a) */
  switch (a.type) {
    case intObj: {
      integer ai = a.val.ival;
      switch (b.type) {
        case intObj: {
	  integer bi = b.val.ival;
	  return (boolean)(ai < bi);
	  }
        case realObj: {
	  extended br = (extended)b.val.rval;
	  return (boolean)((extended)ai < br);
	  }
        default: goto typeerror;
        }
      }
    case realObj: {
      extended ar = (extended) a.val.rval;
      switch (b.type) {
        case intObj: {
	  integer bi = b.val.ival;
	  return (boolean)(ar < (extended)bi);
	  }
        case realObj: {
	  extended br = (extended)b.val.rval;
	  return (boolean)(ar < br);
	  }
        default: goto typeerror;
        }
      }
    case strObj:
      if (b.type == strObj) {
        if ((a.access & b.access & rAccess) == 0) InvlAccess();
        return(StringCompare(a, b) < 0);
        }
    }
  typeerror: TypeCheck(); /*NOTREACHED*/
  } /* end of LGt */


public procedure PSEq() {
  Object a, b;
  PopP(&b);
  PopP(&a);
  PushBoolean(Equal(a, b));
  }  /* end of PSEq */


public procedure PSNe()
{Object a, b; PopP(&b); PopP(&a); PushBoolean((boolean)!Equal(a, b));}

public procedure PSLt()
{Object a, b; PopP(&b); PopP(&a); PushBoolean(LGt(a, b));}

public procedure PSLe()
{Object a, b; PopP(&b); PopP(&a); PushBoolean((boolean)!LGt(b, a));}

public procedure PSGt()
{Object a, b; PopP(&b); PopP(&a); PushBoolean(LGt(b, a));}

public procedure PSGe()
{Object a, b; PopP(&b); PopP(&a); PushBoolean((boolean)!LGt(a, b));}

public procedure PSSin() {
  real r;
  PopPReal(&r);
  r = os_sin(RAD(r));
  PushPReal(&r);
  }  /* end of PSSin */

public procedure PSCos() {
  real r;
  PopPReal(&r);
  r = os_cos(RAD(r));
  PushPReal(&r);
  }  /* end of PSCos */

public procedure PSATan() {
  real a, b, c;
  PopPReal(&b);
  PopPReal(&a);
  if (RealEq0(a) && RealEq0(b)) UndefResult();
  c = DEG(os_atan2(a, b));
  while (RealLt0(c)) c += fp360;
  PushPReal(&c);
  }  /* end of PSATan */


public procedure PSExp()
{
real a, b;
PopPReal(&b);
PopPReal(&a);
a = os_pow(a, b);	/* May raise ecUndefResult to be caught elsewhere */
PushPReal(&a);
}  /* end of PSExp */


public procedure PSLog()
{
real a;
PopPReal(&a);
if (RealLe0(a)) RangeCheck();
a = os_log10(a);
PushPReal(&a);
}  /* end of PSLog */

public procedure PSLn()
{
real a;
PopPReal(&a);
if (RealLe0(a)) RangeCheck();
a = os_log(a);
PushPReal(&a);
} /* end of PSLn */


public procedure PSSqRt()
{
real a;
PopPReal(&a);
if (RealLt0(a)) RangeCheck();
a = os_sqrt(a);
PushPReal(&a);
}  /* end of PSSqrt */


public procedure PSNot()
{
Object ob;
PopP(&ob);
switch (ob.type) {
  case boolObj: ob.val.bval = !ob.val.bval; break;
  case intObj: ob.val.ival = ~ob.val.ival; break;
  default: TypeCheck();
  }
PushP(&ob);
}  /* end of PSNot */


public procedure PSAnd()
{
Object ob;
PopP(&ob);
switch (ob.type) {
  case boolObj: PushBoolean((boolean)(PopBoolean() && ob.val.bval)); break;
  case intObj: PushInteger(PopInteger() & ob.val.ival); break;
  default: TypeCheck();
  }
}  /* end of PSAnd */


public procedure PSOr()
{
Object ob;
PopP(&ob);
switch (ob.type) {
  case boolObj: PushBoolean((boolean)(PopBoolean() || ob.val.bval)); break;
  case intObj: PushInteger(PopInteger() | ob.val.ival); break;
  default: TypeCheck();
  }
}  /* end of PSOr */


public procedure PSXor()
{
Object ob;
PopP(&ob);
switch (ob.type) {
  case boolObj: PushBoolean((boolean)(PopBoolean() != ob.val.bval)); break;
  case intObj: PushInteger(PopInteger() ^ ob.val.ival); break;
  default: TypeCheck();
  }
}  /* end of PSXor */


public procedure PSBitShift()
{
longcardinal a;
integer b;
b = PopInteger();
a = (longcardinal)PopInteger();
PushInteger((integer)((b < 0) ? (a>>-b) : (a<<b)));
}  /* end of PSBitShift */

/* Improved implementation of random number generator operators.
   Has full period; avoids short-period patterns in low-order bits; is
   portable to all 32-bit machines; doesn't depend on overflow behavior.
   Produces results in the range [1, 2**31 - 2], instead of
   [0, 2**31 - 1] as specified in the red book.
   Algorithm is: r = (a * r) mod m
   where a = 16807 and m = 2**31 - 1 = 2147483647
   See [Park & Miller], CACM vol. 31 no. 10 p. 1195, Oct. 1988
*/

#define RAND_m 2147483647       /* 2**31 - 1 */
#define RAND_a 16807            /* 7**5; primitive root of m */
#define RAND_q 127773           /* m / a */
#define RAND_r 2836             /* m % a */

public procedure PSSRand()
{
  register integer seed = PopInteger();
  if (seed <= 0) seed = -(seed % (RAND_m - 1)) + 1;
  if (seed > RAND_m - 1) seed = RAND_m - 1;
  randx = seed;
}

public procedure PSRRand() {PushInteger((integer)randx);} 

public procedure PSRand()
{
  register integer result = randx;
  result = RAND_a * (result % RAND_q) - RAND_r * (result / RAND_q);
  if (result <= 0) result += RAND_m;
  randx = result;
  PushInteger(result);
}  /* end of PSRand */

public procedure MathInit(reason)  InitReason reason;
{
switch (reason)
  {
  case init:
   /* See LanguageDataHandler in exec.c */
   break;
  case romreg:
   break;
  }
}  /* end of MathInit */
