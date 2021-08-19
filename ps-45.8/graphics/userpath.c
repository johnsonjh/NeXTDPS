/* PostScript Graphics User Path Procedures

              Copyright 1983, 1986, 1987 -- Adobe Systems, Inc.
            PostScript is a trademark of Adobe Systems, Inc.
NOTICE:  All information contained herein or attendant hereto is, and
remains, the property of Adobe Systems, Inc.  Many of the intellectual
and technical concepts contained herein are proprietary to Adobe Systems,
Inc. and may be covered by U.S. and Foreign Patents or Patents Pending or
are protected as trade secrets.  Any dissemination of this information or
reproduction of this material are strictly forbidden unless prior written
permission is obtained from Adobe Systems, Inc.

Original version: Mike Schuster: Fri Apr  3 13:45:51 1987
Edit History:
Larry Baer: Wed Nov 29 13:07:51 1989
Scott Byer: Thu Jun  1 12:49:23 1989
Mike Schuster: Fri May 22 08:37:57 1987
Bill Paxton: Fri Aug 19 12:41:48 1988
Jim Sandman: Wed Dec 13 13:21:00 1989
Ivor Durham: Fri Sep 23 16:08:30 1988
Ed Taft: Fri Jul 29 11:43:34 1988
Joe Pasqua: Thu Jan 19 15:13:00 1989
Mark Francis: Thu Nov  9 18:05:28 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include DEVICE
#include ERROR
#include EXCEPT
#include FP
#include GRAPHICS
#include LANGUAGE
#include VM

#include "path.h"
#include "stroke.h"
#include "graphicspriv.h"
#include "userpath.h"
#include "graphicsnames.h"

extern procedure NoOp();
extern boolean HasCurrentPoint();
extern boolean IsPathEmpty();
extern DevPrim *UCGetDevPrim();
extern boolean RdcStroke();

#if DPSXA
Cd UOffset, uXAc1, uXAc2;
boolean uXARectangle;
extern boolean strokeOp;
#endif DPSXA

/* User Path name & command constants */
/* Note that for systemdict operators, nmval and cmdval are the same */

#define arcNm graphicsNames[nm_arc].val.nmval
#define arcnNm graphicsNames[nm_arcn].val.nmval
#define arctNm graphicsNames[nm_arct].val.nmval
#define setbboxNm graphicsNames[nm_setbbox].val.nmval
#define curvetoNm graphicsNames[nm_curveto].val.nmval
#define closepathNm graphicsNames[nm_closepath].val.nmval
#define linetoNm graphicsNames[nm_lineto].val.nmval
#define movetoNm graphicsNames[nm_moveto].val.nmval
#define rcurvetoNm graphicsNames[nm_rcurveto].val.nmval
#define rlinetoNm graphicsNames[nm_rlineto].val.nmval
#define rmovetoNm graphicsNames[nm_rmoveto].val.nmval
#define ucacheNm graphicsNames[nm_ucache].val.nmval

private MakeBBox(mtx, vals, bbox) PMtx mtx; real *vals; BBox bbox; {
  Cd cd;
  TfmPCd(*cd1, mtx, &cd);
  bbox->bl = cd;
  bbox->tr = cd;
  TfmPCd(*cd2, mtx, &cd);
  BBoxUpdate(cd.x, bbox->bl.x, bbox->tr.x);
  BBoxUpdate(cd.y, bbox->bl.y, bbox->tr.y);
  vals[4] = vals[0]; vals[5] = vals[3];
  vals[6] = vals[2]; vals[7] = vals[1];
  TfmPCd(*cd3, mtx, &cd);
  BBoxUpdate(cd.x, bbox->bl.x, bbox->tr.x);
  BBoxUpdate(cd.y, bbox->bl.y, bbox->tr.y);
  TfmPCd(*cd4, mtx, &cd);
  BBoxUpdate(cd.x, bbox->bl.x, bbox->tr.x);
  BBoxUpdate(cd.y, bbox->bl.y, bbox->tr.y);
  /* enlarge bbox a little to compensate for floating point fuzz */
  bbox->bl.x -= fpOne;	/* -- causes incorrect code on microVAX here */
  bbox->bl.y -= fpOne;
  bbox->tr.x += fpOne;
  bbox->tr.y += fpOne;
  }

public procedure PSSetBBox() {
  Cd ll, ur;
  real vals[8];
  BBoxRec bbox;
  register PPath path;
  PopPCd(&ur); PopPCd(&ll);
  if ((ur.x < ll.x) || (ur.y < ll.y))
    RangeCheck();
  vals[0] = ll.x; vals[1] = ll.y;
  vals[2] = ur.x; vals[3] = ur.y;
  MakeBBox(&gs->matrix, vals, &bbox);
  path = &gs->path;
  if (path->length > 0) { /* merge with current bbox */
    if (path->bbox.bl.x < bbox.bl.x) bbox.bl.x = path->bbox.bl.x;
    if (path->bbox.bl.y < bbox.bl.y) bbox.bl.y = path->bbox.bl.y;
    if (path->bbox.tr.x > bbox.tr.x) bbox.tr.x = path->bbox.tr.x;
    if (path->bbox.tr.y > bbox.tr.y) bbox.tr.y = path->bbox.tr.y;
    }
  path->bbox = bbox;
  path->setbbox = true;
  }

public procedure UsrPthBBox(context)
  register PUserPathContext context; {
  real vals[8];
  register Preal val;
  Object ob;
  register PObject argument, operator;
  context->matrix = gs->matrix;
  RRoundP(&context->matrix.tx, &context->matrix.tx);
  RRoundP(&context->matrix.ty, &context->matrix.ty);
  if (context->encoded)
    GetEUsrPthBBox(context, vals);
  else {
    AssertCheck(context->aryObj.length >= 5);
    val = vals;
    if (context->packed) {
      integer i;
      for (i = 0; i < 4; i++) {
        VMCarCdr(&context->aryObj, &ob);
        switch (ob.type) {
          case realObj: *(val++) = (Component) ob.val.rval; break;
          case  intObj: *(val++) = (Component) ob.val.ival; break;
          default: TypeCheck();
          }
        }
      VMCarCdr(&context->aryObj, &ob);
      operator = &ob;
      }
    else {
      argument = context->aryObj.val.arrayval;
      operator = argument + 4;
      while (argument < operator) {
        switch (argument->type) {
          case realObj: *(val++) = (Component) argument->val.rval; break;
          case  intObj: *(val++) = (Component) argument->val.ival; break;
          default: TypeCheck();
          }
        argument++;
        }
      }
    AssertCheck(
      (operator->type == nameObj || operator->type == cmdObj) &&
      operator->val.nmval == setbboxNm);
    }
  context->ubbox.bl = *cd1;
  context->ubbox.tr = *cd2;
  MakeBBox(&context->matrix, vals, &context->bbox);
  }

public procedure UsrPthDoMoveTo(context, cd, absFlg)
  register PUserPathContext context; Cd cd; boolean absFlg; {
  PMtx mtx = &context->matrix;
  if (absFlg)
    TfmPCd(cd, mtx, &cd);
#if DPSXA
  	cd.x += UOffset.x;
  	cd.y += UOffset.y;
#endif /* DPSXA */
  else
    RTfmPCd(cd, mtx, context->cp, &cd);
  context->sp = cd;
  context->cp = cd;
  switch (context->state) {
    case currentState: break;
    case pathState:
      (*context->endproc)();
      context->state = currentState;
      break;
    case initState: context->state = currentState; break;
    case appendState: (*context->MoveTo)(cd, context); break;
    }
  BBoxTest(cd, context->bbox);
  }

