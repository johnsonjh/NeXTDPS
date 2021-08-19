/*
  vm.h

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
Scott Byer: Fri May 19 12:48:11 1989
Ivor Durham: Sat May  6 16:43:09 1989
Ed Taft: Sun Dec 17 14:53:56 1989
Joe Pasqua: Mon Jan  9 13:38:03 1989
Jim Sandman: Mon Oct 17 10:19:20 1988
Perry Caro: Fri Nov  4 11:53:20 1988
William Bilodeau: Tue Nov 22 14:48:36 1988
End Edit History.
*/

#ifndef	VM_H
#define	VM_H

#include BASICTYPES
#include ENVIRONMENT
#include STREAM

/* 
 * VM version number -- change whenever VM data structure definitions
 * are modified. This is for the purpose of checking for compatibility
 * between the producer and consumer of a saved VM. Note: you may also
 * need to change field descriptions in vm_reverse.c.
 */

#define VMVERSION ((SWITCHESVERSION*32) + 3)

/*
 * Configuration switches
 *
 * Switches derived from ISP, OS, and STAGE are in environment.h;
 * others are here.
 */

#if ISP==isp_unknown || OS==os_unknown || STAGE==stage_unknown
ConfigurationError("Unknown ISP, OS, or STAGE");
#endif

/*
 * SPACE compiles code for detailed monitoring of the storage allocator
 */

#ifndef SPACE
#define SPACE 0
#endif

/*
 * Root Interface
 */

typedef struct _t_SR *PSR;	/* Opaque pointer to save/restore machinery */
typedef struct _t_FinalizeNode *PFinalizeNode;	/* Ditto */

/* Note that elements are aligned according to PREFERREDALIGN = 4 */

typedef struct _t_VMRoot {
  GenericID spaceID;
  AryObj  trickyDicts;		/* array of real surrogates for tricky dicts */
  PFinalizeNode finalizeChain;	/* chain of finalizable objects allocated in
				   this VM, in decreasing save level order */
  NameArrayObj nameMap;		/* name index to NameEntry map */
  PStmBody stms;		/* list of StmBody for this space */
  BitField version:8;
  BitField shared:1;
  BitField pad:(32-8-1);
  union {
    struct {
      integer stamp;		/* Used in stmStamps only */
      DictObj sysDict,
              sharedDict,
              internalDict,
	      sharedFontDirectory;
      AryObj  param,
	      regNameArray,	/* array of arrays of registered names */
	      regOpNameArray;	/* array of arrays of operator names */
      StrObj  regOpIDArray;	/* OpSet IDs, represented as array of Int16 */
      cardinal cmdCnt;
      cardinal dictCount;
      NameArrayObj nameTable;	/* name hash table; type is objNameArray */
      Card16  hashindexmask;	/* mask for name index */
      Card16  hashindexfield;	/* field width for hash table */
      charptr downloadChain;
    } Shared;

    struct {
      PSR srList;		/* save/restore list */
      Level   level;		/* current save level */
    } Private;	/* Cannot be private because that means static! */
  } vm;
} VMRoot, *PVMRoot;

/* rootShared->.Shared.param array indices */

#define rpFontKey 0		/* font decryption key */
#define rpStreamKey 1		/* stream decryption key */
#define rpInternalKey 2		/* internaldict key */
#define MAXrootParam 4		/* leave a couple spares */

/* root->trickyDicts array indices */

#define	tdDummy 0		/* Not used -- algorithm simplicity */
#define tdUserDict 1		/* userdict */
#define tdErrorDict 2		/* errordict */
#define tdStatusDict 3		/* statusdict */
#define	tdFontDirectory 4	/* FontDirectory */

#define MAXlevel 15		/* max save level */

/* Inline Procedures */

#define SysDictGetP(k,v)\
        DictGetP(rootShared->vm.Shared.sysDict,k,v)

/*
 * Part of Dictonary Interface (!) because of types of results of VM access
 * (Taken from LANGUAGE interface.)
 */

typedef struct _t_KeyVal {
  Object  key;
  Object  value;
} KeyVal, *PKeyVal;

