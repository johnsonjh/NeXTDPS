/*
  numstrm.c

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

Original version: Bill Paxton Mon Feb 29 14:00:46 1988
Edit History:
Larry Baer: Fri Nov 17 10:33:50 1989
Bill Paxton: Tue Apr 12 15:27:04 1988
Ivor Durham: Sun Aug 14 10:25:26 1988
Joe Pasqua: Thu Jan  5 10:39:28 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include EXCEPT
#include FP
#include LANGUAGE
#include PSLIB
#include VM

public Fixed HF4F(ns) register PNumStrm ns; {
  /* high byte first; 4 byte fixed point; Fixed result */
  register Card32 u;
  register Fixed f;
  register Int16 scale = ns->scale;
  register character *ss = ns->str;
  ns->str += 4;
  u = (*ss++);
  u <<= 8;
  u |= (*ss++);
  u <<= 8;
  u |= (*ss++);
  u <<= 8;
  u |= (*ss);
  f = u; /* move to signed int to get sign extension on right shift */
  if (scale > 16)
    f >>= scale - 16;
  else if (scale < 16)
    f <<= 16 - scale;
  return f;
  }

private Fixed LF4F(ns) register PNumStrm ns; {
  /* low byte first; 4 byte fixed point; Fixed result */
  register Card32 u;
  register Fixed f;
  register Int16 scale = ns->scale;
  register character *ss = ns->str;
  ns->str += 4;
  u = (*ss++);
  u |= (*ss++) << 8;
  u |= (*ss++) << 16;
  u |= (*ss)   << 24;
  f = u; /* move to signed int to get sign extension on right shift */
  if (scale > 16)
    f >>= scale - 16;
  else if (scale < 16)
    f <<= 16 - scale;
  return f;
  }

private Fixed HF2F(ns) register PNumStrm ns; {
  /* high byte first; 2 byte fixed point; Fixed result */
  register Card32 u;
  register Fixed f;
  register character *ss = ns->str;
  ns->str += 2;
  u = (*ss++);
  u <<= 8; u |= (*ss);
  u <<= 16;
  f = u; /* move to signed int to get sign extension on right shift */
  f >>= ns->scale;
  return f;
  }

private Fixed LF2F(ns) register PNumStrm ns; {
  /* low byte first; 2 byte fixed point; Fixed result */
  register Card32 u;
  register Fixed f;
  register character *ss = ns->str;
  ns->str += 2;
  u = (*ss++);
  u |= (*ss) << 8;
  u <<= 16;
  f = u; /* move to signed int to get sign extension on right shift */
  f >>= ns->scale;
  return f;
  }

private procedure HF4R(ns, p) register PNumStrm ns; real *p; {
  /* high byte first; 4 byte fixed point; real result */
  register Card32 u;
  register integer f;
  register real r;
  register character *ss = ns->str;
  ns->str += 4;
  u = (*ss++);
  u <<= 8;
  u |= (*ss++);
  u <<= 8;
  u |= (*ss++);
  u <<= 8;
  u |= (*ss);
  f = u; /* convert to signed integer */
  r = f; /* convert to real */
  *p = r / (1 << ns->scale);
  }

private procedure LF4R(ns, p) register PNumStrm ns; real *p; {
  /* low byte first; 4 byte fixed point; real result */
  register Card32 u;
  register integer f;
  register real r;
  register character *ss = ns->str;
  ns->str += 4;
  u = (*ss++);
  u |= (*ss++) << 8;
  u |= (*ss++) << 16;
  u |= (*ss)   << 24;
  f = u; /* convert to signed integer */
  r = f; /* convert to real */
  *p = r / (1 << ns->scale);
  }

private procedure HF2R(ns, p) PNumStrm ns; real *p; {
  /* high byte first; 2 byte fixed point; real result */
  fixtopflt(HF2F(ns), p);
  }

private procedure LF2R(ns, p) register PNumStrm ns; real *p; {
  /* low byte first; 2 byte fixed point; real result */
  fixtopflt(LF2F(ns), p);
  }

#if IEEEFLOAT
#else IEEEFLOAT

private procedure HIRR(ns, p) register PNumStrm ns; real *p; {
  /* high byte first; IEEE real; real result */
  register Card32 u;
  Card32 r;
  register character *ss = ns->str;
  ns->str += 4;
  u = (*ss++);
  u <<= 8;
  u |= (*ss++);
  u <<= 8;
  u |= (*ss++);
  u <<= 8;
  u |= (*ss);
  r = u;
  IEEEHighToNative((real *)(&r), p); /* convert to native real */
  }

private procedure LIRR(ns, p) register PNumStrm ns; real *p; {
  /* low byte first; IEEE real; real result */
  register Card32 u;
  Card32 r;
  register character *ss = ns->str;
  ns->str += 4;
  u = (*ss++);
  u |= (*ss++) << 8;
  u |= (*ss++) << 16;
  u |= (*ss)   << 24;
  r = u;
  IEEELowToNative((real *)(&r), p); /* convert to native real */
  }

