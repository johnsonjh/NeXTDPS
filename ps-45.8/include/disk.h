/*
  disk.h

Copyright (c) 1985, '86, '87, '88 Adobe Systems Incorporated.
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
Ed Taft: Sat Dec  9 16:02:53 1989
Ivor Durham: Thu Aug 11 08:53:23 1988
Jeff Peters: Tue Dec 16 10:07:21 1986
End Edit History.

Disk data structures and procedures for the Disk IO level of
the file system.
*/

#ifndef DISK_H
#define DISK_H

#include FILETYPES
#include PUBLICTYPES

/* Disk data structures */

typedef unsigned short int LdrOffset; /* offset from base of leader page */

typedef struct _t_LdrHeader {	/* fixed header for leader page */
  long int seal;		/* constant LEADERSEAL */
  long int checksum;		/* checksum over entire leader page */
  LdrOffset endFixed;		/* end of fixed portion of header */
  LdrOffset runTable;		/* start of RunTable */
  DiskAddress badAddr;		/* address of known bad page if nonzero */
  FileID id;
  FileTime createTime, updateTime, accessTime;
  long int byteLength;
  LdrOffset name;		/* where name string is stored */
  } LdrHeader;

typedef struct _t_RunEntry {	/* run table entry */
  FilePage page;		/* file page number of start of run */
  DiskAddress addr;		/* disk address of start of run */
  } RunEntry;

typedef struct _t_RunTable {	/* run table */
  unsigned short int count;	/* number of runs in table */
  unsigned short int nLeaderPages; /* number of leader pages */
  RunEntry runs[3];		/* start of array of count RunEntries */
  /* The FilePage for the first run is the "page number" of the leader page
     itself, i.e., LEADERPAGE. If there is more than one leader page,
     this run may be more than one page long, and/or there may be more
     than one leader page run (negative FilePage). In any event, there is
     a "hole" in the file between the last leader page and data page 0; and
     data page 0 always starts a new run even if the leader page(s) and
     data page 0 are contiguous on the disk. */
  /* Immediately following the runs array is a FilePage giving the number of
     data pages in the file, or alternatively the page number of the first
     page not in the file. This looks like the FilePage portion of another
     RunEntry, and is required to determine the length of the last run. */
  } RunTable;

#define LEADERSEAL 0x1eade460	/* seal of leader page */
#define LEADERPAGE (FilePage)(-1000) /* file "page number" of leader page */

#define FIRSTFILEID 0x1000	/* small integers aren't legal FileIDs */
#define NROOTFILES 2		/* number of copies of root file */
/* FileIDs [FIRSTFILEID, FIRSTFILEID+NROOTFILES) are reserved for the
   root files */

typedef struct _t_SysRoot {	/* data page 0 of system root file */
  long int seal;		/* constant ROOTSEAL */
  long int checksum;		/* checksum over entire root page */
  FileTime updateTime;		/* time of last root file update */
  /* Note: in the root records written on the disk, the disk field of
     a FilePointer is irrelevant. It is of interest only in the root
     record that is part of the Disk object in memory. */
  FilePointer fpAllocMap;	/* disk allocation map file */
  FilePointer fpRootDirectory;	/* root directory file */
  DiskAddress rootAddrs[NROOTFILES]; /* disk addresses of root files */
  FileID reservedID;		/* limit of file IDs reserved by software */
  FileTime reservedTime;	/* limit of timestamps reserved by software */
  DiskAddress size;		/* total size of file system in pages */
  unsigned short int badOffset;	/* offset of bad page table */
  unsigned short int badCount;	/* count of bad pages */
  } SysRoot;

/* Bad page table is simply an array of DiskAddress, in no particular
   order but with no duplicates. */

#define ROOTSEAL 0x5fa87d27

typedef short int DEType;	/* directory entry type */
#define deFree 1		/* free entry */
#define deFreeEnd 2		/* free; no further non-free entries exist */
#define deFile 3		/* entry for (normal) file */
/* DEType is logically an enum, but there's no way to put one in a short
   field in the DirEntry without either the compiler or lint complaining. */

