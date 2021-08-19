/*
  streamimpl.c

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
Ed Taft: Tue May  3 09:59:29 1988
Paul Rovner: Thursday, October 8, 1987 9:49:10 PM
Ivor Durham: Sat Feb 13 14:52:46 1988
Jim Sandman: Fri Mar 11 10:51:26 1988
Perry Caro: Thu Nov  3 16:35:26 1988
Paul Rovner: Wednesday, November 30, 1988 6:44:00 PM
Joe Pasqua: Thu Dec 15 13:14:50 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include PSLIB
#include PUBLICTYPES
#include STREAM

public Stm os_stdin, os_stdout, os_stderr;

public int StmErr() {return EOF;}
public long int StmErrLong() {return EOF;}
public long int StmZeroLong() {return 0;}

public readonly StmProcs closedStmProcs = {
  StmErr, StmErr, StmZeroLong, StmZeroLong, StmErr, StmErr,
  StmErr, StmErr, StmErr, StmErr, StmErr, StmErrLong,
  "Closed"};

typedef struct {Links links; StmRec stmRec;} StmElem;

private Links stmList;


/* Exported procedures */

public procedure StmInit()
  {
  InitLink(&stmList);
  os_stdin = os_stdout = os_stderr = NULL;
  }

public procedure os_cleanup()
  /* Called from exit code to clean up all open files. */
  {
  Stm stm;
  while (stmList.next != &stmList)
    {
    stm = & ((StmElem *) stmList.next)->stmRec;
    fclose(stm);
    }
  }

public Stm StmCreate(procs, xtraBytes)
  StmProcs *procs; integer xtraBytes;
  {
  StmElem *elem;
  elem = (StmElem *) os_sureCalloc((integer) 1, (integer) sizeof(StmElem) + xtraBytes);
  if (elem == NULL) return NULL;
  elem->stmRec.procs = procs;
  InsertLink(&stmList, &elem->links);
  return &elem->stmRec;
  }

public procedure StmDestroy(stm)
  Stm stm;
  {
  StmElem *elem;
  elem = (StmElem *) ((Links *) stm - 1); /* elem -> containing StmElem */
  RemoveLink(&elem->links);
  /* give StmRec benign contents before deallocating it */
  os_bzero((char *) stm, (integer) sizeof(StmRec));
  stm->procs = &closedStmProcs;
  os_free((char *) elem);
  }


/* Generic stream operations -- for use with buffered streams that define
 * no special semantics for these operations */

public long int StmFRead(ptr, itemSize, nItems, stm)
  register char *ptr;
  long int itemSize, nItems;
  register Stm stm;
  {
  long int chars = nItems*itemSize;
  while (chars != 0)
    if (stm->cnt > 0) {
      register long int thisChars = chars < stm->cnt? chars : stm->cnt;
      os_bcopy(stm->ptr, ptr, thisChars);
      ptr += thisChars;
      chars -= thisChars;
      stm->ptr += thisChars;
      stm->cnt -= thisChars;
      }
    else {
      register int ch;
      if ((ch = (*stm->procs->FilBuf)(stm)) < 0)
        return nItems - (chars+itemSize-1)/itemSize;
      *ptr++ = ch;
      chars--;
      }
  return nItems;
  }

public long int StmFWrite(ptr, itemSize, nItems, stm)
  register char *ptr;
  long int itemSize, nItems;
  register Stm stm;
  {
  long int chars = nItems*itemSize;
  while (chars != 0)
    if (stm->cnt > 0) {
      register long int thisChars = chars < stm->cnt? chars : stm->cnt;
      os_bcopy(ptr, stm->ptr, thisChars);
      ptr += thisChars;
      chars -= thisChars;
      stm->ptr += thisChars;
      stm->cnt -= thisChars;
      }
    else {
      if ((*stm->procs->FlsBuf)((unsigned char)*ptr++, stm) < 0)
        return nItems - (chars+itemSize-1)/itemSize;
      chars--;
      }
  return nItems;
  }

public int StmUnGetc(ch, stm)
  register int ch;
  register Stm stm;
  {
  if (ch == EOF) return -1;
  if (!stm->flags.read || stm->ptr <= stm->base)
    if (stm->ptr == stm->base && stm->cnt == 0)
      stm->ptr++;
    else return -1;
  stm->cnt++;
  *--stm->ptr = ch;
  stm->flags.eof = 0;
  return ch;
  }
