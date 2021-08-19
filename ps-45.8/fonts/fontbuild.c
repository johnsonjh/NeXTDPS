/*
  fontbuild.c

Copyright (c) 1984-1990 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Chuck Geschke: November 5, 1983
Edit History:
Scott Byer: Wed Aug 29 15:39:58 1990
Chuck Geschke: Fri Dec 13 13:57:11 1985
Andrew Shore: Thu Jun 14 13:42:53 1984
Doug Brotz: Mon Jan 19 17:25:40 1987
Ed Taft: Mon Feb 12 15:56:38 1990
Bill Paxton: Fri Jun 15 10:13:15 1990
John Gaffney: Fri Feb  1 13:02:51 1985
Dick Sweet: Tue Feb 17 14:15:19 PST 1987
Ivor Durham: Wed Aug 17 15:31:43 1988
Jim Sandman: Thu Mar  8 14:05:31 1990
Linda Gass: Tue Nov 10 11:56:35 1987
Paul Rovner: Wednesday, September 21, 1988 3:23:22 PM
Perry Caro: Wed Nov  9 14:10:18 1988
Joe Pasqua: Tue Feb 28 13:27:16 1989
End Edit History.
*/

#include "atm.h"

#ifndef DPS_CODE
#define DPS_CODE (0)
#endif

#ifndef CSCANRETRIES
#define CSCANRETRIES	10
#endif	CSCANRETRIES

#define DO_OFFSET (0)
#define FBDEBUG (0)

#if FBDEBUG
#define FD(x) ((float)(x) / 65536.0)
#endif

#if !ATM
#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include FP
#include LANGUAGE
#include GRAPHICS
#include PSLIB
#include VM
#include "fontdata.h"
#include "fontbuild.h"
#include "fontcache.h"
#include "fontspriv.h"
#include "fontshow.h"
#include "fontmatrix.h"
#if !PPS
#include "fontsnames.h"
#endif
#else /* ATM */
#include <setjmp.h>
#include "pubtypes.h"
#include "font.h"
#include "fontpriv.h"
#include "buildch.h"
#include "matrix.h"
#endif /* ATM */

#if PPS
#define RAISE raise
#define rootSysDict root->sysDict
#define rootInternalDict root->internalDict
#define os_labs(x) abs(x)   
#define os_sqrt(x) sqrt(x)   
#define os_fabs(x) (RealLt0(x)?-(x):(x))
#define os_malloc(i) malloc(i)
#define PSPushPReal PushPReal
#define PSPopPReal  PopPReal
#define PSPopInteger PopInteger
private NameObj BCcmdName, strokewidthname, charstringsname,
                   pnttypnm, fntpthnm, stdencname, notdefname, chardataname,
                   bluename, mnftrnm, blfuzznm, subrsnm, mtrcsnm,
                   scrtnm, stkwdthname, bslnname, cphghtname, bovrname,
                   xhghtname, dscndname, ascndname, ovrshtname, bboxname,
                   xovrname, capovrname, aovrname, hlfswname, ocsbrsnm,
                   fdgbndsnm, eNm, lNm, ErNm, charoffsetsname,
                   blueShiftName, blueScaleName, lenIVname,
                   otherBluesName, engineclassnm, idealwidthNm, gsfactorNm,
                   stdhwNm, stdvwNm, stemsnaphNm, stemsnapvNm,
                   famBluesNm, famOtherBluesNm, encname, rndwidthNm,
#if GLOBALCOLORING 
		   rndstmname, expfctrname,
#endif
		   cdevprocname, metricsname, wvname;

#define FTruncF(x) ((Fixed) ((integer)(x) & 0xFFFF0000))
#define FCeilF(x) (FTruncF((x) + 0xFFFF))
#define FCeil(x) (((integer)(x+0xFFFF))>>16)
  /* Returns the smallest integer >= the Fixed number x. */
#define FRoundF(x) ((((integer)(x))+(1<<15)) & 0xFFFF0000)
#else /*PPS*/
#define rootSysDict rootShared->vm.Shared.sysDict
#define rootInternalDict rootShared->vm.Shared.internalDict
#define BCcmdName fontsNames[nm_BuildChar]
#define matrixname fontsNames[nm_FontMatrix]
#define fntypname fontsNames[nm_FontType]
#define prvtnm fontsNames[nm_Private]
#define pnttypnm fontsNames[nm_PaintType]
#define encname fontsNames[nm_Encoding]
#define metricsname fontsNames[nm_Metrics]
#define wvname fontsNames[nm_WeightVector]
#define bboxname fontsNames[nm_FontBBox]
#define metrics2name fontsNames[nm_Metrics2]
#define cdevprocname fontsNames[nm_CDevProc]
#define fdgbndsnm fontsNames[nm_FudgeBands]
#define ErNm fontsNames[nm_Erode]
#define eNm fontsNames[nm_erosion]
#define stdencname fontsNames[nm_StandardEncoding]
#define bluename fontsNames[nm_BlueValues]
#define otherBluesName fontsNames[nm_OtherBlues]
#define famBluesNm fontsNames[nm_FamilyBlues]
#define famOtherBluesNm fontsNames[nm_FamilyOtherBlues]
#define engineclassnm fontsNames[nm_engineclass]
#define stdhwNm fontsNames[nm_StdHW]
#define stemsnaphNm fontsNames[nm_StemSnapH]
#define stdvwNm fontsNames[nm_StdVW]
#define stemsnapvNm fontsNames[nm_StemSnapV]
#define scrtnm fontsNames[nm_password]
#define lNm fontsNames[nm_locktype]
#define strokewidthname fontsNames[nm_StrokeWidth]
#define blfuzznm fontsNames[nm_BlueFuzz]
#define blueShiftName fontsNames[nm_BlueShift]
#define blueScaleName fontsNames[nm_BlueScale]
#define lenIVname fontsNames[nm_lenIV]
#define rndstmname fontsNames[nm_RndStemUp]
#define expfctrname fontsNames[nm_ExpansionFactor]
#define subrsnm fontsNames[nm_Subrs]
#define ocsbrsnm fontsNames[nm_OtherSubrs]
#define dscndname fontsNames[nm_descend]
#define ascndname fontsNames[nm_ascend]
#define ovrshtname fontsNames[nm_overshoot]
#define xovrname fontsNames[nm_xover]
#define capovrname fontsNames[nm_capover]
#define aovrname fontsNames[nm_aover]
#define hlfswname fontsNames[nm_halfsw]
#define stkwdthname fontsNames[nm_strokewidth]
#define bslnname fontsNames[nm_baseline]
#define cphghtname fontsNames[nm_capheight]
#define bovrname fontsNames[nm_bover]
#define xhghtname fontsNames[nm_xheight]
#define charstringsname fontsNames[nm_CharStrings]
#define notdefname fontsNames[nm_notdef]
#define chardataname fontsNames[nm_CharData]
#define charoffsetsname fontsNames[nm_CharOffsets]
#define rndwidthNm fontsNames[nm_rndwidth]
#define idealwidthNm fontsNames[nm_idealwidth]
#define gsfactorNm fontsNames[nm_gsfactor]
#endif /*PPS*/


/* ************* shared declarations ************* */

#define wbnd (FixInt(35)/100)  /* max allowable value is .35 */

#define MAXlokpairIndex 63
#define Nlokpairs (MAXlokpairIndex+1)

typedef struct
  {
  Fixed c1;  /* the ideal location in character space */
  Fixed c2;  /* the locked location in character space */
  Fixed slope;
  Fixed hw1; /* the ideal half width in character space */
  Fixed hw2; /* the locked half width in character space */
  } LokPair, *PLokPair;

typedef struct
  {
  Int16 count;
  LokPair pair[Nlokpairs /*64*/ ];
  Fixed unitDist; /* char space distance equal to 1 pixel */
  } LokSeq, *PLokSeq;

#if !ATM
typedef struct _LokData
  {
  LokSeq X, Y;
  BitField slopesInited: 1,
	   temporary: 1,
	   inUse: 1,
	   fixedmap: 1,
	   fixmapok: 1;
  } LokData, *PLokData;
#else /* ATM */
typedef struct _LokData
  {
  Int16 fillerX; /* long word align LokPairs in X*/
  LokSeq X;
  Int16 fillerY; /* long word align LokPairs in Y*/
  LokSeq Y;
  } LokData, *PLokData;
#endif /* ATM */

#if GLOBALCOLORING

typedef struct _gclist {
  struct _gclist *next;
  Fixed w; /* device width */
  } GCList, *PGCList;

#include "glbclr.h"

#endif /* GLOBALCOLORING */

#if !ATM  /* ********** header for PS ************* */

#define internal static

/* added this line here because of forward reference : LS */
private Fixed RoundSW( /* Fixed swval */);
private boolean secret;	/* for secret charpaths */
public procedure CScan();

#define CHECKOKBUILD (false) /* OkToBuild must be redone, else will break */

#define FixedOne 0x10000
#define FixedHalf 0x8000L
#define FixedTwo 0x20000
#define gsmatrix (&fntmtx)
#define FntTfmP(c1,c2) (*gsmatrix->tfm)(c1,c2)
#define FntDTfmP(c1,c2) (*gsmatrix->dtfm)(c1,c2)
#define FntITfmP(c1,c2) (*gsmatrix->itfm)(c1,c2)
#define FntIDTfmP(c1,c2) (*gsmatrix->idtfm)(c1,c2)

typedef integer IntX;

extern real strkFoo; /* controls amount of variable erosion in stroke code */
extern boolean SetCchDevice(), SetCchDevice2();
extern procedure FlexProc(), FlexProc2(), RFlexProc();

public PVoidProc fetchCharOutline;

private boolean setupCache;

typedef struct{
  Mtx imtx, fmtx, itmtx;
  Int16 acode, ccode;
  real cdx, cdy, adx, ady;
  Object ccn, acn;
  } CCInfo, *PCCInfo;

#if STAGE==DEVELOP
private integer cslimit, oflimit;
#define CSLIMIT cslimit  /* max pixelsize at which use cscan */
#define OFLIMIT oflimit  /* pixelsize at which dont offsetfill */
#else
#define CSLIMIT 800  /* max pixelsize at which use cscan */
#define OFLIMIT 2000  /* pixelsize at which dont offsetfill */
#endif /* STAGE==DEVELOP */

extern Fixed F_Dist(); /* exported by stroke.c */
extern procedure FFCurveTo();
extern procedure ResizeCrossBuf();
extern procedure ResetCScan();
extern procedure CSClose();
extern procedure CSNewPoint();

typedef Fixed *PFixed;

#if GLOBALCOLORING

typedef struct {
  char *ptr;		/* Ptr to bytes in buffer */
  Card32 len;		/* Number of bytes in buffer */
  } GrowableBuffer, *PGrowableBuffer;

private GrowableBuffer gblClrBuf1, gblClrBuf2, gblClrBuf3;

#endif /* GLOBALCOLORING */

#if !PPS
#define pLokData (fontCtx->fontBuild._pLokData)
#define dooffsetlock (fontCtx->fontBuild._dooffsetlock)
#define isoutline (fontCtx->fontBuild._outline)
#define fntmtx (fontCtx->fontBuild._fntmtx)
#define locktype (fontCtx->fontBuild._locktype)
#define usefix (fontCtx->fontBuild._usefix)
#define doFixupMap (fontCtx->fontBuild._doFixup)
#define toosmall (fontCtx->fontBuild._toosmall)
#define lockoffset (fontCtx->fontBuild._lockoffset)
#define blueScale (fontCtx->fontBuild._blueScale)
#define gsfactor (fontCtx->fontBuild._gsfactor)
#define rndwidth (fontCtx->fontBuild._rndwidth)
#define idealwidth (fontCtx->fontBuild._idealwidth)
#define blueShiftStart (fontCtx->fontBuild._blueShiftStart)
#define erosion (fontCtx->fontBuild._erosion)
#define tfmLockPt (fontCtx->fontBuild._tfmLockPt)
#define lineto (fontCtx->fontBuild._lineto)
#define curveto (fontCtx->fontBuild._curveto)
#define moveto (fontCtx->fontBuild._moveto)
#define closepath (fontCtx->fontBuild._closepath)
#define glcrPrepass (fontCtx->fontBuild._glcrPrepass)
#define glcrOn (fontCtx->fontBuild._glcrOn)
#define glcrFailure (fontCtx->fontBuild._glcrFailure)
#define devsweven (fontCtx->fontBuild._devsweven)
#define glcrRoundUp (fontCtx->fontBuild._glcrRoundUp)
#define nGlbClrs (fontCtx->fontBuild._nGlbClrs)
#define nGlbCntrs (fontCtx->fontBuild._nGlbCntrs)
#define glbClrLst ((PGlbClr)(fontCtx->fontBuild._glbClrLst))
#define endGlbClrLst ((PGlbClr)(fontCtx->fontBuild._endGlbClrLst))
#define freeGlbClr ((PGlbClr)(fontCtx->fontBuild._freeGlbClr))
#define glbCntrLst ((PGlbCntr)(fontCtx->fontBuild._glbCntrLst))
#define endGlbCntrLst ((PGlbCntr)(fontCtx->fontBuild._endGlbCntrLst))
#define freeGlbCntr ((PGlbCntr)(fontCtx->fontBuild._freeGlbCntr))
#define pGblClrBuf ((PGrowableBuffer)(fontCtx->fontBuild._pGblClrBuf))
#define pGblCntrBuf ((PGrowableBuffer)(fontCtx->fontBuild._pGblCntrBuf))
#define stdvw (fontCtx->fontBuild._stdvw)
#define stdhw (fontCtx->fontBuild._stdhw)
#define lenstdvw (fontCtx->fontBuild._lenstdvw)
#define lenstdhw (fontCtx->fontBuild._lenstdhw)
#define PccCount (fontCtx->fontBuild._PccCount)
#define hasCDevProc (fontCtx->fontBuild._hasCDevProc)
#define endchar (fontCtx->fontBuild._endchar)
#else /*PPS*/
private PLokData pLokData;
private boolean dooffsetlock, doFixupMap, devsweven;
public boolean isoutline;
private IntX locktype, gsfactor;
private Fixed rndwidth, idealwidth;
private boolean usefix;
private boolean toosmall;
private FCd lockoffset;
private Fixed blueScale, blueShiftStart;
public  Fixed erosion;
public procedure (*tfmLockPt)();
public procedure (*lineto)();
public procedure (*curveto)();
private procedure (*moveto)();
private procedure (*closepath)();
private boolean glcrPrepass, glcrOn, glcrFailure, glcrRoundUp;
#if GLOBALCOLORING
private short int nGlbClrs, nGlbCntrs;
private PGlbClr glbClrLst, endGlbClrLst, freeGlbClr;
private PGlbCntr glbCntrLst, endGlbCntrLst, freeGlbCntr;
private PGrowableBuffer pGblClrBuf, pGblCntrBuf;
#endif
private Fixed stdvw[MAXSTDW], stdhw[MAXSTDW];
private IntX lenstdvw, lenstdhw;
private int PccCount;
private boolean hasCDevProc;
private procedure (*endchar)();
#endif /*PPS*/

typedef struct {
  PLokData _stdLokData;
  AryObj  _emptyarray,
          _stdencvec;
  Fixed   _maxOffsetVal,
          _info_devsw,
          _info_offsetval,
          _info_fooFactor;
  PVoidProc _fontSemaphore;
}       GlobalsRec, *Globals;

private Globals globals;

#define stdLokData	globals->_stdLokData
#define emptyarray	globals->_emptyarray
#define stdencvec	globals->_stdencvec
#define maxOffsetVal	globals->_maxOffsetVal
#define info_devsw	globals->_info_devsw
#define info_offsetval	globals->_info_offsetval
#define info_fooFactor	globals->_info_fooFactor
#define fontSemaphore	globals->_fontSemaphore

#if (OS != os_mpw)
public boolean alwaysErode;
public Fixed erodeOffset, erodeFoo;
#endif

private boolean InternalBuildChar(), ChrMapBuildChar();

public procedure PFCdToPRCd(f, r) PFCd f; PRCd r; {
  fixtopflt(f->x, &r->x);
  fixtopflt(f->y, &r->y);
  }

public procedure PRCdToPFCd(r, f) PRCd r; PFCd f; {
  f->x = pflttofix(&r->x);
  f->y = pflttofix(&r->y);
  }

private procedure RMoveTo(c) PRCd c; {
  MoveTo(*c, &gs->path);
  }

private procedure RLineTo(c) PRCd c; {
  LineTo(*c, &gs->path);
  }

private procedure RCurveTo(c0, c1, c2, c3) PRCd c0, c1, c2, c3; {
  CurveTo(*c1, *c2, *c3, &gs->path);
  }

private procedure RClosePath() {
  ClosePath(&gs->path);
  }

private procedure FMoveTo(c) PFCd c; {
  RCd rc;
  PFCdToPRCd(c, &rc);
  MoveTo(rc, &gs->path);
  }

private procedure FLineTo(c) PFCd c; {
  RCd rc;
  PFCdToPRCd(c, &rc);
  LineTo(rc, &gs->path);
  }

private procedure FCurveTo(c0, c1, c2, c3) PFCd c0, c1, c2, c3; {
  RCd rc1, rc2, rc3;
  PFCdToPRCd(c1, &rc1);
  PFCdToPRCd(c2, &rc2);
  PFCdToPRCd(c3, &rc3);
  CurveTo(rc1, rc2, rc3, &gs->path);
  }

private procedure PushFixed(f) Fixed f; {
  real r;
  fixtopflt(f, &r);
  PSPushPReal(&r);
  }

private Fixed PopFixed() {
  real r;
  PSPopPReal(&r);
  return pflttofix(&r);
  }

private procedure PushPFCd(f) PFCd f; {
  PushFixed(f->x);
  PushFixed(f->y);
  }

private procedure PopPFCd(f) PFCd f; {
  f->y = PopFixed();
  f->x = PopFixed();
  }

private Fixed FixedValue(pobj) PObject pobj; {
  switch (pobj->type)
    {
    case realObj: return pflttofix(&pobj->val.rval); break;
    case intObj:  return FixInt(pobj->val.ival); break;
    default: TypeCheck();
    }
  return 0;	/* Make the compiler happy. */
  }

private procedure F2RMetrics(fm, rm) FMetrics *fm; RMetrics *rm; {
  PFCdToPRCd(&fm->w0, &rm->w0);
  PFCdToPRCd(&fm->w1, &rm->w1);
  PFCdToPRCd(&fm->c0, &rm->c0);
  PFCdToPRCd(&fm->c1, &rm->c1);
  PFCdToPRCd(&fm->v, &rm->v);
  }
  

public procedure RgstFontsSemaphoreProc (proc)
  PVoidProc proc /*( int level )*/; {
  fontSemaphore = proc;
}

#if PPS
public procedure SetFont(fdict)  DictObj fdict;
{
if (Equal(fdict, gs->fontDict)) return;
gs->fontDict = fdict;
gs->matrixID = MIDNULL;
}  /* end of SetFont */
#endif /*PPS*/

public boolean BuildChar(c,pcn) integer c; PObject pcn;
/* Returns true if an exception is raised during execution; no exception
   ever pops out of BuildChar */
{
Mtx mtx; DictObj fdict; IntObj ftype;  Object ob;
boolean cleanup = false; CmdObj BCcmd;
#if PPS
fdict = CrFntDict();
#else
fdict = gs->fontDict;
#endif /*PPS*/
DURING
  {
  GSave();
  NewPath();
  gs->matrix.tx = 0; gs->matrix.ty = 0;
#if PPS
  gs->inBuildChar = true;
  ss->cmo = CMNULL;
#else
  gs->strokeAdjust = false;
  gs->circleAdjust = false;
#endif /*PPS*/
  DictGetP(fdict, matrixname, &ob);
  DictGetP(fdict, fntypname, &ftype);
  PAryToMtx(&ob, &mtx);
  Cnct(&mtx);
  if (ftype.type != intObj) InvlFont();
  switch (ftype.val.ival)
    {
    case PSFILEtype:
    case PSVMtype:
      if (c < 0) InvlFont();
      PushP(&fdict);
      PushInteger(c);
      DictGetP(fdict, BCcmdName, &BCcmd);
#if !PPS
      if (fontSemaphore != NIL)
	(*fontSemaphore)(1);
#endif
      cleanup = psExecute(BCcmd);
#if !PPS 
      if (fontSemaphore != NIL)
	(*fontSemaphore)(-1);
#endif
      break;
#if PPS
    case ENCRPTFILEtype:
#endif
    case ENCRPTVMtype:
      cleanup = InternalBuildChar(fdict,c,pcn,ftype.val.ival);
      break;
    case CHRMAPtype:
#if !PPS
    case ECHRMAPtype:
      cleanup = ChrMapBuildChar(fdict,c,pcn,ftype.val.ival);
#else
      cleanup = ChrMapBuildChar(fdict,c,pcn);
#endif
      break;
    default: InvlFont();
    }
  GRstr();
  } /* end of DURING */
HANDLER
  {
  GRstr();
  SetAbort((integer)Exception.Code);
  cleanup = true;
  }
END_HANDLER;
return cleanup;
}  /* end of BuildChar */

private Fixed AFixedCar(pao)  PAryObj pao;
{
Object ob;
VMCarCdr(pao, &ob);
switch (ob.type)
  {
  case intObj: return FixInt(ob.val.ival);
  case realObj: return pflttofix(&ob.val.rval);
  default: TypeCheck(); /* NOTREACHED */
  }
return 0;	/* Make the compiler happy. */
}  /* end of AFixedCar */

private procedure ARealCarP(pao,p)  PAryObj pao; Preal p;
{Object ob;  VMCarCdr(pao, &ob);  PRealValue(ob, p);}

private integer FontEncode
(ccode,fd,pcn)
  integer ccode; DictObj fd; NameObj *pcn;
{
NameObj fname; register integer c, len; AryObj encvec;
AGetP(stdencvec, (cardinal)ccode, pcn); /* standard name of character */
DictGetP(fd, encname, &encvec);
len = encvec.length;
for (c=0; c < len; c++)
  {
  VMCarCdr(&encvec, &fname);
  if (pcn->val.nmval==fname.val.nmval) break;
  }
if (c==len) c = -1; /* cannot find it in the Encoding */
return c;
}  /* end of FontEncode */

#if !PPS || PPSkanji
private boolean GetMetrics2 (fd, pcn, fm)
  DictObj fd; Object *pcn; FMetrics *fm; {
  DictObj mtrcsdict;
  Object obj;
  if (Known (fd, metrics2name) && PccCount == 0) {
    DictGetP (fd, metrics2name, &mtrcsdict);
    if (Known (mtrcsdict, *pcn)) {
      DictGetP (mtrcsdict, *pcn, &obj);
      if ((obj.type != pkdaryObj) && (obj.type != arrayObj)) InvlFont ();
      if (obj.length != 4) InvlFont ();
      fm->w1.x = AFixedCar(&obj);
      fm->w1.y = AFixedCar(&obj);
      fm->v.x = AFixedCar(&obj);
      fm->v.y = AFixedCar(&obj);
      return true;
      }
    }
  return false;
  }
#endif /* !PPS || PPSkanji */

public procedure ModifyCachingParams (fd, pcn, m)
  DictObj fd; Object *pcn; RMetrics *m;
{
  Object obj; real t; boolean xx, xy;
  DictGetP (fd, cdevprocname, &obj);
  if ((obj.type != pkdaryObj) && (obj.type != arrayObj)) InvlFont ();
  if (xx = (m->c0.x > m->c1.x)) {
    t = m->c0.x; m->c0.x = m->c1.x; m->c1.x = t;
    }
  if (xy = (m->c0.y > m->c1.y)) {
    t = m->c0.y; m->c0.y = m->c1.y; m->c1.y = t;
    }
  PushPCd (&m->w0);
  PushPCd (&m->c0);
  PushPCd (&m->c1);
  PushPCd (&m->w1);
  PushPCd (&m->v);
  PushP (pcn);
  (void) psExecute (obj);
  PopPCd (&m->v);
  PopPCd (&m->w1);
  PopPCd (&m->c1);
  PopPCd (&m->c0);
  PopPCd (&m->w0);
  if (xx) {t = m->c0.x; m->c0.x = m->c1.x; m->c1.x = t;}
  if (xy) {t = m->c0.y; m->c0.y = m->c1.y; m->c1.y = t;}
}

private procedure GetFontBBox(fd,swval,bl,tr)
  DictObj fd; Fixed swval; PFCd bl, tr; {
  AryObj bbox;
  register Fixed swval2 = swval + swval;
  DictGetP(fd, bboxname, &bbox);
  bl->x = AFixedCar(&bbox);
  bl->y = AFixedCar(&bbox);
  tr->x = AFixedCar(&bbox);
  tr->y = AFixedCar(&bbox);
  bl->x -= swval2;
  bl->y -= swval2;
  tr->x += swval2;
  tr->y += swval2;
  }

#if CHECKOKBUILD

public boolean ROkToBuild(m) RMetrics *m; {
  /* args are same as for SetCchDevice (except delta result) */
  /* if char will be outside current visible area, return false */
  Cd devll, devur, delta;
  DevBBox bb;
  extern BBox GetDevClipDevBBox();
  register ShowState* ssr = ss;
  if (gs->isCharPath) /* charpath command */
    return true;
  else if (ssr->noShow) /* stringwidth command */
    return true;
  else { /* some kind of show command */
#if 0
    FontComputeBB(*ll, *ur, &devll, &devur);
    bb = GetDevClipBBox();
    GetCPDelta(ssr, &delta);
    devur.x += delta.x + fpTwo; devll.x += delta.x - fpTwo;
    devur.y += delta.y + fpTwo; devll.y += delta.y - fpTwo;
    /* throw in a margin of 2.0 around the charbbox just to be safe */
    if (bb->bl.x <= devur.x && bb->bl.y <= devur.y &&
        bb->tr.x >= devll.x && bb->tr.y >= devll.y)
#endif
      return true;
    }
#if (! DPS_CODE)
  SetCharWidth((gs->writingmode == 0) ? m->w0 : m->w1);
#else
  SetCharWidth(m->w0);
#endif /* ! DPS_CODE */
  return false;
  }

