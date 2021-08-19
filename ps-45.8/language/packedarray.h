/*
  packedarray.h

Copyright (c) 1984, '86 Adobe Systems Incorporated.
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
Ivor Durham: Mon May 16 13:14:47 1988
Joe Pasqua: Wed Jan  4 17:55:38 1989
End Edit History.
*/

#ifndef	PACKEDARRAY_H
#define	PACKEDARRAY_H

#include BASICTYPES

#define _pkdary extern

/* Exported Procedures */

_pkdary	procedure	BindPkdary(/*PkdaryObj*/);
_pkdary	procedure	DecodeObj(/*PPkdaryObj,PObject*/);
_pkdary procedure	Pkdary(/*stack_entry_count:cardinal,PPkdaryObj*/);
_pkdary procedure	PkdaryInit(/* InitReason reason */);

#endif	PACKEDARRAY_H
