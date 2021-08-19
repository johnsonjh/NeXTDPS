/*
  euserpath.c

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
Mike Schuster: Fri May 22 08:37:57 1987
Bill Paxton: Thu Apr 28 10:03:26 1988
Ivor Durham: Sat Oct 15 06:27:29 1988
Jim Sandman: Thu Apr 13 14:45:04 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
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

private character ReadCtrlByte(context) register PUserPathContext context; {
  register character *ss;
  Assert(context->ctrllen > 0); /* already checked by caller */
  context->ctrllen--;
  ss = context->ctrlstr;
  context->ctrlstr++;
  return *ss;
  }

private integer ComputeDataLen(context) PUserPathContext context; {
  /* returns how many numbers must be in data to satisfy control */
  register integer len = 0, cnt;
  register character *ss = context->ctrlstr, *endss, c;
  endss = ss + context->ctrllen;
  while (ss < endss) {
    c = *ss++;
    if (c > 32) {
      cnt = c - 32;
      if (ss == endss) TypeCheck();
      c = *ss++;
      }
    else cnt = 1;
    switch (c) {
      case 0: len += cnt << 2; break;
      case 1: case 2: len += cnt << 1; break;
      case 3: case 4: len += cnt << 1; break;
      case 5: case 6: len += cnt * 6; break;
      case 7: case 8: case 9: len += cnt * 5; break;
      case 10: case 11: break;
      default: TypeCheck();
      }
    }
  return len;
  }

public procedure GetEUsrPthBBox(context, vals)
  PUserPathContext context; real *vals; {
  /* vals has room for llx lly urx ury of bbox */
  integer i;
  PNumStrm s = &context->datastrm;
  if (ComputeDataLen(context) != s->len) TypeCheck();
  if (ReadCtrlByte(context) != 0) TypeCheck();
  for (i = 0; i < 4; i++)
    (*s->GetReal)(s, &vals[i]);
  }

public procedure DoEUserPath(context)
  register PUserPathContext context; {
  register PNumStrm ds = &context->datastrm;
  register integer count, i;
  real vals[6];
  boolean absFlg;
  FltnRec fr;
  procedure (*getreal)() = ds->GetReal;
#if DPSXA
  unsigned short save_ctrllen;
  PCard8 save_ctrlstr, save_str;
  PObject save_aptr;
  struct _t_Object save_ao;
  /* Since extended addressing will cause this routine to be called more than
     once with the same context, the following values need to be saved.
  */ 
  save_ctrllen = context->ctrllen;
  save_ctrlstr = context->ctrlstr;
  save_aptr = ds->aptr;
  save_ao = ds->ao;
  save_str = ds->str;
#endif DPSXA  
  fr.report = context->ltproc;
  fr.reps = context->reps;
  fr.ll = context->ll;
  fr.ur = context->ur;
  fr.reportFixed = context->reportFixed;
  fr.feps = context->feps;
  while (context->ctrllen > 0) {
    absFlg = false;
    i = ReadCtrlByte(context);
    if (i > 32) {
      count = i - 32;
      i = ReadCtrlByte(context);
      }
    else count = 1;
    switch (i) {
      case 1: /* moveto */ absFlg = true;
      case 2: /* rmoveto */
        while (count-- > 0) {
          (*getreal)(ds, &cd1->x);
	  (*getreal)(ds, &cd1->y);
          UsrPthDoMoveTo(context, *cd1, absFlg);
	  }
        break;
      case 3: /* lineto */ absFlg = true;
      case 4: /* rlineto */
        while (count-- > 0) {
          (*getreal)(ds, &cd1->x);
	  (*getreal)(ds, &cd1->y);
          UsrPthDoLineTo(context, *cd1, absFlg);
          }
        break;
      case 5: /* curveto */ absFlg = true;
      case 6: /* rcurveto */
        while (count-- > 0) {
	  /*
	    id: GCC optimizer gets a fatal signal in cc1 on this for loop:
		  for (i = 0; i < 6; i++)
	            (*getreal)(ds, &vals[i]);
	    Recoding as a while loop circumvents the problem:
	   */
	  i = 0;
	  while (i < 6)
            (*getreal)(ds, &vals[i++]);
          UsrPthDoCurveTo(context, vals, &fr, absFlg);
          }
        break;
      case 7: /* arc */ absFlg = true;
      case 8: /* arcn */
        while (count-- > 0) {
          for (i = 0; i < 5; i++)
            (*getreal)(ds, &vals[i]);
	  UsrPthArc(*cd1, vals[2], vals[3], vals[4], absFlg, context);
	  }
        break;
      case 9: /* arc2 */
        while (count-- > 0) {
          for (i = 0; i < 5; i++)
            (*getreal)(ds, &vals[i]);
	  UsrPthArcTo(*cd1, *cd2, vals[4], context);
	  }
        break;
      case 10: /* closepath */
        UsrPthDoClsPth(context);
        break;
      default: TypeCheck();
      }
    }
  UsrPthDoFinish(context);
#if DPSXA
  context->ctrllen = save_ctrllen;
  context->ctrlstr = save_ctrlstr;
  ds->aptr = save_aptr;
  ds->ao = save_ao;
  ds->str = save_str;
#endif DPSXA
  }

