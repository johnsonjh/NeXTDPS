/*
  fontbuild.h

Copyright (c) 1987 Adobe Systems Incorporated.
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
Ivor Durham: Fri Jul 15 18:17:52 1988
Ed Taft: Sun Feb 11 16:22:10 1990
Jim Sandman: Tue Dec 12 15:47:25 1989
End Edit History.
*/

#ifndef	FONTBUILD_H
#define	FONTBUILD_H

#include BASICTYPES

/* Exported Procedures */

#define _Fbuild	extern

#define BUILTINKEY 5839
#define INTERNALKEY 1183615869

_Fbuild boolean		BuildChar(/*integer c; PObject pcn*/);


#endif	FONTBUILD_H
