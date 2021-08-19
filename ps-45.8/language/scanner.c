/*
  scanner.c

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

Original version: Chuck Geschke: January 24, 1983
Edit History:
Larry Baer: Fri Nov 10 14:37:29 1989
Chuck Geschke: Fri Mar 28 16:40:19 1986
Doug Brotz: Fri Jun  6 09:50:46 1986
Ed Taft: Sun Dec 17 19:08:44 1989
John Gaffney: Thu Jan  3 17:32:01 1985
Don Andrews: Wed Mar 12 10:19:12 1986
Ivor Durham: Sun May 14 13:26:32 1989
Linda Gass: Wed Aug  5 16:54:15 1987
Paul Rovner: Friday, September 25, 1987 11:31:13 AM
Joe Pasqua: Fri Jan  6 14:59:10 1989
Jim Sandman: Thu Oct 26 14:42:42 1989
End Edit History.

Contemplated improvements:
1. Scan strings directly into VM if possible; switch to private buffer
   only if a context switch or reentrant call occurs.
2. Fix bug wherein "//mark" in an array causes confusion.
3. LineComplete should understand about binary tokens.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include FP
#include LANGUAGE
#include PSLIB
#include VM
#include RECYCLER

#include "array.h"
#include "exec.h"
#include "name.h"
#include "packedarray.h"
#include "streampriv.h"
#include "langdata.h"
#include "scanner.h"
#include "grammar.h"

/* Temporary work-around for MIPS compiler bug */

#if	(ISP == isp_r2000) && (OS == os_sysv)
#ifdef	LStrObj
#undef	LStrObj
#endif	LStrObj
#define	LStrObj(o,l,v) \
	o = iLStrObj; (o).length=(l); (o).val.strval=(v); \
	if (CurrentShared()) \
	  {(o).shared = true; (o).level = 0;}\
	else \
	  {(o).shared = false; (o).level = level;}
#endif	(ISP == isp_r2000) && (OS == os_sysv)

/* End of MIPS work-around */

/* Unsigned arithmetic problem work-around for portable-C derived compilers */

private Card32 baseLimitDiv10;	/* Static initialised in ScannerInit */
private Card32 baseLimitMod10;

/* End unsigned work-around. */

private NameObj beginDAcmd, endDAcmd;
  /* These NameObj's are shared across all contexts	*/


/* Character class tables */

private readonly Class noBinClassArray[] = {
#define BINARY_ENCODINGS false
#include "classarray.h"
};

private readonly Class binClassArray[] = {
#undef BINARY_ENCODINGS
#define BINARY_ENCODINGS true
#include "classarray.h"
};


/* Map from digit to binary for all bases [2, 36]; table starts at '0'
   and includes ['A', 'Z'] and ['a', 'z'] 
 */
private readonly Card8 digitToBinary[] = {
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 99, 99, 99, 99, 99, 99,
  99, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
  25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 99, 99, 99, 99, 99,
  99, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
  25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};


/* Map from "\" escaped char to actual char, for chars ['b', 't'] only */
private readonly Card8 escToChar[] = {
  BS/*b*/, 077, 077, 077, FF/*f*/, 077, 077, 077, 077, 077,
  077, 077, EOL/*n*/, 077, 077, 077, CR/*r*/, 077, TAB/*t*/};


/* Table of negative powers of 10 greater than 2**-31 */
private readonly double negP10Tab[] = {
  1.0e0, 1.0e-1, 1.0e-2, 1.0e-3, 1.0e-4, 1.0e-5,
  1.0e-6, 1.0e-7, 1.0e-8, 1.0e-9};


/* Table of binary token (header) sizes for token types
   [bt_objSeqHiIEEE, bt_numArray] only. Sizes are exclusive of
   the initial token type character, except for object sequences
   and number arrays, where they are inclusive.
 */
private readonly Card8 tokenToSize[] = {
  4, 4, 4, 4, 4, 4, 2, 2, 1, 0, 4, 4, 4, 1, 1, 2, 2, 1, 1, 1, 1, 4};


/* Map from binary token type to action to perform after token is read */
private readonly Card8 tokenToAction[] = {
  ba_objSeq, ba_objSeq, ba_objSeq, ba_objSeq,
  ba_int32, ba_int32,
  ba_int16, ba_int16,
  ba_int8,
  ba_fixed,
  ba_realIEEE, ba_realIEEE,
  ba_realNative,
  ba_bool,
  ba_sstr,
  ba_lstr, ba_lstr,
  ba_systemName, ba_systemName,
  ba_userName, ba_userName,
  ba_numArray};


/* The following data structures are regenerated automatically by the
   new_grammar script; they should not be edited manually. */

/* BEGIN_TABLE */
private readonly StateRec startState = {
 {_empty, discard, discard, discard, _discard,
  _begname, _begxtext, _errini, _begxtext, _errini,
  axbeg, axend, adbeg, adend, _begxtext,
  _begxnum, _begxnum, _begname, _begnum, _begnum,
  _begname, _begname, _begname, _begname, _begname,
  _begname, abtokhi, abtoklo, abtokn, abfn,
  abtokhdr, _errini},
 {start, start, start, start, comnt,
  ident, litstr, start, hexstr, start,
  start, start, start, start, slash,
  msign, point, ident, ipart, ipart,
  ident, ident, ident, ident, ident,
  ident, start, start, start, start,
  start, start}};

private readonly StateRec comntState = {
 {_empty, _discard, _discard, discard, discard,
  discard, discard, discard, discard, discard,
  discard, discard, discard, discard, discard,
  discard, discard, discard, discard, discard,
  discard, discard, discard, discard, discard,
  discard, discard, discard, discard, discard,
  discard, discard},
 {start, start, start, comnt, comnt,
  comnt, comnt, comnt, comnt, comnt,
  comnt, comnt, comnt, comnt, comnt,
  comnt, comnt, comnt, comnt, comnt,
  comnt, comnt, comnt, comnt, comnt,
  comnt, comnt, comnt, comnt, comnt,
  comnt, comnt}};

private readonly StateRec msignState = {
 {_xname, cdisceol, _xname, _xname, _uxname,
  _appname, _uxname, _uxname, _uxname, _uxname,
  _uxname, _uxname, _uxname, _uxname, _uxname,
  _appname, _appname, _appname, _appint, _appint,
  _appname, _appname, _appname, _appname, _appname,
  _appname, _uxname, _uxname, _uxname, _uxname,
  _uxname, _uxname},
 {start, msign, start, start, start,
  ident, start, start, start, start,
  start, start, start, start, start,
  ident, point, ident, ipart, ipart,
  ident, ident, ident, ident, ident,
  ident, start, start, start, start,
  start, start}};