typedef struct _t_DirEntry {	/* directory entry */
  unsigned short int size;	/* length of entire entry in bytes */
  DEType type;			/* type of entry */
  /* remaining fields pertain to files of type deFile only */
  FileID id;
  DiskAddress addr;
  char name[MAXNAMESIZE];
  } DirEntry;

#define DEALIGN 4		/* align DirEntries to 4-byte boundaries */


/* Software data structures and operations */

typedef short int DiskCommand;
#define diskRead 0x7a90		/* magic numbers for disk commands */
#define diskWrite 0x7a91
				/* modifiers bits for DiskCommand */
#define fixedMemAddr 2		/* transfer every page to/from same buffer */
#define disableRecovery 4	/* disable error recovery */

typedef struct _t_DiskOp {	/* disk operation record */
  DiskAddress diskAddr;		/* initial linear disk address */
  char *memAddr;		/* initial memory address */
  FileID fileID;		/* FileID of file being accessed */
  FilePage page;		/* initial file page number */
  long int count;		/* count of pages to transfer */
  DiskCommand command;		/* what to do */
  FileErrorCode status;		/* ending status is stored here */
  long int priv[4];		/* implementation-dependent uses */
  } DiskOp;

#define MAXDIRSIZE 30		/* max length of directory name */

typedef struct _t_DiskRec {	/* The Disk object, representing an
				   active file system */
  SysRoot root;			/* copy of system root record */
  int initialized;		/* nonzero if file system has been created */
  int dataAlign;		/* data alignment required by controller
				  (1 = byte, 2 = word, 4 = long word, etc.) */
  DiskAddress physSize;		/* physical size of disk device */
  char name[MAXDIRSIZE];	/* name of this disk device */
  FileID allocID;		/* next available file ID */
  DiskAddress nextAllocAddr;	/* disk addr for next free-choice allocate */
  long int freePages;		/* number of free pages in file system */
  long int priv[4];		/* implementation-dependent uses */
  long int (*Allocate)();	/* allocator (normally AllocateDiskPages) */
  DiskOp errorOp;		/* copy of most recent DiskOp that failed */
  int disableCompact;		/* disable directory compaction if nonzero */

  /* device-dependent Disk operations */

  procedure (*Initiate)(/* Disk disk; DiskOp *op */);
  /* Initiates a disk operation as specified in the diskAddr, memAddr,
     page, count, and command fields of the DiskOp record. No failures are
     reported by this procedure. */

  DiskOp *(*Poll)(/* Disk disk */);
  /* Returns a previously initiated disk operation that has completed,
     or NULL if no operation has completed yet. The status field of the
     returned operation record reports the outcome. If it is ecOK,
     the diskAddr, memAddr, and page fields are updated by the amount of the
     transfer and the count field is zero. If an error occurred, the
     diskAddr and memAddr fields designate the page at which the error
     occurred, and the count field specifies the number of remaining
     pages, including the one in error. */

  int (*Format)(
    /* Disk disk; DiskAddress addr; long int count; (*BadPage)() */);
  /* Writes device-dependent formatting information to the interval
     [addr, addr+count) on disk. If bad pages can be detected during
     this procedure, it calls BadPage(disk, addr) where addr is the
     address of the bad page. */

  procedure (*PrintError)(/* Disk disk; Stm stm */);
  /* Prints to stm a human-readable description of any device-dependent
     error information contained in disk->errorOp. */

  int (*Park)(/* Disk disk */);
  /* Stops drive motor and parks heads on drives where this is possible.
     This is a device-dependent function which may do nothing for some
     unit types. */

  } DiskRec;


/* Disk operations common to all devices */

extern procedure DiskUtilStart();
/* Initializes the common disk-level facilities. */

extern Disk DiskStart();
/* Called repeatedly at init time to obtain all the Disks that
   the io package supports in the current configuration.
   Each Disk returned has been registered with DiskAdd already. */

extern procedure DiskAdd(/* Disk disk */);
/* Adds disk to the list of disks known to the software. At the time
   of the call, the following fields must already be filled in:
   name, physSize, dataAlign, Initiate, Poll, Format, and Scan. */

extern procedure DiskRemove(/* Disk disk */);
/* Remove disk from the list of disks known to the software. */

#endif DISK_H
