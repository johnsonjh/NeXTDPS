/*
  string.c

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
Chuck Geschke: Fri Oct 11 07:18:49 1985
Doug Brotz: Tue Oct 14 17:11:39 1986
Ed Taft: Sun Dec 17 14:00:57 1989
Ivor Durham: Thu May 11 11:52:18 1989
Linda Gass: Thu Aug  6 09:39:23 1987
Joe Pasqua: Thu Jan  5 11:59:16 1989
Jim Sandman: Wed Mar  9 17:03:21 1988
Paul Rovner: Wednesday, June 8, 1988 4:53:14 PM
Mark Francis: Wed Jul 12 13:10:02 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT

#if OS==os_mpw
#include <Packages.h>
#define Fixed MPWFixed
#endif OS==os_mpw

#include BASICTYPES
#include ERROR
#include EXCEPT
#include LANGUAGE
#include PSLIB
#include RECYCLER
#include VM

#include "name.h"
#include "stack.h"

private CmdObj strcmd;
  /* This CmdObj is shared by all contexts and	is	*/
  /* used in the call to RgstMark for stringforall	*/

private readonly character novalue[] = "--nostringval--";

private procedure SHead(s, n, pstrob)
  StrObj s; cardinal n;  register PStrObj pstrob;
{
*pstrob = s;
if (n < pstrob->length) pstrob->length = n;
}  /* end of SHead */

private procedure STail(s, n, pstrob)
  StrObj s; cardinal n;  register PStrObj pstrob;
{
*pstrob = s;
if (n > pstrob->length) n = pstrob->length;
pstrob->length -= n;
pstrob->val.strval += n;
}  /* end of STail */

public StrObj makestring(s,t) /* always access through macros */
	string s; cardinal t;
{
StrObj so;
AllocPString((cardinal)StrLen(s), &so);
VMPutText(so,s);
so.tag = t;
return so;
}  /* end of makestring */

public integer StringCompare(a,b)
	StrObj a,b;
{
  register integer i = MIN(a.length,b.length);
  register charptr ca = a.val.strval;
  register charptr cb = b.val.strval;
  if (i != 0){
    do{
      if (*ca != *cb) return ((*ca<*cb)?-1:1);
      ca++; cb++;} while (--i != 0);}
  if (a.length == b.length) return 0;
  return ((a.length<b.length)?-1:1);
}

public procedure SubPString(s, beg, len, pstrob)
  StrObj s; cardinal beg, len;  PStrObj pstrob;
{
if (beg > s.length || len > (s.length-beg)) RangeCheck();
STail(s, beg, pstrob);
SHead(*pstrob, len, pstrob);
}  /* end of SubPString */

public procedure PutString(from,beg,into)
	StrObj from; cardinal beg; StrObj into;
{
StrObj strob;
if ((beg > into.length)||(from.length>(into.length-beg))) RangeCheck();
STail(into, beg, &strob);
VMCopyString(from, strob);
}  /* end of PutString */

private boolean sStringMatch(s,t,j)
	StrObj s,t; integer j;
{
  register integer i;
  register charptr sp = s.val.strval;
  register charptr tp = t.val.strval+j;
  if ((i = s.length) !=0){
    do{if (*(sp++) != *(tp++)) return false;} while (--i !=0);}
  return true;
}

/* intrinsics */

public procedure PSString()
{
StrObj s;
cardinal n = PopLimitCard();
ReclaimRecyclableVM ();
AllocPString(n, &s);
os_bzero((char *)s.val.strval, (long int)n);
PushP(&s);
}  /* end of PSString */

public procedure PSSearch()
{
StrObj s,t; register cardinal j;
StrObj r, temp;
register PStrObj ptemp;
ptemp = &temp;
PopPRString(&s);
PopPRString(&t);
for (j=0; ((j < t.length) && ((t.length-j) >= s.length)); j++)
  {
  if (sStringMatch(s,t,(integer)j))
    { /* j is match position in t */
    STail(t, j, &r);
    STail(r, s.length, ptemp); /* part of t following match */
    PushP(ptemp);
    SHead(r, s.length, ptemp); /* part of t matching s */
    PushP(ptemp);
    SHead(t, j, ptemp); /* part of t preceding match */
    PushP(ptemp);
    PushBoolean(true);
    return;
    }
  }
PushP(&t);  /* no match, just push t */
PushBoolean(false);
}  /* end of StringSearch */

public procedure PSAnchorSearch()
{
StrObj s,t, temp;
register PStrObj ptemp;
ptemp = &temp;
PopPRString(&s);
PopPRString(&t);
if ((t.length >= s.length) && sStringMatch(s,t,(integer)0))
  {
  STail(t, s.length, ptemp); /*remainder of t following match*/
  PushP(ptemp);
  SHead(t, s.length, ptemp); /*part of t matching s*/
  PushP(ptemp);
  PushBoolean(true);
  }
else
  {
  PushP(&t);  /* no match just push t */
  PushBoolean(false);
  }
}  /* end of StringAnchorSearch */

