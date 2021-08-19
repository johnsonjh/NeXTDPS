/*
  langdata.h

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

Original version: 
Edit History:
Ivor Durham: Fri Oct  7 09:16:28 1988
Joe Pasqua: Fri Aug 12 13:40:42 1988
Ed Taft: Sat May 21 15:18:23 1988
Jim Sandman: Wed Apr 12 17:02:28 1989
End Edit History.
*/

#ifndef	LANGDATA_H
#define	LANGDATA_H

#include ENVIRONMENT

struct _ExecData {	/* Data for exec implementation		*/
  boolean _stackRstr;	/* EXPORTed to exec.h			*/
  PStack _psFULLSTACK;	/* EXPORTed to exec.h			*/
  integer _execAbort, _execAbortAsync, _execAbortPending;
  integer _superExec;
  integer _execLevel;
  };

struct _MathData {	/* Data for Math implementation		*/
  longcardinal _randx;
  };

#define	randx		(languageCtxt->mathData._randx)

struct _ScannerData {	/* Data for scanner implementation	*/
  boolean _packedArrayMode;
  integer _objectFormat;
#if STAGE==DEVELOP
  integer _strStorageBufCount;
#endif STAGE==DEVELOP
  };

#define	packedArrayMode (languageCtxt->scannerData._packedArrayMode)
#define objectFormat (languageCtxt->scannerData._objectFormat)
#define strStorageBufCount (languageCtxt->scannerData._strStorageBufCount)

struct _StackData {	/* Data for stack implementation	*/
  cardinal _curStackLimit;
  PStack _opStk;
  PStack _execStk;
  PStack _dictStk;
  PStack _refStk;
  };

#define	curStackLimit	(languageCtxt->stackData._curStackLimit)

struct _StreamData {	/* Data for stream implementation	*/
  StmObj _stdinStm;
  StmObj _stdoutStm;
  boolean _echo;
  };

#define	echo		(languageCtxt->streamData._echo)
#define	stdinStm	(languageCtxt->streamData._stdinStm)
#define	stdoutStm	(languageCtxt->streamData._stdoutStm)

struct _DictData {
  GenericID _timestamp;
};

extern GenericID timestamp;

typedef struct {
  struct	_ExecData	execData;
  struct	_MathData	mathData;
  struct	_ScannerData	scannerData;
  struct	_StackData	stackData;
  struct	_StreamData	streamData;
  struct	_DictData	dictData;
  } LanguageData, *PLanguageData;

extern PLanguageData languageCtxt;

#endif	LANGDATA_H
