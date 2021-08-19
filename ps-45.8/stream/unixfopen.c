/*
  unixfopen.c
CONFIDENTIAL
Copyright (c) 1988 NeXT, Inc.
Copyright (c) 1984, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version:
Edit History:
Ed Taft: Tue May  3 09:50:20 1988
Paul Rovner: Tuesday, June 7, 1988 6:05:34 PM
Ivor Durham: Sat Jun 25 19:17:20 1988
Leo Hourvitz: Wed Aug 31 23:23:00 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include PUBLICTYPES
#include STREAM
#include "unixstmpriv.h"

#if (OS == os_mach)
#undef MONITOR
#include <mach.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif (OS == os_mach)

#ifdef VAXC
#include errno
#else VAXC
#include <errno.h>
#include <cthreads.h>
#endif VAXC

extern procedure close();
extern integer open();
extern integer lseek();
extern integer creat();

extern readonly StmProcs unixStmProcs;
#if (OS == os_mach)
extern readonly StmProcs mappedStmProcs;
#endif (OS == os_mach)

#if (OS == os_mpw)

extern integer DPSCall1Sanitized();
extern integer DPSCall2Sanitized();
extern integer DPSCall3Sanitized();
#endif

#if (OS == os_mach)
/* Mapped file defines */
#define FileSize(stm) ((int)((stm)->data.b))
#define FileFd(stm) ((int)(stm->data.a))
#define MAPLOG 1
#if MAPLOG
extern procedure AddMappedFile(/*size*/);
extern procedure RemoveMappedFile(/*size*/);
#endif MAPLOG
#endif (OS == os_mach)

public Stm os_fopen(file, mode)
  char *file;
  register char *mode;
{
  register int fd,
          rw;
  register Stm stm;
  extern readonly StmProcs unixStmProcs;
  extern int IsContextWriteProhibited();

  rw = mode[1] == '+';

  /* hack security fix, if the context has been write protected and
   * they are trying to open a writeable file, silently give them
   * a descriptor in /dev/null.
   */
  if( (*mode != 'r' || rw) && IsContextWriteProhibited()) 
      file = "/dev/null"; 

  if (*file == 0) /* Have to specially protect against this, */
    return(NULL); /* open will go ahead and open the current directory */
  switch (*mode) {
   case 'w':
    fd = creat (file, 0666);

    if (rw && fd >= 0) {
      close (fd);
      fd = open (file, 2);
    }
    break;

   case 'a':
    fd = open (file, rw ? 2 : 1);

    if (fd < 0) {
      if (cthread_errno() == ENOENT) {
	fd = creat (file, 0666);

	if (rw && fd >= 0) {
	  close (fd);
	  fd = open (file, 2);
	}
      }
    }
    if (fd >= 0) {
      lseek (fd, 0L, 2);
    }
    break;

   case 'r':
    fd = open (file, rw ? 2 : 0);
    break;

   default:
    fd = -1;
    break;
  }

  if (fd < 0) {
    return NULL;
  }
#if (OS == os_mach)
  return(os_fdopen(fd,mode));
#else (OS == os_mach)
  if ((stm = StmCreate (&unixStmProcs, 0)) == NULL)
    return NULL;
  fileno (stm) = fd;
  if (rw)
    stm->flags.readWrite = 1;
  else if (*mode != 'r')
    stm->flags.write = 1;
  else
    stm->flags.read = 1;

  /* Terminal streams are serially reusable; others are not */
  stm->flags.reusable = isatty (fd);

  /*
   * Assume that files other than terminals are positionable; this is not
   * correct for pipes and such.
   */
  stm->flags.positionable = !stm->flags.reusable;

  return stm;
#endif (OS == os_mach)
}

public Stm os_fdopen(fd, mode)
  int fd;
  char *mode;
{
  Stm stm;
  extern readonly StmProcs unixStmProcs;
#if (OS == os_mach)
  extern readonly StmProcs mappedStmProcs;
  struct stat statBuf;
  /* Decide whether or not to create mapped file */
  if ((mode[0] != 'r')||		/* mapped files are read-only */
      (mode[1] != 0) ||			/* mapped can't have second char */
      (fstat(fd,&statBuf) == -1)||	/* they have to be stat-able */
      ((statBuf.st_mode&S_IFMT) != S_IFREG)) /* & have to be regular files */
  {
#endif (OS == os_mach)
  if ((stm = StmCreate(&unixStmProcs, 0)) != NULL)
    {
    fileno(stm) = fd;
    if (*mode == 'r') stm->flags.read = 1;
    else
      {
      stm->flags.write = 1;
      if (*mode == 'a') {
	    lseek(fd, 0L, 2);
		}
      }
    if (mode[1] == '+')
      {
      stm->flags.read = stm->flags.write = 0;
      stm->flags.readWrite = 1;
      }
    stm->flags.reusable = isatty(fd);
    stm->flags.positionable = !stm->flags.reusable;
    }
#if (OS == os_mach)
  }
  else
  { /* Create a mapped file! */
  if ((stm = StmCreate (&mappedStmProcs, 0)) == NULL)
    return NULL;
  if (statBuf.st_size != 0) { /* can't map 0-length but do want it open! */
    if (KERN_SUCCESS !=
      map_fd(fd,0,(vm_offset_t)&stm->base,TRUE,statBuf.st_size))
    {
      StmDestroy(stm);
      return NULL;
    }
#if MAPLOG
    AddMappedFile(statBuf.st_size);
#endif MAPLOG
  }
  FileFd(stm) = fd;
  stm->cnt = FileSize(stm) = statBuf.st_size;
  stm->ptr = stm->base;
  stm->flags.read = stm->flags.positionable = 1;
  stm->flags.reusable = 0;
  }
#endif (OS == os_mach)
  return stm;
}

public int os_fileno(stm)
  Stm stm;
{
#if (OS == os_mach)
  return((stm->procs == &unixStmProcs) ? fileno(stm) : FileFd(stm) );
#else (OS == os_mach)
  return fileno(stm);
#endif (OS == os_mach)
}