typedef struct _t_DictBody {
  BitVector	bitvector;	/* 1-bit non-unique ID for dict		   */
  PKeyVal	begin;		/* beginning keyval table		   */
  PKeyVal	end;		/* end of keyval table			   */
  Card16	curlength;	/* current # of keyvals			   */
  Card16	maxlength;	/* maximum # of keyvals			   */
  Card16	curmax;		/* current maximum # of keyvals		   */
  Card16	size;		/* size of keyval table			   */
  BitField	shared: 1;	/* shared or private vm			   */
  BitField	seen: 1;	/* trace bit for garbage collection	   */
  BitField	isfont: 1;	/* this dict definess a font		   */
  BitField	tricky: 1;	/* this  is the real  dict corresponding to a
				   tricky dict				   */
  BitField	level: 4;	/* current save-restore level		   */
  BitField	privTraceKeys: 1; /* Trace keys during private GC?         */
  BitField	unused: 4;	/* unused				   */
  BitField	access: 3;	/* protection mechanism			   */
} DictBody;

/* Inline procedures */

#define	TrickyDict(pObj) ((pObj)->length != 0)
/* Returns true iff this is a TrickyDict. PDictObj pObj */

#define	DoDictXlat(pObj) \
  ((PDictObj) & rootPrivate->trickyDicts.val.arrayval[(Int32)((pObj)->val.dictval)])

#define	XlatDictRef(pObj) \
	( TrickyDict(pObj) ? (DoDictXlat(pObj)) : (pObj) )
/* Normally, simply returns pObj. If *pObj is tricky, instead returns
   a pointer to the corresponding real dictionary object in the per-space
   Root structure. */

/*
 * Common part of generic object representation
 */

typedef	struct _t_GenericBody {
  BitField	type:8;
  BitField	level:4;
  BitField	seen:1;
  BitField	pad:3;
  BitField	length:16;	/* including this header */
} GenericBody;

/*
 * Name object value, including name cache
 */

typedef struct _t_NameEntry {
  PNameEntry	link;		/* -> next NameEntry in hash chain	   */
  charptr	str;		/* -> name string			   */
  BitVector	vec;		/* set  of dicts in  which  name  appears  as
				   key					   */
  GenericID	ts;		/* if ts  ==   global timestamp, this  is the
				   top-level binding for name		   */
  PKeyVal	kvloc;		/* if not NIL, -> some valid binding	   */
  PDictBody	dict;		/* -> dict in which kvloc binding exists   */
  Card16	nameindex;	/* encoding for name, used in packedarrays */
  Card16 /* CIOffset */ ncilink; /* chain of font CIItems for this name */
  Card8		strLen;		/* length of name string		   */
  BitField	seen: 1;	/* trace bit for garbage collection	   */
  BitField	unused: 7;	/* unused				   */
} NameEntry;

typedef struct _t_NameArrayBody {
  GenericBody header;		/* common header */
  Card16 length;		/* number of PNameEntry in array */
  Card16 unused;		/* alignment */
  PNameEntry nmEntry[1];	/* array of PNameEntry */
} NameArrayBody;

/*
 * Stream (file) objects
 */

typedef struct _t_StmBody {
  PStmBody link;	/* link to another StmBody in same space */
  Stm stm;		/* -> underlying stream; NIL if has been closed */
  Card16 generation;	/* StmObj.length must match to be valid;
			   incremented when stm is closed */
  BitField level:8;	/* save level at which stream was opened */
  BitField shared:1;	/* shared or private vm */
  BitField seen:1;	/* seen flag for garbage collector */
  BitField unused:6;
} StmBody;

/*
 * VM Data Types
 */

typedef struct _t_VMSegment *PVMSegment;
typedef struct _t_Recycler *PRecycler;
typedef struct _t_VMPrivateData *PVMPrivateData;

typedef struct _t_VMStructure {
  integer password;
  BitField shared:1;
  BitField wholeCloth:1;	/* VM was created from whole cloth */
  BitField valueForSeen:1;
  BitField collectThisSpace:1;	/* Perform auto-collection */
  BitField pad:(32 - 4);
  PCard8  free,			/* First free byte in current segment */
          last;			/* Last free byte in current segment */
  PVMSegment head,		/* First allocated segment in sequence */
          tail,			/* Last  allocated segment in sequence */
          current,		/* Current segment for allocations */
          reserve;		/* Emergency segment when no more VM */
  Card32  expansion_size;
  PRecycler recycler;		/* Recycler state */
  PVMPrivateData priv;		/* Data internal to VM machinery */
  Card32  allocCounter;		/* bytes "permanently" alloced since reclaim */
  Card32  allocThreshold;	/* Threshold for auto reclamations	 */
} VMStructure, *PVM;