private character DigitEncode(x)
	cardinal x;
{ /* x is a digit (< 36) to be converted to character encoding */
  if (x < 10) return ('0'+x);
  x -= 10;
  return ('A'+x);
}

private NumEncode(s,n,r)
	string s; Card32 n; cardinal r;
{
  Card32 d; cardinal i,j; character t[MAXnumeralString];
  for (i=0; ;i++) {
    if (n < r) {t[i] = DigitEncode((cardinal)n); goto ret;}
    d = n % r;
    t[i] = DigitEncode((cardinal)d);
    n = n/r;}
  /* now reverse string */
  ret:
  for (j=0; j <= i; j++) s[j] = t[i-j];
  s[j] = NUL;
}

private procedure TextIntoString(from, into)  string from; PStrObj into;
{
cardinal fromlen = StrLen(from);
if (fromlen > into->length) RangeCheck();
into->length = fromlen;
VMPutText(*into,from);
}  /* end of TextIntoString */

private procedure StrIntoStr(from, into)  StrObj from;  PStrObj into;
{
PutString(from,0,*into);
into->length = from.length;
}  /* end of StrIntoStr */

public procedure PSCVN()
{
StrObj str;  NameObj no;
PopPString(&str);
if (str.length > MAXnameLength) RangeCheck();
StrToName(str, &no);
PushP(&no);
}  /* end of CVN */

public procedure PSCVS()
{
Object ob;
StrObj str, temp;
character s[MAXnumeralString+2];
PopPString(&str);
PopP(&ob);
switch (ob.type)
  {
  case intObj:
    os_sprintf((char *)s,"%d",ob.val.ival);
    TextIntoString(s, &str);
    break;
  case realObj:
    {
    integer i,l; boolean dot;
    os_sprintf((char *)s,"%g",ob.val.rval);
    l = StrLen(s); dot = false;
    for(i=0; i < l; i++)
      {if ((s[i] == '.') || (s[i] == 'e')) {dot = true; break;}}
    if (!dot) {s[l] = '.'; s[l+1] = '0'; s[l+2] = NUL;}
    TextIntoString(s, &str);
    break;
    }
  case boolObj:
    TextIntoString((ob.val.bval) ? (string)"true" : (string)"false", &str);
    break;
  case strObj:
    if (ob.length > str.length) RangeCheck();
    VMCopyString(ob,str); str.length = ob.length;
    break;
  case cmdObj:
    if ((ob.length < rootShared->vm.Shared.cmdCnt) && (ob.val.cmdval != NIL))
      ob.type = nameObj; /* quick and dirty "name object" */
    else {ob = unregistered;
    /* fall through to name case */}
  case nameObj:
    NameToPString(ob, &temp);
    StrIntoStr(temp, &str);
    break;
  default: TextIntoString(novalue, &str);
  }
PushP(&str);
}  /* end of CVS */

public procedure PSCVRS()
{
cardinal r; StrObj str; Object ob; character s[MAXnumeralString];
integer i;
PopPString(&str);
r = PopCardinal();
PopP(&ob);
if ((r == 10) && ((ob.type == intObj) || (ob.type == realObj)))
  {PushP(&ob); PushP(&str); PSCVS(); return;};
if ((r < 2) || (r > 36)) RangeCheck();
switch (ob.type)
  {
  case intObj:
    NumEncode(s,(Card32)ob.val.ival,r); TextIntoString(s, &str); break;
  case realObj:
    if (ob.val.rval < (real) MINInt32 || ob.val.rval > (real) MAXInt32)
      RangeCheck();
    i = ob.val.rval;
    NumEncode(s, (Card32) i, r); TextIntoString(s, &str); break;
  default: TextIntoString(novalue, &str);
  }
PushP(&str);
}  /* end of CVRS */

public procedure StrForAll(strOb, procOb)  StrObj strOb;  AryObj procOb;
{
if ((strOb.access & rAccess) == 0) InvlAccess();
EPushP(&procOb);
EPushP(&strOb);
EPushP(&strcmd);
}  /* end of StrForAll */

private procedure SFAProc()
{
StrObj so, rest; Object ob; integer item;
EPopP(&so);
if (so.type != strObj) TypeCheck();
ETopP(&ob);
if (so.length != 0)
  {
  item = VMGetChar(so, 0);
  STail(so, 1, &rest);
  EPushP(&rest);
  EPushP(&strcmd);
  PushInteger(item);
  EPushP(&ob);}
else EPopP(&ob);
}  /* end of SFAProc */

public procedure StringInit(reason)  InitReason reason;
{
  switch (reason){
    case init: {
      break;}
    case romreg: {
      RgstMark("@stringforall", SFAProc, (integer)(mrk2Args), &strcmd);
      break;}
    endswitch}
}
