/*
  array.h

Copyright (c) 1983, '86, '87, '88 Adobe Systems Incorporated.
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
Ivor Durham: Tue Dec  2 15:41:27 1986
End Edit History.
*/

#ifndef	ARRAY_H
#define	ARRAY_H

#include BASICTYPES

#define _array extern

#define MAXarrayLength MAXcardinal

/* Exported Procedures */

_array	procedure	AGetP(/*AnyAryObj,index:cardinal,PObject*/);
_array	procedure	APut(/*AryObj,index:cardinal,valueOb*/);
_array	procedure	ArrayInit(/*InitReason*/);
_array	procedure	AryForAll(/*AnyAryOb,procOb*/);
_array	procedure	AStore(/*AryObj*/);
_array	procedure	ForceAGetP(/*AnyAryObj,index:cardinal,PObject*/);
_array	procedure	PutArray(/*from:AnyAryObj,cardinal,into:AryObj*/);
_array	procedure	SubPArray(/*AnyAryObj,first,len:cardinal,PAnyAryObj*/);

#endif	ARRAY_H