private readonly StateRec ipartState = {
 {_inum, cdisceol, _inum, _inum, _uinum,
  _appname, _uinum, _uinum, _uinum, _uinum,
  _uinum, _uinum, _uinum, _uinum, _uinum,
  _appname, _appname, _appname, appint, appint,
  _begexp, _appname, _appname, _appname, _appname,
  _appname, _uinum, _uinum, _uinum, _uinum,
  _uinum, _uinum},
 {start, ipart, start, start, start,
  ident, start, start, start, start,
  start, start, start, start, start,
  ident, fpart, bbase, ipart, ipart,
  expon, ident, ident, ident, ident,
  ident, start, start, start, start,
  start, start}};

private readonly StateRec pointState = {
 {_xname, cdisceol, _xname, _xname, _uxname,
  _appname, _uxname, _uxname, _uxname, _uxname,
  _uxname, _uxname, _uxname, _uxname, _uxname,
  _appname, _appname, _appname, _appfrac, _appfrac,
  _appname, _appname, _appname, _appname, _appname,
  _appname, _uxname, _uxname, _uxname, _uxname,
  _uxname, _uxname},
 {start, point, start, start, start,
  ident, start, start, start, start,
  start, start, start, start, start,
  ident, ident, ident, fpart, fpart,
  ident, ident, ident, ident, ident,
  ident, start, start, start, start,
  start, start}};

private readonly StateRec fpartState = {
 {_rnum, cdisceol, _rnum, _rnum, _urnum,
  _appname, _urnum, _urnum, _urnum, _urnum,
  _urnum, _urnum, _urnum, _urnum, _urnum,
  _appname, _appname, _appname, appfrac, appfrac,
  _begexp, _appname, _appname, _appname, _appname,
  _appname, _urnum, _urnum, _urnum, _urnum,
  _urnum, _urnum},
 {start, fpart, start, start, start,
  ident, start, start, start, start,
  start, start, start, start, start,
  ident, ident, ident, fpart, fpart,
  expon, ident, ident, ident, ident,
  ident, start, start, start, start,
  start, start}};

private readonly StateRec exponState = {
 {_xname, cdisceol, _xname, _xname, _uxname,
  _appname, _uxname, _uxname, _uxname, _uxname,
  _uxname, _uxname, _uxname, _uxname, _uxname,
  _appname, _appname, _appname, _appname, _appname,
  _appname, _appname, _appname, _appname, _appname,
  _appname, _uxname, _uxname, _uxname, _uxname,
  _uxname, _uxname},
 {start, expon, start, start, start,
  ident, start, start, start, start,
  start, start, start, start, start,
  esign, ident, ident, epart, epart,
  ident, ident, ident, ident, ident,
  ident, start, start, start, start,
  start, start}};

private readonly StateRec esignState = {
 {_xname, cdisceol, _xname, _xname, _uxname,
  _appname, _uxname, _uxname, _uxname, _uxname,
  _uxname, _uxname, _uxname, _uxname, _uxname,
  _appname, _appname, _appname, _appname, _appname,
  _appname, _appname, _appname, _appname, _appname,
  _appname, _uxname, _uxname, _uxname, _uxname,
  _uxname, _uxname},
 {start, esign, start, start, start,
  ident, start, start, start, start,
  start, start, start, start, start,
  ident, ident, ident, epart, epart,
  ident, ident, ident, ident, ident,
  ident, start, start, start, start,
  start, start}};

private readonly StateRec epartState = {
 {_rnum, cdisceol, _rnum, _rnum, _urnum,
  _appname, _urnum, _urnum, _urnum, _urnum,
  _urnum, _urnum, _urnum, _urnum, _urnum,
  _appname, _appname, _appname, appname, appname,
  _appname, _appname, _appname, _appname, _appname,
  _appname, _urnum, _urnum, _urnum, _urnum,
  _urnum, _urnum},
 {start, epart, start, start, start,
  ident, start, start, start, start,
  start, start, start, start, start,
  ident, ident, ident, epart, epart,
  ident, ident, ident, ident, ident,
  ident, start, start, start, start,
  start, start}};

private readonly StateRec bbaseState = {
 {_xname, cdisceol, _xname, _xname, _uxname,
  _appname, _uxname, _uxname, _uxname, _uxname,
  _uxname, _uxname, _uxname, _uxname, _uxname,
  _appname, _appname, _appname, _begbnum, _begbnum,
  _begbnum, _begbnum, _begbnum, _begbnum, _begbnum,
  _appname, _uxname, _uxname, _uxname, _uxname,
  _uxname, _uxname},
 {start, bbase, start, start, start,
  ident, start, start, start, start,
  start, start, start, start, start,
  ident, ident, ident, bpart, bpart,
  bpart, bpart, bpart, bpart, bpart,
  ident, start, start, start, start,
  start, start}};

private readonly StateRec bpartState = {
 {_bnum, cdisceol, _bnum, _bnum, _ubnum,
  _appname, _ubnum, _ubnum, _ubnum, _ubnum,
  _ubnum, _ubnum, _ubnum, _ubnum, _ubnum,
  _appname, _appname, _appname, appbnum, appbnum,
  appbnum, appbnum, appbnum, appbnum, appbnum,
  _appname, _ubnum, _ubnum, _ubnum, _ubnum,
  _ubnum, _ubnum},
 {start, bpart, start, start, start,
  ident, start, start, start, start,
  start, start, start, start, start,
  ident, ident, ident, bpart, bpart,
  bpart, bpart, bpart, bpart, bpart,
  ident, start, start, start, start,
  start, start}};

private readonly StateRec identState = {
 {_xname, cdisceol, _xname, _xname, _uxname,
  appname, _uxname, _uxname, _uxname, _uxname,
  _uxname, _uxname, _uxname, _uxname, _uxname,
  appname, appname, appname, appname, appname,
  appname, appname, appname, appname, appname,
  appname, _uxname, _uxname, _uxname, _uxname,
  _uxname, _uxname},
 {start, ident, start, start, start,
  ident, start, start, start, start,
  start, start, start, start, start,
  ident, ident, ident, ident, ident,
  ident, ident, ident, ident, ident,
  ident, start, start, start, start,
  start, start}};

