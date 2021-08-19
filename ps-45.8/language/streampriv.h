/*
  streampriv.h

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

Original version: Ed Taft: Fri May 15 14:07:23 1987
Edit History:
Ed Taft: Fri Dec  1 16:15:35 1989
Joe Pasqua: Thu Jan 19 15:05:09 1989
Ivor Durham: Fri Oct 14 16:48:24 1988
Jim Sandman: Thu Oct 26 14:41:39 1989
End Edit History.
*/
#include PACKAGE_SPECS
#include ENVIRONMENT

extern procedure StreamInit(/* InitReason reason */);
/* Initialize the stream module */

extern procedure StmCtxCreate();
/* Prepare stream module's per context data */

extern procedure PrivateStreamRoots(/* GC_Info info */);
/* Push stream module's private GC roots */

extern procedure HandlePendingEOL(/* Stm stm; */);
/* The PostScript scanner and operators that interpret a stream as "text"
   give uniform treatment to all styles of EOL (CR, LF, or CR-LF).
   If an operator stops reading upon encountering a CR, it sets the
   stm->flags.f1 flag as an indication that the next character should
   be discarded if it is a LF. This procedure should be called
   at the beginning of every stream reading operator if the flag is set. */

extern procedure StmCtxCreate();
/* Called during context creation to create PostScript StmObjects
   for the standard input and output streams
 */

extern Stm StoDevCreateStm(/* char *name, *access */);
extern procedure StoDevStrStatus(/* StrObj str */);

extern boolean IsCrFile(/* StmObj so */);
/* Returns true iff so is the same as currentfile */

extern char *hexToBinary;
#define NOTHEX -1

#if (OS == os_vms)
globalref StmProcs strStmProcs;
#else (OS == os_vms)
extern readonly StmProcs strStmProcs;
#endif (OS == os_vms)

/* Definitions shared among filter stream implementations
   Note: a filter stream is distinguished by having a StmProcs.type
   whose first character is "!".
 */

/* FilterDataRec is shared among all filter files. It appears at the
   beginning of the private extension to the generic StmRec.
 */

typedef struct _t_FilterDataRec {
  short int subObjCount;	/* count of subsidiary objects */
  Object subObj[1];		/* start of array of subsidiary objects */
} FilterDataRec, *PFilterDataRec;

extern procedure GetSrcOrTgt(/* PStmObj pStmObj, boolean readMode */);
/* Pops an operand and sets it up as the source or target for a filter.
   If the operand is a file, it is stored directly in *pStmObj. If the
   operand is a string or procedure, an intermediate file is constructed
   and stored in *pStmObj. readMode is true for a source, false for
   a target. Raises an exception if a problem is detected.
 */

extern boolean CloseIfNecessary(/* Stm baseStm */);
/* If baseStm was an intermediate stream created by GetSrcOrTgt, closes
   it and returns true; otherwise returns false.
 */

extern procedure HandleFilterStream(/* PStmBody psb, GC_Info info */);
/* Called during garbage collection to push roots referenced from *psb,
   which the caller asserts is a filter stream.
 */
