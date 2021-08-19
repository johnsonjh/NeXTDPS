/*
  language.h

Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
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
Ivor Durham: Sun May  7 10:02:24 1989
Ed Taft: Sun Dec 17 18:47:54 1989
Joe Pasqua: Wed Feb  8 14:18:50 1989
Perry Caro: Mon Nov  7 14:54:57 1988
Jim Sandman: Tue Apr 11 15:29:52 1989
Mark Francis: Wed Mar 28 11:12:34 1990
End Edit History.
*/

#ifndef	LANGUAGE_H
#define	LANGUAGE_H

#include BASICTYPES
#include ENVIRONMENT
#include ORPHANS
#include RECYCLER
#include VM

/*
 * Interpreter interface 
 */

extern	boolean	psExecute(/* Object obj */);
extern	boolean	psExecuteCoroutine(/* Object obj */);

extern procedure SetAbort (/* integer reason */);
extern integer GetAbort ();
extern procedure NotifyAbort ();

extern procedure RgstContextProcs (
  /* PVoidProc monitorProc, yieldRequestProc, yieldTimeLimitProc,
     getNotifyCodeProc */);

extern procedure SetTimeLimit (/* Card32 limit */);
  /* Set the upper bound for the clock or execution unit count for the
     current context */

extern procedure IncrementTimeUsed (/* integer units */);
  /* Adds the given number of units to the current context count. */

#define	SetRealClockAddress(clockAddress) {pTimeSliceClock = clockAddress;}
  /*
    Set address of free-running clock for time slicing.  Must be set before
    initialising the language package otherwise time slicing will be based
    on the count of operators executed.
   */
 
extern PCard32	pTimeSliceClock;

/*
 * Generic Object Operations
 */

/* Exported Procedures */

#define _type extern

_type	integer		EPopInteger();
_type	procedure	EPopPReal(/*Preal*/);
_type	procedure	EPushInteger(/*integer*/);
_type	procedure	EPushPReal(/*Preal*/);
_type	boolean		Equal(/*a,b:Object*/);
_type	procedure	FixCd(/*Cd,PDevCd*/);
_type	boolean		PSPopBoolean();
_type	cardinal	PopCardinal();
_type	integer		PSPopInteger();
_type	procedure	PopP(/*PObject*/);
_type	procedure	PopPArray(/*PAryObj*/);
_type	procedure	PSPopPCd(/*PCd*/);
_type	procedure	PopPDict(/*PDictObj*/);
_type	procedure	PSPopPReal(/*Preal*/);
_type	procedure	PopPRString(/*PStrObj*/);
_type	procedure	PopPStream(/*PStmObj*/);
_type	procedure	PopPString(/*PStrObj*/);
_type	procedure	PSPushBoolean(/*boolean*/);
_type	procedure	PushCardinal(/*cardinal*/);
_type	procedure	PSPushPCd(/*PCd*/);
_type	procedure	PSPushInteger(/*integer*/);
_type	procedure	PSPushPReal(/*Preal*/);
_type	procedure	RRoundP(/*Preal,Preal*/);
_type	procedure	UnFixCd(/*DevCd, PCd*/);
#define PopBoolean	PSPopBoolean
#define PopInteger	PSPopInteger
#define PopPCd		PSPopPCd
#define PopPReal	PSPopPReal
#define PushBoolean	PSPushBoolean
#define PushInteger	PSPushInteger
#define PushPCd		PSPushPCd
#define PushPReal	PSPushPReal

extern integer GetObjFormat();
extern procedure SetObjFormat(/* integer */);
extern procedure RgstPARelocator();


/*
 * Dictionary Interface
 */

typedef	boolean (*PDictEnumProc)(/* char *, PKeyVal */);

/*
 * Definition of DictBody and PDictBody hidden in VM interface for now because
 * some VM procedure results are of type PDictBody.  This needs to be sorted
 * out properly.  Ditto KeyVal and PKeyVal.
 */ 

#define MAXdctCount MAXcardinal

/* Exported Procedures */

#define _dict extern

_dict	procedure	AllocCopyDict(/* DictObj from, integer increment,
					 PDictObj pResult */);
