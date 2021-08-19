/*
  postscript.c

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

Original version: Chuck Geschke: February 11, 1983
Edit History:
Scott Byer: Thu Jun  1 15:34:12 1989
Ivor Durham: Tue May  9 00:02:47 1989
Ed Taft: Wed Nov 29 09:34:12 1989
Linda Gass: Tue Aug 11 14:36:47 1987
Paul Rovner:Wednesday, October 7, 1987 6:00:03 PM
Jim Sandman: Tue Apr  4 17:26:33 1989
Joe Pasqua: Tue Feb 28 13:49:28 1989
Perry Caro: Wed Nov  9 17:32:44 1988
Bill Bilodeau: Wed Jul 26 14:46:47 PDT 1989
Paul Rovner: Mon Aug 28 10:15:11 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT

#include COPYRIGHT

#include BASICTYPES
#include ERROR
#include EXCEPT
#include GC
#include GRAPHICS
#include LANGUAGE
#include ORPHANS
#include POSTSCRIPT
#include PSLIB
#include RECYCLER
#include SIZES
#include VM

#include "postscriptnames.h"

extern procedure ReadVMFromFile();
extern procedure VM_Usage();
  /* These procs aren't exported by the language/vm	*/
  /* pkgs, thats why they're redeclared here.		*/

extern PSContext CreateContext();
extern procedure InitializePackages();
extern procedure ControlInit(), PSSpaceInit(),
                 ContextOpsInit(), CustomOpsInit();
  /* These should be in a package specific include file	*/


public PNameObj postscriptNames;

/*
   These are the TrickyDicts to be placed in systemdict at initialization
   time.
 */

/*-- BEGIN GLOBALS --*/

public  DictObj trickyUserDict;
private DictObj trickyErrorDict;
public  DictObj trickyStatusDict;
public  DictObj trickyFontDirectory;

/*-- END GLOBALS --*/

#define initSysDictSize 315
#define initUserDictSize 200
#define	initSharedDictSize 50
#define initErrorDictSize 32
#define initInternalDictSize 40	/* leave 10 or so spares for future use */
#define initStatusDictSize 65	/* leave 10 or so spares for patching */
#define initTrickyArraySize 20
#define initSharedFontDirectorySize 250
#define initFontDirectorySize 250

#define	defaultCustomDictSize 8

#define SwitchListDefaultValue -1	/* Ought to go to arguments interface */

#define DvmSharedSize 22000

/*
 * define name (hash) table parameters
 * name table must be a power of 2 long, and if you change it to
 * anything besides 1024 then you should look at the hash algorithm
 * in FastName.
 */

#define HASHINDEXFIELD	10			  /* field size for NEASize */
#define NEASize (1 << HASHINDEXFIELD)
#define HASHINDEXMASK	(~(-1 << HASHINDEXFIELD)) /* bitmask for field */

/*
 * Local state
 */

public integer exitCode;

/*
 * Forward declarations
 */

public procedure TopError();
private procedure Quit();

