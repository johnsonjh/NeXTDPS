/*
  orphans.h

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
Ivor Durham: Thu May 11 11:49:29 1989
Ed Taft: Fri Nov 24 16:25:03 1989
Joe Pasqua: Wed Feb 22 16:25:27 1989
Paul Rovner: Wednesday, May 4, 1988 12:52:31 PM
Perry Caro: Fri Nov  4 11:19:42 1988
End Edit History.
*/

/*
 * Definitions that have migrated downwards because of circular dependencies.
 * These definitions need to be distributed to the appropriate abstractions
 * in the forthcoming re-organisation.
 */

#ifndef	ORPHANS_H
#define	ORPHANS_H

#include BASICTYPES

/*
 * Static Name and Command Registration Interface
 */

/* Package name index values (index in VMRoot.vm.Shared.regNameArray) */
#define pni_vm 0
#define pni_language 1
#define pni_graphics 2
#define pni_fonts 3
#define pni_postscript 4
#define pni_end 5	/* first unused index */

extern PNameObj RgstPackageNames(/* integer pkgIndex, count */);
/* Declares that the package identified by pkgIndex requires count
   statically registered names. Returns pointer to base of array of
   statically registered name objects for the package. If the array
   doesn't exist yet (whole cloth init), it is created (but the names
   aren't filled in until PS execution begins). This procedure must
   be called at "romreg" time by each package that uses statically
   registered names.
 */

extern procedure RgstOpSet(
  /* procedure (**cmdProcs)(), Card16 numProcs, Int16 opSetID */);
/* Registers the C procedures for a statically registered operator set.
   cmdProcs is a pointer to an array of pointers to C procedures;
   numProcs is the number of procedures. opSetID is the unique OpSet
   identifier; it should be PACKAGE_INDEX*256 + opSetNumber, where
   opSetNumber is the OpSet number within the package (second argument
   to OpSet macro in <package>names.h).
   This procedure must be called at "romreg" time by each package
   that implements statically registered operators.
 */

extern procedure CmdIndexObj(/* Card16 cmdIndex, PCmdObj pcmd */);
/* Given a command index (e.g., from a packed array), constructs a
   completely filled in command object and stores it in *pcmd.
   cmdIndex must be valid, else CantHappen. Note that this works
   for both statically and dynamically registered operators.
 */

/* No-op macros for operator definitions appearing in <package>names.h;
   those macros are actually used only during update_registered_names.
 */

#define OpSet(name, number, dict)
#define Op(name, proc)

/*
 * Dynamic Command Registration Interface
 */

/* Data Types */

typedef struct _t_CmdTable {
  integer max;
  procedure (**cmds)(); /* -> array of pointers to cmd procedures */
}  CmdTable;

typedef struct _t_RgCmdEntry {
  char   *name;
  procedure (*proc) ();
}  RgCmdEntry,
  *PRgCmdEntry,
   RgCmdTable[];

typedef struct _t_RgNameEntry {
  char   *name;
  PNameObj pob;
}  RgNameEntry,
  *PRgNameEntry,
   RgNameTable[];

/* command mark codes stored in the access fields of internal
 * CmdObjects used to mark the stack */

#define mrkNone		0	/* ordinary command */
#define mrkExec		1	/* base of psExecute */
#define mrkStopped	2	/* stopped command */
#define mrkMonitor	3	/* monitor command */
#define	mrkRun		4	/* run command */
#define	mrk1Arg		5	/* 1 operand left on exec stack */
#define	mrk2Args	6	/* 2 operands left on exec stack */
#define	mrk4Args	7	/* 4 operands left on exec stack */

/* Exported Procedures */

#define	_rgstcmds	extern

_rgstcmds	procedure	RgstExplicit(/*string,procedure*/);
_rgstcmds	procedure	RgstInternal(/*string,procedure,PCmdObj*/);
_rgstcmds	procedure	RgstMark(/*string,procedure,integer,PCmdObj*/);
_rgstcmds	procedure	RgstMCmds(/*PRgCmdEntry*/);
_rgstcmds	procedure	RgstMNames(/*PRgNameEntry*/);

