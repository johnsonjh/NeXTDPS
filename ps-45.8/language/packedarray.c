/*
  packedarray.c

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

Original version: John Gaffney: November 13, 1984
Edit History:
Larry Baer: Fri Nov 17 10:37:34 1989
John Gaffney: Thu Jan 10 12:20:57 1985
Ed Taft: Sun Dec 17 19:08:17 1989
Chuck Geschke: Thu Oct 10 06:30:07 1985
Doug Brotz: Mon Jun  2 10:33:33 1986
Joe Pasqua: Thu Jul 13 13:59:01 1989
Ivor Durham: Mon May  8 11:13:34 1989
Jim Sandman: Mon Oct 16 11:12:43 1989
Bill Bilodeau Tue Apr 25 13:19:46 PDT 1989
End Edit History.
*/


#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include GC
#include LANGUAGE
#include VM
#include RECYCLER

#include "array.h"
#include "exec.h"
#include "name.h"
#include "opcodes.h"

/* quick dispatch table for decoding object from packed arrays */

/*	We return contiguously defined codes from this table
 *	so the compiler will generate a corresponding dispatch
 *	table for the case statements found in DecodeObj.
 */

public readonly Code opType[256] = 
{ INVALIDTYPE, ESCAPETYPE, 
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, /* 4 * 16 = 64 */
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE,
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE,
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE,
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE,
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE,
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE,
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE,
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE,
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE,
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE,
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE,
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE,
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE,
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE,
  LITNAMETYPE, LITNAMETYPE, LITNAMETYPE, LITNAMETYPE,
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, /* 4 * 16 = 64 */
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE,
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE,
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE,
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE,
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE,
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE,
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE,
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE,
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE,
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE,
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE,
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE,
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE,
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE,
  EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE, EXECNAMETYPE,
  CMDTYPE, CMDTYPE, CMDTYPE, CMDTYPE,
  REALTYPE, REALTYPE, REALTYPE,
  INTEGERTYPE, INTEGERTYPE, INTEGERTYPE, INTEGERTYPE, /* [-1,+18] => 20 */
  INTEGERTYPE, INTEGERTYPE, INTEGERTYPE, INTEGERTYPE,
  INTEGERTYPE, INTEGERTYPE, INTEGERTYPE, INTEGERTYPE,
  INTEGERTYPE, INTEGERTYPE, INTEGERTYPE, INTEGERTYPE,
  INTEGERTYPE, INTEGERTYPE, INTEGERTYPE, INTEGERTYPE,
  BOOLEANTYPE, BOOLEANTYPE,
  RELPKDARYTYPE,
  RELSTRINGTYPE
  /* and the rest will get padded with invalid entries = 0 */
};

public readonly real encRealValues[RealCodes] = {-1.0, 0.0, 1.0};


#if (MINALIGN==1)
#define CopyObject(d, s) *(Object *)(d) = *(Object *)(s)
#else (MINALIGN==1)
#define CopyObject(d, s) os_bcopy((char *)(s), (char *)(d), sizeof(Object))
#endif (MINALIGN==1)


