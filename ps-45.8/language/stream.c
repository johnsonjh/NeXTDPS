/*
  stream.c

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

Original version: Chuck Geschke: February 13, 1983
Edit History:
Larry Baer: Mon Nov 20 09:33:26 1989
Chuck Geschke: Tue Dec 31 16:22:21 1985
Tom Boynton: Fri Mar 18 10:18:56 1983
Doug Brotz: Fri Aug 29 16:15:49 1986
Ed Taft: Sun Dec 17 19:09:44 1989
Dick Sweet: Mon Oct 20 22:27:15 PDT 1986
Bill Paxton: Mon Jan 14 14:05:33 1985
Don Andrews: Tue Apr 7 11:20:44 PST 1987
Bill McCoy: Mon Mar 16 18:21:42 PST 1987
John Nash: Tue Apr 21 10:04:21 PDT 1987
Ivor Durham: Thu May 18 19:09:00 1989
Leo Hourvitz 25May87
Linda Gass: Thu Aug  6 09:27:57 1987
Joe Pasqua: Wed Feb 22 16:47:23 1989
Paul Rovner: Wednesday, October 7, 1987 6:12:56 PM
Jim Sandman: Thu Oct 26 14:41:57 1989
Perry Caro: Mon Nov  7 16:46:06 1988
Mark Francis: Wed Mar 28 11:19:48 1990
End Edit History.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include GC
#include LANGUAGE
#include PSLIB
#include STODEV
#include STREAM
#include VM

#define maxName 100
#define maxAcc 25

#include "streampriv.h"
#include "langdata.h"

#ifndef DPSONLY
#define DPSONLY 0
#endif /* DPSONLY */

/*	----- Begin data shared by all contexts -----	*/
/* The following items are not replicated per context. They are	*/
/* used for stream management across all contexts.		*/
private Stm closedStm;

/*	----- End data shared by all contexts -----	*/

public procedure HandlePendingEOL(stm)
  register Stm stm;
  {
  register integer c;
  stm->flags.f1 = false;
  if ((c = getc(stm)) != '\n') 
    if (c != EOF) ungetc(c, stm);
  }


public procedure MakePStm(stm, tag, pobj)
  register Stm stm; cardinal tag;  register PObject pobj;
  {
  register PStmBody sb;
  PVMRoot root;
  integer lvl;

  if (stm == NIL) stm = closedStm;
  if (CurrentShared())
    {root = rootShared; lvl = 0;}
  else
    {root = rootPrivate; lvl = level;}

  /* First look for duplicate Stm in either private or shared VM */
  for (sb = rootPrivate->stms; sb != NIL; sb = sb->link)
    if ((stm == sb->stm)&&stm->flags.reusable) goto found;
  for (sb = rootShared->stms; sb != NIL; sb = sb->link)
    if ((stm == sb->stm)&&stm->flags.reusable) goto found;

  /* None found. Look for suitable unused StmBody in current VM.
     Note that we re-use a StmBody only if we are at the save level
     at which the StmBody was originally created. This ensures that
     restore will close the stream at the correct time due to
     StmBody finalization.
   */
  for (sb = root->stms; sb != NIL; sb = sb->link)
    if (sb->stm == NIL && sb->level == lvl) break;

  if (sb == NIL)
    {
    AllocPStream(pobj);
    sb = pobj->val.stmval;
    sb->link = root->stms;
    root->stms = pobj->val.stmval;
    sb->level = lvl;
    }
  sb->stm = stm;

found:
  LStmObj(*pobj, sb->generation, sb);
  pobj->shared = sb->shared;
  pobj->level = sb->level;
  if (stm == closedStm) --pobj->length; /* closedStm always invalid */
  pobj->access = 0;
  if (stm->flags.read || stm->flags.readWrite) pobj->access |= rAccess|xAccess;
  if (stm->flags.write || stm->flags.readWrite) pobj->access |= wAccess;
  pobj->tag = tag;
  }  /* end of MakePStm */