/* Exported Procedures */

#define _vm extern

_vm	PCard8		AllocAligned(/* integer nBytes */);
_vm	PCard8		AllocChars(/* integer nBytes */);
_vm	procedure	AllocPArray(/*length:cardinal, pob:PObject*/);
_vm	procedure	AllocPDict(/*size:cardinal, pob:PObject*/);
_vm	procedure	AllocPStream(/* PStmObj pob */);
_vm	procedure	AllocPString(/*length:cardinal, pob:PObject*/);
_vm	PCard8		AppendVM(/*length:cardinal,ptrtostring*/);
_vm	procedure	ClaimPreallocChars(/*vmOffset:PCard8,nBytes:integer*/);
_inline	/*boolean	CurrentShared ();*/
_vm	procedure	DestroyVM (/* PVM */);
_vm	procedure	ExpandVM();
_inline /*PCard8	GetVMFirst();*/
_inline /*PCard8	GetVMLast();*/
_vm	procedure	LoadVM (/* PVM */);
_vm	procedure	NewRelocationEntry (/* PCard8 *address */);
_vm	PCard8		PreallocChars(/* integer nBytes */);
_vm	procedure	RgstGCContextProcs(/* (see gc spec) */);
_vm	procedure	RgstPackedArrayRelocator(/*procedure*/);
_vm	procedure	RgstStackChecker(/*procedure*/);
_vm	procedure	RgstRstrProc(/*procedure*/);
_vm	procedure	RgstSaveProc(/*procedure*/);
_vm	procedure	RgstSaveSemaphoreProc (/* PVoidProc */);
_vm	procedure	SetShared(/* boolean shared */);
_inline	/*		StringText(StrObj,string);*/
_inline /*PObject	VMArrayPtr(AryObj);*/		
_vm	procedure	VMCarCdr(/*PAnyAryObj,PObject*/);
_vm	procedure	VMCopyArray(/*src,dst:AryObj*/);
_vm	procedure	VMCopyString(/*src,dst:StrObj*/);
_inline	/*character	VMGetChar(StrObj,integer);*/
_inline	/*		VMGetDict(PDictBody,DictObj);*/
_inline	/*Object	VMGetElem(AryObj,integer);*/
_inline	/*		VMGetKeyVal(PKeyVal,PKeyVal);*/
_vm	procedure	VMGetText(/*StrObj,string*/);
_inline	/*		VMGetValue(PObject,PKeyVal);*/
_vm	procedure	VMInit(/*InitReason*/);
_vm	procedure	VMObjForPString(/*string,PStrObj*/);
_vm	procedure	VMPutChar(/*StrObj,integer,character*/);
_vm	procedure	VMPutDict(/*DictObj,PDictBody*/);
_vm	procedure	VMPutDKeyVal(/*PKeyVal,PKeyVal*/);
_vm	procedure	VMPutDValue(/*PKeyVal,PObject*/);
_vm	procedure	VMPutElem(/*AryObj,cardinal,Object*/);
_vm	procedure	VMPutNChars(/*StrObj,string,count*/);
_vm	procedure	VMPutText(/*StrObj,string*/);
_vm	procedure	VMSetRAMAlloc();
_vm	procedure	VMSetROMAlloc();
_vm	procedure	VMSetStackChecker(/*CheckingProc*/);
/* the following procedures are "externed" for access via macros */
_vm	procedure	MFree(/*PCard8*/); /* NEVER call directly */
_vm	PCard8		MNew(/*count,size:integer*/); /* NEVER call directly */

extern procedure AllocGenericObject (/* Card8 Type; cardinal Size; PObject pObject */);
 /*
    Allocate a generic object body of Size bytes, modifying the object pObject
    to have the given Type and address of the body.  Size includes the
    size of a GenericObjectHeader.

    pre: Size >= sizeof (GenericObjectHeader)
    post: *pObject->type == Type, *pObject->val.???? == address of new body
	  *pObject->access == aAccess
  */

