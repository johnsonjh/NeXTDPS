/*
  stodevunix.c

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

Original version: Don Andrews: 1986
Edit History:
Ed Taft: Mon Oct 23 17:48:46 1989
Ivor Durham: Tue Oct 25 11:28:21 1988
Joe Pasqua: Mon Jan  9 16:48:18 1989
Jim Sandman: Wed Mar 16 15:53:01 1988
Paul Rovner: Tuesday, June 7, 1988 9:37:04 AM
Perry Caro: Thu Nov  3 17:01:20 1988
End Edit History.

Implementation of StoDev for access to general files provided by the
underlying operating system. This uses the "Unix I/O" level of
library procedures; see also unixstream.h.

The Enumerate primitive supports a limited degree of pattern matching.
It supports two forms of patterns: "%devpattern%filepattern" and
"filepattern", where devpattern or filepattern or both contain wildcards
and filepattern does not begin with "%".

The devpattern, if present, determines whether or not this device is to
be considered at all.

The filepattern is scanned for wildcards and "/". The initial substring
up to the last "/" preceding the first wildcard is taken to be the
directory name; the remainder is taken to be a pattern to be matched
against all file names in that directory. If no "/" precedes the first
wildcard (or no "/" appears at all), the current working directory
is used and the entire filepattern is matched against file names
in that directory. File names beginning with "." are excluded from the
enumeration unless the pattern has an explicit "." in the same position.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT

#if (OS == os_mpw)
#include <Files.h>
#include <Strings.h>
#define Fixed MPWFixed
#endif (OS == os_mpw)

#include PUBLICTYPES
#include EXCEPT
#include PSLIB
#include STODEV
#include STREAM
#include UNIXSTREAM

#if (OS == os_mpw)
#include DPSMACSANITIZE
#endif (OS == os_mpw)

#ifdef VAXC
#include types
#include errno
#if OS==os_vms
#include stat
#endif OS==os_vms
#else VAXC
#include <errno.h>
extern int errno;
#if (OS != os_mpw)
#include <sys/types.h>
#include <sys/stat.h>
#endif (OS == os_mpw)
#endif VAXC

/* ENUM_SUPPORTED means the OS supports BSD 4.x style directory enumeration */
#define ENUM_SUPPORTED \
  (OS==os_bsd || OS==os_sun || OS==os_ultrix || OS==os_mach)

#if ENUM_SUPPORTED
#include <sys/dir.h>
#endif ENUM_SUPPORTED

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif BUFSIZ

extern int unlink();

#if	 (OS == os_mpw)
#ifdef MPW3
extern short fsrename();
#else
extern integer Rename();
#endif
#else	(OS == os_mpw)
extern integer rename();
#endif	(OS == os_mpw)

private procedure MapErrno()
{
  int ec;
  switch (errno)
    {
    case ENOENT:
      ec = ecUndefFileName; break;
    case EACCES:
    case EEXIST:
    case EISDIR:
    case EROFS:
    case -1:  /* hack for invalid access string (no errno code for that) */
      ec = ecInvalidFileAccess; break;
    default:
      ec = ecIOError; break;
    }
  RAISE(ec, (char *)NIL);
}


private Stm UDiskStmCreate(dev, name, acc)
  PStoDev dev; char* name; char* acc;
{
  Stm stm;
  errno = -1;
  stm = os_fopen(name, acc);
  if (stm == NIL && errno != ENOENT) MapErrno();
  return stm;
}


private boolean UDiskFind(dev, name)
  PStoDev dev; char* name;
{
  Stm f;
  f = os_fopen(name, "r");
  if (f == NIL) return false;
  else {fclose(f); return true;}
}


#if	(OS == os_mpw)
private OSErr MPWUDReadAttr(dev, name, attr)
  PStoDev dev; char* name; StoDevFileAttributes *attr;
{
  OSErr err;
  FileParam fp;

  fp.ioCompletion = 0;
  fp.ioVRefNum = 0;
  fp.ioNamePtr = c2pstr(name);
  fp.ioFVersNum = 0;
  fp.ioFDirIndex = 0;
  err = PBGetFInfo((ParmBlkPtr)&fp, false);
  p2cstr(name);
  if (err != noErr) return err;
  attr->pages = ((fp.ioFlLgLen + BUFSIZ - 1)/BUFSIZ);
  attr->byteLength = (fp.ioFlLgLen);
  attr->accessTime = (long int)(fp.ioFlMdDat);
  attr->createTime = (long int)(fp.ioFlCrDat);
  return noErr;
}
#endif	(OS == os_mpw)

private procedure UDReadAttr(dev, name, attr)
  PStoDev dev; char* name; StoDevFileAttributes *attr;
{
#if	(OS == os_mpw)
  OSErr err = (OSErr) DPSCall3Sanitized(MPWUDReadAttr, dev, name, attr);
  if (err != noErr) RAISE(ecIOError, NIL);
#else	(OS == os_mpw)
  struct stat buf;
  if (stat(name, &buf) < 0) MapErrno();
  attr->pages = (integer)((buf.st_size + BUFSIZ - 1)/BUFSIZ);
  attr->byteLength = (integer)(buf.st_size);
  attr->accessTime = (integer)(buf.st_atime);
  attr->createTime = (integer)(buf.st_mtime);
#endif	(OS == os_mpw)
}


