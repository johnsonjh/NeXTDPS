/*
  debug.c

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
Chuck Geschke: Mon Nov 25 14:50:06 1985
Doug Brotz: Mon Jun  2 23:04:59 1986
Ed Taft: Fri Dec  1 11:55:39 1989
Ivor Durham: Wed Aug 17 18:54:06 1988
Jim Sandman: Mon Apr 10 10:36:54 1989
Joe Pasqua: Tue Jan 17 13:53:50 1989
End Edit History.
*/


#include PACKAGE_SPECS
#include BASICTYPES
#include EXCEPT
#include GRAPHICS
#include LANGUAGE
#include VM

#include "fontcache.h"
#include "fontsnames.h"

/* The following procedures are extern'd here since	*/
/* they're not exported by the language package.	*/
extern procedure DecodeObj();
extern procedure NameToPString();

private boolean calledFromPS;

private boolean Aborted() {return calledFromPS && GetAbort() != 0;}

public Stm debugStm;
/* If this stream is non-NIL, these procedures send output to debugStm
   when called directly from the debugger; otherwise they send output
   to os_stderr. In any event, when called from the PostScript
   "printobj" and "printvalue" operators, they send output to os_stdout. */

private Stm GetDebugStm()
{
  return (calledFromPS)? os_stdout :
    (debugStm != NIL)? debugStm :
    os_stderr;
}


private PrintAccess(a)
	Access a;
{
  Stm stm = GetDebugStm();
  char s[4]; integer i = 0;
  if ((a & rAccess) != 0) s[i++] = 'R';
  if ((a & wAccess) != 0) s[i++] = 'W';
  if ((a & xAccess) != 0) s[i++] = 'X';
  s[i] = 0;
  os_fprintf(stm, ", acc=%s", (i == 0? "NONE": s));
}

private DPrintSOP(sop)
	PStrObj sop;
{
  Stm stm = GetDebugStm();
  cardinal i;
  for (i=0; i < sop->length && !Aborted(); i++) putc(VMGetChar(*sop,i), stm);
  fflush(stm);
}

public PrintSOP(sop)
	PStrObj sop;
/* This is called from several debugging operators in other packages
   and therefore should always write on os_stdout */
{
  Stm stm = GetDebugStm();
  cardinal i;
  for (i=0; i < sop->length && !Aborted(); i++) putchar(VMGetChar(*sop,i));
  fflush(stm);
}

private PrintNameString(pn)
  PNameObj pn;
{
  StrObj so;
  NameToPString(*pn, &so);
  DPrintSOP(&so);
}

public PrintBlanks(i)
	integer i;
{
  Stm stm = GetDebugStm();
  while (i-- > 0) putc(' ', stm);
}

private pcom(indent,s,x)
	integer indent; char *s; PObject x;
{
  Stm stm = GetDebugStm();
  PrintBlanks(indent);
  os_fprintf(stm, "%c %-6s, level=%2d, shared=%d, seen=%d",
    (x->tag==Lobj? 'L':'X'), s, x->level, x->shared, x->seen);
}

