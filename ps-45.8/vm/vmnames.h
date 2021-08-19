/*
  vmnames.h

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Thu Jul  7 14:34:27 1988
Edit History:
Ed Taft: Wed Jul  5 14:02:29 1989
Ivor Durham: Mon Aug 22 18:31:55 1988
Joe Pasqua: Tue Oct 25 14:25:09 1988
End Edit History.

Definition of operators and registered names for the vm package.
This is used both as a C header file and as the input to a shell
script, update_registered_names, that derives a PostScript program
to register the names and one or more C programs to register the
C procedures for the operators.
*/

#ifndef	VMNAMES_H
#define	VMNAMES_H

/*
  The following macro defines the package index for this package.
  It must be consistent with the one defined in orphans.h.
 */

#define PACKAGE_INDEX 0  /* = pni_vm */

/*
  The following macro calls define all standard PostScript operator names
  and their corresponding C procedures. The operators are (or can be)
  divided into multiple operator sets, each corresponding to a distinct
  language extension.

  The first two arguments of the OpSet macro specify a name and a number
  to identify the operator set. The name and number must both be unique
  within the package (the number is regenerated automatically by
  update_registered_names). The third argument identifies the dictionary
  in which the operators are to be registered.
 */

OpSet(ops_vmDPS, 1, systemdict)
  Op(restore, PSRstr)
  Op(save, PSSave)
  Op(setvmthreshold, PSSetThresh)
  Op(vmreclaim, PSCollect)
  Op(vmstatus, PSVMStatus)

/*
  The following macros define the indices for all registered names
  other than operators. The actual PostScript name is simply
  the macro name with the "nm_" prefix removed.

  The macro definitions are renumbered automatically by the
  update_registered_names script.

  *** The public interface error.h must be kept in sync with this ***
 */

#define nm_dictfull 0
#define nm_dictstackoverflow 1
#define nm_dictstackunderflow 2
#define nm_execstackoverflow 3
#define nm_invalidaccess 4
#define nm_invalidcontext 5
#define nm_invalidexit 6
#define nm_invalidfileaccess 7
#define nm_invalidfont 8
#define nm_invalidid 9
#define nm_invalidrestore 10
#define nm_ioerror 11
#define nm_limitcheck 12
#define nm_nocurrentpoint 13
#define nm_rangecheck 14
#define nm_stackunderflow 15
#define nm_stackoverflow 16
#define nm_syntaxerror 17
#define nm_typecheck 18
#define nm_undefined 19
#define nm_undefinedfilename 20
#define nm_undefinedresult 21
#define nm_unmatchedmark 22
#define nm_unregistered 23
#define nm_VMerror 24


/* The following definition is regenerated by update_registered_names */
#define NUM_PACKAGE_NAMES 25

/* No need for PNameObj vmNames variable */
/* PNameObj errorNames is declared in error.h */

#endif	VMNAMES_H