private cardinal EncodeObj(ob,codeOffset)
Object ob;			/* object to encode */
PCard8 codeOffset;
/*
 *  This routine encodes the object "ob" and stores its encoding
 *  in VM at the specified offset.  The number of characters
 *  placed in the buffer is returned as the function's value.
 */
{
  integer i;			/* scratch integer */
  cardinal arylength;		/* array length */
  integer relaryaddress;	/* array offset */
  Code *p = (Code *)codeOffset;	/* byte pointer into VM buffer */

#define SAMETAGS(ob1,ob2) \
		((ob1).tag == (ob2).tag && (ob1).access == (ob2).access)

  switch (ob.type)
  {
  case intObj:
    i = ob.val.ival;			/* get integer's value */
    if (SAMETAGS(ob,iLIntObj) && MinInteger <= i && i <= MaxInteger)
    { *p = IntegerBase + (i - MinInteger); /* return biased integer code */
      return sizeof(Code);
    }
    break;

  case boolObj:
    if (SAMETAGS(ob,iLBoolObj))
    { *p = BooleanBase + (ob.val.bval? 1 : 0);
      return sizeof(Code);
    }
    break;

  case realObj:
    if (SAMETAGS(ob,iLRealObj))
    { for (i = 0; i < RealCodes; i++)
        if (ob.val.rval == encRealValues[i]) {
	  *p = RealBase+i; return sizeof(Code);}
    }
    break;

  case nameObj:
  { NameIndex nameindex;		/* name index buffer */
    Code code;				/* scratch code */

    if (SAMETAGS(ob,iXNameObj))
      code = ExecNameBase;
    else if (SAMETAGS(ob,iLNameObj))
      code = LitNameBase;
    else
      break;
    if ((nameindex = ob.val.nmval->nameindex) > MAXNameIndex)
      break;		/* name index is too large to encode */
    *p++ = code + (nameindex >> 8);	/* plant biased code */
    *p = (Code)(nameindex & 0xFF);	/* then lo order part of name index */
    return sizeof(NameIndex);
  }

  case pkdaryObj:
  { RelAry relary;			/* scratch relative arrary */

#if	false		/* Disable relative addresses */
    if (!SAMETAGS(ob,iXPkdaryObj))
      break;
    arylength = ob.length;
    relaryaddress = ob.val.pkdaryval - codeOffset; /* relative address */
    if (MINArrayLength <= arylength && arylength <= MAXArrayLength)
    { if (MAXOffset <= relaryaddress && relaryaddress <= MINOffset)
      { *p++ = RelPkdary;
	relary = 			 /* build relative array */
          ((arylength - MINArrayLength) << BitsForOffset) |
          (~(-1 << BitsForOffset) & (relaryaddress - MINOffset));
	*p++ = relary >> 8;		/* plant hi-order byte */
        *p = (Code)(relary & 0xFF);	/* now lo-order byte */
        return sizeof(Code)+sizeof(RelAry);
      }
    }
#endif	false
    break;
  }


  case strObj:
  { RelAry relary;			/* scratch relative array */

#if	false		/* Disable relative addressing */
    if (!SAMETAGS(ob,iLStrObj))
      break;
    arylength = ob.length;
    relaryaddress = ob.val.strval - codeOffset; /* get relative address */
    if (MINArrayLength <= arylength && arylength <= MAXArrayLength)
    { if (MAXOffset <= relaryaddress && relaryaddress <= MINOffset)
      { *p++ = RelString;
	relary = 			 /* build relative array */
          ((arylength - MINArrayLength) << BitsForOffset) |
          (~(-1 << BitsForOffset) & (relaryaddress - MINOffset));
        *p++ = relary >> 8;	/* plant hi-order byte */
        *p = (Code)relary;
        return sizeof(Code)+sizeof(RelAry);
      }
    }
#endif	false
    break;
  }

  case cmdObj:
    if (SAMETAGS(ob,iXCmdObj) && ob.length < CmdValues)
    { *p++ = (ob.length >> 8) + CmdBase;	/* plant biased code */
      *p = (Code)ob.length;			/* and then lo-order byte */
      return sizeof(CmdIndex);
    }
    break;

  default: break;
  }

  /* the object cannot be encoded if we fall through to here */
  *p++ = ObjectEscape;		/* return escape code */
  CopyObject(p,&ob);		/* copy out object */
  return sizeof(Object) + sizeof(Code);
}


/* The following redefinition causes the object construction macros
   called in DecodeObj to use the save level of the parent object
   rather than the current save level when reconstituting elements
   of a packed array. */
#undef LEVEL
#define LEVEL pObj->level

