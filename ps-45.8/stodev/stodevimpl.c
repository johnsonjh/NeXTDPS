/*
  stodevimpl.c

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
Jim Sandman: Fri Mar 11 13:09:57 1988
Dick Sweet: Mon Dec 1 10:31:03 PST 1986
Don Andrews: Wed Feb 4 20:31:06 PST 1987
Ed Taft: Tue May 10 13:46:24 1988
Joe Pasqua: Thu Dec  3 11:47:48 1987
Ivor Durham: Sat Feb 13 13:07:41 1988
End Edit History.

Implementation of StoDev utility procedures shared among implementations
of various storage devices.
*/


#include PACKAGE_SPECS
#include PUBLICTYPES
#include ENVIRONMENT
#include EXCEPT
#include PSLIB
#include STODEV

private PStoDev stoDevList;


public PStoDev FndStoDev(name, suffix)
  char *name, **suffix;
{
  register PStoDev dev;
  register char *np, *dp;

  /* extract device name */  
  if (name[0] == LDELIM)
    {
    /* match with device names */
    for (dev = stoDevList; dev != NIL; dev = dev->next)
      for (np = &name[1], dp = dev->name; ; )
        {
        if (*dp == '\0' && (*np == '\0' || *np == RDELIM))
	  {
	  /* Found. Store suffix pointer if requested */
	  if (*np == RDELIM) np++;
	  if (suffix != NIL) *suffix = np;
	  return dev;
	  }
	if (*np++ != *dp++) break;
	}
    /* no such device, or device name is malformed */
    if (suffix != NIL) *suffix = NIL;
    }
  else
    {
    /* device not specified; suffix is just the entire name */
    if (suffix != NIL) *suffix = name;
    }

  return NIL;
}


public PStoDev FndStoFile(name, suffix)
  char *name, **suffix;
{
  register PStoDev dev;
  register boolean searching;

  dev = FndStoDev(name, &name);
  if (suffix != NIL) *suffix = name;
  searching = (dev == NIL);
  if (searching && name != NIL)
    dev = stoDevList;

  while (dev != NIL)
    {
    if ((dev->mounted || ! dev->removable) &&
        (dev->searchable || ! searching) &&
	(! dev->hasNames || (*dev->procs->FindFile)(dev, name)))
      break;
    dev = (searching)? dev->next : NIL;
    }

  return dev;
}


public procedure RgstStoDevice(dev)
  PStoDev dev;
{
  PStoDev succ, *pred;

  /* find proper position in list
     order by searchOrder field, smallest first
     ties go to the first registered */

  for (pred = &stoDevList; (succ = *pred) != NIL; pred = &succ->next)
    if (dev->searchOrder < succ->searchOrder)
      break;

  *pred = dev;
  dev->next = succ;
}


public procedure UnRgstStoDevice(dev)
  PStoDev dev;
{
  PStoDev succ, *pred;

  for (pred = &stoDevList; (succ = *pred) != NIL; pred = &succ->next)
    if (dev == succ)
      {*pred = succ->next; return;}

  CantHappen();  /* dev not registered */
}


public PStoDev StoDevGetNext(dev)
  PStoDev dev;
{
  return (dev == NIL)? stoDevList : dev->next;
}


public procedure StoDevInit()
{
  stoDevList = NIL;
}
