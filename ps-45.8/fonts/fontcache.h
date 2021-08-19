/*
  fontcache.h

Copyright (c) 1984, '85, '86, '87, '88, '90 Adobe Systems Incorporated.
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
Ivor Durham: Sun Aug 14 14:00:18 1988
Ed Taft: Thu Jul 28 17:37:22 1988
Jim Sandman: Mon May  8 16:48:31 1989
Joe Pasqua: Wed Dec 14 19:30:48 1988
Paul Rovner: Thu Aug 10 14:23:26 1989
Chuck Jordan: Thu Mar 22 11:34:30 PST 1990
End Edit History.
*/

#ifndef	FONTCACHE_H
#define	FONTCACHE_H

#include BASICTYPES
#include GRAPHICS
#include ENVIRONMENT

#include "fontspriv.h"

#define dfCompThreshold 1250	/* min size for compressed character */
#define dfCacheThreshold 12500	/* max size for any cached character */

/* Data Types */

/* statistical data for Cache */

typedef struct {
  Int32   MIDcnt,
          CIcnt;
} FCDataRec;

/* Exported Procedures */

extern	RstrFC(/*name:string*/);
extern	SaveFC(/*name:string*/); /*FontDisk*/

/* Semi-Public Procedures (used within package only) */

extern	CIOffset	CIAlloc(/*dontretry*/);
extern	procedure	CIFree(/*CIOffset*/);
extern	procedure	StartCache();

extern procedure GetFontDirectory(/* PDictObj fd */);
/* Stores in *fd the current binding of FontDirectory, i.e., private or
   shared according to the current VM allocation mode. If the value
   is a TrickyDict, it is dereferenced.
 */

extern procedure GetPrivFontDirectory(/* PDictObj fd */);
/* Stores in *fd the private FontDirectory, disregarding the current
   VM allocation mode. If the value is a TrickyDict, it is dereferenced.
 */


/*****************  MID management ****************/

extern Card16 HashMID(/*
  FID fid, PMtx mtx, DevShort maskID, Card16 tblSize */);
/* Computes a hash probe using fid, mtx, and maskID as a key; returns
   a result in [0, tblSize). maskID should be 0 except when generating
   a hash probe for a setfont MID.
 */

extern MID InsertMID(/*
  FID fid, PMtx cmtx, DictObj fontdict, DevShort maskID,
  boolean (*mcomp)(Mtx amtx, btmx),
  boolean mf */);
/* Attempts to find or create a suitable MID for the specified arguments;
   returns that MID if successful, MIDNULL if the cache is full.
   If mf is true, a makefont MID is desired; if false, a setfont MID.
   fontdict is the "original" font dictionary (passed to definefont);
   it may be zero or more generations removed from the current font.
   The matrix cmtx describes the desired transformation relative
   to the original font; for a setfont MID, this also has the CTM
   postmultipied. The mcomp procedure is the matrix compare used during
   the lookup; it should compute "exactly equal" for a makefont MID
   and "close enough" for a setfont MID. The maskID argument is used
   only for a setfont MID (it should be zero otherwise); it identifies
   the device for which the MID's masks are valid.
 */

extern MID MakeMID(/* DictObj fdict, PMtx mtx, DevShort maskID */);
/* Attempts to find or create a suitable setfont MID for the specified
   arguments; returns that MID if successful, MIDNULL if the cache is full.
   fdict is the current font dictionary; mtx is the CTM.
 */

extern MID MMEldest();
extern MID MSEldest();
extern procedure InitMID();
extern procedure PurgeMID(/* MID mid; */);
extern procedure PurgeOnGC();
extern procedure PurgeOnRstr();

#define DecrSetMIDAge(mid) MA[mid] = --CurrentMIDAge
#define SetMIDAge(mid) MA[mid] = CurrentMIDAge
#define SetCharAge(c) (c)->touched = true
#define PeterPan 0x0
#define Methuselah 0xffffffff


extern	Int32		MTSize;	/* fixed at initialization */
extern	Int32		MMSize;	/* fixed at initialization */
extern	Int32		MSSize;	/* fixed at initialization */
extern MID		*MM, *MMEnd;
extern MID		*MS, *MSEnd;
extern MID		MTFreeHead;
extern PCard32		MA;
extern Card32		CurrentMIDAge;
extern PCard16		IndMArray, IndSArray;
extern integer		*MIDCount;


/*********** Select Font Cache Management **********/

extern procedure InitSFCache();
extern procedure PurgeSFForKey(/* Object key; */);
extern procedure PurgeSFForMID(/* MID m; */);

extern boolean SearchSelectFont(
  /* Object key, PMtx mtx, boolean isScaleFont, PDictObj pDict */);
/* Searches for font specified by [key, mtx] in the selectfont cache.
   If it is present, stores the corresponding makefont dictionary in
   *pDict and returns true; if not, returns false. If isScaleFont is true,
   this is a "scalefont" form of "selectfont"; the scale is given in
   mtx->a and the rest of the matrix is undefined. If isScaleFont is false,
   the transformation is specified by the complete matrix *mtx.
   This procedure correctly observes the private/shared FontDirectory
   semantics; i.e., it ignores the private FontDirectory if the current
   VM allocation mode is shared. It resets the age for the makefont MID.
 */

extern procedure InsertSelectFont(
  /* Object key, PMtx mtx, boolean shared, MID mid */);
/* Inserts [key, mtx] into the selectfont cache with the value mid,
   which must be a valid makefont MID.
   Caller asserts that the [key, mtx] is not already in the cache.
   shared means that the key is in SharedFontDirectory; caller asserts
   that the key is not also in private FontDirectory.
 */


/******************  CI Management  *********************/

extern procedure DeleteCIs(/* MID m; */);
extern boolean FlushMID(/* MID m; */);
extern CIOffset CIAlloc();
extern procedure CIFree(/* CIOffset cio; */);
extern procedure SortCharAges();

extern	Int32		CISize; /* fixed at initialization */
extern CIItem		*CI, *CIEnd;
extern	CIOffset	CIFreeHead;

extern PNameEntry	*CN; /* parallel array to CI for nameentries */
extern integer		charSortInterval;
#define MaxCharSortInterval 300

/* Exported Data */

extern	integer defaultCompThreshold;
extern	integer defaultCacheThreshold;

extern	boolean		cchInited, dlFC;
extern	FCDataRec	fcData;

#if STAGE == DEVELOP
extern boolean fcdebug, fchange, fcCheck;
#endif STAGE == DEVELOP

#endif	FONTCACHE_H
