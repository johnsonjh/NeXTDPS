/*
  filesystem.h

Copyright (c) 1985, '86, '87, '88, '89 Adobe Systems Incorporated.
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
Ed Taft: Sun Dec  3 15:59:20 1989
Ivor Durham: Thu Aug 11 08:53:20 1988
End Edit History.

Public data types and procedures needed by clients of the file system

All procedures in this interface acquire and release the foreground
monitor during their execution so as to protect against concurrent
access, except as noted otherwise.
*/

#ifndef	FILESYSTEM_H
#define	FILESYSTEM_H

#include FILETYPES
#include STREAM	

/* Data structures */

/* The following types are defined in filetypes.h:

     Disk, DiskAddress,
     FileErrorCode, FileID, FilePage, FilePointer, FileTime,
     MAXFILEPAGE, MAXNAMESIZE, PAGESIZE, logPAGESIZE

   Also, the following procedures are defined in filetypes.h for convenience:

     DiskFind, FileError
*/

typedef struct _t_FileAttributes {/* file attributes record passed to and
				     returned from procedures */
  FileTime createTime;		/* time information in file was created */
  FileTime updateTime;		/* time file was last written */
  FileTime accessTime;		/* time file was last accessed (read|write) */
  FilePage size;		/* number of data pages in file */
  long int byteLength;		/* length of file in bytes */
  char *name;			/* -> file name (null-terminated C string
				   of at most MAXNAMESIZE characters) */
  } FileAttributes;

typedef enum {			/* transfer operation (File/Cache levels) */
  xopRead,			/* read from file/cache */
  xopWrite,			/* write to file/cache */
  xopWriteThrough,		/* write to cache; update file immediately
				   (cache ops only) */
  xopWriteBypass		/* write to file; do not invalidate cache
				   (file ops only) */
  } FileTransferOp;

typedef enum {			/* file access type (Stream level) */
  accRead,			/* read-only */
  accOverwrite,			/* writing new (contents of) file */
  accUpdate			/* updating existing file in-place */
  } FileAccess;

typedef struct _t_DiskAttributes {/* disk information (DiskGetAttributes) */
  int initialized;		/* if nonzero, available for normal file
				   access; if zero, "raw" access only */
  int dataAlign;		/* data alignment required by controller
				   (1 = byte, 2 = word, 4 = long word) */
  DiskAddress physSize;		/* physical size of disk device */
  /* remaining fields defined only if initialized is nonzero */
  DiskAddress size;		/* total size of file system on disk */
  long int freePages;		/* number of free pages in file system */
} DiskAttributes;


/* Disk File operations */

extern procedure FileCreate(
  /* FilePointer *fp; FilePage size; FileAttributes *attr */);
/* Creates a file on fp->disk containing size data pages and the specified
   initial attributes. The data pages initially have undefined contents.
   Fills in fp->id and fp->addr for the file that was created. If the
   supplied attr->createTime is zero, fills it in with the current time;
   unconditionally fills in attr->updateTime and attr->accessTime with
   the current time. */

extern procedure FileDelete(/* FilePointer *fp */);
/* Deletes the specified file. */

extern procedure FileChangeSize(/* FilePointer *fp; FilePage size */);
/* Changes the file's size to the specified new value by adding or removing
   pages at the end. Added pages have undefined contents. This operation
   updates the updateTime and accessTime attributes. */

extern procedure FileReadAttributes(
  /* FilePointer *fp; FileAttributes *attr */);
/* Reads the file's attributes into *attr. attr->name, if not NULL,
   must be a pointer to a string of length MAXNAMESIZE into which the
   file's name may be stored by this procedure. */

extern procedure FileWriteAttributes(
  /* FilePointer *fp; FileAttributes *attr */);
/* Writes the file's attributes from *attr. The size, updateTime, and
   accessTime fields are ignored (the file's size is changed only by
   FileChangeSize; the updateTime and accessTime are maintained
   automatically). The file's name is changed only if attr->name is
   not NULL. */

extern procedure FileTransferPages(
  /* FilePointer *fp; FilePage page; long int count; char *where;
     FileTransferOp xop */);
/* Transfers count data pages between the file starting at the specified page
   and memory starting at where. The xop must be one of:
     xopRead	transfer from file to memory, ignoring the cache.
     xopWrite	transfer from memory to file, invalidating any cached entries.
     xopWriteBypass  transfer from memory to file, ignoring the cache.
   The memory pointer must conform to the alignment requirement given by
   fp->disk->align. The file pages must exist, else an error will occur; that
   is, this procedure will not extend the file (whether reading or writing).
   This procedure updates the accessTime attribute and also the updateTime
   attribute if the operation is a write. */

extern procedure FileReadPages(
  /* FilePointer *fp; FilePage page; long int count; char *where */);