private procedure UDDelete(dev, name)
  PStoDev dev; char *name;
{
#if OS==os_vms
  if (delete(name) < 0) MapErrno();
#else OS==os_vms
  integer i;

#if (OS == os_mpw)
  i = (integer) DPSCall1Sanitized(unlink, name);
#else (OS == os_mpw)
  
#if (OS == os_mach) /* NeXT mod for security hack */
  {
      extern int IsContextWriteProhibited();
      if(IsContextWriteProhibited()) 
	  /* for now, make the failure mode silent */
	  return;
/*	  RAISE(ecInvalidFileAccess, (char *)NIL); */
  }
#endif 
  i = unlink(name);
#endif (OS == os_mpw)

  if (i < 0) MapErrno();
#endif OS==os_vms
}


private int UDEnumerate(dev, pattern, action, arg)
  PStoDev dev; char *pattern;
  int (*action)(/* char *name, *arg */); char *arg;
{
  int result = 0;
#if ENUM_SUPPORTED
  int fullNames = (pattern[0] == '%');
  char nameBuf[MAXNAMELENGTH];
  register char *name, *np, *pp;
  DIR *dir;
  struct direct *dirent;

  if (fullNames)
    {
    /* Pattern begins with "%". First, break it into device and file
       pattern portions and see whether the device pattern matches
       this device's name; if not, return immediately. */
    os_strcpy(nameBuf, pattern + 1);
    np = os_index(nameBuf, '%');
    if (np == NIL) return 0;
    *np = '\0';
    if (! SimpleMatch(dev->name, nameBuf)) return 0;

    /* advance pattern to file portion */
    pattern += np - nameBuf + 2;
    }

  /* Attempt to extract directory name from prefix of file pattern;
     stop if a wildcard character is encountered. */
  name = NIL;
  for (np = nameBuf, pp = pattern; *pp != '\0'; )
    switch (*np++ = *pp++)
      {
      case '/':
        pattern = pp; name = np; break;
      case '*':
      case '?':
        goto done;
      case '\\':
        /* "\" suppresses semantics of wildcards but not of "/" or null */
        --np;
	if (*pp == '*' || *pp == '?') *np++ = *pp++;
	break;
      }
  done:

  /* If name is not NIL, the string [nameBuf, name) is the directory,
     and pattern has been advanced past the directory. If name is NIL,
     no directory is present and pattern has not been changed. */
  if (name != NIL) *name = '\0';    
  else os_strcpy(nameBuf, ".");
  dir = opendir(nameBuf);
  if (dir == NIL) return 0;

  if (fullNames)
    {
    /* We will be enumerating complete names, so set up a buffer in
       which they will be built. */
    name = nameBuf;
    os_strcpy(name, "%");
    os_strcat(name, dev->name);
    os_strcat(name, "%");
    np = name + os_strlen(name);
    }

  if (*pattern == '\0') pattern = "*";

  DURING
    while ((dirent = readdir(dir)) != NIL)
      if (SimpleMatch(dirent->d_name, pattern) &&
          (dirent->d_name[0] != '.' || pattern[0] == '.'))
        {
        if (fullNames) os_strcpy(np, dirent->d_name);
        else name = dirent->d_name;
        if ((result = (*action)(name, arg)) != 0)
          break;
        }
  HANDLER
    closedir(dir);
    RERAISE;
  END_HANDLER;

  closedir(dir);
#endif ENUM_SUPPORTED
  return result;
}


private procedure UDNop (dev)
  PStoDev dev;
{
}


private procedure UDUndef (dev)
  PStoDev dev;
{
  RAISE(ecUndef, (char *)NIL);
}


#if	(OS == os_aix)
char Error[128];	/* Error buffer used by rename sys call	*/
#endif	(OS == os_aix)

private procedure UDRename (dev, old, new)
  PStoDev dev; char *new, *old;
{
#if	(OS == os_mpw)
#ifdef MPW3
  integer i = (integer) DPSCall3Sanitized(fsrename, old, 0, new);
#else
  integer i = (integer) DPSCall3Sanitized(Rename, old, 0, new);
#endif
  if (i != noErr) RAISE(ecIOError, NIL);
#else	(OS == os_mpw)
  
#if (OS == os_mach) /* NeXT mod for security hack */
  {
      extern int IsContextWriteProhibited();
      if(IsContextWriteProhibited())
	  /* for now, make the failure mode silent */
	  return;
/*	  RAISE(ecInvalidFileAccess, (char *)NIL); */
  }
#endif 

  if (rename(old, new) < 0) MapErrno();
#endif	(OS == os_mpw)
}


private procedure UDDevAttr(dev, attr)
  PStoDev dev; StoDevAttributes *attr;
{
  attr->freePages = attr->size = 0;
}


private readonly StoDevProcs Uprocs =
  {
  UDiskFind,		/* FindFile */
  UDiskStmCreate,	/* CreateDevStm */
  UDDelete,		/* Delete */
  UDRename,		/* Rename */
  UDReadAttr,		/* ReadAttributes */
  UDEnumerate,		/* Enumerate */
  UDDevAttr,		/* DevAttributes -- vestigial implementation */
  UDUndef,		/* Format -- not allowed */
  UDNop,		/* Mount -- not defined for this device */
  UDNop			/* Dismount -- not defined for this device */
  };


public procedure UnixStoDevInit()
/* create Unix disk storage device */
{
  PStoDev dev = (PStoDev)os_sureCalloc(
    (long int)1, (long int)sizeof(StoDev));

  dev->procs = &Uprocs;
  dev->removable = false;
  dev->hasNames = true;
  dev->writeable = true;
  dev->searchable = true;
  dev->mounted = true;
  dev->searchOrder = 2;
  dev->name = "os";

  RgstStoDevice(dev);
}
