/*
  stodevfont.c

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

Original version: Ivor Durham, Mon Mar 28 1988
Edit History:
Ivor Durham: Tue Oct 25 11:28:21 1988
Ed Taft: Tue May 10 15:51:09 1988
Mike Schuster: August 3, 1988
Paul Rovner: Friday, August 12, 1988 8:31:40 AM
Perry Caro: Thu Nov  3 16:59:33 1988
Joe Pasqua: Mon Jan  9 16:46:08 1989
End Edit History.
*/

/*
  This module implements a storage device for fonts.  It is derived directly
  from the Unix storage device, but differs in that (a) it is readonly and
  (b) a string is prepended to filenames to insert the appropriate path for
  finding font files.  The string is supplied to the initialisation procedure.
  The Enumerate operation treats the string as a directory name; the
  match excludes names that begin with "." unless the pattern itself
  begins with ".".
 */

#import <errno.h>
#import <cthreads.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <strings.h>
#import PACKAGE_SPECS
#import ENVIRONMENT
#import PUBLICTYPES
#import EXCEPT
#import PSLIB
#import STODEV
#import STREAM
#import UNIXSTREAM

/* ENUM_SUPPORTED means the OS supports BSD 4.x style directory enumeration */
#define ENUM_SUPPORTED \
  (OS==os_bsd || OS==os_sun || OS==os_ultrix || OS==os_mach)

#if ENUM_SUPPORTED
#include <sys/dir.h>
#endif ENUM_SUPPORTED

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif BUFSIZ

static char *fontDirPath;

private procedure MapErrno()
{
  int ec;
  switch (cthread_errno())
    {
    case ENOENT:
      ec = ecUndefFileName; break;
    case EACCES:
    case EEXIST:
    case EISDIR:
    case EROFS:
      ec = ecInvalidFileAccess; break;
    default:
      ec = ecIOError; break;
    }
  RAISE(ec, (char *)NIL);
}

int FontPathSearch(char *fontName, char *fileName, char filePath[], int size)
{
    int ret;
    char name[256];
    
	/* Bail out when fontName is not present */
	if (!fontName) return -1;

    /* Search for "<fontName>.font/<fileName>" in fontDirPath */
    os_strcpy(name, fontName);
    os_strcat(name, ".font/");
    os_strcat(name, fileName);
    if((ret =FilePathSearch(fontDirPath,name,strlen(name),filePath,size)) >= 0)
	return ret;
    
    /* Search for outline files in "outline/<fileName>" 
     * Search for bitmap  files in "bitmap/<filename>"
     */
    os_strcpy(name, (fontName == fileName) ? "outline/" : "bitmap/");
    os_strcat(name, fileName);
    return FilePathSearch(fontDirPath, name, strlen(name), filePath, size);
}

private Stm FontStmCreate(dev, name, acc)
  PStoDev dev; char* name; char* acc;
{
    Stm     stm = NIL;
    char pathName[256];
    
    if (acc[0] != 'r' || acc[1] == '+')
    	RAISE(ecInvalidFileAccess, (char *)NIL);
    if (FontPathSearch(name,name,pathName,sizeof(pathName)) < 0)
	return((Stm)NIL);
    stm = os_fopen (pathName, acc);
    if (stm == NIL && cthread_errno() != ENOENT) MapErrno();
    return stm;
}


private boolean FontFind(dev, name)
  PStoDev dev; char* name;
{
    char pathName[256];

    return(FontPathSearch(name,name,pathName,sizeof(pathName)) == 0);
}


private procedure FontReadAttr(dev, name, attr)
  PStoDev dev; char* name; StoDevFileAttributes *attr;
{
    struct stat buf;
    char pathName[256];
    
    if (FontPathSearch(name,name,pathName,sizeof(pathName)) < 0)
    	RAISE(ecUndefFileName,(char *)NIL);
    if (stat(pathName, &buf) < 0) MapErrno();
    attr->pages = (integer) ((buf.st_size + BUFSIZ - 1) / BUFSIZ);
    attr->byteLength = (integer) (buf.st_size);
    attr->accessTime = (integer) (buf.st_atime);
    attr->createTime = (integer) (buf.st_mtime);
}