#if	VMINIT
private CreateSharedVM ()
 /*
  * pre:  root initialized.
  * post: sysDict, userDict, errorDict, and statusDict created and in root.
  *
  * The system dictionary must be created first (position 0 in the name
  * bitvector is reserved for it) 
  */
{
  vmShared = CreateVM(true, true);

  SetShared (true);

  /* The root structure is *assumed* to be the very first item in a VM */

  VMSetRAMAlloc();
  rootShared = (PVMRoot) AllocChars ((integer)sizeof (VMRoot));
  ResetRecycler (sharedRecycler);

  rootShared->spaceID.stamp = 0;
  rootShared->version = VMVERSION;
  rootShared->shared = true;

  AllocPArray (MAXrootParam, &rootShared->vm.Shared.param);
  ResetRecycler (sharedRecycler);

  rootShared->vm.Shared.stamp = 0;
  rootShared->vm.Shared.dictCount = 0;	/* precondition of first call of Dict */
  rootShared->vm.Shared.downloadChain = NIL;
  rootShared->vm.Shared.cmdCnt = 0;

  AllocPNameArray (NEASize, &rootShared->vm.Shared.nameTable);
  ResetRecycler (sharedRecycler);

  rootShared->vm.Shared.hashindexmask = HASHINDEXMASK;	/* name index mask */
  rootShared->vm.Shared.hashindexfield = HASHINDEXFIELD;/* field width */

  /* Initial dictionaries */

  VMSetROMAlloc();
  DictP ((cardinal) NumCArg ('S', (integer) 10, (integer) initSysDictSize),
	 &rootShared->vm.Shared.sysDict);

  VMSetRAMAlloc();
  DictP ((cardinal) NumCArg ('N', (integer) 10, (integer) initInternalDictSize),
	 &rootShared->vm.Shared.internalDict);

  DictP ((cardinal) NumCArg ('C', (integer) 10, (integer) initSharedDictSize),
	 &rootShared->vm.Shared.sharedDict);

  DictP ((cardinal) NumCArg ('G', (integer) 10, (integer) initSharedFontDirectorySize),
	 &rootShared->vm.Shared.sharedFontDirectory);

  SetDictAccess (rootShared->vm.Shared.sharedFontDirectory, rAccess);

  /*
   * Creation of built-in tricky dictionaries must be in order of tricky dict
   * index; see vm.h
   */

  VMSetROMAlloc();
  AllocPArray ((cardinal) NumCArg ('D', (integer) 10, (integer) initTrickyArraySize),
	       &rootShared->trickyDicts);
  ResetRecycler (sharedRecycler);

  /* Real dictionaries for main tricky dicts. */

  rootPrivate = rootShared; /* rootPrivate referenced in TrickyDictP */

  TrickyDictP ((cardinal) NumCArg ('U', (integer) 10, (integer) initUserDictSize),
	       &trickyUserDict);
  Assert ((Card32)trickyUserDict.val.dictval == tdUserDict);

  TrickyDictP ((cardinal) NumCArg ('E', (integer) 10, (integer) initErrorDictSize),
	       &trickyErrorDict);
  Assert ((Card32)trickyErrorDict.val.dictval == tdErrorDict);

  /* Allocate primordial statusdict in RAM so that additional operators
     can be registered during product init */
  VMSetRAMAlloc();
  TrickyDictP ((cardinal) NumCArg ('T', (integer) 10, (integer) initStatusDictSize),
	       &trickyStatusDict);
  Assert ((Card32)trickyStatusDict.val.dictval == tdStatusDict);

  VMSetROMAlloc();
  TrickyDictP ((cardinal) NumCArg ('g', (integer) 10, (integer) initFontDirectorySize),
	       &trickyFontDirectory);
  Assert ((Card32)trickyFontDirectory.val.dictval == tdFontDirectory);

  SetDictAccess (trickyFontDirectory, rAccess);
  VMSetRAMAlloc();
}
#endif	VMINIT

private CheckVersion ()
{
  rootShared->vm.Shared.stamp++;

  if (rootShared->version != VMVERSION) {
#if OS!=os_ps
    os_fprintf (os_stderr, "\007\n\007\nThe VM being restored is not compatible with this version of PS.\n");
    os_exit (exitError);
#else OS!=os_ps
    CantHappen();
#endif OS!=os_ps
  };
}

private procedure BuildInitialState (vmStm)
  Stm vmStm;
 /*
   pre: vmStm == NIL or vmStm == VM file stream.
   post: vmStm closed if there was one.
  */
{
   /* Attempt to restore previously saved state. Note that vmStm==NIL
      can mean either that we are to build the VM from scratch or
      that it is already bound into the code; StartVM knows which. */
  if (StartVM (vmStm))
    {
    if (vmStm != NIL) fclose(vmStm);
    CheckVersion ();
    return;
    }

#if	VMINIT
  /*
   * Create initial state from whole cloth
   */
  vISP = GetCSwitch("ISP", ISP);
  vOS = GetCSwitch("OS", OS);
  vSTAGE = GetCSwitch("STAGE", STAGE);
  if (GetCSwitch("EXPORT", 0)) vSTAGE = EXPORT;
  vLANGUAGE_LEVEL = GetCSwitch("LANGUAGE_LEVEL", LANGUAGE_LEVEL);
  vSWAPBITS = GetCSwitch("SWAPBITS", SWAPBITS);
  vPREFERREDALIGN = GetCSwitch("PREFERREDALIGN", PREFERREDALIGN);

  CreateSharedVM ();
  return;

#else	VMINIT
#if	OS!=os_ps
  os_fprintf (os_stderr, "Can't find a VM to restore!\n");
  os_abort ();
#else	OS!=os_ps
  CantHappen ();
#endif	OS!=os_ps
#endif	VMINIT
}

