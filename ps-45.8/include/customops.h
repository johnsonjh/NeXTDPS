/*
  customops.h

Copyright (c) 1984, '85, '86, '87, '88, '89, '90 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

customops.h is one of the Display PostScript "glue" interfaces. It is
implemented by the kernel. It provides procedures for defining new operators
with C-callable procedures as their implementations.

NOTE
customops.h should be used ONLY to provide access to unique features
of the host system. Programs that use operators defined by PSRegister
ARE NOT PORTABLE to other systems.

A limited collection of procedures for managing the PostScript operand
stack are defined here. These should be called ONLY from procedures
registered via PSRegister when such a registered procedure is called from
the PostScript interpreter.

The procedures for managing the PostScript operand stack can raise
exceptions (see their definitions, below, also except.h). Normally, there
is no need to handle exceptions in the body of a registered procedure
because the PostScript interpreter handles them, but there are cases when it
is necessary to intercept an exception, clean up, then pass it on. For
example, if your registered procedure must allocate storage before calling
one of the procedures below and this storage would be assigned a home after
the call returns, then you will want to embed the call in a handler, as
follows, to release the storage before the C stack gets unwound if an
exception occurs:

    integer i;
	char *p = os_malloc(sizeof(MyRec));
	...
	DURING
      PSPushInteger(i); / may raise an exception if operand stack full. /
    HANDLER
      <<os_free(p) and perform other cleanup>>
	  RERAISE;
    END_HANDLER
	...

Within a registered procedure, you can call the procedures defined here
(but not PSRegister), procedures defined in other glue interfaces, e.g.,
pslib, devcreate.h and except.h, but not procedures that would be called
directly on behalf of an application, for example ones defined in
dpsclient.h, dpsfriends.h, postscript.h or psscheduler.h.

*/

#ifndef    CUSTOMOPS_H
#define    CUSTOMOPS_H

#include PUBLICTYPES
#include DEVICETYPES
#include STREAM

/* the definition of basic PostScript objects */

#define	nullType	0
#define	intType		1
#define	realType	2
#define	boolType	4
#define	strType		5
#define	arrayType	9

#define	Lobj	0	/* tag for literal object */
#define	Xobj	1	/* tag for executable object */

/* The following define opaque pointers for objects stored in VM */

typedef struct _t_PSObject	*PPSObject;
typedef struct _t_PSGState	*PPSGState;

/* Object definition */

typedef struct _t_PSObject {
  BitField	tag:1;
  BitField	priv1:3;
  BitField	type:4;
  BitField	priv2:8;
  BitField	length:16;
  union {
   /* null */	Int32		nullval;
   /* int */	Int32		ival;
   /* real */	real		rval;
   /* bool */	boolean		bval;
   /* str */	charptr		strval;
   /* array */	PPSObject	arrayval;
   /* gstate */ PPSGState	gstateval;
  }		val;
}	PSObject;

/* The above types provide a limited ability to directly manipulate
   objects.  Most applications should not have to access fields of the
   object directly. If the following rules for dealing with objects are
   not followed, the system may break at unpredictable times.

   1. The priv1 and priv2 fields should never be touched.
   2. If the type field is not one of the "*Type" values defined above,
      none of the fields of the object should be referenced.
   3. If the object is a string, ob.type == strType, ob.length is the
      length of the string and ob.val.strval is the pointer to the
      characters. The length and srtval fields may be altered to
      length' and strval' but only such that ob.val.strval <=
      ob.val.strval' < (ob.val.strval + ob.length) and ob.val.strval' +
      ob.length' < (ob.val.strval + ob.length). The length may also be
      set to 0 and the strval to NULL.
   4. If the object is an array, ob.type == arrayType, ob.length is
      the length of the array and ob.val.arrayval is the pointer to the
      characters. The length and srtval fields may be altered to
      length' and arrayval' but only such that ob.val.arrayval <=
      ob.val.arrayval' < (ob.val.arrayval + ob.length) and
      ob.val.arrayval' + ob.length' < (ob.val.arrayval + ob.length).
      The length may also be set to 0 and the arrayval to NULL.
   5. Objects should be accessed only from registered procedures.
   6. Objects should be created by executing PostScript code and
      then popping them off the stack or accessing them from an array.
*/


