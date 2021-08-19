/*
  fontspriv.h

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

Original version: 
Edit History:
Chuck Geschke: Mon Dec 23 09:37:49 1985
Ed Taft: Mon Jun 13 13:08:05 1988
Doug Brotz: Wed Aug 20 19:52:28 1986
Bill Paxton: Fri Jun 13 14:50:26 1986
Ivor Durham: Sun Aug 14 14:00:50 1988
Jim Sandman: Thu May 11 16:46:21 1989
Linda Gass: Mon Nov 30 18:13:13 1987
Joe Pasqua: Wed Dec 14 15:12:58 1988
End Edit History.
*/

#ifndef	FONTSPRIV_H
#define	FONTSPRIV_H

#include BASICTYPES
#include ENVIRONMENT
#include GRAPHICS

/* types and constants */

/*
  Storage types -- used in both CIItem.type and MTItem.type, though
  their semantics are slightly different. The values of this
  enumeration also specify individual attributes in the low 3 bits.
  These specify cache storage classes; if any are non-zero, the item
  is allocated and in use.

  There are 3 classes of cache storage:
    V = volatile -- in RAM and purgeable

  A CIItem's type describes the storage classes occupied by an individual
  cached character. Note that a character can occupy two storage classes
  simultaneously (Fmem, Vmem). For a CIItem to be present at all,
  it must at least be in class Fmem or Vmem.

  An MIDItem's storage class is, in principle, the union of the storage
  classes of all characters cached for the MID. However, this is not
  done consistently; only Vmem and FVmem are actually used. The storage
  class is meaningful only for setfont MIDs, but it is set for makefont
  MIDs also.
 */
#define freemem 0	/* not allocated */
#define Vmem 1		/* volatile storage */
#define allocmem 2	/* allocated but not completely filled in yet */

typedef Card32 FID;  /* unique id for a font name */
#define FIDNULL 0

/*

The FID for a font needs to encode the prebuilt options for the font
as well as the identity of the font. The FID is now a function of
type of font, unique ID, and prebuilt options.

*/

#define clientFID 0
#define adobeFID 1
#define ndcFID 2
#define userFID 3

#define useBitmapWidthsFID 0x80	    /* this value adjusts type of FID */
#define transformFIDShift 2	    /* this value adjusts type of FID */
#define transformFIDMask 0x1F	    /* this value adjusts type of FID */
#define exactSizeFIDFactor 1
#define inBetweenSizeFIDFactor 3
#define transformedCharFIDFactor 9

#define shiftFIDType 24
#define prebuiltFIDMask 0x3FFFFFF   /* this masks of adjustments to FID */
#define maskFIDType 0x3

#define TypeOfFID(fid) (((fid)>>shiftFIDType)&maskFIDType)
#define BitmapWidthsFID(fid) (((fid)>>shiftFIDType)&useBitmapWidthsFID)
#define TransformFID(fid) \
  ((((fid)>>shiftFIDType)>>transformFIDShift)&transformFIDMask)

#define COMPOSEDtype 0
#define ENCRPTVMtype 1
#define ENCRPTFILEtype 2
#define PSVMtype 3
#define PSFILEtype 4
#define CHRMAPtype 5
#define ECHRMAPtype 6

#define LASTFONTTYPE ECHRMAPtype

#define FMapNone 0
#define FMapFSA 1
#define FMap88 2
#define FMapEscape 3
#define FMap17 4
#define FMap97 5
#define FMapGen 6


typedef struct {
  DevCd   ll,
          ur;
} DevBB;

/*
  An MID is an index into the MTItem array.
  There are two types of MIDs, "makefont" (mf) and "setfont" (sf).
  They are logically separate; they occupy the same identifier space
  and share representations only because many of the algorithms for
  managing them are the same for both.

  A makefont MID has a 4-part key:
    [FID, makefont matrix, origdict, space]
  where origdict is the original (definefont) font dictionary and
  space identifies the private VM of the context executing makefont.
  It is generated when a makefont or scalefont is done; its value
  is the transformed font dictionary (mfdict) produced by makefont or
  scalefont. Makefont MIDs serve only to cache those dictionaries so
  that they are re-used by a subsequent makefont or scalefont with the
  same operands. Note that there can be multiple makefont MIDs for a
  given font, representing different transformations, re-encodings, etc.;
  all such MIDs will refer to the same origdict but different mfdicts.
  Makefont MIDs are not involved at all in the generation or caching
  of individual characters.

  A setfont MID (misnamed, because it is really used only during show)
  has a 3-part key:
    [FID, (makefont matrix * CTM), maskID]
  It is generated when a character is shown from a font with a given
  FID and complete transform on a device with a given maskID.
  All the cached characters (CIItems) for a given FID and complete
  transform are labelled with this MID, independent of the font
  dictionary they were built from. The "current" MID (gs->matrixID)
  is always a setfont MID; curMT is a cached pointer to the
  corresponding MTItem.

  There are separate hash tables and aging lists for the two types
  of MIDs: MM and MMEldest for makefont MIDs; MS and MSEldest for
  setfont MIDs.

  An original font dictionary installed by definefont can be in either
  private or shared VM. Transformed font dictionaries created by
  makefont and scalefont are match the VM of the original font.
  Setfont MIDs and the associated CIItems are independent of VM and
  are valid in all VMs.

 */