private Fixed HIRF(ns) register PNumStrm ns; {
  /* high byte first; IEEE real; Fixed result */
  real r;
  HIRR(ns, &r);
  return pflttofix(&r);
  }

private Fixed LIRF(ns) register PNumStrm ns; {
  /* low byte first; IEEE real; Fixed result */
  real r;
  LIRR(ns, &r);
  return pflttofix(&r);
  }

#endif IEEEFLOAT

private procedure HNRR(ns, p) register PNumStrm ns; real *p; {
  /* high byte first; native real; real result */
  register Card32 u;
  register character *ss = ns->str;
  ns->str += 4;
  u = (*ss++);
  u <<= 8;
  u |= (*ss++);
  u <<= 8;
  u |= (*ss++);
  u <<= 8;
  u |= (*ss);
  *(PCard32)(p) = u;
  }

private procedure LNRR(ns, p) register PNumStrm ns; real *p; {
  /* low byte first; native real; real result */
  register Card32 u;
  register character *ss = ns->str;
  ns->str += 4;
  u = (*ss++);
  u |= (*ss++) << 8;
  u |= (*ss++) << 16;
  u |= (*ss)   << 24;
  *(PCard32)(p) = u;
  }

public Fixed HNRF(ns) register PNumStrm ns; {
  /* high byte first; native real; Fixed result */
  register Card32 u;
  register character *ss = ns->str;
  Card32 i;
  ns->str += 4;
  u = (*ss++);
  u <<= 8;
  u |= (*ss++);
  u <<= 8;
  u |= (*ss++);
  u <<= 8;
  u |= (*ss);
  i = u;
  return pflttofix((real *)(&i));
  }

private Fixed LNRF(ns) register PNumStrm ns; {
  /* low byte first; native real; Fixed result */
  register Card32 u;
  register character *ss = ns->str;
  Card32 i;
  ns->str += 4;
  u = (*ss++);
  u |= (*ss++) << 8;
  u |= (*ss++) << 16;
  u |= (*ss)   << 24;
  i = u;
  return pflttofix((real *)(&i));
  }

private procedure PkAObjR(ns, p) PNumStrm ns; real *p; {
  Object ob;
  VMCarCdr(&ns->ao, &ob);
  switch (ob.type) {
    case realObj: *p = (Component) ob.val.rval; break;
    case  intObj: *p = (Component) ob.val.ival; break;
    default: TypeCheck();
    }
  }

private Fixed PkAObjF(ns) PNumStrm ns; {
  Object ob;
  VMCarCdr(&ns->ao, &ob);
  switch (ob.type) {
    case realObj: return pflttofix(&ob.val.rval);
    case  intObj: return FixInt(ob.val.ival);
    default: TypeCheck();
    }
  }

private procedure AObjR(ns, p) PNumStrm ns; real *p; {
  register PObject pob = ns->aptr;
  ns->aptr++;
  switch (pob->type) {
    case realObj: *p = (Component) pob->val.rval; break;
    case  intObj: *p = (Component) pob->val.ival; break;
    default: TypeCheck();
    }
  }

private Fixed AObjF(ns) PNumStrm ns; {
  register PObject pob = ns->aptr;
  ns->aptr++;
  switch (pob->type) {
    case realObj: return pflttofix(&pob->val.rval);
    case  intObj: return FixInt(pob->val.ival);
    default: TypeCheck();
    }
  }