public procedure UsrPthDoLineTo(context, cd, absFlg)
  register PUserPathContext context; Cd cd; boolean absFlg; {
  DevCd d0;
  PMtx mtx = &context->matrix;
  if (absFlg)
    TfmPCd(cd, mtx, &cd);
#if DPSXA
  	cd.x += UOffset.x;
  	cd.y += UOffset.y;
#endif DPSXA
  else
    RTfmPCd(cd, mtx, context->cp, &cd);
  switch (context->state) {
    case currentState:
      if (context->reportFixed) {
        FixCd(context->cp, &d0); (*context->stproc)(d0); }
      else (*context->stproc)(context->cp);
      context->state = pathState;
      /* fall through to call ltproc */
    case pathState:
      if (context->reportFixed) {
        FixCd(cd, &d0); (*context->ltproc)(d0); }
      else (*context->ltproc)(cd);
      break;
    case initState: TypeCheck(); break;
    case appendState: (*context->LineTo)(cd, context); break;
    }
  context->cp = cd;
  BBoxTest(cd, context->bbox);
  }

public procedure UsrPthDoCurveTo(context, vals, fr, absFlg)
  register PUserPathContext context; real *vals; FltnRec *fr; boolean absFlg; {
  DevCd d0, d1, d2, d3;
  PMtx mtx = &context->matrix;
  if (absFlg) {
    TfmPCd(*cd1, mtx, cd1);
    TfmPCd(*cd2, mtx, cd2);
    TfmPCd(*cd3, mtx, cd3);
#if DPSXA
	cd1->x += UOffset.x;
	cd1->y += UOffset.y;
	cd2->x += UOffset.x;
	cd2->y += UOffset.y;
	cd3->x += UOffset.x;
	cd3->y += UOffset.y;
#endif /* DPSXA */
    }
  else {
    RTfmPCd(*cd1, mtx, context->cp, cd1);
    RTfmPCd(*cd2, mtx, context->cp, cd2);
    RTfmPCd(*cd3, mtx, context->cp, cd3);
    }
  BBoxTest(*cd1, context->bbox);
  BBoxTest(*cd2, context->bbox);
  BBoxTest(*cd3, context->bbox);
  switch (context->state) {
    case currentState:
      if (context->reportFixed) {
        FixCd(context->cp, &d0); (*context->stproc)(d0); }
      else (*context->stproc)(context->cp);
      context->state = pathState;
      /* fall through to call ctproc */
    case pathState:
      (*context->ctproc)(true);
      fr->limit = FLATTENLIMIT;
      if (context->useFixed) {
        FixCd(context->cp, &d0);
        FixCd(*cd1, &d1);
        FixCd(*cd2, &d2);
        FixCd(*cd3, &d3);
	FFltnCurve(d0, d1, d2, d3, fr, (boolean)!(context->testRect));
        }
      else
        FltnCurve(context->cp, *cd1, *cd2, *cd3, fr);
        (*context->ctproc)(false);
      break;
    case initState: TypeCheck(); break;
    case appendState: (*context->CurveTo)(*cd1, *cd2, *cd3, context); break;
    }
  context->cp = *cd3;
  }

public procedure UsrPthDoClsPth(context)
  register PUserPathContext context; {
  DevCd d0;
  switch (context->state) {
    case currentState:
      if (context->reportFixed) {
         FixCd(context->sp, &d0); (*context->stproc)(d0); }
      else (*context->stproc)(context->sp);
      (*context->clsproc)();
      break;
    case pathState:
      (*context->clsproc)();
      context->cp = context->sp;
      context->state = currentState;
      break;
    case initState: break;
    case appendState: (*context->ClosePath)(context); break;
    }
  }

public procedure UsrPthDoFinish(context)
  register PUserPathContext context; {
  DevCd d0;
  switch (context->state) {
    case currentState:
      if (!context->noendst) {
        if (context->reportFixed) {
          FixCd(context->sp, &d0); (*context->stproc)(d0); }
        else (*context->stproc)(context->sp);
        }
      break;
    case pathState:
      (*context->endproc)();
      break;
    case initState: break;
    case appendState: break;
    default: TypeCheck();
    }
  context->state = initState;
  }

private boolean UsrPthInit(context)
  PUserPathContext context; {
  return context->state == initState;
  }

private procedure UsrPthMoveTo(cd, context)
register PUserPathContext context;
Cd cd; {
  context->sp = cd;
  context->cp = cd;
  switch (context->state) {
    case initState:
      context->state = currentState;
      break;
    case currentState:
      break;
    case pathState:
      (*context->endproc)();
      context->state = currentState;
      break;
    case appendState:
      (*context->MoveTo)(cd, context);
      break;
    default:
      AssertCheck(0);
  }
}

private procedure UsrPthLineTo(cd, context)
register PUserPathContext context;
Cd cd; {
  DevCd d0;
  switch (context->state) {
    case initState:
      TypeCheck();
      break;
    case currentState:
      if (context->reportFixed) {
        FixCd(context->cp, &d0); 
	(*context->stproc)(d0);
	}
      else (*context->stproc)(context->cp);
      context->state = pathState;
      /* fall through */
    case pathState:
      if (context->reportFixed) {
        FixCd(cd, &d0); (*context->ltproc)(d0); }
      else (*context->ltproc)(cd);
      break;
    case appendState:
      (*context->LineTo)(cd, context);
      break;
    default:
      AssertCheck(0);
    }
  context->cp = cd;
}

private procedure UsrPthCurveTo(c1, c2, c3, context)
  Cd c1, c2, c3; register PUserPathContext context; {
  DevCd d0, d1, d2, d3;
  FltnRec fr;
  switch (context->state) {
    case initState:
      TypeCheck();
      break;
    case currentState:
      if (context->reportFixed) {
        FixCd(context->cp, &d0); 
	(*context->stproc)(d0);
	}
      else (*context->stproc)(context->cp);
      context->state = pathState;
      /* fall through */
    case pathState:
      (*context->ctproc)(true);
      fr.report = context->ltproc;
      fr.reps = context->reps;
      fr.ll = context->ll;
      fr.ur = context->ur;
      fr.reportFixed = context->reportFixed;
      fr.feps = context->feps;
      fr.limit = FLATTENLIMIT;
      if (context->useFixed) {
        FixCd(context->cp, &d0);
        FixCd(c1, &d1);
        FixCd(c2, &d2);
        FixCd(c3, &d3);
	FFltnCurve(d0, d1, d2, d3, &fr, (boolean)!(context->testRect));
        }
      else
        FltnCurve(context->cp, c1, c2, c3, &fr);
      (*context->ctproc)(false);
      break;
    case appendState:
      (*context->CurveTo)(c1, c2, c3, context);
      break;
    default:
      AssertCheck(0);
    }
  context->cp = c3;
}

private boolean UPArcInit(context) PUserPathContext context;
   { return !HasCurrentPoint(&gs->path); }