private readonly StateRec slashState = {
 {_lname, cdisceol, _lname, _lname, _ulname,
  _appname, _ulname, _ulname, _ulname, _ulname,
  _ulname, _ulname, _ulname, _ulname, _discard,
  _appname, _appname, _appname, _appname, _appname,
  _appname, _appname, _appname, _appname, _appname,
  _appname, _ulname, _ulname, _ulname, _ulname,
  _ulname, _ulname},
 {start, slash, start, start, start,
  ldent, start, start, start, start,
  start, start, start, start, edent,
  ldent, ldent, ldent, ldent, ldent,
  ldent, ldent, ldent, ldent, ldent,
  ldent, start, start, start, start,
  start, start}};

private readonly StateRec ldentState = {
 {_lname, cdisceol, _lname, _lname, _ulname,
  appname, _ulname, _ulname, _ulname, _ulname,
  _ulname, _ulname, _ulname, _ulname, _ulname,
  appname, appname, appname, appname, appname,
  appname, appname, appname, appname, appname,
  appname, _ulname, _ulname, _ulname, _ulname,
  _ulname, _ulname},
 {start, ldent, start, start, start,
  ldent, start, start, start, start,
  start, start, start, start, start,
  ldent, ldent, ldent, ldent, ldent,
  ldent, ldent, ldent, ldent, ldent,
  ldent, start, start, start, start,
  start, start}};

private readonly StateRec edentState = {
 {_ename, cdisceol, _ename, _ename, _uename,
  appname, _uename, _uename, _uename, _uename,
  _uename, _uename, _uename, _uename, _uename,
  appname, appname, appname, appname, appname,
  appname, appname, appname, appname, appname,
  appname, _uename, _uename, _uename, _uename,
  _uename, _uename},
 {start, edent, start, start, start,
  edent, start, start, start, start,
  start, start, start, start, start,
  edent, edent, edent, edent, edent,
  edent, edent, edent, edent, edent,
  edent, start, start, start, start,
  start, start}};

private readonly StateRec litstrState = {
 {_errstr, disceol, _appeol, appstr, appstr,
  _discard, _strnest, _strg, appstr, appstr,
  appstr, appstr, appstr, appstr, appstr,
  appstr, appstr, appstr, appstr, appstr,
  appstr, appstr, appstr, appstr, appstr,
  appstr, appstr, appstr, appstr, appstr,
  appstr, appstr},
 {start, litstr, litstr, litstr, litstr,
  escchr, litstr, start, litstr, litstr,
  litstr, litstr, litstr, litstr, litstr,
  litstr, litstr, litstr, litstr, litstr,
  litstr, litstr, litstr, litstr, litstr,
  litstr, litstr, litstr, litstr, litstr,
  litstr, litstr}};

private readonly StateRec escchrState = {
 {_errstr, _esceol, _esceol, _appstr, _appstr,
  _appstr, _appstr, _appstr, _appstr, _appstr,
  _appstr, _appstr, _appstr, _appstr, _appstr,
  _appstr, _appstr, _appstr, _begoct, _appstr,
  _appstr, _appesc, _appesc, _appstr, _appstr,
  _appstr, _appstr, _appstr, _appstr, _appstr,
  _appstr, _appstr},
 {start, litstr, litstr, litstr, litstr,
  litstr, litstr, litstr, litstr, litstr,
  litstr, litstr, litstr, litstr, litstr,
  litstr, litstr, litstr, escoct1, litstr,
  litstr, litstr, litstr, litstr, litstr,
  litstr, litstr, litstr, litstr, litstr,
  litstr, litstr}};

private readonly StateRec escoct1State = {
 {_errstr, disceol, _appeol, _appstr, _appstr,
  _discard, _strnest, _strg, _appstr, _appstr,
  _appstr, _appstr, _appstr, _appstr, _appstr,
  _appstr, _appstr, _appstr, _appoct, _appstr,
  _appstr, _appstr, _appstr, _appstr, _appstr,
  _appstr, _appstr, _appstr, _appstr, _appstr,
  _appstr, _appstr},
 {start, escoct1, litstr, litstr, litstr,
  escchr, litstr, start, litstr, litstr,
  litstr, litstr, litstr, litstr, litstr,
  litstr, litstr, litstr, escoct2, litstr,
  litstr, litstr, litstr, litstr, litstr,
  litstr, litstr, litstr, litstr, litstr,
  litstr, litstr}};

private readonly StateRec escoct2State = {
 {_errstr, disceol, _appeol, _appstr, _appstr,
  _discard, _strnest, _strg, _appstr, _appstr,
  _appstr, _appstr, _appstr, _appstr, _appstr,
  _appstr, _appstr, _appstr, _appoct, _appstr,
  _appstr, _appstr, _appstr, _appstr, _appstr,
  _appstr, _appstr, _appstr, _appstr, _appstr,
  _appstr, _appstr},
 {start, escoct2, litstr, litstr, litstr,
  escchr, litstr, start, litstr, litstr,
  litstr, litstr, litstr, litstr, litstr,
  litstr, litstr, litstr, litstr, litstr,
  litstr, litstr, litstr, litstr, litstr,
  litstr, litstr, litstr, litstr, litstr,
  litstr, litstr}};

private readonly StateRec hexstrState = {
 {_errstr, discard, discard, discard, _errhex,
  _errhex, _errhex, _errhex, _errhex, _hstrg,
  _errhex, _errhex, _errhex, _errhex, _errhex,
  _errhex, _errhex, _errhex, apphex, apphex,
  apphex, apphex, _errhex, apphex, _errhex,
  _errhex, _errhex, _errhex, _errhex, _errhex,
  _errhex, _errhex},
 {start, hexstr, hexstr, hexstr, start,
  start, start, start, start, start,
  start, start, start, start, start,
  start, start, start, hexstr, hexstr,
  hexstr, hexstr, start, hexstr, start,
  start, start, start, start, start,
  start, start}};


private readonly PStateRec stateArray[] = {
  &startState, &comntState, &msignState, &ipartState, &pointState,
  &fpartState, &exponState, &esignState, &epartState, &bbaseState,
  &bpartState, &identState, &slashState, &ldentState, &edentState,
  &litstrState, &escchrState, &escoct1State, &escoct2State, &hexstrState};
/* END_TABLE */


