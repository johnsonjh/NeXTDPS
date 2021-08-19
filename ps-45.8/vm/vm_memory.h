/*
  vm_memory.h

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
Ivor Durham: Wed Oct 26 08:55:42 1988
Ed Taft: Sun Apr 10 14:37:14 1988
End Edit History.
*/

#ifndef	VM_MEMORY_H
#define	VM_MEMORY_H

#include BASICTYPES
#include VM

/*
 * Data types
 */

typedef struct _t_GC_Data *GC_PData;

typedef struct _t_VMPrivateData {
  GC_PData gcData;		/* Garbage collector private data */
  char   *_lastContext;		/* Record of last context to use VM */
} VMPrivateData;

/*
 * Inline procedures
 */

#define	Is_Shared(o) InVMSection (vmShared, o)

#endif	VM_MEMORY_H