public procedure DecodeObj(pObj, robj)
register PPkdaryObj pObj;	/* ptr to packed array object */
register PObject robj;		/* ptr to returned object */
/*
 *	This function decodes the first element of the packed array
 *	object at *pObj, reconstitutes a normal Object from that element,
 *	and returns it.  The packed array object at *pObj is updated
 *	to describe the remainder of the packed array.
 */
{ 
  register Code code;		/* the code byte from VM */
  register Code tablecode;	/* the dispatch table code entry */
  register charptr p,oldp;	/* pointers into VM */
#if STAGE==DEVELOP
  if (pObj->length == 0) CantHappen();
#endif STAGE==DEVELOP
  oldp = p = pObj->val.pkdaryval;		/* grab ptr into VM */
  switch (tablecode = opType[code = *p++])	/* dispatch on its code */
  {
  case ESCAPETYPE:
    CopyObject(robj,p);		/* copy out the object */
    p += sizeof(Object);	/* point to next encoded object */
    break;

  case REALTYPE:
    LRealObj(*robj, encRealValues[code - RealBase]);
    break;

  case EXECNAMETYPE:
    code -= ExecNameBase;	/* unbias code */
    NameIndexObj((code << 8) | *p++, robj);	/* get name object */
    robj->tag = Xobj;		/* mark it executable */
    break;

  case LITNAMETYPE:
    code -= LitNameBase;	/* unbias code */
    NameIndexObj((code << 8) | *p++, robj);	/* get name object */
    robj->tag = Lobj;		/* mark it literal */
    break;

  case INTEGERTYPE:
    LIntObj(*robj,code - IntegerBase + MinInteger);
    break;

  case BOOLEANTYPE:
    LBoolObj(*robj, code - BooleanBase);
    break;

  case CMDTYPE:
    {
    CmdIndex cmdindex;			/* scratch command number */
    code -= CmdBase;			/* unbias code */
    cmdindex = (code << 8) | *p++;	/* get biased command number */
    CmdIndexObj(cmdindex, robj);	/* build command obj from index */
    break;
    }
 
  case RELSTRINGTYPE:
  case RELPKDARYTYPE:
  { RelAry relary;			/* holder for relative array info */
    cardinal length;			/* scratch length */
    PCard8 ptr;				/* scratch pointer */

    code = *p++;			/* grab hi order byte */
    relary = ((code << 8) | *p++);	/* build entire RelAry */
					/* reconstruct length & offset... */
    length = (relary >> BitsForOffset) + MINArrayLength;
    ptr = (pObj->val.pkdaryval + (((int)relary + MINOffset) |
      (-1 << BitsForOffset)));
    /* reconstruct appropriate object */
    if (tablecode == RELPKDARYTYPE)
    { XPkdaryObj(*robj,length,ptr);	/* build packed array */
    }
    else
    { LStrObj(*robj,length,ptr);	/* build string */
    }
    break;
  }

  default: CantHappen();
  }

  pObj->val.pkdaryval += (p - oldp);	/* bump offset by delta */
  pObj->length -= 1;			/* reduce length by one */
}  /* end of DecodeObj */


#undef LEVEL
#define LEVEL level


public procedure Pkdary(n, ppary)
  cardinal n;			/* number of stack entries to encode */
  register PPkdaryObj ppary;	/* returned object */
/*
 *	This routine returns a packed array object containing the encoded
 *	"n" objects on the operand stack.
 *
 *	The strategy is to pop "n" objects onto the stack's free list and
 *	encode each popped object.  This has the effect of
 *	reversing the stack.
 */
{
  PObject mark = opStk->head;	/* save stacks free list pointer */
  PObject curObj;		/* pointer to current stack node */
  cardinal i;			/* scratch counter */
  Object  discard;		/* object for throwing away stack objects */
  PCard8  Elements;		/* Address of packed elements */
  integer ActualLength;

  /* treat zero length array as special case to avoid recycler confusion */
  /* id: Confirm that this is no longer necessary. */
  if (n == 0) {
    LPkdaryObj (*ppary, 0, NIL);
    return;
  }
  for (i = 0; i < n; i++) {
    IPop (opStk, &discard);
    ConditionalInvalidateRecycler (opStk->head);
  }

  Elements = PreallocChars ((integer)(n * (sizeof(Object) + sizeof(Code))));
  LPkdaryObj (*ppary, 0, Elements);	/* init return packed array object */

  ActualLength = 0;

  curObj = opStk->head;		/* point to first object in free list */

  while (curObj != mark) {	/* now encode each object */
    ActualLength += EncodeObj (*curObj, Elements + ActualLength);	/* encode object */
    ppary->length++;		/* and bump number of objects by 1 */
    curObj++;			/* point to next entry */
  }

  ClaimPreallocChars (Elements, ActualLength);
  ConditionalResetRecycler(vmCurrent->recycler, Elements);
}				/* end of Pkdary */