_dict	procedure	Begin(/* DictObj dict */);
_dict	procedure	CopyDict(/* DictObj from, PDictObj pto */);
_dict	procedure	Def(/* Object key, value */);
_dict	procedure	DictP(/* cardinal maxlength, PDictObj pdobj */);
_dict	procedure	DictForAll(/* DictObj dictOb, AryObj procOb */);
_dict	procedure	DictGetP(/* DictObj d, Object key, PObject pval */);
_dict	cardinal	DictLength(/* DictObj d */);
_dict	procedure	DictPut(/* DictObj d, Object key, value*/);
_dict	procedure	DictUnDef(/* DictObj dict, Object key */);
_dict	procedure	End();
_dict	PKeyVal		EnumerateDict(/* DictObj, PDictEnumProc, char * */);
_dict	procedure	ForceGetP(/* DictObj d, Object key, PObject pval */);
_dict	boolean		ForceKnown(/* DictObj d, NameObj name */);
_dict	procedure	ForcePut(/* DictObj d, Object key, value */);
_dict	procedure	ForceUnDef(/* DictObj dict, Object key */);
_dict	boolean		Known(/* DictObj d, NameObj name */);
_dict	boolean		Load(/* Object key, PObject pval */);
_dict	boolean		LoadName(/* PNameEntry key, PObject pval */);
_dict	procedure	PSEnd();
_dict	procedure	SetCETimeStamp(/* Card32 index */);

_dict	procedure	ResetNameCache(/* integer action */);
  /* Resets all entries in name cache, as specified by action:
     0 => just zap timestamps, thereby invalidating top-level bindings;
     1 => flush all bindings;
     2 => zap bitvectors in preparation for rebuilding them.
     3 => zap timestamps with the current timestamp.id.index
  */

_dict	boolean		SearchDict(/* PDictBody dp, Object key, PKeyVal *pResult */);
_dict	procedure	SetDictAccess(/* DictObj d, Access access */);

#if VMINIT
_dict	procedure	TrickyDictP(/* cardinal maxlength, PDictObj pdobj */);
  /* Creates dictionary with specified maxlength, allocates a tricky dict
     entry for it, saves the new dictionary in the trickyDict table, and
     sets *pdobj to be a tricky DictObj that refers to the entry. The
     current VM allocation mode must be shared, and the number of tricky
     dict entries is limited.
  */
#endif VMINIT

/*
 * Stack Interface
 */

/* Data Types */

/* This definition is here only so that the inline procs can	*/
/* reference it. It must not be refernced by any module (impl.	*/
/* or interface) outside of the language package.		*/
typedef struct _t_Stack {
  PObject head;		/* First free space in stack		*/
  PObject base;		/* Base of storage for the stack	*/
  PObject limit;	/* one past last useable object storage	*/
			/* pos in stack (end of our storage)	*/
} Stack, *PStack;

/* Exported Procedures */

#define _stack extern

_stack	procedure	ArrayFromStack(/*PAryObj,PStack*/);
_stack	procedure	ClearStack(/*PStack*/);
_stack	procedure	Copy(/*PStack,cardinal*/);
_stack	procedure	CopyStack(/* PStack src, dst; cardinal n */);
_stack	cardinal	CountStack(/*PStack,max:cardinal*/);
_stack	cardinal	CountToMark(/*PStack*/);
_stack	procedure	CreateStacks(/*cardinal*/);
_stack	procedure	DPopP(/*PObject*/);
_stack	procedure	DPushP(/*PObject*/);
_stack	procedure	DTopP(/*PObject*/);
_stack	procedure	EnumStack(/*PStack, callBackProc, clientData*/);
_stack	procedure	EPopP(/*PObject*/);
_stack	procedure	EPushP(/*PObject*/);
_stack	procedure	ETopP(/*PObject*/);
_inline	/*		IPop(PStack,PObject);*/
_inline	/*		IPopDiscard(PStack);*/
_inline	/*		IPopNotEmpty(PStack,PObject);*/
_inline	/*		IPush(PStack,Object);*/
_stack	procedure	Overflow(/*PStack*/);
_stack	procedure	PSClrToMrk();
_stack	procedure	PushP(/*PObject*/);
_stack	procedure	RstrStack(/*PStack,mark:PNode*/);
_stack	procedure	TopP(/*PObject*/);
_stack	procedure	Underflow(/*PStack*/);

/* Inline Procedures */

/* Inline stack operations. Note that IPop is different	*/
/* from Pop: it stores the result object through a ptr	*/
/* rather than returning it. In using IPopDiscard and	*/
/* IPopNotEmpty, the caller asserts that the stack is	*/
/* initially not empty.					*/

