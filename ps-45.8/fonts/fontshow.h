/*
  fontshow.h

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
Ivor Durham: Fri Apr  8 17:25:19 1988
Ed Taft: Mon Dec 18 15:22:21 1989
Jim Sandman: Wed Oct  4 11:55:06 1989
Linda Gass: Tue Nov 10 11:44:40 1987
Bill Paxton: Sat Apr  2 10:24:17 1988
End Edit History.
*/

#ifndef	FONTSHOW_H
#define	FONTSHOW_H

#include BASICTYPES
#include GRAPHICS
#include LANGUAGE

#include "fontcache.h"
#include "fontspriv.h"

typedef struct _ShowState {
  struct _ShowState *link;	/* -> nested ShowState or next free */
  DevCd		fdcp;
  Cd		rdcp;
  BitField	noShow:1,	/* stringwidth */
		wShow:1,	/* widthshow or awidthshow */
		aShow:1,	/* ashow or awidthshow */
		kShow:1,	/* kshow */
		xShow:1,	/* xyshow or xshow */
		yShow:1,        /* xyshow or yshow */
		cShow:1, 	/* cshow */
		useReal:1,	/* position calculations require reals */
		inBuildChar:1,	/* BuildChar in progress */
		temporary:1,	/* this ShowState is allocated temporarily */
		havefont:1,	
		NewMID:1,	
		caching:1,	
		unused:3;	/* unused */
  Card16	wChar,		/* char code for widthshow or awidthshow */
		level;		/* show nesting level */
  Card16	fmap;
  StrObj	so;
  DevCd		fdW,
		fdA;
  Cd		rdW,
		rdA;
  CharMetrics	hMetrics, vMetrics;
  Object	ko;
  /* The following 2 fields, if non-null, refer to resources that are
     currently owned by this ShowState and not yet linked into the
     font cache data structures */
  PMask		mask;
  CIOffset	cio;	
  FCd		delta;	
  DevCd		fdinc;
  Cd		rdinc;
  Fixed		xymax, mtx_a, mtx_b, mtx_c, mtx_d;
  NumStrm	ns, initns;  /* for xyshow */
  PObject	pcn;
  AryObj	encAry, fdary, peary;
  StrObj	kvary, svary;
  UniqueMID	*miary;
  DictObj	fontDict, DelayedFont;
  AryObj	delayedAry;
  UniqueMID	DelayedMID;
} ShowState, *PShowState;

#define UPPERSHOWBOUND (integer)(1<<13)
#define LOWERSHOWBOUND (-UPPERSHOWBOUND)

#define crMID (curMT->umid.rep.mid)
  /* This is valid only if curMT is valid */

/* Exported Procedures */


extern boolean CvtToFixed(/* FCd *pfcd, Cd cd, integer lower, upper */);
extern procedure FontShwDataHandler(/* StaticEvent code */);
extern procedure PurgeFSCache(/* MID mid */);
extern boolean SetCchDevice(/* RMetrics *m; Cd *delta */);
extern boolean MakeCacheDev(/* .... */);
extern boolean MakeCacheDev2(/* .... */);
extern procedure CSRun(/* .... */);
extern procedure SimpleShowByName(/* integer c; Object *pcn */);
extern integer FastShow(/* boolean; */);
extern integer CompShow(/* boolean; */);
extern procedure CompSlowShow();
extern procedure FSInit();
extern boolean ReValidateMID();
extern boolean ShowByName(
  /*integer c; PObject cn; integer fno; PMTItem CFmp; */);
extern boolean SSSOutCall();

#define ValidMID() \
  ((AlreadyValidMID() && curMT->maskID == gs->device->maskID) \
   || ReValidateMID())
  /* Ensures that curMT points to the MTItem corresponding to the current
     font, matrix, and maskID. Must be called before accessing curMT
     following any activity that might cause MIDs to be reclaimed.
     If ValidMID returns true, gs->matrixID is up-to-date and curMT
     points to the corresponding MTItem. If ValidMID returns false, the
     MID table is full and the font cache cannot be used for the operation
     being performed; gs->matrixID is set to MIDNULL and curMT to NIL.

     Note: do not call ValidMID during the BuildChar procedure for
     the current font (e.g., in setcachedevice), since the MID
     so generated would have the FontMatrix concatenated twice.
   */

#define AlreadyValidMID() \
  (curMT != NIL && curMT->umid.stamp == gs->matrixID.stamp)

/* Exported Data */

#define sizeShowCache 900
extern DevMask *showCache;
extern ShowState *ss;

#ifndef XA
#define XA 0
#define RdSt "No way this will work"
#endif

#if XA
#define FRACTION 8
#define PFLTTOFIX(x,y) (y) = pflttoxfix((x))
#define FIXTOPFLT(x,y) xfixtopflt((x),(y))
#define FIXINT(x) ((x)<<8)
#define FTRUNC(x) ((x)>>8)
#define FROUND(x) (((x)+128)>>8)
#define FIXCD(cd,pdc) {pdc->x=pflttoxfix(&((cd).x));pdc->y = pflttoxfix(&((cd).y));}
#define UNFIXCD(dc,pcd) {xfixtopflt((dc).x,&((pcd)->x));xfixtopflt((dc).y,&((pcd)->y));}
#define FIXTOXFIX(x) ((x)>>8)

#else XA
#define FRACTION 16
#define PFLTTOFIX(x,y) (y) = pflttofix((x))
#define FIXTOPFLT(x,y) fixtopflt((x),(y))
#define FIXINT(x) FixInt(x)
#define FTRUNC(x) FTrunc(x)
#define FROUND(x) FRound(x)
#define FIXCD(cd,pdc) FixCd(cd,pdc)
#define UNFIXCD(dc,pcd) UnFixCd(dc,pcd)
#define FIXTOXFIX(x) (x)
#define CvtToXFixed CvtToFixed
#endif XA

#define FixedMoveTo(c) {Cd fmtcd; UNFIXCD(c,&fmtcd); MoveTo(fmtcd,&gs->path);}

#endif	FONTSHOW_H