public procedure BindPkdary(pa)
PkdaryObj pa;			/* the packed array to bind */
/*
 *	This routine binds the argument packed array by converting
 *	all names referring to commands to their command numbers.
 */
{
  Object  ob;			/* object holder */
  PCard8  oldptr;		/* address of beginning of name index */
  charptr p;			/* ptr to bytes in packed array */

  while (pa.length != 0) {	/* for all objects in packed array */
    oldptr = pa.val.pkdaryval;	/* save address in VM */
    DecodeObj (&pa, &ob);	/* grab the next element */
    if (ob.tag != Xobj)
      continue;			/* only executable objects get bound */
    switch (ob.type) {
     case nameObj:
      if (Load (ob, &ob)) {	/* known somewhere? */
	if (ob.type == cmdObj) {/* and it is a command */
	  p = oldptr;/* find real address of encoding */
	  if (*p != ObjectEscape) {	/* do not replace unencoded name! */
	    *p++ = (ob.length >> 8) + CmdBase;	/* plant biased code */
	    *p = (Code) ob.length;	/* and then lo-order byte */
	  }
	}
      }
      break;
     case pkdaryObj:
      BindPkdary (ob);		/* recurse if another packed array */
      break;
     case arrayObj:
      BindArray(ob);	/* bind ordinary array & make it readonly */
      ob.access &= ~wAccess;
      p = oldptr;
#if STAGE==DEVELOP
      if (*p != ObjectEscape)
	CantHappen ();
#endif STAGE==DEVELOP
      (void) EncodeObj (ob, p);
      break;
     default:
      break;
    }
  }
}

public procedure PSPkdary()
/*
 *	This routine creates a packed array object and leaves it on the
 *	operand stack.  It expects as arguments the integer N (top) 
 *	designating the number of stack entries under N to encode into
 *	the packed array.
 */
{ 
PkdaryObj pao;
cardinal n = PopCardinal();	/* grab N */

/* Redundant test (says gcc) because of types--
  if (n > MAXarrayLength) LimitCheck();
 */
Pkdary(n, &pao);
PushP(&pao);			/* push resulting packed array object */
}  /* end of PSPkdary */

private Card32 EnumerateComposites(pObj, info)
PPkdaryObj pObj;      /* ptr to packed array object   */
GC_Info info;         /* The GC info structure        */
/*
 * This function enumerates the composite objects found in a
 * specified packed array. It pushes each such object, onto
 * the supplied GC stack.
 */
{
  register int i;
  register Code code;		/* code byte from VM	*/
  register PCard8 initP, p;	/* pointers into VM	*/
  
  initP = p = pObj->val.pkdaryval;
  for (i = pObj->length; i > 0; i--)
    {
    switch (opType[code = *p++])	/* dispatch on its code	*/
      {
      case REALTYPE:
      case INTEGERTYPE:
      case BOOLEANTYPE:
        /* These are one byte types, so there's nothing to do	*/ 
	break;
      case EXECNAMETYPE:
	GC_HandleIndex(((code-ExecNameBase) << 8) | *p++, 0, info);
	break;
      case LITNAMETYPE:
	GC_HandleIndex(((code-LitNameBase) << 8) | *p++, 0, info);
	break;
      case CMDTYPE:
	GC_HandleIndex(((code-CmdBase) << 8) | *p++, 1, info);
	break;
      case RELSTRINGTYPE:
      case RELPKDARYTYPE:
        /* These are 3 byte types, so add 2 to the pointer	*/ 
	p += 2;
	break;
      case ESCAPETYPE:
        /* These are the guys we're interested in...		*/
	GC_Push(info, (PObject)p);	/* Here's one...		*/
	p += sizeof(Object);	/* point to next encoded object	*/
	break;
      default:
        CantHappen();
      }
    }
  return((Card32)(p - initP));
}  /* end of EnumerateComposites */

public procedure PkdaryInit(reason)  InitReason reason;
{ switch (reason)
  {
  case init:
    GC_RgstPkdAryEnumerator(EnumerateComposites);
    break;
  case romreg:
    break;
  endswitch
  }
}
