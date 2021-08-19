/*
  rgstcmds.c

Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ivor Durham: November 12, 1986
Edit History:
Ivor Durham: Fri Aug 12 15:19:25 1988
Ed Taft: Wed Dec 13 15:54:43 1989
Joe Pasqua: Wed Feb 22 16:27:33 1989
Jim Sandman: Thu Apr  6 12:49:09 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include EXCEPT
#include ORPHANS
#include PSLIB
#include VM

#include "vmnames.h"
#include "vm_segment.h"
#include "vm_space.h"

/* The following are from the language package. They can not	*/
/* be imported since they would cause circular dependencies.	*/
extern boolean Known(), Load();
extern integer PSPopInteger();
#define PopInteger PSPopInteger
extern procedure Begin(), End();
extern procedure DictGetP(), DictPut(), Def();
extern procedure MakeStaticPName(), MakePName(), AllocPNameArray();
extern procedure EPushP(), PopPArray(), PopP();
extern procedure PSCntToMark(), PSClrToMrk();

#define dynamicCmdsDelta 50	/* increment to expand cmd table */
#define maxCmdsExpand 8		/* max times cmd table can be expanded */

#if VMINIT
#define initOpDefSize	 100	/* default value of -O option */
#define initOpSetLimit (4 * pni_end + maxCmdsExpand)
  /* default value of -P option: 4 OpSets/package, plus dynamic expansion */
#endif VMINIT

public CmdTable cmds;

private Card16	/* variables for dynamically registered commands */
  dynOpSetIndex,	/* current OpSet table index for dynamic cmds */
  dynCmdIndex,		/* base cmdIndex for current OpSet */
  dynOpIndex,		/* index of next op to assign in current OpSet */
  dynOpLimit;		/* size of current OpSet */
private Int16 dynOpSetID; /* next OpSet ID to assign */


private PutCmd(index, proc)
  cardinal index;
  procedure (*proc)();
{
  Assert (index < rootShared->vm.Shared.cmdCnt);
  cmds.cmds[index] = proc;
}

private procedure PSUnregistered()
{
  PSError (unregistered);
}

private procedure AllocCmds(cmdIndex, numCmds)
  Card16 cmdIndex, numCmds;
{
  register Card16 oldmax, newmax, i;

  cmdIndex += numCmds;
  if (cmdIndex > cmds.max)
    {
    oldmax = cmds.max;
    newmax = os_max((integer)cmdIndex, (integer)cmds.max + dynamicCmdsDelta);
    cmds.cmds = (procedure (**)()) EXPAND(
      (charptr)cmds.cmds, (integer)newmax,
      (integer)sizeof(procedure (*)()));
    cmds.max = newmax;
    for (i = oldmax; i < newmax; cmds.cmds[i++] = PSUnregistered);
    }
  if (cmdIndex > rootShared->vm.Shared.cmdCnt)
    rootShared->vm.Shared.cmdCnt = cmdIndex;
}

public PNameObj RgstPackageNames(pkgIndex, count)
  integer pkgIndex, count;
{
  AryObj ao;
  NameObj unreg;
  register integer i;

  DebugAssert(pkgIndex >= 0 && pkgIndex < pni_end && count > 0);
  ao = VMGetElem(rootShared->vm.Shared.regNameArray, pkgIndex);

#if VMINIT
  if (ao.type != arrayObj)
    {
    Level origSegType = CurrentVMSegmentType();
    Assert(vmCurrent->wholeCloth && CurrentShared());
    VMSetROMAlloc();  /* allocate static name arrays in ROM */
    DURING
      AllocPArray((cardinal)count, (PObject)&ao);
    HANDLER
      SetVMSegmentType(origSegType);
      RERAISE;
    END_HANDLER;
    SetVMSegmentType(origSegType);
    VMPutElem(rootShared->vm.Shared.regNameArray, (cardinal)pkgIndex, ao);
    /* need to construct name "unregistered" from whole cloth, since
       static names may not have been filled in yet */
    MakePName("unregistered", &unreg);
    for (i = count; --i >= 0; )
      VMPutElem(ao, (cardinal) i, unreg);
    }
#endif VMINIT

  Assert(ao.type == arrayObj && count == ao.length);
  return (ao.val.arrayval);
}