public procedure InitPostScript (psParameters)
  PPostScriptParameters psParameters;
{
  PSSpace InitialSpace;
  PSContext InitialContext;
  Stm     saveos_stdin = os_stdin,
          saveos_stdout = os_stdout;

  os_stderr = psParameters->errStm;
  SetRealClockAddress (psParameters->realClock);

  DURING {
    InitializePackages (init);

    if (psParameters->gStateExtProc != NULL)
      SetGStateExtProc(psParameters->gStateExtProc);
  
    BuildInitialState (psParameters->vmStm);

    InitialSpace = CreatePSSpace ();
    InitialContext = (PSContext) CreateContext (InitialSpace, (Stm)NIL, (Stm)NIL, iLNullObj);

    SetShared (true);

    if (!vmShared->wholeCloth) {
      LDictObj (trickyUserDict, (PDictBody) tdUserDict);
      trickyUserDict.length = 1;
      LDictObj (trickyErrorDict, (PDictBody) tdErrorDict);
      trickyErrorDict.length = 1;
      LDictObj (trickyStatusDict, (PDictBody) tdStatusDict);
      trickyStatusDict.length = 1;
      LDictObj (trickyFontDirectory, (PDictBody) tdFontDirectory);
      trickyFontDirectory.length = 1;
    }

    ResetNameCache ((integer)0);

#if VMINIT
    if (vmShared->wholeCloth || MAKEVM)
      Begin (rootShared->vm.Shared.internalDict);
#endif VMINIT
    Begin (rootShared->vm.Shared.sysDict);

    InitializePackages (romreg);

    Begin (rootShared->vm.Shared.sharedDict);
    Begin (trickyUserDict);

    InitializePackages (ramreg);

    if (psParameters->customProc != NIL) {
      Begin (rootShared->vm.Shared.sysDict);
      (*(psParameters->customProc)) ();
      End ();
    }

#if OS!=os_ps
    if (psParameters->writeVMProc != NIL) {
    	(*(psParameters->writeVMProc)) ();
	if (GetCArg('e') != NULL)
	    vmShared->wholeCloth = true;
    }
#endif OS!=os_ps

  } HANDLER {
    os_stdin = saveos_stdin;
    os_stdout = saveos_stdout;
    TopError (Exception.Code, "initialization");
  } END_HANDLER;

  SetShared (vmShared->wholeCloth);
  
  if (!vmShared->wholeCloth && (psParameters->writeVMProc == NIL))
    SetDictAccess(rootShared->vm.Shared.sysDict, rAccess);

  DestroyPSContext ();
  DestroyPSSpace (InitialSpace);

  os_stdin = saveos_stdin;
  os_stdout = saveos_stdout;
}

public procedure TopError(errorCode, what)
  int errorCode; char *what;
{
  PNameEntry pne;
  switch (errorCode) {
    case PS_STOP:
      os_fprintf (os_stderr, "%s procedure lost control\n", what);
      break;
    case PS_EXIT:
      os_fprintf (os_stderr, "Invalid exit during %s procedure\n", what);
      break;
    case PS_ERROR:
      os_fprintf (os_stderr, "Unhandled PostScript error \"");
      if (psERROR.type == nameObj) {
        pne = psERROR.val.nmval;
        fwrite (pne->str, 1, pne->strLen, os_stderr);
      }
      os_fprintf (os_stderr, "\" during %s procedure\n", what);
      break;
    default:
      RAISE ((int)GetAbort (), (char *)NIL);
        /* will cause an uncaught exception abort */
  }

  if (postscriptNames != NIL &&
      postscriptNames[nm_handleerror].type != nullObj) {
    SetAbort ((integer) 0);
    (void) psExecute (postscriptNames[nm_handleerror]);
      /* try to print an error message */
  }

  exitCode = exitError;
  RAISE (PS_TERMINATE, (char *)NIL);
}

#if VMINIT
private procedure PSAbort()
{
  os_exit(exitError);
}
#endif VMINIT

public procedure StateInit (reason)
  InitReason reason;
{
  NameObj version;

  switch (reason) {
   case init:
    exitCode = exitNormal;
    break;
   case romreg:
    postscriptNames = RgstPackageNames((integer)PACKAGE_INDEX, NUM_PACKAGE_NAMES);
#include "ops_postscriptDPS.c"
#include "ops_postscriptInternal.c"
#if VMINIT
    if (vmShared->wholeCloth) {
      RgstObject ("systemdict", rootShared->vm.Shared.sysDict);
      RgstObject ("shareddict", rootShared->vm.Shared.sharedDict);
      RgstObject ("SharedFontDirectory", rootShared->vm.Shared.sharedFontDirectory);

      RgstObject ("userdict", trickyUserDict);
      RgstObject ("errordict", trickyErrorDict);
      RgstObject ("statusdict", trickyStatusDict);
      RgstObject ("FontDirectory", trickyFontDirectory);

      MakePName("version", &version);

      if (!Known (rootShared->vm.Shared.sysDict, version)) {
	string  s;
	StrObj  v;

	if ((s = (string) GetCArg ('V')) == NULL) {
	  os_fprintf (os_stderr, "Version definition missing; use -V\n");
	  s = (string) "0.0";
	}
	v = MakeStr (s);
	v.access = rAccess;
	Def (version, v);
      }
      Begin (rootShared->vm.Shared.internalDict);
      RgstExplicit ("abort", PSAbort);
      End ();
    }
#endif VMINIT
    break;
    endswitch
  }
}

public procedure PostScriptInit(reason)
  InitReason reason;
{
  StateInit(reason);
  ControlInit(reason);
  PSSpaceInit(reason);
  ContextOpsInit(reason);
  CustomOpsInit(reason);
}