private procedure AryToMrk(pobj, nest, sqarray)
  PObject pobj; integer nest; boolean sqarray;
{
  cardinal size = CountToMark(opStk);
  Object m;

  if (packedArrayMode && !sqarray)
    Pkdary(size, pobj);
  else
#if 0
    Array(size, pobj);
#else
    {
      AllocPArray(size, pobj);
      AStore(*pobj);
    }
#endif
  if (!sqarray) pobj->tag = Xobj;
  IPopSimple(opStk, &m);
  DebugAssert(m.type == escObj && m.length == objMark);
}


typedef struct _StrExtension {
  struct _StrExtension *link;
  Card8 buf[1024 - sizeof(struct _StrExtension *)];
} StrExtension, *PStrExtension;

typedef struct {
  Int32 count;		/* chars in all buffers prior to current one;
			   0 means no extensions are present */
  PStrExtension first,	/* -> first extension in chain */
		curr;	/* -> current (i.e., last) extension in chain */
  union {		/* original (non-extended) buffer */
    Card8 bytes[MAXnameLength + 1]; /* raw bytes */
    Int32 int32;	/* various binary token interpretations */
    Int16 int16;
    Card16 card16;
    FloatRep rval;
    BOSHeader objSeq;
    HNAHeader numArray;
    } buf;
} StrStorage, *PStrStorage;


private boolean ExtendStrStorage(pss)
  PStrStorage pss;
/* Attempts to add another extension buffer to string storage.
   If successful, updates pss->count to reflect having filled
   the previous buffer (either the original buf or a previous
   extension) and makes pss->curr point to the new extension.
   Returns true normally. If unsuccessful, returns false; the existing
   string storage is not disturbed and the count is not updated.
 */
{
  PStrExtension pse;
  if (pss->count >= MAXstringLength - sizeof(pse->buf) ||
      (pse = (PStrExtension) os_malloc((integer) sizeof(StrExtension)))
      == NIL)
    return false;  /* exceeded max string length or exhausted malloc */
  if (pss->count == 0)
    {
    pss->first = pss->curr = pse;
    pss->count = MAXnameLength;
    }
  else
    {
    pss->curr->link = pse;
    pss->curr = pse;
    pss->count += sizeof(pse->buf);
    }
  pse->link = NIL;
#if STAGE==DEVELOP
  strStorageBufCount++;
#endif STAGE==DEVELOP
  return true;
}


private procedure FreeStrStorage(pss)
  PStrStorage pss;
{
  PStrExtension pse;
  if (pss->count == 0) return;
  while ((pse = pss->first) != NIL)
    {
    pss->first = pse->link;
    os_free((char *) pse);
#if STAGE==DEVELOP
    strStorageBufCount--;
#endif STAGE==DEVELOP
    }
  pss->count = 0;
}


private procedure ObjFromStrStorage(pss, bufPtr, pobj)
  PStrStorage pss; PCard8 bufPtr; PStrObj pobj;
/* Allocates a string object in VM, copies the contents of string storage
   into it, and frees the string storage. bufPtr is the ending buffer
   pointer and is used in computing the size of the string.
   The resulting string object is returned in *pobj. If the VM allocation
   causes an error, the string storage is freed nonetheless.
 */
{
  PCard8 cp;
  integer count;
  PStrExtension pse;

  if ((count = pss->count) == 0) count = bufPtr - &pss->buf.bytes[0];
  else count += bufPtr - &pss->curr->buf[0];
  DebugAssert(count <= MAXstringLength);

  DURING
    AllocPString((cardinal)count, pobj);
  HANDLER
    FreeStrStorage(pss);
    RERAISE;
  END_HANDLER;

  cp = pobj->val.strval;
  os_bcopy(
    (char *)pss->buf.bytes, (char *)cp,
    (integer) (count < MAXnameLength) ? count : MAXnameLength);
  cp += MAXnameLength;
  count -= MAXnameLength;
  for (pse = pss->first; count > 0; pse = pse->link)
    {
    os_bcopy(
      (char *)pse->buf, (char *)cp,
      (integer) (count < sizeof(pse->buf)) ? count : sizeof(pse->buf));
    cp += sizeof(pse->buf);
    count -= sizeof(pse->buf);
    }
  FreeStrStorage(pss);
}


public procedure UndefNameIndex(table, index)
  char *table; integer index;
{
  NameObj nobj;
  char str[20];
  os_sprintf(str, "%s%D", table, index);
  MakePName(str, &nobj);
  PushP(&nobj);
  stackRstr = false;
  Undefined();
}


private int SafeFilBuf(stm, pss)
  Stm stm; PStrStorage pss;
{
  int c;
  DURING
    c = (*stm->procs->FilBuf)(stm);
  HANDLER
    FreeStrStorage(pss);
    RERAISE;
  END_HANDLER;
  return c;
}


private procedure SafeUnGetc(c, stm, pss)
  int c; Stm stm; PStrStorage pss;
{
  DURING
    if (ungetc(c, stm) < 0) CantHappen();
  HANDLER
    FreeStrStorage(pss);
    RERAISE;
  END_HANDLER;
}


#define SETUPSTM stmPtr = stm->ptr; stmCnt = stm->cnt
#define UPDATESTM stm->ptr = stmPtr; stm->cnt = stmCnt

#define GETC \
  if (--stmCnt < 0) { \
    UPDATESTM; \
    if (ss.count != 0) c = SafeFilBuf(stm, &ss); \
    else c = (*stm->procs->FilBuf)(stm); \
    stm->flags.f1 = false; \
    SETUPSTM; \
    } \
  else c = (unsigned char) *stmPtr++

/* Note: stm->ptr (therefore stmPtr) is undefined for an unbuffered stream.
   stmCnt > 0 is a sufficient indication that the stream is buffered;
   stmPtr > stm->ptr is a sufficient indication that we actually read the
   last character from the buffer and therefore don't need to call ungetc
   to put it back.
 */
#define UNGETC \
  if (stmCnt <= 0 || stmPtr <= stm->ptr) { \
    UPDATESTM; \
    SafeUnGetc(c, stm, &ss); \
    SETUPSTM; \
    } \
  else {stmPtr--; stmCnt++;}

#define NEWSTATE state = stateArray[state->newStates[class]]