private boolean CheckIfOkToBuild(fd,swval,wid)
  DictObj fd; Fixed swval; FCd wid; {
  FCd fbl, ftr;
  extern boolean OkToBuild();
  GetFontBBox(fd,swval,&fbl,&ftr);
  return OkToBuild(&wid,&fbl,&ftr);
  }
  
#endif /* CHECKOKBUILD */

private procedure GetMetrics(fd,pcn,gotptr,csptr,cwptr)
  DictObj fd; Object *pcn; integer *gotptr; PFCd csptr, cwptr;
{
integer gotmetrics; FCd cs, cw;
DictObj mtrcsdict;
Object mtrcs;
gotmetrics = 0;
cw.x = cw.y = cs.x = cs.y = 0; /* width and side bearing */
/* get Metrics entry, if there */
if (Known(fd, metricsname) && PccCount == 0)
  {
  DictGetP(fd, metricsname, &mtrcsdict);
  if (Known(mtrcsdict, *pcn))
    {
    DictGetP(mtrcsdict, *pcn, &mtrcs);
    switch (mtrcs.type)
      {
      case realObj:
      case intObj:
        cw.x = FixedValue(&mtrcs);
        gotmetrics = 1;
        break;
      case pkdaryObj:
      case arrayObj:
        switch (mtrcs.length)
          {
          case 2:
            cs.x = AFixedCar(&mtrcs);
            cw.x = AFixedCar(&mtrcs); 
            gotmetrics = 2;
            break;
          case 4:
            cs.x = AFixedCar(&mtrcs);
            cs.y = AFixedCar(&mtrcs);
            cw.x = AFixedCar(&mtrcs);
            cw.y = AFixedCar(&mtrcs);
            gotmetrics = 4;
            break;
          default: InvlFont();
          }
        break;
      default: InvlFont();
      }
    }
  }
*gotptr = gotmetrics;
*csptr = cs;
*cwptr = cw;
}  /* end of GetMetrics */

private procedure FudgeBlueBands(privdict,bbnds,lenbbnds,tbnds,lentbnds,swval)
  DictObj privdict;  Fixed *bbnds, *tbnds;
  register integer lenbbnds, lentbnds; Fixed swval;
{
  integer t, b; Fixed rsw; boolean knwn;
  register integer i;
  Fixed dy;
  knwn = ForceKnown(privdict, fdgbndsnm);
  if (!knwn) return;
  rsw = RoundSW(swval);
  dy = (swval-rsw) >> 1;
  if (dy > 0) { /* rounded is smaller than swval */
    /* raise top of bot bands, lower bottom of top bands */
    b = 1; t = 0;
    }
  else { /* rounded is bigger than swval */
    /* lower bottom of bot bands, raise top of top bands */
    b = 0; t = 1;
    }
  for (i = b; i < lenbbnds; i=i+2) bbnds[i] += dy;
  for (i = t; i < lentbnds; i=i+2) tbnds[i] -= dy;
}  /* end of FudgeBlueBands */

private Fixed GetStemWidth(privdict) DictObj privdict; {
  AryObj aob;
  Object ob;
  Fixed stemwidth;
  if (!ForceKnown(privdict, ErNm)) return 0;
  ForceGetP(privdict, ErNm, &aob);
  if ((aob.type != arrayObj && aob.type != pkdaryObj) || aob.length <= 15)
    return 0;
  ForceAGetP(aob,15L,&ob);
  switch (ob.type) {
    case intObj: stemwidth = FixInt(ob.val.ival); break;
    case realObj: stemwidth = pflttofix(&ob.val.rval); break;
    default: stemwidth = 0; break;
    }
  /* sanity check */
  if (stemwidth < FixInt(20) || stemwidth > FixInt(300)) stemwidth = 0;
  return stemwidth;
  }

private procedure GetErosion(privdict,swval,optr,fptr)
  DictObj privdict; Fixed swval, *optr, *fptr;
{
Object eVal, ob, engineClass;
Fixed offsetval, fooFactor;
Fixed varcoeff, offsetcoeff;
real r, ec;
boolean cleanup;
DictGetP(rootInternalDict, engineclassnm, &engineClass);
PRealValue(engineClass, &ec);
if (ec == 10.0)
  varcoeff = offsetcoeff = FixedOne;
else {
  varcoeff = FixedOne + FixedHalf;
  offsetcoeff = 0;
  if (ForceKnown(privdict, ErNm))
    { /* let the font compute new erosion coeffs */
    Begin(rootSysDict);
    PushFixed(offsetcoeff);
    PushFixed(varcoeff);
    PushPReal(&ec);
    ForceGetP(privdict, ErNm, &ob);
    cleanup = psExecute(ob);
    End();
    if (cleanup) RAISE(GetAbort(), "");
    varcoeff = PopFixed();
    offsetcoeff = PopFixed();
    }
  }
fooFactor = 58928;       /* 58928 == Fixed 0.9 */
offsetval = 29491;       /* 29491 == Fixed 0.45 */
fooFactor = fixmul(fooFactor, fixmul(offsetval, varcoeff));
if (offsetval > maxOffsetVal) offsetval = maxOffsetVal;
offsetval = fixmul(offsetval, offsetcoeff);
*optr = offsetval; *fptr = fooFactor;
erosion = offsetval + offsetval;
if (alwaysErode) erosion = FixedOne;
erodeOffset = offsetval;
erodeFoo = fooFactor;
fixtopflt(erosion, &r);
LRealObj(eVal, r);
DictPut(rootInternalDict, eNm, eVal);
}  /* end of GetErosion */

#else /* ATM */ 

/* ***ATM*** this is the head section of fontbuild for ATM */

#define os_labs(x) ABS(x) 
#define ATM_CSLIMIT 400			/* len1000 over this mean use qreducer */

#if GLOBALCOLORING
#ifndef USE68KATM
internal boolean glcrPrepass, glcrOn;
#endif /* USE68KATM */
internal boolean8 glcrFailure /*, glcrRoundUp */;
private PGlbClr glbClrLst, endGlbClrLst, freeGlbClr;
private PGlbCntr glbCntrLst, endGlbCntrLst, freeGlbCntr;
private short int nGlbClrs, nGlbCntrs;
private PGrowableBuffer pGblClrBuf, pGblCntrBuf;
#endif /* GLOBALCOLORING */

public PMtx gsmatrix;
public PBuildCharProcs bprocs;
internal Fixed *stdvw, *stdhw;
internal IntX lenstdvw, lenstdhw; /* 680x0 BuildChar code treats these as CardX */
internal Fixed l0, l1, r0, r1;
internal PLokSeq lokSeqX, lokSeqY;
internal boolean8 lokSlopesInited;
public boolean8 isoutline; /* declared as public for flex.c */
internal boolean8 devsweven;
#ifndef USE68KATM
private boolean doFixupMap, lokFixMapOk, lokFixedMap;
#if DO_OFFSET
private boolean dooffsetlock;
private FCd lockoffset;
#endif /* DO_OFFSET */
#endif /* USE68KATM */
public IntX locktype, gsfactor;
public Fixed rndwidth, idealwidth;
#if GSMASK
internal Int16 gmscale;
#endif /*GSMASK*/
internal Fixed blueScale, blueShiftStart;
public Fixed erosion;
global Fixed flatEps;
public procedure (*moveto)();
public procedure (*lineto)();
public procedure (*curveto)();
public procedure (*closepath)();
public jmp_buf buildError;

#if PROTOTYPES
private boolean GetDSW(Fixed swval, Fixed *p);
#if FBDEBUG
#if GLOBALCOLORING
private Fixed tfmloc(Fixed loc,boolean yflg);
private procedure printmap(PLokSeq pls,boolean yflg);
private procedure PrntClr(PGlbClr g);
#endif /* GLOBALCOLORING */
private procedure PrintClrs(boolean yflg);
private procedure PrintCntrs(void);
private procedure PrintBlueValues(
  Fixed *botBands, integer lenBotBands,
  Fixed *topBands, integer lenTopBands);
private procedure PrintBlueLocs(
  FCd *botLocs, IntX lenBotBands, FCd *topLocs, IntX lenTopBands);
#endif  /* FBDEBUG */

public int setjmp(jmp_buf);
public procedure longjmp(jmp_buf, int);

public procedure IniCScan( PGrowableBuffer b1, PGrowableBuffer b2,
                           PGrowableBuffer b3, PGrowableBuffer b4);
public procedure ATMIniQReducer( PGrowableBuffer b1, PGrowableBuffer b2,
                                 PGrowableBuffer b3, PGrowableBuffer b4);
internal procedure InvlFont(void);
#if GLOBALCOLORING
internal procedure StartGlcrLock(void);
#endif /* GLOBALCOLORING */
#ifndef USE68KATM
private boolean InsertLock(PLokSeq pls, PLokPair lp);
#endif /* USE68KATM */
internal procedure SetXLock(
  Fixed f1, Fixed f2, boolean anchored, Fixed hw1, Fixed hw2);
internal procedure SetYLock(
  Fixed f1, Fixed f2, boolean anchored, Fixed hw1, Fixed hw2);
#ifndef USE68KATM
private procedure initslope(PLokSeq pls);
private procedure fixupmap(PLokSeq pls);
private procedure mapedges(PLokSeq pls);
private procedure InitSlopes(void);
private Fixed Map(PLokSeq pls, Fixed x);
#endif /* USE68KATM */
internal Fixed ErodeSW(Fixed swval, Fixed w, boolean erode);
internal Fixed Adjust(Fixed w, Fixed t);
internal Fixed CalcHW2(Fixed hw1, Fixed wd1, Fixed wd2, boolean yflg);
#ifndef USE68KATM
private PFixed DoBlend(Fixed *pp, Fixed *wv, IntX k, IntX r);
private Fixed PreXLock(Fixed xf, Fixed xn, Fixed *xl, Fixed *xa);
private Fixed PreYLock(Fixed yf, Fixed yn, Fixed *yd, Fixed *ya);
private procedure TfmLockPt1(PFCd pt, PFCd p);
#if DO_OFFSET
private procedure TfmLockPt2(PFCd pt, PFCd p);
#endif /* DO_OFFSET */
private procedure TfmLockPt3(PFCd pt, PFCd p);
private procedure CCBuild(PFCd p, PCCInfo pcc, Fixed cpx);
#endif /* USE68KATM */
internal procedure TriLock(
  PFCd p, Fixed sb, Fixed (*pre)(), procedure (*post)());
#ifndef USE68KATM
private procedure RMLock(PFCd p, Fixed sbx);
private procedure RYLock(PFCd p, Fixed sbx);
private procedure RVLock(PFCd p, Fixed sby);
private procedure RBLock(PFCd p, Fixed sby,
  Fixed *botBands, IntX lenBotBands, Fixed *topBands, IntX lenTopBands,
  FCd *botLocs, FCd *topLocs, Fixed bluefuzz);
#endif /* USE68KATM */
internal procedure BlueLock(Fixed yf, Fixed yn,
#if GLOBALCOLORING
  Fixed w, PGlbClr g,
#endif /* GLOBALCOLORING */
  Fixed *botBands, IntX lenBotBands, Fixed *topBands, IntX lenTopBands,
  FCd *botLocs, FCd *topLocs, Fixed bluefuzz);
private procedure GetBlueArrays(
  Int16 *pblues, IntX nblues, Int16 *pothers, IntX nothers,
  Fixed *bbnds, IntX *lenbbnds, Fixed *tbnds, IntX *lentbnds);
internal procedure GetBlueValues(
  PFontPrivDesc priv, Fixed *bbnds, IntX *lenbbnds,
  Fixed *tbnds, IntX *lentbnds);
internal procedure FamilyBlueLocs(
  PFontPrivDesc priv, FCd *botLocs, IntX lenBotBands,
  FCd *topLocs, IntX lenTopBands, boolean raisetops);
internal procedure BoostBotLocs(FCd *botLocs, IntX lenBotBands);
internal procedure CheckBlueScale(
  Fixed *botBands, IntX lenBotBands, Fixed *topBands, IntX lenTopBands);
private procedure AdjustBlues(FCd *botLocs, IntX lenBotBands,
  FCd *topLocs, IntX lenTopBands,
  Fixed *famBotBands, IntX lenFamBotBands,
  Fixed *famTopBands, IntX lenFamTopBands, boolean raisetops);
internal procedure SetupBlueLocs(Fixed *botBands, IntX lenBotBands,
  Fixed *topBands, IntX lenTopBands, FCd *botLocs, FCd *topLocs);
internal boolean UseStdWidth(Fixed *pw, Fixed *stdws, IntX numstd);
private IntX SetupStdWs(IntX numstd, Int16 *pstd, IntX numstms, Int16 *pstms,
  Fixed stemwidth, Fixed *pstdw, boolean yflg);
private IntX GetStdW(IntX len, Int16 *src, Fixed *stdw);
private procedure PutStdW(Fixed *pstdw, IntX len, boolean yflg, Fixed stdw);
internal procedure GetStandardWidths(PFontPrivDesc priv, Fixed stemwidth);
#if GLOBALCOLORING
internal procedure AdjustToStdWidths(void);
private procedure GetITfmX(Fixed *px, Fixed *p);
private procedure GetITfmY(Fixed *py, Fixed *p);
private procedure GetTfmX(Fixed *px, Fixed *p);
private procedure GetTfmY(Fixed *py, Fixed *p);
private PGlbClr NewGlbClr(void);
internal procedure EnterGlbClr(Fixed *pf, Fixed *pn, boolean yflg);
private PGlbClr FindGlbClr(Fixed *pf, Fixed *pn, boolean yflg);
private procedure FinGlbClrLocs(PGlbClr g, Fixed d);
internal procedure CalcGlbClrLocs(PGlbClr g);
internal procedure CalcAnchGlbClrLocs(PGlbClr g, Fixed w);
internal procedure DoLock(Fixed *pf, Fixed *pn, boolean yflg);
internal Fixed EnterTriXLock(Fixed xf, Fixed xn, Fixed *p0, Fixed *p1);
internal Fixed EnterTriYLock(Fixed yf, Fixed yn, Fixed *p0, Fixed *p1);
internal Fixed DoTriXLock(Fixed xf, Fixed xn, Fixed *p0, Fixed *p1);
internal Fixed DoTriYLock(Fixed yf, Fixed yn, Fixed *p0, Fixed *p1);
internal procedure GlbClrLine(FCd c0, FCd c1);
/* private PGlbClr NxtGClr(PGlbClr g); */
private PGlbCntr NewGlbCntr(void);
private procedure EnterGlbCntr(PGlbClr upper, PGlbClr lower);
private procedure BuildGlbCntrs(void);
private procedure GlbFixLocs(void);
internal procedure ProcessGlbClrs(
  Fixed *botBands, IntX lenBotBands, Fixed *topBands, IntX lenTopBands, 
  Fixed bluefuzz, PGrowableBuffer b3, FCd *botLocs, FCd *topLocs, Fixed ef);
public boolean GlobalColoring(
  PGlbClr glbClrLst, PGlbCntr glbCntrLst, PGrowableBuffer b3, Fixed ef,
  integer nCntr, integer nStems);
internal procedure IniGlbClrBuffs(PGrowableBuffer b1, PGrowableBuffer b2);
#endif /* GLOBALCOLORING */
#endif /* PROTOTYPES */

public procedure BCERROR(code) IntX code; {
  longjmp(buildError, code); }
internal procedure InvlFont() { BCERROR(BE_INVLFONT); }
public procedure UndefResult() { BCERROR(BE_UNDEF); }
public procedure CantHappen() { BCERROR(BE_CANTHAPPEN); }

#endif /* ATM */

/* ******* procs with different defs for ATM and PS ******* */

#if !ATM
private boolean GetDSW(swval, p) Fixed swval, *p; {
  FCd fcd; RCd rcd;  real r;
  if (!usefix) {
    fixtopflt(swval, &rcd.y);  rcd.x = fpZero;
    DTfmP(rcd, &rcd);
    r = os_sqrt(rcd.x * rcd.x + rcd.y * rcd.y);
    if (r > 32000.0) return false;
    *p = pflttofix(&r);
    }
  else {
    fcd.y = swval; fcd.x = 0;
    /* use y vector so get correct locking for outlined hstems */
    FntDTfmP(fcd, &fcd);
    *p = F_Dist(fcd.x, fcd.y);
    }
  return true;
  }
#else
private boolean GetDSW(swval, p) Fixed swval, *p; {
  FCd fcd;
  fcd.y = swval; fcd.x = 0;
  /* use y vector so get correct locking for outlined hstems */
  *p = ApproxDLen(&fcd);
  return true;
  }
#endif

#if !ATM
private Fixed ErodeSW(swval, w, erode) Fixed swval, w; boolean erode;
/* sets gs->lineWidth (normalized in dev space) */
  {
  Fixed rdsw, dswlength;
  gs->devhlw = 0;
  if (swval == 0) return (gs->lineWidth = 0);
  if (!GetDSW(swval, &dswlength)) {
    fixtopflt(swval, &gs->lineWidth);  
    return FixInt(16000);  /* just used for bitmap bounding box anyway */
    }
  rdsw = FRoundF(dswlength);
  if (erode) rdsw -= w;
  if (rdsw <= 0) return (gs->lineWidth = 0);
  swval = muldiv(swval, rdsw, dswlength, false);
  fixtopflt(swval, &gs->lineWidth);  
  return rdsw;
  }
#else /*ATM*/
internal Fixed ErodeSW(swval, w, erode) Fixed swval, w; boolean erode;
/* sets gs->lineWidth (normalized in dev space) */
  {
  Fixed rdsw, dswlength;
  if (swval == 0) return 0;
  if (!GetDSW(swval, &dswlength))
    return FixInt(16000);
  rdsw = FRoundF(dswlength);
  if (erode) rdsw -= w;
  if (rdsw <= 0) return 0;
  return rdsw;
  }
#endif /*ATM*/

#define NormalizeSW(swval) \
   ErodeSW(swval, FixedOne, (boolean)(erosion >= FixedHalf))

#if !ATM
public procedure PSErodeSW() {
  real w;
  PopPReal(&w);
  ErodeSW(pflttofix(&gs->lineWidth), pflttofix(&w), true); 
  }

private Fixed RoundSW(swval) Fixed swval; {
  Fixed rdsw, dswlength;
  if (swval == 0) return 0;
  if (!GetDSW(swval, &dswlength))
    return swval;
  rdsw = FRoundF(dswlength);
  if (rdsw == 0) rdsw = FixedOne;
  return muldiv(swval, rdsw, dswlength, false);
  }

#if PPS
#define KEYHASH 0x3f8927b5
#if STAGE==DEVELOP
private procedure PSStFKey()	/* setfontkey */
  {
  IntObj keyObj;
  LIntObj(keyObj, PopInteger() ^ KEYHASH);
  VMPutElem(root->param, rpFontKey, keyObj);
  }
#endif /* STAGE==DEVELOP */
#endif /*PPS*/
#endif /*!ATM*/


#if !ATM
private PCCInfo CCBuild(pcn, p,fd,sbx,cpx,cpy,swval,fm,gotmetrics)
  PObject pcn; DictObj fd; register PFCd p; FMetrics *fm;
  Fixed sbx, cpx, cpy, swval; integer gotmetrics; {
  register PCCInfo pcc;
  RMetrics rm;
  RCd rtmp, delta;
  Fixed dx, dy, adx, ady, cdx, cdy, asbx, ccsb, truecpx, dccsb;
  integer acode, ccode;
  Object acn, ccn;
  asbx = p[0].x;
  dx = p[0].y; dy = p[1].x;
  SysDictGetP(stdencname, &stdencvec);
  ccode = FTrunc(p[1].y);
  ccode = FontEncode(ccode,fd,&ccn);
  acode = FTrunc(p[2].x);
  acode = FontEncode(acode,fd,&acn);

  /* compute composite char sidebearing WITHOUT Metrics dict - ccsb */
  ccsb = MIN(sbx,sbx + dx);
  /* then find out the amount we should move the whole comp char - dccsb */
  /* cpx has composite char sidebearing */
  truecpx = (gotmetrics) ? cpx : ccsb;
  dccsb = truecpx - ccsb;
  /* adx, cdx - offsets for the accent and base */
  cdx = dccsb;
  adx = dccsb + sbx - asbx + dx;

  ccsb = MIN(0,dy);
  truecpx = (gotmetrics) ? cpy : ccsb;
  dccsb = truecpx - ccsb;
  cdy = dccsb;
  ady = dccsb + dy;

  /* compute bounding box based on FontBBox (this is conservative) */
  GetFontBBox(fd,swval,&fm->c0,&fm->c1);
#if CHECKOKBUILD
  if (!OkToBuild(wid, &fbl, &ftr))
    return NULL;
#endif /* CHECKOKBUILD */
  F2RMetrics(fm, &rm);
#if !PPS || PPSkanji
  if (hasCDevProc && PccCount == 0)
    ModifyCachingParams (fd,pcn,&rm);
#endif /* !PPS || PPSkanji */
#if !PPS
  if (!SetCchDevice(&rm, &delta))
#else
  if (!SetCchDevice(rm.w0, rm.c0, rm.c1, &delta))
#endif
    return NULL; 
  pcc = (PCCInfo)NEW(1,sizeof(CCInfo));
  fixtopflt(adx, &pcc->adx);
  fixtopflt(cdx, &pcc->cdx);
  fixtopflt(ady, &pcc->ady);
  fixtopflt(cdy, &pcc->cdy);
#if !PPS || PPSkanji
  if (hasCDevProc && PccCount == 0) {
    PFCdToPRCd(&fm->c0, &rtmp);
    VecSub(rtmp, rm.c0, &rtmp);
    pcc->adx -= rtmp.x;
    pcc->cdx -= rtmp.x;
    pcc->ady -= rtmp.y;
    pcc->cdy -= rtmp.y;
    }
#endif /* !PPS || PPSkanji */
  pcc->acode = acode;
  pcc->ccode = ccode;
  pcc->acn = acn;
  pcc->ccn = ccn;
  PccCount++;
  return (pcc); }
#else /* ATM */  
#ifndef USE68KATM
private procedure CCBuild(p, pcc, cpx)
  register PFCd p; register PCCInfo pcc; Fixed cpx;
{ /* p stack contains: asb dx dy ccode acode */
  FCd delta;
  Fixed asb, dx;
  asb = p[0].x;
  dx = p[0].y;
  delta.y = p[1].x;
  pcc->ccode = FTrunc(p[1].y);
  pcc->acode = FTrunc(p[2].x);
  delta.x = cpx + dx - asb;
  FntDTfmP(delta, &delta);
  pcc->dx = FRound(delta.x);
  pcc->dy = FRound(delta.y);
}
#endif /* USE68KATM */
#endif /* ATM */

#if FBDEBUG
private procedure PrintBlueValues(botBands,lenBotBands,topBands,lenTopBands)
  Fixed *botBands, *topBands; integer lenBotBands, lenTopBands; {
  IntX i;
  for (i = 0; i < lenBotBands; i += 2) {
    printf("bottom band %g %g\n", FD(botBands[i]), FD(botBands[i+1]));
    }
  for (i = 0; i < lenTopBands; i += 2) {
    printf("top band %g %g\n", FD(topBands[i]), FD(topBands[i+1]));
    }
  }
#endif

#if !ATM
private procedure GetBlueArrays(bnm,obnm,
  privdict,bbnds,lenbbnds,tbnds,lentbnds)
  Object bnm, obnm; DictObj privdict; register Fixed *bbnds, *tbnds;
  IntX *lenbbnds, *lentbnds; {
  register IntX i, nblues; AryObj bluearray;
  if (!ForceKnown(privdict, bnm)) {
    *lenbbnds = 0; *lentbnds = 0; return; }
  ForceGetP(privdict, bnm, &bluearray);
  if ((nblues = bluearray.length) > MAXBLUELEN) InvlFont();
  if (nblues == 0) {
    *lenbbnds = 0; *lentbnds = 0; return; }
  if ((nblues & 1) != 0) InvlFont();
  for (i = 0; i < 2; i++) bbnds[i] = AFixedCar(&bluearray);
  for (i = 2; i < nblues; i++) tbnds[i-2] = AFixedCar(&bluearray);
  *lenbbnds = 2; *lentbnds = nblues-2;
  if (!ForceKnown(privdict, obnm)) return;
  ForceGetP(privdict, obnm, &bluearray);
  nblues = bluearray.length;
  if (nblues == 0) return;
  if (nblues > MAXOTHERBLUELEN || nblues < 2 || (nblues & 1) != 0)
    InvlFont();
  for (i = 0; i < nblues; i++) bbnds[i+2] = AFixedCar(&bluearray);
  *lenbbnds = 2 + nblues;
  }
