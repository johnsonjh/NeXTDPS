/*
Copyright 1990 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of
Adobe Systems Incorporated.  Many of the intellectual and
technical concepts contained herein are proprietary to Adobe,
are protected as trade secrets, and are made available only to
Adobe licensees for their internal use.  Any reproduction or
dissemination of this software is strictly forbidden unless
prior written permission is obtained from Adobe.

Original version: Fred Drinkwater. Jun, 1990.
Edit History:
Scott Byer: Fri Aug 24 15:09:14 1990
End Edit History.
*/

#ifndef CSCANCOMMON_H
#define CSCANCOMMON_H

/*
 * cscancommon.h -
 *   Extracts from bc placed here to avoid touching publictypes.h etc.
 */
/*
 * USE68KATM gates use of assembly code versions in Macintosh world;
 * cf bc/<recent>/bc.log files for description.
 */
#ifndef USE68KATM
#define USE68KATM 0
#endif	/* USE68KATM */

#if USE68KATM
#define internal
#else
#define internal static
#endif

typedef unsigned char boolean8;

/* These differ from "int" and "unsigned int" in that they mean
 * "use whatever width you like".  It is assumed to be at least 16 bits.
 * "int" should be used only to talk to the OS and built-in libraries.
 */
typedef int IntX;		/* Natural int */
typedef unsigned int CardX;	/* Natural unsigned int */

#define MAXInt16 (0x7FFF)
#define MINInt16 ((Int16)0x8000)
#define MAXIntX MaxInt16

/*
 * NOTE:
 *  Fixed, Frac, and UFrac are defined slightly differently in 
 *  bc pubtypes -vs- 2ps basictypes publictypes.  Here, we defer
 *  to the 2ps basictypes definitions.
 */
typedef Fixed *PFixed;
typedef Frac *PFrac;


typedef struct {
  FCd bl;
  FCd tr;
  } FBBox, *PFBBox;

typedef struct {
  Fixed a, b, c, d, tx, ty;
  } FixMtx, *PFixMtx;

typedef struct {
  Frac a, b, c, d, tx, ty;
  } FracMtx, *PFracMtx;

typedef struct {
  real a, b, c, d, tx, ty;
  } RMtx, *PRMtx;

typedef enum {
  none, bc_zero, ad_zero, general
  } MtxType;

#if 0
/* I think these are irrelevant to cscan & cscanasm */
#if !USE68KATM

typedef struct {
  CardX mtxtype:2, imtxtype:2;
  boolean isFixed:1;
  boolean noYSkew:1;
  boolean noXSkew:1;
  boolean mirr:1;
  boolean switchAxis:1;
  /* 7 unused bits */
  union {
    FixMtx fx;
    FracMtx fr;
    } m;
  FixMtx im; /* inverse */
  Fixed len1000;
  } Mtx, *PMtx;

#else /* USE68KATM */

typedef struct {
  boolean8 switchAxis:1;
  boolean8 mirr:1;
  boolean8 noYSkew:1;
  boolean8 noXSkew:1;
  /* 4 unused bits */
  boolean8 isFixed;
  unsigned char mtxtype, imtxtype;
  Fixed len1000;
  /* matrices are long word aligned */
  union {
    FixMtx fx;
    FracMtx fr;
    } m;
  FixMtx im; /* inverse */
  } Mtx, *PMtx;

#endif /* USE68KATM */
#endif /* 0 */

#endif /* CSCANCOMMON_H */


