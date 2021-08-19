/*
  stodevpsdisk.c

Copyright (c) 1986, '87, '88, '89 Adobe Systems Incorporated.
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
Ed Taft: Thu Jan  4 17:51:34 1990
Bill McCoy: Mon Nov 14 10:52:39 1988
Peter Hibbard: Wed May 25 20:39:02 1988
Paul Rovner: Wed Aug  9 10:00:23 1989
End Edit History.

PostScript filesystem storage device
*/

#include PACKAGE_SPECS
#include DISK
#include ENVIRONMENT
#include EXCEPT
#include FILESYSTEM
#include PSLIB
#include STODEV
#include STREAM

#define nFileBuffers 20


#define StoDevDisk(dev) (* (Disk *) &dev->private1)

private procedure ValidFileAccess(name) string name;
{
if (*name=='/')
  {
  name++;
  while (*name!=0 && *name!='/') name++;
  if (*name!=0) name++;
  }
if ((name[0]=='S' && name[1]=='y' && name[2]=='s' && name[3]=='/') ||
    (name[0]=='D' && name[1]=='B' && name[2]=='/') ||
    (name[0]=='F' && name[1]=='C' && name[2]=='/'))
  CheckAccess();
}  /* end of ValidFileAccess */

private procedure MapError(fileError)
  FileErrorCode fileError;
{
  int ec;
  switch (fileError)
    {
    case ecNameNotFound:
    case ecNameTooLong:
    case ecNameMalformed:
    case ecIllegalDirName:
      ec = ecUndefFileName; break;
    case ecWriteProtected:
    case ecNameAlreadyExists:
      ec = ecInvalidFileAccess; break;
    default:
      ec = ecIOError; break;
    }
  RAISE(ec, (char *)NIL);
}

Stm PDiskStmCreate(dev, name, acc)
PStoDev dev; char* name; char* acc;
{
  Stm stm; FileAccess access;
  boolean append = false;
  if (os_strlen(acc) > 2 || ! (acc[1] == '+' || acc[1] == '\0'))
    RAISE(ecInvalidFileAccess, (char *) NIL);
  switch (acc[0])
    {
    case 'r':
      access = (acc[1] == '+')? accUpdate : accRead;
      break;
    case 'w':
      access = accOverwrite;
      break;
    case 'a':
      access = accUpdate;
      append = true;
      break;
    default:
      RAISE(ecInvalidFileAccess, (char *) NIL);
    }

 ValidFileAccess(name);
 DURING
   stm = FileDirStmOpen(StoDevDisk(dev), (char *)name, access, 0);
   if (append) fseek(stm, (long int) 0, 2);
 HANDLER
   if ((FileErrorCode) Exception.Code == ecNameNotFound) return NIL;
   else MapError(Exception.Code);
 END_HANDLER;
 return stm;
}  /* end of PDiskStmCreate */

private boolean PDiskFind(dev, name)
  PStoDev dev; char* name;
{ boolean found;
  FilePointer fp;
 DURING found = FileDirFind(StoDevDisk(dev), (char *)name, &fp);
 HANDLER MapError(Exception.Code); END_HANDLER;
 return found;
}  /* end of PDiskFind */

typedef struct {
  int (*action)(/*char *name; char *arg*/);
  char nameBuf[MAXNAMELENGTH];
  char *fileName;  /* points to end of device name, if non-NIL */
  char *arg;
} EnumActionRec;

private int EnumAction (name, fp, arg) 
  char *name; FilePointer *fp; EnumActionRec *arg;
  {
  if (arg->fileName != NIL)
    {
    os_strcpy(arg->fileName, name);
    name = arg->nameBuf;
    }
  return (*arg->action)(name, arg->arg);
  }

private int PDEnumerate(dev, pattern, action, arg)
  PStoDev dev; char *pattern;
  int (*action)(/* char *name, *arg */); char *arg;
{
  int result = 0;
  int fullNames = (pattern[0] == '%');
  EnumActionRec devdata;
  register char *np;

  if (fullNames)
    {
    /* Pattern begins with "%". First, break it into device and file
       pattern portions and see whether the device pattern matches
       this device's name; if not, return immediately. */
    os_strcpy(devdata.nameBuf, pattern + 1);
    np = os_index(devdata.nameBuf, '%');
    if (np == NIL) return 0;
    *np = '\0';
    if (! SimpleMatch(dev->name, devdata.nameBuf)) return 0;

    /* advance pattern to file portion */
    pattern += np - devdata.nameBuf + 2;

    /* We will be enumerating complete names, so set up a buffer in
       which they will be built. */
    os_strcpy(devdata.nameBuf, "%");
    os_strcat(devdata.nameBuf, dev->name);
    os_strcat(devdata.nameBuf, "%");
    devdata.fileName = devdata.nameBuf + os_strlen(devdata.nameBuf);
    }
  else
    devdata.fileName = NIL;

  devdata.action = action;
  devdata.arg = arg;

  DURING
    result = FileDirEnum(StoDevDisk(dev), pattern, SimpleMatch,
                         EnumAction, (char *) &devdata);
  HANDLER MapError(Exception.Code); END_HANDLER;

  return result;
}