typedef enum {

  dpsNullObj,
  dpsIntObj,
  dpsRealObj,
  dpsNameObj,
  dpsBoolObj,
  dpsStrObj,
  dpsStmObj,
  dpsCmdObj,
  dpsDictObj,
  dpsArrayObj,
  dpsPkdaryObj,
  dpsGStateObj,
  dpsCondObj,
  dpsLockObj,
  dpsOtherObj,
  dpsAnyObj

  } PSOperandType;


/* The procedures below may be called only from a registered proc. They may
   raise exceptions, as specified,
 */



extern PSOperandType PSGetOperandType();
  /* returns the type of the object on top of the stack. Raises exception
     if stack is empty. */
     
extern void PSPop();
  /* Pops one object from the stack and discards it. Raises exception
     if stack is empty. */

extern long int PSPopInteger();
  /* Pops an integer from the PostScript operand stack and returns it.
     Raises an exception if the stack is empty or if the top operand is not
     an integer.*/

extern void PSPushInteger(/* long int i; */);
  /* Pushes the given integer onto the PostScript operand stack.
     Raises an exception if the stack is full.    */


extern boolean PSPopBoolean();
  /* Pops a boolean from the PostScript operand stack and returns 0
     if false, 1 if true.  Raises an exception if the stack is empty or
     if the top operand is not a boolean.    */
     
extern void PSPushBoolean(/* boolean b; */);
  /* Pushes the given boolean onto the PostScript operand stack.
     0 means false, 1 means true.  Raises an exception if the stack is
     full.    */

extern void PSPushReal(/* float r; */);
  /* Pushes the given float onto the PostScript operand stack as a real.
     Raises an exception if the stack is full.    */

extern real PSPopReal();
  /* Pops and returns a real from the PostScript operand stack.
     This will convert an integer operand to a float.  Raises an
     exception if the stack is empty or if the top operand is not a
     real or integer. */

extern void PSPopPReal(/* float *r; */);
  /* Pops a real from the PostScript operand stack and fills in *r.
     This will convert an integer operand to a float.  Raises an
     exception if the stack is empty or if the top operand is not a
     real. */
     
extern void PSPushPReal(/* float *r; */);
  /* Pushes the given float onto the PostScript operand stack as a real.
     Raises an exception if the stack is full.    */

extern void PSPopPCd(/* Cd *cd; */);
  /* Equivalent to PSPopPReal(&cd.y); PSPopPReal(&cd.x); */

extern void PSPushPCd(/* Cd *cd; */);
  /* Equivalent to PSPushPReal(&cd.x); PSPushPReal(&cd.y); */

extern void PSPopPMtx(/* PMtx m; */);
  /* Pops a matrix from the PostScript operand stack and fills in *m.
     Raises an exception if the stack is empty or if the top operand is not
     a matrix.    */

extern long int PSStringLength();
  /* Returns the length of the string on the top of the operand stack.
     Raises an exception if the stack is empty or if the top operand is not
     a string. */
     
extern void PSPopString(/* char *sP; long int nChars; */);
  /* Pops a string from the PostScript operand stack and copies it thru sP,
     followed by a null byte.  Raises an exception if the stack is
     empty or if the top operand is not a string or if nChars <
     (stringlength+1).    */
     
extern void PSPushString(/* char *sP; */);
  /* Pushes a copy of the given (null-terminated) string onto the PostScript
     operand stack.  Raises an exception if the stack is full.    */

extern Stm PSPopStream();
  /* Pops a file object from the PostScript operand stack and returns the
     'Stm' that it references.  Raises an exception if the stack is
     empty or if the top operand is not a file.
     DO NOT use fclose to close the Stm; only the PostScript operator
     'closefile' should do that.
     See stream.h. See the PostScript operators file, closefile, etc.    */