#else /* ATM */
private procedure GetBlueArrays(
  pblues,nblues,pothers,nothers,bbnds,lenbbnds,tbnds,lentbnds)
  Int16 *pblues, *pothers; IntX nblues, nothers;
  register Fixed *bbnds, *tbnds; IntX *lenbbnds, *lentbnds; {
  register IntX i;
  if (nblues > MAXBLUELEN) { InvlFont(); return; }
  if (nblues == 0) {
    *lenbbnds = 0; *lentbnds = 0; return; }
  if ((nblues & 1) != 0) { InvlFont(); return; }
  for (i = 0; i < 2; i++) bbnds[i] = FixInt(pblues[i]);
  for (i = 2; i < nblues; i++) tbnds[i-2] = FixInt(pblues[i]);
  *lenbbnds = 2; *lentbnds = nblues-2;
  if (nothers == 0) return;
  if (nothers > MAXOTHERBLUELEN || (nothers & 1) != 0 || nothers < 2) {
    InvlFont(); return; }
  for (i = 0; i < nothers; i++) bbnds[i+2] = FixInt(pothers[i]);
  *lenbbnds = 2 + nothers;
  }
#endif /* ATM */

#if !ATM
private procedure GetBlueValues(privdict,bbnds,lenbbnds,tbnds,lentbnds)
  DictObj privdict; register Fixed *bbnds, *tbnds;
  IntX *lenbbnds, *lentbnds; {
  GetBlueArrays(bluename, otherBluesName,
                privdict,bbnds,lenbbnds,tbnds,lentbnds);
#if FBDEBUG
  PrintBlueValues(bbnds, *lenbbnds, tbnds, *lentbnds);
#endif
  }
#else /* ATM */
internal procedure GetBlueValues(priv,bbnds,lenbbnds,tbnds,lentbnds)
  PFontPrivDesc priv; register Fixed *bbnds, *tbnds;
  IntX *lenbbnds, *lentbnds; {
  GetBlueArrays(priv->blues, (IntX)(priv->numBlues),
    priv->otherBlues, (IntX)(priv->numOtherBlues),
    bbnds, lenbbnds, tbnds, lentbnds);
#if FBDEBUG
  PrintBlueValues(bbnds, *lenbbnds, tbnds, *lentbnds);
#endif
  }
#endif /* ATM */

#if !ATM
private IntX GetStdW(privdict,nm,stdw)
  DictObj privdict; Object nm; register Fixed *stdw; {
  register IntX len, i;
  AryObj widtharray;
  if (!ForceKnown(privdict, nm)) return 0;
  ForceGetP(privdict, nm, &widtharray);
  len = widtharray.length;
  if (len <= 0) return 0;
  if (len > MAXSTMS) { InvlFont(); return 0; }
  for (i = 0; i < len; i++) 
    *stdw++ = AFixedCar(&widtharray);
  return len;
  }
#else /* ATM */
private IntX GetStdW(len,src,stdw)
  register IntX len; register Int16 *src; register Fixed *stdw; {
  register IntX i;
  if (len <= 0) return 0;
  if (len > MAXSTMS) { InvlFont(); return 0; }
  for (i = 0; i < len; i++) 
    *stdw++ = FixInt(*src++);
  return len;
  }
#endif /* ATM */

private procedure PutStdW(pstdw,len,yflg,stdw)
  register Fixed *pstdw; register IntX len; boolean yflg; Fixed stdw; {
  register Fixed u=0, r, center, upper, lower;
  IntX cnt = len;
  Fixed *initstdw = pstdw;
  FCd c;
  while (--len > 0) { /* make sure widths are in increasing order */
    r = *pstdw++;
    if (r > *pstdw) InvlFont();
    }
  len = cnt; pstdw = initstdw;
  while (len-- > 0) {
    if (yflg) { c.x = 0; c.y = *pstdw; }
    else { c.x = *pstdw; c.y = 0; }
    if (locktype == 0)
      r = ApproxDLen(&c);
    else {
      FntDTfmP(c, &c); /* width in device space */
      r = (yflg == (locktype < 0))? c.x : c.y;
      }
    *pstdw++ = os_labs(r);
    }
  /* now adjust the widths to combine close ones */
  /* depends on stdw's being sorted by increasing width */
  /* widths less than stdw move up, those greater than stdw move down */
  pstdw = initstdw; len = cnt;
  while (len > 0) { /* skip over widths less than FixedOne */
    if (*pstdw >= FixedOne) break;
    pstdw++; len--;
    }
  while (len-- > 0) { /* for widths less than stdw */
    IntX i;
    r = *pstdw++; /* consider pulling r up */
    if (r >= stdw) { len++; pstdw--; break; }
    center = FTruncF(r) + FixedHalf;
    lower = center - wbnd;
    if (r < lower || r >= center) continue;
    /* find the smallest width that is >= center */
    for (i = 0; i < len; i++) 
      if ((u = pstdw[i]) >= center) break;
    if (i == len || u > stdw) { len++; pstdw--; break; }
    upper = center + FixedHalf;
    if (u >= upper) continue;
    if (fixmul(r,upper) >= fixmul(u,center)) { /* ensure monotonicity */
      initstdw = pstdw - 1;
      while (*initstdw < u)
        *initstdw++ = center; /* change to larger width */
      }
    }
  while (len-- > 0) { /* for widths >= stdw */
    r = *pstdw++;
    center = FTruncF(r) + FixedHalf;
    if (r >= center) continue;
    /* find the largest r that is < center */
    while (len > 0 && *pstdw < center) { len--; r = *pstdw++; }
    lower = center - FixedHalf;
    if (r <= lower) continue;
    upper = center + wbnd;
    while (len > 0 && (u = *pstdw) <= upper) {
      if (u <= stdw) break;
      if (fixmul(u,lower) >= fixmul(r,center)) break; /* ensure monotonicity */
      *pstdw++ = center-1;  /* change to smaller width */
      r = u; len--;
      }
    }
  }

#if !ATM
private IntX SetupStdWs(
  privdict, nmstd, nmstms, stemwidth, pstdw, yflg)
  DictObj privdict; Object nmstd, nmstms; 
  Fixed stemwidth; Fixed *pstdw; boolean yflg; {
  IntX len, numstd, numstms;
  Fixed stdw;
  numstd = GetStdW(privdict, nmstd, pstdw);
  if (numstd != 1) stdw = 0;
  else {
    /* if the standard width is less than 1.5 pixels then use it alone */
    PutStdW(pstdw, (IntX)1, yflg, 0L);
    stdw = pstdw[0]; /* this is the dominant standard */
    if (stdw < (FixedOne+FixedHalf))
      return 1;
    }
  numstms = GetStdW(privdict, nmstms, pstdw);
  if (numstms > 0)
    len = numstms;
  else if (numstd > 0) {
    len = numstd;
    if (len == 1) /* already did PutStdW above */
      return len;
    }
  else if (stemwidth > 0) {
    len = 1; pstdw[0] = stemwidth; }
  else
    len = 0;
  if (len > 0)
    PutStdW(pstdw, len, yflg, stdw);
  return len;
  }
#else /* ATM */
private IntX SetupStdWs(
  numstd, pstd, numstms, pstms, stemwidth, pstdw, yflg)
  IntX numstd, numstms; Int16 *pstd, *pstms;
  Fixed stemwidth; Fixed *pstdw; boolean yflg; {
  IntX len;
  Fixed stdw;
  if (numstd != 1) stdw = 0;
  else {
    /* if the standard width is less than 1.5 pixels then use it alone */
    *pstdw = FixInt(*pstd);
    PutStdW(pstdw, (IntX)1, yflg, 0L);
    stdw = pstdw[0]; /* this is the dominant standard */
    if (stdw < (FixedOne+FixedHalf))
      return 1;
    }
  if (numstms > 0)
    len = GetStdW(numstms, pstms, pstdw);
  else if (numstd > 0)
    len = GetStdW(numstd, pstd, pstdw);
  else if (stemwidth > 0) {
    len = 1; pstdw[0] = stemwidth; }
  else
    len = 0;
  if (len > 0)
    PutStdW(pstdw, len, yflg, stdw);
  return len;
  }
#endif /* ATM */

#if !ATM
private procedure GetStandardWidths(privdict, stemwidth)
  DictObj privdict; Fixed stemwidth; {
  if (!usefix)
  	lenstdhw = lenstdvw = 0;
  else {
    lenstdhw = SetupStdWs(privdict,
      stdhwNm, stemsnaphNm, stemwidth, &stdhw[0], true);
    lenstdvw = SetupStdWs(privdict,
      stdvwNm, stemsnapvNm, stemwidth, &stdvw[0], false);
  }
}
#else /* ATM */
internal procedure GetStandardWidths(priv, stemwidth)
  PFontPrivDesc priv; Fixed stemwidth; {
  lenstdhw = SetupStdWs(
    priv->numStdHW, priv->stdHW, priv->numHStmWs, priv->hStmWs,
    stemwidth, &stdhw[0], true);
  lenstdvw = SetupStdWs(
    priv->numStdVW, priv->stdVW, priv->numVStmWs, priv->vStmWs,
    stemwidth, &stdvw[0], false);
  }
#endif /* ATM */

#if !ATM
private IntX GetWeightVector(fd, wv) DictObj fd; PFixed wv; {
  AryObj wvarray;
  IntX wvlen, i;
  if (!Known(fd, wvname))
    return 0;
  DictGetP(fd, wvname, &wvarray);
  wvlen = wvarray.length;
  if (wvlen > MAXWEIGHTS)
    InvlFont();
  for (i = 0; i < wvlen; i++)
    wv[i] = AFixedCar(&wvarray);
  return wvlen;
  }
#endif /* ATM */

/* ***MID*** this is the shared section of fontbuild */

/* encoded font outline command numbers */

#define CBcmd	0
#define RBcmd	1
#define CYcmd	2
#define RYcmd	3
#define VMTcmd	4
#define RDTcmd	5
#define HDTcmd	6
#define VDTcmd	7
#define RCTcmd	8
#define CPcmd	9
#define DOSUBcmd 10
#define RETcmd	11
#define ESCcmd	12
#define SBXcmd	13
#define EDcmd	14
#define MTcmd	15
#define DTcmd	16
#define CTcmd 17
#define MINcmd	18
#define STcmd	19
#define NPcmd	20
#define RMTcmd	21
#define HMTcmd	22
#define SLCcmd	23
#define MULcmd	24
#define STKWDTHcmd 25
#define BSLNcmd	26
#define CPHGHTcmd	27
#define BOVRcmd	28
#define XHGHTcmd 29
#define VHCTcmd 30
#define HVCTcmd 31
#define BIGNUM	255

#define FLesc 0
#define RMesc 1
#define RVesc 2
#define FIesc 3
#define ARCesc 4
#define SLWesc 5
#define CCesc 6
#define SBesc 7
#define SSPCesc 8
#define ESPCesc 9
#define ADDesc 10
#define SUBesc 11
#define DIVesc 12
#define MAXesc 13
#define NEGesc 14
#define IFGTADDesc 15
#define DOesc 16
#define POPesc 17
#define DSCNDesc 18
#define ASCNDesc 19
#define OVRSHTesc 20
#define SLJesc 21
#define XOVResc 22
#define CAPOVResc 23
#define AOVResc 24
#define HLFSWesc 25
#define RNDSWesc 26
#define ARCNesc 27
#define EXCHesc 28
#define INDXesc 29
#define CRBesc 30
#define CRYesc 31
#define PUSHCPesc 32
#define POPCPesc 33

#if !ATM
public procedure StartLock() {
  register PLokData pld;
#if GLOBALCOLORING
  register PGlbClr g;
  if (glcrOn && glcrPrepass)
    for (g = glbClrLst; g; g = g->next) g->active = false;
#endif /* GLOBALCOLORING */
  if ((pld = pLokData) == NIL) {
    if ((pld = stdLokData)->inUse) {
      pld = (PLokData)NEW(1, sizeof(LokData));
      pld->temporary = true;
      }
    pld->inUse = true;
    pLokData = pld;
    }
  pld->X.count = 0;
  pld->Y.count = 0;
  pld->slopesInited = false;
  pld->fixmapok = false;
  }

private procedure FinishLock() {
  if (pLokData != NIL)
    {
    if (pLokData->temporary) FREE((char *) pLokData);
    else pLokData->inUse = false;
    pLokData = NIL;
    }
  }
#else /* ATM */
#if GLOBALCOLORING
internal procedure StartGlcrLock() {
  register PGlbClr g;
  for (g = glbClrLst; g; g = g->next) g->active = false;
  }
#endif /* GLOBALCOLORING */
#endif /* ATM */

#ifndef USE68KATM
private boolean InsertLock(pls, lp)
  register PLokSeq pls; PLokPair lp;
{
register PLokPair p, pn;
register boolean firstPoint = true;
#if !ATM
Assert(pLokData != NIL);
#endif /* ATM */
if (pls->count >= Nlokpairs) InvlFont();
p = &pls->pair[0];
pn = p + pls->count;
#if !ATM
pLokData->slopesInited = false; /* reset false whenever add a new point */
#else /* ATM */
lokSlopesInited = false; /* reset false whenever add a new point */
#endif /* ATM */
if (pls->count == 0) {
  *p = *lp;
  return true;
  }
while (true)
  {
  if (lp->c1 == p->c1)
    return false;
  if (lp->c1 < p->c1)
    {  /* This is the spot.  Either modify a point or insert this one. */
    /* Check slope to next point */
    if (lp->c2 > p->c2)
      /* "Negative slope implied by lock pair -- ignored." */
      return false;
    if (lp->c2 == p->c2) /* zero slope to next point */
      goto samec2;
    if (!firstPoint)
      { /* Check slope to previous point */
      p--;
      if (lp->c2 < p->c2)
        return false;
      if (lp->c2 == p->c2)
        goto samec2;
      p++;
      }
    while (true) {*pn = *(pn-1);  pn--;  if (pn == p) break;} 
    *p = *lp;
    return true;
    }
  p++;
  if (p == pn) {p--; break;}
  firstPoint = false;
  }
if (lp->c2 < p->c2)
  /* "Negative slope implied by lock pair -- ignored." */
  return false;
if (lp->c2 == p->c2)
  goto samec2;
*pn = *lp;
return true;
samec2:
if (p->slope != 0 && lp->slope == 0) {
  /* use p since it is anchored and lp is not */
  return false;
  }
if (p->slope == 0 && lp->slope != 0) {
  /* use lp since it is anchored and p is not */
  *p = *lp;
  return false;
  }
/* use one with greater c1 */
if (lp->c1 > p->c1) *p = *lp;
return false;
}  /* end of InsertLock */
#endif /* USE68KATM */

#ifndef USE68KATM
internal procedure SetXLock(f1, f2, anchored, hw1, hw2)
  Fixed f1, f2, hw1, hw2; boolean anchored;
  {
  LokPair lp;
  lp.c2 = f2; lp.c1 = f1; lp.slope = anchored ? FixedOne : 0;
  lp.hw1 = hw1; lp.hw2 = hw2;
#if !ATM
  if (InsertLock(&pLokData->X, &lp)) pLokData->X.count++;
#else /* ATM */
  if (InsertLock(lokSeqX, &lp)) lokSeqX->count++;
#endif /* ATM */
  }  /* end of SetXLock */
#endif /* USE68KATM */

#ifndef USE68KATM
internal procedure SetYLock(f1, f2, anchored, hw1, hw2)
  Fixed f1, f2, hw1, hw2; boolean anchored;
  {
  LokPair lp;
  lp.c2 = f2; lp.c1 = f1; lp.slope = anchored ? FixedOne : 0;
  lp.hw1 = hw1; lp.hw2 = hw2;
#if !ATM
  if (InsertLock(&pLokData->Y, &lp)) pLokData->Y.count++;
#else /* ATM */
  if (InsertLock(lokSeqY, &lp)) lokSeqY->count++;
#endif /* ATM */
  }  /* end of SetYLock */
#endif /* USE68KATM */

#ifndef USE68KATM
private procedure initslope(pls)
  register PLokSeq pls;
{
register PLokPair p, pn, pnext;
register Fixed delta;
p = &pls->pair[0];
pn = p + pls->count - 1;
while (p < pn)
  {
  pnext = p + 1;
  delta = pnext->c1 - p->c1;
  if (delta <= 0) p->slope = 0;
  else p->slope = fixdiv(pnext->c2 - p->c2, delta);
  p = pnext;
  }
}  /* end of initslope */
#endif /* USE68KATM */

#ifndef USE68KATM
private procedure fixupmap(pls) PLokSeq pls;
{
Fixed unitdist = pls->unitDist;
register PLokPair p = &pls->pair[0];
register IntX n = pls->count - 1;
register Fixed fixdist;
register PLokPair pn, pnext;
PLokPair p0 = p;
pn = p + n;
fixdist = unitdist + (unitdist >> 1);
while (p < pn)
  {
  pnext = p + 1;
  if (pnext->c2 - p->c2 < fixdist) { /* collision */
    if (pnext->slope == 0 &&
        (pnext == pn || pnext->c2 + unitdist + fixdist < pnext[1].c2)) {
#if !ATM
      pnext->c2 += unitdist; pLokData->fixedmap = true; goto nxt; }
#else
      pnext->c2 += unitdist; lokFixedMap = true; goto nxt; }
#endif /* ATM */
    else if (p->slope == 0 &&
             (p == p0 || p->c2 - unitdist - fixdist > p[-1].c2)) {
#if !ATM
      p->c2 -= unitdist; pLokData->fixedmap= true; goto nxt; }
#else
      p->c2 -= unitdist; lokFixedMap = true; goto nxt; }
#endif /* ATM */
    }
  nxt:
  p = pnext;
  }
}
#endif /* USE68KATM */

#ifndef USE68KATM
internal procedure mapedges(pls) register PLokSeq pls;
{
register PLokPair p = &pls->pair[0];
IntX n = pls->count;
register PLokPair pnext;
PLokPair pn, p0 = p; /* first entry */
register IntX cnt;
register Fixed hw1, hw2;
Fixed mid;
IntX cnt0;
pn = p + n - 1; /* last entry */
cnt = 0;
while (p <= pn) {
  if (p->hw1 != 0) cnt++;
  p++;
  }
/* cnt is the number of entries to be split */
p = pn;
cnt0 = cnt;
if (n + cnt > Nlokpairs)
  return;
#if (OS == os_msdos || OS == os_os2)
#pragma loop_opt (off)
#endif
while (p >= p0 && cnt > 0) {
  hw1 = p->hw1;
  if (hw1 != 0) { /* split it */
    pnext = p + cnt; *pnext = *p;
    hw2 = p->hw2;
    pnext->c1 += hw1; pnext->c2 += hw2;
    pnext--; *pnext = *p;
    pnext->c1 -= hw1; pnext->c2 -= hw2;
    cnt--;
    }
  else { /* just copy it */
    pnext = p + cnt; *pnext = *p;
    }
  p--;
  }
#if (OS == os_msdos || OS == os_os2)
#pragma loop_opt ()
#endif
p = p0; pnext = p + 1; pn += cnt0;
while (p < pn) {
  if (p->c1 > pnext->c1) {
    mid = (p->c1 >> 1) + (pnext->c1 >> 1); /* move both to midpoint */
    p->c2 += mid - p->c1; p->c1 = mid;
    pnext->c2 += mid - pnext->c1; pnext->c1 = mid;
    }
  p++; pnext++;
  }
pls->count += cnt0;
}
#endif /* USE68KATM */

#if FBDEBUG
private Fixed tfmloc(loc,yflg) Fixed loc; boolean yflg; {
  FCd temp;
  if (yflg) { temp.x = 0; temp.y = loc; }
  else { temp.x = loc; temp.y = 0; }
  FntTfmP(temp, &temp);
  if (yflg == (locktype < 0)) return temp.x;
  return temp.y;
  }

private procedure printmap(pls,yflg) register PLokSeq pls; boolean yflg;
{
register PLokPair p, pn;
p = &pls->pair[0];
pn = p + pls->count;
while (p < pn) {
  printf("c1 %g c2 %g loc %g hw1 %g hw2 %g slope %g\n",
    FD(p->c1), FD(p->c2), FD(tfmloc(p->c2, yflg)),
    FD(p->hw1), FD(p->hw2), FD(p->slope));
  p++;
  }
}
#endif

#if !ATM
private procedure InitSlopes()
{
if (doFixupMap && pLokData->fixmapok) {
  fixupmap(&pLokData->X);
  fixupmap(&pLokData->Y);
  }
mapedges(&pLokData->X);
mapedges(&pLokData->Y);
initslope(&pLokData->X);
initslope(&pLokData->Y);
pLokData->slopesInited = true;
#if FBDEBUG
printf("X map\n");
printmap(&pLokData->X,false);
printf("Y map\n");
printmap(&pLokData->Y,true);
#endif
}  /* end of InitSlopes */
#else /* ATM */
#ifndef USE68KATM
private procedure InitSlopes()
{
if (doFixupMap && lokFixMapOk) {
  fixupmap(lokSeqX);
  fixupmap(lokSeqY);
  }
mapedges(lokSeqX);
mapedges(lokSeqY);
initslope(lokSeqX);
initslope(lokSeqY);
lokSlopesInited = true;
}  /* end of InitSlopes */
#endif /* USE68KATM */
#endif /* ATM */

#if !ATM
private Fixed Map(pls, x)
  register PLokSeq pls; register Fixed x;
{
register PLokPair p, plast;
register IntX n;
n = pls->count;
if (n == 0) return x;
p = &pls->pair[0];
if ((n == 1) || (x <= p->c1))
  return (x + (p->c2 - p->c1));
plast = p + (n - 1);
if (x >= plast->c1)
  return (x + (plast->c2 - plast->c1));
/*i=0; until ((x >= p[i].c1) && (x < p[i+1].c1)) {i++;}*/
while (x >= (p+1)->c1) {p++;}
x -= p->c1;
if ((x & 0xF0007FFF) == 0)
  return (((x >> 15) * p->slope) >> 1) + p->c2;
return fixmul(p->slope, x) + p->c2;
}  /* end of Map */
#else /* ATM */
#ifndef USE68KATM
private Fixed Map(pls, x)
  register PLokSeq pls; register Fixed x;
{
register PLokPair p, plast;
register IntX n;
n = pls->count;
if (n == 0) return x;
p = &pls->pair[0];
if ((n == 1) || (x <= p->c1))
  return (x - p->c1 + p->c2);
plast = p + (n - 1);
if (x >= plast->c1)
  return (x - plast->c1 + plast->c2);
/*i=0; until ((x >= p[i].c1) && (x <= p[i+1].c1)) {i++;}*/
while (x >= (p+1)->c1) {p++;}
x -= p->c1;
if ((x & 0xF0007FFF) == 0)
  return (((x >> 15) * p->slope) >> 1) + p->c2;
return fixmul(p->slope, x) + p->c2;
}  /* end of Map */
#endif /* USE68KATM */
#endif /* ATM */

#ifndef USE68KATM
internal Fixed Adjust(w, t)  Fixed w, t;
  {
  IntX wi;
  boolean even;
  wi = FRound(w);
  if (wi == 0) {
    if (isoutline) wi = 1;
    else goto midPt;
    }
  even = (boolean)((wi & 01) == 0);
  if ((isoutline && devsweven) || (!isoutline && (FTrunc(w - erosion) >= wi)))
      even = even ? false : true;
  if (even) return FRoundF(t);
  midPt:
  t = FTruncF(t);
  return t + FixedHalf;
  }  /* end of Adjust */
#endif /* USE68KATM */

#ifndef USE68KATM
internal boolean UseStdWidth(pw, stdws, numstd)
  /* pw points to the width to be adjusted (in place). width >= 0. */
  /* stdws points to an array of standard widths in device space.
     sorted with smallest width first */
  /* numstd is the number of standard widths in the array */
  /* return true if the width has matched a standard width */
  register Fixed *pw, *stdws; IntX numstd; {
  Fixed center, upper, lower, mn, mx;
  register Fixed dw = 0, nw, w;
  IntX i;
  if (numstd <= 0) return false;
  w = *pw;
  for (i = 0; i < numstd; i++) { /* loop to find closest standard width */
    nw = *stdws++;
    if (nw == w) /* return true to indicate a match */
      return true;
    if (nw < w) { dw = nw; continue; }
    if (i == 0 || nw-w < w-dw) dw = nw;
    break;
    }
  /* dw is now the closest standard width to w */
  if (w > dw) { mn = dw; mx = w; }
  else { mn = w; mx = dw; }
  center = FTruncF(mx) + FixedHalf;
  if (mn >= center || mx < center)
    return false;
  upper = center + wbnd;
  lower = center - ((dw >= center || center > FixInt(2)) ? wbnd : (FixInt(3) >> 2));
  if (mn < lower || mx > upper) return false;
  if (w > dw) { /* consider adjusting w down to dw */
    if (fixmul(lower, w) >= fixmul(dw, center)) return false;
      /* must not adjust because would fail to be monotonic */
      /* as decrease scale w must cross center before dw crosses lower */
      /* or else w will fail to be adjusted as get smaller */
    }
  else { /* consider adjusting w up to dw */
    if (fixmul(upper, w) < fixmul(dw, center)) return false;
      /* must not adjust because would fail to be monotonic */
      /* as increase scale w must cross center before dw crosses upper */
      /* or else w will fail to be adjusted as get larger */
    }
  *pw = dw; return true;
  }
#endif /* USE68KATM */

#if GLOBALCOLORING
internal procedure AdjustToStdWidths() {
  register PGlbClr g;
  if (lenstdhw == 0 && lenstdvw == 0) return;
  for (g = glbClrLst; g; g = g->next) {
    if ((g->yflg && lenstdhw > 0 && UseStdWidth(&g->w, stdhw, lenstdhw)) ||
        (!g->yflg && lenstdvw > 0 && UseStdWidth(&g->w, stdvw, lenstdvw)))
      g->stdwidth = true;
    }
  } 
#endif /* GLOBALCOLORING */

