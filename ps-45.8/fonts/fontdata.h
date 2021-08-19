/*
  fontdata.h

Copyright (c) 1987-1990 Adobe Systems Incorporated.
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
Scott Byer: Tue Aug 21 11:23:26 1990
Ivor Durham: Fri Jun 24 12:12:43 1988
Ed Taft: Fri Feb  9 13:52:34 1990
Jim Sandman: Mon Sep 25 15:17:57 1989
Paul Rovner: Thu Aug 31 17:25:11 1989
Bill Paxton: Tue May 15 13:30:00 1990
End Edit History.
*/

#ifndef	FONTDATA_H
#define	FONTDATA_H

#include BASICTYPES
#include GRAPHICS

#include "fontshow.h"
#include "atm.h"

/* definitions from fontbuild */

typedef struct {
  Fixed a, b, c, d, tx, ty;
  } FixMtx, *PFixMtx;

typedef struct _t_RMetrics {
  RCd w0, w1, c0, c1, v;
  } RMetrics;
  
typedef struct _t_FMetrics {
  FCd w0, w1, c0, c1, v;
  } FMetrics;
  
typedef struct {
  Frac a, b, c, d, tx, ty;
  } FracMtx, *PFracMtx;

typedef struct {
  real a, b, c, d, tx, ty;
  } RMtx, *PRMtx;

typedef enum {
  none, identity, flipY, bc_zero, ad_zero, general
  } MtxType;

typedef struct {
  BitField mtxtype:2, imtxtype:2;
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
  procedure (*tfm)(), (*itfm)(), (*dtfm)(), (*idtfm)();
  } FntMtx, *PFntMtx;

#define MAXSTDW (12)
#define MAXSTMS (12)
#define MAXBLUES (12)
#define MAXBLUELEN (MAXBLUES+2)
#define MAXOTHERBLUELEN (MAXBLUES-2)
#define MAXWEIGHTS (16)

typedef struct {
  boolean _dooffsetlock:1,
          _hasCDevProc:1,
          _usefix:1,
          _outline:1,
	  _doFixup:1,
	  _toosmall:1,
	  _glcrPrepass:1,
	  _glcrOn:1,
	  _glcrFailure:1,
	  _devsweven:1,
	  _glcrRoundUp:1;
  integer _PccCount;
  FntMtx _fntmtx;
  short int _locktype, _gsfactor;
  Fixed _rndwidth, _idealwidth;
#if GLOBALCOLORING
  short int _nGlbClrs, _nGlbCntrs;
  string _glbClrLst, _endGlbClrLst, _freeGlbClr, _pGblClrBuf;
  string _glbCntrLst, _endGlbCntrLst, _freeGlbCntr, _pGblCntrBuf;
#endif
  Fixed _stdvw[MAXSTDW], _stdhw[MAXSTDW];
  short int _lenstdvw, _lenstdhw;
  FCd _lockoffset;
  Fixed _blueScale, _blueShiftStart, _erosion;
  struct _LokData *_pLokData;
  procedure (*_tfmLockPt)();
  procedure (*_lineto)();
  procedure (*_curveto)(); 
  procedure (*_moveto)();
  procedure (*_closepath)();
  procedure (*_endchar)();
  

  } FontBuildData;


/* definitions from fontshow */


typedef struct {
  PMTItem _curMT;		/* cached -> MTItem for gs->matrixID */
  ShowState *_ss;  /* pointer to state for current level of show */
  integer _showLevel;  /* number of BuildChar calls in progress */
  DictObj _pfont;
  } FontShowData;


typedef struct {
  FontBuildData fontBuild;
  FontShowData fontShow;
  integer _compThreshold;
  integer _cacheThreshold;
  } FontCtx, *PFontCtx;

extern PFontCtx fontCtx;

#define curMT (fontCtx->fontShow._curMT)
#define showLevel (fontCtx->fontShow._showLevel)
#define ss (fontCtx->fontShow._ss)
#define ctxCompThreshold (fontCtx->_compThreshold)
#define ctxCacheThreshold (fontCtx->_cacheThreshold)
#define pfont (fontCtx->fontShow._pfont)

#endif	FONTDATA_H