public procedure UsrPthArc(cd, radius, startAng, endAng, ccWise, context)
  PUserPathContext context;
  Cd cd; Component radius, startAng, endAng; boolean ccWise; {
  switch (context->state) {
    case initState:
      /* fall through */
    case currentState:
      /* fall through */
    case pathState:
#if DPSXA
      ArcInternal(cd, radius, startAng, endAng, ccWise,
                  UsrPthInit, UsrPthMoveTo, UsrPthLineTo, UsrPthCurveTo,
		  &context->matrix, (char *)context, true);
#else /* DPSXA */
      ArcInternal(cd, radius, startAng, endAng, ccWise,
                  UsrPthInit, UsrPthMoveTo, UsrPthLineTo, UsrPthCurveTo,
		  &context->matrix, (char *)context);
#endif /* DPSXA */
      break;
    case appendState:
      context->doneMoveTo = true;
#if DPSXA
      ArcInternal(cd, radius, startAng, endAng, ccWise, UPArcInit,
                  context->MoveTo, context->LineTo, context->CurveTo,
		  &context->matrix, (char *)context, false);
#else DPSXA
      ArcInternal(cd, radius, startAng, endAng, ccWise, UPArcInit,
                  context->MoveTo, context->LineTo, context->CurveTo,
		  &context->matrix, (char *)context);
#endif DPSXA
      break;
    default:
      AssertCheck(0);
    }
  }

public procedure UsrPthArcTo(c1, c2, radius, context)
  PUserPathContext context;
  Cd c1, c2; Component radius; {
  Cd c0;
  Cd ct1;
  Cd ct2;
  switch (context->state) {
    case currentState:
      /* fall through */
    case pathState:
      ITfmP(context->cp, &c0);
#if DPSXA
	  c0.x -= UOffset.x;
	  c0.y -= UOffset.y;
      ArcToInternal(c0, c1, c2, radius, &ct1, &ct2,
                    UsrPthInit, UsrPthMoveTo, UsrPthLineTo, UsrPthCurveTo,
		    &context->matrix, (char *)context,true);
#else DPSXA
      ArcToInternal(c0, c1, c2, radius, &ct1, &ct2,
                    UsrPthInit, UsrPthMoveTo, UsrPthLineTo, UsrPthCurveTo,
		    &context->matrix, (char *)context);
#endif DPSXA
      break;
    case initState:
      TypeCheck();
      break;
    case appendState:
      CheckForCurrentPoint(&gs->path);
      ITfmP(gs->cp, &c0);
      ArcToInternal(c0, c1, c2, radius, &ct1, &ct2, UPArcInit,
                    context->MoveTo, context->LineTo, context->CurveTo,
		    &context->matrix, (char *)context);
      break;
    default:
      AssertCheck(0);
    }
}

private procedure DoUserPath(context)
register PUserPathContext context; {
register PObject argument;
register PObject last;
register PNameEntry opval; /* either for nameObj or cmdObj */
register Component *val;
Int16 len, args;
PObject ar;
Object ob;
Component vals[6];
boolean reportFixed = context->reportFixed;
FltnRec fr;
#if DPSXA
PObject save_arrayval;
int save_length;
#endif /* DPSXA */
fr.report = context->ltproc;
fr.reps = context->reps;
fr.ll = context->ll;
fr.ur = context->ur;
fr.reportFixed = reportFixed;
fr.feps = context->feps;
ar = &context->aryObj;
#if DPSXA
/* Need to save these values since this proc will be called
   again with the same context.
*/
save_length = ar->length;
save_arrayval = ar->val.arrayval;
#endif DPSXA
if (!context->packed) {
  len = context->aryObj.length;
  if (len == 5) return; /* just a bbox */
  argument = context->aryObj.val.arrayval;
  last = argument + (len - 1);
  AssertCheck(last->type == nameObj || last->type == cmdObj);
  /* last array element is a name or command */
  argument += 5; /* skip the bbox */
  }
else { argument = ar; last = ar + 1; }
while (argument <= last && ar->length > 0) {
  val = vals; args = 0;
  if (context->packed) {
    while (true) {
      if (ar->length == 0) TypeCheck();
      VMCarCdr(ar, &ob);
      switch (ob.type) {
        case realObj: *(val++) = (Component) ob.val.rval; break;
        case intObj: *(val++) = (Component) ob.val.ival; break;
        case nameObj: opval = ob.val.nmval; goto nextOp;
        case cmdObj:  opval = ob.val.cmdval; goto nextOp;
        default: TypeCheck();
        }
      args++;
      AssertCheck(args <= 6);
      }
    }
  else {
    while (true) {
      switch (argument->type) {
        case realObj: *(val++) = (Component) argument->val.rval; break;
        case intObj: *(val++) = (Component) argument->val.ival; break;
        case nameObj: opval = argument->val.nmval; goto nextOp;
        case cmdObj:  opval = argument->val.cmdval; goto nextOp;
        default: TypeCheck();
        }
      argument++;
      args++;
      AssertCheck(args <= 6);
      }
    }
  nextOp:
  switch (args) {
    case 0: {
      if (opval != closepathNm) TypeCheck();
      UsrPthDoClsPth(context);
      break;
      }
    case 1: TypeCheck(); break;
    case 2: {
      if (opval == linetoNm || opval == rlinetoNm)
        UsrPthDoLineTo(context, *cd1, (boolean)(opval == linetoNm));
      else if (opval == rmovetoNm || opval == movetoNm)
        UsrPthDoMoveTo(context, *cd1, (boolean)(opval == movetoNm));
      else TypeCheck();
      break;
      }
    case 3: TypeCheck(); break;
    case 4: TypeCheck(); break; /* setbbox only allowed in front */
    case 5: {
      if (opval == arcnNm || opval == arcNm)
        UsrPthArc(*cd1, vals[2], vals[3], vals[4],
                  (boolean)(opval == arcNm), context);
      else if (opval == arctNm)
        UsrPthArcTo(*cd1, *cd2, vals[4], context);
      else TypeCheck();
      break;
      }
    case 6: {
      if (opval == rcurvetoNm || opval == curvetoNm)
        UsrPthDoCurveTo(context, vals, &fr, (boolean)(opval == curvetoNm));
      else TypeCheck();
      break;
      }
    default: TypeCheck();
    }
  if (!context->packed) argument++;
  }
UsrPthDoFinish(context);
#if DPSXA
ar->length = save_length;
ar->val.arrayval = save_arrayval;
#endif DPSXA
}

private procedure CheckFixCd(cd, dc, bb)
  Cd cd; register DevCd *dc; register DevBBox bb; {
  FixCd(cd, dc);
  if (dc->x < bb->bl.x || dc->x > bb->tr.x ||
      dc->y < bb->bl.y || dc->y > bb->tr.y) RangeCheck();
  }

private procedure QUsrPthLineTo(cd, context)
  register PUserPathContext context; Cd cd; {
  DevCd dc;
  register DevBBox bb = &context->fbbox;
  switch (context->state) {
    case currentState:
      CheckFixCd(context->cp, &dc, bb);
      (*context->stproc)(dc);
      context->state = pathState;
      /* fall through */
    case pathState:
      CheckFixCd(cd, &dc, bb);
      (*context->ltproc)(dc);
      break;
    case initState:
      TypeCheck();
      break;
    }
  context->cp = cd;
}

