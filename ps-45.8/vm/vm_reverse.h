/*
  vm_reverse.h

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

Original version: Ed Taft: Sun Dec 17 14:21:03 1989
Edit History:
Ed Taft: Sun Dec 17 15:22:10 1989
End Edit History.
*/

#ifndef	VM_REVERSE_H
#define	VM_REVERSE_H

#include ENVIRONMENT
#include PUBLICTYPES

#define CANREVERSEVM (OS==os_sun && STAGE==DEVELOP)
  /* configuration(s) supporting VM reversal */

#if CANREVERSEVM

extern procedure ReverseVM();
/* Reverses everything that can be reached from rootShared. */

extern procedure ReverseSegment(/* PVMSegment segment */);
/* Reverses a VMSegment, with no tracing. */

extern procedure ReverseStructure(/* PVM vms */);
/* Reverses a VMStructure, with no tracing. */

extern procedure ReverseRelocationTable(/* boolean addrp */);
/* Reverses the value fields in the current relocationTable. If addrp
   is true, it also reverses the address fields.
 */

extern procedure ReverseInteger(/* integer *pInt */);
/* Reverses a single integer */

public procedure ReverseFields(/* char *ptr, *fields */);
/* Reverses endianness of structure at *ptr. The layout of that structure
   is described as a sequence of field sizes (in bits) in consecutive
   bytes of *fields, terminated by a zero byte. This description must
   include all unused fields and holes and must end at a byte boundary.
   A field cannot be longer than 32 bits or violate an alignment constraint
   for either the current or the target architecture.

   A negative field size indicates a field of the negative of that size
   that is to be skipped over without being reversed. This works only
   for a field whose size is a multiple of 8, aligned on a byte boundary.
 */

#endif CANREVERSEVM

#endif	VM_REVERSE_H
