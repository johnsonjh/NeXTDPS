/*
  vm_space.h

Copyright (c) 1984, '86, '87, '88 Adobe Systems Incorporated.
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
Ivor Durham: Mon Feb  8 18:17:46 1988
End Edit History.
*/

#ifndef	VM_SPACE_H
#define	VM_SPACE_H

/*
 * This module handles space allocation from the C-environment "malloc"
 * world.  Use of this module may be monitored by defining the symbol SPACE
 * to be true.
 */

#include BASICTYPES

/* Exported Procedures */

extern			Init_VM_Space (/* InitReason reason*/);
extern procedure	PSPrintChunks();

#if	STAGE==DEVELOP
extern charptr		EXPAND(/* charptr current, integer n, size*/);
#endif	STAGE==DEVELOP

#endif	VM_SPACE_H
