/*
  filetypes.h

Copyright (c) 1987, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Thu Jan 24 10:47:20 1985
Edit History:
Ed Taft: Tue Jul  7 09:45:39 1987
Ivor Durham: Sun Aug 14 14:29:33 1988
End Edit History.

Basic data types and procedures needed both by clients of the file system
and by low-level disk device implementations. This is logically part of
filesystem.h, but is separate to avoid circularities.
*/

#ifndef FILETYPES_H
#define FILETYPES_H

typedef struct _t_DiskRec *Disk;
/* A Disk is a pointer to a data structure representing a particular
   file system. Its actual representation is defined in disk.h.
   Most clients do not need to know anything about a Disk and can
   treat it as an "opaque type" (however, see DiskAttributes). */

typedef long int FileID;	/* file ID, unique within file system */
typedef long int DiskAddress;	/* linear (virtual) disk address */

typedef struct {		/* File designator for most operations */
  Disk disk;			/* -> file system */
  FileID id;			/* unique ID within file system */
  DiskAddress addr;		/* address of leader page 0 */
  } FilePointer;

typedef long int FilePage;	/* page number within file */
typedef unsigned long int FileTime; /* file time stamp */

#define PAGESIZE 1024		/* bytes per file page */
#define logPAGESIZE 10
#define MAXNAMESIZE 100		/* maximum length of file name string,
				   including all directory prefixes */
#define MAXFILEPAGE 0x40000000	/* max data pages in file (2**30) */


/* Error handling */

/* There are two classes of exception. Those that represent negative
   responses to a well-formed query (e.g., DirFind for a nonexistent
   file name) are indicated by a return value. But those that represent
   a calling error (e.g., bad file ID, nonexistent page, etc.) or an
   unexpected failure (hardware error, disk full, etc.) cause an exception
   to be raised which some caller must catch. This is done because certain
   errors need to propagate through many levels of abstraction, and
   explicitly checking for errors at each level would be tedious.

   The following are all error codes that accompany exceptions raised
   by procedures defined in file.h and disk.h. */

typedef enum {
  ecOK = 0,			/* (never raised as an exception) */
  ecDataError,			/* problem with data read from disk */
  ecNotReady,			/* disk drive not ready */
  ecDeviceError,		/* can't perform disk op (e.g., seek error) */
  ecWriteProtected,		/* can't write on read-only device */
  ecWrongFileID,		/* FileID mismatch in leader page or label */
  ecFileMalformed,		/* file data structures scrambled */
  ecFileSystemFull,		/* can't allocate any more disk space */
  ecNonexistentFilePage,	/* read/write beyond last page of file */
  ecIllegalSize,		/* size argument not in [0, MAXFILEPAGE] */
  ecTooFragmented,		/* too many runs in file */
  ecPageLocked,			/* can't remove locked page from cache */
  ecPageNotLocked,		/* tried to unlock page that wasn't locked */
  ecCacheFull,			/* completely occupied by locked pages */
  ecCacheMalfunction,		/* impossible error in cache */
  ecNameNotFound,		/* directory lookup failed */
  ecNameAlreadyExists,		/* attempted to insert duplicate name */
  ecNameTooLong,		/* name longer than MAXNAMESIZE */
  ecNameMalformed,		/* name syntax error */
  ecDirectoryMalformed,		/* directory data structures scrambled */
  ecIllegalDirName,		/* directory name too long or lacks '/' */
  ecRenameImpossible,		/* can't rename across devices */
  ecInsufficientMemory,		/* out of memory during initialization */
  ecTooManyBadPages,		/* excessive bad pages found during format */
  ecNotInitialized,		/* disk apparently hasn't been initialized */
  ecBug				/* software problem */
  } FileErrorCode;



void FileError(/* FileErrorCode ec */);
/* Simply raises the exception ec */

Disk DiskFind(/* char *name; int opened */);
/* Returns a pointer to the Disk object having the specified name, or
   NULL if the name is not found. The name must begin with "/" and
   ends at the next "/" (following characters are ignored). If opened
   is nonzero then DiskFind will consider only those Disks that have
   been made available for normal file access (by DiskOpen or
   DiskInitialize); if zero, all Disks will be considered. */

#endif FILETYPES_H
/* v003 durham Sun Feb 7 11:47:57 PST 1988 */
/* v004 durham Mon May 16 16:00:36 PDT 1988 */
/* v005 durham Sun Aug 14 14:30:31 PDT 1988 */
