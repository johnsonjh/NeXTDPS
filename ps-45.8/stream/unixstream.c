/*
  unixstream.c

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Thu Mar 22 14:24:35 1984
Edit History:
Ed Taft: Mon May  2 17:13:22 1988
John Gaffney: Wed Oct  9 15:47:33 1985
Peter Hibbard: Tue Dec  3 12:53:45 1985
Chuck Geschke: Tue Dec 31 14:40:21 1985
Ivor Durham: Sun Jul 17 18:53:28 1988
Jim Sandman: Wed Mar 16 16:40:57 1988
Paul Rovner: Monday, June 6, 1988 3:53:40 PM
End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include ENVIRONMENT
#include PSLIB
#include STREAM
#include "unixstmpriv.h"

/* UNIXSYS controls the availability of Unix system-specific .h files
   and the special system calls fstat and ioctl. */
#ifndef UNIXSYS
#define UNIXSYS (OS==os_bsd || OS==os_sun || OS==os_ultrix || OS==os_aix || \
	OS==os_xenix || OS == os_mach)
#endif UNIXSYS

#if UNIXSYS
#include <sgtty.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif UNIXSYS

extern integer lseek();
extern integer read();
extern integer write();
extern integer close();

#if	(OS == os_mpw)
extern integer DPSCall3Sanitized();
#endif	(OS == os_mpw)

public procedure UnixStmInit()
{
  os_stdin = os_fdopen(0, "r");
  os_stdout = os_fdopen(1, "w");
  os_stderr = os_fdopen(2, "w");
  GetPUnixData(os_stderr)->noBuffer = 1;
}


/* Unix class implementations of the stream operations. Note that these
 * are now all lumped together; there is no point in keeping them as
 * separate files since they are always referenced (from unixStmProcs). */

private int UFilBuf(stm)
  register Stm stm;
{
  char ch;
  char *base;
#if UNIXSYS
  struct stat stbuf;
#endif UNIXSYS
  register UnixData *data = GetPUnixData(stm);
  if (stm->flags.readWrite) stm->flags.read = 1;
  if (stm->flags.eof || !stm->flags.read) return EOF;
  base = stm->base;
tryagain:
  if (base == NULL)
    {
    if (data->noBuffer)
      {base = &ch; goto tryagain;}
    if (data->bufsiz == 0)
      {
#if UNIXSYS
      stbuf.st_blksize = 0;	/* in case host doesn't set it */
      if (fstat(fileno(stm), &stbuf) < 0 || stbuf.st_blksize <= 0)
        data->bufsiz = BUFSIZ;
      else data->bufsiz = stbuf.st_blksize;
#else UNIXSYS
      data->bufsiz = BUFSIZ;
#endif UNIXSYS
      }
    if ((stm->base = base = os_malloc(data->bufsiz)) == NULL)
      {data->noBuffer = 1; goto tryagain;}
    data->myBuffer = 1;
    }
  if (stm == os_stdin)
    {
    if (GetPUnixData(os_stdout)->lineBuffer) fflush(os_stdout);
    if (GetPUnixData(os_stderr)->lineBuffer) fflush(os_stderr);
    }

#if	(OS == os_mpw)
  stm->cnt = DPSCall3Sanitized(read, data->file, base, data->noBuffer? 1 : data->bufsiz);
#else	(OS == os_mpw)
  stm->cnt = read(data->file, base, data->noBuffer? 1 : data->bufsiz);
#endif	(OS == os_mpw)

  stm->ptr = base;
  if (--stm->cnt < 0)
    {
    if (stm->cnt == -1)
      {
      stm->flags.eof = 1;
      if (stm->flags.readWrite) stm->flags.read = 0;
      }
    else stm->flags.error = 1;
    stm->cnt = 0;
    return -1;
    }
  return (unsigned char)*stm->ptr++;
}