private integer FindOpSet(opSetID, create, pCmdIndex)
  register Int16 opSetID; boolean create; Card16 *pCmdIndex;
/* Searches regOpIDArray for opSetID and returns its OpSet table index.
   If not found and "create" is true, creates new entry; otherwise returns -1.
   *pCmdIndex receives the cmdIndex for the first command of the OpSet.
 */
{
  Int16 *opSetIDTbl = (PInt16) rootShared->vm.Shared.regOpIDArray.val.strval;
  Card16 limit = rootShared->vm.Shared.regOpIDArray.length / sizeof(Int16);
  register Card16 i;
  register Card16 cmdIndex = 0;
  NameArrayObj nao;

  DebugAssert(opSetID != 0);
  for (i = 0; i < limit; i++)
    {
    if (opSetID == opSetIDTbl[i]) goto found;
    if (opSetIDTbl[i] == 0) break;
    nao = VMGetElem(rootShared->vm.Shared.regOpNameArray, i);
    DebugAssert(nao.type == escObj && nao.length == objNameArray);
    cmdIndex += nao.val.namearrayval->length;
    }

  if (! create) return -1;
  if (i >= limit) LimitCheck(); /* too many OpSets; increase limit using -P */
  opSetIDTbl[i] = opSetID;

found:
  *pCmdIndex = cmdIndex;
  return i;
}

public procedure RgstOpSet(cmdProcs, numProcs, opSetID)
  procedure (**cmdProcs)(); Card16 numProcs; Int16 opSetID;
{
  NameArrayObj nao;
  Card16 cmdIndex, opSetIndex;
  boolean oldShared;
#if VMINIT
  Level origSegType;
#endif VMINIT

  opSetIndex = FindOpSet(opSetID, true, &cmdIndex);
  nao = VMGetElem(rootShared->vm.Shared.regOpNameArray, opSetIndex);
  if (nao.type == nullObj)
    {
    /* Either we are building the VM from scratch or this OpSet
       is newly created due to dynamic operator registration.
       When we're registering an OpSet statically, the NameArray
       can be in ROM, but a dynamic OpSet must have its NameArray
       in RAM since product initialization may add things to it.
     */
    DebugAssert(vmCurrent->wholeCloth || cmdProcs == NIL);
    oldShared = CurrentShared();
    SetShared(true);
#if VMINIT
    origSegType = CurrentVMSegmentType();
    SetVMSegmentType((cmdProcs == NIL)? stPermanentRAM : stROM);
#endif VMINIT
    DURING
      AllocPNameArray(numProcs, &nao);
    HANDLER
      SetShared(oldShared);
#if VMINIT
      SetVMSegmentType(origSegType);
#endif VMINIT
      RERAISE;
    END_HANDLER;
    SetShared(oldShared);
#if VMINIT
    SetVMSegmentType(origSegType);
#endif VMINIT
    VMPutElem(rootShared->vm.Shared.regOpNameArray, opSetIndex, nao);
    }
  else
    {
    DebugAssert(! vmCurrent->wholeCloth); /* duplicate opSetID */
    DebugAssert(nao.type = escObj && nao.length == objNameArray &&
                numProcs == nao.val.namearrayval->length);
    }

  AllocCmds(cmdIndex, numProcs);
  if (cmdProcs != NIL)
    os_bcopy((char *) cmdProcs, (char *) &cmds.cmds[cmdIndex],
             (integer) (numProcs * sizeof (procedure (*)())));
}