private procedure QUsrPthCurveTo(c1, c2, c3, context)
  register PUserPathContext context;
  Cd c1; Cd c2; Cd c3; {
  DevCd d0, d1, d2, d3;
  FltnRec fr;
  register DevBBox bb = &context->fbbox;
  CheckFixCd(context->cp, &d0, bb);
  switch (context->state) {
    case initState:
      TypeCheck();
      break;
    case currentState:
      (*context->stproc)(d0);
      context->state = pathState;
      /* fall through */
    case pathState:
      (*context->ctproc)(true);
      CheckFixCd(c1, &d1, bb);
      CheckFixCd(c2, &d2, bb);
      CheckFixCd(c3, &d3, bb);
      fr.report = context->ltproc;
      fr.ll = context->ll;
      fr.ur = context->ur;
      fr.reportFixed = true;
      fr.feps = context->feps;
      fr.limit = FLATTENLIMIT;
      FFltnCurve(d0, d1, d2, d3, &fr, (boolean)!(context->testRect));
      (*context->ctproc)(false);
      break;
    case appendState:
      (*context->CurveTo)(c1, c2, c3, context);
      break;
    default:
      AssertCheck(0);
    }
  context->cp = c3;
  }

public procedure QUsrPthArc(cd, radius, startAng, endAng, ccWise, context)
  PUserPathContext context;
  Cd cd; Component radius, startAng, endAng; 
  boolean ccWise; {
#if DPSXA
  ArcInternal(cd, radius, startAng, endAng, ccWise,
              UsrPthInit, UsrPthMoveTo, QUsrPthLineTo, QUsrPthCurveTo,
	      &context->matrix, (char *)context,true);
#else /* DPSXA */
  ArcInternal(cd, radius, startAng, endAng, ccWise,
              UsrPthInit, UsrPthMoveTo, QUsrPthLineTo, QUsrPthCurveTo,
	      &context->matrix, (char *)context);
#endif /* DPSXA */
  }

public procedure QUsrPthArcTo(c1, c2, radius, context)
  PUserPathContext context;
  Cd c1, c2; Component radius; {
  Cd c0, ct1, ct2;
  if (context->state == initState) TypeCheck();
  ITfmP(context->cp, &c0);
#if DPSXA
  c0.x -= UOffset.x;
  c0.y -= UOffset.y;
  ArcToInternal(c0, c1, c2, radius, &ct1, &ct2,
              UsrPthInit, UsrPthMoveTo, QUsrPthLineTo, QUsrPthCurveTo,
	      &context->matrix, (char *)context,true);
#else /* DPSXA */
  ArcToInternal(c0, c1, c2, radius, &ct1, &ct2,
              UsrPthInit, UsrPthMoveTo, QUsrPthLineTo, QUsrPthCurveTo,
	      &context->matrix, (char *)context);
#endif /* DPSXA */
  }

#define dc1 ((DevCd *) &vals[0])
#define dc2 ((DevCd *) &vals[2])
#define dc3 ((DevCd *) &vals[4])

private procedure QDoUserPath(context)
PUserPathContext context; {
register PObject argument;
PObject last;
register PNameEntry opval;
register Fixed *val, x, y;
integer len, args;
DevCd cp, sp;
Fixed vals[8];
procedure (*stproc)() = context->stproc;
procedure (*ltproc)() = context->ltproc;
procedure (*clsproc)() = context->clsproc;
procedure (*endproc)() = context->endproc;
procedure (*ctproc)() = context->ctproc;
MtxType mtxtype = context->mtxtype;
PMtx mtx = &context->matrix;
register PFixMtx fmtx = &context->fmtx;
register PathState state = context->state;
DevBBoxRec bb;
boolean notTestRect = !(context->testRect);
FltnRec fr;
fr.report = ltproc;
fr.reps = context->reps;
fr.ll = context->ll;
fr.ur = context->ur;
fr.reportFixed = true;
fr.feps = context->feps;
len = context->aryObj.length;
if (len == 5) return; /* just a bbox */
argument = context->aryObj.val.arrayval;
last = argument + (len - 1);
AssertCheck(last->type == nameObj || last->type == cmdObj);
  /* last array element is a name or command */
argument += 5; /* skip the bbox */
bb = context->fbbox;
Assert(state == initState);
while (argument <= last) {
  val = vals; args = 0;
  while (true) {
    switch (argument->type) {
      case realObj: *(val++) = pflttofix(&argument->val.rval); break;
      case intObj:  *(val++) = FixInt(argument->val.ival); break;
      case nameObj: opval = argument->val.nmval; goto nextOp;
      case cmdObj:  opval = argument->val.cmdval; goto nextOp;
      default: TypeCheck();
      }
    argument++;
    args++;
    AssertCheck(args <= 6);
    }
  nextOp:
  switch (args) {
    case 0: {
      if (opval != closepathNm) TypeCheck();
      switch (state) {
        case currentState:
          (*stproc)(sp);
          (*clsproc)();
          break;
        case pathState:
          (*clsproc)();
          cp = sp;
          state = currentState;
          break;
        case initState:
          break;
        }
      break;
      }
    case 1: TypeCheck(); break;
    case 2: {
      x = vals[0]; y = vals[1];
      switch (mtxtype) {
        case identity: break;
	case flipY: y = -y; break;
        case bc_zero:
          x = fixmul(x, fmtx->a);
          y = fixmul(y, fmtx->d);
          break;
        case ad_zero:
          x = fixmul(y, fmtx->c);
          y = fixmul(vals[0], fmtx->b);
          break;
        case general:
          x = fixmul(x, fmtx->a) + fixmul(y, fmtx->c);
          y = fixmul(vals[0], fmtx->b) + fixmul(y, fmtx->d);
          break;
        }
      if (opval == linetoNm || opval == movetoNm) {
        x += fmtx->tx; y += fmtx->ty; }
      else if (opval == rlinetoNm || opval == rmovetoNm) {
        x += cp.x; y += cp.y; }
      else TypeCheck();
      if (x < bb.bl.x || x > bb.tr.x ||
          y < bb.bl.y || y > bb.tr.y) RangeCheck();
      if (opval == linetoNm || opval == rlinetoNm) {
        switch (state) {
          case currentState:
            (*stproc)(cp);
            state = pathState;
            /* fall through to call ltproc */
          case pathState: {
	    DevCd cd; cd.x = x; cd.y = y;
            (*ltproc)(cd);
	    }
            break;
          case initState: TypeCheck(); break;
          }
        }
      else { /* moveto or rmoveto */
        sp.x = x; sp.y = y;
        switch (state) {
          case currentState: break;
          case pathState:
            (*endproc)();
            state = currentState;
            break;
          case initState:
            state = currentState;
            break;
          }
        }
      cp.x = x; cp.y = y;
      break;
      }
    case 3: TypeCheck(); break;
    case 4: TypeCheck(); break;
    case 5: {
      Cd c1, c2;
      real r;
      context->state = state;
      fixtopflt(cp.x, &context->cp.x);
      fixtopflt(cp.y, &context->cp.y);
      fixtopflt(sp.x, &context->sp.x);
      fixtopflt(sp.y, &context->sp.y);
      fixtopflt(vals[0], &c1.x);
      fixtopflt(vals[1], &c1.y);
      fixtopflt(vals[2], &c2.x);
      fixtopflt(vals[3], &c2.y);
      fixtopflt(vals[4], &r);
      if (opval == arcnNm || opval == arcNm)
        QUsrPthArc(c1, c2.x, c2.y, r,
                   (boolean)(opval == arcNm), context);
      else if (opval == arctNm)
        QUsrPthArcTo(c1, c2, r, context);
      else TypeCheck();
      state = context->state;
      cp.x = pflttofix(&context->cp.x);
      cp.y = pflttofix(&context->cp.y);
      sp.x = pflttofix(&context->sp.x);
      sp.y = pflttofix(&context->sp.y);
      break;
      }
    case 6: {
      if (opval == rcurvetoNm || opval == curvetoNm) {
        val = vals;
        for (args = 3; args > 0; args--) {
          x = val[0]; y = val[1];
          switch (mtxtype) {
            case identity: break;
	    case flipY: y = -y; break;
            case bc_zero:
              x = fixmul(x, fmtx->a);
              y = fixmul(y, fmtx->d);
              break;
            case ad_zero:
              x = fixmul(y, fmtx->c);
              y = fixmul(val[0], fmtx->b);
              break;
            case general:
              x = fixmul(x, fmtx->a) + fixmul(y, fmtx->c);
              y = fixmul(val[0], fmtx->b) + fixmul(y, fmtx->d);
              break;
            }
          if (opval == curvetoNm) {
            x += fmtx->tx; y += fmtx->ty; }
          else { x += cp.x; y += cp.y; }
          if (x < bb.bl.x || x > bb.tr.x ||
              y < bb.bl.y || y > bb.tr.y) RangeCheck();
          *(val++) = x;
          *(val++) = y;
          }
        switch (state) {
          case currentState:
            (*stproc)(cp);
            state = pathState;
            /* fall through to call ctproc */
          case pathState:
            (*ctproc)(true);
	    fr.limit = FLATTENLIMIT;
	    FFltnCurve(cp, *dc1, *dc2, *dc3, &fr, notTestRect);
            (*ctproc)(false);
            break;
          case initState: TypeCheck(); break;
          }
        cp = *dc3;
        }
      else TypeCheck();
      break;
      }
    default: TypeCheck();
    }
  argument++;
  }
switch (state) {
  case currentState:
    if (!context->noendst) (*stproc)(sp);
    break;
  case pathState:
    (*endproc)();
    break;
  case initState: break;
  }
context->state = initState;
}