public boolean StmToken(stm, pobj, exec)
  Stm stm; PObject pobj; boolean exec;
{
  register char *stmPtr;	/* temporary stm->ptr */
  register PCard8 bufPtr;	/* tail of text being accumulated */
  register Int32 stmCnt,	/* temporary stm->cnt */
		 bufCnt;	/* space remaining in text buffer */
  register int c,		/* current character */
	       class;		/* current character class */
  register PStateRec state;	/* current state */
  PCard8 classArray;		/* -> binClassArray or noBinClassArray */
  Int32 nest,			/* nesting level in { } */
	scale;			/* decimal scale factor for number;
				   base part of based number;
				   nesting level in ( );
				   even/odd flag in < > */
  Card32 num,			/* number being accumulated */
	 baseLimit;		/* (max representable value + 1)/base;
				   0 if num has overflowed */
  boolean negative;		/* true if num is negative */
  StrStorage ss;		/* text buffer and extension chain */

  classArray = (objectFormat == 0)? &noBinClassArray[1] : &binClassArray[1];
  nest = 0;
  ss.count = 0;
  state = &startState;
  ReclaimRecyclableVM ();	/* Reclaim VM before scanning token */
#if STAGE==DEVELOP
  DebugAssert(strStorageBufCount == 0);
    /* This could fail due to a reentrant call by the same context,
       but that should not happen in practice. */
#endif STAGE==DEVELOP
  SETUPSTM;

  while (true)
    {
    GETC;

  DispatchClass:
    class = classArray[c];

    switch (state->actions[class])
      {
      case discard:
        break;

      case cdisceol:
        /* Conditionally discard EOL: inspect next character only if
	   input buffer is non-empty or we are inside a procedure;
	   otherwise, set the "EOL pending" flag in the stream.
	   This is used only for a CR that terminates a top-level token;
	   we can't read ahead because doing so might cause a deadlock.
	 */
	if (stm->cnt <= 0 && nest == 0)
	  {stm->flags.f1 = true; goto ReDispatchLF;}
	/* fall through */

      case disceol:
	GETC;
	if (c != LF && c != EOF) UNGETC;
      ReDispatchLF:
	c = LF;
	goto DispatchClass;

      case _begxnum:
        num = 0;
	negative = (c == '-');
	goto NumCommon;

      case _begnum:
        num = c - '0';
	negative = false;
      NumCommon:
	scale = 0;
	baseLimit = baseLimitDiv10;
	/* fall through */

      case _begname:
        bufPtr = &ss.buf.bytes[0];
	*bufPtr++ = c;
	bufCnt = MAXnameLength - 1;
	goto NewState;

      case _begxtext:
	scale = 0;  /* init string nesting level */
        bufPtr = &ss.buf.bytes[0];
	bufCnt = MAXnameLength;
	goto NewState;

      case apphex:
        c = digitToBinary[c - '0'];
	if (--scale < 0) {scale = 1; c <<= 4; goto AppendStr;}
	else *(bufPtr - 1) += c;
	break;

      case _appesc:
        c = escToChar[c - 'b'];
	goto AppendStrNS;

      case _appeol:
        switch (c)
	  {
	  case CR:
	    break;  /* can't happen, because scanner always consumes CR
		       and invokes disceol, so next char is always LF */
	  case LF:
	    c = EOL;  /* map to canonical end-of-line character */
	    goto AppendStrNS;
	  case FF:
	    goto AppendStrNS;
	  }
	CantHappen();

      case _esceol:
        switch (c)
	  {
	  case CR:
	    GETC;
	    if (c != LF) UNGETC;
	    goto NewState;
	  case LF:
	    goto NewState;
	  case FF:
	    goto AppendStrNS;
	  }
	CantHappen();

      case _begoct:
        c -= '0';
	goto AppendStrNS;

      case _appoct:
        *(bufPtr - 1) = (*(bufPtr - 1) << 3) + c - '0';
	goto NewState;

      case _appint:
        NEWSTATE;
	goto DoAppint;

      case _appfrac:
        NEWSTATE;
	/* fall through */

      case appfrac:
        scale++;
	/* fall through */

      case appint:
      DoAppint:
	/* Test explicitly for overflow, since the handling of integer
	   overflows is not well defined in C. This test assumes 2's
	   complement representation; i.e., the most negative integer's
	   magnitude is one greater than the most positive integer. */
	if (num >= baseLimit &&
	    ! (num == baseLimit &&
	       ((negative)? c <= '0' + baseLimitMod10 :
	       		   c < '0' + baseLimitMod10)))
	  baseLimit = 0;  /* number has overflowed */
	else num = 10 * num + c - '0';
	goto AppendName;

      case _begbnum:
	if (num < 2 || num > 36 || negative)
	  {
	  /* if base not in [2, 36], token is syntactically not a number */
	  state = &identState; goto AppendName;
	  }
	scale = num;  /* this is the base */
	num = 0;
	baseLimit = MAXCard32 / (Card32) scale;
        NEWSTATE;
	/* fall through */

      case appbnum:
        class = digitToBinary[c - '0']; /* class holds digit temporarily */
	if (class >= scale)
	  {
	  /* if digit >= base, token is syntactically not a number */
	  state = &identState; goto AppendName;
	  }
	if (num >= baseLimit &&
	    ! (num == baseLimit && class <= MAXCard32 % (Card32) scale))
	  baseLimit = 0;  /* number has overflowed */
	else num = (Card32) scale * num + class;
	goto AppendName;

      case _begexp:
        baseLimit = 0;  /* punt conversion; let atof do it */
	goto AppendNameNS;

      case _uxname:
        UNGETC;
	/* fall through */

      case _xname:
        UPDATESTM;
        FastName(
          &ss.buf.bytes[0], (cardinal)(bufPtr - &ss.buf.bytes[0]), pobj);
	goto RetXObj;

      case _ulname:
        UNGETC;
	/* fall through */

      case _lname:
        UPDATESTM;
        FastName(
          &ss.buf.bytes[0], (cardinal)(bufPtr - &ss.buf.bytes[0]), pobj);
	pobj->tag = Lobj;
	goto RetLObj;

      case _uename:
        UNGETC;
	/* fall through */

      case _ename:
        UPDATESTM;
        FastName(
          &ss.buf.bytes[0], (cardinal)(bufPtr - &ss.buf.bytes[0]), pobj);
	if (! LoadName(pobj->val.nmval, pobj))
	  {
	  PushP(pobj);
	  stackRstr = false;
	  psERROR = undefined;
	  num = PS_ERROR;
	  goto DoUPDATEDError;
	  }
	if (pobj->tag != Lobj)
	  switch (pobj->type)
	    {
	    case arrayObj:
	    case pkdaryObj:
	      goto RetLObj;  /* deferred array execution */
	    default:
	      goto RetXObj;
	    }
	goto RetLObj;

      case _strnest:
        scale++;  /* increment ( ) nesting level */
	goto AppendStrNS;

      case _strg:
        if (--scale >= 0)  /* decrement and test ( ) nesting level */
	  {
	  state = &litstrState;
	  goto AppendStr;
	  }
	/* fall through */

      case _hstrg:
	UPDATESTM;
	if ((bufCnt = bufPtr - &ss.buf.bytes[0]) == 0)
	  {
	  /* Special case zero length strings */
	  LStrObj(*pobj, 0, NIL);
	  goto RetLObj;
	  }
	if (ss.count == 0 &&
	    vmCurrent->free + bufCnt <= vmCurrent->last)
	  {
	  LStrObj(*pobj, bufCnt, vmCurrent->free);
	  os_bcopy((char *)&ss.buf.bytes[0], (char *)vmCurrent->free, bufCnt);
	  vmCurrent->free += bufCnt;
	  ExtendRecycler (vmCurrent->recycler, vmCurrent->free);
	  }
	else
	  ObjFromStrStorage(&ss, bufPtr, pobj);

        goto RetLObj;

      case _uinum:
        UNGETC;
	/* fall through */

      case _inum:
      DoInum:
        UPDATESTM;
        if (baseLimit == 0) goto CallAtof;  /* overflowed, treat as real */
        if (negative) {LIntObj(*pobj, - (Int32) num);}
	else {LIntObj(*pobj, (Int32) num);}
	goto RetLObj;

      case _ubnum:
        UNGETC;
	/* fall through */

      case _bnum:
	UPDATESTM;
        if (baseLimit == 0)
	  {num = ecLimitCheck; goto DoUPDATEDError;}
	LIntObj(*pobj, num);
	goto RetLObj;

      case _urnum:
        UNGETC;
	/* fall through */

      case _rnum:
        UPDATESTM;
        if (baseLimit == 0 || scale >= sizeof(negP10Tab)/sizeof(double))
	  goto CallAtof;
	bufCnt = num;  /* num is a Card32; launder it through an Int32 */
	if (negative) bufCnt = -bufCnt;
	LRealObj(*pobj, (double) bufCnt * negP10Tab[scale]);
	goto RetLObj;

      CallAtof:
        *bufPtr = '\0';
	DURING
	  LRealObj(*pobj, os_atof((char *) &ss.buf.bytes[0]));
	  if (! IsValidReal(&pobj->val.rval)) LimitCheck();
	HANDLER
	  num = ecLimitCheck;
	  goto DoUPDATEDError;
	END_HANDLER;
	goto RetLObj;

      case adbeg:
        UPDATESTM;
        *pobj = beginDAcmd;
	goto RetXObj;

      case adend:
        UPDATESTM;
        *pobj = endDAcmd;
	goto RetXObj;

      case axbeg:
        UPDATESTM;
	nest++;
	LMarkObj(*pobj);
	goto PushObj;

      case axend:
        UPDATESTM;
	if (nest == 0) goto DoErrini;  /* extraneous "}" */
	AryToMrk(pobj, nest, false); /* build executable array */
	nest--;
	goto RetLObj; /* not RetXObj; arrays aren't immediately executed */

      case _empty:
        UPDATESTM;
	if (nest == 0)
	  {  /* stream ended before any token */
	  DebugAssert(ss.count == 0 && strStorageBufCount == 0);
	  return false;
	  }

	/* Stream ended with unterminated array. Finish building the
	   array and then report it as the error-causing object */
	do
	  {
	  AryToMrk(pobj, nest, false);
	  PushP(pobj);
	  }
	while (--nest != 0);
	stackRstr = false;
	PSError(syntaxerror);

      case _errini:
      DoErrini:
        /* Error on initial character of a token. Construct a string
	   object consisting of the single offending character */
        bufPtr = &ss.buf.bytes[0];
	bufCnt = MAXnameLength;
	/* fall through */

      case _errhex:
        /* Error in body of token. Construct a string object consisting
	   of the text accumulated so far, including the new character. */
	if (--bufCnt >= 0) *bufPtr++ = c;  /* append only if it fits */
	/* fall through */

      case _errstr:
      DoSyntaxError:
        num = PS_ERROR;
        psERROR = syntaxerror;
	/* fall through */

      DoError:
        /* Report exception whose code is num; if num is PS_ERROR,
           the error name is already stored in psERROR. Normally,
	   the "offending command" will be a string object constructed
	   from the contents of string storage. However, if an error
	   object has already been pushed (stackRstr false), that object
	   is reported and string storage is ignored. */
        UPDATESTM;
      DoUPDATEDError:
	if (! stackRstr) PopP(pobj);
	while (--nest >= 0) PSClrToMrk();  /* discard partial arrays */
	ReclaimRecyclableVM ();
	if (stackRstr) ObjFromStrStorage(&ss, bufPtr, pobj);
	PushP(pobj);
	stackRstr = false;
	RAISE((int)num, (char *)NIL);
	break;

      case _appname:
      AppendNameNS:	/* append to name and switch state */
        NEWSTATE;
	/* fall through */

      case appname:
      AppendName:	/* append to name and do not switch state */
        if (--bufCnt >= 0)
	  {*bufPtr++ = c; break;}
	num = ecLimitCheck;
	goto DoError;

      case _appstr:
      AppendStrNS:	/* append to string and switch state */
        NEWSTATE;
	/* fall through */

      case appstr:
      AppendStr:	/* append to string and do not switch state */
        if (--bufCnt >= 0)
	  {*bufPtr++ = c; break;}
	if (! ExtendStrStorage(&ss))
	  {num = ecLimitCheck; goto DoError;}
	bufPtr = &ss.curr->buf[0];
	bufCnt = MAXstringLength - ss.count - 1;
	if (bufCnt > sizeof(ss.curr->buf) - 1)
	  bufCnt = sizeof(ss.curr->buf) - 1;
	*bufPtr++ = c;
	break;

      case abtokhdr:
        UNGETC;		/* cause initial character to be captured in buffer */
	goto BTokHeader;

      case abfn:
        num = c - bt_objSeqHiIEEE;  /* save token type byte */
	GETC;
	if ((scale = c) >= 128) scale -= 128;
	if (scale < 0 || scale >= 48) goto BTokError;
	negative = scale < 32;  /* true if 32-bit, false if 16-bit */
	if (negative) bufCnt = 4;
	else {scale -= 32; bufCnt = 2;}
#if SWAPBITS
	if (c >= 128) goto BTokNative; else goto BTokReverse;
#else SWAPBITS
	if (c >= 128) goto BTokReverse; else goto BTokNative;
#endif SWAPBITS

#if SWAPBITS
      case abtoklo:
#else SWAPBITS
      case abtokhi:
#endif SWAPBITS
      case abtokn:
      BTokHeader:
	num = c - bt_objSeqHiIEEE;  /* save token type byte */
	bufCnt = tokenToSize[num];

      BTokNative:	/* binary token, bytes in native order */
	bufPtr = &ss.buf.bytes[0];
	do
	  {
	  GETC;
	  if (c < 0) goto BTokError;
	  *bufPtr++ = c;
	  } while (--bufCnt > 0);
	goto BTokDispatch;

#if SWAPBITS
      case abtokhi:
#else SWAPBITS
      case abtoklo:
#endif SWAPBITS
	num = c - bt_objSeqHiIEEE;  /* save token type byte */
	bufCnt = tokenToSize[num];

      BTokReverse:	/* binary token, bytes in reverse order */
        bufPtr = &ss.buf.bytes[bufCnt];
	do
	  {
	  GETC;
	  if (c < 0) goto BTokError;
	  *--bufPtr = c;
	  } while (--bufCnt > 0);

      BTokDispatch:	/* value now in buf with bytes in native order */
        UPDATESTM;
        switch (tokenToAction[num])
	  {
	  case ba_objSeq:
	    DURING
	      ReadBinObjSeq(stm, &ss.buf.objSeq, pobj);
	    HANDLER
	      bufPtr = &ss.buf.bytes[0];
	      os_sprintf((char *)bufPtr,
	        "bin obj seq, type=%d, elements=%d, size=%d",
		ss.buf.objSeq.type, ss.buf.objSeq.lenTopArray,
		ss.buf.objSeq.size);
	      if (Exception.Message != NIL)
	        {
		os_strcat((char *)bufPtr, ", ");
		os_strcat((char *)bufPtr, Exception.Message);
		}
	      bufPtr = &ss.buf.bytes[os_strlen((char *)bufPtr)];
	      num = Exception.Code;
	      goto DoUPDATEDError;
	    END_HANDLER;
	    goto RetXObj;

	  case ba_int32:
	    LIntObj(*pobj, ss.buf.int32);
	    goto RetLObj;

	  case ba_int16:
	    LIntObj(*pobj, ss.buf.int16);
	    goto RetLObj;

	  case ba_int8:
	    /* not all C implementations have signed chars, so be safe */
	    LIntObj(*pobj, ss.buf.bytes[0]);
	    if (pobj->val.ival >= 128) pobj->val.ival -= 256;
	    goto RetLObj;

	  case ba_fixed:
	    if (! negative) ss.buf.int32 = ss.buf.int16;
	    if (scale == 0) {LIntObj(*pobj, ss.buf.int32);}
	    else {LRealObj(*pobj, os_ldexp((double) ss.buf.int32, -scale));}
	    goto RetLObj;

	  case ba_realIEEE:
	    /* IEEE real is now in native order */
#if (! IEEEFLOAT)
	    LRealObj(*pobj, 0.0);
	    DURING
#if SWAPBITS
	      IEEELowToNative(&ss.buf.rval.ieee, &pobj->val.rval);
#else SWAPBITS
	      IEEEHighToNative(&ss.buf.rval.ieee, &pobj->val.rval);
#endif SWAPBITS
	    HANDLER num = Exception.Code; goto DoError; END_HANDLER;
	    goto RetLObj;
#endif (! IEEEFLOAT)
	    /* else fall through (native format is IEEE) */

	  case ba_realNative:
	    LRealObj(*pobj, ss.buf.rval.native);
	    if (! IsValidReal(&pobj->val.rval))
	      {num = ecUndefResult; goto DoError;}
	    goto RetLObj;

	  case ba_bool:
	    if ((num = ss.buf.bytes[0]) > 1) goto BTokError;
	    LBoolObj(*pobj, num);
	    goto RetLObj;

	  case ba_sstr:
	    ss.buf.card16 = ss.buf.bytes[0];
	    /* fall through */

	  case ba_lstr:
	    bufCnt = ss.buf.card16;
	    if (bufCnt == 0)
	      {LStrObj(*pobj, 0, NIL); goto RetLObj;}
	    if (vmCurrent->free + bufCnt <= vmCurrent->last)
	      {
	      LStrObj(*pobj, bufCnt, vmCurrent->free);
	      vmCurrent->free += bufCnt;
	      }
	    else AllocPString((cardinal)bufCnt, pobj);
	    if (fread(pobj->val.strval, 1, bufCnt, stm) != bufCnt)
	      goto BTokError;
	    goto RetLObj;

	  case ba_systemName:
	    scale = (Int32) rootShared->nameMap.val.namearrayval->nmEntry[ss.buf.bytes[0]];
	    if (scale == NIL)
	      UndefNameIndex("system", (integer)ss.buf.bytes[0]);
	    goto BNameCommon;

	  case ba_userName:
	    {
	    PNameArrayBody pna = rootPrivate->nameMap.val.namearrayval;
	    scale = ss.buf.bytes[0];
	    if (pna == NIL ||
	        scale >= pna->length ||
		(scale = (Int32)pna->nmEntry[scale]) == NIL)
	      UndefNameIndex("user", (integer)ss.buf.bytes[0]);
	    }
	    /* fall through */

	  BNameCommon:
	    switch (num)
	      {
	      case bt_systemLName - bt_objSeqHiIEEE:
	      case bt_userLName - bt_objSeqHiIEEE:
	        LNameObj(*pobj, (PNameEntry)scale);
		goto RetLObj;
	      case bt_systemXName - bt_objSeqHiIEEE:
	      case bt_userXName - bt_objSeqHiIEEE:
	        XNameObj(*pobj, (PNameEntry)scale);
		goto RetXObj;
	      default:
	        CantHappen();
	      }

	  case ba_numArray:
	    DURING
	      ReadNumAry(stm, &ss.buf.numArray, pobj);
	    HANDLER
	      bufPtr = &ss.buf.bytes[0];
	      os_sprintf((char *)bufPtr,
	        "bin num array, rep=%d, length=%d",
		ss.buf.numArray.rep, ss.buf.numArray.length);
	      if (Exception.Message != NIL)
	        {
		os_strcat((char *)bufPtr, ", ");
		os_strcat((char *)bufPtr, Exception.Message);
		}
	      bufPtr = &ss.buf.bytes[os_strlen((char *)bufPtr)];
	      num = Exception.Code;
	      goto DoUPDATEDError;
	    END_HANDLER;
	    goto RetLObj;

	  default:
	    CantHappen();
	  }

      BTokError:
        os_sprintf((char *)&ss.buf.bytes[0],
	  "binary token, type=%d", class + bt_objSeqHiIEEE);
	bufPtr = &ss.buf.bytes[os_strlen((char *)&ss.buf.bytes[0])];
	goto DoSyntaxError;

      RetLObj:		/* return literal object (UPDATESTM done by now) */
        if (exec) goto PushObj;
      RetXObj:		/* return executable object (UPDATESTM done by now) */
        DebugAssert(ss.count == 0 && strStorageBufCount == 0);
        if (nest == 0) return true;
      PushObj:
        IPush(opStk, *pobj);
	SETUPSTM;
	/* fall through */

      case _discard:
      NewState:		/* switch to new state */
        NEWSTATE;
	break;

      default:
        CantHappen();
      }
    }
}