public procedure CmdIndexObj(cmdIndex, pcmd)
  Card16 cmdIndex; PCmdObj pcmd;
{
  NameArrayObj nao;
  PNameEntry pne;
  register Card16 i, ci;

  ci = cmdIndex;
  for (i = 0; i < rootShared->vm.Shared.regOpNameArray.length; i++)
    {
    nao = VMGetElem(rootShared->vm.Shared.regOpNameArray, i);
    if (nao.type == nullObj) break;
    DebugAssert(nao.type == escObj && nao.length == objNameArray);
    if (ci < nao.val.namearrayval->length)
      {
      pne = nao.val.namearrayval->nmEntry[ci];
      DebugAssert(pne != NIL);
      XCmdObj(*pcmd, pne, cmdIndex);
      return;
      }
    ci -= nao.val.namearrayval->length;
    }

  CantHappen();  /* cmdIndex out of bounds */
}

private procedure RgstDynamicCmd(name, proc, pCmd)
  NameObj name; procedure (*proc)(); PCmdObj pCmd;
{
  Card16 cmdIndex;
  NameArrayObj nao;
  integer i;

  if (dynOpSetID == 0)
    { /* first call; need to find first unused dynamic operator */
    dynOpSetID = 256 * pni_end;
    while ((i = FindOpSet(dynOpSetID, false, &dynCmdIndex)) >= 0)
      {dynOpSetIndex = i; dynOpSetID++;}
    if (dynOpSetID > 256 * pni_end)
      {
      nao = VMGetElem(rootShared->vm.Shared.regOpNameArray, dynOpSetIndex);
      DebugAssert(nao.type = escObj && nao.length == objNameArray);
      dynOpLimit = nao.val.namearrayval->length;
      for (dynOpIndex = 0; dynOpIndex < dynOpLimit; dynOpIndex++)
        if (nao.val.namearrayval->nmEntry[dynOpIndex] == NIL) break;
      }
    }

  if (dynOpIndex >= dynOpLimit)
    { /* need to create new dynamic OpSet */
    dynOpSetIndex = FindOpSet(dynOpSetID, true, &dynCmdIndex);
    RgstOpSet(NIL, dynamicCmdsDelta, dynOpSetID++);
    dynOpIndex = 0;
    dynOpLimit = dynamicCmdsDelta;
    }

  nao = VMGetElem(rootShared->vm.Shared.regOpNameArray, dynOpSetIndex);
  DebugAssert(nao.type == escObj && nao.length == objNameArray &&
              dynOpIndex < nao.val.namearrayval->length &&
	      nao.val.namearrayval->nmEntry[dynOpIndex] == NIL);
  nao.val.namearrayval->nmEntry[dynOpIndex] = name.val.nmval;
  cmdIndex = dynCmdIndex + dynOpIndex++;
  PutCmd(cmdIndex, proc);
  XCmdObj(*pCmd, name.val.nmval, cmdIndex);
}

public procedure RgstMark(s, proc, mark, pcobj)
  char *s;  procedure (*proc)(); integer mark;  register PCmdObj pcobj;
{
  NameObj name;

  MakePName (s, &name);
  if (Known (rootShared->vm.Shared.internalDict, name))
    {
    DictGetP (rootShared->vm.Shared.internalDict, name, pcobj);
    PutCmd(pcobj->length, proc);
    }
  else
    {
    RgstDynamicCmd(name, proc, pcobj);
    pcobj->access = mark;
    DictPut (rootShared->vm.Shared.internalDict, name, *pcobj);
    }
}				/* end of RgstMark */

public procedure RgstInternal(s, proc, rcobj)
  char *s;  procedure (*proc)();  PCmdObj rcobj;
{RgstMark(s, proc, (integer) mrkNone, rcobj);}

public RgstObject(s, val)  char *s;  Object val;
{
  NameObj name;
  Object  oldVal;

  MakeStaticPName (s, &name);

  if (!Load (name, &oldVal))
    Def (name, val);
}