#define dc1 ((DevCd *) &vals[0])
#define dc2 ((DevCd *) &vals[2])
#define dc3 ((DevCd *) &vals[4])

public procedure QDoEUsrPth(context)
register PUserPathContext context; {
  PNumStrm ds = &context->datastrm;
  integer count, i;
  Fixed vals[6];
  Fixed (*getfixed)() = ds->GetFixed;
  register Fixed *val, x, y;
  register Card32 u;
  character *cmds = context->ctrlstr, cmd, *endcmds;
  DevCd cp, sp;
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
  boolean absFlg, movetoFlg;
  FltnRec fr;

  /* Try to do it the fast way */
  if (QDoEUsrPthFloat(context)) return;

  Assert(!context->packed);
  endcmds = cmds + context->ctrllen;
  fr.report = ltproc;
  fr.reps = context->reps;
  fr.ll = context->ll;
  fr.ur = context->ur;
  fr.reportFixed = true;
  fr.feps = context->feps;
  bb = context->fbbox;
  Assert(state == initState);
  while (cmds < endcmds) {
    absFlg = movetoFlg = false;
    cmd = *cmds++;
    if (cmd > 32) {
      count = cmd - 32;
      cmd = *cmds++;
      }
    else count = 1;
    switch (cmd) {
      case 1: /* moveto */ absFlg = true;
      case 2: /* rmoveto */ movetoFlg = true;
      case 3: /* lineto */ if (!movetoFlg) absFlg = true;
      case 4: /* rlineto */
	while (count-- > 0) {
          x = (*getfixed)(ds); vals[0] = x;
	  y = (*getfixed)(ds); 
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
          if (absFlg) { x += fmtx->tx; y += fmtx->ty; }
          else { x += cp.x; y += cp.y; }
          if (x < bb.bl.x || x > bb.tr.x ||
              y < bb.bl.y || y > bb.tr.y) RangeCheck();
          if (movetoFlg) {
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
          else { /* lineto */
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
          cp.x = x; cp.y = y;
	  }
        break;
      case 5: /* curveto */ absFlg = true;
      case 6: /* rcurveto */
        while (count-- > 0) {
          val = vals;
          for (i = 3; i > 0; i--) {
	    x = (*getfixed)(ds); val[0] = x;
	    y = (*getfixed)(ds);
            switch (mtxtype) {
              case identity:
                break;
	      case flipY:
	        y = -y;
	        break;
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
            if (absFlg) { x += fmtx->tx; y += fmtx->ty; }
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
        break;
      case 7: /* arc */ absFlg = true;
      case 8: /* arcn */
      case 9: /* arc2 */
        while (count-- > 0) {
          Cd c1, c2;
          real r;
          context->state = state;
          fixtopflt(cp.x, &context->cp.x);
          fixtopflt(cp.y, &context->cp.y);
          fixtopflt(sp.x, &context->sp.x);
          fixtopflt(sp.y, &context->sp.y);
	  (*ds->GetReal)(ds, &c1.x);
	  (*ds->GetReal)(ds, &c1.y);
	  (*ds->GetReal)(ds, &c2.x);
	  (*ds->GetReal)(ds, &c2.y);
	  (*ds->GetReal)(ds, &r);
          if (cmd != 9)
            QUsrPthArc(c1, c2.x, c2.y, r, absFlg, context);
          else
            QUsrPthArcTo(c1, c2, r, context);
          state = context->state;
          cp.x = pflttofix(&context->cp.x);
          cp.y = pflttofix(&context->cp.y);
          sp.x = pflttofix(&context->sp.x);
          sp.y = pflttofix(&context->sp.y);
	  }
        break;
      case 10: /* closepath */
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
      default: TypeCheck();
      }
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

public boolean EUsrPthCheckMtLt(pAryObj, pc1, pc2)
PAryObj pAryObj; PCd pc1, pc2; {
register PObject argument;
return false;
}

extern Fixed HNRF();
extern procedure NoOp();

public int QDoEUsrPthFloat(context)
register PUserPathContext context; {
  PNumStrm ds = &context->datastrm;
  integer count, i;
  Fixed (*getfixed)() = ds->GetFixed;
  Fixed x, y, tx, ty, cx, cy, sx, sy;
  character *cmds = context->ctrlstr, cmd;
  MtxType mtxtype = context->mtxtype;
  PathState state = context->state;
  DevBBoxRec bb;
  DevBounds bounds;
  float *fp;

  /* This code deals only with long-aligned Homogeneous arrays of floats,
   * and identity or flipped Y matrices.  It will abort and return 0 if
   * anything other than [r]moveto, [r]lineto, or closepath is encountered.
   * Fortunately, no state is changed by this procedure.  Only use this when
   * drawing unadjusted zero-width lines (proc vector for end is a NoOp, and
   * minTrapPrecision is 0).
   */
  if (((int)(ds->str) & 0x3) || !(mtxtype == identity || mtxtype == flipY) ||
      getfixed != HNRF || context->endproc != NoOp || minTrapPrecision)
      return 0;

  /* We begin with    state = initState */
  fp = (float *) ds->str;
  count = context->ctrllen;
  bb = context->fbbox;
  tx = context->fmtx.tx;
  ty = context->fmtx.ty;

  bounds.x.l = FTrunc(bb.bl.x);
  bounds.x.g = FTrunc(bb.tr.x) + 1;
  bounds.y.l = FTrunc(bb.bl.y);
  bounds.y.g = FTrunc(bb.tr.y) + 1;
  ms->trapsDP->bounds = bounds;

  while (count-- != 0) {
    cmd = *cmds++;
    if (cmd > 0 && cmd < 5) { /* 1=moveto, 2=rmoveto, 3=lineto, 4=rlineto */
      x = pflttofix(fp); fp++;
      y = pflttofix(fp); fp++;
      if (mtxtype != identity) y = -y;

      if (cmd & 1) { x += tx;	y += ty; }
      else	   { x += cx;	y += cy; }
      if (x < bb.bl.x || x > bb.tr.x ||
          y < bb.bl.y || y > bb.tr.y) RangeCheck();
      if (cmd < 3) { /* [r]moveto */
        sx = x; sy = y;
        state = currentState;
      } else { /* [r]lineto */
	if (state == initState) TypeCheck();
	state = pathState;
	QBresenhamMT(bounds, cx, cy, x-cx, y-cy);
      }
      cx = x; cy = y;
    } else if (cmd == 10) { /* 10=closepath */
      if (state == pathState) {
	QBresenhamMT(bounds, cx, cy, sx-cx, sy-cy);
	cx = sx; cy = sy;
	state = currentState;
      }
    } else return 0;
  }

  return 1;
}