public boolean StrToken (ob, rem, ret, exec)
  StrObj ob; PStrObj rem; PObject ret; boolean exec;
{
  boolean result;
  Int32 count;
  StmRec srec;

  srec.procs = &strStmProcs;
  srec.base = srec.ptr = (char *) ob.val.strval;
  srec.cnt = ob.length;
  result = StmToken(&srec, ret, exec);
  count = srec.ptr - srec.base;
  ob.val.strval += count;
  ob.length -= count;
  *rem = ob;
  return result;
}


public boolean LineComplete(stm)
  register Stm stm;
{
  register int c,		/* current character */
	       class;		/* current character class */
  register PStateRec state;	/* current state */
  PCard8 classArray;		/* -> binClassArray or noBinClassArray */
  Int32 nest,			/* nesting level in { } */
	scale;			/* decimal scale factor for number;
				   base part of based number;
				   nesting level in ( );
				   even/odd flag in < > */

  classArray = (objectFormat == 0)? &noBinClassArray[1] : &binClassArray[1];
  nest = 0;
  scale = 0;
  state = &startState;

  while (true)
    {
    c = getc(stm);

  DispatchClass:
    class = classArray[c];

    switch (state->actions[class])
      {
      /* These actions are the only ones that affect the state machine in
         any way that would affect nesting level or division into tokens */
      case cdisceol:
      case disceol:
        /* don't fool around; scanner may see extra LF, but no matter */
        c = LF;
	goto DispatchClass;

      case _uxname:
      case _ulname:
      case _uename:
      case _uinum:
      case _urnum:
        if (ungetc(c, stm) < 0) CantHappen;
	break;

      case _strnest:
        scale++;  /* increment ( ) nesting level */
	break;

      case _strg:
        if (--scale >= 0)  /* decrement and test ( ) nesting level */
	  {state = &litstrState; continue;}	/* bypass usual NEWSTATE */
	else scale = 0;
	break;

      case axbeg:
        nest++;
	break;

      case _empty:
        return (nest == 0);

      case _errstr:
        return false;

      case axend:
        if (--nest >= 0) break;
	/* fall through */

      case _errini:
      case _errhex:
        fflush(stm);
	return true;

      default:
        break;
      }
    NEWSTATE;
    }
}


