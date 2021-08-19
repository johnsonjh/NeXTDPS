/*
  error.h

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

Original version: 
Edit History:
Ivor Durham: Mon Aug 22 18:32:55 1988
Ed Taft: Mon Aug  1 09:28:06 1988
Jim Sandman: Thu Apr 13 15:42:50 1989
Paul Rovner: Wednesday, May 4, 1988 4:56:26 PM
End Edit History.
*/

#ifndef	ERROR_H
#define	ERROR_H

#include BASICTYPES

/* Data Types */

/* these are the only Exceptions that PS raises; they are also
 * returned as the abort reason by GetAbort (zero means no abort
 * in progress) */

#define PS_ERROR -2
#define PS_STKOVRFLW -3
#define PS_STOP -4
#define PS_INTERRUPT -5
#define PS_EXIT -6
#define PS_DONE -7
#define PS_SYSIN -8
#define PS_REINITCACHE -9
#define PS_TERMINATE -10
#define PS_YIELD -11
#define	PS_TIMEOUT -12

#define _error extern

/* Exported Procedures */

_error	procedure	PSError(/*NameObj*/);
_error	procedure	_FInvlAccess(); /* fatal invalidaccess */
_error	procedure	PSInvlAccess();  /* overridable invalidaccess */
_error	procedure	_InvlFont();
_error	procedure	PSLimitCheck();
_error	procedure	_NoCurrentPoint();
_error	procedure	PSRangeCheck();
_error	procedure	PSTypeCheck();
_error	procedure	PSUndefFileName();
_error	procedure	PSUndefined();
_error	procedure	PSUndefResult();
_error	procedure	_VMERROR();

/* The error procedures are called via macros for hysterical reasons */

#define FInvlAccess	_FInvlAccess
#define InvlAccess	PSInvlAccess
#define InvlFont	_InvlFont
#define LimitCheck	PSLimitCheck
#define NoCurrentPoint	_NoCurrentPoint
#define RangeCheck	PSRangeCheck
#define TypeCheck	PSTypeCheck
#define UndefFileName	PSUndefFileName
#define Undefined	PSUndefined
#define UndefResult	PSUndefResult
#define VMERROR		_VMERROR

/* Exported Data */

extern PNameObj errorNames;

/* Following enumeration must be kept in sync with names in vmnames.h */

#define dictfull errorNames[0]
#define dstkoverflow errorNames[1]
#define dstkunderflow errorNames[2]
#define estkoverflow errorNames[3]
#define invlaccess errorNames[4]
#define	invlcontext errorNames[5]
#define invlexit errorNames[6]
#define invlflaccess errorNames[7]
#define invlfont errorNames[8]
#define invlid errorNames[9]
#define invlrestore errorNames[10]
#define ioerror errorNames[11]
#define limitcheck errorNames[12]
#define nocurrentpoint errorNames[13]
#define rangecheck errorNames[14]
#define stackunderflow errorNames[15]
#define stackoverflow errorNames[16]
#define syntaxerror errorNames[17]
#define typecheck errorNames[18]
#define undefined errorNames[19]
#define undeffilename errorNames[20]
#define undefresult errorNames[21]
#define unmatchedmark errorNames[22]
#define unregistered errorNames[23]
#define VMerror errorNames[24]

#endif	ERROR_H
