/*
  pa_relocator.c

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

Original version: Bill Bilodeau April 13, 1989 
Edit History:
Bill Bilodeau Thu Apr 13 18:39:24 PDT 1989 (taken from packedarray.c)
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



#if (MINALIGN==1)
#define CopyObject(d, s) *(Object *)(d) = *(Object *)(s)
#else (MINALIGN==1)
#define CopyObject(d, s) os_bcopy((char *)(s), (char *)(d), sizeof(Object))
#endif (MINALIGN==1)



public procedure RelocateObj(pObj, robj)
register PPkdaryObj pObj;	/* ptr to packed array object */
register PObject robj;		/* ptr to returned object */
/*
 *	This function decodes the first element of the packed array
 *	object at *pObj, relocates its reference into VM if any
 *	and returns its type in robj.  pObj is updated to
 *	to describe the remainder of the packed array.
 */
{
  register Code code;		/* the code byte from VM */
  register Code tablecode;	/* the dispatch table code entry */
  register charptr p,
          oldp;			/* pointers into VM */

#if STAGE==DEVELOP
  if (pObj->length == 0)
    CantHappen ();
#endif STAGE==DEVELOP
  oldp = p = pObj->val.pkdaryval;	/* grab ptr into VM */
  switch (tablecode = opType[code = *p++]) {	/* dispatch on its code */
   case ESCAPETYPE:
    CopyObject (robj, p);	/* copy out the object */
    switch (robj->type == escObj ? robj->length : robj->type) {
      /*
       * Body of switch should match disposition of objects in RelocateObject
       * in vm_relocate.c
       */
     case dictObj:
      if (TrickyDict (robj))
	break;			/* Do not relocate trickydicts */
     case arrayObj:
     case pkdaryObj:
     case strObj:
     case nameObj:
     case cmdObj:		/* This may be an error if not encoded */
     case objCond:
     case objLock:
      NewRelocationEntry ((PCard8 *)&(((PObject) p)->val.nullval));
      break;
     case nullObj:
     case intObj:
     case realObj:
     case boolObj:
     case fontObj:
     case objMark:
      /* No action */
      break;
     case stmObj:
     case objSave:
     case objGState:
     case objNameArray:
       /* Fall through to CantHappen () */ ;
     default:
      CantHappen ();
    }

    p += sizeof (Object);	/* point to next encoded object */
    break;

   case REALTYPE:
    LRealObj (*robj, encRealValues[code - RealBase]);
    break;

   case EXECNAMETYPE:
    code -= ExecNameBase;	/* unbias code */
    NameIndexObj ((code << 8) | *p++, robj);	/* get name object */
    robj->tag = Xobj;		/* mark it executable */
    break;

   case LITNAMETYPE:
    code -= LitNameBase;	/* unbias code */
    NameIndexObj ((code << 8) | *p++, robj);	/* get name object */
    robj->tag = Lobj;		/* mark it literal */
    break;

   case INTEGERTYPE:
    LIntObj (*robj, code - IntegerBase + MinInteger);
    break;

   case BOOLEANTYPE:
    LBoolObj (*robj, code - BooleanBase);
    break;

   case CMDTYPE:
    {
      CmdIndex cmdindex;	/* scratch command number */

      code -= CmdBase;		/* unbias code */
      cmdindex = (code << 8) | *p++;	/* get biased command number */
      /* CmdIndexObj(cmdindex, robj); *//* build command obj from index */
      robj->type = cmdObj;
      break;
    }

   case RELSTRINGTYPE:
   case RELPKDARYTYPE:
    {
      RelAry  relary;		/* holder for relative array info */
      cardinal length;		/* scratch length */
      PCard8  ptr;		/* scratch pointer */

      code = *p++;		/* grab hi order byte */
      relary = ((code << 8) | *p++);	/* build entire RelAry */
      /* reconstruct length & offset... */
      length = (relary >> BitsForOffset) + MINArrayLength;
      ptr = (pObj->val.pkdaryval + (((int) relary + MINOffset) |
				    (-1 << BitsForOffset)));
      /* reconstruct appropriate object */
      if (tablecode == RELPKDARYTYPE) {
	XPkdaryObj (*robj, length, ptr);	/* build packed array */
      } else {
	LStrObj (*robj, length, ptr);	/* build string */
      }
      break;
    }

   default:
    CantHappen ();
  }

  pObj->val.pkdaryval += (p - oldp);	/* bump offset by delta */
  pObj->length -= 1;		/* reduce length by one */
}				/* end of RelocateObj */



public procedure RgstPARelocator()
{
    RgstPackedArrayRelocator (RelocateObj);
}