typedef struct {char *name, *access; Stm *stm;} BIF;
private readonly BIF builtInFiles[] = {
  {"%stdin", "r", &os_stdin},
  {"%stdout", "w", &os_stdout},
  {"%stderr", "w", &os_stderr},
  NIL};

public Stm CreateFileStm(name, acc)
  string name; string acc;
{
  register Stm stm = NIL;
  register BIF *pbif;
  if (name[0] == '%')
    {
    for (pbif = builtInFiles; pbif->name != NIL; pbif++)
      if (os_strcmp((char *)name, pbif->name) == 0)
        {
        if (os_strcmp((char *)acc, pbif->access) != 0)
          PSError(invlflaccess);
        stm = *pbif->stm;
        break;
        }
    }
  if (stm==NIL) stm = StoDevCreateStm(name, acc);
  if (stm == NIL) UndefFileName();
  return(stm);
} /* end of CreateFileStm */

public procedure CreateFileStream(str, acc, psob)
  StrObj str; string acc; PStmObj psob;
{
  character name[maxName];
  register Stm stm = NIL;
  register BIF *pbif;
  if (str.length>=maxName) LimitCheck();
  StringText(str, name);
  stm = CreateFileStm(name, acc);
  DURING
    MakePStm(stm, Lobj, psob);
  HANDLER {
    (void) fclose(stm); RERAISE;}
  END_HANDLER;
}  /* end of CreateFileStream */


private boolean CloseStmForSB(sb)
  PStmBody sb;
/* Returns false normally, true if an ioerror occurred during close.
   The stream is nevertheless closed and removed from the StmBody. */
  {
  Stm stm = sb->stm;
  boolean errPreExisting = ferror(stm);
  sb->stm = NIL;
  sb->generation++;
  return (fclose(stm) == EOF) && !errPreExisting;
  }


public procedure CloseFile(sob, force)
  StmObj sob;
  boolean force;
  {
  register Stm stm;
  register PStmBody sb = sob.val.stmval;
  if (sob.length != sb->generation) return;
  if ((stm = sb->stm) == NIL) CantHappen();
  if (stm->flags.reusable && !force)
    {
    sb->generation++;  /* this makes the StmObj invalid */
    os_clearerr(stm);
    if (stm->flags.write && fputeof(stm) < 0) StreamError(stm);
    }
  else
    if (CloseStmForSB(sb)) StreamError(stm);
  }


public procedure CrFile(pstm)
  register PStmObj pstm;
{
  register PObject pobj;
  for (pobj = execStk->head - 1; pobj >= execStk->base; pobj--)
    if (pobj->type == stmObj)
      {*pstm = *pobj; return;}
  MakePStm(closedStm, Lobj, pstm);
}  /* end of CrFile */


/*ARGSUSED*/
public Stm InvlStm(sob)
  StmObj sob;
  /* Called from GetStream macro for an invalid stream object */
  {
  return closedStm;		/* any attempted operation will return EOF */
  }


public procedure StreamError(stm)
  Stm stm;
  {
  if (stm != closedStm) os_clearerr(stm);
  PSError(ioerror);
  }


/* String streams (input only; at present used only by EExec) */

private int SSFilBuf(stm) Stm stm; {stm->flags.eof = 1; return EOF;}

private int SSUnGetc(c, stm)
  int c; Stm stm;
  {
  if (stm->ptr > stm->base) {stm->ptr--; stm->cnt++; return c;}
  else return EOF;
  }

private int SSFlush(stm) Stm stm; {stm->ptr += stm->cnt; stm->cnt = 0;}

private int SSClose(stm) Stm stm; {StmDestroy(stm); return 0;}

private int SSFAvail(stm) Stm stm; {return stm->cnt;}

#if (OS == os_vms)
globaldef StmProcs strStmProcs = {
  SSFilBuf, StmErr, StmFRead, StmZeroLong, SSUnGetc, SSFlush,
  SSClose, SSFAvail, SSFlush, StmErr, StmErr, StmErrLong,
  "String"};