public procedure PSToken()
{
  Object ob, token;
  StrObj rem;
  boolean result;
  Stm stm;

  PopP(&ob);
  switch (ob.type)
    {
    case strObj:
      if ((ob.access & rAccess) == 0) InvlAccess();
      RecyclerPush (&ob);	/* Ensure recyclable string not eaten */
      result = StrToken(ob, &rem, &token, false);
      RecyclerPop (&ob);
      if (result) {PushP(&rem); PushP(&token);}
      break;
    case stmObj:
      if ((ob.access & rAccess) == 0 && !IsCrFile(ob)) InvlAccess();
      stm = GetStream(ob);
      result = StmToken(stm, &token, false);
      if (result) PushP(&token);
      else if (ferror(stm)) StreamError(stm);
      else CloseFile(ob, false);
      break;
    default: TypeCheck();
    }
  PushBoolean(result);
}


private procedure PSLBrak()
{
  MarkObj m;
  LMarkObj(m);
  PushP(&m);
}


private procedure PSRBrak()
{
  AryObj a;
  ReclaimRecyclableVM ();
  AryToMrk(&a, (integer)0, true);
  PushP(&a);
}


public procedure PSStPacking()
{
  boolean b = PopBoolean();
  WriteContextParam(
    (char *)&packedArrayMode, (char *)&b,
    (integer)sizeof(packedArrayMode), (PVoidProc)NIL);
}