#define	IPopSimple(stack, pObj)		\
{					\
  if (stack->head <= stack->base)	\
    Underflow(stack);			\
  *pObj = *(--stack->head);		\
}

#define IPop(stack, pObj)		\
{					\
  IPopSimple(stack, pObj);		\
  RecyclerPop(pObj);			\
}

#define IPopOp(pObj)			\
{					\
  IPopSimple(opStk, pObj);		\
  RecyclerPop(pObj);			\
  IPushSimple(refStk, *pObj);		\
}

#if STAGE==DEVELOP
#define IPopDiscard(stack)		\
{					\
  Assert(stack->head > stack->base);	\
  stack->head--;			\
  RecyclerPop(stack->head);		\
}

#define IPopNotEmpty(stack, pObj)	\
{					\
  Assert(stack->head > stack->base);	\
  *pObj = *(--stack->head);		\
  RecyclerPop(pObj);			\
}

#else STAGE==DEVELOP
#define IPopDiscard(stack) {stack->head--; RecyclerPop(stack->head);}

#define IPopNotEmpty(stack, pObj)	\
{					\
  *pObj = *(--stack->head);		\
  RecyclerPop(pObj);			\
}
#endif STAGE==DEVELOP

#define IPushSimple(stack, obj)		\
{					\
  if (stack->head >= stack->limit)	\
     Overflow(stack);			\
  *(stack->head)++ = (obj);		\
}

#define IPush(stack, obj)		\
{					\
  IPushSimple (stack, obj);		\
  RecyclerPush(&obj);			\
}

/* Exported Data */

extern	PStack	opStk;
extern	PStack	execStk;
extern	PStack	dictStk;
extern	PStack	refStk;

/*
 * String Interface
 */

/* Constants */

#define MAXstringLength MAXcardinal
#define MAXtimeString 24	/* must be at least 24 on UNIX */
#define MAXnumeralString 35	/* must be at least 32 on a VAX */

/* Exported Procedures */

#define _string extern

_inline	/*StrObj	MakeStr(string);*/
_inline	/*StrObj	MakeXStr(string);*/
_string	procedure	PutString(/*from,cardinal,into*/);
_string	procedure	StrForAll(/*strOb,procOb*/);
_string	integer		StringCompare(/*a,b:StrObj*/);
_inline /*integer	StrLen(string);*/
_string	procedure	SubPString(/*StrObj,first,len:cardinal,PStrObj*/);

_priv	StrObj	makestring(/*string,obtype:cardinal*/);

/* Inline Procedures */

#define MakeStr(s)\
	makestring((s),Lobj)

#define MakeXStr(s)\
	makestring((s),Xobj)


/*
 * Name Interface
 */

/* Constants */

#define MAXnameLength 127

/* Exported Procedures */

extern	procedure	AllocPNameArray(/* Card16 len, PNameArrayObj pObj */);
extern	procedure	FastName(/* str:string, len:cardinal, pobj:PNameObj */);
extern	procedure	NameToPString(/*NameObj,PObject*/);

/*
 * Stream Interface
 */

#include BASICTYPES
#include STREAM

/* Data Types */

/* Tell MakeDecryptionStm what kind of stream to construct */

typedef enum {
  hexStream,		/* stream will decrypt hex characters */
  binStream		/* stream will decrypt binary characters */
  } DecryptionType;

#define _stream extern

/* Exported Procedures */

_stream	procedure	CloseFile(/*StmObj,force:boolean*/);
  /* Closes file given StmObj; the StmObj (and all copies of it) becomes
     invalid. If the stream is serially reusable, the underlying stream
     is not actually closed; instead, its EOF status is reset (input) or
     an EOF is sent (output), and it is left in the StmBody.
     The force argument can be used to force the stream closed. */

_stream	procedure	CreateFileStream(/*name:StrObj,access:string,PStmObj*/);
_stream Stm		CreateFileStm(/*name:StrObj,access:string*/);
_stream procedure	CrFile(/*PStmObj*/);
_stream	boolean		EncryptedStream(/*StmObj*/);
_inline /*Stm		GetStream(StmObj);*/
_stream	Stm		InvlStm(/*sob: StmObj*/);
_stream	procedure	MakePStm(/*Stm,tag:cardinal,PObject*/);
_stream	procedure	RgstDiskProc(/*procedure*/);
_stream	procedure	SetStdIO(/*in,out,err:StmObj*/);