#ifndef USE68KATM
internal Fixed CalcHW2(hw1, wd1, wd2, yflg)
  Fixed hw1, wd1, wd2; boolean yflg; {
  /* hw1 is unmodified stem half width in character space */
  /* wd1 is unmodified stem width in device space */
  /* wd2 is desired stem width in device space */
  /* result is desired stem half width in character space */
  Fixed hw2, rw;
  FCd temp;
  if (hw1 < 0) hw1 = -hw1;
  if (wd1 < 0) wd1 = -wd1;
  if (wd2 < 0) wd2 = -wd2;
#if FBDEBUG
  printf("CalcHW2 hw1 %g wd1 %g wd2 %g yflg %d ",
    FD(hw1), FD(wd1), FD(wd2), yflg);
#endif
  hw2 = hw1;
  if (isoutline || erosion < FixedHalf)
    goto done;
  rw = FRoundF(wd2);
#if GSMASK
  if (gmscale > 1) {
    if (rw == 0) rw = FixedOne;
    wd2 = rw - 0x2000L;
    }
  else
#endif /*GSMASK*/
    {
#define T1 0x2000L
#define T2 0x5000L
#define T3 0xE000L
    if (rw > FixedOne && wd1 <= rw - T1 && wd1 >= rw - T2)
      goto done;
    if (rw <= FixedOne) {
      wd2 = T3;
        /* do not make T3 too small or drop features like crossbar on
           Helvetica and HelveticaBold "t" at 8pt 72dpi
	   and serifs on TimesRoman "h" at 9pt 72dpi */
      }
    else if (wd2 > rw - T1)
      wd2 = rw - T1;
    else if (wd2 < rw - T2)
      wd2 = rw - T2;
#undef T1
#undef T2
#undef T3
    }
  if (yflg == (locktype < 0)) { temp.x = wd2; temp.y = 0; }
  else { temp.x = 0; temp.y = wd2; }
#if FBDEBUG
  printf("temp %g ", FD(wd2));
#endif
  FntIDTfmP(temp, &temp);
  wd2 = (yflg)? temp.y : temp.x;
  hw2 = os_labs(wd2) >> 1;
 done:
#if FBDEBUG
  printf("result %g\n", FD(hw2));
#endif
  return hw2;
  }
#endif /* USE68KATM */

#ifndef USE68KATM
private PFixed DoBlend(pp, wv, k, r)
  register PFixed pp; /* top of operand stack */
  register PFixed wv; /* WeightVector */
  register IntX k; /* length of WeightVector */
  register IntX r; /* number of results */
  {
  register Fixed sum;
  register PFixed p = NULL;
  register IntX i, j;
  for (i = 0; i < r; i++) {
    sum = 0; p = pp + i;
    for (j = k-1; j >= 0; j--) {
      p -= r;
      sum += fixmul(wv[j], *p); /* DO THIS IN-LINE IF POSSIBLE */
      }
    *p = sum;
    }
  return p + 1;
  }
#endif /* USE68KATM */

#ifndef USE68KATM
private Fixed PreXLock(xf, xn, xl, xa)
        Fixed xf, xn;                /* first and next points */
        Fixed *xl, *xa;              /* center point */
	    /* return desired half width in character space */
  {
  FCd temp;
  register Fixed hw, wd1;
  Fixed wd2;
  temp.y = 0; hw = (temp.x = xn - xf) >> 1; /* width in character space */
  FntDTfmP(temp, &temp); /* get width in device space */
  wd1 = (locktype < 0)? temp.y : temp.x;
  if (wd1 < 0) wd1 = -wd1;
  wd2 = wd1;
  if (lenstdvw > 0) UseStdWidth(&wd2, stdvw, lenstdvw);
  temp.y = 0; *xa = temp.x = xf + hw; /* center of stroke */
  FntTfmP(temp, &temp); /* center of stroke in device space */
  if (locktype < 0) temp.y = Adjust(wd2, temp.y);
  else              temp.x = Adjust(wd2, temp.x);
  FntITfmP(temp, &temp); /* adjusted center back in character space */
  *xl = temp.x;
  return CalcHW2(hw, wd1, wd2, false);
  }
#endif /* USE68KATM */

#ifndef USE68KATM
private Fixed PreYLock(yf, yn, yd, ya)
        Fixed yf, yn;                /* first and next points */
        Fixed *yd, *ya;              /* center point */
	    /* return desired half width in character space */
  {
  FCd temp;
  register Fixed hw, wd1;
  Fixed wd2;
  temp.x = 0; hw = (temp.y = yn - yf) >> 1; /* width in character space */
  FntDTfmP(temp, &temp); /* get width in device space */
  wd1 = (locktype < 0)? temp.x : temp.y;
  if (wd1 < 0) wd1 = -wd1;
  wd2 = wd1;
  if (lenstdhw > 0) UseStdWidth(&wd2, stdhw, lenstdhw);
  temp.x = 0; *ya = temp.y = yf + hw; /* center of stroke */
  FntTfmP(temp, &temp); /* center of stroke in device space */
  if (locktype < 0) temp.x = Adjust(wd2, temp.x);
  else              temp.y = Adjust(wd2, temp.y);
  FntITfmP(temp, &temp); /* adjusted center back in character space */
  *yd = temp.y;
  return CalcHW2(hw, wd1, wd2, true);
  }  /* end of PreYLock */
#endif /* USE68KATM */

#ifndef USE68KATM
private procedure TfmLockPt1(pt, p)  PFCd pt, p;
{ /* locktype != 0 && !dooffsetlock */
FCd fcd;
#if GLOBALCOLORING
if (glcrOn && glcrPrepass) { *p = *pt; return; }
#endif /* GLOBALCOLORING */ 
#if !ATM
if (!pLokData->slopesInited) InitSlopes();
fcd.x = Map(&pLokData->X, pt->x);
fcd.y = Map(&pLokData->Y, pt->y);
#else /* ATM */
if (!lokSlopesInited) InitSlopes();
fcd.x = Map(lokSeqX, pt->x);
fcd.y = Map(lokSeqY, pt->y);
#endif /* ATM */
FntTfmP(fcd, p);
}

#if DO_OFFSET
private procedure TfmLockPt2(pt, p)  PFCd pt, p;
{ /* locktype != 0 && dooffsetlock */
FCd fcd;
#if GLOBALCOLORING
if (glcrOn && glcrPrepass) { *p = *pt; return; }
#endif /* GLOBALCOLORING */ 
fcd.x = pt->x + lockoffset.x;  fcd.y = pt->y + lockoffset.y;
FntTfmP(fcd, p);
}
#endif /* DO_OFFSET */

private procedure TfmLockPt3(pt, p)  PFCd pt, p;
{ /* locktype == 0 */
FntTfmP(*pt, p);
}
#endif /* USE68KATM */

#if !ATM
private procedure TfmLockPt4(pt, p)  PFCd pt; PRCd p;
{ /* real numbers; locktype == 0 */
RCd c;
PFCdToPRCd(pt, &c);
TfmP(c, p);
}
#endif /* ATM */

internal procedure TriLock(p, sb, pre, post)
  register PFCd p; Fixed sb;
  Fixed (*pre)(/*xf, xn, xl, xa*/);
  procedure (*post)(/*ya, yd*/);
  {
  /* <x1> <dx1> <x2> <dx2> <x3> <dx3> */
  Fixed m0, m1, m2, md0, md1, md2, m0w, m1w, m2w, dhw0, dhw1, dhw2;
  Fixed hw0, hw1, hw2;
  register Fixed mtemp;
  if (locktype == 0) return;
  mtemp = sb;
  p[0].x += mtemp;  /* side bearings */
  p[1].x += mtemp; 
  p[2].x += mtemp; 
  m0w = p[0].y; m1w = p[1].y; m2w = p[2].y; /* width in character space */
  dhw0 = (*pre)(p[0].x, (p[0].y + p[0].x), &md0, &m0);
  dhw1 = (*pre)(p[1].x, (p[1].y + p[1].x), &md1, &m1);
  if (m1 < m0) {/* flip them */
    mtemp = md0; md0 = md1; md1 = mtemp;
    mtemp = m0; m0 = m1; m1 = mtemp;
    mtemp = m0w; m0w = m1w; m1w = mtemp;
    mtemp = dhw0; dhw0 = dhw1; dhw1 = mtemp;
    }
  dhw2 = (*pre)(p[2].x, (p[2].y + p[2].x), &md2, &m2);
#if GLOBALCOLORING
#if PPS
  if (post == NULL) return;
#else
  if (post == (void (*)())NULL) return;
#endif /*PPS*/
#endif /* GLOBALCOLORING */
  if (m2 < m0) {/* flip them */
    mtemp = md2; md2 = md1; md1 = md0; md0 = mtemp;
    mtemp = m2; m2 = m1; m1 = m0; m0 = mtemp;
    mtemp = m2w; m2w = m1w; m1w = m0w; m0w = mtemp;
    mtemp = dhw2; dhw2 = dhw1; dhw1 = dhw0; dhw0 = mtemp;
    }
  else if (m2 < m1) {/* flip them */
    mtemp = md2; md2 = md1; md1 = mtemp;
    mtemp = m2; m2 = m1; m1 = mtemp;
    mtemp = m2w; m2w = m1w; m1w = mtemp;
    mtemp = dhw2; dhw2 = dhw1; dhw1 = mtemp;
    }
  if ((md2 - md0) > (m2 - m0)) {
    mtemp = MIN((md1 - md0),(md2 - md1)); }
  else {
    mtemp = MAX((md1 - md0),(md2 - md1)); }
  md1 = md0 + mtemp;
  md2 = md1 + mtemp;
  hw0 = os_labs(m0w) >> 1;
  hw1 = os_labs(m1w) >> 1;
  hw2 = os_labs(m2w) >> 1;
  if (dhw0 == 0) dhw0 = hw0;
  if (dhw1 == 0) dhw1 = hw1;
  if (dhw2 == 0) dhw2 = hw2;
  (*post)(m0, md0, true, hw0, dhw0);
  (*post)(m1, md1, true, hw1, dhw1);
  (*post)(m2, md2, true, hw2, dhw2);
  }

#if GLOBALCOLORING

private procedure GetITfmX(px, p) Fixed *px, *p; {
  FCd c;
  if (locktype < 0) { c.y = *px; c.x = 0; }
  else { c.x = *px; c.y = 0; }
  FntITfmP(c, &c);
  *p = c.x;
  }

private procedure GetITfmY(py, p) Fixed *py, *p; {
  FCd c;
  if (locktype < 0) { c.x = *py; c.y = 0; }
  else { c.y = *py; c.x = 0; }
  FntITfmP(c, &c);
  *p = c.y;
  }

private procedure GetTfmX(px, p) Fixed *px, *p; {
  FCd c; c.y = 0; c.x = *px; FntTfmP(c, &c);
  *p = ((locktype < 0)? c.y : c.x); }

private procedure GetTfmY(py, p) Fixed *py, *p; {
  FCd c; c.x = 0; c.y = *py; FntTfmP(c, &c);
  *p = ((locktype < 0)? c.x : c.y); } 

private PGlbClr NewGlbClr() {
  if (freeGlbClr >= endGlbClrLst) { /* must grow buffer */
#if !ATM
    return NULL;
#else
    PGrowableBuffer b = pGblClrBuf;
    register PGlbClr oldLst = (PGlbClr)b->ptr;
    register PGlbClr nxt, newLst, g;
    if (!(*bprocs->GrowBuff)(b, (Int32)sizeof(GlbClr), true))
      return NULL; /* build the character without global coloring */
    newLst = (PGlbClr)b->ptr;
    if (newLst != oldLst) { /* fixup pointers */
      freeGlbClr = newLst + (freeGlbClr - oldLst);
      if (glbClrLst) {
        g = glbClrLst = newLst + (glbClrLst - oldLst);
        while ((nxt = g->next) != NIL) {
          g->next = newLst + (nxt - oldLst);
          g = g->next;
          }
        }
      }
    endGlbClrLst = newLst + b->len/sizeof(GlbClr);
#endif    
    }
  ++nGlbClrs;
  return freeGlbClr++;
  }

internal procedure EnterGlbClr(pf, pn, yflg) Fixed *pf, *pn; boolean yflg; {
  register PGlbClr g, lst, prv;
  Fixed cf, cn, f, n, tmp, w;
  cf = *pf; cn = *pn;
  if (cn < cf) { tmp = cn; cn = cf; cf = tmp; }
  if (yflg) { GetTfmY(pf, &f); GetTfmY(pn, &n); }
  else { GetTfmX(pf, &f); GetTfmX(pn, &n); }
  if (n < f) { tmp = n; n = f; f = tmp; }
  w = n - f;
  /* keep list sorted by increasing width */
  prv = NULL; lst = glbClrLst;
  while (lst && lst->w < w) { prv = lst; lst = lst->next; }
  while (lst && lst->w == w) {
    if (lst->cf == cf && lst->cn == cn && lst->yflg == yflg) {
      lst->active = true;
      return; /* duplicate */
      }
    prv = lst; lst = lst->next;
    }
  g = NewGlbClr();
  if (!g) {glcrFailure = true; return;}
  g->cf = cf; g->cn = cn;
  g->mn = FixInt(16000); g->mx = -g->mn;
  g->f = f; g->n = n; g->yflg = yflg; g->stdwidth = false;
  g->w = g->w1 = w; g->anchored = false; g->active = true;
  if (!prv) { g->next = glbClrLst; glbClrLst = g; }
  else { prv->next = g; g->next = lst; }
  }

private PGlbClr FindGlbClr(pf, pn, yflg) Fixed *pf, *pn; boolean yflg; {
  register PGlbClr g;
  Fixed f, n, tmp, w;
  f = *pf; n = *pn;
  if (n < f) { tmp = n; n = f; f = tmp; }
  w = n - f;
  g = glbClrLst;
  while (g) {
    if (g->cf == f && g->cn == n && g->yflg == yflg)
      return g;
    g = g->next;
    }
  return NULL;
  }

private procedure FinGlbClrLocs(g, d) register PGlbClr g; Fixed d; {
  Fixed ff, nn, hw1;
  ff = g->f + d; /* postcolor device coords of edges */
  nn = g->n + d;
  if (!isoutline && erosion >= FixedHalf) {
    ff += FixedHalf; nn -= FixedHalf; }
  g->ff = FTruncF(ff);
  g->nn = FTruncF(nn) + FixedOne;
  if (g->nn <= g->ff) g->nn = g->ff + FixedOne;
  hw1 = (g->cn - g->cf) >> 1;
  g->hw2 = CalcHW2(hw1, g->w1, g->w, g->yflg);
  }

internal procedure CalcGlbClrLocs(g) register PGlbClr g; {
  Fixed m;
  m = (g->f >> 1) + (g->n >> 1);
  FinGlbClrLocs(g, Adjust(g->w, m) - m);
  }
 
internal procedure CalcAnchGlbClrLocs(g, w) register PGlbClr g; Fixed w; {
  FinGlbClrLocs(g, w - ((g->f >> 1) + (g->n >> 1)));
  g->anchored = true;
  }

internal procedure DoLock(pf, pn, yflg) Fixed *pf, *pn; boolean yflg; {
  PGlbClr g;
  Fixed hw1, mm;
  procedure (*setlock)();
  g = FindGlbClr(pf, pn, yflg);
  hw1 = (*pn - *pf) >> 1; /* ideal half width in character space */
  if (hw1 < 0) hw1 = -hw1;
  mm = (g->ff >> 1) + (g->nn >> 1); /* postcolor midpt; device space */
  if (yflg) { GetITfmY(&mm, &mm); setlock = SetYLock; }
  else      { GetITfmX(&mm, &mm); setlock = SetXLock; }
  (*setlock)(*pf + hw1, mm, true, hw1, g->hw2);
  }

internal Fixed EnterTriXLock(xf, xn, p0, p1) Fixed xf, xn, *p0, *p1; {
  Fixed f, n; f = xf; n = xn; EnterGlbClr(&f, &n, false);
  return *p0 = *p1 = 0L; }

internal Fixed EnterTriYLock(yf, yn, p0, p1) Fixed yf, yn, *p0, *p1; {
  Fixed f, n; f = yf; n = yn; EnterGlbClr(&f, &n, true);
  return *p0 = *p1 = 0L; }

internal Fixed DoTriXLock(xf, xn, p0, p1) Fixed xf, xn, *p0, *p1; {
  Fixed f, n; f = xf; n = xn; DoLock(&f, &n, false); return 0L; }

internal Fixed DoTriYLock(yf, yn, p0, p1) Fixed yf, yn, *p0, *p1; {
  Fixed f, n; f = yf; n = yn; DoLock(&f, &n, true); return 0L;}

internal procedure GlbClrLine(c0, c1) FCd c0, c1; {
  /* c0 is start, c1 is end.  both are in character space coords */
  Fixed absdx, absdy, dx, dy, mn, mx, center;
  boolean first, yflg;
  PGlbClr g, bst;
  Fixed bstd, d;
  dx = c1.x - c0.x; dy = c1.y - c0.y;
  absdx = os_labs(dx); absdy = os_labs(dy);
  if (absdx <= FixInt(2) && absdy >= FixInt(15)) { /* vertical */
    yflg = false;
    center = (dx == 0) ? c0.x : c0.x + (dx >> 1);
    if (dy < 0) {
      first = true; mx = c0.y; mn = c1.y; }
    else {
      first = false; mx = c1.y; mn = c0.y; }
    }
  else if (absdy <= FixInt(2) && absdx >= FixInt(15)) { /* horizontal */
    yflg = true;
    center = (dy == 0) ? c0.y : c0.y + (dy >> 1);
    if (dx < 0) {
      first = false; mx = c0.x; mn = c1.x; }
    else {
      first = true; mx = c1.x; mn = c0.x; }
    }
  else return;
  bst = NULL; bstd = FixInt(10000);
  for (g = glbClrLst; g; g = g->next) {
    if (!g->active || g->yflg != yflg) continue;
    d = center - (first ? g->cf : g->cn); d = os_labs(d);
    if (d < bstd) { bstd = d; bst = g; }
    }
  if (bstd <= FixInt(3)) {
    if (mn < bst->mn) bst->mn = mn;
    if (mx > bst->mx) bst->mx = mx;
    }
  }

#endif /* GLOBALCOLORING */

#ifndef USE68KATM
private procedure RVLock(p, sby) PFCd p; Fixed sby; {
  if (locktype == 0 || !gsmatrix->noYSkew) return;
#if GLOBALCOLORING
  if (glcrOn)
    TriLock(p, sby, (glcrPrepass ? EnterTriYLock : DoTriYLock),
            (void (*)())NULL);
  else
#endif /* GLOBALCOLORING */
    TriLock(p, sby, PreYLock, SetYLock);
  }  /* end of RVLock */
#endif /* USE68KATM */

#ifndef USE68KATM
private procedure RMLock(p, sbx) PFCd p; Fixed sbx; {
  if (locktype == 0) return;
#if GLOBALCOLORING
  if (glcrOn)
    TriLock(p, sbx, (glcrPrepass ? EnterTriXLock : DoTriXLock),
            (void (*)())NULL);
  else
#endif /* GLOBALCOLORING */
    TriLock(p, sbx, PreXLock, SetXLock);
  }  /* end of RMLock */
#endif /* USE68KATM */

#define BLUESCALE 2597
  /* Fixed (((300.0/72.0)*9.51)/1000.0) */
  /* == 0.039625 */
  /* == scalefactor of 9.51 pt font at 300 dpi */
#define MAXBLUESHIFT (FixedHalf-1)
  /* Fixed (.499) */
  /* by a shift of this amount we can control overshoots of up to 25 units
      in character space -- larger overshoots will show up before 9.5 pt */

#define Bpick(cd) ((locktype < 0) ? cd.x : cd.y)

#if FBDEBUG && GLOBALCOLORING

private procedure PrntClr(g) register PGlbClr g; {
  printf(" cf %g cn %g f %g n %g ff %g nn %g mn %g mx %g\n",
    FD(g->cf), FD(g->cn), FD(g->f), FD(g->n), FD(g->ff), FD(g->nn),
    FD(g->mn), FD(g->mx));
  }

private procedure PrintClrs(yflg) boolean yflg; {
  register PGlbClr g;
  for (g = glbClrLst; g; g = g->next) {
    if (g->yflg == yflg) PrntClr(g);
    }
  }

private procedure PrintCntrs() {
  register PGlbCntr g;
  for (g = glbCntrLst; g; g = g->next) {
    printf(g->upper->yflg ? "Y counter" : "X counter");
    printf(": width %g\n", FD(g->upper->ff - g->lower->nn));
    printf("    upper: "); PrntClr(g->upper);
    printf("    lower: "); PrntClr(g->lower);
    printf("\n");
    }
  }
#endif /* FBDEBUG */

internal procedure CheckBlueScale(botBands, lenBotBands, topBands, lenTopBands)
  IntX lenBotBands, lenTopBands; Fixed *botBands, *topBands; {
  Fixed maxovershoot, overshoot;
  register IntX i, nblues;
  register Fixed *gblues;
  boolean baseline;
  maxovershoot = 0;
  nblues = lenBotBands; gblues = botBands; baseline = true;
  while (true) {
    for (i = 0; i < nblues; i += 2) {
      overshoot = gblues[1] - gblues[0];
      if (overshoot > maxovershoot) maxovershoot = overshoot;
      }
    if (!baseline) break;
    baseline = false; nblues = lenTopBands; gblues = topBands;
    }
  overshoot = fixmul(maxovershoot,blueScale);
  if (overshoot < FixedOne) return;
  blueScale = fixdiv(FixedOne, maxovershoot) - 1;
  }

internal procedure SetupBlueLocs(
  botBands, lenBotBands, topBands, lenTopBands, botLocs, topLocs)
  IntX lenBotBands, lenTopBands;
  Fixed *botBands, *topBands; FCd *botLocs, *topLocs; {
  IntX i, nblues;
  register Fixed *gblues;
  register FCd *glocs;
  boolean baseline;
  FCd tfm;
  baseline = true; nblues = lenBotBands;
  gblues = botBands; glocs = botLocs;
  while (true) {
    for (i = 0; i < nblues; i++) {
      tfm.x = 0; tfm.y = gblues[i];
      FntTfmP(tfm, &glocs[i]);
      }
    if (!baseline) break;
    baseline = false; nblues = lenTopBands;
    gblues = topBands; glocs = topLocs;
    }
  }

#if FBDEBUG
private procedure PrintBlueLocs(botLocs, lenBotBands, topLocs, lenTopBands)
  FCd *botLocs, *topLocs; IntX lenBotBands, lenTopBands; {
  IntX i;
  for (i = 0; i < lenBotBands; i += 2)
    printf("bottom band locs %g %g\n",
      FD(Bpick(botLocs[i])), FD(Bpick(botLocs[i+1])));
  for (i = 0; i < lenTopBands; i += 2)
    printf("top band locs %g %g\n",
      FD(Bpick(topLocs[i])), FD(Bpick(topLocs[i+1])));
  }
#endif

private procedure AdjustBlues(botLocs, lenBotBands, topLocs, lenTopBands,
  famBotBands, lenFamBotBands, famTopBands, lenFamTopBands, raisetops)
  FCd *botLocs, *topLocs; IntX lenFamBotBands, lenFamTopBands;
  boolean raisetops; Fixed *famBotBands, *famTopBands;
  IntX lenBotBands, lenTopBands; {
  FCd famBotLocs[MAXBLUES], famTopLocs[MAXBLUES], c0, c1;
  register FCd *gblocs = NULL, *gflocs = NULL;
  register Fixed b, d, fd;
  register IntX j, k, nblues = 0, nfblues = 0;
  IntX i, bst = 0;
  SetupBlueLocs(famBotBands, lenFamBotBands, famTopBands, lenFamTopBands,
                famBotLocs, famTopLocs);
#if FBDEBUG
  printf("family blue locs\n");
  PrintBlueLocs(famBotLocs, lenFamBotBands, famTopLocs, lenFamTopBands);
  printf("initial blue locs\n");
  PrintBlueLocs(botLocs, lenBotBands, topLocs, lenTopBands);
#endif
  for (i = 0; i < 4; i++) {
    switch (i) {
      case 0: gblocs = botLocs; nblues = lenBotBands;
              gflocs = famBotLocs; nfblues = lenFamBotBands; break;
      case 1: gblocs = &botLocs[1]; nblues = lenBotBands;
              gflocs = &famBotLocs[1]; nfblues = lenFamBotBands; break;
      case 2: gblocs = topLocs; nblues = lenTopBands;
              gflocs = famTopLocs; nfblues = lenFamTopBands; break;
      case 3: gblocs = &topLocs[1]; nblues = lenTopBands;
              gflocs = &famTopLocs[1]; nfblues = lenFamTopBands; break;
      }
    for (j = 0; j < nblues; j += 2) {
      b = Bpick(gblocs[j]); d = FixInt(1000);
      for (k = 0; k < nfblues; k += 2) { /* find closest family blue */
	fd = Bpick(gflocs[k]) - b;
	fd = os_labs(fd);
	if (fd < d) {
          bst = k; d = fd; }
        }
      if (d < FixedOne) /* adjust blue loc to family loc */
        gblocs[j] = gflocs[bst];
      if (raisetops && i == 3 && stdhw[0] < FixedOne) {
        Fixed toploc, botloc, edgeloc, midloc;
        toploc = Bpick(gblocs[j]); botloc = Bpick(gblocs[j-1]);
	edgeloc = (toploc > botloc)? toploc - stdhw[0] : toploc + stdhw[0];
	midloc = (toploc >> 1) + (edgeloc >> 1);
	if (FTrunc(midloc) == FTrunc(toploc)) {
          /* take edge that corresponds to larger value in character space */
          if (locktype < 0) {
            fd = gblocs[j].x; fd -= FRoundF(fd); fd = os_labs(fd);
	    if (fd > 0x2000L) {
              c0.x = gblocs[j].x = FTruncF(gblocs[j].x); c0.y = 0;
	      FntITfmP(c0, &c0);
	      c1.x = gblocs[j].x + FixedOne; c1.y = 0;
	      FntITfmP(c1, &c1);
	      if (c1.y > c0.y) gblocs[j].x += FixedOne;
	      }
	    }
          else if (locktype > 0) {
            fd = gblocs[j].y; fd -= FRoundF(fd); fd = os_labs(fd);
	    if (fd > 0x2000L) {
              c0.y = gblocs[j].y = FTruncF(gblocs[j].y); c0.x = 0;
	      FntITfmP(c0, &c0);
	      c1.y = gblocs[j].y + FixedOne; c1.x = 0;
	      FntITfmP(c1, &c1);
	      if (c1.y > c0.y) gblocs[j].y += FixedOne;
	      }
            }
	  }
        }
      }
    }
  }