#undef dc1
#undef dc2
#undef dc3

#undef cd1
#undef cd2
#undef cd3
#undef cd4
#undef BBoxTest

private boolean CheckIfEncodedUserPath(context)
  register PUserPathContext context; {
  register PObject args;
  Object ob[2], ar;
  if (context->aryObj.length == 2) {
    if (context->packed) {
      ar = context->aryObj;
      VMCarCdr(&ar, &ob[0]);
      VMCarCdr(&ar, &ob[1]);
      args = ob;
      }
    else
      args = context->aryObj.val.arrayval;
    if ((args[0].type == strObj ||
         args[0].type == arrayObj ||
	 args[0].type == pkdaryObj) &&
        args[1].type == strObj && args[1].length > 0) {
      if ((args[0].access & rAccess) == 0 || (args[1].access & rAccess) == 0)
        InvlAccess();
      context->ctrlstr = args[1].val.strval;
      context->ctrllen = args[1].length;
      SetupNumStrm(&args[0], &context->datastrm);
      if (context->datastrm.len < 4) TypeCheck();
      return true;
      }
    }
  return false;
  }

private boolean CheckPkdMtLt(pAryObj, pc1, pc2)
  PAryObj pAryObj; Cd *pc1, *pc2; {
  Object ob, ar;
  Cd cd;
  PNameEntry opval;
  ar = *pAryObj;
  VMCarCdr(&ar, &ob);
  switch (ob.type) {
    case realObj: cd.x = ob.val.rval; break;
    case intObj: cd.x = (Component) ob.val.ival; break;
    default: return false;
    }
  VMCarCdr(&ar, &ob);
  switch (ob.type) {
    case realObj: cd.y = ob.val.rval; break;
    case intObj: cd.y = (Component) ob.val.ival; break;
    default: return false;
    }
  VMCarCdr(&ar, &ob);
  switch (ob.type) {
    case nameObj: opval = ob.val.nmval; break;
    case cmdObj:  opval = ob.val.cmdval; break;
    default: return false;
    }
  if (opval != movetoNm) return false;
  TfmPCd(cd, &gs->matrix, pc1);
  VMCarCdr(&ar, &ob);
  switch (ob.type) {
    case realObj: cd.x = ob.val.rval; break;
    case intObj: cd.x = (Component) ob.val.ival; break;
    default: return false;
    }
  VMCarCdr(&ar, &ob);
  switch (ob.type) {
    case realObj: cd.y = ob.val.rval; break;
    case intObj: cd.y = (Component) ob.val.ival; break;
    default: return false;
    }
  VMCarCdr(&ar, &ob);
  switch (ob.type) {
    case nameObj: opval = ob.val.nmval; break;
    case cmdObj:  opval = ob.val.cmdval; break;
    default: return false;
    }
  if (opval == linetoNm)
    TfmPCd(cd, &gs->matrix, pc2);
  else if (opval == rlinetoNm)
    RTfmPCd(cd, &gs->matrix, *pc1, pc2);
  else return false;
  return true;
  }

public boolean UsrPthCheckMtLt(pAryObj, pc1, pc2)
  PAryObj pAryObj; Cd *pc1, *pc2; {
  register PObject ary, arg;
  register PNameEntry opval;
  Cd cd;
  if (pAryObj->type == pkdaryObj) {
    if (pAryObj->length != 6) return false;
    return CheckPkdMtLt(pAryObj, pc1, pc2);
    }
  if (pAryObj->length != 11) return false;
  ary = pAryObj->val.arrayval;
  arg = ary + 7; /* this should be moveto or rmoveto */
  switch (arg->type) {
    case nameObj: opval = arg->val.nmval; break;
    case cmdObj:  opval = arg->val.cmdval; break;
    default: return false;
    }
  if (opval != movetoNm) return false;
  arg = ary + 5;
  switch (arg->type) {
    case realObj: cd.x = arg->val.rval; break;
    case intObj: cd.x = (Component) arg->val.ival; break;
    default: return false;
    }
  arg++;
  switch (arg->type) {
    case realObj: cd.y = arg->val.rval; break;
    case intObj: cd.y = (Component) arg->val.ival; break;
    default: return false;
    }
  TfmPCd(cd, &gs->matrix, pc1);
  arg = ary + 10; /* this should be lineto or rlineto */
  switch (arg->type) {
    case nameObj: opval = arg->val.nmval; break;
    case cmdObj:  opval = arg->val.cmdval; break;
    default: return false;
    }
  if (opval != linetoNm  && opval != rlinetoNm) return false;
  arg = ary + 8;
  switch (arg->type) {
    case realObj: cd.x = arg->val.rval; break;
    case intObj: cd.x = (Component) arg->val.ival; break;
    default: return false;
    }
  arg++;
  switch (arg->type) {
    case realObj: cd.y = arg->val.rval; break;
    case intObj: cd.y = (Component) arg->val.ival; break;
    default: return false;
    }
  if (opval == linetoNm)
    TfmPCd(cd, &gs->matrix, pc2);
  else
    RTfmPCd(cd, &gs->matrix, *pc1, pc2);
  return true;
  }