extern procedure AllocPName(/* Card16 length, PNameObj pno */);
/* Allocates a NameEntry and a name string of the specified length.
   Stores the resulting NameObj in *pno. This procedure takes care of
   performing allocations in the correct VM segments. The resulting
   NameEntry has its str, strLen, and seen fields initialized properly;
   all other fields zeroed.
 */

extern PVM CreateVM(/* boolean shared, createSegment */);
/* Creates a VMStructure of the specified type (private or shared).
   If createSegment is true, creates an initial segment for the VM;
   the segment's size is obtained from the sizes interface or the
   appropriate command line option. Regardless of createSegment,
   always creates a reserve segment for the VM.
 */

extern boolean StartVM(/* Stm vmStm */);
/* Attempts to start up an existing shared VM. If vmStm is not NIL,
   it is assumed to be positioned at the beginning of a VM file that
   is to be read in; it leaves vmStm open and positioned at the end
   of the VM file. If vmStm is NIL, StartVM may attempt to start up
   a VM that is already bound into the code, or it may fail.
   Returns true iff successful.
 */

extern procedure InitMakeVM();
/* Registers the optional makevm operator */

extern procedure VMPutGeneric (/* PObject pObject; PCard8 newBody */);
 /*
    Assign the new body (exclusive of the header) to the generic object.

    pre: pObject is a valid generic object && newBody addresses data the
	 length of which is the same as the length of the current body
	 beyond the header.

    post: Body of pObject replaced by newBody.

    NOTE: This procedure knows nothing about the representation of the
    body. If the body contains Objects, it is the caller's responsibility
    to ensure that those Objects are all shared if the destination
    GenericObj is shared.
  */

extern procedure VMCopyGeneric (/* Object Src, Dst */);
 /*
    Copy the body of generic object Src into the body of Dst.

    pre: Src && Dst are valid generic objects of the same type and size.

    post: The body of Dst is a duplicate of the body of Src.

    NOTE: This procedure knows nothing about the representation of the
    body. If the body contains Objects, it is the caller's responsibility
    to ensure that those Objects are all shared if the destination
    GenericObj is shared.
  */

extern procedure WriteContextParam (/*
	char *param, *newValue; integer size;
	PVoidProc proc (char *param; integer size) */);
 /*
   Copies size bytes from *newValue to *param.  If the value is changing
   and the old value needs to be saved for a subsequent "restore",
   WriteContextParam saves the old vlaue and associates proc with it.
   Later, when a "restore" is done, the old value is restored automatically
   and then proc is called (if proc is not NIL).
  */

extern boolean AddressValidAtLevel (/* char *address; Level level */);
 /*
   Returns true if, and only if, the given address is valid in VM at the
   specified save level.  Otherwise returns false.
  */

/*
   Call-backs for object finalization and related operations

   IMPORTANT: finalization is meaningful only for composite objects.
   At present it is not implemented for string, array, or packedarray
   objects. Objects reclaimed by the recycler are NOT finalized.
*/

typedef enum {
  /* Following reasons imply that the object's value is being reclaimed
     and that the object itself is becoming invalid */
  fr_restore,		/* reclaimed by a restore */
  fr_privateReclaim,	/* reclaimed by garbage collection of private VM */
  fr_sharedReclaim,	/* reclaimed by garbage collection of shared VM;
			   the object itself may be private or shared */
  fr_destroyVM,		/* reclaimed by destruction of private VM */

  /* Following reasons do NOT imply reclamation; the object remains valid.
     These apply only to Generic objects. Note that the overwrite event
     may be generated automatically by a restore (however, copy events are
     not generated for the value that is saved and restored) */
  fr_overwrite,		/* object's value is about to be overwritten */
  fr_copy,		/* object's value has just been copied from
			   another; this is the copy */

  /* following reasons do NOT imply reclamation; they indicate that the
     object has been encountered during enumeration of all finalizable
     objects on the specified occasion */
  fr_privateEnum,	/* enumeration at end of a private VM collection */
  fr_sharedEnum,	/* enumeration at end of a shared VM collection */
  } FinalizeReason;