public PrintObj(indent,x)
	integer indent; PObject x;
{
  Stm stm = GetDebugStm();
  switch (x->type){
    case nullObj:{
	pcom(indent,"null",x);
	putc('\n', stm);
	return;}
    case intObj:{
	pcom(indent,"int",x);
	os_fprintf(stm, ", val=%d\n",
		x->val.ival);
	return;}
    case realObj:{
	pcom(indent,"real",x);
	os_fprintf(stm, ", val=%e\n",
		x->val.rval);
	return;}
    case nameObj:{
	pcom(indent,"name",x);
	os_fprintf(stm, ", val=0x%x, \"",
		x->val.nmval);
	PrintNameString(x); os_fprintf(stm, "\"\n");
	return;}
    case boolObj:{
	pcom(indent,"bool",x);
	os_fprintf(stm, ", val=");
	switch (x->val.bval) {
	  case false: os_fprintf(stm, "false\n"); break;
	  case true: os_fprintf(stm, "true\n"); break;
	  default: os_fprintf(stm, "??[%d]\\n", x->val.bval);
	  }
	return;}
    case strObj:{
	pcom(indent,"str",x);
	os_fprintf(stm, ", length=%d, val=0x%x",
		x->length, x->val.strval);
	PrintAccess(x->access);
	putc('\n', stm);
	return;}
    case stmObj:{
	pcom(indent,"stm",x);
	PrintAccess(x->access);
	os_fprintf(stm, ", length=%d, val=0x%x\n",
		x->length,x->val.stmval);
	return;}
    case cmdObj:{NameObj no;
	pcom(indent,"cmd",x);
	os_fprintf(stm, ", mrk=%d, idx=%d, \"",
		x->access,x->length);
	LNameObj(no,x->val.cmdval);
	PrintNameString(&no); os_fprintf(stm, "\"\n");
	return;}
    case dictObj:{
	pcom(indent,"dict",x);
	os_fprintf(stm, ", val=0x%x\n",
		x->val.dictval);
	return;}
    case arrayObj:{
	pcom(indent,"array",x);
	PrintAccess(x->access);
	os_fprintf(stm, ", length=%d, val=0x%x\n",
		x->length,x->val.arrayval);
	return;}
    case pkdaryObj:{
	pcom(indent,"pkdary",x);
	PrintAccess(x->access);
	os_fprintf(stm, ", length=%d, val=0x%x\n",
		x->length,x->val.arrayval);
	return;}
    case fontObj:{
	pcom(indent,"font",x);
	os_fprintf(stm, ", fid=0x%X\n",
		x->val.fontval);
	return;}
    case escObj:
      switch (x->length){
	case objMark:{
	    pcom(indent,"mark",x);
	    putc('\n', stm);
	    return;}
	case objSave:{
	    pcom(indent,"save",x);
	    os_fprintf(stm, ", savedlevel=%d\n",
		    x->val.saveval);
	    return;}
      }
  }
}

private integer PrAryBody();
private procedure PrPkdaryBody();
private procedure PrStmBody();

public PrintVal(indent,x)
	integer indent; PObject x;
{
  Stm stm = GetDebugStm();
  PrintObj(indent, x);
  indent += 2;
  switch (x->type){
    case nameObj:
      PrintNameEntry(indent, x);
      return;
    case strObj:
      PrintBlanks(indent);
      os_fprintf(stm, "text = "); DPrintSOP(x); putc('\n', stm);
      return;
    case dictObj:
      DumpDict(x);
      return;
    case arrayObj:
      PrAryBody(indent, x, 0, x->length-1, true);
      return;
    case pkdaryObj:
      PrPkdaryBody(indent, x);
      return;
    case stmObj:
      PrStmBody(indent, x);
      return;
  }
}

public PrintNameEntry(indent, x)
  integer indent; PNameObj x;
  {
  Stm stm = GetDebugStm();
  PNameEntry pne = x->val.nmval;
  PrintBlanks(indent);
  os_fprintf(stm, "nameindex=%d, ncilink=%d, link=0x%x, vec=0x%x, seen=%d\n",
    pne->nameindex, pne->ncilink, pne->link, pne->vec, pne->seen);
  if (pne->dict == 0) {
    PrintBlanks(indent);
    os_fprintf(stm, "cache empty\n");
    }
  else {
    PrintBlanks(indent);
    os_fprintf(stm, "kvloc=0x%x, dict=0x%x, ts=%d\n", pne->kvloc, pne->dict, pne->ts);
    PrintKeyVal(indent, pne->kvloc);
    }
  fflush(stm);
  }