/* Equivalent to FileTransferPages with the same arguments and
   an xop argument of xopRead. */

extern procedure FileWritePages(
  /* FilePointer *fp; FilePage page; long int count; char *where */);
/* Equivalent to FileTransferPages with the same arguments and
   an xop argument of xopWrite. */


/* Directory operations */

/* Note: file names are always relative to a specific Disk; manangement
   of a name space spanning multiple Disks is the caller's responsibility.
   At present, there is only one directory per Disk, namely the
   disk's root directory.

   File names are stored in the directory as given, and file name
   matching is case-sensitive.
 */


extern int FileDirFind(/* Disk disk, char *name, FilePointer *fp */);
/* Looks up name and, if found, stores the associated value in *fp.
   Returns 1 if the file was found, 0 if not. */

extern procedure FileDirInsert(/* Disk disk, char *name, FilePointer *fp */);
/* Inserts *name into the directory and associates it with the
   value *fp. If *name already exists in the directory, an error results. */

extern procedure FileDirRemove(/* Disk disk, char *name */);
/* Removes *name and its associated value from the directory. */

extern int FileDirEnum(
   /* Disk disk,
      char *pattern,
      int (*match)(char *name; char *pattern),
      int (*action)(char *name; FilePointer *fp; char *arg),
      char *arg */);
/* Enumerates the files designated by pattern, i.e., all files whose
   names match that pattern.

   For each file enumerated, DirEnum first calls match(name, pattern),
   where name is the file's name and pattern is the "pattern" argument
   passed to DirEnum. If match returns a nonzero value, DirEnum then calls
   action(name, fp, arg), where name is the same as what was passed
   to the match procedure, fp is the file's FilePointer, and arg is
   the "arg" argument passed to DirEnum (which is uninterpreted and may
   be recast to any pointer type).

   The enumeration continues either until the directory is exhausted,
   in which case DirEnum returns zero, or until action returns a nonzero
   value, in which case DirEnum immediately returns that value.

   The reason for the separate match and action procedures is that
   match is not permitted to invoke any file system directory procedures
   whereas action may perform any operation. Additionally, FileDirEnum
   releases the foreground monitor before calling action and reacquires
   it afterward. The usual use of these procedures is that match serves
   as a filter on the file names and action performs some operation on
   every file that is accepted by the filter.

   A match procedure of NULL is equivalent to one that always returns
   a nonzero value (i.e., a filter that accepts every file in the directory).
   An action procedure of NULL is equivalent to one that performs no action
   and always returns zero (to continue the enumeration).

   The order of enumeration of entries in the directory is unspecified.
   If new entries are inserted into the directory during an enumeration,
   it is unspecified whether or not they will be encountered later in
   the same enumeration. */

extern procedure FileDirCreate(
  /* Disk disk, char *name, FilePage size,
     FileAttributes *attr, FilePointer *fp */);
/* Creates a file with the specified size and attributes and inserts it
   into the directory with the specified name. If attr==NULL then
   default attributes are used; and in any event attr->name is not used.
   Stores in *fp the FilePointer for the file that was created. */

extern procedure FileDirDelete(/* Disk disk, char *name */);
/* Removes the name from the directory and deletes the file. */

extern procedure FileDirRename(/* Disk disk, char *oldName, *newName */);
/* Renames the file *oldName to be *newName. There must be an existing
   file whose name is *oldName, and there must not be an existing file
   whose name is *newName, else an error will occur. It is not possible
   to move files from one Disk to another by this means. */


/* File Stream operations */

/* Note: the procedures defined here report errors by raising exceptions,
   the same as the other file system procedures. However, the stream
   operations (getc, putc, etc.) report errors by returning EOF, the
   same as they do for all kinds of streams. The FileErrorCode that
   describes the exception may be obtained by calling GetFileStmError.

   Note: the exported procedures acquire and release the foreground
   monitor as necessary during their execution. This protects access
   to the filesystem and shared data structures; however, it does NOT
   protect access to individual streams. In other words, it is illegal
   to attempt concurrent operations on a single stream, but concurrent
   operations on different streams are OK.
 */

extern Stm FileStmCreate(/* FilePointer *fp; FileAccess access */);
/* Creates a stream (Stm) for accessing the specified existing file.
   Sets the file's createTime and updateTime to "now" if access is
   accOverwrite or accUpdate; also sets the byteLength to 0 if accOverwrite.

   Standard stream operations may subsequently be performed (see stream.h);
   special semantics of these operations are as follows:

   getc (and other read operations): returns EOF if the current stream
     position is greater than or equal to the file's byteLength attribute.

   putc (and other write operations): returns EOF if access is accRead;
     extends the file if necessary.

   fclose: sets the file's byteLength attribute to the maximum of its
     former value and the greatest stream position reached.

   fseek: if the specified position is greater than the existing byteLength,
     returns EOF if access is accRead, otherwise extends the file.

   ?? should freset do anything special ??
 */