#else (OS == os_vms)
readonly StmProcs strStmProcs = {
  SSFilBuf, StmErr, StmFRead, StmZeroLong, SSUnGetc, SSFlush,
  SSClose, SSFAvail, SSFlush, StmErr, StmErr, StmErrLong,
  "String"};
#endif (OS == os_vms)

private Stm StrStmCreate(str)
  StrObj str;
  {
  Stm stm = StmCreate(&strStmProcs, (integer)0);
  if (stm != NIL) {
    stm->base = stm->ptr = (char *)str.val.strval;
    stm->cnt = str.length;
    stm->flags.read = 1;
    }
  return stm;
  }


/* Encrypted file handling */

#define C1 1069928045
#define C2 226908351
#define lenIV 4

#define Decrypt(r, clear, cipher)\
  clear = ((cipher)^(r>>8)) & 0xFF; r = ((cipher) + r)*C1 + C2

#define KEYHASH 0x9a3704d3

#if STAGE==DEVELOP
private procedure PSStSKey()	/* setstreamkey */
{
IntObj keyObj;
LIntObj(keyObj, PopInteger() ^ KEYHASH);
VMPutElem(rootShared->vm.Shared.param, (cardinal)rpStreamKey, keyObj);
}  /* end of PSStKey */
#endif STAGE==DEVELOP

/* Overlays for various fields of the standard StmRec that are not
   otherwise used by this stream class (note that the FilBuf procedures
   for the stream always just return one character and do not set up
   any buffer, so the stm->ptr and stm->base fields are unused) */
#define CStmSource(stm) *(StmObj *)&(stm)->data
#define CStmRndNum(stm) *(longcardinal *)&(stm)->base
#define CStmLeftover(stm) *(integer *)&(stm)->ptr

private int CStmBFilBuf(stm)
  register Stm stm;
  {
  register integer c, t;
  register Stm ssh;
  if (CStmLeftover(stm) >= 0) {
    t = CStmLeftover(stm); CStmLeftover(stm) = -1; return t;}
  ssh = GetStream(CStmSource(stm));
  c = getc(ssh);
  if (c < 0) {stm->flags.eof = 1; return EOF;}
  Decrypt(CStmRndNum(stm), t, c);
  return t;
  }

private int CStmHFilBuf(stm)
  register Stm stm;
  {
  register int c, t, count;
  register Stm ssh;
  if (CStmLeftover(stm) >= 0) {
    t = CStmLeftover(stm); CStmLeftover(stm) = -1; return t;}
  ssh = GetStream(CStmSource(stm));
  t = 0;
  count = 2;
  while (true) {
    c = getc(ssh);
    if (c < 0) {stm->flags.eof = 1; return EOF;}
    else if ((c = hexToBinary[c]) == NOTHEX) continue;
    t = (t << 4) + c;
    if (--count == 0) break;
    }
  Decrypt(CStmRndNum(stm), c, t);
  return c;
  }

private int CStmUnGetc(ch, stm)
  Stm stm;
  {
  if (ch < 0 || CStmLeftover(stm) >= 0) return EOF;
  else CStmLeftover(stm) = ch;
  return ch;
  }

private int CStmFlush(stm)
  Stm stm;
  {
  Stm ssh;
  integer res;
  ssh = GetStream(CStmSource(stm));
  res = fflush(ssh);
  CStmLeftover(stm) = -1;
  return res;
  }

private int CStmClose(stm)
  Stm stm;
  {
  StmObj sStmObj; Stm ssh;
  sStmObj = CStmSource(stm);
  StmDestroy(stm);
  /* do following after StmDestroy since it can raise ioerror */
  ssh = GetStream(sStmObj);
  if (feof(ssh)) CloseFile(sStmObj, false);
  if ((dictStk->head-1)->val.dictval == rootShared->vm.Shared.sysDict.val.dictval)
    End(); /* pop systemdict, which was pushed by eexec */
  return 0;
  }

