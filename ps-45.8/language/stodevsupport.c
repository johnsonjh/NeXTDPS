/*
  stodevsupport.c

Copyright (c) 1986, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Don Andrews: October 1986
Edit History:
Jim Sandman: Fri Mar 11 13:08:53 1988
Dick Sweet: Mon Dec 1 10:31:03 PST 1986
Don Andrews: Wed Feb 4 20:31:06 PST 1987
Ed Taft: Mon Sep 18 16:18:06 1989
Joe Pasqua: Thu Jan  5 11:39:48 1989
Ivor Durham: Sat May  7 01:29:29 1988
End Edit History.

PostScript language interface to Storage Devices
*/


#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include LANGUAGE
#include ORPHANS
#include PSLIB
#include STODEV
#include VM

#include "streampriv.h"


private procedure PopFlName(name)
  char *name;
  /* Pops a string object from the stack and stores its contents
     into *name, which must be a string of length MAXNAMELENGTH.
     Does not return if an error occurs. */
{
  StrObj str;
  PopPRString(&str);
  if (str.length >= MAXNAMELENGTH) LimitCheck();
  StringText(str, (string) name);
}


/* StoDevSupport implementation */

public Stm StoDevCreateStm(name, acc)
  char *name, *acc;
{
  register PStoDev dev;
  register boolean searching;
  Stm stm;

  dev = FndStoDev(name, &name);
  searching = dev == NIL;
  if (searching && name != NIL)
    dev = StoDevGetNext((PStoDev)NIL);

  while (dev != NIL)
    {
    if ((dev->mounted || ! dev->removable) &&
        (dev->searchable || ! searching) &&
	(stm = (*dev->procs->CreateDevStm)(dev, name, acc)) != NIL)
      return stm;
    dev = (searching)? StoDevGetNext(dev) : (PStoDev) NIL;
    }

  return NIL;
}

public procedure StoDevStrStatus(ob)
  StrObj ob;
{
  PStoDev dev;
  char name[MAXNAMELENGTH], *suffix;
  StoDevFileAttributes attr;
  if ((ob.access & rAccess) == 0) InvlAccess();
  if (ob.length>=MAXNAMELENGTH) LimitCheck();
  StringText(ob, (string)name);

  if ((dev = FndStoFile(name, &suffix)) != NIL)
    {
    DURING
      (*dev->procs->ReadAttributes)(dev, suffix, &attr);
    HANDLER
      dev = NIL;
    END_HANDLER;
    if (dev != NIL)
      {
      PushInteger(attr.pages);
      PushInteger(attr.byteLength);
      PushInteger((integer)attr.accessTime);
      PushInteger((integer)attr.createTime);
      PushBoolean(true);
      return;
      }
    }

  PushBoolean(false); /* no file found */
}


/* PostScript operators */

public procedure PSDeleteFile() /* string deletefile -- */
{
  char name[MAXNAMELENGTH], *suffix;
  PStoDev dev;
  (void)PopFlName(name);
  if ((dev = FndStoFile(name, &suffix)) == NIL) UndefFileName();
  (*dev->procs->Delete)(dev, suffix);
}


public procedure PSRenameFile() /* strold strnew renamefile */
{
  char old[MAXNAMELENGTH], new[MAXNAMELENGTH],
    *oldSuffix, *newSuffix;
  PStoDev dev, newDev;
  PopFlName(new);
  PopFlName(old);
  if ((dev = FndStoFile(old, &oldSuffix)) == NIL) UndefFileName();
  newDev = FndStoDev(new, &newSuffix);
  if (dev != newDev && newDev != NIL) PSError(invlflaccess);
  if (newSuffix == NIL) UndefFileName();
  (*dev->procs->Rename)(dev, oldSuffix, newSuffix);
}


public procedure PSFilPos()	/* fileposition */
{
  StmObj sob; Stm stm;
  integer i;
  PopPStream(&sob);
  stm = GetStream(sob);
  if (stm->flags.f1) HandlePendingEOL(stm);
  if ((i = ftell(stm)) < 0) StreamError(stm);
  else PushInteger(i);
}


public procedure PSStFilPos()	/* setfileposition */
{
  StmObj sob; Stm stm;
  integer i;
  i = PopInteger();
  PopPStream(&sob);
  stm = GetStream(sob);
  stm->flags.f1 = false;
  if (fseek(stm, i, 0) < 0) StreamError(stm);
}