private int UFlsBuf(ch, stm)
  register int ch;
  register Stm stm;
{
  register int n, rn;
#if UNIXSYS
  struct stat stbuf;
#endif UNIXSYS
  UnixData *data = GetPUnixData(stm);
  if (stm->flags.readWrite)
    {
    stm->flags.write = 1;
    stm->flags.eof = 0;
    }
  if (!stm->flags.write) return EOF;
tryagain:
  if (data->lineBuffer)
    {
    *stm->ptr++ = ch;
    if (stm->ptr >= stm->base + data->bufsiz || ch == '\n')
      {
#if	(OS == os_mpw)
      n = DPSCall3Sanitized(write, data->file, stm->base, (rn = stm->ptr - stm->base));
#else	(OS == os_mpw)
      n = write(data->file, stm->base, (rn = stm->ptr - stm->base));
#endif	(OS == os_mpw)
      stm->ptr = stm->base;
      }
    else rn = n = 0;
    stm->cnt = 0;
    }
  else if (data->noBuffer)
    {
    char ch1 = ch;
    rn = 1;

#if	(OS == os_mpw)
    n = DPSCall3Sanitized(write, data->file, &ch1, rn);
#else	(OS == os_mpw)
    n = write(data->file, &ch1, rn);
#endif	(OS == os_mpw)

    stm->cnt = 0;
    }
  else
    {
    if (stm->base == NULL)
      {
      if (data->bufsiz == 0)
        {
#if	UNIXSYS
        stbuf.st_blksize = 0;	/* in case host doesn't set it */
        if (fstat(fileno(stm), &stbuf) < 0 || stbuf.st_blksize <= 0)
          data->bufsiz = BUFSIZ;
        else data->bufsiz = stbuf.st_blksize;
#else	UNIXSYS
        data->bufsiz = BUFSIZ;
#endif	UNIXSYS
	}
      if ((stm->base = os_malloc(data->bufsiz)) == NULL)
        {data->noBuffer = 1; goto tryagain;}
      stm->ptr = stm->base;
      data->myBuffer = 1;
      data->lineBuffer = stm == os_stdout && isatty(data->file);
      goto tryagain;
      }
    else if ((rn = n = stm->ptr - stm->base) > 0) {
#if	(OS == os_mpw)
      n = DPSCall3Sanitized(write, data->file, stm->base, n);
#else	(OS == os_mpw)
      n = write(data->file, stm->base, n);
#endif	(OS == os_mpw)
	  }
    stm->ptr = stm->base;
    stm->cnt = data->bufsiz-1;
    *stm->ptr++ = ch;
    }

  if (rn != n)
    {stm->flags.error = 1; return EOF;}
  return ch;
}

private int UFFlush(stm)
  register Stm stm;
{
  register int n, m;
  register UnixData *data = GetPUnixData(stm);
  if (stm->flags.write && !data->noBuffer &&
      stm->base != NULL && (n = stm->ptr - stm->base) > 0)
    {
#if	(OS == os_mpw)
    m = DPSCall3Sanitized(write, data->file, (stm->ptr = stm->base), n);
#else	(OS == os_mpw)
    m = write(data->file, (stm->ptr = stm->base), n);
#endif	(OS == os_mpw)

    if (m != n)
      {stm->flags.error = 1; return EOF;}
    stm->cnt = data->lineBuffer || data->noBuffer? 0 : BUFSIZ;
    }
  else if (stm->flags.read && !stm->flags.readWrite)
    {

#if	(OS == os_mpw)
    (void) DPSCall3Sanitized(lseek, data->file, 0L, 2);
#else	(OS == os_mpw)
    lseek(data->file, 0L, 2);
#endif	(OS == os_mpw)
    while (!(UFilBuf(stm) == EOF && feof(stm))) stm->flags.error = 0;
    stm->cnt = 0;
    }
  return 0;
}