private int CStmFAvail(stm)
  Stm stm;
  {
  Stm ssh;
  if (stm->flags.eof) return EOF;
  ssh = GetStream(CStmSource(stm));
  return favail(ssh);
  }

private int CStmFSeek(stm, offset, base)
  Stm stm;
  long int offset;
  int base;
  {
  Stm ssh;
  ssh = GetStream(CStmSource(stm));
  return fseek(ssh, offset, base);
  }

private long int CStmFTell(stm)
  Stm stm;
  {
  Stm ssh;
  ssh = GetStream(CStmSource(stm));
  return ftell(ssh);
  }

private readonly StmProcs cStmBinProcs = {
  CStmBFilBuf, StmErr, StmFRead, StmZeroLong, CStmUnGetc, CStmFlush,
  CStmClose, CStmFAvail, StmErr, StmErr, CStmFSeek, CStmFTell,
  "CryptBin"};

private readonly StmProcs cStmHexProcs = {
  CStmHFilBuf, StmErr, StmFRead, StmZeroLong, CStmUnGetc, CStmFlush,
  CStmClose, CStmFAvail, StmErr, StmErr, CStmFSeek, CStmFTell,
  "CryptHex"};

public Stm MakeDecryptionStm(source, kind)
  StmObj source;	/* underlying stream */
  DecryptionType kind;	/* kind of stream (hex or binary) */
  {
  register Stm ssh, rsh;
  StmProcs *procs;

   switch (kind) {
    case hexStream: procs = &cStmHexProcs; break;
    case binStream: procs = &cStmBinProcs; break;
    default: CantHappen();
    }
 ssh = GetStream(source);
  rsh = StmCreate(procs, (integer)0);
  rsh->flags.read = true;
  rsh->flags.positionable = ssh->flags.positionable;
  CStmSource(rsh) = source;
  CStmLeftover(rsh) = -1;
  return(rsh);
  }

public procedure SetStmDecryptionKey(stm, key)
  Stm stm;
  longcardinal key;
  {
  CStmRndNum(stm) = key;
  }

public longcardinal GetStmDecryptionKey(stm)
  Stm stm;
  {
  return(CStmRndNum(stm));
  }

public boolean EncryptedStream(so)
  StmObj so;
  {
  Stm sh = GetStream(so);
  return ((sh->procs == &cStmBinProcs) || (sh->procs == &cStmHexProcs));
  }

public procedure PSEExec()
  {
  Object source, result;
  IntObj keyObj;
  register Stm ssh, rsh;
  register integer c, i, leftover; integer trash;
  longcardinal hexRndnum; boolean isHex;
  boolean origShared;
  PopP(&source);
  ConditionalInvalidateRecycler (&source);
  switch (source.type){
    case stmObj:{
      if ((source.access & (rAccess|xAccess)) == 0) InvlAccess();
      ssh = GetStream(source);
      if (ssh->flags.write || ssh->flags.readWrite) PSError(invlflaccess);
      if (ssh->flags.f1) HandlePendingEOL(ssh);
      break;}
    case strObj:{
      if ((source.access & (rAccess|xAccess)) == 0) InvlAccess();
      if ((ssh = StrStmCreate(source)) == NIL) LimitCheck();
      DURING {MakePStm(ssh, Lobj, &source);}
      HANDLER {fclose(ssh); RERAISE;} END_HANDLER;
      break;}
    default: TypeCheck();
    }
  rsh = MakeDecryptionStm(source, binStream);
  origShared = CurrentShared();
  SetShared(source.shared);
  DURING {MakePStm(rsh, Xobj, &result);}
  HANDLER {fclose(rsh); SetShared(origShared); RERAISE;} END_HANDLER;
  SetShared(origShared);
  result.access = xAccess;
  keyObj = VMGetElem(rootShared->vm.Shared.param, rpStreamKey);
  CStmRndNum(rsh) = hexRndnum = keyObj.val.ival ^ KEYHASH;
  isHex = true; leftover = -1;
  do {c = getc(ssh);} while (c==' ' || c=='\t' || c=='\n' || c=='\r');
  if (c != EOF) ungetc(c, ssh);
  for (i = 0; i < lenIV || (isHex && i < 2*lenIV); i++) {
    c = getc(ssh);
    Decrypt(CStmRndNum(rsh), trash, c);
    if ((c = hexToBinary[c]) == NOTHEX) isHex = false;
    if (leftover < 0) leftover = c;
    else {
      c += leftover<<4;
      leftover = -1;
      Decrypt(hexRndnum, trash, c);
      }
    }
  if (isHex) {rsh->procs = &cStmHexProcs; CStmRndNum(rsh) = hexRndnum;}
  Begin(rootShared->vm.Shared.sysDict);
  EPushP(&result);
  }  /* end of PSEExec */


