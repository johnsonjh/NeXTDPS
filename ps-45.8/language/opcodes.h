/*
  opcodes.h

Copyright (c) 1984, '85, '86, '87 Adobe Systems Incorporated.
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
John Gaffney: Thu Jan 10 12:20:57 1985
Ed Taft: Wed Apr 16 08:42:18 1986
Ivor Durham: Wed Jan 14 11:56:53 1987
End Edit History.
*/

#include BASICTYPES

typedef unsigned char Code;		/* encoded byte */
typedef cardinal NameIndex;		/* name index */
typedef cardinal CmdIndex;		/* command index */

/*
 *	Define encoding values for packed array elements
 */

#define InvalidEntry	0				/* invalid */

#define ObjectEscape	(InvalidEntry+1)		/* unencoded object */

#define LitNameBase	(ObjectEscape+1)		/* literal names */
#define NameCodes	64
#define MAXNameIndex	(NameCodes * 256 - 1)

#define ExecNameBase	(LitNameBase+NameCodes)		/* executable names */

#define CmdBase		(ExecNameBase+NameCodes)	/* commands */
#define CmdCodes	4
#define CmdValues	CmdCodes * 256

#define RealBase	(CmdBase+CmdCodes)		/* real constants */
#define RealCodes	3

#define IntegerBase	(RealBase+RealCodes)		/* small integers */
#define MinInteger	-1
#define MaxInteger	18
#define IntegerCodes	(MaxInteger - MinInteger + 1)

#define BooleanBase	(IntegerBase+IntegerCodes)	/* booleans */
#define	BooleanCodes	2

#define RelPkdary	(BooleanBase+BooleanCodes)	/* packed arrays */

#define RelString	(RelPkdary+1)			/* strings */

#define ExpansionRoom	(256-RelString)

/* opcode decoding table -- maps opcodes to opcode types */

extern readonly Code opType[];

#define INVALIDTYPE		0
#define ESCAPETYPE		1
#define LITNAMETYPE		2
#define EXECNAMETYPE		3
#define CMDTYPE			4
#define REALTYPE		5
#define INTEGERTYPE		6
#define BOOLEANTYPE		7
#define RELPKDARYTYPE		8
#define RELSTRINGTYPE		9


/* encoded real value table -- indexed by (code - RealBase) */

extern readonly real encRealValues[];


typedef unsigned short int RelAry;	/* relative array descriptor */

/* relative arrays look like (length:BitsForLength,reloffset:BitsForOffset) */

/* definitions for relative arrays */
#define BitsForLength		5	/* field size for length */
#define BitsForOffset		11	/* field size for offset */
#define MINArrayLength		1
#define MAXArrayLength		(MINArrayLength + (1<<BitsForLength) - 1)
#define MINOffset		-1
#define MAXOffset		(MINOffset - ((1<<BitsForOffset) - 1))