public procedure RgstExplicit(s,proc)
	char *s; procedure (*proc)();
{
  Object  val;
  cardinal index;
  NameObj name;

  MakePName (s, &name);

  if (Load (name, &val)) {
    if (val.type == cmdObj) {
      index = val.length;
      if (cmds.cmds[index] != (void (*)()) PSUnregistered)
        InvlAccess(); /* not allowed to redefine existing operators */
      PutCmd (index, proc);
      }
    else
      return;
    }
  else
    {
    RgstDynamicCmd(name, proc, &val);
    Def (name, val);
    }
}

public procedure RgstMCmds(p)  PRgCmdEntry p;
{
  while (p->name != NIL) {
    RgstExplicit (p->name, p->proc);
    p++;
  }
}

public procedure RgstMNames(p)  PRgNameEntry p;
{
  while (p->name != NIL) {
    MakePName (p->name, p->pob);
    p++;
  }
}

#if VMINIT
/* opdef machinery */

public Card16 currentCmd;

typedef struct {integer cmdIndex; Object impl;} OpDefEntry;

OpDefEntry *opDefTable;
integer sizeOpDefTable;

private OpDefEntry *FindOpDef(cmdIndex, new)
  register integer cmdIndex; boolean new;
{
  register OpDefEntry *initProbe, *probe;
  initProbe = probe = &opDefTable[cmdIndex % sizeOpDefTable];
  do {
    if (probe->cmdIndex == cmdIndex) return probe;
    if (probe->cmdIndex == -1)
      if (new) return probe;
      else CantHappen();
    if (--probe < opDefTable) probe = &opDefTable[sizeOpDefTable-1];
  } while (probe != initProbe);
  if (new) LimitCheck();
  else CantHappen();
  /*NOTREACHED*/
}

private procedure OpDefProc()
{
  EPushP(&(FindOpDef((integer)currentCmd, false))->impl);
}

private procedure PSOpDef()  /* opdef */
{
  NameObj name; AnyAryObj proc; Object cmd;
  OpDefEntry *Entry;
  PopPArray(&proc);
  PopP(&name);
  if ((proc.access & (xAccess|rAccess)) == 0) InvlAccess();
  if (name.type != nameObj) TypeCheck();
  name.tag = Xobj;
  if (Load(name, &cmd) && cmd.type == cmdObj)
    {
    Entry = FindOpDef((integer)cmd.length, true);
    PutCmd(cmd.length, OpDefProc);
    }
  else
    {
    RgstDynamicCmd(name, OpDefProc, &cmd);
    Entry = FindOpDef((integer)cmd.length, true);
    Def(name, cmd);
    }
  Entry->cmdIndex = cmd.length;
  Entry->impl = proc;
}

/* Static name and operator registration operators */

private procedure PSRgstNames()
/* mark /name1 /name2 ... /nameN pkgIndex  registernames  -- */
{
  AryObj ao;
  NameObj name;
  integer pkgIndex;
  register integer count;

  pkgIndex = PopInteger();
  if (pkgIndex < 0 || pkgIndex >= pni_end) RangeCheck();
  ao = VMGetElem(rootShared->vm.Shared.regNameArray, pkgIndex);
  Assert(ao.type == arrayObj)

  PSCntToMark();
  count = PopInteger();
  if (count != ao.length) RangeCheck();

  while (--count >= 0)
    {
    PopP(&name);
    if (name.type != nameObj) TypeCheck();
    name.tag = Xobj;  /* always executable */
    VMPutElem(ao, (cardinal) count, name);
    }

  PSClrToMrk();
}