public PrintDOB(indent,x)
  integer indent;  PDictObj x;
{ DictBody db;
  Stm stm = GetDebugStm();
  x = XlatDictRef(x);
  VMGetDict(&db,*x);
  PrintBlanks(indent);
  os_fprintf(stm, "curlength=%d, maxlength=%d, size=%d",
    db.curlength,db.maxlength,db.size);
  PrintAccess(db.access);
  putc('\n', stm);
  PrintBlanks(indent);
  os_fprintf(stm, "bitvector=0x%x, begin=0x%x, end=0x%x\n",
    db.bitvector, db.begin, db.end);
  PrintBlanks(indent);
  os_fprintf(stm, "level=%d, shared=%d, seen=%d, isfont=%d, tricky=%d\n",
    db.level, db.shared, db.seen, db.isfont, db.tricky);
  fflush(stm);
}

public boolean PrintNode(x, data)
  PObject x;
  charptr data;	/* Unused */
{
  PrintObj((integer)0, x);
  return (true);
}

public DumpStack(x)  PStack x;
{
  EnumStack (x, PrintNode, (charptr)NIL);
}

public DumpDefault()
{
  Stm stm = GetDebugStm();
  os_fprintf(stm, "Operand stack:\n");
  DumpStack(opStk);
  os_fprintf(stm, "Execution stack:\n");
  DumpStack(execStk);
  os_fprintf(stm, "Dictionary stack:\n");
  DumpStack(dictStk);
}

public PrintKeyVal(indent, x)  integer indent; PKeyVal x;
{
  Stm stm = GetDebugStm();
KeyVal keyval; VMGetKeyVal(&keyval,x);
PrintBlanks(indent); os_fprintf(stm, "key: ");
PrintObj((integer)0, &(keyval.key));
PrintBlanks(indent); os_fprintf(stm, "value: ");
PrintObj((integer)0, &(keyval.value));
fflush(stm);
}

public DumpSysDict()  {DumpDict(&(rootShared->vm.Shared.sysDict));}

public DumpDict(dict)  PDictObj dict;
{
  Stm stm = GetDebugStm();
PKeyVal kvloc;
DictBody db;
dict = XlatDictRef(dict);
VMGetDict(&db, *dict);
PrintDOB((integer)2, dict);
for (kvloc=db.begin; kvloc<db.end && !Aborted(); kvloc++)
  {
  if ((kvloc->key.type != nullObj) || (kvloc->value.type != nullObj))
    {
    os_fprintf(stm, "%d:", (kvloc-db.begin));
    PrintKeyVal((integer)4, kvloc);
    }
  }
}

private integer PrAryBody(indent,aop,start,end,skipnull)
  integer indent; PAryObj aop; cardinal start, end; boolean skipnull;
{
  Stm stm = GetDebugStm();
cardinal i,c=0; Object ob;
for (i=start; i <=end && !Aborted(); i++){
 ob=VMGetElem(*aop,i);
 if (!(skipnull && ob.type == nullObj)) {
   PrintBlanks(indent);
   c++;  os_fprintf(stm, "%2d: ",i);  PrintObj(indent, &ob);}}
fflush(stm);
return c;
}

public DumpArray(aop, start, end)
  PAryObj aop; cardinal start, end;
  {
  PrintObj((integer)0, aop);
  PrAryBody((integer)2, aop, start, end, false);
  }

private procedure PrPkdaryBody(indent, x)
  integer indent; PObject x;
  {
  Stm stm = GetDebugStm();
  charptr p, np; Object pa, ob; integer i;
  pa = *x;
  while (pa.length != 0 && !Aborted()) {
    p = pa.val.pkdaryval;
    DecodeObj(&pa, &ob);
    np = pa.val.pkdaryval;
    PrintBlanks(indent);
    for (i = 0; i < 9; i++)
      if (p < np) os_fprintf(stm, "%02x", *p++); else os_fprintf(stm, "  ");
    PrintObj((integer)2, &ob);
    }
   fflush(stm);
  }