#if !ATM
private procedure FamilyBlueLocs(
  privdict, botLocs, lenBotBands, topLocs, lenTopBands, raisetops)
  DictObj privdict; FCd *botLocs, *topLocs; IntX lenBotBands, lenTopBands;
  boolean raisetops; {
  Fixed famBotBands[MAXBLUES], famTopBands[MAXBLUES];
  IntX lenFamBotBands, lenFamTopBands;
  GetBlueArrays(famBluesNm, famOtherBluesNm,
       privdict, famBotBands, &lenFamBotBands, famTopBands, &lenFamTopBands);
  if (lenFamBotBands == 0 && lenFamTopBands == 0) return;
  AdjustBlues(botLocs, lenBotBands, topLocs, lenTopBands,
              famBotBands, lenFamBotBands, famTopBands, lenFamTopBands,
	      raisetops);
  }
#else /* ATM */
internal procedure FamilyBlueLocs(
  priv, botLocs, lenBotBands, topLocs, lenTopBands, raisetops)
  PFontPrivDesc priv; FCd *botLocs, *topLocs; boolean raisetops;
  IntX lenBotBands, lenTopBands; {
  Fixed famBotBands[MAXBLUES], famTopBands[MAXBLUES];
  IntX lenFamBotBands, lenFamTopBands;
  if (priv->numFamilyBlues == 0 && priv->numFamilyOtherBlues == 0)
    return;
  GetBlueArrays(priv->familyBlues, (IntX)(priv->numFamilyBlues),
    priv->familyOtherBlues, (IntX)(priv->numFamilyOtherBlues),
    famBotBands, &lenFamBotBands, famTopBands, &lenFamTopBands);
  AdjustBlues(botLocs, lenBotBands, topLocs, lenTopBands, famBotBands,
    lenFamBotBands, famTopBands, lenFamTopBands, raisetops);
  }
#endif /* ATM */

internal procedure BoostBotLocs(botLocs, lenBotBands)
  register FCd *botLocs; register IntX lenBotBands; {
  register Fixed bot, dist;
  register IntX i;
  if (lenBotBands <= 2 || Bpick(botLocs[1]) != 0)
    return;
  for (i = 3; i < lenBotBands; i += 2) {
    bot = Bpick(botLocs[i]);
    dist = os_labs(bot);
    if (dist > FixedOne && dist < (FixedOne+FixedHalf)) {
      /* move so that have a gap between this one and baseline */
      /* Courier-Bold 10pt @ 72dpi descenders need this (q,p,y,g) */
      bot = (bot > 0)? (FixedOne+FixedHalf+1) : -(FixedOne+FixedHalf+1);
      if (locktype < 0) botLocs[i].x = bot;
      else botLocs[i].y = bot;
      }
    }
  }

#ifndef USE68KATM
internal procedure BlueLock(yf, yn,
#if GLOBALCOLORING
    w, g,
#endif /* GLOBALCOLORING */
    botBands, lenBotBands, topBands, lenTopBands, botLocs, topLocs, bluefuzz)
  Fixed yf, yn;
#if GLOBALCOLORING
  Fixed w; PGlbClr g;
#endif /* GLOBALCOLORING */ 
  Fixed *botBands; IntX lenBotBands;
  Fixed *topBands; IntX lenTopBands;
  FCd *botLocs, *topLocs;
  Fixed bluefuzz;
  {
  register FCd *glocs = NULL;
  register Fixed yv0, yv1, blueshift, ydist, *gblues = NULL;
  Fixed globy = 0, wabs, halfw, ww, ya, q, k;
  Fixed yb, fuzzdist, hw1, hw2, wd1, wd2;
  FCd temp, tfm;
  register IntX i;
  IntX nblues = 0, m, n;
  boolean baseline = false, done;

  temp.x = 0; hw1 = (temp.y = yn - yf) >> 1;
  FntDTfmP(temp, &temp);
  wd1 = ww = Bpick(temp);
#if GLOBALCOLORING
  if (!g && lenstdhw > 0)
#else /* GLOBALCOLORING */
  if (lenstdhw > 0)
#endif /* GLOBALCOLORING */
    {
    wabs = os_labs(ww);
    if (UseStdWidth(&wabs, stdhw, lenstdhw))
      ww = (ww < 0)? -wabs: wabs;
    }
#if GLOBALCOLORING
  if (g)
    ww = (ww < 0)? -w : w;
#endif /* GLOBALCOLORING */ 
  ya = yf + hw1;
  fuzzdist = 0;
  for (n = 0; n < 2; n++) {
    if (n > 0) {
      if (bluefuzz == 0) break;
      fuzzdist = bluefuzz;
      }
    for (m = 0; m < 3; m++) {
      switch (m) {
        case 0:
          baseline = true; nblues = (lenBotBands >= 2) ? 2 : 0;
  	  gblues = botBands; glocs = botLocs;
  	break;
        case 1:
          baseline = false; nblues = lenTopBands;
  	  gblues = topBands; glocs = topLocs;
  	break;
        case 2:
          baseline = true; nblues = (lenBotBands >= 2) ? lenBotBands-2 : 0;
  	  gblues = botBands + 2; glocs = botLocs + 2;
  	break;
        }
      for (i = 0; i < nblues; i += 2) {
        yv0 = gblues[i];
        yv1 = gblues[i+1];
        yb = baseline? yf : yn;
        if ((yb >= (yv0 - fuzzdist)) && (yb <= (yv1 + fuzzdist)))
          {
          yf = yb-yv0;
          if (os_labs(yf) <= bluefuzz) yb = yv0;
          yf = yb-yv1;
          if (os_labs(yf) <= bluefuzz) yb = yv1;
          done = false;
          if (!baseline) {
            k = FRoundF(fixmul(blueScale, yv1));
            if (k < 0) k = -k;
            /* height in pixels where ok to start to overshoot */
            temp.x = 0; temp.y = yv0; FntDTfmP(temp, &temp);
            q = FRoundF(Bpick(temp));
            if (q < 0) q = -q;
            /* height in pixels of bottom of current band */
            if (q < k) { /* no overshoot */
              tfm = glocs[i+1]; globy = FRoundF(Bpick(tfm));
              done = true;
              }
            }
          if (!done) {
            tfm = glocs[(baseline)? i+1 : i];
            globy = FRoundF(Bpick(tfm));
            yf = (baseline) ? yv1-yb : yb-yv0;
            if (yf != 0) {
              temp.x = 0; temp.y = yf; FntDTfmP(temp, &temp);
              ydist = Bpick(temp);
              blueshift = os_labs(yf);
              if (blueshift >= blueShiftStart) {
                blueshift = fixmul(blueshift, blueScale) - FixedHalf;
                if (blueshift > MAXBLUESHIFT) blueshift = MAXBLUESHIFT;
                if (blueshift < -MAXBLUESHIFT) blueshift = -MAXBLUESHIFT;
                ydist = (ydist >= 0) ? (ydist-blueshift): (ydist+blueshift);
                }
              ydist = FRoundF(ydist);
              if (baseline) globy -= ydist; else globy += ydist;
              }
            }
          wd2 = os_labs(ww);
          if ((wabs = FRoundF(wd2)) == 0 && (isoutline || erosion >= FixedHalf))
            wabs = FixedOne;
          halfw = wabs >> 1;
          if ((isoutline && devsweven) || (!isoutline && erosion < FixedHalf))
            halfw += FixedHalf;
          if (ww < 0) halfw = -halfw; /* in case y axis is flipped */
          globy = baseline? (globy+halfw): (globy-halfw);
#if GLOBALCOLORING
          if (g) {
            CalcAnchGlbClrLocs(g, globy);
            return;
            }
#endif /* GLOBALCOLORING */
          /* do Bflip */
          if (locktype < 0) tfm.x = globy;
          else tfm.y = globy;
          FntITfmP(tfm, &tfm);
          hw2 = CalcHW2(hw1, wd1, wd2, true);
          SetYLock(ya, tfm.y, true, hw1, hw2);
          return;
          }
        /* continue next loop iteration */
        }
      }
    }
  /* not a special coloring band */
#if GLOBALCOLORING
  if (g) {
    CalcGlbClrLocs(g);
    return; }
#endif /* GLOBALCOLORING */
  wd2 = os_labs(ww);
  temp.x = 0; temp.y = ya;
  FntTfmP(temp, &temp);
  if (locktype < 0) temp.x = Adjust(wd2, temp.x);
  else              temp.y = Adjust(wd2, temp.y);
  FntITfmP(temp, &temp);
  hw2 = CalcHW2(hw1, wd1, wd2, true);
  SetYLock(ya, temp.y, false, hw1, hw2);
  }  /* end of BlueLock */
#endif /* USE68KATM */

#ifndef USE68KATM
private procedure RBLock(p, sby,
    botBands, lenBotBands, topBands, lenTopBands, botLocs, topLocs, bluefuzz)
  PFCd p; Fixed sby;
  Fixed *botBands; IntX lenBotBands;
  Fixed *topBands; IntX lenTopBands;
  FCd *botLocs, *topLocs;
  Fixed bluefuzz;
  {
  Fixed yf, yn, ftmp;
  if (locktype == 0 || !gsmatrix->noYSkew) return;
  yn = (yf = sby + p[0].x) + p[0].y;
  if (yn < yf) {ftmp = yn; yn = yf; yf = ftmp;}
#if GLOBALCOLORING
  if (glcrOn) {
    if (glcrPrepass) EnterGlbClr(&yf, &yn, true);
    else DoLock(&yf, &yn, true);
    return;
    }
#endif /* GLOBALCOLORING */
  BlueLock(yf, yn,
#if GLOBALCOLORING
      0, NULL,
#endif /* GLOBALCOLORING */
      botBands, lenBotBands, topBands, lenTopBands, botLocs, topLocs, bluefuzz);
  }  /* end of RBLock */
#endif /* USE68KATM */

#ifndef USE68KATM
private procedure RYLock(p, sbx)
  PFCd p; Fixed sbx; {
  Fixed xf, xn, hw2, ftmp1, ftmp2;
  if (locktype == 0) return;
  /* do locking even if there is skew in X direction */
  xn = (xf = sbx + p[0].x) + p[0].y;
  if (xn < xf) {ftmp1 = xn; xn = xf; xf = ftmp1;}
#if GLOBALCOLORING
  if (glcrOn) {
    if (glcrPrepass) EnterGlbClr(&xf, &xn, false);
    else             DoLock(&xf, &xn, false);
    return;
    }
#endif /* GLOBALCOLORING */
  hw2 = PreXLock(xf, xn, &ftmp2, &ftmp1);
  SetXLock(ftmp1, ftmp2, false, ((xn - xf) >> 1), hw2);
  }  /* end of RYLock */
#endif /* USE68KATM */

#if GLOBALCOLORING

#if 0
private PGlbClr NxtGClr(g) register PGlbClr g; {
  /* filter out the standard widths so they do not take part in adjustments */
  g = g->next;
  while (g && g->stdwidth) g = g->next;
  return g;
  }
#endif

private PGlbCntr NewGlbCntr() {
  if (freeGlbCntr >= endGlbCntrLst) { /* must grow buffer */
#if !ATM
    return NULL;
#else
    PGrowableBuffer b = pGblCntrBuf;
    register PGlbCntr oldLst = (PGlbCntr)b->ptr;
    register PGlbCntr nxt, newLst, g;
    if (!(*bprocs->GrowBuff)(b, (Int32)sizeof(GlbCntr), true))
      return NULL; /* build the character without global coloring */
    newLst = (PGlbCntr)b->ptr;
    if (newLst != oldLst) { /* fixup pointers */
      freeGlbCntr = newLst + (freeGlbCntr - oldLst);
      if (glbCntrLst) {
        g = glbCntrLst = newLst + (glbCntrLst - oldLst);
        while (nxt = g->next) {
          g->next = newLst + (nxt - oldLst);
          g = g->next;
          }
        }
      }
    endGlbCntrLst = newLst + b->len/sizeof(GlbCntr);
#endif    
    }
  ++nGlbCntrs;
  return freeGlbCntr++;
  }

private procedure EnterGlbCntr(upper, lower) PGlbClr upper, lower; {
  register PGlbCntr g;
  Fixed w = upper->f - lower->n;
  /* check overlap before create a counter */
  Fixed mn, mx, overlaplen, upperlen, lowerlen;
  mn = MAX(upper->mn, lower->mn);
  mx = MIN(upper->mx, lower->mx);
  overlaplen = mx - mn;
  if (overlaplen <= 0) return;
  upperlen = upper->mx - upper->mn;
  lowerlen = lower->mx - lower->mn;
  if ((overlaplen << 1) < MIN(upperlen, lowerlen)) return;
  Assert(w > 0);
  g = NewGlbCntr();
  if (!g) {glcrFailure = true; return;}
  g->w = w; g->upper = upper; g->lower = lower;
  g->next = glbCntrLst; glbCntrLst = g;
  }

private procedure BuildGlbCntrs() {
  register PGlbClr g, gnxt;
  register boolean yflg;
  register Fixed f, n;
  for (g = glbClrLst; g; g = g->next) {
    yflg = g->yflg; f = g->f; n = g->n;
    for (gnxt = g->next; gnxt; gnxt = gnxt->next) {
      if (gnxt->yflg != yflg) continue;
      if (gnxt->f > n) EnterGlbCntr(gnxt, g);
      else if (f > gnxt->n) EnterGlbCntr(g, gnxt);
      }
    }
  }

private procedure GlbFixLocs() {
  register PGlbClr g1, g2, g3=NULL, g0;
  register Fixed w;
  for (g1 = glbClrLst; g1; g1 = g1->next) {
    for (g2 = g1->next; g2; g2 = g2->next) {
      if (g1->yflg != g2->yflg) continue;
      if (!g1->yflg) {
        if (g1->f == g2->f && g1->ff != g2->ff) {
          /* left edges same in character space but not in device space */
          if (g1->n < g2->n && !g2->anchored) { /* use left edge of g1 */
            w = g2->nn - g2->ff; g2->ff = g1->ff; g2->nn = g2->ff + w;
	    }
          else if (!g1->anchored) { /* use left edge of g2 */
            w = g1->nn - g1->ff; g1->ff = g2->ff; g1->nn = g1->ff + w;
	    }
          }
        else if (g1->n == g2->n && g1->nn != g2->nn) {
          /* right edges same in character space but not in device space */
          if (g1->f > g2->f && !g2->anchored) { /* use right edge of g1 */
            w = g2->nn - g2->ff; g2->nn = g1->nn; g2->ff = g2->nn - w;
	    }
          else if (!g1->anchored) { /* use right edge of g2 */
            w = g1->nn - g1->ff; g1->nn = g2->nn; g1->ff = g1->nn - w;
	    }
          }
        }
      g0 = NULL;
      if (g1->f >= g2->f && g1->n <= g2->n) { /* g1 inside g2 */
        g0 = g1; g3 = g2; }
      else if (g2->f >= g1->f && g2->n <= g1->n) { /* g2 inside g1 */
        g0 = g2; g3 = g1; }
      if (g0) { /* g0 inside g3 */
        if (g0->ff < g3->ff) { /* align ff side */
          if (!g3->anchored) { /* move g3 */
            w = g3->nn - g3->ff; g3->ff = g0->ff; g3->nn = g3->ff + w;
            }
          else if (!g0->anchored) { /* move g0 */
            w = g0->nn - g0->ff; g0->ff = g3->ff; g0->nn = g0->ff + w;
            }
          }
	else if (g0->nn > g3->nn) { /* align nn side */
          if (!g3->anchored) { /* move g3 */
            w = g3->nn - g3->ff; g3->nn = g0->nn; g3->ff = g3->nn - w;
	    }
          else if (!g0->anchored) { /* move g0 */
            w = g0->nn - g0->ff; g0->nn = g3->nn; g0->ff = g0->nn - w;
	    }
          }
        }
      }
    }
  }

internal procedure ProcessGlbClrs(botBands, lenBotBands, topBands, lenTopBands,
    bluefuzz, b3, botLocs, topLocs, ef)
  IntX lenBotBands, lenTopBands; PGrowableBuffer b3;
  Fixed *botBands, *topBands, bluefuzz, ef;
  FCd *botLocs, *topLocs; {
  register PGlbClr g;
  g = glbClrLst;
  while (g) {
    if (!g->yflg)
      CalcGlbClrLocs(g);
    else {
      /* cf and cn must be sorted properly for the following line.  If the
         global coloring code is changed, a conditional exchange might be
         needed here. */
      BlueLock(g->cf,g->cn,g->w,g,botBands,lenBotBands,topBands,lenTopBands,
              botLocs,topLocs,bluefuzz);
      }
    g = g->next;
    }
  GlbFixLocs();
  for (g = glbClrLst; g; g = g->next) g->w = g->nn - g->ff;
  BuildGlbCntrs();
  if (!GlobalColoring(glbClrLst, glbCntrLst, b3, ef, nGlbCntrs, nGlbClrs))
    glcrFailure = true;
  }

#if !ATM
private procedure IniGlbClrBuffs()
#else /* ATM */
internal procedure IniGlbClrBuffs(b1, b2) PGrowableBuffer b1, b2;
#endif /* ATM */
  {
#if !ATM
  /* fix this someday to get rid of max size mallocs */
  IntX i;
  i = sizeof(GlbCntr) * NDIFF;
  if (gblClrBuf1.ptr == NULL) {
    gblClrBuf1.ptr = (char *)os_malloc(i);
    gblClrBuf1.len = i;
    }
  pGblCntrBuf = &gblClrBuf1;
  i = sizeof(GlbClr) * MAXBANDS;
  if (gblClrBuf2.ptr == NULL) {
    gblClrBuf2.ptr = (char *)os_malloc(i);
    gblClrBuf2.len = i;
    }
  pGblClrBuf = &gblClrBuf2;
  i = sizeof(PGlbClr) * MAXBANDS + sizeof(PGlbCntr) * NDIFF;
  if (gblClrBuf3.ptr == NULL) {
    gblClrBuf3.ptr = (char *)os_malloc(i);
    gblClrBuf3.len = i;
    }
#else /* ATM */
  pGblClrBuf = b2; pGblCntrBuf = b1;
#endif /* ATM */
  freeGlbClr = (PGlbClr)(pGblClrBuf->ptr);
  endGlbClrLst = freeGlbClr + pGblClrBuf->len/sizeof(GlbClr);
  glbClrLst = NULL;
  freeGlbCntr = (PGlbCntr)(pGblCntrBuf->ptr);
  endGlbCntrLst = freeGlbCntr + pGblCntrBuf->len/sizeof(GlbCntr);
  glbCntrLst = NULL;
  nGlbClrs = 0;
  nGlbCntrs = 0;
  }

#endif /* GLOBALCOLORING */

#define FONTKEY ((Card16) 4330)
#define C1 ((Card16) 52845)
#define C2 ((Card16) 22719)

#define Decrypt(r, clear, cipher)\
  clear = (unsigned char)((cipher)^(r>>8)); \
  r = (Card16)((Card16)(cipher) + r)*C1 + C2
  
#define MAXSUB 10
#define MAXCDs 12

/* ***PS*** this is the tail section of fontbuild for PS */

#if !ATM

private procedure LockPFCd(pfcd, pfcdret)  PFCd pfcd, pfcdret;
{
if (!pLokData->slopesInited) InitSlopes();
pfcdret->x = Map(&pLokData->X, pfcd->x);
pfcdret->y = Map(&pLokData->Y, pfcd->y);
}  /* end of LockPFCd */

public Cd LockCd(cp)  Cd cp;
{
FCd fcd;
PRCdToPFCd(&cp, &fcd);
LockPFCd(&fcd, &fcd);
PFCdToPRCd(&fcd, &cp);
return cp;
}  /* end of LockCd */

public procedure PSSetXLock()
{
real c1, c2;
Fixed f1, f2;
PopPReal(&c2); PopPReal(&c1);
f1 = pflttofix(&c1);  f2 = pflttofix(&c2);
SetXLock(f1, f2, false, 0L, 0L);
}  /* end of PSSetXLock */

public procedure PSSetYLock()
{
real c1, c2;
Fixed f1, f2;
PopPReal(&c2); PopPReal(&c1);
f1 = pflttofix(&c1);  f2 = pflttofix(&c2);
SetYLock(f1, f2, false, 0L, 0L);
}  /* end of PSSetYLock */

public procedure PSLck()
{
Cd c;
PopPCd(&c);
c = LockCd(c);
PushPCd(&c);
}  /* end of PSLck */

private Fixed GetFromPrivDict(privdict, nm, val)
  DictObj privdict; NameObj nm; Fixed val;
{
Object ob;
if (ForceKnown(privdict, nm))
  {ForceGetP(privdict, nm, &ob);  return FixedValue(&ob);}
else return val;
}  /* end of GetFromPrivDict */

private procedure DictGetFontP(f, nm, p) DictObj f; Object nm, *p; {
  if (!Known(f, nm)) InvlFont();
  DictGetP(f, nm, p);
  }

private procedure SetRealPathProcs() {
  moveto = RMoveTo; lineto = RLineTo; curveto = RCurveTo;
  closepath = RClosePath;
  }

private procedure SetFixedPathProcs() {
  moveto = FMoveTo; lineto = FLineTo; curveto = FCurveTo;
  closepath = RClosePath; /* does the same thing */
  }

private procedure GetMtxInfo() {
  usefix = SetupFntMtx();
  toosmall=(gsmatrix->len1000 < 0x40000);
  locktype =
    (!usefix || gs->isCharPath || toosmall ||
      (!gsmatrix->noYSkew && !gsmatrix->noXSkew)) ? 0 :
    (gsmatrix->switchAxis)? -1 : 1;
  if (!usefix) {
    tfmLockPt = TfmLockPt4;
    SetRealPathProcs();
    }
  else {
    if (locktype == 0) tfmLockPt = TfmLockPt3;
    else tfmLockPt = TfmLockPt1;
    SetFixedPathProcs();
    }
  }

private procedure PathEndChar(fd, pcn, fm)
  DictObj fd; Object *pcn; FMetrics *fm; {
  RMetrics metrics;
  Object obj;
  Fixed devsw;
  real oldStrkFoo = strkFoo;
  RCd tempcd, delta;
  PFCdToPRCd(&fm->w0, &metrics.w0);
#if !PPS || PPSkanji
  PFCdToPRCd(&fm->w1, &metrics.w1);
  PFCdToPRCd(&fm->v, &metrics.v);
#endif
  devsw = info_devsw; if (devsw < FixedOne) devsw = FixedOne;
  fixtopflt(devsw << 1, &delta.x);
  delta.y = delta.x;
  VecSub(gs->path.bbox.bl, delta, &metrics.c0);
  ITfmP(metrics.c0, &metrics.c0);
  VecAdd(gs->path.bbox.tr, delta, &metrics.c1);
  ITfmP(metrics.c1, &metrics.c1);
  tempcd = metrics.c0;
#if !PPS || PPSkanji
  if (hasCDevProc && PccCount == 0)
    ModifyCachingParams (fd,pcn,&metrics);
#endif
#if !PPS
  if (!SetCchDevice(&metrics,&delta))
#else
  if (!SetCchDevice(metrics.w0, metrics.c0, metrics.c1, &delta))
#endif
    return;
  if ((metrics.c0.x != tempcd.x) || (metrics.c0.y != tempcd.y)) {
    VecSub (tempcd, metrics.c0, &tempcd);
    DTfmP (tempcd, &tempcd);
    VecSub (delta, tempcd, &delta);
    }
  TlatPath(&gs->path, delta);
  DictGetP(fd, pnttypnm, &obj); /* paint type */
  switch (obj.val.ival)
    {
    case 0: 
      if (erosion < FixedHalf || toosmall ||
          gsmatrix->len1000 > FixInt(OFLIMIT))
        Fill(&gs->path, false);
      else OffsetFill(&gs->path, info_offsetval, info_fooFactor);
      FrPth(&gs->path);
      break;
    case 1:
    case 2:
      gs->lineJoin = 0; /* mitre */
      fixtopflt(info_fooFactor, &strkFoo);
      Stroke(&gs->path);
      strkFoo = oldStrkFoo;
      /* fall through */
    case 3: break;
    default: InvlFont();
    }
  }

#if !PPS
private boolean SetupCacheDevice(fm) FMetrics *fm; {
#if (! DPS_CODE)
  if (hasCDevProc && PccCount == 0) 
    return MakeCacheDev2(fm);
  else
#endif /* ! DPS_CODE */
    return MakeCacheDev(fm);
  }
#endif
     
private procedure CSEndChar(fd, pcn, fm)
  DictObj fd; 
  Object *pcn; 
  FMetrics *fm; 
  {
   static IntX	callTry = 0;

#if PPS
   FCd  delta;
   if (!FSetCchDevice(fm, &delta))
     return;
   CScan(CSRun, delta);
#else				/* PPS */
   if (setupCache && !SetupCacheDevice(fm))
     return;
   setupCache = false;
   CScan(CSRun);
#endif				/* PPS */
  }