public boolean UsrPthQRdcOk(context, fill)
  PUserPathContext context;
  boolean fill; {
/* reproduce the logic of QRdcOk from path.c */
/* the old reducer has a performance advantage at large sizes */
  integer dy, len;
  register PObject argument;
  if (context->encoded)
    len = (context->datastrm.len) >> 2; /* rough estimate */
  else
    len = (context->aryObj.length - 5) >> 1; /* rough estimate */
  { /* check the bbox */
  register BBox bb = &context->bbox;
  register integer th, ll, ur;
  th = 700 + ((len - 4) << 6); /* ad hoc function */
  ll = bb->bl.y; ur = bb->tr.y;
  if ((dy=ur-ll) > th) return false;
  }
  if (!fill) return true; /* offset fill is biased toward new reducer */
  if (len > 16) return true;
  if (dy < 100 && len > 8) return true;
  /* if the path is all lines use the old reducer */
  if (context->encoded) {
    register character *ss = context->ctrlstr;
    register integer clen = context->ctrllen;
    while (clen-- > 0) {
      switch (*ss++) {
        case 0: /* bbox */
	case 1: /* moveto */
	case 2: /* rmoveto */
          break;
	case 3: /* lineto */
        case 4: /* rlineto */
          clen--; ss++; break;
        case 10: /* closepath */
          break;
        default: /* curve or arc */
          return true;
        }
      }
    return false;
    }
  if (context->packed) {
    register PNameEntry opval;
    Object ob, ar;
    ar = context->aryObj;
    while (ar.length > 0) {
      VMCarCdr(&ar, &ob);
      switch (ob.type) {
        case nameObj:
	case cmdObj:
          opval = ob.val.nmval;
          if (opval == curvetoNm || opval == rcurvetoNm ||
              opval == arcNm || opval == arcnNm || opval == arctNm)
              return true;
          break;
        }
      }
    return false;
    }
  {
  register PNameEntry opval;
  register PObject limit;
  argument = context->aryObj.val.arrayval + 5;
  limit = argument + context->aryObj.length;
  while (argument < limit) {
    switch (argument->type) {
      case nameObj:
      case cmdObj:
        opval = argument->val.nmval;
        if (opval == curvetoNm || opval == rcurvetoNm ||
            opval == arcNm || opval == arcnNm || opval == arctNm)
            return true;
        break;
      }
    argument++;
    }
  return false;
  }
}

private procedure PreEnumerateSetup(context, start, line, close,
    end, curve, noendst, reps, reportFixed, testRect, ll, ur)
  register PUserPathContext context; 
  procedure (*start)();
  procedure (*line)();
  procedure (*close)();
  procedure (*end)();
  procedure (*curve)();
  boolean noendst, reportFixed;
  real reps;
  boolean testRect; DevCd ll, ur;
  {
    /* Standard initialization */
    context->state = initState;
    context->feps = dbltofix(reps);
    context->reps = reps;
    context->useFixed = reportFixed ||
       (boolean)
        (context->bbox.bl.x >= -fp16k &&
         context->bbox.bl.y >= -fp16k &&
         context->bbox.tr.x <=  fp16k &&
         context->bbox.tr.y <=  fp16k);

    /* Custom initialization */
    context->stproc = start;
    context->ltproc = line;
    context->clsproc = close;
    context->endproc = end;
    context->ctproc = curve;
    context->noendst = noendst;
    context->cp = gs->cp;
    context->testRect = testRect;
    context->ll = ll;
    context->ur = ur;
    context->reportFixed = reportFixed;
    }


public procedure FillUserPathEnumerate(
  context, startLine, closeEnd, reportFixed, testRect, ll, ur)
  PUserPathContext context;
  procedure (*startLine)();
  procedure (*closeEnd)();
  boolean reportFixed, testRect;
  DevCd ll, ur;
  {
  /* Initialize the context */
  PreEnumerateSetup(context, startLine, startLine, closeEnd,
      closeEnd, NoOp, true, gs->flatEps, reportFixed, testRect, ll, ur);
  /* Enumerate to fill */
  if (context->encoded) DoEUserPath(context);
  else DoUserPath(context);
  }

public procedure StrokeUserPathEnumerate(context, start, line, close,
    end, curve, noendst, reps, reportFixed, testRect, ll, ur)
  PUserPathContext context; 
  procedure (*start)();
  procedure (*line)();
  procedure (*close)();
  procedure (*end)();
  procedure (*curve)();
  boolean noendst;
  real reps;
  boolean reportFixed, testRect;
  DevCd ll, ur;
  {
  /* Initialize the context */
  PreEnumerateSetup(context, start, line, close, end, curve, noendst, reps,
                    reportFixed, testRect, ll, ur);
  /* Enumerate to stroke */
  if (context->encoded) DoEUserPath(context);
  else DoUserPath(context);
  }

private procedure FTfm(x, y, fmtx, mtxtype, ct)
  Fixed x, y; register PFixMtx fmtx; MtxType mtxtype; DevCd *ct; {
  Fixed x0;
  switch (mtxtype) {
    case identity: break;
    case flipY: y = -y; break;
    case bc_zero:
      x = fixmul(x, fmtx->a);
      y = fixmul(y, fmtx->d);
      break;
    case ad_zero:
      x0 = x;
      x = fixmul(y, fmtx->c);
      y = fixmul(x0, fmtx->b);
      break;
    case general:
      x0 = x;
      x = fixmul(x, fmtx->a) + fixmul(y, fmtx->c);
      y = fixmul(x0, fmtx->b) + fixmul(y, fmtx->d);
      break;
    }
  ct->x = x + fmtx->tx;
  ct->y = y + fmtx->ty;
  }  

public boolean QEnumOk(context) register PUserPathContext context; {
  PMtx mtx = &context->matrix;
  DevBBoxRec rbbox, fbbox;
    /* rbbox comes from real tfm calculations;
       fbbox comes from fixed tfms; */
  DevCd cd, bl, tr;
  register PFixMtx fmtx = &context->fmtx;
  MtxType mtxtype;
  if (context->packed) return false; /* no quick version for packed arrays */
  fmtx->a = pflttofix(&mtx->a);
  fmtx->b = pflttofix(&mtx->b);
  fmtx->c = pflttofix(&mtx->c);
  fmtx->d = pflttofix(&mtx->d);
  fmtx->tx = pflttofix(&mtx->tx);
  fmtx->ty = pflttofix(&mtx->ty);
  if (fmtx->b == 0 && fmtx->c== 0) {
    if (fmtx->a == FixInt(1) && fmtx->d == FixInt(1))
      mtxtype = identity;
    else if (fmtx->a == FixInt(1) && fmtx->d == FixInt(-1))
      mtxtype = flipY;
    else mtxtype = bc_zero;
    }
  else if (fmtx->a == 0 && fmtx->d == 0) mtxtype = ad_zero;
  else mtxtype = general;
  context->mtxtype = mtxtype;
  bl.x = pflttofix(&context->ubbox.bl.x);
  bl.y = pflttofix(&context->ubbox.bl.y);
  tr.x = pflttofix(&context->ubbox.tr.x);
  tr.y = pflttofix(&context->ubbox.tr.y);
  FTfm(bl.x, bl.y, fmtx, mtxtype, &cd);
  fbbox.bl = fbbox.tr = cd;
  FTfm(tr.x, tr.y, fmtx, mtxtype, &cd);
  BBoxUpdate(cd.x, fbbox.bl.x, fbbox.tr.x);
  BBoxUpdate(cd.y, fbbox.bl.y, fbbox.tr.y);
  FTfm(bl.x, tr.y, fmtx, mtxtype, &cd);
  BBoxUpdate(cd.x, fbbox.bl.x, fbbox.tr.x);
  BBoxUpdate(cd.y, fbbox.bl.y, fbbox.tr.y);
  FTfm(tr.x, bl.y, fmtx, mtxtype, &cd);
  BBoxUpdate(cd.x, fbbox.bl.x, fbbox.tr.x);
  BBoxUpdate(cd.y, fbbox.bl.y, fbbox.tr.y);
  fbbox.tr.x += 0x10000;
  fbbox.tr.y += 0x10000;
  fbbox.bl.x -= 0x10000;
  fbbox.bl.y -= 0x10000;
  context->fbbox = fbbox;
  rbbox.bl.x = pflttofix(&context->bbox.bl.x);
  rbbox.bl.y = pflttofix(&context->bbox.bl.y);
  rbbox.tr.x = pflttofix(&context->bbox.tr.x);
  rbbox.tr.y = pflttofix(&context->bbox.tr.y);

    { 	/* Compensate for os_labs having to be a macro.			   */
     register integer temp;
     return
       (((temp = rbbox.bl.x - fbbox.bl.x, os_labs(temp)) < 0x4000L) &&
	((temp = rbbox.bl.y - fbbox.bl.y, os_labs(temp)) < 0x4000L) &&
	((temp = rbbox.tr.x - fbbox.tr.x, os_labs(temp)) < 0x4000L) &&
	((temp = rbbox.tr.y - fbbox.tr.y, os_labs(temp)) < 0x4000L));
    }
}

