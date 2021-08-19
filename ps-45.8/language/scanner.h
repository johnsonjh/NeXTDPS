/*
  scanner.h

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

Original version: Ed Taft, May 1988
Edit History:
Ed Taft: Tue May 24 16:51:15 1988
Joe Pasqua: Wed Jan  4 17:54:36 1989
End Edit History.
*/

#ifndef	SCANNER_H
#define	SCANNER_H

#include BASICTYPES
#include "langdata.h"

/* Data Types */

/* object format enumeration for "setobjectformat" */
#define of_disable 0		/* disable binary encodings */
#define of_highIEEE 1		/* high byte first, IEEE real */
#define of_lowIEEE 2		/* low byte first, IEEE real */
#define of_highNative 3		/* high byte first, native real */
#define of_lowNative 4		/* low byte first, native real */
#define of_max 4

/* binary token types (see language manual) */
#define bt_objSeqHiIEEE 128
#define bt_objSeqLoIEEE 129
#define bt_objSeqHiNative 130
#define bt_objSeqLoNative 131
#define bt_int32Hi 132
#define bt_int32Lo 133
#define bt_int16Hi 134
#define bt_int16Lo 135
#define bt_int8 136
#define bt_fixed 137
#define bt_realHiIEEE 138
#define bt_realLoIEEE 139
#define bt_realNative 140
#define bt_bool 141
#define bt_str 142
#define bt_strLongHi 143
#define bt_strLongLo 144
#define bt_systemLName 145
#define bt_systemXName 146
#define bt_userLName 147
#define bt_userXName 148
#define bt_numArray 149
#define bt_implDependent 159


typedef struct {	/* binary object, as defined by language */
  Card8 type;
  Card8 pad;
  Card16 length;
  Card32 value;
} BObject, *PBObject;

typedef union {
  Object o;
  BObject b;
} UObject, *PUObject;

/* Binary object sequence header, as it is interpreted after the
   size field has been put in native order */
typedef struct {
  Card8 type,		/* token type */
	lenTopArray;	/* elements in top-level array */
  Card16 size;		/* total size in bytes */
} BOSHeader, *PBOSHeader;

/* Homogeneous number array token header, as it is interpreted after
   the length field has been put in native order */
typedef struct {
  Card8 type,		/* token type */
	rep;		/* number representation */
  Card16 length;	/* elements in array */
} HNAHeader, *PHNAHeader;


/* Exported Procedures */

extern boolean StmToken(/* Stm stm, PObject pobj, boolean exec */);
/* Scans a token from stm and returns the resulting object in *pobj.
   Returns true if successful, false if the stream reaches EOF before
   a token is encountered. Raises an exception (syntaxerror, limitcheck)
   if an error is detected; it pushes the "offending command" object on
   the operand stack and sets stackRstr to false.

   If exec is true, StmToken returns only when an executable object
   is encountered; it pushes intervening literal objects on the
   operand stack. It also does not return executable array objects
   whose execution would be deferred; however, it does return an
   executable array object for a binary object sequence, which should
   be executed immediately.

   If exec is false, StmToken returns the first token encountered.
 */

extern boolean StrToken(/* StrObj ob, PStrObj rem, PObject pobj,
  boolean exec */);
/* Scans a token from a string object ob, returns the resulting object
   in *pobj, and returns a string object for the leftover tail in *rem.
   The exec argument and the result are as for StmToken.
 */

extern boolean LineComplete(/* Stm stm */);
/* Scans from stm until end-of-file, analyzing the syntax but not
   creating any objects. Returns true if the data comprises one or
   more complete objects or if a syntax error is detected. Returns
   false if the input is incomplete, i.e., has one or more unmatched
   open brackets. Does not ever raise an exception.
 */

extern procedure UndefNameIndex(/* char *table, integer index */);
/* Raises an "undefined" error arising from an undefined name index.
   table is either "system" or "user"; index is the offending index.
 */

extern procedure ReadBinObjSeq(/* Stm stm, PBosHeader hdr, PObject ret */);
/* Reads the body of a binary object sequence whose header has already
   been read into *hdr; returns the resulting object in *ret.
   The header is in the order it was read, i.e., not necessarily
   native order.

   If an error occurs, this procedure raises an exception. If the
   error was an undefined name, the offending object will already
   have been pushed and stackRstr set to false. Otherwise, it is
   the caller's responsibility to fabricate an offending command
   object.
 */

extern procedure ReadNumAry(/* Stm stm, PHNAHeader hdr, PObject ret */);
/* Reads the body of a homogeneous number array token whose header has
   already been read into *hdr; returns the resulting object in *ret.
   The header is in the order it was read, i.e., not necessarily
   native order. If an error occurs, this procedure raises an exception.
 */

extern procedure ScannerInit(/* InitReason reason */);
/* Initializes the scanner */

extern procedure BinObjInit(/* InitReason reason */);
/* Initializes the binaryobject module */

/* Exported Data */

#endif	SCANNER_H