public procedure SetupNumStrm(pob, ns) PObject pob; register PNumStrm ns; {
  character r;
  integer len, bytespernum;
  boolean hi_byte_first;
  string s;
  boolean noReadAccess;

  /* error if noReadAccess, but only if obj is array, packedarray, or string */
  noReadAccess = ((pob->access & rAccess) == 0);
  ns->ao = *pob;
  bytespernum = 4;
  switch (pob->type) {
    case arrayObj:
      if (noReadAccess) InvlAccess();
      ns->len = pob->length;
      ns->GetReal = AObjR;
      ns->GetFixed = AObjF;
      ns->aptr = pob->val.arrayval;
      break;
    case pkdaryObj:
      if (noReadAccess) InvlAccess();
      ns->len = pob->length;
      ns->GetReal = PkAObjR;
      ns->GetFixed = PkAObjF;
      ns->ao = *pob;
      break;
    case strObj:
      if (noReadAccess) InvlAccess();
      s = pob->val.strval;
      if (pob->length < 4) TypeCheck();
      if (s[0] != 149) TypeCheck(); /* 149 is the code for number array */
      r = s[1];
      if (r >= 128) {
        hi_byte_first = false; r -= 128;
	len = s[3] << 8; len |= s[2];
	}
      else {
        hi_byte_first = true;
	len = s[2] << 8; len |= s[3];
	}
      if (r <= 31) {
        ns->scale = r;
	if (hi_byte_first) {
          ns->GetReal = HF4R;
	  ns->GetFixed = HF4F;
          }
        else {
          ns->GetReal = LF4R;
	  ns->GetFixed = LF4F;
          }
        }
      else if (r <= 47) {
        bytespernum = 2;
        ns->scale = r - 32;
	if (hi_byte_first) {
          ns->GetReal = HF2R;
	  ns->GetFixed = HF2F;
          }
        else {
          ns->GetReal = LF2R;
	  ns->GetFixed = LF2F;
          }
        }
#if IEEEFLOAT
      else if (r == 48 || r == 49) {
        if (hi_byte_first) {
          ns->GetReal = HNRR;
	  ns->GetFixed = HNRF;
	  }
        else {
          ns->GetReal = LNRR;
	  ns->GetFixed = LNRF;
	  }
        }
#else IEEEFLOAT
      else if (r == 48)
        {
        if (hi_byte_first) {
          ns->GetReal = HIRR;
	  ns->GetFixed = HIRF;
	  }
        else {
          ns->GetReal = LIRR;
	  ns->GetFixed = LIRF;
	  }
        }
      else if (r == 49) {
        if (hi_byte_first) {
          ns->GetReal = HNRR;
	  ns->GetFixed = HNRF;
	  }
        else {
          ns->GetReal = LNRR;
	  ns->GetFixed = LNRF;
	  }
        }
#endif IEEEFLOAT
      else TypeCheck();
      ns->str = s + 4;
      ns->len = len;
      if (pob->length - 4 < len * bytespernum) TypeCheck();
      break;
    default: TypeCheck();
    }
  ns->bytespernum = bytespernum;
  }

/* the following routines are for the userpath cache */

public integer SizeNumStrmForCache(ns) PNumStrm ns; {
  integer size;
  switch (ns->ao.type) {
    case strObj:
      size = ns->bytespernum * ns->len + 4;
      break;
    case arrayObj:
    case pkdaryObj:
      size = ns->ao.length * (sizeof(Card32) + 1);
      break;
    default: CantHappen();
    }
  return size;
  }

public procedure CopyNumStrmForCache(ns, s32, s8)
  register PNumStrm ns; PCard32 *s32; string *s8; {
  register string s = NULL;
  register PCard32 p = NULL;
  register integer len;
  switch (ns->ao.type) {
    case strObj:
      len = ns->bytespernum * ns->len + 4;
      s = (string)NEW(len, 1);
      *s8 = s; *s32 = NULL;
      os_bcopy((char *)ns->ao.val.strval, (char *)s, len);
      break;
    case arrayObj:
    case pkdaryObj:
      len = ns->ao.length;
      DURING
      s = (string)NEW(len, 1);
      p = (PCard32)NEW(len, sizeof(Card32));
      *s8 = s; *s32 = p;
      HANDLER { if (s) FREE(s); if (p) FREE(p); RERAISE; }
      END_HANDLER;
      if (ns->ao.type == arrayObj) {
        register PObject aptr = ns->ao.val.arrayval;
	while (--len >= 0) {
          *p++ = aptr->val.ival;
	  *s++ = aptr->type;
	  aptr++;
	  }
        }
      else {
        Object ao, ob;
	ao = ns->ao;
	while (--len >= 0) {
          VMCarCdr(&ao, &ob);
	  *p++ = ob.val.ival;
	  *s++ = ob.type;
	  }
        }
      break;
    default: CantHappen();
    }
  }

public boolean EqNumStrmCache(ns, s32, s8)
  PNumStrm ns; PCard32 s32; string s8; {
  register integer size;
  boolean failed = false;
  switch (ns->ao.type) {
    case strObj: {
      register string aos = ns->ao.val.strval, s = s8;
      size = ns->bytespernum * ns->len + 4;
      while (--size >= 0) {
        if (*aos++ != *s++) { failed = true; break; }
	}
      break;
      }
    case arrayObj: {
      register string s = s8;
      register PCard32 p = s32;
      register PObject aptr = ns->ao.val.arrayval;
      size = ns->ao.length;
      while (--size >= 0) {
        if (aptr->val.ival != *p++ || aptr->type != *s++)
          { failed = true; break; }
        aptr++;
        }
      break;
      }
    case pkdaryObj: {
      Object ao,ob;
      register string s = s8;
      register PCard32 p = s32;
      ao = ns->ao;
      size = ns->ao.length;
      while (--size >= 0) {
        VMCarCdr(&ao, &ob);
	if (ob.val.ival != *p++ || ob.type != *s++)
          { failed = true; break; }
        }
      break;
      }
    default: CantHappen();
    }
  return !failed;
  }