public procedure QFillUserPathEnumerate(
  context, startLine, closeEnd, testRect, ll, ur)
  PUserPathContext context;
  procedure (*startLine)();
  procedure (*closeEnd)();
  boolean testRect; DevCd ll, ur;
  {
  /* Initialize the context */
  PreEnumerateSetup(context, startLine, startLine, closeEnd,
      closeEnd, NoOp, true, gs->flatEps, true, testRect, ll, ur);
  /* Quick Fill */
  if (context->encoded) QDoEUsrPth(context);
  else QDoUserPath(context);
  }

public procedure QStrokeUserPathEnumerate(context, start, line, close,
    end, curve, noendst, reps, testRect, ll, ur)
  PUserPathContext context; 
  procedure (*start)();
  procedure (*line)();
  procedure (*close)();
  procedure (*end)();
  procedure (*curve)();
  boolean noendst;
  real reps;
  boolean testRect;
  DevCd ll, ur;
  {
  /* Initialize the context */
  PreEnumerateSetup(context, start, line, close, end, curve, noendst, reps,
                    true, testRect, ll, ur);
  /* Quick Stroke */
  if (context->encoded) QDoEUsrPth(context);
  else QDoUserPath(context);
  }

private boolean CheckIfUCache(context)
  register PUserPathContext context; {
  Object ob, ar;
  if (context->encoded) {
    if (*(context->ctrlstr) == 11) {
      context->ctrllen--; context->ctrlstr++; return true; }
    return false;
    }
  AssertCheck(context->aryObj.length > 0);
  ar = context->aryObj;
  VMCarCdr(&ar, &ob);
  if ((ob.type == nameObj || ob.type == cmdObj) && ob.val.nmval == ucacheNm) {
    context->aryObj = ar;
    return true;
    }
  return false;
  }

public procedure GetUsrPthAry(context)
  register PUserPathContext context; {
  PopPArray(&context->aryObj);
  if ((context->aryObj.access & rAccess) == 0) InvlAccess();
  context->packed = (context->aryObj.type == pkdaryObj);
  context->encoded = CheckIfEncodedUserPath(context);
  context->ucache = CheckIfUCache(context);
  context->state = initState;
  }
  
#if DPSXA
private procedure UXAFillPath(pContext, evenOdd)
PUserPathContext pContext;
boolean evenOdd;
{
  FillPath(evenOdd, (char *)pContext, &pContext->bbox,
           UsrPthQRdcOk, FillUserPathEnumerate,
           QEnumOk, QFillUserPathEnumerate);
}
#endif DPSXA

private procedure FillUserPath(evenOdd) boolean evenOdd; {
  UserPathContext context;
  /* Obtain argument */
  GetUsrPthAry(&context);
  if (context.ucache) {
    context.fill = true; context.evenOdd = evenOdd;
    if (UCacheMark(&context, (PMtx)NULL))
      return;
    }
  UsrPthBBox(&context);
  /* Fill path */
#if DPSXA
  BreakUpPath(UXAFillPath, &context, evenOdd, true);
#else DPSXA
  FillPath(evenOdd, (char *)&context, &context.bbox,
           UsrPthQRdcOk, FillUserPathEnumerate,
           QEnumOk, QFillUserPathEnumerate);
#endif DPSXA
  }

public procedure PSUFill() { FillUserPath(false); }

public procedure PSUEOFill() { FillUserPath(true); }

public boolean CheckForMtx() {
  Object ar;
  TopP(&ar); /* top of operand stack */
  if (ar.length != 6) return false;
  if (ar.type == arrayObj) {
    register PObject argument = ar.val.arrayval;
    register PObject last = argument + 5;
    while (argument <= last) {
      switch (argument->type) {
        case realObj: case intObj: break;
	default: return false;
	}
      argument++;
      }
    return true;
    }
  if (ar.type == pkdaryObj) {
    Object ob;
    integer i;
    for (i = 0; i < 6; i++) {
      VMCarCdr(&ar, &ob);
      switch (ob.type) {
        case realObj: case intObj: break;
	default: return false;
	}
      }
    return true;
    }
  return false;
  }
  
#if DPSXA
private procedure UXAStroke(pContext,bool)
PUserPathContext pContext;
boolean bool;
{
  uXAc1.x += UOffset.x;
  uXAc1.y += UOffset.y;
  uXAc2.x += UOffset.x;
  uXAc2.y += UOffset.y;
  if (DoStroke(false, (char *)pContext, &pContext->bbox,
               uXARectangle, uXAc1, uXAc2, StrokeUserPathEnumerate,
               QEnumOk, QStrokeUserPathEnumerate, false,
               GetDevClipBBox(), DevClipIsRect(), (DevBBox)NULL)) {
    (*ms->procs->termMark)(ms);
    FinStroke();
    }
}
#endif DPSXA

public procedure PSUStroke() {
  UserPathContext context;
  boolean rectangle;
  Cd c1, c2;
  Mtx sMtx, gMtx;
  boolean ismtx = CheckForMtx();
  /* Obtain argument(s) */
  if (ismtx) PopMtx(&sMtx);
  GetUsrPthAry(&context);
  if (context.ucache) {
    context.fill = false;
    context.circletraps = false;
    if (UCacheMark(&context, ismtx? &sMtx : (PMtx)NULL))
      return;
    }
  UsrPthBBox(&context);
  /* check if path is a moveto lineto combination */
  if (context.encoded)
    rectangle = EUsrPthCheckMtLt(&context.aryObj, &c1, &c2);
  else
    rectangle = UsrPthCheckMtLt(&context.aryObj, &c1, &c2);
  /* Stroke path */
  if (ismtx) { gMtx = gs->matrix; Cnct(&sMtx); }
  DURING
#if DPSXA
  uXARectangle = rectangle;
  uXAc1 = c1;
  uXAc2 = c2;
  strokeOp = true;
  BreakUpPath(UXAStroke,&context,false,true);
  strokeOp = false;
#else /* DPSXA */
  if (DoStroke(false, (char *)&context, &context.bbox,
               rectangle, c1, c2, StrokeUserPathEnumerate,
               QEnumOk, QStrokeUserPathEnumerate, false,
               GetDevClipBBox(), DevClipIsRect(), (DevBBox)NULL)) {
    (*ms->procs->termMark)(ms);
    FinStroke();
    }
#endif /* DPSXA */
  HANDLER { if (ismtx) SetMtx(&gMtx); RERAISE; }
  END_HANDLER;
  if (ismtx) SetMtx(&gMtx);
  }