/* Exported Data */

_rgstcmds	CmdTable cmds;
_rgstcmds	Card16	currentCmd;

/*
 * Program arguments interface
 */

/* Exported Procedures */

extern char *GetCArg(/* char option */);
/* Looks for an instance of "-c arg" on the command line, where c is
   the specified option character. If one is found, returns a pointer
   to the arg string (the next token after the option). If not found,
   returns NIL.
 */

extern integer NumCArg(/* char option, integer radix, deflt */);
/* Like GetCArg, but interprets the arg as a number (unsigned) according
   to the specified radix and returns it. If not found, returns deflt.
 */

extern integer GetCSwitch(/* char *name, integer deflt */);
/* Looks for an instance of the specified switch on the command line.
   A switch is a token not starting with "-" and not preceded by a
   "-c" (option) token. If the switch is not present, GetCSwitch
   returns deflt. If the switch is present as a simple token "name",
   GetCSwitch returns 1. If a token is found of the form "name=value"
   (no spaces), GetCSwitch interprets value as a decimal number
   (unsigned) and returns it.
 */

extern procedure BeginParseArguments (/* int argc, char *argv[] */);
/* Sets up to parse arguments. This procedure retains its argv argument
   until EndParseArguments.
 */

extern boolean EndParseArguments();
/* Declares end of argument parsing. If any arguments were unused,
   prints a warning message to os_stderr and returns true; otherwise
   returns false.
 */

/*
 * Static data management interface
 */

/* Data Types */

typedef enum {
  staticsCreate,	/* New static data block created -- initialise */
  staticsDestroy,	/* Clean up -- data block about to be destroyed*/
  staticsLoad,		/* Static record pointers just loaded */
  staticsUnload,	/* Static record pointers about to be unloaded */
  staticsSpaceDestroy	/* Clean up -- Space about to be destroyed */
} StaticEvent;

/* StaticEvent mask is bit-wise OR or STATICEVENTFLAGs */

#define	STATICEVENTFLAG(x)	(1 << ((integer)(x)))

/* Exported Procedures */

extern procedure CallDataProcedures (/* Code */);
 /* StaticEvent Code; */
 /*
  * Invoke registered procedures for which Code is enabled in the mask.
  *
  * pre:  Static data pointers have been loaded.
  * post: Each registered procedure that is enabled for Code has been
  *       invoked once.
  */

extern procedure RegisterData (/*PointerVariable, Size, Procedure, Mask*/);
 /* PCard8 *PointerVariable;	/* Address of pointer to static record */
 /* integer Size;		/* Required size of static record */
 /* procedure (*Procedure)();	/* Initialisation procedure */
 /* integer Mask;		/* Enabling Mask of StaticEvents */
 /*
  * Register a static data record pointer along with a procedure to invoke
  * for the various static data events and a mask to indicate which of the
  * events the procedure is interested in.
  */

extern PCard8 CreateData ();
 /*
  * Generate and load new initial data block and invoke initialisation
  * procedures for which the staticCreate event is enabled.
  *
  * Note: Invokes LoadData before invoking procedures for staticsCreate.
  *
  * pre: No pointers loaded.
  * post: RESULT == address of new data block ||
  *	  RESULT == NIL if a new block cannot be allocated.
  */

extern DestroyData (/* StaticData */);
 /* PCard8 StaticData; */
 /*
  * Invokes the static procedures with staticsDestroy and then frees
  * the storage.
  */

extern procedure LoadData (/* To_StaticData */);
 /* PCard8 To_StaticData; */
 /*
  * Load static data pointers with addresses of records in the To_StaticData
  * block and invoke the procedures enabled for the staticLoad event.
  *
  * pre:  Data pointers unloaded.
  * post: Data pointers loaded with addresses inside To_StaticData.
  */

extern PCard8 UnloadData ();
 /*
  * Invokes the static data procedures with staticsUnload.
  *
  * pre:  true
  * post: RESULT == address of unloaded data block or NIL if there was none.
  */

#endif	ORPHANS_H