extern void PSPushStream(/* Stm stm; long int executable; */);
  /* Pushes a file object that references the given Stm onto the PostScript
     operand stack. Makes the Stm executable if executable = 1.
     Raises an exception if the stack is full.    */


extern procedure PSPopTempObject(/* DPSOperandType type; PPSObject pobj; */);
  /* Pops an object from the stack and fills in *pobj with the object.
     The object is valid only for the duration of the call on the
     registered procedure. Raises an exception if the operand stack is
     empty, if the top object's type is not type and type is not
     dpsAnyType, or if the top object is not one for which it makes
     sense to construct a handle, i.e., if
     (PSGetOperandType() == dpsOtherObj).    */


extern PPSObject PSPopManagedObject(/* DPSOperandType type; */);
  /* Pops one PostScript object from the PostScript operand stack, 
     places it in a table of managed objects and returns a pointer to
     the object.  Remember to call PSReleaseObject when you are done
     with the object.  Raises an exception if the operand stack is
     empty, if the top object's type is not type and type is not
     dpsAnyType, or if the top object is not one for which it makes
     sense to construct a handle, i.e., if
	(PSGetOperandType() == dpsOtherObj).    */

extern void PSPushObject(/* PPSObject pobj; */);
  /* Pushes the PostScript object referenced by pobj onto the
     PostScript operand stack.  Raises an exception if the operand
     stack is full.    */

extern void PSReleaseManagedObject(/* PPSObject pobj; */);
  /* Call this when you are finished with the object.
     Raises an exception if pobj is an invalid handle.   */

  /* To use locks, your code should do the following:

     PPSObject pLockObj;
     PSExecuteString("currentshared true setshared lock exch setshared");
     pLockObj = PSPopManagedObject(dpsLockObj);
     ...
     PSAcquireLock(pLockObj);
     ...
     PSReleaseLock(pLockObj);
     ...
     PSReleaseObject(pLockObj);
   */
 
extern void PSAcquireLock(/* PPSObject pobj; */);
  /* Call this to acquire a lock. */

extern void PSReleaseLock(/* PPSObject pobj; */);
  /* Call this to release a lock. */

extern PSOperandType PSGetObjectType(/* PPSObject pobj; */);
  /* Returns the type of the object.    */

extern boolean PSSharedObject(/* PPSObject pobj; */);
  /* Returns true if the object referenced by pobj is simple or if its
     value is located in sharedVM, false otherwise. Performs the 
     equivalent of the scheck operator on the object. */

extern void PSPushPMtx(/* PPSObject pobj; PMtx m; */);
  /* Fills in the array object specified by pobj with the values from
     m. Raises an exception if the array is not a matrix or if the stack
     is full.    */
     
     
extern void PSExecuteOperator(/* long int index */); 
  /* Performs the equivalent of "systemdict /name get exec" where name
     corresponds to the system name index, index. If index is out of
     range or has no object associated with it or if the corresponding
     name is does not have a value in systemdict an undefined error
     occurs. Other errors may result from the execution of the
     operator. */

extern boolean PSExecuteString(/* char *sP; */);
  /* copies the given (null-terminated) string and gives it to the
     PostScript interpreter to be executed.  Returns 1 if the
     interpreter reports an error, 0 otherwise.  If an error is
     reported, the PSHandleExecError should be called.   */

extern boolean PSExecuteObject(/* PSObject obj; */);
  /* Executes the object.  Returns 1 if the interpreter reports
     an error, 0 otherwise.  If an error is reported, the
     PSHandleExecError should be called.    */

extern void PSHandleExecError();
  /* Handles an error generated by invoking either PSExecuteString
     or PSExecuteObject. */


extern PPSGState PSPopGState();
  /* Pops an gstate object from the PostScript operand stack and returns a
     pointer that may be used in the procedures below to get information
     from the graphics state. If the object is null it returns NULL.
     Raises an exception if the stack is empty or if the top operand is not
     an gstate object or null.
     BEWARE:
       Unless you acquire a handle on the gstate object via
       PSPopObject, the pointer returned is valid only for the duration
       of the call on the registered procedure.    */


