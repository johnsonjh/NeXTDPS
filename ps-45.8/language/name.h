/*
  name.h

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
Ivor Durham: Sun Jul 17 18:28:29 1988
Ed Taft: Thu Nov 23 16:28:30 1989
End Edit History.
*/

#ifndef	NAME_H
#define	NAME_H

/* Constants */

#define minNameTableLength 64
#define maxNameTableLength 1024

/* Exported Procedures */

#define _name extern

_name	procedure	DestroyNameMap ();
_name	procedure	FastName(/*str,strlen,PObject*/);
_name	procedure	MakePName(/*string,PNameObj*/);
_name	procedure	NameIndexObj(/*nameindex:cardinal,PNameObj*/);
_name	procedure	NameInit(/*InitReason*/);
_name	procedure	StrToName(/*StrObj,PObject*/);
#endif	NAME_H