private procedure PDRename (dev, old, new)
  PStoDev dev; char *old, *new;
{   
  DURING 
    ValidFileAccess(old);
    ValidFileAccess(new);
    FileDirRename(StoDevDisk(dev), old, new);
  HANDLER MapError(Exception.Code); END_HANDLER;
}  /* end of PDRename */

private procedure PDReadAttr (dev, name, attr)
  PStoDev dev; char* name; StoDevFileAttributes* attr;
{ 
  FilePointer fp;
  FileAttributes fattr;
  fattr.name = NIL;
  DURING
    if (FileDirFind(StoDevDisk(dev), (char *)name, &fp))
      FileReadAttributes(&fp, &fattr);
    else RAISE(ecNameNotFound, (char *) NIL);
  HANDLER MapError(Exception.Code); END_HANDLER;
  attr->pages = fattr.size;
  attr->byteLength = fattr.byteLength;
  attr->accessTime = fattr.accessTime;
  attr->createTime = fattr.createTime;
} /* end of PDReadAttr */

private procedure PDDelete( dev, name )
  PStoDev dev; char* name;
{ 
  DURING 
    ValidFileAccess(name);
    FileDirDelete(StoDevDisk(dev), (char *)name);
  HANDLER MapError(Exception.Code); END_HANDLER;
}  /* end of PDDelete */

private procedure PDDevAttr (dev, attr)
  PStoDev dev; StoDevAttributes *attr;
{
  DiskAttributes dattr;
  attr->size = 0; attr->freePages = 0;
  DiskGetAttributes(StoDevDisk(dev), &dattr);
  if (dattr.initialized)
    {
    attr->freePages = dattr.freePages;
    attr->size = dattr.size;
    }
}  /* end of PDDevAttr */

private procedure PDFormat (dev, pages, format)
  PStoDev dev; integer pages, format;
{
  if (pages < 0) RAISE(ecRangeCheck, (char *) NIL);
  CheckAccess();
  DURING
    dev->mounted = false;
    DiskInitialize(StoDevDisk(dev), pages, format);
    dev->mounted = true;
  HANDLER RAISE(ecIOError, (char *) NIL); END_HANDLER;
}  /* end of PDFormat */

private procedure PDMount(dev)
  PStoDev dev;
{
  DiskOpen(StoDevDisk(dev));
  dev->mounted = true;
}  /* end of PDMount */

private procedure PDDismount (dev )
  PStoDev dev;
{
  Disk disk = StoDevDisk(dev);
  dev->mounted = false;
  DiskClose(disk);
  (*disk->Park)(disk);
}  /* end of PDDismount */


 
private procedure PDNop ()
{}  /* end of PDNop */

private boolean PDBNop ()
{return false;}  /* end of PDBNop */
  
private readonly StoDevProcs Pprocs = {
  PDiskFind,		/* FindFile */
  PDiskStmCreate,	/* CreateDevStm */
  PDDelete,		/* Delete */
  PDRename,		/* Rename */
  PDReadAttr,		/* ReadAttributes */
  PDEnumerate,		/* Enumerate */
  PDDevAttr,		/* DevAttributes */
  PDFormat,		/* Format */
  PDMount,		/* Mount */
  PDDismount		/* Dismount */
};

private char* StripFSDelims(str)
char* str;
{  char* tn;
   if (*str == '/') {
     *str++;
     tn = str;
     while (*tn++)
       if (*tn == '/') {
         *tn = 0;
         break;
       }
   }
   return str;
}
     
private procedure RgstPSDisk(disk, order, mounted)
  Disk disk; integer order; boolean mounted;
{
  PStoDev dev = (PStoDev)os_sureCalloc(
    (long int)1, (long int)sizeof(StoDev));

  dev->procs = &Pprocs;
  dev->removable = true;
  dev->hasNames = true;
  dev->writeable = true;
  dev->searchable = true;
  dev->mounted = mounted;
  dev->searchOrder = order;
  dev->name = os_sureMalloc(os_strlen(disk->name));
  os_strcpy(dev->name, disk->name);
  dev->name = StripFSDelims(dev->name);
  StoDevDisk(dev) = disk;

  RgstStoDevice(dev);
}  /* end of RgstPSDisk */

public procedure PSDiskStoDevInit()
/* create PostScript filesystem storage device */
{
  Disk disk;
  integer timeout;
  integer order = 0;
  boolean mounted;
  FileStart(nFileBuffers);
  timeout = settimer(30000);
  while ((disk = DiskStart()) != NULL)
    {
    while (!timerexpired(timeout))
      {
      DURING
        mounted = false;
        DiskOpen(disk);
	mounted = true;
      HANDLER
        if ((FileErrorCode)Exception.Code == ecNotReady) continue;
      END_HANDLER;
      break;
      }
    RgstPSDisk(disk, order++, mounted);
    }
}
