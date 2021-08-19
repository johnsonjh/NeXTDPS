/*
  basictypes.h

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
Ivor Durham: Fri Aug 12 12:14:09 1988
Ed Taft: Mon May  9 16:25:09 1988
Joe Pasqua: Fri Oct 16 11:36:21 1987
Jim Sandman: Tue Mar 15 13:15:55 1988
Perry Caro: Tue Nov  1 15:37:46 1988
End Edit History.
*/

#ifndef	BASICTYPES_H
#define	BASICTYPES_H

#include PUBLICTYPES

/* Machine Units */

typedef long int word, *wordptr;

#define chPerPg 1024		/* VAX specific */
#define chPerWd 4
#define wdPerPg chPerPg/chPerWd

/* the definition of basic PostScript objects */

#define	nullObj		0
#define	intObj		1
#define	realObj		2
#define	nameObj		3
#define	boolObj		4
#define	strObj		5
#define	stmObj		6
#define	cmdObj		7
#define	dictObj		8
#define	arrayObj	9
#define	fontObj		10
/* future 		11 */
/* future		12 */
#define	pkdaryObj	13
/* obsolete objSeq	14 */
#define	escObj		15
#define	nBaseObTypes	16	/* 4 bit field */

#define	objMark		16
#define	objSave		17
#define	objGState	18
#define	objCond		19
#define	objLock		20
#define	objNameArray	21
#define nEscObTypes	6
#define	nObTypes	(nBaseObTypes + nEscObTypes)

#define	nBitVectorBits 32
typedef Card32 BitVector;

typedef Card8 Level;

/* Access codes are relevant only in objects of type strObj, stmObj,
   aryObj, and pkdaryObj. The access of a dictionary is kept in its
   body, not in the DictObj. */

typedef BitField Access;

#define	nAccess	0
#define	rAccess	1
#define	wAccess	2
#define	xAccess	4
#define	aAccess	7

#define	Lobj	0	/* tag for literal object */
#define	Xobj	1	/* tag for executable object */

/* The following define opaque pointers for objects stored in VM */

typedef struct _t_Object	*PObject;
typedef struct _t_DictBody	*PDictBody;
typedef struct _t_NameEntry	*PNameEntry;
typedef struct _t_StmBody	*PStmBody;
typedef struct _t_GenericBody	*PGenericBody;
typedef struct _t_GState	*PGState;
typedef struct _t_Lock		*PLock;
typedef struct _t_Condition	*PCondition;
typedef struct _t_NameArrayBody	*PNameArrayBody;

/* Object definition */

typedef struct _t_Object {
  BitField	tag:1;
  Access	access:3;	/* Access is actually a BitField too */
  BitField	type:4;
  BitField	shared:1;
  BitField	seen:1;
  BitField	pad:2;
  BitField	level:4;
  BitField	length:16;
  union {
   /* null */	Int32		nullval;
   /* int */	Int32		ival;
   /* real */	real		rval;
   /* name */	PNameEntry	nmval;
   /* bool */	boolean		bval;
   /* str */	charptr		strval;
   /* stm */	PStmBody	stmval;
   /* cmd */	PNameEntry	cmdval;
   /* dict */	PDictBody	dictval;
   /* array */	PObject		arrayval;
   /* pkdary */	PCard8		pkdaryval;
   /* mark */	Int32		markval;
   /* save */	Int32		saveval;
   /* font */	Int32		fontval;
   /* generic */PGenericBody	genericval;
   /* gstate */ PGState		gstateval;
   /* cond */   PCondition	condval;
   /* lock */   PLock		lockval;
   /* namearray*/PNameArrayBody	namearrayval;
  }		val;
}	Object,
	NullObj,	*PNullObj,
	IntObj,		*PIntObj,
	RealObj,	*PRealObj,
	NameObj,	*PNameObj,
	BoolObj,	*PBoolObj,
	StrObj,		*PStrObj,
	StmObj,		*PStmObj,
	CmdObj,		*PCmdObj,
	DictObj,	*PDictObj,
	AryObj,		*PAryObj,
	MarkObj,	*PMarkObj,
	PkdaryObj,	*PPkdaryObj,
	AnyAryObj,	*PAnyAryObj, /* either AryObj or PkdAryObj */
	SaveObj,	*PSaveObj,
	FontObj,	*PFontObj,
	GenericObj,	*PGenericObj,
	GStateObj,	*PGstateObj,
	CondObj,	*PCondObj,
	LockObj,	*PLockObj,
	NameArrayObj,	*PNameArrayObj;

/* following macros are useful for contructing Objects */

#define	LNullObj(o)	o = iLNullObj
#define	LIntObj(o,v)	o = iLIntObj; (o).val.ival=(v)
#define	LRealObj(o,v)	o = iLRealObj; (o).val.rval=(v)
#define	LNameObj(o,v)	o = iLNameObj; (o).val.nmval=(v)
#define	LBoolObj(o,v)	o = iLBoolObj; (o).val.bval=(v)
#define	LStrObj(o,l,v)						\
	o = iLStrObj;	(o).level=LEVEL; (o).length=(l);	\
	(o).val.strval=(v); (o).shared = vmCurrent->shared
#define	LStmObj(o,i,s)						\
	o = iLStmObj;	(o).level=LEVEL; (o).length=(i);	\
	(o).val.stmval=(s); (o).shared = vmCurrent->shared