private int FontEnumOneDir(thisDir,pattern,fullNames,action,arg)
char *thisDir,*pattern;
int (*action)(/* char *name, *arg */), fullNames; char *arg;
{
    char nameBuf[256],dirBuf[256],newpattern[256],*np;
    int	result = 0;
    DIR *dir;
    struct direct *dirent;
    
    if (fullNames)
    {
	/* We will be enumerating complete names, so set up a buffer in
	which they will be built. */
	os_strcpy(nameBuf, "%font%");
	np = nameBuf + 6;
    }
    else
    	np = nameBuf;
    /* Argh!  Check for ~ */
    if (*thisDir == '~')
    {
    	char *hd;
	
	/* myGetpwdir is out of miscops.c */
        if ((hd = (char *)myGetpwdir(geteuid()))&&(*hd))
	{
	    strcpy(dirBuf,hd);
	    strcat(dirBuf,thisDir+1);
	    thisDir = dirBuf;
	}
	else
	    return(0);
    }
    dir = opendir(thisDir);
    if (dir == NIL) return 0;
    
    /* Append ".font" extension to the search pattern */
    if (*pattern == '\0') strcpy(newpattern, "*.font");
    else {
	os_strcpy(newpattern, pattern);
	os_strcat(newpattern, ".font");
    }
    
    DURING
	while ((dirent = readdir(dir)) != NIL)
	    if (SimpleMatch(dirent->d_name, newpattern) &&
	    	(dirent->d_name[0] != '.' || newpattern[0] == '.'))
	    {
	        /* Set np to "fontname.font/fontname" */
		os_strcpy(np, dirent->d_name);
		os_strcat(np, "/");
		os_strcat(np, dirent->d_name);
		np[os_strlen(np)-5] = '\0';
		if ((result = (*action)(nameBuf, arg)) != 0)
		    break;
	    }
    HANDLER
    closedir(dir);
    RERAISE;
    END_HANDLER;
    
    closedir(dir);
    return(result);
}

private int FontEnumerate(dev, pattern, action, arg)
PStoDev dev; char *pattern;
int (*action)(/* char *name, *arg */); char *arg;
{
    int result = 0;
    int fullNames = (pattern[0] == '%');
    char *thisDir,*dirList,dirPathBuffer[256],*colon;
    DIR *dir;
    struct direct *dirent;
    
    if (fullNames)
    {
        char *np;
	
	/* Pattern begins with "%". First, break it into device and file
	pattern portions and see whether the device pattern matches
	this device's name; if not, return immediately. */
	np = os_index(pattern+1, '%');
	if ((np == NIL)||((np-(pattern+1)) != strlen(dev->name)))
	    return 0;
	if (strncmp(dev->name, pattern+1, strlen(dev->name)))
	    return 0;
	/* advance pattern to file portion */
	pattern = np + 1;
    }
    /* Call FontEnumOneDir for each of the directories in directory list */
    strcpy(dirPathBuffer,fontDirPath);
    dirList = dirPathBuffer;
    while(*dirList) {
    	colon = index(dirList,':');
	thisDir = dirList;
	if (colon)
	{
	    *colon = (char)0;
	    dirList = colon+1;
	}
	else
	    dirList += strlen(dirList);
	if (result = FontEnumOneDir(thisDir,pattern,fullNames,action,arg))
	    break;
    }
    return result;
}

private procedure FontNop (dev)
  PStoDev dev;
{
}

private procedure FontUndef (dev)
  PStoDev dev;
{
  RAISE(ecUndef, (char *)NIL);
}

private procedure FontDevAttr(dev, attr)
  PStoDev dev; StoDevAttributes *attr;
{
  attr->freePages = attr->size = 0;
}

private readonly StoDevProcs FontProcs =
  {
  FontFind,		/* FindFile */
  FontStmCreate,	/* CreateDevStm */
  FontUndef,		/* Delete -- not allowed */
  FontUndef,		/* Rename -- not allowed */
  FontReadAttr,		/* ReadAttributes */
  FontEnumerate,	/* Enumerate */
  FontDevAttr,		/* DevAttributes -- vestigial implementation */
  FontUndef,		/* Format -- not allowed */
  FontNop,		/* Mount -- not defined for this device */
  FontNop		/* Dismount -- not defined for this device */
  };

private procedure RgstFontDevice()
{
  PStoDev dev = (PStoDev)os_sureCalloc((long int)1, (long int)sizeof(StoDev));

  dev->procs = &FontProcs;
  dev->removable = false;
  dev->hasNames = true;
  dev->writeable = false;
  dev->searchable = false;
  dev->mounted = true;
  dev->name = "font";

  RgstStoDevice (dev);
}

public procedure FontStoDevInit(initFontDirPath)
char *initFontDirPath;
{
    fontDirPath = initFontDirPath;
    RgstFontDevice();
}