public procedure PSCrPacking()
{
  PushBoolean (packedArrayMode);
}


public procedure ScannerInit(reason)  InitReason reason;
{
Object ob;
switch (reason)
  {
  case init:
    baseLimitDiv10 = (Card32) MAXInt32 + 1;
    baseLimitDiv10 /= (Card32) 10;
    baseLimitMod10 = (Card32) MAXInt32 + 1;
    baseLimitMod10 %= (Card32) 10;
#if STAGE==DEVELOP
    Assert(sizeof(binClassArray) == 257 && sizeof(noBinClassArray) == 257);
    Assert(binClassArray['0'+1] == oct && binClassArray['A'+1] == hex &&
           binClassArray['a'+1] == hex && binClassArray['}'+1] == rbr &&
	   binClassArray[149+1] == btokhdr && noBinClassArray[149+1] == oth);
#endif STAGE==DEVELOP
#if OS==os_vaxeln
    pCrtlCtx = eln$locate_crtl_ctx();
#endif OS==os_vaxeln
    break;
  case romreg:
    RgstExplicit("[", PSLBrak);
    RgstExplicit("]", PSRBrak);
    MakePName("[", &beginDAcmd);
    MakePName("]", &endDAcmd);
    break;
  case ramreg:
    break;
  endswitch
  }
}
