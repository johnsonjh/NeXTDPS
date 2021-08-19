/*
  makeobjfile.h

Copyright (c) 1989 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Sat Nov 18 14:43:50 1989
Edit History:
Ed Taft: Sun Dec 17 15:04:39 1989
End Edit History.
*/

#ifndef MAKEOBJFILE_H
#define MAKEOBJFILE_H

#include PUBLICTYPES

extern int ObjFileBegin(/* char *name */);
/* Opens the specified file for writing. The file must be positionable
   (i.e., fseek and ftell must work).  ObjFileBegin must be called before
   any other procedure defined here; only one object file at a time can
   be built. Returns 1 normally, 0 if file cannot be opened
   (see errno for reason).
 */

extern int ObjFileEnd();
/* Finishes up and closes the file. Returns 1 normally, 0 if an I/O error
   has occurred while writing the file (see errno for reason).
 */

extern procedure ObjPutText(/* char *ptr, long int count */);
/* Appends count bytes starting at *ptr to the text portion of the file;
   advances the text location count by the same amount.
 */

extern procedure ObjPutSymbol(/*
  char *name,
  unsigned char type,
  char other,
  short int desc,
  unsigned long int value */);
/* Defines a symbol with the specified name and parameters (see nlist
   structure in a.out.h; types match those of struct fields).
   This is specific to BSD 4.* Unix; following procs are more generic.
 */

extern procedure ObjPutTextSym(/* char *name */);
/* Defines an external text symbol whose value is the current location
   counter in the text portion of the file.
 */

extern procedure ObjPutBSSSym(/* char *name; long int count */);
/* Defines an external bss (uninitialized data) symbol whose value is the
   current location counter in the bss portion of the file; advances
   the bss location counter by the specified count (bytes).
 */

#endif MAKEOBJFILE_H