private procedure PrStmBody(indent, x)
  integer indent; PObject x;
  {
  Stm stm = GetDebugStm();
  PStmBody pb = x->val.stmval;
  PrintBlanks(indent);
  os_fprintf(stm, "link=0x%x, stm=0x%x, generation=%d, level=%d, shared=%d, seen=%d\n",
    pb->link, pb->stm, pb->generation, pb->level, pb->shared, pb->seen);
  fflush(stm);
  }

private integer PrNameAryBody(indent,aop,start,end,skipnull)
  integer indent; PAryObj aop; cardinal start, end; boolean skipnull;
{
  Stm stm = GetDebugStm();
  cardinal i,c=0; Object ob;
  for (i=start; i <=end && !Aborted(); i++){
    LNameObj(ob, rootShared->vm.Shared.nameTable.val.namearrayval->nmEntry[i]);
    if (! (skipnull && ob.val.nmval == NIL)){
      PrintBlanks(indent);
      c++;  os_fprintf(stm, "%2d: val=0x%x, \"", i, ob.val.nmval);
      PrintNameString(&ob); os_fprintf(stm, "\"\n");}}
  fflush(stm);
  return c;
}

public DumpNameArray(aop, start, end)
  PAryObj aop; cardinal start, end;
  {
  PrintObj((integer)0, aop);
  PrNameAryBody((integer)2, aop, start, end, false);
  }

public DumpRoot()
{
  Stm stm = GetDebugStm();
  integer c;
  os_fprintf(stm, "stamp: %d\n",rootShared->vm.Shared.stamp);
  os_fprintf(stm, "sysDict:"); DumpSysDict();
  os_fprintf(stm, "commandCount: %d\n",rootShared->vm.Shared.cmdCnt);
  os_fprintf(stm, "nameTable: ");
  PrintObj((integer)0, &(rootShared->vm.Shared.nameTable));
  c = PrNameAryBody((integer)2, &(rootShared->vm.Shared.nameTable),
    0, rootShared->vm.Shared.nameTable.val.namearrayval->length-1, true);
  os_fprintf(stm, "nameTable: used = %d, total = %d\n",
    c, rootShared->vm.Shared.nameTable.val.namearrayval->length);
  fflush(stm);
}

private procedure PSPrObject()
{
Object ob;
PopP(&ob);
calledFromPS = true;
PrintObj((integer)0, &ob);
calledFromPS = false;
fflush (os_stdout);
}  /* end of PSPrObject */

private procedure PSPrValue()
{
Object ob;
PopP(&ob);
calledFromPS = true;
PrintVal((integer)0, &ob);
calledFromPS = false;
fflush (os_stdout);
}  /* end of PSPrValue */

private procedure pFD()
{
  Stm stm = GetDebugStm();
PDictBody FDdp;
register PKeyVal FDkvp; Object obj;
DictObj FD;
os_fprintf(stm, "FontDirectory:\n");
GetFontDirectory (&FD);
FDdp = FD.val.dictval;
for (FDkvp = FDdp->begin; FDkvp != FDdp->end && ! Aborted(); FDkvp++)
  {
  if ((FDkvp->key.type != nullObj) && (FDkvp->value.type == dictObj))
    {
    os_fprintf(stm, "  key: "); PrintNameString(&FDkvp->key);
    if (Known(FDkvp->value, fontsNames[nm_FontName]))
      {
      DictGetP(FDkvp->value, fontsNames[nm_FontName], &obj);
      os_fprintf(stm, ", name: "); PrintNameString(&obj);
      }
    DictGetP(FDkvp->value, fontsNames[nm_FID], &obj);
    os_fprintf(stm, ", fid: 0x%X, dict: 0x%x\n",
      obj.val.fontval,FDkvp->value.val.dictval);
    PrintObj((integer)4, &FDkvp->value);
    }
  }
os_fprintf(stm, "\n");
fflush(stm);
}  /* end of pFD */

private PrintBoolean(b)  boolean b;
{
  Stm stm = GetDebugStm();
  os_fprintf(stm, "%s", (b) ? "true" : "false");
  fflush(stm);
}