/* Commonly used FinalizeReason sets (see VMRgstFinalize) */

#define frset_reclaim (1<<fr_restore | 1<<fr_privateReclaim | \
  1<<fr_sharedReclaim | 1<<fr_destroyVM)
    /* call when object's value is reclaimed for any reason */

#define frset_overwriteCopy (1<<fr_overwrite | 1<<fr_copy)
    /* call when object's value is overwritten or copied */

typedef Card16 FinalizeReasonSet;  /* assert: fewer than 16 FinalizeReasons */
typedef procedure (*FinalizeProc)(/* Object obj, FinalizeReason reason */);

extern procedure VMRgstFinalize(/*
  Card8 type, FinalizeProc proc, FinalizeReasonSet reasonSet */);
/* Registers a finalization procedure to be called for objects of the
   specified type. reasonSet is a mask in which the positions of
   one bits (counting from the low-order bit) correspond to FinalizeReasons
   for which the finalization procedure is to be called.

   If VMRgstFinalize has been called for a particular type, the VM
   machinery keeps track of all instances of values of that type.
   During any of the occasions indicated by one bits in reasonSet,
   it calls (*proc)(obj, reason) for each object of that type;
   reason identifies the occasion.
*/

/* Inline Procedures */

#define StringText(s,t)\
	VMGetText((s),(t))

#define	CurrentShared()	(vmCurrent->shared)

/*
  Definition of inline procedures for vm access -- either inline or
  a procedure call, depending on whether runtime interpretation of
  VM access is required
 */

#define GetVMFirst() VMPtr(vmCurrent->free)
#define GetVMLast() VMPtr(vmCurrent->last)

#define VMGetChar(so,i)	((so).val.strval)[i]
#define VMGetDict(dp,d) *(dp) = *((d).val.dictval)
#define VMGetElem(ao,i) (((ao).val.arrayval)[i])
#define VMGetKeyVal(kvp,kvo) *(kvp) = *(kvo)
#define VMGetValue(obp,kvo) *(obp) = (kvo)->value

#define ForAllNames(pno, pne, i) \
  for (pno = & (rootShared->vm.Shared.nameTable.val.namearrayval)->nmEntry[0],\
       i = (rootShared->vm.Shared.nameTable.val.namearrayval)->length; \
       --i >= 0; pno++) \
    for (pne = *pno; pne != NIL; pne = pne->link)
/* Enumerator for all NameEntries. Intended to be used as follows:
     PNameEntry *pno, pne; integer i;
     ForAllNames(pno, pne, i)
       {
       ... code to be executed once per NameEntry (pointed to by pne) ...
       }
*/

/*
 * vm_space interface.
 */

_inline /*		FREE(PCard8);*/
_inline	/*PCard8	NEW(count,size:integer);*/

/* Inline Procedures */

#define NEW(n,size)\
	MNew( (integer) (n), (integer) (size))

#define FREE(ptr)\
	MFree( (PCard8) (ptr) )

/*
 * Exported PostScript Context
 */

extern	PVM vmPrivate, vmShared, vmCurrent;

_vm	PVMRoot	rootShared, rootPrivate;

_vm	Level level;
_vm	Object NOLL;

#endif	VM_H
/* v007 durham Mon Nov 30 11:36:28 PST 1987 */
/* v008 durham Mon Feb 8 10:01:42 PST 1988 */
/* v009 sandman Thu Mar 10 09:37:58 PST 1988 */
/* v010 durham Fri Apr 8 11:36:51 PDT 1988 */
/* v011 durham Wed Jun 15 14:25:14 PDT 1988 */
/* v012 taft Wed Jul 27 16:56:03 PDT 1988 */
/* v013 caro Mon Nov 7 13:21:21 PST 1988 */
/* v014 pasqua Mon Dec 12 14:53:11 PST 1988 */
/* v015 pasqua Wed Jan 18 11:42:10 PST 1989 */
/* v016 bilodeau Fri Apr 21 16:13:31 PDT 1989 */
/* v017 durham Sat May 6 16:47:52 PDT 1989 */
/* v018 taft Fri Jul 7 15:50:24 PDT 1989 */
/* v020 taft Thu Nov 23 15:02:26 PST 1989 */
