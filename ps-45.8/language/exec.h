/*
  exec.h

Copyright (c) 1983, '86, '87 Adobe Systems Incorporated.
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
Ivor Durham: Mon Jan 19 15:17:11 1987
Joe Pasqua: Sun Oct 25 16:24:50 1987
End Edit History.
*/

#ifndef	EXEC_H
#define	EXEC_H

#include BASICTYPES
#include VM	/* For PStack, for now */
#include "langdata.h"

/* Data Types */

#define MAXexecLevel 10		/* max recursion level of psExecute */
#define UOBJINITSIZE 10		/* initial number of entries in UserObjects */
#define MAXUSEROBJECTINDEX 800
/* Exported Procedures */

#define _exec extern

_exec	procedure	BindArray(/*AryObj*/);
_exec	procedure	ExecInit(/*InitReason*/);
_exec	integer		GetAbort();
_exec	boolean		psExecute(/*Object*/);
_exec	procedure	SetAbort(/*reason:integer*/);
_exec			UnmarkLoop();

/*
 * Exported Data
 */

#define	stackRstr	(languageCtxt->execData._stackRstr)
#define	psFULLSTACK	(languageCtxt->execData._psFULLSTACK)

#endif	EXEC_H