private procedure PSRgstOps()
/* mark /name1 /name2 ... /nameN /dict opSetID  registeroperators  -- */
{
  NameArrayObj nao;
  NameObj name, dictName;
  DictObj dict;
  CmdObj cmd;
  Card16 cmdIndex;
  Int16 opSetID;
  register integer count;
  integer opSetIndex;

  opSetID = PopInteger();

  /* Pop dictionary name and translate to dictionary.
     Note: "internaldict" must be handled specially, since it's an
     operator rather than a dictionary. */
  PopP(&dictName);
  if (dictName.type != nameObj) TypeCheck();
  MakePName("internaldict", &name);
  if (dictName.val.nmval == name.val.nmval)
    dict = rootShared->vm.Shared.internalDict;
  else
    {
    if (! Load(dictName, &dict)) Undefined();
    if (dict.type != dictObj) TypeCheck();
    }

  PSCntToMark();
  count = PopInteger();

  opSetIndex = FindOpSet(opSetID, (opSetID < 0), &cmdIndex);
  if (opSetIndex < 0)
    {PSClrToMrk(); return;}  /* OpSet not registered; ignore */

  nao = VMGetElem(rootShared->vm.Shared.regOpNameArray, opSetIndex);
  if (nao.type == nullObj)
    RgstOpSet(NIL, count, opSetID);
  else
    {
    DebugAssert(nao.type == escObj && nao.length == objNameArray);
    if (count != nao.val.namearrayval->length) RangeCheck();
    }

  cmdIndex += count;
  Assert(cmdIndex <= rootShared->vm.Shared.cmdCnt);
  while (--count >= 0)
    {
    PopP(&name);
    if (name.type != nameObj) TypeCheck();
    nao.val.namearrayval->nmEntry[count] = name.val.nmval;
    XCmdObj(cmd, name.val.nmval, --cmdIndex);
    DictPut(dict, name, cmd);
    }

  PSClrToMrk();
}
#endif VMINIT


public procedure Init_Cmds (reason)
  InitReason reason;
/* Must be absolutely the first package initialization procedure called */
{
  register integer i;
  switch (reason)
  {
    case romreg:
#if VMINIT
      if (vmCurrent->wholeCloth)
        {
	Assert(CurrentShared() && CurrentVMSegmentType() != stROM);
	AllocPArray((Card16) pni_end, &rootShared->vm.Shared.regNameArray);

        i = NumCArg('P', (integer)10, (integer)initOpSetLimit);
	AllocPArray((Card16) i, &rootShared->vm.Shared.regOpNameArray);

	i *= sizeof(Int16);
	LStrObj(rootShared->vm.Shared.regOpIDArray, i, AllocAligned(i));
	os_bzero((char *) rootShared->vm.Shared.regOpIDArray.val.strval, i);

	/* Init cmd table assuming that systemdict, internaldict, and
	   statusdict will be completely filled with registered operators */
	Assert(rootShared->vm.Shared.cmdCnt == 0);
	cmds.max = rootShared->vm.Shared.sysDict.val.dictval->maxlength +
	  rootShared->vm.Shared.internalDict.val.dictval->maxlength +
	  VMGetElem(rootShared->trickyDicts, tdStatusDict).val.dictval->maxlength;
	}
      else
#endif VMINIT
        {
	cmds.max = rootShared->vm.Shared.cmdCnt + dynamicCmdsDelta;
	}
      cmds.cmds = (procedure (**)()) NEW (cmds.max, sizeof (procedure (*)()));
      for (i = cmds.max; --i >= 0; ) cmds.cmds[i] = PSUnregistered;

#include "ops_vmDPS.c"

#if VMINIT
      if (vmCurrent->wholeCloth)
        {
        sizeOpDefTable = NumCArg('O', (integer)10, (integer)initOpDefSize);
        opDefTable = (OpDefEntry *) NEW(sizeOpDefTable, sizeof (OpDefEntry));
        for (i = sizeOpDefTable; --i >= 0; ) opDefTable[i].cmdIndex = -1;
        Begin(rootShared->vm.Shared.internalDict);
        RgstExplicit("opdef", PSOpDef);
	RgstExplicit("registernames", PSRgstNames);
	RgstExplicit("registeroperators", PSRgstOps);
        End();
	}
#endif VMINIT
      break;
  }
}