extern Stm FileDirStmOpen(
  /* Disk disk, char *name, FileAccess access, long int byteLength */);
/* Creates a stream (Stm) for accessing the file whose name is *name,
   as described under FileStmCreate. If no file by that name exists
   and access is accOverwrite or accUpdate, a new file is created
   and entered into the directory; if access is accRead, an error occurs.
   If a new file is created, byteLength is the number of bytes preallocated
   for it; this should be the expected eventual byteLength of the file.
   If the eventual byteLength is not known in advance, it is OK for
   byteLength to be zero; this will cause the file to be grown piecemeal. */

extern FileErrorCode GetFileStmError(/* Stm stm */);
/* Returns the reason for the most recent error encountered by Stm;
   ecOK if no errors have been encountered. */

extern procedure GetFilePointer(/* Stm stm; FilePointer *fp */);
/* Stores in *fp the FilePointer for the file associated with stm. */

extern int FileTruncate(/* Stm stm */);
/* Immediately sets the file's byteLength to the current stream
   position and deletes any pages past end-of-file. Returns 0
   normally, EOF if unsuccessful. */


/* Disk and file system startup and initialization operations */

extern procedure FileStart(/* int cacheCount */);
/* Starts the file system software, allocating cacheCount buffers of
   PAGESIZE each for use by the cache (cacheCount must be at least 5).
   This procedure merely initializes software data structures and does
   not access any disks.  It must be called before any other file
   system procedure. Does NOT acquire the foreground monitor.
 */

/* The device-dependent driver for each type of disk exports a Start
   procedure that returns a Disk object for accessing that disk
   and makes the Disk known to the rest of the file system.
   No Disks are available for use until at least one driver Start
   procedure has been called.

   The driver Start procedure may require device-dependent arguments,
   and it may fail because the device in question does not exist.
   The Disk so created may be used only to access the "raw" disk (via
   the Initiate and Poll procedures) until the DiskOpen or DiskInitialize
   procedure has been called. That is, the device in question need not
   have had a file structure written on it yet.
   
   The driver Start procedures are not documented here since they are
   different for each type of disk. Driver Start procedures do NOT
   acquire the foreground monitor.
 */

extern Disk DiskGetNext(/* Disk disk */);
/* Stateless enumerator for extant Disks, in the order that they
   were registered by driver Start procedures. If disk==NULL, returns
   the first registered Disk; otherwise returns the one following
   disk (NULL if disk is the last). The returned Disk is in an arbitrary
   state; it may or may not have been opened and/or initialized. */

extern procedure DiskOpen(/* Disk disk */);
/* Attempts to read vital data structures (root record, allocation map)
   from the disk and to set all variable fields of the Disk object, and
   makes the Disk available for normal file access. This will fail if
   the disk has not previously been Initialized. */

extern procedure DiskInitialize(/* Disk disk; DiskAddress size; int format */);
/* Creates a new file system on the specified disk.
   Size, if nonzero, is the number of pages to include in the file system;
   if zero, the entire capacity of the physical disk is used.
   If format is nonzero, the disk is formatted first (by calling the
   device-dependent Format procedure). If format is greater than 1,
   the disk surface is tested by writing and reading the entire disk
   that number of times; any bad pages encountered are recorded
   permanently. If initialization is completed
   successfully, the disk becomes available for normal file access,
   as if DiskOpen had been called. */

extern procedure DiskClose(/* Disk disk */);
/* Makes the Disk unavailable for normal file access, and makes it
   invisible to DiskFind when called with opened = 0. */

extern procedure DiskGetAttributes(/* Disk disk; DiskAttributes *attr */);
/* Stores current disk attributes into *attr. */

extern procedure DiskPrintError(/* Disk disk; Stm stm */);
/* Prints to stm human-readable information about the most recent
   error encountered on disk. This information is meaningful only
   for the FileErrors ecDeviceError and ecDataError. */

#endif	FILESYSTEM_H
/* v004 durham Sun Feb 7 13:12:06 PST 1988 */
/* v005 sandman Wed Mar 9 14:43:21 PST 1988 */
/* v006 durham Mon May 16 19:43:13 PDT 1988 */
/* v008 durham Thu Aug 11 13:08:53 PDT 1988 */
/* v009 durham Fri Nov 18 11:08:23 PST 1988 */
/* v010 pasqua Thu Dec 15 11:05:34 PST 1988 */
/* v011 byer Wed May 17 15:04:15 PDT 1989 */
/* v012 taft Thu Jun 29 09:56:29 PDT 1989 */
/* v013 sandman Mon Oct 16 17:03:22 PDT 1989 */
/* v014 taft Sun Dec 3 12:22:37 PST 1989 */
