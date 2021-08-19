/*
  stodevedit.c

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

Original version: Chuck Geschke: February 13, 1983
Edit History:
Chuck Geschke: Tue Dec 31 16:22:21 1985
Tom Boynton: Fri Mar 18 10:18:56 1983
Doug Brotz: Fri Aug 29 16:15:49 1986
Ed Taft: Tue Aug 15 13:57:51 1989
Dick Sweet: Mon Oct 20 22:27:15 PDT 1986
Bill Paxton: Mon Jan 14 14:05:33 1985
Don Andrews: Tue Apr 7 11:20:44 PST 1987
Bill McCoy: Mon Mar 16 18:21:42 PST 1987
John Nash: Tue Apr 21 10:04:21 PDT 1987
Ivor Durham: Sun Oct 30 16:52:21 1988
Leo Hourvitz 25May87
Linda Gass: Thu Aug  6 09:27:57 1987
Joe Pasqua: Thu Jan  5 11:48:40 1989
Paul Rovner: Wednesday, October 7, 1987 6:12:56 PM
Jim Sandman: Wed Mar  9 17:02:28 1988
Perry Caro: Mon Nov  7 16:44:40 1988
End Edit History.

Implementation of "%lineedit" and "%statementedit" files and the "echo"
operator. This is optional and is included by calling StoDevEditInit.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include LANGUAGE
#include PSLIB
#include STODEV
#include STREAM
#include VM

#include "scanner.h"
#include "streampriv.h"
#include "langdata.h"

/* Interactive line editor */

#define NBUFCHARS (2*sizeof(char *) + sizeof(integer))

typedef struct _BufItem {
  struct _BufItem *prev, *next;
  integer count;
  char chars[NBUFCHARS];
  } BufItem;

typedef struct {BufItem *firstBuf, *curBuf;} LSData;

private procedure Rewind(stm)
  Stm stm;
  {
  LSData *data = (LSData *)(&stm->data);
  data->curBuf = data->firstBuf;
  stm->base = stm->ptr = &data->curBuf->chars[0];
  stm->cnt = data->curBuf->count;
  }

private readonly character eraseStr[] = "\010 \010";
private procedure Erase(c, out)
  character c;
  Stm out;
  {
  if (echo)
    {
    if (c<' ' && c!='\n' && c!='\t') os_fputs((char *)eraseStr, out);
    os_fputs((char *)eraseStr, out);
    }
  }

private procedure Truncate(stm)
  Stm stm;
  {
  LSData *data = (LSData *)(&stm->data);
  BufItem *bufItem;
  while ((bufItem = data->curBuf->next) != NIL) {
    data->curBuf->next = bufItem->next; FREE(bufItem);}
  data->curBuf->count = stm->ptr - stm->base;
  stm->cnt = 0;
  }

private int LineFilBuf(stm)
  Stm stm;
  {
  LSData *data = (LSData *)(&stm->data);
  BufItem *curBuf;
  if ((curBuf = data->curBuf->next) == NIL) return EOF;
  data->curBuf = curBuf;
  stm->ptr = stm->base = &curBuf->chars[0];
  stm->cnt = curBuf->count;
  return getc(stm);
  }

/*ARGSUSED*/
private int LineUnGetc(ch, stm)
  int ch;
  Stm stm;
  {
  LSData *data = (LSData *)(&stm->data);
  BufItem *curBuf;
  if (stm->ptr == stm->base) {
    if ((curBuf = data->curBuf->prev) == NIL) return EOF;
    data->curBuf = curBuf;
    stm->ptr = (stm->base = &curBuf->chars[0]) + NBUFCHARS;
    stm->cnt = 0;
    }
  stm->cnt++;
  return *--stm->ptr;
  }

private int LineFFlush(stm)
  Stm stm;
  {
  while (LineFilBuf(stm) != EOF) ;
  stm->cnt = 0;
  }

private int LineFClose(stm)
  Stm stm;
  {
  LSData *data = (LSData *)(&stm->data);
  BufItem *bufItem;
  while ((bufItem = data->firstBuf) != NIL) {
    data->firstBuf = bufItem->next; FREE(bufItem);}
  StmDestroy(stm);
  return 0;
  }

private int LineFAvail(stm)
  Stm stm;
  {
  LSData *data = (LSData *)(&stm->data);
  BufItem *buf;
  integer n;
  if (stm->flags.eof) return EOF;
  n = stm->cnt;
  for (buf = data->curBuf->next; buf != NIL; buf = buf->next)
    n += buf->count;
  return n;
  }

private readonly StmProcs lineStmProcs = {
  LineFilBuf, StmErr, StmFRead, StmZeroLong, LineUnGetc, LineFFlush,
  LineFClose, LineFAvail, LineFFlush, StmErr, StmErr, StmErrLong,
  "LineEdit"};