typedef struct {
  boolean	mfmid:1;	/* 1 = makefont MID, 0 = setfont MID */
  /* following 7 one-bit fields valid only for setfont MIDs */
  boolean	shownchar:1;	/* char was output for this MID sometime */
  boolean	monotonic:1;	/* this font's width vectors are monotonic */
  boolean	xinc:1;		/* at least one char's x width is positive */
  boolean	yinc:1;		/* at least one char's y width is positive */
  boolean	xdec:1;		/* at least one char's x width is negative */
  boolean	ydec:1;		/* at least one char's y width is negative */
  boolean	monotonic1:1;	/* this font's width vectors are monotonic */
  boolean	xinc1:1;	/* at least one char's x width is positive */
  boolean	yinc1:1;	/* at least one char's y width is positive */
  boolean	xdec1:1;	/* at least one char's x width is negative */
  boolean	ydec1:1;	/* at least one char's y width is negative */
  CItype	type:4;		/* storage type (only 4 bits needed) */
  BitField	hash:8;		/* hash index for this item; <= MAXMSize */
  BitField	pe:1;		/* has PrefEnc (composite font) */
  BitField	mapType:3;	/* map type (composite font) */
  BitField	unused:4;
  /* following 4 fields declared as "Card16 xx" instead of as "MID xx:16"
     in order to avoid VAX compiler bug */
  Card16/*MID*/	mlink;		/* next in hash chain */
  Card16/*MID*/	maskID;		/* device for which this sf MID is valid */
  FID		fid;		/* font ID key */
  Mtx		mtx;		/* matrix key (see comments above) */
  UniqueMID	umid;		/* full identifier for this MID */
  union {
    struct {			/* makefont MID variant part */
      DictObj	origdict,	/* original (definefont) font */
		mfdict;		/* font derived by makefont or scalefont */
      GenericID	dictSpace;	/* private VM in which mfdict exists */
    } mf;
    struct {			/* setfont MID variant part */
      DevBB	bb;		/* bbox enclosing all cached chars */
      DevBB	bb1;		/* bbox enclosing all cached chars */
    } sf;	
  } values;
} MTItem, *PMTItem;
/* sizeof(MTItem) MUST be multiple of 4 bytes!! (for AssembleROM) */

extern	PMTItem		MT, MTEnd;

#define MAXMSize          256
#define MAXMasks          8000
#define MAXMTSize         MAXMasks/12

#define BytesForMTPE(n) (((n)+1)>>1)
#define GetMTPE(p, i) (((i)&1)? (((p)[(i)>>1])>>4) : (((p)[(i)>>1])&0xF))
#define SetMTPE(p, i, v) \
  if ((i) & 1) { \
    (p)[(i)>>1] &= 0xF; \
    (p)[(i)>>1] |= ((v) << 4); \
    } \
  else { \
    (p)[(i)>>1] &= 0xF0; \
    (p)[(i)>>1] |= (v); \
    } 

#define PEMASK 8
#define MTMASK 7

#define szSelectFontCache 25	/* total entries in selectfont cache */
#define szSelectFontHash 7	/* size of hash table (prime) */

typedef struct _SFCEntry {
  struct _SFCEntry *link;	/* -> next in chain */
  PNameEntry key;		/* font name (must be nameObj) */
  Mtx mtx;			/* makefont matrix */
  BitField isScaleFont:1,	/* true if mtx.a == mtx.d and all others 0 */
    shared: 1,			/* key is in SharedFontDirectory (only) */
    unused: 6,			/* unused */
    hash: 8,			/* hash index for this entry */
    mid: 16;			/* makefont MID */
} SFCEntry, *PSFCEntry;

typedef struct {
  PSFCEntry hashTable[szSelectFontHash];
  SFCEntry entries[szSelectFontCache];
#if STAGE==DEVELOP
  Card32 searches, hits, reorders;
#endif STAGE==DEVELOP
} SFCache, *PSFCache;

extern PSFCache sfCache;
extern PSFCEntry sfcFreeList,	/* -> head of free list */
  sfcRover;			/* -> next entry to displace */

/* macros for sequencing though all items in MM, MS, MT, and CI */

#define forallMM(x) for(x = &MM[0]; x < MMEnd; x++)
#define forallMS(x) for(x = &MS[0]; x < MSEnd; x++)
#define forallMT(x) for(x = &MT[1]; x < MTEnd; x++)
#define forallCI(x) for(x = &CI[1]; x < CIEnd; x++)
#define forallSFC(phead, psf) \
  for (phead = &sfCache->hashTable[0]; \
       phead < &sfCache->hashTable[szSelectFontHash]; ) \
    for (psf = *phead++; psf != NIL; psf = psf->link)



#define MaxSSS 6

#endif	FONTSPRIV_H
