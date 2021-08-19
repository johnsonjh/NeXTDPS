/*
  languagenames.h

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
Ed Taft: Mon Aug 14 16:56:39 1989
Joe Pasqua: Wed Feb  8 13:39:35 1989
Bill Bilodeau Tue Jan 17 19:54:59 PST 1989
Jim Sandman: Thu Oct 19 16:34:57 1989
End Edit History.

Definition of operators and registered names for the language package.
This is used both as a C header file and as the input to a shell
script, update_registered_names, that derives a PostScript program
to register the names and one or more C programs to register the
C procedures for the operators.
*/

#ifndef	LANGUAGENAMES_H
#define	LANGUAGENAMES_H

/*
  The following macro defines the package index for this package.
  It must be consistent with the one defined in orphans.h.
 */

#define PACKAGE_INDEX 1  /* = pni_language */

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

OpSet(ops_languageDPS, 1, systemdict)
  Op(abs, PSAbs)
  Op(add, PSAdd)
  Op(aload, PSALoad)
  Op(anchorsearch, PSAnchorSearch)
  Op(and, PSAnd)
  Op(array, PSArray)
  Op(astore, PSAStore)
  Op(atan, PSATan)
  Op(begin, PSBegin)
  Op(bind, PSBind)
  Op(bitshift, PSBitShift)
  Op(bytesavailable, PSBytesAvailable)
  Op(ceiling, PSCeiling)
  Op(clear, PSClear)
  Op(cleardictstack, PSClearDictStack)
  Op(cleartomark, PSClrToMrk)
  Op(closefile, PSCloseFile)
  Op(copy, PSCopy)
  Op(cos, PSCos)
  Op(count, PSCount)
  Op(countdictstack, PSCntDictStack)
  Op(countexecstack, PSCntExecStack)
  Op(counttomark, PSCntToMark)
  Op(currentdict, PSCrDict)
  Op(currentfile,PSCrFile)
  Op(currentobjectformat, PSCrObjFormat)
  Op(currentpacking, PSCrPacking)
  Op(cvi, PSCvI)
  Op(cvlit, PSCvLit)
  Op(cvn, PSCVN)
  Op(cvr, PSCvR)
  Op(cvrs, PSCVRS)
  Op(cvs, PSCVS)
  Op(cvx, PSCvX)
  Op(def, PSDef)
  Op(defineusername, PSDefUserName)
  Op(defineuserobject, PSDefUserObj)
  Op(deletefile, PSDeleteFile)
  Op(devdismount, PSDevDisMount)
  Op(devforall, PSDevForAll)
  Op(devformat, PSDevFormat)
  Op(devmount, PSDevMount)
  Op(devstatus, PSDevStatus)
  Op(dict, PSDict)
  Op(dictstack, PSDictStack)
  Op(div, PSDiv)
  Op(dup, PSDup)
  Op(eexec, PSEExec)
  Op(end, PSEnd)
  Op(eq, PSEq)
  Op(exch, PSExch)
  Op(exec, PSExec)
  Op(execuserobject, PSExecUserObj)
  Op(execstack, PSExecStack)
  Op(executeonly, PSXctOnly)
  Op(exit, PSExit)
  Op(exp, PSExp)
  Op(file, PSFile)
  Op(filenameforall, PSFileNameForAll)
  Op(fileposition, PSFilPos)
  Op(floor, PSFloor)
  Op(flush, PSFls)
  Op(flushfile, PSFlsFile)
  Op(for, PSFor)
  Op(forall, PSForAll)
  Op(ge, PSGe)
  Op(get, PSGet)
  Op(getinterval, PSGetInterval)
  Op(gt, PSGt)
  Op(idiv, PSIDiv)
  Op(if, PSIf)
  Op(ifelse, PSIfElse)
  Op(index, PSIndex)
  Op(known, PSKnown)
  Op(le, PSLe)
  Op(length, PSLength)
  Op(ln, PSLn)
  Op(load, PSLoad)
  Op(log, PSLog)
  Op(loop, PSLoop)
  Op(lt, PSLt)
  Op(mark, PSMark)
  Op(maxlength, PSMaxLength)
  Op(mod, PSMod)
  Op(mul, PSMul)
  Op(ne, PSNe)
  Op(neg, PSNeg)
  Op(noaccess, PSNoAccess)
  Op(not, PSNot)
  Op(or, PSOr)
  Op(packedarray, PSPkdary)
  Op(pop, PSPop)
  Op(print,PSPrint)
  Op(printobject, PSPrObject)
  Op(put, PSPut)
  Op(putinterval, PSPutInterval)
  Op(rand, PSRand)
  Op(rcheck, PSRCheck)
  Op(read, PSRead)
  Op(readhexstring, PSReadHexString)
  Op(readline, PSReadLine)
  Op(readonly, PSReadOnly)
  Op(readstring, PSReadString)
  Op(renamefile, PSRenameFile)
  Op(repeat, PSRepeat)
  Op(resetfile, PSResFile)
  Op(roll, PSRoll)
  Op(round, PSRound)
  Op(rrand, PSRRand)
  Op(scheck, PSSCheck)
  Op(search, PSSearch)
  Op(setfileposition, PSStFilPos)
  Op(setobjectformat, PSStObjFormat)
  Op(setpacking, PSStPacking)
  Op(sin, PSSin)
  Op(sqrt, PSSqRt)
  Op(srand, PSSRand)
  Op(status, PSStatus)
  Op(stop, PSStop)
  Op(store, PSStore)
  Op(string, PSString)
  Op(sub, PSSub)
  Op(token, PSToken)
  Op(truncate, PSTruncate)
  Op(type, PSType)
  Op(undef, PSUnDef)
  Op(undefineuserobject, PSUndefUserObj)
  Op(wcheck, PSWCheck)
  Op(where, PSWhere)
  Op(write, PSWrite)
  Op(writehexstring, PSWrtHexString)
  Op(writeobject, PSWrObject)
  Op(writestring, PSWrtString)
  Op(xcheck, PSXCheck)
  Op(xor, PSXor)

/*
  The following macros define the indices for all registered names
  other than operators. The actual PostScript name is simply
  the macro name with the "nm_" prefix removed.

  The macro definitions are renumbered automatically by the
  update_registered_names script. For ease of maintenance, they
  should be in alphabetical order.
 */


#define nm_FilterDirectory 0
#define nm_FontDirectory 1
#define nm_UserObjects 2
#define nm_interrupt 3
#define nm_timeout 4

/* order of following must correspond to object types in basictypes.h */
#define nm_nulltype 5
#define nm_integertype 6
#define nm_realtype 7
#define nm_nametype 8
#define nm_booleantype 9
#define nm_stringtype 10
#define nm_filetype 11
#define nm_operatortype 12
#define nm_dicttype 13
#define nm_arraytype 14
#define nm_fonttype 15
#define nm_t11 16
#define nm_t12 17
#define nm_packedarraytype 18
#define nm_t14 19
#define nm_t15 20
#define nm_marktype 21
#define nm_savetype 22
#define nm_gstatetype 23
#define nm_conditiontype 24
#define nm_locktype 25
#define nm_namearraytype 26


/* The following definition is regenerated by update_registered_names */
#define NUM_PACKAGE_NAMES 27

extern PNameObj languageNames;

#endif	LANGUAGENAMES_H
