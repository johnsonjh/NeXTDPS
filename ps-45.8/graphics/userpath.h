/*
  userpath.h

Copyright (c) 1987, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Mike Schuster: Fri Apr  3 13:45:51 1987
Edit History:
Larry Baer: Fri Nov 17 15:15:12 1989
Mike Schuster: Fri May 22 08:37:57 1987
Ivor Durham: Wed May  4 16:24:28 1988
Bill Paxton: Wed Apr 27 08:02:25 1988
Joe Pasqua: Thu Jan 19 15:15:38 1989
Jim Sandman: Wed Dec 13 13:08:13 1989
End Edit History.
*/

#include LANGUAGE

typedef enum {
  currentState, pathState, initState, appendState
  } PathState;

typedef struct {
  Fixed a, b, c, d, tx, ty;
  } FixMtx, *PFixMtx;

typedef enum { identity, flipY, bc_zero, ad_zero, general } MtxType;

typedef struct {
    AryObj aryObj;
    PathState state;
    BBoxRec bbox;    /* Bounding box in device coords */
    BBoxRec ubbox;   /* Bounding box in user coords */
    Cd sp;    /* Start point -- closepath goes to here */
    Cd cp;    /* Current point */
    procedure (*stproc)();
    procedure (*ltproc)();
    procedure (*clsproc)();
    procedure (*endproc)();
    procedure (*ctproc)();
    procedure (*MoveTo)();
    procedure (*LineTo)();
    procedure (*CurveTo)();
    procedure (*ClosePath)();
    real reps;
    Fixed feps;
    boolean noendst:1;
    boolean useFixed:1;
    boolean encoded:1;
    boolean testRect:1;
    boolean reportFixed:1;
    boolean packed:1;
    boolean ucache:1;
    boolean fill:1;
    boolean evenOdd:1;
    boolean circletraps:1;
    boolean dispose:1;
    boolean doneMoveTo:1;
    boolean unused:4;
    Mtx matrix;
    FixMtx fmtx;
    MtxType mtxtype;
    DevBBoxRec fbbox;
    DevCd ll, ur;
    NumStrm datastrm;
    string ctrlstr;
    Card16 ctrllen;
} UserPathContext, *PUserPathContext;

#define AssertCheck(assert) {if (!(assert)) TypeCheck();}

#define cd1 ((PCd) &vals[0])
#define cd2 ((PCd) &vals[2])
#define cd3 ((PCd) &vals[4])
#define cd4 ((PCd) &vals[6])

#define BBoxUpdate(val, min, max) \
{if (val < min) min = val; else if (val > max) max = val;}

#define BBoxTest(cd, bbox) \
{if ((cd).x < (bbox).bl.x || (cd).x > (bbox).tr.x || \
     (cd).y < (bbox).bl.y || (cd).y > (bbox).tr.y) RangeCheck();}

extern boolean UCacheFill();
extern procedure UsrPthDoMoveTo();
extern procedure UsrPthDoLineTo();
extern procedure UsrPthDoCurveTo();
extern procedure UsrPthDoClsPth();
extern procedure UsrPthArc();
extern procedure UsrPthArcTo();
extern procedure UsrPthDoFinish();
extern procedure QUsrPthArc();
extern procedure QUsrPthArcTo();
extern boolean CheckForMtx();
extern procedure DoEUserPath();
extern procedure GetEUsrPthBBox();
extern procedure QDoEUsrPth();
extern boolean EUsrPthCheckMtLt();