private int UFClose(stm)
  register Stm stm;
{
  register int r = EOF;
  if (stm->flags.read || stm->flags.write || stm->flags.readWrite)
    {
    integer n;
	r = 0;
    if (stm->flags.write || stm->flags.readWrite) r = fflush(stm);
#if	(OS == os_mpw)
    {
	extern integer DPSCall1Sanitized();
    n = DPSCall1Sanitized(close, fileno(stm));
	}
#else	(OS == os_mpw)
    n = close(fileno(stm));
#endif	(OS == os_mpw)

    if (n < 0) r = EOF;
    if (GetPUnixData(stm)->myBuffer) os_free(stm->base);
    }
  StmDestroy(stm);
  return r;
}

private int UFAvail(stm)
  register Stm stm;
{
  int n;
  if (stm->cnt > 0) return stm->cnt;
#if UNIXSYS
  if (stm->flags.eof || ioctl(fileno(stm), FIONREAD, &n) != 0)
#endif UNIXSYS
    n = -1;
  return n;
}

private int UFPutEOF(stm)
  Stm stm;
{
  return (stm->flags.write)? UFFlush(stm) : EOF;
}

private int UFSeek(stm, offset, origin)
  Stm stm; long int offset; int origin;
{
  register int resync, c;
  register UnixData *data = GetPUnixData(stm);
  long int p;
  stm->flags.eof = 0;
  if (stm->flags.read)
    {
    if (origin < 2 && stm->base != 0 && !data->noBuffer)
      {
      c = stm->cnt;
      p = offset;
      if (origin == 0) {
        integer n;

#if	(OS == os_mpw)
        n = DPSCall3Sanitized(lseek, data->file, 0L, 1);
#else	(OS == os_mpw)
        n = lseek(data->file, 0L, 1);
#endif	(OS == os_mpw)

	    p += c - n;
		}
      else offset -= c;
      if (!stm->flags.readWrite && c > 0 && p <= c &&
          p >= stm->base - stm->ptr)
	{
        stm->ptr += p;
        stm->cnt -= p;
        return 0;
        }
      /* Position to a natural addressing boundary in the file so that
         when the file is read into the stream buffer the bytes are
	 in sync; this makes the read operation go much faster. */
      resync = offset & (PREFERREDALIGN - 1);
      }
    else resync = 0;
    if (stm->flags.readWrite)
      {
      stm->ptr = stm->base;
      stm->flags.read = 0;
      }

#if	(OS == os_mpw)
    p = DPSCall3Sanitized(lseek, data->file, offset - resync, origin);
#else	(OS == os_mpw)
    p = lseek(data->file, offset - resync, origin);
#endif	(OS == os_mpw)

    stm->cnt = 0;
    while (--resync >= 0) getc(stm);
    }
  else if (stm->flags.write || stm->flags.readWrite)
    {
    fflush(stm);
    if (stm->flags.readWrite)
      {
      stm->cnt = 0;
      stm->flags.write = 0;
      stm->ptr = stm->base;
      }

#if	(OS == os_mpw)
    p = DPSCall3Sanitized(lseek, data->file, offset, origin);
#else	(OS == os_mpw)
    p = lseek(data->file, offset, origin);
#endif	(OS == os_mpw)

    }
  return (p < 0)? EOF : 0;
}

private long int UFTell(stm)
  register Stm stm;
{
  long int r;
  int adjust;
  if (stm->cnt < 0) stm->cnt = 0;
  if (stm->flags.read) adjust = -stm->cnt;
  else if (stm->flags.write || stm->flags.readWrite)
    {
    adjust = 0;
    if (stm->flags.write && stm->base != 0 &&
        ! GetPUnixData(stm)->noBuffer)
      adjust = stm->ptr - stm->base;
    }
  else return -1;

#if	(OS == os_mpw)
  r = DPSCall3Sanitized(lseek, fileno(stm), 0L, 1);
#else	(OS == os_mpw)
  r = lseek(fileno(stm), 0L, 1);
#endif	(OS == os_mpw)

  if (r >= 0) r += adjust;
  return r;
}

public readonly StmProcs unixStmProcs = {
  UFilBuf, UFlsBuf, StmFRead, StmFWrite, StmUnGetc, UFFlush,
  UFClose, UFAvail, StmErr, UFPutEOF, UFSeek, UFTell,
  "Unix"};