#define GetDecrypt\
  {ec = isstring ? *sp++ : getc(sh);\
   if (lenIV < 0) c = ec; else {Decrypt(rndnum, c, ec);}}

#define GetStdByte\
  {c = isstring ? *sp++ : getc(sh);}

typedef struct{
        longcardinal seed;
        string stp;
        } SubrFrame;

private PCCInfo CCRunStd(fd,pcn,s,privdict)
        DictObj fd; Object *pcn, s; DictObj privdict;
{
register string sp = NULL; register integer /*character*/ c, ec;
register PFCd p;
FCd cp, delta, cs, sb, tempfcd, dcp;
FMetrics fMetrics;
Object obj, tempobj;
PCCInfo pcc;
#if GLOBALCOLORING
FCd prevcp, initcp;
Fixed ef;
#endif /* GLOBALCOLORING */
register PFixed pp;
register Fixed ftmp;
register /*short*/ integer buf;
IntX i, gotmetrics, lenIV, indx, paintType, countTry = 0;
Card16 rndnum;
boolean isstring = false, cleanup, arcdirbool, useCScan;
Fixed bluefuzz = 0, ftmp2, ftmp3;
Stm sh = 0; 
FCd lockdxy;
Fixed swval, sbx, sby;
real oldStrkFoo;
IntX subindex, lenTopBands, lenBotBands;
struct baseinfo {boolean isstr; union {string sp; Stm sh;} src;} 
          base;
RCd rtmpcd;
real r1, r2, r3;
Fixed botBands[MAXBLUES], topBands[MAXBLUES];
FCd botLocs[MAXBLUES], topLocs[MAXBLUES], pstk[MAXCDs];
Fixed wv[MAXWEIGHTS];
IntX wvlen;
SubrFrame substck[MAXSUB];
#define PSSTACKSIZE (3)
Fixed psstack[PSSTACKSIZE];
#define MAXFLEX (8)
FCd flexCds[MAXFLEX];
IntX psstackcnt, flexIndx = 0, popcnt;
Fixed dmin;
boolean dooslock, integerdividend, doingFlex, closed, gotMetrics2, usePath = false;
procedure (*flexproc)();

oldStrkFoo = strkFoo;

gsfactor = 1; /* UNTIL WE DECIDE HOW TO USE THIS IN PS */

GSave();

pcc = NIL;
p = pstk;

fMetrics.v.x = 0;
fMetrics.v.y = 0;

r1 = os_fabs(ss->rdcp.x);
r2 = os_fabs(ss->rdcp.y);
if (r1 < r2) r1 = r2;
usePath = ((r1 + UPPERSHOWBOUND) >= 32000.0);

setupCache = true;
  
retryWithPath: /* retry if limitcheck in non-path case */

DURING
  {
  /* verify dictionary integrity */
  if (!ForceKnown(privdict,scrtnm)) InvlFont();
  ForceGetP(privdict, scrtnm, &obj);
  if (obj.val.ival != 5839) InvlFont();
#if !PPS || PPSkanji
  hasCDevProc = Known(fd, cdevprocname);
#endif
  
  GetMtxInfo();
  /* save locktype in internaldict */
  LIntObj(obj, locktype);
  DictPut(rootInternalDict, lNm, obj);

  /* get StrokeWidth, if there */
  if (Known(fd,strokewidthname)) {
    DictGetP(fd, strokewidthname, &obj);
    swval = FixedValue(&obj);}
  else swval = 0;

  /* get metrics info */
  GetMetrics(fd,pcn,&gotmetrics,&cs,&fMetrics.w0);
#if !PPS || PPSkanji
  gotMetrics2 = GetMetrics2 (fd, pcn, &fMetrics);
#endif
  cp = cs;
  sbx = cs.x; sby = cs.y;

  wvlen = GetWeightVector(fd, wv);

  /* see if we are outlining a font designed for fill.  changes locking */
  DictGetP(fd, pnttypnm, &obj); /* paint type */
  paintType = obj.val.ival;
  isoutline = (boolean)(paintType == 2);

  /* get standard horizontal and vertical stem widths */
  GetStandardWidths(privdict, GetStemWidth(privdict));
  
  /* get TopBands and BotBands */
  if (locktype != 0) {
    GetBlueValues(privdict,botBands,&lenBotBands,topBands,&lenTopBands);
    FudgeBlueBands(privdict,botBands,lenBotBands,topBands,lenTopBands,swval);
    SetupBlueLocs(botBands,lenBotBands,topBands,lenTopBands,botLocs,topLocs);
    FamilyBlueLocs(privdict,botLocs,lenBotBands,topLocs,lenTopBands,
            usefix && (lenstdhw > 0));
    BoostBotLocs(botLocs,lenBotBands);
#if FBDEBUG
    printf("blue locs\n");
    PrintBlueLocs(botLocs, lenBotBands, topLocs, lenTopBands);
#endif
    }

  /* get Private dict parameters */
  if (locktype != 0) {
    bluefuzz = GetFromPrivDict(privdict, blfuzznm, FixedOne);
    blueShiftStart = GetFromPrivDict(privdict, blueShiftName, FixInt(7));
    blueShiftStart = FTruncF(blueShiftStart + (FixedOne - 1)); /* use ceil */
    blueScale = GetFromPrivDict(privdict, blueScaleName, BLUESCALE);
    CheckBlueScale(botBands, lenBotBands, topBands, lenTopBands);
    }

  if (ForceKnown(privdict, lenIVname)) {
    ForceGetP(privdict, lenIVname, &obj); lenIV = obj.val.ival;}
  else lenIV = 4;
  
  /* compute erosion parameters  */
  GetErosion(privdict,swval,&info_offsetval,&info_fooFactor);

  doFixupMap = locktype != 0 &&
    gsmatrix->len1000 > 0x68000L && /* turn it on at 6.5 */
    gsmatrix->len1000 < 0x118000L; /* turn it off at 17.5 */

#if GLOBALCOLORING
  glcrOn = false; /* need to do this before call StartLock */
#endif /* GLOBALCOLORING */  
  StartLock();

  pLokData->fixmapok = true;
  if (locktype != 0 && gsmatrix->len1000 > 0x68000L &&
      gsmatrix->len1000 < 0x118000L) {
    delta.x = delta.y = 0;
    if (locktype == -1) delta.x = FixedOne;
    else delta.y = FixedOne;
    FntIDTfmP(delta, &delta);
    pLokData->Y.unitDist = os_labs(delta.y);
    delta.x = delta.y = 0;
    if (locktype == -1) delta.y = FixedOne;
    else delta.x = FixedOne;
    FntIDTfmP(delta, &delta);
    pLokData->X.unitDist= os_labs(delta.x);
    }

#if GLOBALCOLORING

  glcrFailure = false;
  /* If rdnstemupname in private dict then global coloring is turned on */
  if (ForceKnown(privdict, rndstmname)) {
    ForceGetP(privdict, rndstmname, &obj);
    glcrOn = true;
    }
  else {
    glcrOn = (lenBotBands < 2 || botBands[1] < -5);}
  if (locktype == 0) glcrOn = false;
  ef = GetFromPrivDict(privdict, expfctrname, 3932);
    /* 0.06 in fixed point */

  glcr:

  if (glcrOn) {
    /* Initialize global coloring machinery: */
    glcrPrepass = true;
    IniGlbClrBuffs();
  } else
    glcrPrepass = false;

#endif /* GLOBALCOLORING */

  if (gsmatrix->len1000 < FixInt(34))
    fixtopflt(0x3000L, &gs->flatEps);
  else
    gs->flatEps = fpHalf;
  gs->dashArray = emptyarray;
  gs->miterlimit = (isoutline) ? fpTwo : 10.0;

  restart:  /* come back to here for global coloring second pass */

  pLokData->fixedmap = false;
  psstackcnt = popcnt = 0;
  p = pstk;
  doingFlex = false;
  closed = true;
  subindex = 0;
  dooffsetlock = false;
  lockoffset.x = lockoffset.y = 0;
  useCScan = false;
  if (!gs->isCharPath && paintType == 0 && usefix && !usePath &&
      erosion >= FixedHalf  && gsmatrix->len1000 < FixInt(CSLIMIT)) {
    Fixed	w, sumcnt;
    rndwidth = 0; idealwidth = 0; sumcnt = 0;
    if (lenstdvw > 0) {
      w = FRoundF(stdvw[0]); if (w == 0) w = FixedOne;
      rndwidth += w; idealwidth += stdvw[0]; sumcnt++;
      }
    if (lenstdhw > 0) {
      w = FRoundF(stdhw[0]); if (w == 0) w = FixedOne;
      rndwidth += w; idealwidth += stdhw[0]; sumcnt++;
      }
    if (sumcnt == 2) {
      rndwidth >>= 1; idealwidth >>= 1; }
    useCScan = true;
    erosion = FixedOne;

    ResetCScan(true, locktype != 0, gsmatrix->len1000,
               rndwidth, idealwidth, gsfactor);
    moveto = CSNewPoint;
    lineto = CSNewPoint;
    curveto = FFCurveTo;
    closepath = CSClose;
    endchar = CSEndChar;
    }
  else {
    usePath = true;
    endchar = PathEndChar;
    }
    
  if (!usefix) 
    flexproc = RFlexProc;
  else if (gs->isCharPath)
    flexproc = FlexProc2;
  else
    flexproc = FlexProc;

  info_devsw = NormalizeSW(swval); /* gs->lineWidth = swval; sort of */
  devsweven = (FRound(info_devsw) & 01) == 0;

  NewPath();
  gs->path.secret = secret;

  switch (s.type)
    {
    case strObj:
#if PPS
      sp = VMStrPtr(s);
#else
      sp = s.val.strval;
#endif
      isstring = true;
      break;
    case stmObj:
      sh = GetStream(s);
      isstring = false;
      break;
    default: CantHappen();
    }

  integerdividend = false;
  rndnum = FONTKEY;
  for (i = lenIV; --i >= 0; ) {GetDecrypt;}
  pp = (PFixed)p;
  while (true)
    {
    GetDecrypt;
    opswitch: /* branch back here from CRYesc and CRBesc */
    switch ((int)c)
      { /* 0..31 are opcodes */
      case CBcmd: /* courier blue */
        ftmp = RoundSW(swval);
        p[0].x -= (ftmp >> 1);
        p[0].y = ftmp;
        /* fall through to normal RBcmd */
      case RBcmd: /* must immediately follow CBcmd */  /* <y> <dy> */
        RBLock(p,sby,botBands,lenBotBands,topBands,lenTopBands,
          botLocs,topLocs,bluefuzz);
        break;
      case CYcmd:  /* courier yellow */
        ftmp = RoundSW(swval);
        p[0].x -= (ftmp >> 1);
        p[0].y = ftmp;
        /* fall through to normal RYcmd */
      case RYcmd:  /* must immediately follow CYcmd! */  /* <x> <dx> */
        RYLock(p,sbx);
        break;
      case VMTcmd: /* <dy> */
        cp.y += p[0].x;
       MoveToCmd:
	(*tfmLockPt)(&cp, &tempfcd);
#if GLOBALCOLORING
        initcp = prevcp = cp;
#endif /* GLOBALCOLORING */
	if (doingFlex && flexIndx < MAXFLEX) {
	  if (usefix)
            FntITfmP(tempfcd, &flexCds[flexIndx]);
	  else
	    ITfmP(tempfcd, &flexCds[flexIndx]);
	  flexIndx++;
          }
        else  {
          dcp = tempfcd;
#if GLOBALCOLORING
          if (!glcrOn || !glcrPrepass)
#endif /* GLOBALCOLORING */
            {
            if (!closed && paintType != 3) (*closepath)();
            (*moveto)(&dcp);
	    closed = false;
	    }
	  }
        break;
      case RDTcmd:  /* <dx> <dy> */
        cp.x += p[0].x;  cp.y += p[0].y;
       LineToCmd:
#if GLOBALCOLORING
        if (glcrOn && glcrPrepass) {
          GlbClrLine(prevcp, cp);
	  dcp = prevcp = cp; }
        else
#endif /* GLOBALCOLORING */
          { (*tfmLockPt)(&cp, &dcp);
            (*lineto)(&dcp); }
        break;
      case HDTcmd:  /* <dx> */
        cp.x += p[0].x;
        goto LineToCmd;
      case VDTcmd:  /* <dy> */
        cp.y += p[0].x;
        goto LineToCmd;
      case RCTcmd: /* <dx1><dy1><dx2><dy2><dx3><dy3> */
        p[0].x += cp.x;    p[0].y += cp.y;
        p[1].x += p[0].x;  p[1].y += p[0].y;
        p[2].x += p[1].x;  p[2].y += p[1].y;
        goto CurveToCmd;
      case CPcmd:
#if GLOBALCOLORING
        if (glcrOn && glcrPrepass)
          GlbClrLine(cp, initcp);
        else
#endif /* GLOBALCOLORING */
          { if (!closed) (*closepath)(); closed = true; }
        break;
      case DOSUBcmd:  /* <sub number> */
        if (subindex == MAXSUB) { InvlFont(); goto donewithchar; }
        i = FTrunc(*--pp);
        if (subindex == 0)
          {/* first dosub call */
	  base.isstr = isstring;
          if (isstring) base.src.sp = sp;
          else base.src.sh = sh;
          }
        substck[subindex].seed = rndnum; /* stack the seed */
        substck[subindex++].stp = sp; /* may be garbage */
        ForceGetP(privdict, subrsnm, &obj);
        ForceAGetP(obj, (cardinal)i, &obj);
#if PPS
        sp = VMStrPtr(obj);
#else
        sp = obj.val.strval;
#endif
        isstring = true;
        rndnum = FONTKEY;
        for (i = lenIV; --i >= 0; ) {GetDecrypt;}
        continue;
      case RETcmd:  /* return from dosub */
        if (subindex == 0) { InvlFont(); goto donewithchar; }
        rndnum = substck[--subindex].seed;
        sp = substck[subindex].stp;
        if (subindex == 0)
          {
          isstring = base.isstr;
          if (isstring) sp = base.src.sp;
          else sh = base.src.sh;
          }
        continue;
      case ESCcmd:  /* esc op comes in next byte */
        GetDecrypt;
        switch ((int)c)
          {
          case FLesc: /* flip type of locking */
#if DO_OFFSET
            dooffsetlock = !dooffsetlock;
            if (dooffsetlock && locktype != 0)
              {
              tempfcd.x = cp.x; tempfcd.y = cp.y;
              LockPFCd(&tempfcd, &tempfcd);
              lockoffset.x = tempfcd.x - cp.x;
              lockoffset.y = tempfcd.y - cp.y;
              }
            else {lockoffset.x = lockoffset.y = 0;} /* to be safe */
	    if (!usefix)		tfmLockPt = TfmLockPt4;
	    else if (locktype == 0)	tfmLockPt = TfmLockPt3;
	    else if (!dooffsetlock)	tfmLockPt = TfmLockPt1;
	    else 			tfmLockPt = TfmLockPt2;
#endif /* DO_OFFSET */
            break;
          case RMesc:
            RMLock(p, sbx);
            break;
          case RVesc:  /* rv locking */
            RVLock(p, sby);
            break;
          case FIesc:
	    if (toosmall) Fill(&gs->path, false);
#if DPSXA
            else XAOFill(&gs->path, info_offsetval, info_fooFactor);                         
#else /* DPSXA */
            else OffsetFill(&gs->path, info_offsetval, info_fooFactor);
#endif /* DPSXA */ 
            FrPth(&gs->path);
            break;
          case ARCesc:  /* cx cy r a1 a2 */
            arcdirbool = true;
            goto ARCcommon;
          case SLWesc:
            NormalizeSW(p[0].x); 
            break;
          case CCesc:
#if !PPS || PPSkanji
            if (!gotMetrics2)
              fMetrics.w1 = fMetrics.w0;
#endif
            pcc = CCBuild(pcn,p,fd,sb.x,sbx,sby,swval,&fMetrics,gotmetrics);
	    if (pcc == NIL) GRstr();
            goto ccexit;
          case SBesc:  /* <sbx> <sby> <widx> <widy> */
            if (gotmetrics <= 1)
              {sbx = cp.x = p[0].x; sby = cp.y = p[0].y;}
            if (gotmetrics == 0)
              {fMetrics.w0.x = p[1].x; fMetrics.w0.y = p[1].y;}
            sb.x = p[0].x; sb.y = p[0].y;
#if GLOBALCOLORING 
            if (glcrOn && glcrPrepass) break;
#endif /* GLOBALCOLORING */ 
#if CHECKOKBUILD
	    if (!CheckIfOkToBuild(fd,swval,fMetrics.w0))
              goto donewithchar;
#endif /* CHECKOKBUILD */
            break;
          case SSPCesc:  /* <llx><lly><urx><ury> */
            {
            RMetrics rm;
            ftmp = swval + swval;
            fMetrics.c0.x = p[0].x - ftmp + sbx;
            fMetrics.c0.y = p[0].y - ftmp;
            fMetrics.c1.x = p[1].x + ftmp + sbx;
            fMetrics.c1.y = p[1].y + ftmp;
#if !PPS || PPSkanji
            if (!gotMetrics2)
              fMetrics.w1 = fMetrics.w0;
#endif
	    F2RMetrics(&fMetrics, &rm);
#if !PPS || PPSkanji 
	    if (hasCDevProc && PccCount == 0)
	      ModifyCachingParams (fd,pcn,&rm);
#endif
#if CHECKOKBUILD
	    if (!OkToBuild(&rm))
	      goto donewithchar;
#endif /* CHECKOKBUILD */
#if PPS
            if (!SetCchDevice(rm.w0, rm.c0, rm.c1, &delta))
#else
	    if (!SetCchDevice(&rm,&delta))
#endif
	      goto donewithchar;
	    if ((usefix = SetupFntMtx())) /* gs->matrix has changed */
	      SetFixedPathProcs();
	    else
	      SetRealPathProcs();
	    GetMtxInfo(); /* gs->matrix has changed */
            break;
            }
          case ESPCesc: /* end self-painting char */
            goto donewithchar;
          case ADDesc:
            ftmp = *--pp;
            *(pp-1) += ftmp;
            continue;
          case SUBesc:
            ftmp = *--pp;
            *(pp-1) -= ftmp;
            continue;
          case DIVesc:
            ftmp = *--pp;
            if (integerdividend) {ftmp = FTrunc(ftmp); integerdividend =false;}
            *(pp-1) = fixdiv(*(pp-1), ftmp);
            continue;
          case MAXesc:
            ftmp = *--pp;
            ftmp2 = *(pp-1);
            *(pp-1) = MAX(ftmp,ftmp2);
            continue;
          case NEGesc:
            *(pp-1) = -*(pp-1);
            continue;
          case IFGTADDesc:
            ftmp = *--pp;
            ftmp2 = *--pp;
            ftmp3 = *--pp;
            if (ftmp3 > ftmp2) *(pp-1) += ftmp;
            continue;
          case DOesc:  /* <a1>...<ai><i><index> */
            indx = FTrunc(*--pp);
	    i = FTrunc(*--pp);
	    if (paintType != 3) {
	      switch ((int)indx) {
		case 0: /* flex proc */
		  if (flexIndx != MAXFLEX-1) { InvlFont(); goto donewithchar; }
		  if (i != 3) { InvlFont(); goto donewithchar; }
		  tempfcd.y = *--pp + (sby - sb.y);
		  tempfcd.x = *--pp + (sbx - sb.x);
		  /* sbx is the x sidebearing */
		  /* sb.x is the default x sidebearing */
		  dmin = *--pp;
		  flexCds[MAXFLEX-1] = tempfcd;
#if GLOBALCOLORING
		  if (!glcrOn || !glcrPrepass)
#endif /* GLOBALCOLORING */
		    {
		    (*flexproc)(flexCds, dmin, &dcp);
		    tempfcd = flexCds[0];
		    }
		  psstack[psstackcnt] = tempfcd.y; psstackcnt++;
		  psstack[psstackcnt] = tempfcd.x; psstackcnt++;
		  doingFlex = false;
		  break;
		case 1: /* preflex1 proc */
		  doingFlex = true;
		  flexIndx = 0;
		  break;
		case 2: /* preflex2 proc */
		  break;
		case 3: /* Multi-coloring proc */
		  if (i != 1) { InvlFont(); goto donewithchar; }
		  StartLock();
		  psstack[psstackcnt] = *--pp;
		  psstackcnt++;
                  if (pLokData->fixedmap)
                    goto restart; /* cannot do fixupmap with multicoloring */
		  break;
		case 4:
		  /* Ignore check for pgfont id */
		  if (i != 1)
		    InvlFont();
		  pp--;
		  psstack[psstackcnt] = FixInt(3);
		  psstackcnt++;
		  continue;
#if GLOBALCOLORING
		case 6:
		  /* turn on global coloring for this character */
                  if (i != 0) InvlFont();
		  if (locktype == 0 || glcrOn || glcrFailure) continue;
		  glcrOn = true;
		  goto glcr;
#endif /* GLOBALCOLORING */
                case 11: indx++;
                case 7: case 8: case 9: case 10:
                  popcnt = indx-6; /* number of results */
                  pp = DoBlend(pp, wv, wvlen, popcnt);
                  break;
		default:
                  goto dosub;
		}
	      }
	    else {
              dosub:
	      ForceGetP(privdict, ocsbrsnm, &obj);
	      ForceAGetP(obj, (cardinal)indx, &obj);
	      while (--i >= 0) {--pp;  PushFixed(*pp);}
	      if (pp < (PFixed)p) InvlFont();
	      Begin(rootSysDict); Begin(fd);
	      dooslock = dooffsetlock;  lockdxy = lockoffset;
#if !PPS
	      if (fontSemaphore != NIL)
		(*fontSemaphore)(1);
#endif
	      cleanup = psExecute(obj);
#if !PPS
	      if (fontSemaphore != NIL)
		(*fontSemaphore)(-1);
#endif
	      dooffsetlock = dooslock;  lockoffset = lockdxy;
	      End(); End();
	      if (cleanup) RAISE((int)GetAbort(), (char *)NIL);
	      }
            continue;
          case POPesc: /* pop number from PS stack to internal stack */
            if (popcnt > 0) popcnt--;
	    else {
              if (psstackcnt <= 0) *pp++ = PopFixed();
	      else { psstackcnt--; *pp++ = psstack[psstackcnt]; }
	      }
            continue; /* loop without resetting pp */
          case DSCNDesc:
            tempobj = dscndname; goto GetConstCmd;
          case ASCNDesc:
            tempobj = ascndname; goto GetConstCmd;
          case OVRSHTesc:
            tempobj = ovrshtname; goto GetConstCmd;
          case SLJesc:
            gs->lineJoin = FTrunc(p[0].x);
            break;
          case XOVResc:
            tempobj = xovrname; goto GetConstCmd;
          case CAPOVResc:
            tempobj = capovrname; goto GetConstCmd;
          case AOVResc:
            tempobj = aovrname; goto GetConstCmd;
          case HLFSWesc:
            tempobj = hlfswname; goto GetConstCmd;
          case RNDSWesc:
            *(pp-1) = RoundSW(*(pp-1));
            continue;
          case ARCNesc: /* cx cy r a1 a2 */
            arcdirbool = false;
          ARCcommon:
            p[0].x += sbx;
	    if (locktype != 0) {
              if (dooffsetlock) {
                p[0].x += lockoffset.x;  p[0].y += lockoffset.y;}
              else LockPFCd(&p[0], &p[0]);
	      }
            PFCdToPRCd(&p[0], &rtmpcd);
            fixtopflt(p[1].x, &r1);
            fixtopflt(p[1].y, &r2);
            fixtopflt(p[2].x, &r3); 
            Arc(rtmpcd, r1, r2, r3, arcdirbool, &gs->path);
            break; 
          case EXCHesc:  /* exchange top two stack entries */
            ftmp = *(pp-2);
            *(pp-2) = *(pp-1);
            *(pp-1) = ftmp;
            continue;
          case INDXesc:
             i = FTrunc(*(pp-1));
             *(pp-1) = *(pp-i-2);
             continue;
          case CRBesc:  /* <y> <dy> */
            if (p[0].y < 0) { p[0].y = -p[0].y; p[0].x -= p[0].y; }
            ftmp = RoundSW(p[0].y);
            p[0].x -= (ftmp-p[0].y) >> 1;
            p[0].y = ftmp;
            c = RBcmd;
            goto opswitch;
          case CRYesc:  /* <x> <dx> */
            if (RealLt0(p[0].y)) { p[0].y = -p[0].y; p[0].x -= p[0].y; }
            ftmp = RoundSW(p[0].y);
            p[0].x -= (ftmp-p[0].y) >> 1;
            p[0].y = ftmp;
            c = RYcmd;
            goto opswitch;
          case PUSHCPesc:
            *pp++ = cp.x;
            *pp++ = cp.y;
            break;
          case POPCPesc:
            cp.y = *--pp;
            cp.x = *--pp;
            break;
          default: { InvlFont(); goto donewithchar; }
          }
        break;
      case SBXcmd: /* <sbx> <widx> */
        if (gotmetrics <= 1)
          {/* no override use this sb */
          sbx = cp.x = p[0].x; sby = cp.y = 0;
          }
        if (gotmetrics == 0)
          {/* no override, use this width */
          fMetrics.w0.x = p[0].y; fMetrics.w0.y = 0;
          }
        sb.x = p[0].x; sb.y = 0;
#if GLOBALCOLORING 
        if (glcrOn && glcrPrepass) break;
#endif /* GLOBALCOLORING */
#if CHECKOKBUILD
        if (!CheckIfOkToBuild(fd,swval,fMetrics.w0))
          goto donewithchar;
#endif /* CHECKOKBUILD */
        break;
      case EDcmd:  /* end char -- do the dirty work */
#if GLOBALCOLORING
        if (glcrOn) {
          if (glcrPrepass) {
            glcrPrepass = false;
            AdjustToStdWidths();
	    ProcessGlbClrs(
              botBands,lenBotBands,topBands,lenTopBands,bluefuzz,
              &gblClrBuf3,botLocs,topLocs,ef);
            if (glcrFailure) {glcrOn /* = glcrPrepass */ = false;}
	    goto restart;
	    }
          }
#endif /* GLOBALCOLORING */
        if (!closed && paintType != 3)
          (*closepath)();
        if (!gotMetrics2)
          fMetrics.w1 = fMetrics.w0;
        (*endchar)(fd, pcn, &fMetrics);
        goto donewithchar;
      case MTcmd:
        cp = p[0]; cp.x += sbx;
        goto MoveToCmd;
      case DTcmd:
        cp = p[0]; cp.x += sbx;
        goto LineToCmd;
      case CTcmd:
        p[0].x += sbx; p[1].x += sbx; p[2].x += sbx;
       CurveToCmd:
        cp = p[2];
#if GLOBALCOLORING
        if (glcrOn && glcrPrepass) {
          GlbClrLine(prevcp, p[0]);
          GlbClrLine(p[1], p[2]);
	  prevcp = cp; }
        else 
#endif /* GLOBALCOLORING */
        {
        (*tfmLockPt)(&p[0], &p[0]);
	(*tfmLockPt)(&p[1], &p[1]);
	(*tfmLockPt)(&p[2], &p[2]);
        (*curveto)(&dcp, &p[0], &p[1], &p[2]);
	}
	dcp = p[2];
        break;
      case MINcmd:
        ftmp = *--pp; ftmp2 = *(pp-1); *(pp-1) = MIN(ftmp,ftmp2);
        continue;
      case STcmd:
        fixtopflt(info_fooFactor, &strkFoo);
	Stroke(&gs->path);
	strkFoo = oldStrkFoo;
        break;
      case NPcmd:
        NewPath();
        gs->path.secret = secret;
        break;
      case RMTcmd: /* <dx> <dy> */
        cp.x += p[0].x;  cp.y += p[0].y;
        goto MoveToCmd;
      case HMTcmd: /* <dx> */
        cp.x += p[0].x;
        goto MoveToCmd;
      case SLCcmd:
        gs->lineCap = FTrunc(p[0].x);
        break;
      case MULcmd:
        ftmp = *--pp; *(pp-1) = fixmul(*(pp-1), ftmp);
        continue;
      case STKWDTHcmd:
        tempobj = stkwdthname;
       GetConstCmd:
        ForceGetP(privdict, tempobj, &tempobj);
	*(pp++) = FixedValue(&tempobj);
        continue;
      case BSLNcmd:
        tempobj = bslnname; goto GetConstCmd;
      case CPHGHTcmd:
        tempobj = cphghtname; goto GetConstCmd;
      case BOVRcmd:
        tempobj = bovrname; goto GetConstCmd;
      case XHGHTcmd:
        tempobj = xhghtname; goto GetConstCmd;
      case VHCTcmd:  /* <dy1><dx2><dy2><dx3> */
        p[2].x = p[1].y; p[2].y = 0;
        p[1].y = p[1].x; p[1].x = p[0].y;
        p[0].y = p[0].x; p[0].x = 0;
        c = RCTcmd;
        goto opswitch;
      case HVCTcmd:  /* <dx1><dx2><dy2><dy3> */
        p[2].y = p[1].y; p[2].x = 0;
        p[1].y = p[1].x; p[1].x = p[0].y;
        p[0].y = 0;
        c = RCTcmd;
        goto opswitch;
      default: /* undef and numbers */
        if ((32 <= c) && (c <= 246))
          /* [-107..107] */
          *pp++ = FixInt(c-139);
        else if ((247 <= c) && (c <= 250))
          {/* [108..1131] */
          buf = (c-247) << 8;
          GetDecrypt; buf |= c;
          *pp++ = FixInt(buf + 108);
          }
        else if ((251 <= c) && (c <= 254))
          {/* [-1131..-108] */
          buf = (c-251) << 8;
          GetDecrypt; buf |= c;
          *pp++ = FixInt(-(buf + 108));
          }
        else if (c == BIGNUM)
          {/* four-byte numbers */
          GetDecrypt; buf = c;
          GetDecrypt; buf = (buf << 8) | c;
          GetDecrypt; buf = (buf << 8) | c;
          GetDecrypt; buf = (buf << 8) | c;
          if (buf >= -32000 && buf <= 32000) *pp++ = FixInt(buf);
          else {*pp++ = buf;  integerdividend = true;}
          }
        else
          CantHappen();
        continue; /* loop without resetting pp */
      } /* switch */
    pp = (integer *)p;
    } /* while */
  donewithchar: /* for EDcmd and ESPCesc */
  GRstr();
  ccexit: ; /* for CCesc */
  } /* end of DURING statement */
HANDLER
  {
  FinishLock();
  strkFoo = oldStrkFoo;
#if PPS || !MERCURY
  if (Exception.Code == PS_ERROR && !usePath && countTry++ < CSCANRETRIES) {
     ResizeCrossBuf();
     SetAbort(0);
     goto retryWithPath;
    }
  else if (Exception.Code == PS_ERROR && !usePath)
#else
  if (Exception.Code == ecLimitCheck && !usePath && 
      countTry++ < CSCANRETRIES) {
     ResizeCrossBuf();
     SetAbort(0);
     goto retryWithPath;
    }
  else if (Exception.Code == ecLimitCheck && !usePath)
#endif
    {
    usePath = 1;
    SetAbort(0);
    goto retryWithPath;
    }
  GRstr();
  RERAISE;
  }
END_HANDLER;
FinishLock();
return pcc;
}  /* end of CCRunStd */