#define	LDictObj(o,v)						\
	o = iLDictObj;	(o).level=LEVEL; (o).val.dictval=(v);	\
	(o).shared = vmCurrent->shared
#define	LAryObj(o,l,v)						\
	o = iLAryObj;	(o).level=LEVEL; (o).length=(l);	\
	(o).val.arrayval=(v); (o).shared = vmCurrent->shared
#define	LPkdaryObj(o,l,v)					\
	o = iLPkdaryObj; (o).level=LEVEL; (o).length = (l);	\
	(o).val.pkdaryval=(v); (o).shared = vmCurrent->shared
#define	LMarkObj(o)	o = iLMarkObj
#define	LSaveObj(o,v)						\
	o = iLSaveObj;	(o).level=LEVEL; (o).val.saveval=(v)
#define	LFontObj(o,v)						\
	o = iLFontObj; (o).val.fontval=(v)
#define	XNameObj(o,v)	o = iXNameObj; (o).val.nmval=(v)
#define	XStrObj(o,l,v)						\
	o = iXStrObj;	(o).level=LEVEL; (o).length=(l);	\
	(o).val.strval=(v); (o).shared = vmCurrent->shared
#define	XCmdObj(o,i,v)						\
	o = iXCmdObj; (o).val.cmdval=(i); (o).length=(v)
#define	XAryObj(o,l,v)						\
	o = iXAryObj;	(o).level=LEVEL; (o).length=(l); 	\
	(o).val.arrayval=(v); (o).shared = vmCurrent->shared
#define	XPkdaryObj(o,l,v)					\
	o = iXPkdaryObj; (o).level=LEVEL; (o).length = (l);	\
	(o).val.pkdaryval=(v); (o).shared = vmCurrent->shared
#define	LLockObj(o,v)					\
	o = iLLockObj; (o).level=LEVEL;	\
	(o).val.lockval=(v); (o).shared = vmCurrent->shared
#define	LCondObj(o,v)					\
	o = iLCondObj; (o).level=LEVEL;	\
	(o).val.condval=(v); (o).shared = vmCurrent->shared
#define	LGStateObj(o,v)					\
	o = iLGStateObj; (o).level=LEVEL;	\
	(o).val.gstateval=(v); (o).shared = vmCurrent->shared
#define	LGenericObj(o,t,v)					\
	o = iLGenericObj; (o).level=LEVEL; (o).length = (t);	\
	(o).val.genericval=(v); (o).shared = vmCurrent->shared
#define	LNameArrayObj(o,v) LGenericObject(o,objNameArray,v)

#ifndef LEVEL
#define	LEVEL ((vmCurrent->shared) ? 0 : level)
#endif

extern readonly NullObj	iLNullObj;
extern readonly IntObj	iLIntObj;
extern readonly RealObj	iLRealObj;
extern readonly NameObj	iLNameObj, iXNameObj;
extern readonly BoolObj	iLBoolObj;
extern readonly StrObj	iLStrObj, iXStrObj;
extern readonly StmObj	iLStmObj;
extern readonly CmdObj	iXCmdObj;
extern readonly DictObj	iLDictObj;
extern readonly AryObj	iLAryObj, iXAryObj;
extern readonly PkdaryObj	iLPkdaryObj, iXPkdaryObj;
extern readonly MarkObj	iLMarkObj;
extern readonly SaveObj	iLSaveObj;
extern readonly FontObj	iLFontObj;
extern readonly GStateObj	iLGStateObj;
extern readonly LockObj	iLLockObj;
extern readonly CondObj	iLCondObj;
extern readonly GenericObj	iLGenericObj;

/* Initialization and cleanup */

/* During startup, system init procs are called with "init", "romreg", and
   "ramreg", in that order. During sysin, system init procs are called
   with "goaway" before the VM is replaced and "restart" afterward.
   A downloaded program's start proc is called with "download" immediately
   after downloading. During sysin, start procs in downloaded programs
   in the old VM are called with "goaway" before the VM is replaced;
   start procs in downloaded programs in the new VM are called with
   "restart" after the new VM is brought in. The "relocate" reason
   applies only to downloaded programs.

   Note: system init procs that are called conditionally based on vXXX
   runtime switches may or may not be called with "init", since the
   switches aren't known for certain until the VM is started. */

typedef enum {
  init,				/* data structure initialization (pre-VM) */
  romreg,			/* ROM VM allocation, defs into systemdict */
  ramreg,			/* RAM VM allocation, defs into userdict */
  download,			/* initialize newly-downloaded program */
  goaway,			/* clean up before reloading VM */
  restart,			/* reinitialize after reloading VM */
  relocate			/* reinitialize after relocating VM */
} InitReason;

#endif	BASICTYPES_H
/* v005 durham Sun Feb 7 12:08:10 PST 1988 */
/* v006 sandman Thu Mar 10 09:32:23 PST 1988 */
/* v007 durham Fri Apr 8 11:04:54 PDT 1988 */
/* v008 durham Wed Jun 15 11:16:46 PDT 1988 */
/* v009 durham Thu Aug 11 12:59:29 PDT 1988 */
/* v010 caro Tue Nov 1 15:41:32 PST 1988 */
/* v011 byer Tue May 16 11:12:33 PDT 1989 */