/*ARGSUSED*/
private Stm LineEdit(in, out, stmt)
  Stm in, out;
  boolean stmt;
  /* Interactively reads an edited line, obtaining input from in and writing
   * any user feedback to out. If stmt is true, reads one or more edited
   * lines comprising a complete PostScript statement. When a line or
   * statement has been read, creates and returns a new input stream
   * that will dispense the characters of the line(s) followed by EOF.
   * Returns NIL if in has encountered EOF or error at the time of the call.
   * PS_INTERRUPT can be raised during interactive
   * input (but not during the reading of the edited line). */
  {
  integer c;
  StmObj so;
  Stm stm;
  LSData *data;
#if OS==os_ps
  integer timeout;
#endif OS==os_ps
  if ((stm = StmCreate(&lineStmProcs, (integer)0)) == NULL) LimitCheck();
  stm->flags.read = true;
  data = (LSData *)(&stm->data);
#if OS==os_ps
  timeout = alarm((integer)0);
#endif OS==os_ps
  DURING
    data->firstBuf = (BufItem *) NEW(1, sizeof(BufItem));
    data->firstBuf->prev = data->firstBuf->next = NIL;
    data->firstBuf->count = 0;
    Rewind(stm);

    while (true) {
      if (echo) fflush(out);
#if OS==os_ps
      alarm(timeout);
#endif OS==os_ps
      c = getc(in);
      switch (c) {
        case EOF:
	  if (ferror(in)) StreamError(in);
	  if (stm->ptr == stm->base && data->curBuf == data->firstBuf)
            {
            MakePStm(in, Lobj, &so);
	    CloseFile(so, false);
	    fclose(stm); stm = 0;
	    goto ret;
            }
          goto done;
        case '\025': {		/* control-U */
	  while ((c = ungetc(c, stm)) != EOF) {
	    if (c == '\n') {(void)os_fgetc(stm); break;}
            Erase((character)c, out);
	    }
          Truncate(stm);
	  break;
	  }
        case '\010': case '\177': /* backspace, del */
          if ((c = ungetc(c, stm)) == EOF)
	    {
	    if (echo) os_fputc('\007', out);
	    break;
	    }
	  Truncate(stm);
	  if (c != '\n') {Erase((character)c, out); break;}
          /* backspaced over newline; fall through to retype previous line */
        case '\022': {		/* control-R */
	  Truncate(stm);
	  while ((c = ungetc(c, stm)) != EOF)
	    if (c == '\n') {(void)os_fgetc(stm); break;}
          if (echo) os_fputc('\n', out);
          while ((c = os_fgetc(stm)) != EOF)
	    if (echo)
	      if (c<' ' && c!='\n' && c!='\t') {
	        os_fputc('^', out); os_fputc((int)(c+'\100'), out);}
	      else os_fputc((int)c, out);
	  break;
	  }
        case '\r':
          c = '\n';
          /* fall through */
        default:
          *stm->ptr++ = c;
          if ((data->curBuf->count = stm->ptr - stm->base) == NBUFCHARS) {
	    BufItem *newBuf = (BufItem *) NEW(1, sizeof(BufItem));
	    data->curBuf->next = newBuf; newBuf->prev = data->curBuf;
	    data->curBuf = newBuf;
	    newBuf->count = 0;
	    stm->base = stm->ptr = &newBuf->chars[0];
	    }
          if (echo)
	    if (c<' ' && c!='\n' && c!='\t') {
	      os_fputc('^', out); os_fputc((int)(c+'\100'), out);}
	    else os_fputc((int)c, out);
          if (c == '\n') {
	    if (!stmt) goto done;
	    Rewind(stm);
	    if (LineComplete(stm)) goto done;
	    /* assert: stm now at EOF, since LineComplete must have read
	     * all of it in order to say that the line isn't complete */
	    }
        }
    }
  done:
    if (echo) fflush(out);
    Rewind(stm);
  ret:
  HANDLER {
    fclose(stm);
    RERAISE;
    }
  END_HANDLER;
  return stm;
  }


private procedure PSEcho(b)
{
  boolean newEcho = PopBoolean();
  WriteContextParam(
    (char *)&echo, (char *)&newEcho, (integer)sizeof(echo), (PVoidProc)NIL);
}


private Stm EStmCreate(dev, name, access)
  PStoDev dev; char *name, *access;
{
  if (os_strcmp(access, "r") != 0) RAISE(ecInvalidFileAccess, (char *)NIL);
  return LineEdit(os_stdin, os_stdout, (boolean)dev->private1);
}


private procedure EDevAttr(dev, attr)
  PStoDev dev; StoDevAttributes *attr;
{
  attr->size = 0;
  attr->freePages = 0;
}


private boolean EBNop(dev)
  PStoDev dev;
{
  return false;
}


private procedure EUndef(dev)
  PStoDev dev;
{
  RAISE(ecUndef, (char *)NIL);
}


private int EIUndef(dev)
  PStoDev dev;
{
  RAISE(ecUndef, (char *)NIL);
}


private readonly StoDevProcs Eprocs =
  {
  EBNop,		/* FindFile */
  EStmCreate,		/* CreateDevStm */
  EUndef,		/* Delete */
  EUndef,		/* Rename */
  EUndef,		/* ReadAttributes */
  EIUndef,		/* Enumerate */
  EDevAttr,		/* DevAttributes */
  EUndef,		/* Format */
  EUndef,		/* Mount */
  EUndef		/* Dismount */
  };


private procedure RgstEditorDev(name, which)
  char *name; boolean which;
{
  PStoDev dev = (PStoDev) os_sureCalloc((integer)1, (integer)sizeof(StoDev));
  dev->procs = &Eprocs;
  dev->removable = false;
  dev->hasNames = false;
  dev->writeable = false;
  dev->searchable = false;
  dev->mounted = true;
  dev->name = name;
  dev->private1 = which;
  RgstStoDevice(dev);
}


public procedure StoDevEditInit(reason)
  InitReason reason;
{
  PStoDev dev;
  switch (reason)
    {
    case init:
      RgstEditorDev("lineedit", false);
      RgstEditorDev("statementedit", true);
      break;
    case romreg:
      RgstExplicit("echo", PSEcho);
      break;
    }
}