private procedure CCRun(fd, pcn, s, privdict)
  DictObj fd; Object *pcn, s; DictObj privdict; {
PCCInfo pcc;
pcc = CCRunStd(fd, pcn, s, privdict);
if (pcc == NIL) return;
DURING
  {
  RCd temp;  Object ob;
  GSave();
  DictGetP(fd, matrixname, &ob);
  PAryToMtx(&ob, &pcc->fmtx);
  MtxInvert(&pcc->fmtx,&pcc->imtx); /* inverse of font matrix */
  pcc->fmtx.tx = fpZero; pcc->fmtx.ty = fpZero; /* get rid of translate */
  MtxInvert(&pcc->fmtx,&pcc->itmtx);
  temp.x = pcc->adx; temp.y = pcc->ady;              
  Cnct(&pcc->imtx); Cnct(&pcc->fmtx); /* translation-free font matrix */
  TfmP(temp, &temp);
  MoveTo(temp, &gs->path);
  Cnct(&pcc->itmtx);
  SimpleShowByName(pcc->acode, &pcc->acn);
  GRstr();
  GSave();
  temp.x = pcc->cdx; temp.y = pcc->cdy;
  Cnct(&pcc->imtx); Cnct(&pcc->fmtx);
  TfmP(temp, &temp);
  MoveTo(temp, &gs->path);
  Cnct(&pcc->itmtx);
  SimpleShowByName(pcc->ccode, &pcc->ccn);
  GRstr();
  GRstr(); /* this one balances the one done at start of CCRunStd */
  }
HANDLER
  {
  GRstr();
  FREE(pcc);
  PccCount--;
  RERAISE;
  }
END_HANDLER;
FREE(pcc);
PccCount--;
}  /* end of CCRun */

#if !PPS
#if (OS == os_vms)
globalref StmProcs strStmProcs;
#else /*(OS == os_vms)*/
extern readonly StmProcs strStmProcs;
#endif /*(OS == os_vms)*/

private procedure BMRun (fd, pcn, s, privdict)
  DictObj fd; Object *pcn, s; DictObj privdict; /* Don't use privdict */
{
  register integer c;
  register string sp = NULL;
  boolean isstring = false;
  FMetrics fMetrics;
  RMetrics rMetrics;
  Stm sh = 0;
  integer KByte, Rows, Columns, BaseLine, Top, Bottom, Left, Right, Length;
  integer gotmetrics, IW, IH;
  Component Dx, Ury;
  Mtx ImgMtx;
  Cd delta;
  StmBody stmBody;
  StmRec stmRec;
  Object source;

  GetMetrics (fd, pcn, &gotmetrics, &fMetrics.c0, &fMetrics.w0);
  if (!GetMetrics2(fd, pcn, &fMetrics)) {
    fMetrics.w1 = fMetrics.w0;
    fMetrics.v.x = fMetrics.v.y = 0;
    }
  F2RMetrics(&fMetrics, &rMetrics);
  switch (s.type)
  {
  case strObj:
    sp = s.val.strval;
    isstring = true;
    break;
  case stmObj:
    sh = GetStream (s);
    isstring = false;
    break;
  default:
    InvlFont ();
  }
  Length = 0;
  do {GetStdByte; Length += c;} while (c == 255);
  if (Length == 0) RangeCheck ();
  GetStdByte; KByte = c;
  switch (KByte)
  {
  case 1:
    delta.x = delta.y = 1000.0;
    Scal (delta);
    rMetrics.c0.x *= 0.001; rMetrics.c0.y *= 0.001;
    rMetrics.c1.x *= 0.001; rMetrics.c1.y *= 0.001;
    rMetrics.w0.x *= 0.001; rMetrics.w0.y *= 0.001;
    rMetrics.w1.x *= 0.001; rMetrics.w1.y *= 0.001;
    rMetrics.v.x *= 0.001; rMetrics.v.y *= 0.001;
  case 0:
    if (Length < 8) RangeCheck ();
    GetStdByte; Rows = c;
    GetStdByte; Columns = c;
    GetStdByte; BaseLine = c - 128;
    GetStdByte; Top = c - 128;
    GetStdByte; Bottom = c - 128;
    GetStdByte; Left = c - 128;
    GetStdByte; Right = c - 128;
    Dx = (real) Columns / (real) Rows;
    Ury = (real) BaseLine / (real) Rows;
    if (gotmetrics == 0) rMetrics.w0.x = Dx;
    if (gotmetrics <= 2) rMetrics.c0.y = Ury - 1.0;
    rMetrics.c1.x = Dx + rMetrics.c0.x;
    rMetrics.c1.y = 1.0 + rMetrics.c0.y;
    if (Known(fd, cdevprocname) && PccCount == 0)
      ModifyCachingParams (fd,pcn,&rMetrics);
    (void)SetCchDevice(&rMetrics, &delta);
    ImgMtx.a = Rows;
    ImgMtx.b = 0.0;
    ImgMtx.c = 0.0;
    ImgMtx.d = - Rows;
    ImgMtx.tx = - (rMetrics.c0.x * Rows + Left);
    ImgMtx.ty = (rMetrics.c0.y + 1.0) * Rows - Top;
    if (isstring) {
      stmRec.cnt = Length - 8;
      stmRec.ptr = (char *)sp;
      stmRec.base = (char *)sp;
      stmRec.procs = &strStmProcs;
      stmBody.link = (PStmBody)NULL;
      stmBody.stm = (Stm)&stmRec;
      stmBody.generation = 0;
      stmBody.level = 0;
      stmBody.shared = 0;
      stmBody.seen = 0;
      stmBody.unused = 0;
      LStmObj(source, 0, &stmBody);
      }
    else
      source = s;
    IW = Columns - Left - Right;
    IH = Rows - Top - Bottom;
    ImageInternal(
      IW, IH, 1L, &source, &ImgMtx, true, true, 1L,
      (integer)DEVGRAY_COLOR_SPACE, 1L, 0L, NIL);
    break;
  default:
    InvlFont ();
  }
}

private procedure CCBMRun(proc) PVoidProc proc; {
  DictObj fd;
  character c;
  Object s, ob;
  DictObj privdict;
  PopP(&s);
  if (s.type == dictObj) {
    privdict = s;
    PopP(&s);
    }
  else
    privdict.type = nullObj;
  c = (character)PopInteger();
  PopPDict(&fd);
  if (!fd.val.dictval->isfont)
    InvlFont ();
  DictGetP(fd, encname, &ob);
  AGetP(ob, (cardinal)c, &ob);
  if (privdict.type == nullObj)
    DictGetFontP(fd, prvtnm, &privdict);
  (*proc)(fd, &ob, s, privdict);
  }

  
#define C1o 16477
#define C2o 21483
#define keyo 54261

public procedure PSeCCRun() { 
StrObj cipherstr;
string ptr;
unsigned short int rnum;
unsigned char clear, cipher;
int i;
  if(gs->isCharPath)	/* fonts that use this routine want secret paths */
	secret = true;
  else
  	secret = false;
  PopPString(&cipherstr);
  ptr = (string) cipherstr.val.strval;
  rnum = keyo;
  for (i = cipherstr.length; i > 0; i--)
  { cipher = *ptr;
    clear = cipher^(rnum>>8);
    rnum = (cipher + rnum)*C1o + C2o;
    *ptr++ = clear;
  }
  PushP(&cipherstr);
  CCBMRun(CCRun);
  secret = false;
}

public procedure PSCCRun() {
  CCBMRun(CCRun);
  }  /* end of PSCCRun */

private procedure PSBMRun() {
  CCBMRun(BMRun);
  }  /* end of PSBMRun */

#else /* PPS */

private procedure PSCCRun()
{
DictObj fd, privdict;
character c;
Object s, ob;
PopP(&s);
c = (character)PopInteger();
PopPDict(&fd);
DictGetFontP(fd, encname, &ob); 
AGetP(ob, (cardinal)c, &ob);
DictGetFontP(fd, prvtnm, &privdict);
CCRun(fd, &ob, s, privdict);
}  /* end of psCCRun */

#endif /* PPS */

private boolean ChrMapBuildChar(f, c, pcn, ftype)
  DictObj f; integer c; Object *pcn; integer ftype; {
  StrObj chrdata, chroffsets;
  register string coffsets;
  DictObj charstrs, privdict;
  Object ob;
  register integer cs, pos;
  boolean IsCC, cleanup = false;
  DictGetFontP(f, charstringsname, &charstrs);
  if (!Known(charstrs, *pcn)) pcn = &notdefname;
  DictGetFontP(f, chardataname, &chrdata);
  DictGetFontP(f, charoffsetsname, &chroffsets);
#if PPS
  coffsets = VMStrPtr(chroffsets);
#else
  coffsets = chroffsets.val.strval;
#endif
 Lookup:
  DictGetFontP(charstrs, *pcn, &ob);
  switch (ob.type) {
    case intObj:
#if PPS
      cs = (ob.val.ival << 1);
      if (cs >= chroffsets.length) InvlFont();
      pos = (coffsets[cs] << 8) | coffsets[cs+1];
      if (pos <= 0) {
        if (Equal(*pcn, notdefname)) InvlFont();
        pcn = &notdefname;
        goto Lookup;
        }
      pos--;
      if (pos >= chrdata.length) InvlFont();
      SubPString(chrdata, pos, chrdata.length-pos, &ob);
      DictGetFontP(f, prvtnm, &privdict);
      CCRun(f, pcn, ob, privdict);
#else /*PPS*/
      if (ftype == CHRMAPtype) {
        cs = (ob.val.ival << 1);
        if (cs >= chroffsets.length) InvlFont();
        pos = (coffsets[cs] << 8) | coffsets[cs+1];
        IsCC = true;
        }
      else {
        cs = (ob.val.ival * 3);
        if (cs >= chroffsets.length-1) InvlFont();
        pos = (coffsets[cs] << 16) | (coffsets[cs+1] << 8) | coffsets[cs+2];
        IsCC = (pos & 0x800000) == 0;
        pos &= 0x3fffff;
        }
      if (pos == 0) {
        if (Equal(*pcn, fontsNames[nm_notdef])) InvlFont();
        pcn = &fontsNames[nm_notdef];
        goto Lookup;
        }
      pos--;
      if (ftype != CHRMAPtype) {
        if (pos >= chrdata.length) {
          pos -= chrdata.length;
          DictGetFontP(f, fontsNames[nm_CharData1], &chrdata);
          }
        }
      if (pos >= chrdata.length) InvlFont();
      SubPString(chrdata, (cardinal)pos, (cardinal)chrdata.length-pos, &ob);
      if (IsCC) {
        DictGetFontP(f, fontsNames[nm_Private], &privdict);
        CCRun (f, pcn, ob, privdict);
        }
      else {
        privdict.type = nullObj; /* does not use privdict */
        BMRun (f, pcn, ob, privdict);
	}
#endif
      break;
    case arrayObj:
    case pkdaryObj:
      if (c < 0) InvlFont();
      Begin(rootSysDict); Begin(f);
      PushInteger((integer)c);
#if !PPS
      if (fontSemaphore != NIL)
        (*fontSemaphore)(1);
#endif
      cleanup = psExecute(ob);
#if !PPS
      if (fontSemaphore != NIL)
        (*fontSemaphore)(-1);
#endif
      End(); End();
      break;
    default:
      InvlFont();
    }
  return cleanup;
  }

private boolean InternalBuildChar(f, c, pcn, ftype)
  DictObj f; integer c; Object *pcn; integer ftype;
{
DictObj charstrs, privdict; Object pathops; StrObj chroffsets;
boolean cleanup = false;

if (ftype == ENCRPTVMtype)
  {
  DictGetFontP(f, prvtnm, &privdict);
  DictGetP(f, charstringsname, &charstrs);
  if (!Known(charstrs, *pcn)) pcn = &notdefname;
  DictGetP(charstrs, *pcn, &pathops);
  switch (pathops.type)
    {
    case strObj:
      CCRun(f, pcn, pathops, privdict);
      break;
    case pkdaryObj:
    case arrayObj: /* new escape for redefining characters */
      if (c < 0) InvlFont();
      Begin(rootSysDict); Begin(f);
      PushInteger((integer)c);
#if !PPS
      if (fontSemaphore != NIL)
        (*fontSemaphore)(1);
#endif
      cleanup = psExecute(pathops);
#if !PPS
      if (fontSemaphore != NIL)
        (*fontSemaphore)(-1);
#endif
      End(); End();
      break;
#if !PPS
    case intObj: /* for disk-based treatment of type 1 fonts */
      if (!Known(f, fontsNames[nm_CharOffsets]) || (fetchCharOutline == NULL))
	InvlFont();
      DictGetFontP(f, fontsNames[nm_CharOffsets], &chroffsets);
      (*fetchCharOutline)(chroffsets, pathops.val.ival, &pathops);
      CCRun(f, pcn, pathops, privdict);
      break;
#endif
    default: InvlFont();
    }
  }
else
  {
  CantHappen();
  }
return cleanup;
}  /* end of InternalBuildChar */

public boolean CharStringsVal(fd,pn,pi)
  DictObj fd; PNameObj pn; integer *pi;
{
  DictObj cs; IntObj obj;
  DictGetP(fd,charstringsname,&cs);
  if (! Known(cs,*pn)) return false;
  DictGetP(cs,*pn,&obj);
  *pi = obj.val.ival;
  return true;
}

public procedure PSIntDict()        /* internaldict */
{
Object ob;
if (opStk->head != NIL)
  {
  PopP(&ob);
  if (ob.type == intObj &&
#if PPS
    (ob.val.ival ^ KEYHASH) == VMArrayPtr(root->param)[rpInternalKey].val.ival)
#else
    ob.val.ival == INTERNALKEY)
#endif
    {PushP(&rootInternalDict); return;}
  }
InvlAccess();
}  /* end of PSIntDict */


#if PPS
#define InitRnum(r,s) {r = s;}
#define Rnum8(r)\
  ((r = (((r*C1)+C2) & 0x3fffffff)>>8) & 0xFF)
#define Encrypt(r, clear, cipher)\
  r = ((cipher = ((clear)^(r>>8)) & 0xFF) + r)*C1 + C2

#define lenIV 4			/* length of initialization vector */

#define OutputEnc(ch, stm)\
 {if (charsInLine >= 64) {putc('\n',stm); charsInLine = 0;} \
 else charsInLine+=2;\
 putc(hexmap[(ch>>4)&0xF],stm); putc(hexmap[ch&0xF],stm);}

private char hexmap[] = "0123456789abcdef";
private unsigned int rndnum, rndnumIV;
private int clear;
private int cipher;
private int charsInLine; 

private readonly RgNameTable nameFontBuild = {
  "BuildChar", &BCcmdName,
  "StrokeWidth", &strokewidthname,
  "CharStrings", &charstringsname,
  "CharData", &chardataname,
  "CharOffsets", &charoffsetsname,
  "Metrics", &mtrcsnm,
  "FontBBox", &bboxname,
  "FontPath", &fntpthnm,
  "PaintType", &pnttypnm,
  "BlueValues", &bluename,
  "OtherBlues", &otherBluesName,
  "MinFeature", &mnftrnm,
  "BlueFuzz", &blfuzznm,
  "Subrs", &subrsnm,
  "password", &scrtnm,
  "StandardEncoding", &stdencname,
  "strokewidth", &stkwdthname,
  "baseline", &bslnname,
  "capheight", &cphghtname,
  "bover", &bovrname,
  "xheight", &xhghtname,
  "descend", &dscndname,
  "ascend", &ascndname,
  "overshoot", &ovrshtname,
  "xover", &xovrname,
  "capover", &capovrname,
  "aover", &aovrname,
  "halfsw", &hlfswname,
  "OtherSubrs", &ocsbrsnm,
  "FudgeBands", &fdgbndsnm,
  ".notdef", &notdefname,
  "erosion", &eNm,
  "Erode", &ErNm,
  "locktype", &lNm,
  "BlueShift", &blueShiftName,
  "BlueScale", &blueScaleName,
  "lenIV", &lenIVname,
  "engineclass", &engineclassnm,
  "StdHW", &stdhwNm,
  "StdVW", &stdvwNm,
  "StemSnapH", &stemsnaphNm,
  "StemSnapV", &stemsnapvNm,
  "FamilyBlues", &famBluesNm,
  "FamilyOtherBlues", &famOtherBluesNm,
  "Encoding", &encname,
  "CDevProc", &cdevprocname,
  "Metrics", &metricsname,
  "WeightVector", &wvname,
  "rndwidth", &rndwidthNm,
  "idealwidth", &idealwidthNm,
  "gsfactor", &gsfactorNm,
#if GLOBALCOLORING
  "RndStemUp", &rndstmname,
  "ExpansionFactor", &expfctrname,
#endif
  NIL};


private procedure PSInitWrite() {
  int i, j, ok;
  unsigned int seed;
  char initVec[lenIV];
  StrObj instring;
  StmObj sObj;
  Stm outStm;
  seed = PopInteger();
  PopPStream(&sObj);
  outStm = GetStream(sObj);
  charsInLine = 0;
  InitRnum(rndnumIV, clock());
  for (ok = 0; !ok; )
    { InitRnum(rndnum,seed);
      for (j = 0; j < lenIV; j++)
	{ clear = Rnum8(rndnumIV);
	  Encrypt(rndnum, clear, cipher);
	  initVec[j] = cipher;
	  if (j==0 && (cipher==' ' || cipher=='\t' || cipher=='\n' ||
	      cipher=='\r')) break;
	  if ((cipher<'0'||cipher>'9') && (cipher<'A'||cipher>'F') &&
	      (cipher<'a'||cipher>'f')) ok = 1;
	}
     }
  for (j = 0; j < lenIV; j++)
    OutputEnc(initVec[j], outStm);
  }


private procedure PSWriteData() {
  int j;
  StrObj instring;
  StmObj sObj;
  Stm outStm;
  register string sp;
  PopPString(&instring);
  PopPStream(&sObj);
  outStm = GetStream(sObj);
  for (j = 0, sp = VMStrPtr(instring); j < instring.length; j++)
    { clear = *sp++;
      Encrypt(rndnum, clear, cipher);
      OutputEnc(cipher,outStm);
    }
  } /* WriteData */

private procedure PSExCheck() {
  Object ob;
  PopP(&ob);
  switch (ob.type)
    {
    case arrayObj: case pkdaryObj: case strObj: case stmObj:
      PushBoolean((boolean)(ob.access == xAccess));
      break;
    default: PushBoolean(false);
    }
  } /* PSExCheck */
#endif /*PPS*/

#if STAGE==DEVELOP
private procedure PSAlwaysErode() { alwaysErode = PopBoolean(); }
private procedure PSSetCSLimit() {cslimit = PSPopInteger();}
private procedure PSSetOFLimit() {oflimit = PSPopInteger();}

private readonly RgCmdTable cmdNonExport = {
  "alwayserode", PSAlwaysErode,
  "setcslimit", PSSetCSLimit,
  "setoflimit", PSSetOFLimit,
  NIL};
#endif /* STAGE==DEVELOP */

#if PPS
private readonly RgCmdTable cmdFontBuild = {
  "strtlck", StartLock,
  "xlck", PSSetXLock,
  "ylck", PSSetYLock,
  "lck", PSLck,
  "ErodeSW", PSErodeSW,
  "CCRun", PSCCRun,
  "initwrite", PSInitWrite,
  "writedata", PSWriteData,
  "excheck", PSExCheck,
  NIL};


