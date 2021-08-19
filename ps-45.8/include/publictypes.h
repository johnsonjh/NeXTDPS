/*
				publictypes.h

   Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE: All  information contained herein is  the property of  Adobe  Systems
Incorporated.  Many of  the  intellectual   and technical concepts  contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their  internal  use.  Any reproduction
or dissemination of this software is strictly forbidden unless  prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: 
Edit History:
Scott Byer: Thu May 18 09:33:22 1989
Ivor Durham: Thu Aug 11 08:45:50 1988
Jim Sandman: Fri Jun 23 10:48:06 1989
End Edit History.
*/

#ifndef	PUBLICTYPES_H
#define	PUBLICTYPES_H

/*
 * Note: for the moment we depend absolutely on an int being the same
 * as a long int and being at least 32 bits.
 */

#ifndef VAXC
#define readonly const
#endif VAXC

#define	_inline
#define	_priv	extern

/* Control Constructs */

#define until(x) while (!(x))

#define endswitch default:;

/* Inline Functions */

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

/* Declarative Sugar */

#define private static
#define public
#define global extern

#define procedure void

/* Type boolean based derived from unsigned int */

typedef unsigned int boolean;

#define true 1
#define false 0

/* Various size integers and pointers to them */

typedef char Int8, *PInt8;
typedef short int Int16, *PInt16;
typedef long int Int32, *PInt32;
typedef long int integer;

typedef integer (*PIntProc)();	/* pointer to procedure returning integer */

#define MAXInt32 ((Int32)0x7FFFFFFF)
#define MAXinteger ((integer)0x7FFFFFFF)
#define MINInt32 ((Int32)0x80000000)
#define MINinteger ((integer)0x80000000)

/* Various size cardinals and pointers to them */

typedef unsigned char Card8, *PCard8;
typedef unsigned short int Card16, *PCard16;
typedef unsigned long int Card32, *PCard32;
typedef unsigned short int cardinal;
typedef unsigned long int longcardinal;

#define MAXCard32 ((unsigned long)0xFFFFFFFF)
#define MAXlongcardinal MAXCard32
#define MAXunsignedinteger MAXCard32
#define MAXCard16 0xFFFF
#define MAXcardinal MAXCard16
#define MAXCard8 0xFF

/* Bit fields */

typedef unsigned BitField;

/* Reals and Fixed point */

typedef float real, *Preal;
typedef double longreal;

typedef long int Fixed;	/*  16 bits of integer, 16 bits of fraction */

/* Co-ordinates */

typedef real Component;

typedef struct _t_Cd {
  Component x,
            y;
} Cd, *PCd, RCd, *PRCd;

typedef struct _t_FCd {
  Fixed x, y;
  } FCd, *PFCd;

typedef struct _t_DevCd {
  integer x,
          y;
} DevCd, *PDevCd;

/* Characters and strings */

typedef unsigned char character, *string, *charptr;

/* Inline string function from C library */

#define StrLen(s)\
	os_strlen( (char *) (s) )

#define	NUL	'\0'

/* Generic Pointers */

#ifndef	NULL
#define	NULL	0
#endif	NULL

#define	NIL	NULL

typedef	void (*PVoidProc)();	/* Pointer to procedure returning no result */

/* Codes for exit system calls */

#if VAXC
#define exitNormal 1
#define exitError 2
#else VAXC
#define exitNormal 0
#define exitError 1
#endif VAXC

/* Generic ID for contexts, spaces, name cache, etc. */

#define	BitsInGenericIndex	10
#define	BitsInGenericGeneration	(32 - BitsInGenericIndex)

#define	MAXGenericIDIndex	((Card32)((1 << BitsInGenericIndex) - 1))
#define	MAXGenericIDGeneration	((Card32)((1 << BitsInGenericGeneration) - 1))

typedef union {
  struct {
    BitField index:BitsInGenericIndex;		/* Reusable component */
    BitField generation:BitsInGenericGeneration;/* Non-reusable component */
  } id;
  Card32  stamp;		/* Unique combined id (index, generation) */
} GenericID;			/* For contexts, spaces, streams, names, etc */

#endif	PUBLICTYPES_H