private procedure UPMoveTo(cd, context)
  Cd cd; PUserPathContext context; {
  context->doneMoveTo = true;
  MoveTo(cd, &gs->path);
  }

private procedure UPLineTo(cd, context)
  Cd cd; PUserPathContext context; {
  AssertCheck(context->doneMoveTo);
  LineTo(cd, &gs->path);
  }

private procedure UPCurveTo(cd1, cd2, cd3, context)
  Cd cd1, cd2, cd3; PUserPathContext context; {
  AssertCheck(context->doneMoveTo);
  CurveTo(cd1, cd2, cd3, &gs->path); }

private procedure UPClosePath(context)
  PUserPathContext context; {
  AssertCheck(context->doneMoveTo);
  ClosePath(&gs->path);
  }

private boolean DoUAppend(context, initctx, getdp)
  PUserPathContext context, initctx; boolean getdp; {
  boolean ucache;
  DevPrim *dp, *dp2;
  GetUsrPthAry(context);
  ucache = (context->ucache && IsPathEmpty(&gs->path));
  if (ucache) *initctx = *context;
  UsrPthBBox(context);
  context->state = appendState;
  context->MoveTo = UPMoveTo;
  context->LineTo = UPLineTo;
  context->CurveTo = UPCurveTo;
  context->ClosePath = UPClosePath;
  context->doneMoveTo = false;
#if DPSXA
  UOffset.x = UOffset.y = 0;
#endif /* DPSXA */
  if (context->encoded) DoEUserPath(context);
  else DoUserPath(context);
  if (!getdp || !ucache) return ucache;
  initctx->fill = true; initctx->evenOdd = false;
  dp = UCGetDevPrim(initctx, (PMtx)NULL);
  if (!dp) return true;
  if (!initctx->dispose) {
    dp2 = NULL; 
    DURING
    dp2 = CopyDevPrim(dp);
    HANDLER { SetAbort((integer)0); return true; }
    END_HANDLER;
    dp = dp2; dp2 = NULL;
    }
  DURING
  PutRdc(&gs->path, dp, false);
  HANDLER { DisposeDevPrim(dp); SetAbort((integer)0); }
  END_HANDLER;
  return true;
  }

public procedure PSUAppend() {
  UserPathContext context, initctx;
  DoUAppend(&context, &initctx, true); }


private boolean PSExecOpNm(name) NameObj name; {
  CmdObj cob;
  DictGetP(rootShared->vm.Shared.sysDict, name, &cob);
  if (psExecute(cob)) {
    if (GetAbort() == PS_EXIT) SetAbort((integer)0);
    return true;
    }
  return false;
  }

public procedure PSUPath() {
  register PPthElt pe;
  register PCd pcd;
  Mtx im;
  Cd cd1, cd2;
  AryObj ao;
  PObject p;
  boolean ucache;
  register integer ecnt;
  register PPath path = &gs->path;
  pcd = &cd1;
  ucache = PopBoolean();
  CheckForCurrentPoint(path);
  if (path->secret) PSError(invlaccess); /* NOT InvlAccess() */
  MtxInvert(&gs->matrix, &im);
  if ((PathType)path->type != listPth) ConvertToListPath(path);
  /* if we get back an empty path, this should be equivalent to a
     null path in the first place! */
  if (path->ptr.lp == NULL) NoCurrentPoint();
  ecnt = 0; /* this tells how big the array will need to be */
  if (ucache) ecnt++; /* for ucache */
  ecnt += 5; /* for setbbox */
  for (pe = path->ptr.lp->head; pe != 0; pe = pe->next) {
    switch (pe->tag) {
      case pathstart:
      case pathlineto:
        ecnt += 3;
        break;
      case pathcurveto:
        ecnt += 7;
        pe = pe->next;  
        pe = pe->next;
        break;
      case pathclose:
        ecnt++;
        break;
      default: break;
      }
    };
  PushInteger(ecnt);
  if (PSExecOpNm(graphicsNames[nm_array]) ||
      PSExecOpNm(graphicsNames[nm_cvx])) return;
  TopP(&ao);
  p = ao.val.arrayval;
  if (ucache) *p++ = graphicsNames[nm_ucache];
  GetPathBBoxUserCds(&cd1, &cd2);
  LRealObj(*p, cd1.x); p++;
  LRealObj(*p, cd1.y); p++;
  LRealObj(*p, cd2.x); p++;
  LRealObj(*p, cd2.y); p++;
  *p++ = graphicsNames[nm_setbbox];
  for (pe = path->ptr.lp->head; pe != 0; pe = pe->next) {
    switch (pe->tag) {
      case pathstart:
      case pathlineto:
        TfmPCd(pe->coord, &im, pcd);
	LRealObj(*p, cd1.x); p++;
	LRealObj(*p, cd1.y); p++;
	if (pe->tag == pathstart) *p++ = graphicsNames[nm_moveto];
        else *p++ = graphicsNames[nm_lineto];
        break;
      case pathcurveto:
        TfmPCd(pe->coord, &im, pcd);
	LRealObj(*p, cd1.x); p++;
	LRealObj(*p, cd1.y); p++;
        pe = pe->next;  
        TfmPCd(pe->coord, &im, pcd);
	LRealObj(*p, cd1.x); p++;
	LRealObj(*p, cd1.y); p++;
        pe = pe->next;
        TfmPCd(pe->coord, &im, pcd);
	LRealObj(*p, cd1.x); p++;
	LRealObj(*p, cd1.y); p++;
	*p++ = graphicsNames[nm_curveto];
        break;
      case pathclose:
	*p++ = graphicsNames[nm_closepath];
        break;
      default: break;
      }
    };
  } /* end of PSUPath */

typedef struct {
  PUserPathContext ctx;
  boolean ismtx;
  PMtx gmtx, smtx;
  } StrkPthCtx;

private DevPrim * UStrkPthProc(spctx) StrkPthCtx *spctx; {
  PUserPathContext ctx = spctx->ctx;
  Mtx cmtx;
  DevPrim *dp;
  ctx->fill = false;
  ctx->circletraps = true;
  if (spctx->ismtx) {
    cmtx = gs->matrix;
    SetMtx(spctx->gmtx);
    DURING
    dp = UCGetDevPrim(ctx, spctx->smtx);
    HANDLER { SetMtx(&cmtx); RERAISE; }
    END_HANDLER;
    SetMtx(&cmtx);
    }
  else dp = UCGetDevPrim(ctx, (PMtx)NULL);
  if (!dp) {
    extern DevPrim * StrkPthProc();
    return StrkPthProc(&gs->path);
    }
  if (!ctx->dispose) dp = CopyDevPrim(dp);
  return dp;
  }

public procedure PSUStrokePath() {
  Mtx sMtx, gMtx;
  UserPathContext context, initctx;
  StrkPthCtx spctx;
  boolean ucache;
  boolean ismtx = CheckForMtx();
  if (ismtx) { PopMtx(&sMtx); gMtx = gs->matrix; }
  NewPath();
  ucache = DoUAppend(&context, &initctx, false);
  if (ismtx) Cnct(&sMtx);
  DURING
  if (ucache) {
    spctx.ctx = &initctx;
    spctx.ismtx = ismtx;
    spctx.gmtx = &gMtx;
    spctx.smtx = &sMtx;
    DoStrkPth(UStrkPthProc, (char *)&spctx);
    }
  else PSStrkPth();
  HANDLER { if (ismtx) SetMtx(&gMtx); RERAISE; }
  END_HANDLER;
  if (ismtx) SetMtx(&gMtx);
  }