_stream	procedure	StreamError(/*Stm*/);
  /* Called by anyone who wants to raise PSError(ioerror). This procedure
     resets the stream error indication so that the error happens only once.
     It does not return. */

extern procedure StmCtxDestroy();
/* Called during context destruction to close all streams opened by
   the context.  rootPrivate must be valid for this to be called.
 */

_stream Stm		MakeDecryptionStm(/*StmObj src, DecryptionType type*/);
  /* Creates a decryption stream which feeds from the underlying 'src'
     stream object. 'type' determines which stream procedures to use */

_stream longcardinal	GetStmDecryptionKey(/*stm:Stm*/);
  /* Called by anyone who needs to remember the decryption key
     value at specific positions in an encrypted file. It is
     assumed the stm passed is an encrypted stream. */

_stream procedure	SetStmDecryptionKey(/*stm:Stm,key:longcardinal*/);
  /* Called by anyone who needs to do random file access on an
     encrypted stream, this procedure sets the stream's current
     decryption key state so decryption can be correctly performed */

/* Inline Procedures */

#define GetStream(s) \
  ((s).length == (s).val.stmval->generation) ? (s).val.stmval->stm : InvlStm(s)

/*
 * Encoded Number String Interface
 */

/* Data Structures */

typedef struct _t_NumStrRec {
  character *str;
  Int16   len;
  boolean hi_first;
  Int16   shift;
} NumStrRec, *NumStr;


typedef struct _t_NumStrm {
  procedure (*GetReal) ( /* PNumStrm, preal */ );
  Fixed (*GetFixed) ( /* PNumStrm */ );
  AryObj  ao;
  PObject aptr;
  character *str;
  Int16   len;
  BitField   scale:8;
  BitField   bytespernum:8;
} NumStrm, *PNumStrm;

/* Exported Procedures */

extern procedure SetupNumStrm( /* PObject, PNumStrm */ );

/* the following routines are for the userpath cache (private interface?) */

extern integer SizeNumStrmForCache( /* PNumStrm */ );
extern procedure CopyNumStrmForCache(/* PNumStrm, PCard32*, string* */ );
extern boolean EqNumStrmCache(/* PNumStrm, PCard32, string */ );

/* Exported Per-Context Data	*/

/* The following struct contains per-context data exported thru	*/
/* the language interface. Add new items of this type here.	*/
typedef struct _t_PubLangCtxt {
  NameObj _psERROR;	/* Interpreter interface		*/
  } PubLangCtxt, *PPubLangCtxt;

extern	PPubLangCtxt pubLangCtxt;
  /* Ptr to language pkg context assoc'd with current context	*/

/*
   The following macros are provided for easy access to the fields of the
   language context struct.
 */

#define	psERROR	(pubLangCtxt->_psERROR)

#endif	LANGUAGE_H
/* v006 pasqua Wed Oct 21 12:47:02 PDT 1987 */
/* v007 pasqua Mon Nov 30 17:21:18 PST 1987 */
/* v008 durham Sun Feb 7 13:22:04 PST 1988 */
/* v009 sandman Thu Mar 10 09:37:12 PST 1988 */
/* v010 durham Fri Apr 8 11:41:59 PDT 1988 */
/* v011 durham Wed Jun 15 14:22:34 PDT 1988 */
/* v012 taft Wed Jul 27 16:57:17 PDT 1988 */
/* v013 durham Fri Sep 23 12:58:21 PDT 1988 */
/* v014 caro Mon Nov 7 16:51:04 PST 1988 */
/* v015 pasqua Thu Dec 15 11:10:45 PST 1988 */
/* v016 pasqua Wed Jan 18 11:43:36 PST 1989 */
/* v017 sandman Tue Apr 4 14:17:47 PDT 1989 */
/* v017 bilodeau Fri Apr 21 16:11:49 PDT 1989 */
/* v018 durham Sat May 6 16:50:46 PDT 1989 */
/* v018 francis Thu Jul 13 15:29:18 PDT 1989 */
/* v019 francis Thu Jul 13 15:51:31 PDT 1989 */
/* v020 sandman Mon Oct 16 17:06:26 PDT 1989 */
/* v021 taft Thu Nov 23 15:01:09 PST 1989 */