private PrintDevCoord(pc)  DevCd *pc;
{
  Stm stm = GetDebugStm();
double x,y;
x = fixtodbl(pc->x);
y = fixtodbl(pc->y);
os_fprintf(stm, "(%g,%g)", x, y);
fflush(stm);
}  /* end of PrintDexCoord */

private PrintMTX(pm)
	PMtx pm;
{
  Stm stm = GetDebugStm();
  os_fprintf(stm, "[%g,%g,%g,%g,%g,%g]",pm->a,pm->b,pm->c,pm->d,pm->tx,pm->ty);
  fflush(stm);
}

private PrintMidDict(d)  DictObj d;
{
  Stm stm = GetDebugStm();
  Object ob; Mtx m;
  boolean first = true;

  if (d.type == nullObj) {os_fprintf(stm, "NOLL\n"); return;};
  pcom((integer)0, "dict", &d);
  DictGetP(d, fontsNames[nm_FID], &ob);
  os_fprintf(stm, ", val=0x%x, FID=0x%X\n", d.val.dictval, ob.val.fontval);
  if (Known(d, fontsNames[nm_FontName]))
    {
    if (first) {os_fprintf(stm, "    "); first = false;}
    os_fprintf(stm, "FontName=");
    DictGetP(d, fontsNames[nm_FontName], &ob); PrintNameString(&ob);
    }
  if (Known(d, fontsNames[nm_ScaleMatrix]))
    {
    os_fprintf(stm, first? "    " : ", "); first = false;
    DictGetP(d, fontsNames[nm_ScaleMatrix], &ob);
    PAryToMtx(&ob, &m);
    os_fprintf(stm, "ScaleMatrix=");  PrintMTX(&m);
    }
  if (Known(d, fontsNames[nm_OrigFont]))
    {
    os_fprintf(stm, first? "    " : ", "); first = false;
    DictGetP(d, fontsNames[nm_OrigFont], &ob);
    os_fprintf(stm, "OrigFont=0x%x", ob.val.dictval);
    }
  if (! first) os_fprintf(stm, "\n");
  fflush(stm);
}  /* end of PrintMidDict */

private pMID(m)
  MID m;
{
  StrObj so;  Object ob;  NameObj nobj;
  Stm stm = GetDebugStm();
  PMTItem mp = &MT[m];
  os_fprintf(stm, "mid=%d, hash=%d, umid=[generation=%d, mid=%d], link=%d\n",
      m, mp->hash, mp->umid.rep.generation, mp->umid.rep.mid, mp->mlink);
  os_fprintf(stm, "  fid=0x%x, mtx=", mp->fid); PrintMTX(&mp->mtx);
  os_fprintf(stm, "\n  type=0x%x\n", mp->type);
  os_fprintf(stm, ", mfmid=");PrintBoolean(mp->mfmid);
  os_fprintf(stm, "\n");
  if (mp->mfmid)
    {
    os_fprintf(stm, "  mfdict:   "); PrintMidDict(mp->values.mf.mfdict);
    os_fprintf(stm, "  origdict: "); PrintMidDict(mp->values.mf.origdict);
    os_fprintf(stm, "  dictSpace: %x", mp->values.mf.dictSpace.stamp);
    os_fprintf(stm, "\n");
    }
  else
    {
    os_fprintf(stm, "  bb.ll="); PrintDevCoord(&mp->values.sf.bb.ll);
    os_fprintf(stm, ", bb.ur="); PrintDevCoord(&mp->values.sf.bb.ur);
    os_fprintf(stm, ", shownchar=");PrintBoolean(mp->shownchar);
    os_fprintf(stm, ", monotonic=");PrintBoolean(mp->monotonic);
    os_fprintf(stm, ", maskID=%d\n", mp->maskID);
    }
}