public procedure FontBuildInit(reason)
        InitReason reason;
{
  Object ob;
  switch (reason){
    case init: {
      globals = (Globals) NEW(1, sizeof(GlobalsRec));
      maxOffsetVal = 29491; /* .45 */
#if STAGE==DEVELOP
      cslimit = 800;
      oflimit = 2000;
#endif
	  secret = false;
      stdLokData = (PLokData) NEW(1, sizeof(LokData));
      break;}
    case romreg:
      pLokData = NIL;
      RgstMNames(nameFontBuild);
      RgstExplicit("internaldict", PSIntDict);
      Begin(root->internalDict);
      RgstMCmds(cmdFontBuild);
      End();
#if STAGE==DEVELOP
      if (vSTAGE==DEVELOP) RgstMCmds(cmdNonExport);
      alwaysErode = false;
#endif /* STAGE==DEVELOP */

#if VMINIT
      ob = VMGetElem(root->param, rpFontKey); 
      if (ob.type != intObj) {
        LIntObj(ob, NumCArg('F', (integer)10, (integer)123456) ^ KEYHASH);
        VMPutElem(root->param, rpFontKey, ob);} 
      ob = VMGetElem(root->param, rpInternalKey);
      if (ob.type != intObj) {
        LIntObj(ob, NumCArg('I', (integer)10, (integer)234567) ^ KEYHASH);
        VMPutElem(root->param, rpInternalKey, ob);}
#endif /* VMINIT */
      break;
    case ramreg:
      AllocPArray(0, &emptyarray);
      break;
    endswitch}
}

#else /*PPS*/

public procedure FontBldDataHandler (code)
  StaticEvent code;
{
  switch (code) {
    case staticsDestroy: {
      FinishLock();
      break;
     default:
      break;
    }
  }
}

public procedure FontBuildInit(reason)
        InitReason reason;
{
  switch (reason){
    case init: {
      globals = (Globals) NEW(1, sizeof(GlobalsRec));
      maxOffsetVal = 29491; /* .45 */
#if STAGE==DEVELOP
      cslimit = 800;
      oflimit = 2000;
#endif /* STAGE==DEVELOP */
	  secret = false;
      stdLokData = (PLokData) NEW(1, sizeof(LokData));
      break;}
    case romreg:
      pLokData = NIL;
#if STAGE==DEVELOP
      if (vSTAGE==DEVELOP) RgstMCmds(cmdNonExport);
#endif /* STAGE==DEVELOP */
      break;
    case ramreg:
      AllocPArray(0, &emptyarray);
      break;
    endswitch}
}

#endif /*PPS*/

#else /* ATM */ 

/* ***ATM*** this is the tail section of fontbuild for ATM */

/* Note - The 680x0 assembly language version of BuildChar does
   not support decryption as implemented by the following lines. */

#define GetDecrypt\
  if (lenIV < 0) c = *sp++; else {ec = *sp++; Decrypt(rndnum, c, ec);}

typedef struct{
  longcardinal seed;
  string stp;
  } SubrFrame;

#define PSSTACKSIZE (3)
#define MAXFLEX (8)

#ifndef USE68KATM
public integer BuildChar(fd, chardata, pmtx, procs, b1, b2, b3, b4, b5,
			 pw, psb, fixupok, pcc, pathback, gsfactor, engineClass)
  PFontDesc fd;
  CharDataPtr chardata;
  PMtx pmtx;
  PBuildCharProcs procs;
  PGrowableBuffer b1, b2, b3, b4, b5;
  PFCd pw, psb;
  boolean fixupok;
  PCCInfo pcc;
  boolean pathback;
  integer gsfactor;
  boolean engineClass;
  {
  SubrFrame substck[MAXSUB /*10*/];
  Fixed psstack[PSSTACKSIZE /*3*/];
  FCd flexCds[MAXFLEX /*8*/];
  FCd pstk[MAXCDs /*12*/];
  Fixed botBands[MAXBLUES /*12*/], topBands[MAXBLUES /*12*/];
  FCd botLocs[MAXBLUES /*12*/], topLocs[MAXBLUES /*12*/];
  Fixed stdvw_array[MAXSTMS /*12*/], stdhw_array[MAXSTMS /*12*/];
  PFontPrivDesc priv;
  register string sp;
  register integer /*character*/ c, ec;
  register PFCd p;
  FCd cp, wid, tempfcd, dcp, delta, sb;
#if GLOBALCOLORING
  FCd prevcp, initcp;
#endif /* GLOBALCOLORING */
  register PFixed pp;
  register Fixed ftmp;
  register integer buf;
  IntX i, lenIV, indx;
  Card16 rndnum;
  Fixed bluefuzz;
  Fixed swval, sbx, sby;
  IntX psstackcnt, flexIndx, popcnt;
  Fixed dmin, w;
  IntX sumcnt;
  boolean integerdividend, doingFlex, closed;
  IntX subindex, lenTopBands, lenBotBands;
  int jmpVal;
#if QREDUCER
  boolean useQReducer;
#endif
#if ANSIDECL
  procedure (*tfmLockPt)(PFCd pt, PFCd p);
#else /* ANSIDECL */
  procedure (*tfmLockPt)();
#endif /* ANSIDECL */
#if RELALLOC
  RelOffset *privStrs;	/* Base for finding PrivCharStrings */
#else /* RELALLOC */
  CharDataPtr *privStrs;
#endif /* RELALLOC */

  if (jmpVal=setjmp(buildError))
    return jmpVal;

#if RELALLOC
  priv = (PFontPrivDesc)&((char *)fd)[fd->PrivateDict];
  privStrs = (RelOffset *)&((char *)fd)[fd->PrivCharStrings];
#else /* RELALLOC */
  priv = (PFontPrivDesc)fd->PrivateDict;
  privStrs = (CharDataPtr *)fd->PrivCharStrings;
#endif /* RELALLOC */

  bprocs = procs;
  if (b5->len < sizeof(LokData)) {
    if (!(*bprocs->GrowBuff)(b5,
      (Int32)(sizeof(LokData) - b5->len), false))
        return BE_MEMORY;
    }
  (lokSeqX = &((PLokData)b5->ptr)->X)->count = 0;
  (lokSeqY = &((PLokData)b5->ptr)->Y)->count = 0;
  lokSlopesInited = true;	/* Zero points means no slope to init! */
  lokFixMapOk = true;
  stdvw = stdvw_array; stdhw = stdhw_array;

#if GSMASK 
  gmscale = gsfactor;
#endif /*GSMASK*/
  SetMtx(pmtx);

  /* turn off locking and offsetfill if too small */
  locktype = (
#ifndef BUILDFONT
    (pathback) ||
#endif
    (!gsmatrix->noYSkew && !gsmatrix->noXSkew) ||
    (gsmatrix->len1000 < 0x40000)
    ) ? 0 : (gsmatrix->switchAxis) ? -1 : 1;


#if 1			/* FLAT */
  if (gsmatrix->len1000 <= FixInt(10))
    flatEps = 0x3000L;
  else if (gsmatrix->len1000 >= FixInt(35))
    flatEps = 0x8000L;
  else {
    /* Multiplication factor == (1/2-3/16) / (35-10) == 1/80 == 0x00CCCCCDL (Fract) */
    flatEps = fxfrmul(gsmatrix->len1000-FixInt(10), 0x00CCCCCDL) + 0x3000L;
    }
#else
  if (gsmatrix->len1000 < FixInt(34))
    flatEps = 0x3000L;
  else
    flatEps = 0x8000L;
#endif

  doFixupMap = fixupok && locktype != 0 &&
            gsmatrix->len1000 > 0x68000L &&
            gsmatrix->len1000 < 0x118000L;
  if (doFixupMap) {
    delta.x = delta.y = 0;
    if (locktype == -1) delta.x = FixedOne;
    else delta.y = FixedOne;
    FntIDTfmP(delta, &delta);
    lokSeqY->unitDist = os_labs(delta.y);
    delta.x = delta.y = 0;
    if (locktype == -1) delta.y = FixedOne;
    else delta.x = FixedOne;
    FntIDTfmP(delta, &delta);
    lokSeqX->unitDist = os_labs(delta.x);
    }

  l0 = FixInt(10000); r0 = FixInt(-10000);
  erosion = FixedOne;

  /* get StrokeWidth */
  swval = (fd->PaintType == 0) ? 0 : fd->StrokeWidth;

  /* see if we are outlining a font designed for fill.  changes locking */
  isoutline = (boolean)(fd->PaintType == 2);
  if (isoutline && !pathback)
    devsweven = (FRound(NormalizeSW(swval)) & 01) == 0;

  /* get metrics info */
  sb.x = sb.y = 0;
  if (pw) wid = *pw;
  else wid.x = wid.y = 0;
  if (psb) cp = *psb;
  else cp.x = cp.y = 0;
  sbx = cp.x; sby = cp.y;

  /* get standard horizontal and vertical stem widths */
  GetStandardWidths(priv, priv->stemWidth);

  /* get TopBands and BotBands */
  if (locktype != 0) {
    GetBlueValues(priv,botBands,&lenBotBands,topBands,&lenTopBands);
    SetupBlueLocs(botBands,lenBotBands,topBands,lenTopBands,botLocs,topLocs);
    FamilyBlueLocs(priv,botLocs,lenBotBands,topLocs,lenTopBands,
                lenstdhw > 0);
    BoostBotLocs(botLocs,lenBotBands);
#if FBDEBUG
    printf("blue locs\n");
    PrintBlueLocs(botLocs, lenBotBands, topLocs, lenTopBands);
#endif
    }
  
  /* get Private dict parameters */
  bluefuzz = FixInt(priv->blueFuzz);
  blueShiftStart = FixInt(priv->blueShift);
  blueScale = priv->blueScale;
  lenIV = priv->lenIV;
  if (locktype != 0)
    CheckBlueScale(botBands, lenBotBands, topBands, lenTopBands);
  
  /* StartLock and related code has been inlined above */

#if GLOBALCOLORING
  glcrFailure = false;
  /* If rdnstemupname is in private dict then global coloring is turned on
     and glcrRoundUp is determined from dictionary.  Note that glcrRoundUp
     is never used anywhere. */
  if (priv->rndstmflg) {
/*  glcrRoundUp = priv->rndstemup; */
    glcrOn = (locktype != 0);
    }
  else {
/*  glcrRoundUp = false; */
    glcrOn = ((lenBotBands < 2 || botBands[1] < -5) && (locktype != 0));
    }

  glcr:  /* come back to here to turn on global coloring from the font */

  if (glcrPrepass = glcrOn) { /* Assign and test flag */
    /* Initialize global coloring machinery: */
    IniGlbClrBuffs(b1, b2);
    }
#endif /* GLOBALCOLORING */

  restart:  /* come back to here to restart character processing */

  lokFixedMap = false;
  psstackcnt = popcnt = 0;
  p = pstk;
  doingFlex = false;
  sp = chardata;
  closed = true;
  subindex = 0;
#if DO_OFFSET
  dooffsetlock = false;
  lockoffset.x = lockoffset.y = 0;
#endif /* DO_OFFSET */
  if (locktype == 0)	tfmLockPt = TfmLockPt3;
  else			tfmLockPt = TfmLockPt1;
  
  rndwidth = 0; idealwidth = 0; sumcnt = 0;
  if (lenstdvw > 0) {
    w = FRoundF(stdvw[0]); if (w == 0) w = FixedOne;
    rndwidth += w;
    idealwidth += stdvw[0];
    sumcnt++;
    }
  if (lenstdhw > 0) {
    w = FRoundF(stdhw[0]); if (w == 0) w = FixedOne;
    rndwidth += w;
    idealwidth += stdhw[0];
    sumcnt++;
    }
  if (sumcnt == 2) {
    rndwidth >>= 1; idealwidth >>= 1; }

  if (pathback || isoutline) {
    moveto = bprocs->MoveTo;
    lineto = bprocs->LineTo;
    curveto = bprocs->CurveTo;
    closepath = bprocs->ClosePath;
    }
  else {
#if QREDUCER
    useQReducer = (!engineClass || gsmatrix->len1000 >= FixInt(ATM_CSLIMIT));
    if (useQReducer) {
      erosion = 0;
      moveto = ATMQNewPoint;
      lineto = ATMQNewPoint;
      closepath = ATMQRdcClose;
      ATMIniQReducer(b1, b2, b3, b4);
      ATMQResetReducer();
      }
    else
#endif
	  {
      moveto = CSNewPoint;
      lineto = CSNewPoint;
      closepath = CSClose;
      IniCScan(b1, b2, b3, b4);
      ResetCScan(fixupok, locktype != 0, gsmatrix->len1000,
        rndwidth, idealwidth, gsfactor);
      }
    fixedFltnLineTo = lineto;
    curveto = FixedFltn;
    }

  integerdividend = false;
  rndnum = FONTKEY;
  for (i = lenIV; --i >= 0; ) {GetDecrypt;}
  pp = (PFixed)p;
  while (true)
    {
    GetDecrypt;
    mainswitch:
    switch ((int)c)
      { /* 0..31 are opcodes */
      /* THE OPCODES FOR STROKED COURIER HAVE BEEN REMOVED */
      case RBcmd:
        RBLock(p,sby,botBands,lenBotBands,topBands,lenTopBands,
               botLocs,topLocs,bluefuzz);
        break;
      case RYcmd:
        RYLock(p,sbx);
        break;
      case VMTcmd: /* <dy> */
        cp.y += p[0].x;
       MoveToCmd:
	(*tfmLockPt)(&cp, &tempfcd);
#if GLOBALCOLORING
        initcp = prevcp = cp;
#endif /* GLOBALCOLORING */
	if (doingFlex && flexIndx < MAXFLEX) {
	  FntITfmP(tempfcd, &flexCds[flexIndx]);
	  flexIndx++;
          }
        else  {
          dcp = tempfcd;
#if GLOBALCOLORING
          if (!glcrOn || !glcrPrepass)
#endif /* GLOBALCOLORING */
            {
            if (!closed) (*closepath)();
            (*moveto)(&dcp);
	    closed = false;
	    }
	  }
        break;
      case RDTcmd:  /* <dx> <dy> */
        cp.x += p[0].x;  cp.y += p[0].y;
       LineToCmd:
#if GLOBALCOLORING
        if (glcrOn && glcrPrepass) {
          GlbClrLine(prevcp, cp);
	  dcp = prevcp = cp; }
        else
#endif /* GLOBALCOLORING */
          { (*tfmLockPt)(&cp, &dcp);
            (*lineto)(&dcp); }
        break;
      case HDTcmd:  /* <dx> */
        cp.x += p[0].x;
        goto LineToCmd;
      case VDTcmd:  /* <dy> */
        cp.y += p[0].x;
        goto LineToCmd;
      case RCTcmd: /* <dx1><dy1><dx2><dy2><dx3><dy3> */
       RCurveToCmd:
        p[0].x += cp.x;    p[0].y += cp.y;
        p[1].x += p[0].x;  p[1].y += p[0].y;
        p[2].x += p[1].x;  p[2].y += p[1].y;
        goto CurveToCmd;
      case CPcmd:
#if GLOBALCOLORING
        if (glcrOn && glcrPrepass)
          GlbClrLine(cp, initcp);
        else
#endif /* GLOBALCOLORING */
          { if (!closed) (*closepath)(); closed = true; }
        break;
      case DOSUBcmd:  /* <sub number> */
        if (subindex == MAXSUB) return BE_INVLFONT;
	i = FTrunc(*--pp);
        substck[subindex].seed = rndnum; /* stack the seed */
        substck[subindex++].stp = sp; /* may be garbage */
	if (i >= fd->numPrivCharStrings) return BE_INVLFONT;
#if !RELALLOC
	sp = privStrs[i];
#else
	sp = (string)&((char *)fd)[privStrs[i]];
#endif /* RELALLOC */
        rndnum = FONTKEY;
        for (i = lenIV; --i >= 0; ) {GetDecrypt;}
        continue;
      case RETcmd:  /* return from dosub */
        if (subindex == 0) return BE_INVLFONT;
        rndnum = substck[--subindex].seed;
        sp = substck[subindex].stp;
        continue;
      case ESCcmd:  /* esc op comes in next byte */
        GetDecrypt;
	escswitch:
        switch ((int)c)
          {
          case FLesc: /* flip type of locking */
#if DO_OFFSET
            dooffsetlock = !dooffsetlock;
            if (dooffsetlock && locktype != 0)
              {
	      if (!lokSlopesInited) InitSlopes();
	      tempfcd.x = Map(lokSeqX, cp.x);
	      tempfcd.y = Map(lokSeqY, cp.y);
              lockoffset.x = tempfcd.x - cp.x;
              lockoffset.y = tempfcd.y - cp.y;
              }
            else {lockoffset.x = lockoffset.y = 0;}
	    if (locktype == 0)		tfmLockPt = TfmLockPt3;
	    else if (!dooffsetlock)	tfmLockPt = TfmLockPt1;
	    else 			tfmLockPt = TfmLockPt2;
#endif /* DO_OFFSET */
            break;
          case RMesc:
            RMLock(p, sbx);
            break;
          case RVesc:  /* rv locking */
            RVLock(p, sby);
            break;
          case CCesc:
            CCBuild(p, pcc, sbx);
	    return BE_CCBUILD;
          case SBesc:  /* <sbx> <sby> <widx> <widy> */
	   SideBearingEsc:
            if (!psb)
              {sbx = cp.x = p[0].x; sby = cp.y = p[0].y;}
            if (!pw)
              {wid.x = p[1].x;  wid.y = p[1].y;}
            sb.x = p[0].x; sb.y = p[0].y;
#if GLOBALCOLORING 
            if (glcrOn && glcrPrepass) break;
#endif /* GLOBALCOLORING */
	    FntDTfmP(wid, &delta);
            if (!(*bprocs->OkToBuild)(wid, delta))
              goto donewithchar;
            break;
          case DIVesc:
            ftmp = *--pp;
            if (integerdividend) {ftmp = FTrunc(ftmp); integerdividend =false;}
            *(pp-1) = fixdiv(*(pp-1), ftmp);
            continue;
          case DOesc:  /* <a1>...<ai><i><index> */
            indx = FTrunc(*--pp);
	    i = FTrunc(*--pp);
	    switch ((int)indx) {
              case 0: /* flex proc */
                if (flexIndx != MAXFLEX-1) return BE_INVLFONT;
                if (i != 3) return BE_INVLFONT;
		tempfcd.y = *--pp + (sby - sb.y);
		tempfcd.x = *--pp + (sbx - sb.x);
		/* sbx is the x sidebearing */
		/* sb.x is the default x sidebearing */
		dmin = *--pp;
		flexCds[MAXFLEX-1] = tempfcd;
#if GLOBALCOLORING
                if (!glcrOn || !glcrPrepass)
#endif /* GLOBALCOLORING */
		  {
		  if (pathback)
		    FlexProc2(flexCds, dmin, &dcp);
		  else
		    FlexProc(flexCds, dmin, &dcp);
		  tempfcd = flexCds[0];
		  }
		psstack[psstackcnt] = tempfcd.y; psstackcnt++;
		psstack[psstackcnt] = tempfcd.x; psstackcnt++;
                doingFlex = false;
                break;
	      case 1: /* preflex1 proc */
                doingFlex = true;
		flexIndx = 0;
                break;
	      case 2: /* preflex2 proc */
                break;
	      case 3: /* Multi-coloring proc */
                if (i != 1) return BE_INVLFONT;
		psstack[psstackcnt] = *--pp;
		psstackcnt++;
		if (glcrOn && glcrPrepass)
                  StartGlcrLock();
		lokSeqX->count = 0;
		lokSeqY->count = 0;
		lokSlopesInited = true;	/* Zero points means no slope to init! */
		lokFixMapOk = false;
                if (lokFixedMap)
                  /* cannot do fixupmap with multicoloring */
                  goto restart;
	        break;
#if GLOBALCOLORING
              case 6:
                /* turn on global coloring for this character */
	        if (i != 0) return BE_INVLFONT;
                if (locktype == 0 || glcrOn || glcrFailure) continue;
                glcrOn = true;
	        goto glcr;
#endif /* GLOBALCOLORING */
              case 11: indx++;
              case 7: case 8: case 9: case 10:
                popcnt = indx-6; /* number of results */
                pp = DoBlend(pp, fd->WeightVector,
                               fd->lenWeightVector, popcnt);
                break;
              default:  /* unknown subr number */
		pp -= i; /* discard args from internal stack */
		if (pp < (PFixed)p) return BE_INVLFONT;
		/* process any POPs for the unknown subr */
		while (i-- > 0) {
		  GetDecrypt;
		  if (c != ESCcmd) goto mainswitch;
		  GetDecrypt;
		  if (c != POPesc) goto escswitch;
		  pp++;
		  }
	      }
            continue;
          case POPesc: /* pop number from PS stack to internal stack */
            if (popcnt > 0) popcnt--;
	    else {
              if (psstackcnt <= 0) return BE_INVLFONT;
	      psstackcnt--;
              *pp++ = psstack[psstackcnt];
	      }
            continue; /* loop without resetting pp */
          case PUSHCPesc:
            *pp++ = cp.x;  *pp++ = cp.y;
            break;
          case POPCPesc:
            cp.y = *--pp;  cp.x = *--pp;
            break;
          default: return BE_INVLFONT;
          }
        break;
      case SBXcmd: /* <sbx> <widx> */
	p[1].x = p[0].y;
      	p[1].y = 0; p[0].y = 0;
      	goto SideBearingEsc;
      case EDcmd:  /* end char -- do the dirty work */
#if GLOBALCOLORING
        if (glcrOn && glcrPrepass) {
          glcrPrepass = false;
          AdjustToStdWidths();
	  ProcessGlbClrs(
            botBands,lenBotBands,topBands,lenTopBands,bluefuzz,b3,
	    botLocs,topLocs,priv->expansionfactor);
          if (glcrFailure) {glcrOn /* = glcrPrepass */ = false;}
	  sp = chardata;
	  goto restart;
          }
#endif /* GLOBALCOLORING */
	switch (fd->PaintType)
          {
          case 0: {
            if (!closed)
              (*closepath)();
            if (!pathback) {
              if (l0 > r0) l0 = r0 = l1 = r1 = 0;
              if (!(*bprocs->CharInfo)(l0, l1, r0, r1, idealwidth, rndwidth,
                                priv->forceBoldPresent, priv->forceBoldValue))
                goto donewithchar;
#if QREDUCER
	      if (useQReducer)
		ATMQReduce(true, bprocs->Runs);
	      else
#endif
		CScan(bprocs->Runs);
	      }
            break;
	    }
          case 2: /* stroked font */
            break;
          default: return BE_INVLFONT;
          }
        goto donewithchar;
      case MTcmd:
	cp = p[0]; cp.x += sbx;
	goto MoveToCmd;
      case CTcmd:
        p[0].x += sbx; p[1].x += sbx; p[2].x += sbx;
       CurveToCmd:
        cp = p[2];
#if GLOBALCOLORING
        if (glcrOn && glcrPrepass) {
          GlbClrLine(prevcp, p[0]);
          GlbClrLine(p[1], p[2]);
	  prevcp = cp; }
        else 
#endif /* GLOBALCOLORING */
        {
        (*tfmLockPt)(&p[0], &p[0]);
	(*tfmLockPt)(&p[1], &p[1]);
	(*tfmLockPt)(&p[2], &p[2]);
	(*curveto)(&dcp, &p[0], &p[1], &p[2]);
	}
	dcp = p[2];
        break;
      case RMTcmd: /* <dx> <dy> */
        cp.x += p[0].x;  cp.y += p[0].y;
        goto MoveToCmd;
      case HMTcmd: /* <dx> */
        cp.x += p[0].x;
        goto MoveToCmd;
      case VHCTcmd:  /* <dy1><dx2><dy2><dx3> */
        p[2].x = p[1].y; p[2].y = 0;
        p[1].y = p[1].x; p[1].x = p[0].y;
        p[0].y = p[0].x; p[0].x = 0;
        goto RCurveToCmd;
      case HVCTcmd:  /* <dx1><dx2><dy2><dy3> */
        p[2].y = p[1].y; p[2].x = 0;
        p[1].y = p[1].x; p[1].x = p[0].y;
        p[0].y = 0;
        goto RCurveToCmd;
      case CBcmd:	/* All unused cases MUST be listed here! */
      case CYcmd:
      case DTcmd:
      case MINcmd:
      case STcmd:
      case NPcmd:
      case SLCcmd:
      case MULcmd:
      case STKWDTHcmd:
      case BSLNcmd:
      case CPHGHTcmd:
      case BOVRcmd:
      case XHGHTcmd:
	  return BE_INVLFONT;
      default: /* numbers */
        if (/*(32 <= c) &&*/ (c <= 246))
          /* [-107..107] */
          *pp++ = FixInt(c-139);
        else if (/*(247 <= c) &&*/ (c <= 250))
          {/* [108..1131] */
          buf = (c-247) << 8;
          GetDecrypt; buf |= c;
          *pp++ = FixInt(108 + buf);
          }
        else if (/*(251 <= c) &&*/ (c <= 254))
          {/* [-1131..-108] */
          buf = (c-251) << 8;
          GetDecrypt; buf |= c;
          *pp++ = FixInt(-108 - buf);
          }
        else
          {/* four-byte numbers */
          GetDecrypt; buf = c;
          GetDecrypt; buf = (buf << 8) | c;
          GetDecrypt; buf = (buf << 8) | c;
          GetDecrypt; buf = (buf << 8) | c;
          if (buf >= -32000 && buf <= 32000) *pp++ = FixInt(buf);
          else {*pp++ = buf;  integerdividend = true;}
          }
        continue; /* loop without resetting pp */
      } /* switch */
    pp = (integer *)p;
    } /* while */
  donewithchar:		/* for EDcmd and ESPCesc */
  return BE_NOERR;
  }
#endif /* USE68KATM */

#endif /* ATM */ 
/* BC Version:  v007  Fri Jun 1 10:09:39 PDT 1990 */