/* Intrinsics. */

public procedure PSPrint()
{
  StrObj  so;

  PopPRString (&so);
  RecyclerPush (&so);		/* In case of yield in fwrite */
  DURING {
    fwrite (so.val.strval, 1, so.length, os_stdout);
  } HANDLER {
    RecyclerPop (&so);
    RERAISE;
  } END_HANDLER;
  RecyclerPop (&so);
}				/* end of PSPrint */

public procedure PSFile()
{
StrObj acc, str; StmObj stm; character type[maxAcc];
PopPRString(&acc);
PopPRString(&str);
if (acc.length > maxAcc) PSError(invlflaccess);
StringText(acc,type);
CreateFileStream(str, type, &stm);
PushP(&stm);
}  /* end of PSFile */

public boolean IsCrFile(stm)  StmObj stm;
{
StmObj cstm;
CrFile(&cstm);
return (stm.length == cstm.length && stm.val.stmval == cstm.val.stmval);
}  /* end of IsCrFile */

public procedure PSRead()
{
integer c; boolean retval = true;
StmObj stm; Stm s;
PopPStream(&stm);
if ((stm.access & rAccess) == 0 && !IsCrFile(stm)) InvlAccess();
s = GetStream(stm);
c = getc(s);
if (s->flags.f1) {		/* perform HandlePendingEOL in-line */
  s->flags.f1 = false; if (c == '\n') c = getc(s);}
if (c == EOF)
  {
  if (ferror(s)) StreamError(s);
  CloseFile(stm, false); retval = false;
  }
else PushInteger(c);
PushBoolean(retval);
}  /* end of PSRead */

public procedure PSWrite()
{
integer c; Stm sh; StmObj stm;
c = PopInteger();
PopPStream(&stm);
if ((stm.access & wAccess) == 0) InvlAccess();
sh = GetStream(stm);
if (putc(c, sh) < 0) StreamError(sh);
}  /* end of PSWrite */


/* Fast stream access macros for readline and readhexstring */

#define SETUPSTM stmPtr = stm->ptr; stmCnt = stm->cnt
#define UPDATESTM stm->ptr = stmPtr; stm->cnt = stmCnt

#define GETC \
  if (--stmCnt >= 0) c = (unsigned char)*stmPtr++; \
  else { \
    UPDATESTM; \
    if ((c = (*stm->procs->FilBuf)(stm)) < 0) goto gotEOF; \
    SETUPSTM; \
    }