private procedure pCIs(m)
  MID m;
{
  integer nc, npc, i;
  CIOffset cio;
  PNameEntry *pno;
  PNameEntry pne;
  StrObj so;
  Stm stm = GetDebugStm();

  nc = 0;
  ForAllNames(pno, pne, i)
    {
    for (cio = pne->ncilink; cio != CINULL; cio = CI[cio].cilink)
      if (CI[cio].mid == m)
        {
        if (nc++ == 0) {os_fprintf(stm, "  Chars:"); npc = 8;}
        else {npc++; os_fprintf(stm, ",");}
        if ((npc+pne->strLen) > 74) {os_fprintf(stm, "\n\t"); npc = 8;}
        LStrObj(so, pne->strLen, pne->str);
        DPrintSOP(&so);  npc += pne->strLen;
        os_fprintf(stm, "`"); npc++;
        if ((CI[cio].type & Vmem) != 0) {os_fprintf(stm, "V"); npc++;}
        }
    }
  if (nc != 0) os_fprintf(stm, "\n");
}

private procedure pMM()
{
  Stm stm = GetDebugStm();
  MID m; register MID *map;
  os_fprintf(stm, "MM Table:\n");
  forallMM(map)
    {
    if (Aborted()) return;
    m = *map;
    while (m != MIDNULL)
      {
      pMID(m);
      m = MT[m].mlink;
      }
    }
  fflush(stm);
}  /* end of pMM */

private procedure pMS()
{
  Stm stm = GetDebugStm();
  MID m; MID *map;
  os_fprintf(stm, "MS Table:\n");
  forallMS(map)
    {
    if (Aborted()) return;
    m = *map;
    while (m != MIDNULL)
      {
      pMID(m);
      pCIs(m);
      m = MT[m].mlink;
      }
    }
  fflush(stm);
}  /* end of pMS */

private procedure pEldest()
{
  Stm stm = GetDebugStm();
  MID m;
  os_fprintf(stm, "MMEldest: %d\n", MMEldest());
  fflush(stm);
}

private procedure pSFC()
{
  Stm stm = GetDebugStm();
  PSFCEntry psf, *phead;
  NameObj nob;
  forallSFC(phead, psf)
    {
    if (Aborted()) return;
    LNameObj(nob, psf->key);
    PrintNameString(&nob);
    os_fprintf(stm, ", ");
    PrintMTX(&psf->mtx);
    os_fprintf(stm, ", isScaleFont=%d, shared=%d, hash=%d, mid=%d\n",
      psf->isScaleFont, psf->shared, psf->hash, psf->mid);
    }
#if	STAGE == DEVELOP	/* To allow inclusion in EXPORT */
  os_fprintf(stm, "searches: %d, hits: %d, reorders: %d\n",
    sfCache->searches, sfCache->hits, sfCache->reorders);
#endif	STAGE == DEVELOP
  fflush(stm);
}

private procedure CallFromPS(proc)
  procedure (*proc)();
{
  DURING
    calledFromPS = true;
    (*proc)();
    calledFromPS = false;
  HANDLER
    calledFromPS = false;
    RERAISE;
  END_HANDLER;
  fflush(os_stdout);
}

private procedure PSpFD() {CallFromPS(pFD);}
private procedure PSpMM() {CallFromPS(pMM);}
private procedure PSpMS() {CallFromPS(pMS);}
private procedure PSpEldest() {CallFromPS(pEldest);}
private procedure PSpSFC() {CallFromPS(pSFC);}

public DebugInit(reason)
  InitReason reason;
  {
  switch (reason) {
    case romreg:
      if (vSTAGE == DEVELOP) {
	RgstExplicit("printobj", PSPrObject);
	RgstExplicit("printvalue", PSPrValue);
        RgstExplicit("pFD",PSpFD);
        RgstExplicit("pMM",PSpMM);
        RgstExplicit("pMS",PSpMS);
        RgstExplicit("pEldest",PSpEldest);
        RgstExplicit("pSFC",PSpSFC);
        }
    }
  }
