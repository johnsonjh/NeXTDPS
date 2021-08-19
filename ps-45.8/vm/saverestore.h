/*
  saverestore.h

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
Ivor Durham: Tue Aug 23 16:41:58 1988
Ed Taft: Wed Nov 15 16:12:34 1989
Jim Sandman: Tue Mar 15 11:26:31 1988
End Edit History.
*/

#ifndef	SAVERESTORE_H
#define	SAVERESTORE_H

#include BASICTYPES
#include VM

/* Data Structures
 *
 * These structures, though private to the VM implementation, are
 * actually stored as part of a saved VM. Therefore, changing them will
 * invalidate saved VMs. In particular, fields of these data structures
 * must not be conditionally compiled!
 */

typedef struct _t_SRG {
  struct _t_SRG *link;
  PGenericBody pointer;
  GenericBody header;
} SRG, *PSRG;

typedef struct _t_SRO {
  struct _t_SRO *link;
  PObject pointer;
  Object  o;
} SRO, *PSRO;

typedef struct _t_SRD {
  struct _t_SRD *link;
  PDictBody pointer;
  DictBody db;
} SRD, *PSRD;

typedef struct _t_FinalizeNode {
  struct _t_FinalizeNode *link;
  Object  obj;
} FinalizeNode;		/* PFinalizeNode defined as opaque pointer in vm.h */

typedef struct _t_SR {
  PSR     link;
  Level   level;
  PCard8  free;
  PSRO    objs;
  PSRD    dbs;
  PSRG    generics;
} SR;			/* PSR defined as opaque pointer in vm.h */

/* Remaining definitions are not part of the VM */

/*
 * Exported Procedures
 */

extern procedure _RecordFinalizableObject(/* PVMRoot root, Object obj */);
/* Adds obj to the list of finalizable objects for the VM in which
   obj's value lives (which must be shared or current private VM).
   Finalization must previously have been registered for the
   object's type. This procedure is ordinarily called only from
   the RecordFinalizableObject macro, below.
*/

/*
 * Exported Data
 */

extern Card16 finalizeReasons[nObTypes];
  /* assert: number of FinalizeReasons is 16 or fewer */
extern FinalizeProc finalizeProcs[nObTypes];

/*
 * Inline procedures
 */

#define RecordFinalizableObject(type, obj) \
  if (finalizeReasons[type] != 0) \
    _RecordFinalizableObject(obj)
/* If finalization (for any reason) has been registered for the specified
   type, adds obj to the list of finalizable objects for the VM in
   which obj's value lives (which must be shared or current private VM) */

#define WantToFinalize(type, reason) \
  ((finalizeReasons[type] & (1 << ((int)reason))) != 0)
/* Returns true iff finalization has been registered for the specified
   type and reason. */

#define CallFinalizeProc(type, obj, reason) \
  if (WantToFinalize(type, reason)) \
    (*finalizeProcs[type])(obj, reason)
/* If finalization has been registered for the specified type and reason,
   calls the type's finalization procedure with obj and reason. */

#endif	SAVERESTORE_H