public procedure PSReadLine()
{
  StrObj  str;
  StmObj  sob;
  register Stm stm;
  register integer count;
  register integer c;
  register charptr s;
  register integer stmCnt;
  register char *stmPtr;
  boolean retval = false;

  PopPString (&str);
  PopPStream (&sob);
  if ((sob.access & rAccess) == 0 && !IsCrFile (sob) ||
      (str.access & wAccess) == 0)
    InvlAccess ();
  if ((count = str.length) == 0)
    RangeCheck ();
  s = str.val.strval;
  stm = GetStream (sob);
  if (stm->flags.f1)
    HandlePendingEOL (stm);
  RecyclerPush (&str);
  DURING {
    SETUPSTM;
    while (true) {
      GETC;
      switch (c) {
#if	('\n' == '\r')
       case '\012':
#else	('\n' == '\r')
       case '\r':
#endif	('\n' == '\r')
	if (stmCnt > 0) {
	  if (*stmPtr == '\n') {
	    stmPtr++;
	    stmCnt--;
	  }
	} else
	  stm->flags.f1 = true;	/* note we may have a pending EOL */
	/* fall through */
       case '\n':
	UPDATESTM;
	retval = true;
	goto done;
       default:
	if (--count < 0) {
	  UPDATESTM;
	  ungetc (c, stm);
	  RangeCheck ();
	}
	*s++ = c;
      }
    }

gotEOF:
    if (ferror (stm)) {
      StreamError (stm);
      /* NOT REACHED */
    }
    CloseFile (sob, false);

done:
    RecyclerPop (&str);
    str.length -= count;
    PushP (&str);
    PushBoolean (retval);
  } HANDLER {
    RecyclerPop (&str);
    RERAISE;
  } END_HANDLER;
}				/* end of PSReadLine */

public procedure PSReadString()
{
  StrObj  str;
  Stm     sh;
  StmObj  sob;
  cardinal i = 0;
  boolean retval;

  PopPString (&str);
  PopPStream (&sob);
  if (((sob.access & rAccess) == 0 && !IsCrFile (sob)) ||
      (str.access & wAccess) == 0)
    InvlAccess ();
  if (str.length == 0)
    RangeCheck ();
  sh = GetStream (sob);
  if (sh->flags.f1)
    HandlePendingEOL (sh);
  RecyclerPush (&str);		/* In case of yield inside fread */
  DURING {
    i = fread (str.val.strval, 1, str.length, sh);
  } HANDLER {
    RecyclerPop (&str);
    RERAISE;
  } END_HANDLER;
  RecyclerPop (&str);
  if (!(retval = (i == str.length))) {
    if (ferror (sh))
      StreamError (sh);
    CloseFile (sob, false);
  }
  str.length = i;
  PushP (&str);
  PushBoolean (retval);
}				/* end of PSReadString */

public char *hexToBinary;

private void InitHexToBin(hexToBinary)
/* Only valid for ascii...		*/
char *hexToBinary;
{
  register cardinal i;

  for (i = 0; i < 256; i++) *(hexToBinary + i) = NOTHEX;
  for (i = 'a'; i < 'g'; i++) *(hexToBinary + i) = i - 'a' + 10;
  for (i = 'A'; i < 'G'; i++) *(hexToBinary + i) = i - 'A' + 10;
  for (i = '0'; i <= '9'; i++) *(hexToBinary + i) = i - '0';
}

public procedure PSReadHexString()
{
  StrObj  str;
  StmObj  sob;
  register Stm stm;
  register integer count;
  register integer c,
          c1;
  register charptr s;
  register integer stmCnt;
  register char *stmPtr;

  PopPString (&str);
  PopPStream (&sob);
  if ((sob.access & rAccess) == 0 && !IsCrFile (sob) ||
      (str.access & wAccess) == 0)
    InvlAccess ();
  if ((count = str.length) == 0)
    RangeCheck ();
  s = str.val.strval;
  stm = GetStream (sob);
  stm->flags.f1 = false;	/* do not care about pending EOL */
  RecyclerPush (&str);
  DURING {
    char *h2b = hexToBinary;
    SETUPSTM;
    do {
	c = 256;
	while (!((c | --count | (stmCnt -= 2)) & 0x80000000)) {
	    c = h2b[(unsigned char)*stmPtr++];
	    c <<= 4;
	    c |= h2b[(unsigned char)*stmPtr++];
	    *s++ = (unsigned char) c;
	}
	count++; stmCnt += 2;
	if (c != 256) { stmCnt += 2; stmPtr -= 2; count++; s--; }
	do { GETC; } while ((c1 = h2b[c]) == NOTHEX);
	do { GETC; } while ((c  = h2b[c]) == NOTHEX);
	*s++ = (c1 << 4) + c;
    } while (--count != 0);
    UPDATESTM;
    goto done;

gotEOF:
    if (ferror (stm)) {
      StreamError (stm);
      /* NOT REACHED */
    }
    CloseFile (sob, false);

done:
    RecyclerPop (&str);
    str.length -= count;
    PushP (&str);
    PushBoolean ((boolean) (count == 0));
  } HANDLER {
    RecyclerPop (&str);
    RERAISE;
  } END_HANDLER;
}				/* end of PSReadHexString */