typedef struct {
  AryObj proc;
  StrObj scratch;
  } FFAArgs;

/*ARGSUSED*/
private int FFAAction(name, args)
  string name; FFAArgs *args;
{
  StrObj str;
  str = args->scratch;
  if ((str.length = StrLen(name)) > args->scratch.length)
    RangeCheck();
  VMPutText(str, name);
  PushP(&str);
  return psExecute(args->proc);
}

public procedure PSFileNameForAll()
/* pattern proc scratch filenameforall -- */
{
  FFAArgs args;
  char pattern[MAXNAMELENGTH];
  PStoDev dev;
  boolean searching;

  PopPString(&args.scratch);
  if ((args.scratch.access & wAccess) == 0) InvlAccess();
  PopPArray(&args.proc);
  (void) PopFlName(pattern);
  dev = FndStoDev(pattern, (char **)NIL);
  searching = (dev == NIL);
  if (searching) dev = StoDevGetNext((PStoDev)NIL);

  while (dev != NIL)
    {
    if ((dev->mounted || ! dev->removable) &&
        (dev->searchable || ! searching) &&
	dev->hasNames)
      if ((*dev->procs->Enumerate)(
          dev, pattern, FFAAction, &args) != 0) break;
    dev = (searching)? StoDevGetNext(dev) : NIL;
    }
}


public procedure PSDevForAll() /* proc scratch devforall -- */
{
  register PStoDev dev;
  StrObj scratch, str;
  AryObj ary;
  register char* tn;

  PopPString(&str);
  if ((str.access & wAccess) == 0) InvlAccess();
  PopPArray(&ary);
  for (dev = NIL; (dev = StoDevGetNext(dev)) != NIL; )
    {
    scratch = str;
    if ((scratch.length = StrLen(dev->name) + 2) > str.length)
      RangeCheck();
    tn = (char *) scratch.val.strval;
    *tn = '%';
    os_strcpy(tn + 1, dev->name);
    *(tn + scratch.length - 1) = '%';
    PushP(&scratch);
    if (psExecute(ary)) break;
    }
}  /* end of PSDevForAll */

public procedure PSDevStatus()
{ /* provide status given device name */
  char name[MAXNAMELENGTH];
  PStoDev dev;
  StoDevAttributes attr;

  PopFlName(name);
  if ((dev = FndStoDev(name, NIL)) == NIL) 
    PushBoolean(false);
  else
  {
    (*dev->procs->DevAttributes)(dev, &attr);
    PushBoolean(dev->searchable);
    PushBoolean(dev->writeable);
    PushBoolean(dev->hasNames);
    PushBoolean(dev->mounted);
    PushBoolean(dev->removable);
    PushInteger(dev->searchOrder);
    PushInteger(attr.freePages);
    PushInteger(attr.size);
    PushBoolean(true);
  }
}  /* end of PSDevStatus */

public procedure PSDevFormat()
{ /* format the device if possible */
  /* devname pages formatflag devformat -- */
  char name[MAXNAMELENGTH];
  PStoDev dev;
  integer pages, format;

  format = PopCardinal();
  pages = PopInteger();
  PopFlName(name);
  if ((dev = FndStoDev(name, NIL)) == NIL) UndefFileName();
  (*dev->procs->Format)(dev, pages, format);
}  /* end of PSDevFormat */


public procedure PSDevMount()
{  /* mount the given device if mountable */
  char name[MAXNAMELENGTH];
  PStoDev dev;
  boolean result = false;

  PopFlName(name);
  if ((dev= FndStoDev(name, NIL)) == NIL) UndefFileName();
  if (dev->mounted) result = true;
  else if (dev->removable) {
    DURING
    (*dev->procs->Mount)(dev);
    result = true;
    HANDLER {} END_HANDLER;
    }
  PushBoolean(result);
  return;
}  /* end of PSDevMount */

public procedure PSDevDisMount()
{  /* dismount the given device if mountable */
  char name[MAXNAMELENGTH];
  PStoDev dev;

  PopFlName(name);
  if ((dev= FndStoDev(name, NIL)) == NIL) UndefFileName();
  if (dev->mounted) (*dev->procs->Dismount)(dev);
  return;
}  /* end of PSDevMount */

public procedure StoDevSupportInit(reason)
  InitReason reason;
{
}