extern PMtx PSGetMatrix(/* PPSGState p; */);
  /* Returns a pointer to the transformation matrix from the gstate object p.
     If p == NULL returns the transformation matrix from the current
     graphics state.    */

extern PDevice PSGetDevice(/* PPSGState p; */);
  /* Returns the device from the gstate object p.
     If p == NULL returns the device from the current
     graphics state.    */

extern void PSGetMarkInfo(/* PPSGState p; DevMarkInfo *info; */);
  /* Fills in info with the information from the gstate object p.
     If p == NULL it uses the current graphics state.    */

extern char *PSGetGStateExt(/* PPSGState p; */);
  /* Returns the GStateExt from the gstate object p.
     If p == NULL it uses the current graphics state.    */

extern DevTfrFcn *PSGetTfrFcn();
/* PSGetTfrFcn:  used by nextimage to build its DevImage
   in level 2 probably will need to be exapanded to get gamut transfer
   and color rendering as well
   */

/* information about current graphics state */

extern boolean PSBuildingMasks();
  /* Returns 1 if the current graphics state is building masks, and
     0 otherwise. */
     
extern boolean PSGetClip(/* DevPrim **clip; */);
  /* Fills in clip with the current clip path from the current graphics state.
     The DevPrim storage is owned by the implemention of PSGetClip and
     is valid only for the duration of the call on the registered
     procedure.  Returns true if the clip is a rectangle and false
     otherwise.   */

extern boolean PSReduceRect(/* real x, y, w, h; PMtx mtx; DevPrim *dp; */);
  /* Reduces a user space rectange into trapezoids in device space.
     mtx is used to transform user space to device space. If it is
     NULL, the transformation matrix from the current graphics state is
     used. dp must be a trap DevPrim with at least 7 trapezoids.
     Returns true if the result is also a rectangle in device space and
     false otherwise.  */

extern boolean PSClipInfo(/* Cd *ll, *ur; */);
  /* Returns information about the clip in user space. It fills in ll 
     and ur with bounding box information about the clip and returns
     true if the clip is a rectangle and false otherwise.  */


extern void PSMark();
  /* Pushes a mark on the operand stack */

extern void PSClrToMrk();
  /* Performs the cleartomark operator */

/********* 	  Raising Errors	****************/

extern void PSInvlAccess();
  /* Raises the invalidaccess PostScript error. */
  
extern void PSLimitCheck();
  /* Raises the limitcheck PostScript error. */
  
extern void PSRangeCheck();
  /* Raises the rangecheck PostScript error. */
  
extern void PSTypeCheck();
  /* Raises the typecheck PostScript error. */
  
extern void PSUndefResult();
  /* Raises the undefinedresult PostScript error. */
  
extern void PSUndefined();
  /* Raises the undefined PostScript error. */
  
extern void PSUndefFileName();
  /* Raises the undefinedfilename PostScript error. */
  
extern void PSInvalidID();
  /* Raises the invalidid PostScript error. */


/*********	 Operator Registration        **********/

extern void PSRegister(/* char *opName; void (*proc)(); */);
  /* opName is a null-terminated string that names the new operator.
     proc is called when the operator is executed.  */

extern void PSRegisterStatusDict (/* char *opName; void (*proc)(); */);
  /* opName is a null-terminated string that names the new operator.
     proc is called when the operator is executed.  
     operator will be registered in statusdict instead of the current dict */

typedef struct _t_RgOpEntry {
  char *name;
  void (*proc)();
  } RegOpEntry, *PRgOpEntry, RgOpTable[];

extern void PSRgstOps(/* PRgOpEntry entries; */);
  /* Repeatedly calls PSRegister(entries->name, entries->proc) until
     entries->name == NULL. */
  


#endif    CUSTOMOPS_H