public procedure PSWrtString()
{
  StrObj  str;
  Stm     sh;
  StmObj  stm;
  integer writeResult;

  PopPRString (&str);
  PopPStream (&stm);
  if ((stm.access & wAccess) == 0)
    InvlAccess ();
  if ((str.access & rAccess) == 0)
    InvlAccess ();
  sh = GetStream (stm);
  RecyclerPush (&str);
  DURING {
    writeResult = fwrite (str.val.strval, 1, str.length, sh);
  } HANDLER {
    RecyclerPop (&str);
    RERAISE;
  } END_HANDLER;
  RecyclerPop (&str);
  if (writeResult != str.length)
    StreamError (sh);
}				/* end of PSWWrtString */

private readonly char binaryToHex[] = "0123456789abcdef";

public procedure PSWrtHexString()
{
  StrObj  str;
  Stm     sh;
  StmObj  stm;
  cardinal i;

  PopPRString (&str);
  PopPStream (&stm);
  if ((stm.access & wAccess) == 0)
    InvlAccess ();
  if ((str.access & rAccess) == 0)
    InvlAccess ();
  sh = GetStream (stm);
  RecyclerPush (&str);		/* In case of yield in putc */
  DURING {
    for (i = 0; i < str.length;) {
      register integer c = VMGetChar (str, i++);

      putc (binaryToHex[c >> 4], sh);
      putc (binaryToHex[c & 0xF], sh);
    }
  } HANDLER {
    RecyclerPop (&str);
    RERAISE;
  } END_HANDLER;
  RecyclerPop (&str);
  if (ferror (sh) || feof (sh))
    StreamError (sh);
}				/* end of PSWrtHexString */

public procedure PSCloseFile()
{
StmObj stm;
PopPStream(&stm);
CloseFile(stm, false);
}  /* end of PSCloseFile */

public procedure PSStatus()
  {
  Object ob;

  PopP(&ob);
  switch(ob.type) {
    case stmObj:
      PushBoolean((boolean)
        (ob.length == ob.val.stmval->generation));
      break;
    case strObj:
      StoDevStrStatus(ob);
      break;
    default: TypeCheck();
    }
  }  /* end of PSStatus */

public procedure PSFlsFile()
{
StmObj sob; Stm sh;
PopPStream(&sob);
sh = GetStream(sob);
sh->flags.f1 = false;		/* don't care about pending EOL */
if (fflush(sh) < 0) StreamError(sh);
}  /* end of PSFlsFile */

public procedure PSFls()
{
  if (fflush(os_stdout) < 0) StreamError(os_stdout);
}

public procedure PSResFile()
{
StmObj sob; Stm sh;
PopPStream(&sob);
sh = GetStream(sob);
sh->flags.f1 = false;		/* don't care about pending EOL */
freset(sh);
}  /* end of PSResFile */

public procedure PSCrFile()
{
StmObj stm;
CrFile(&stm);
stm.tag = Lobj;
PushP(&stm);
}  /* end of PSCrFile */

public procedure PSBytesAvailable()
{
StmObj stm; Stm sh; integer c;
PopPStream(&stm);
if ((stm.access & rAccess) == 0) InvlAccess();
sh = GetStream(stm);
c = favail(sh);
if (c != 0 && sh->flags.f1) {	/* handle pending EOL if necessary */
  sh->flags.f1 = false;
  if ((c = getc(sh)) != '\n') ungetc(c, sh);
  c = favail(sh);}
PushInteger(c);
}  /* end of PSBytesAvailable */


/* Initialization and finalization */

public procedure StmCtxCreate()
{
  MakePStm(os_stdin, Lobj, &stdinStm);
  MakePStm(os_stdout, Lobj, &stdoutStm);
}

public procedure StmCtxDestroy()
{
  extern StmProcs closedStmProcs;

  /* Don't try to close a closed stream, since there is no error handler in place.
   * Since our fork gives the new process a closed stdin stm, we must avoid closing
   * it.
   */
  if (stdinStm.val.stmval->stm->procs != &closedStmProcs)
    CloseFile(stdinStm, true);
  if (stdoutStm.val.stmval->stm->procs != &closedStmProcs)
    CloseFile(stdoutStm, true);
}


private procedure StmFinalize(obj, reason)
  Object obj; FinalizeReason reason;
/* Called when StmObj is reclaimed by restore or garbage collection */
{
  register PStmBody sb = obj.val.stmval;
  register PStmBody *plink;

  if (sb->stm != NIL) (void) CloseStmForSB(sb);
  plink = (sb->shared)? &rootShared->stms : &rootPrivate->stms;
  for ( ; *plink != NIL; plink = &(*plink)->link)
    if (*plink == sb)
      {*plink = sb->link; return;}
  CantHappen(); /* StmBody not found in chain */
}

/*	----> GC Support Routines <----		*/
/* The following procs are required for correct	*/
/* operation of the Garbage Collector.		*/

private procedure EnumerateStmBody(obj, info)
  PObject obj; GC_Info info;
/* This procedure is called during GC trace for each live StmObj in order
   to push roots for any composite objects contained in the StmBody */
{
#if (DPSONLY == 0)
  PStmBody sb = obj->val.stmval;
  if ((sb->stm != NIL)             /* stream not closed */
     && ( sb->stm->procs->type[0] == '!'))  /* stream is a filter stream */
    HandleFilterStream(sb, info);     /* push all ref. obj. on gc stack */
#endif /* DPSONLY */
}

public procedure PrivateStreamRoots(info) GC_Info info; {
  /* This proc pushes all roots of the stream pkg	*/
  GC_Push(info, &stdinStm);
  GC_Push(info, &stdoutStm);
  }

/*	-----------------------------------	*/
/*	----> END GC Support Routines <----	*/
/*	-----------------------------------	*/

private readonly StmProcs invlStmProcs = {
  StmErr, StmErr, StmZeroLong, StmZeroLong, StmErr, StmErr,
  StmErr, StmErr, StmErr, StmErr, StmErr, StmErrLong,
  "Invalid"};


public procedure StreamInit(reason)  InitReason reason;
{
  Object  ob;

  switch (reason) {
   case init:
    {
      hexToBinary = (char *) os_sureMalloc((long int)(256 * sizeof(char)));
      InitHexToBin (hexToBinary);
      closedStm = (Stm) os_sureCalloc ((long int)1, (long int)sizeof(StmRec));
      closedStm->procs = &invlStmProcs;
      closedStm->flags.eof = true;
      VMRgstFinalize(stmObj, StmFinalize, frset_reclaim);
      GC_RgstStmEnumerator(EnumerateStmBody);
#if STAGE==DEVELOP
      Assert (hexToBinary['0'] == 0 && hexToBinary['A'] == 0xA &&
	      hexToBinary['a'] == 0xA);
#endif STAGE==DEVELOP
      /* See also LanguageDataHandler in exec.c for per context data */
      break;
    }
   case romreg:
#if STAGE==DEVELOP
    if (vSTAGE == DEVELOP)
      RgstExplicit ("setstreamkey", PSStSKey);
#endif STAGE==DEVELOP
#if VMINIT
    ob = VMGetElem (rootShared->vm.Shared.param, rpStreamKey);
    if (ob.type != intObj) {
      LIntObj (ob, NumCArg ('K', (integer) 10, (integer) 7654321) ^ KEYHASH);
      VMPutElem (rootShared->vm.Shared.param, (cardinal)rpStreamKey, ob);
    }
#endif VMINIT
    break;
    endswitch
  }
}				/* end of StreamInit */
