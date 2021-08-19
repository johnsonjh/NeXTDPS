/* PostScript font's matrix package

		Copyright 1983 -- Adobe Systems, Inc.
	    PostScript is a trademark of Adobe Systems, Inc.
NOTICE:  All information contained herein or attendant hereto is, and
remains, the property of Adobe Systems, Inc.  Many of the intellectual
and technical concepts contained herein are proprietary to Adobe Systems,
Inc. and may be covered by U.S. and Foreign Patents or Patents Pending or
are protected as trade secrets.  Any dissemination of this information or
reproduction of this material are strictly forbidden unless prior written
permission is obtained from Adobe Systems, Inc.

Original version: John Warnock, Mar 29, 1983
Edit History:
John Warnock: Mon Apr 11 15:39:50 1983
Doug Brotz: Tue Mar 28 15:55:39 1989
Chuck Geschke: Fri Oct 11 07:18:47 1985
Ed Taft: Mon Oct 12 15:53:52 1987
Dick Sweet: Tue Mar 24 17:08:09 PST 1987
Don Andrews: Wed Jul  9 18:12:45 1986
Ivor Durham: Tue Jan 20 13:15:41 1987
Ken Lent: Tue Apr 12 15:19:36 1988
Chansler: Mon Oct 24 14:59:02 1988: XYFTfmPDevCd, FixPMtxToPFMtx.
Bill Paxton: Thu Sep 14 15:30:43 1989
Scott Byer: Tue Jun 27 15:55:00 1989
Jim Sandman: Thu Nov  9 09:19:12 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include "fontdata.h"

#define fntmtx (fontCtx->fontBuild._fntmtx)

/* for CScan */

public procedure FntITfmP(cd, pcd) FCd cd, *pcd; {
  (*fntmtx.itfm)(cd, pcd);
  }
  
/* The following set of procedures are the transfomation procedures.
   There are quite a few of them.  See the assignments in SetMtx to
   see how they are used.
   */
   
private procedure bcz_fixed_tfm(c, ct) FCd c; PFCd ct; 
  { register Fixed x=c.x, y=c.y;  register PFixMtx fx = &(fntmtx.m.fx);
    x = fixmul(x, fx->a);  x += fx->tx; 
    y = fixmul(y, fx->d);  y += fx->ty;
    ct->x = x;  ct->y = y; }

private procedure bcz_frac_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y;  register PFracMtx fr = &(fntmtx.m.fr);
    x = fxfrmul(x, fr->a);  x += fr->tx >> 14; 
    y = fxfrmul(y, fr->d);  y += fr->ty >> 14;
    ct->x = x;  ct->y = y; }

private procedure adz_fixed_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y, x0;  register PFixMtx fx = &(fntmtx.m.fx);
    x0 = x;  x = fixmul(y, fx->c);  x += fx->tx; 
    y = fixmul(x0, fx->b);  y += fx->ty;
    ct->x = x;  ct->y = y; }

private procedure adz_frac_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y, x0;  register PFracMtx fr = &(fntmtx.m.fr);
    x0 = x;  x = fxfrmul(y, fr->c);  x += fr->tx >> 14; 
    y = fxfrmul(x0, fr->b);  y += fr->ty >> 14;
    ct->x = x;  ct->y = y; }

private procedure gen_fixed_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y, x0;  register PFixMtx fx = &(fntmtx.m.fx);
    x0 = x;  x = fixmul(x, fx->a);  x += fixmul(y, fx->c);  x += fx->tx; 
    y = fixmul(y, fx->d);  y += fixmul(x0, fx->b);  y += fx->ty;
    ct->x = x;  ct->y = y; }

private procedure gen_frac_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y, x0;  register PFracMtx fr = &(fntmtx.m.fr);
    x0 = x; x = fxfrmul(x, fr->a); x += fxfrmul(y, fr->c); x += fr->tx >> 14; 
    y = fxfrmul(y, fr->d);  y += fxfrmul(x0, fr->b);  y += fr->ty >> 14;
    ct->x = x;  ct->y = y; }

private procedure d_bcz_fixed_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y;  register PFixMtx fx = &(fntmtx.m.fx);
    x = fixmul(x, fx->a);  y = fixmul(y, fx->d);  ct->x = x;  ct->y = y; }

private procedure d_bcz_frac_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y;  register PFracMtx fr = &(fntmtx.m.fr);
    x = fxfrmul(x, fr->a);  y = fxfrmul(y, fr->d);  ct->x = x;  ct->y = y; }

private procedure d_adz_fixed_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y, x0;  register PFixMtx fx = &(fntmtx.m.fx);
    x0 = x; x = fixmul(y, fx->c); y = fixmul(x0, fx->b);
    ct->x = x; ct->y = y; }

private procedure d_adz_frac_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y, x0;  register PFracMtx fr = &(fntmtx.m.fr);
    x0 = x;  x = fxfrmul(y, fr->c);  y = fxfrmul(x0, fr->b);
    ct->x = x;  ct->y = y; }

private procedure d_gen_fixed_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y, x0;  register PFixMtx fx = &(fntmtx.m.fx);
    x0 = x;  x = fixmul(x, fx->a);  x += fixmul(y, fx->c);
    y = fixmul(y, fx->d);  y += fixmul(x0, fx->b);  ct->x = x;  ct->y = y; }

private procedure d_gen_frac_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y, x0;  register PFracMtx fr = &(fntmtx.m.fr);
    x0 = x;  x = fxfrmul(x, fr->a);  x += fxfrmul(y, fr->c);
    y = fxfrmul(y, fr->d);  y += fxfrmul(x0, fr->b);  ct->x = x;  ct->y = y; }

private procedure inv_bcz_fixed_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y;  register PFixMtx fx = &(fntmtx.im);
    x = fixmul(x, fx->a);  x += fx->tx;  y = fixmul(y, fx->d);  y += fx->ty;
    ct->x = x;  ct->y = y; }

private procedure inv_adz_fixed_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y, x0;  register PFixMtx fx = &(fntmtx.im);
    x0 = x;  x = fixmul(y, fx->c);  x += fx->tx;  y = fixmul(x0, fx->b);
    y += fx->ty;  ct->x = x;  ct->y = y; }

private procedure inv_gen_fixed_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y, x0;  register PFixMtx fx = &(fntmtx.im);
    x0 = x;  x = fixmul(x, fx->a);  x += fixmul(y, fx->c);  x += fx->tx; 
    y = fixmul(y, fx->d);  y += fixmul(x0, fx->b);   y += fx->ty;
    ct->x = x;  ct->y = y; }

private procedure inv_d_bcz_fixed_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y;  register PFixMtx fx = &(fntmtx.im);
    x = fixmul(x, fx->a);  y = fixmul(y, fx->d);  ct->x = x;  ct->y = y; }

private procedure inv_d_adz_fixed_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y, x0;  register PFixMtx fx = &(fntmtx.im);
    x0 = x;  x = fixmul(y, fx->c);  y = fixmul(x0, fx->b);
    ct->x = x;  ct->y = y; }

private procedure inv_d_gen_fixed_tfm(c, ct) FCd c; PFCd ct;
  { register Fixed x=c.x, y=c.y, x0;  register PFixMtx fx = &(fntmtx.im);
    x0 = x;  x = fixmul(x, fx->a);  x += fixmul(y, fx->c);
    y = fixmul(y, fx->d);  y += fixmul(x0, fx->b);  ct->x = x;  ct->y = y; }
  
/* 	Procedure to set the current matrix 'gsmatrix' to a new matrix.
	Sets the matrix and then switches in procedures into the global
	transform procedure structure.
	If newmatrix actually points to NULL, nothing is done.
	*/

private procedure SetMtxProcs()
  {     

  /* Assign the tfm and dtfm procs. */
  switch (fntmtx.mtxtype) {
   case bc_zero:
	 if (fntmtx.isFixed) { 
	   fntmtx.dtfm = d_bcz_fixed_tfm;
	   fntmtx.tfm = bcz_fixed_tfm;
	 } else {
	   fntmtx.dtfm = d_bcz_frac_tfm;
	   fntmtx.tfm = bcz_frac_tfm;
	 }
	 break;
   case ad_zero:
	 if (fntmtx.isFixed) { 
	   fntmtx.dtfm = d_adz_fixed_tfm;
	   fntmtx.tfm = adz_fixed_tfm;
	 } else {
	   fntmtx.dtfm = d_adz_frac_tfm;
	   fntmtx.tfm = adz_frac_tfm;
	 }
	 break;
   case general:
   default:
	 if (fntmtx.isFixed) { 
	   fntmtx.dtfm = d_gen_fixed_tfm;
	   fntmtx.tfm = gen_fixed_tfm;
	 } else {
	   fntmtx.dtfm = d_gen_frac_tfm;
	   fntmtx.tfm = gen_frac_tfm;
	 }
	 break;
   }
  
  /* Can the regular transform be optimized to a delta transform? */
  if ((fntmtx.m.fx.tx == 0) && (fntmtx.m.fx.ty == 0)) {
  	fntmtx.tfm = fntmtx.dtfm;
  	}

  /* Assign the itfm and idtfm procs. */
  switch (fntmtx.imtxtype) {
   case bc_zero:
	 fntmtx.idtfm = inv_d_bcz_fixed_tfm;
	 fntmtx.itfm = inv_bcz_fixed_tfm;
	 break;
   case ad_zero:
	 fntmtx.idtfm = inv_d_adz_fixed_tfm;
	 fntmtx.itfm = inv_adz_fixed_tfm;
	break;
   case general:
   default:
	 fntmtx.idtfm = inv_d_gen_fixed_tfm;
	 fntmtx.itfm = inv_gen_fixed_tfm;
	 break;
   }
  
  /* Can the inverse transform be optimized to a delta transform? */
  if ((fntmtx.im.tx == 0) && (fntmtx.im.ty == 0)) {
  	fntmtx.itfm = fntmtx.idtfm;
  	}
  }

public Fixed ApproxDLen(cp) register PFCd cp;
{
/* returns the approximate device length of the vector in cp */
(*fntmtx.dtfm)(*cp, cp);
if (cp->x < 0) cp->x = -cp->x;
if (cp->y < 0) cp->y = -cp->y;
/* 22046 == Fixed representation of .3364 */
if (cp->x >= cp->y) return cp->x + fixmul(cp->y, (Fixed)22046);
else return cp->y + fixmul(cp->x, (Fixed)22046);
/* within 5.5% of actual length */
}  /* end of ApproxDLenP */

private Fixed Len1000() {
  FCd c; Fixed l1, l2;
  c.x = FixInt(1000); c.y = 0; l1 = ApproxDLen(&c);
  c.x = 0; c.y = FixInt(1000); l2 = ApproxDLen(&c);
  return MAX(l1, l2);
  }

public boolean SetupFntMtx()
  /* return true if setup ok.  false if cannot use fixed tfms */
  {
  RMtx t, tinv;
  real eps;
  
  t = *((RMtx *)(&gs->matrix));
  
  fntmtx.mtxtype = fntmtx.imtxtype = (BitField)none;
  eps = 8.0;
  if (os_fabs(t.a) > eps || os_fabs(t.b) > eps ||
      os_fabs(t.c) > eps || os_fabs(t.d) > eps) {
    return false; /* too big for fixed point calculations */
    }
  eps = fpOne;
  /* build Fixed or Frac matrix */
  if (os_fabs(t.a) < eps && os_fabs(t.b) < eps && os_fabs(t.c) < eps &&
      os_fabs(t.d) < eps && os_fabs(t.tx) < eps && os_fabs(t.ty) < eps) {
    fntmtx.isFixed = false;
    fntmtx.m.fr.a = pflttofrac(&t.a);
    fntmtx.m.fr.b = pflttofrac(&t.b);
    fntmtx.m.fr.c = pflttofrac(&t.c);
    fntmtx.m.fr.d = pflttofrac(&t.d);
    fntmtx.m.fr.tx = pflttofrac(&t.tx);
    fntmtx.m.fr.ty = pflttofrac(&t.ty);
    }
  else {
    fntmtx.isFixed = true;
    fntmtx.m.fx.a = pflttofix(&t.a);
    fntmtx.m.fx.b = pflttofix(&t.b);
    fntmtx.m.fx.c = pflttofix(&t.c);
    fntmtx.m.fx.d = pflttofix(&t.d);
    fntmtx.m.fx.tx = pflttofix(&t.tx);
    fntmtx.m.fx.ty = pflttofix(&t.ty);
    }
  fntmtx.mtxtype = (BitField)(
    (fntmtx.m.fx.a == 0 && fntmtx.m.fx.d == 0) ? ad_zero :
    (fntmtx.m.fx.b == 0 && fntmtx.m.fx.c == 0) ? bc_zero :
    general);
  MtxInvert(&t, &tinv);
  fntmtx.im.a = pflttofix(&tinv.a);
  fntmtx.im.b = pflttofix(&tinv.b);
  fntmtx.im.c = pflttofix(&tinv.c);
  fntmtx.im.d = pflttofix(&tinv.d);
  fntmtx.im.tx = pflttofix(&tinv.tx);
  fntmtx.im.ty = pflttofix(&tinv.ty);
  fntmtx.imtxtype = (BitField)(
    (fntmtx.im.a == 0 && fntmtx.im.d == 0) ? ad_zero :
    (fntmtx.im.b == 0 && fntmtx.im.c == 0) ? bc_zero :
    general);
  /* set mirr flag */
  fntmtx.mirr = t.b * t.c > t.a * t.d;
  /* set locking flags */
  /* first try for x axis -> x axis and/or y axis -> y axis */
  eps = 0.00001;
  fntmtx.noYSkew = (os_fabs(t.b) < eps); /* y' independent of x */
  fntmtx.noXSkew = (os_fabs(t.c) < eps); /* x' independent of y */
  if (fntmtx.noYSkew || fntmtx.noXSkew) fntmtx.switchAxis = false;
  else { /* next try for x -> y and/or y -> x */
    fntmtx.noYSkew = (os_fabs(t.a) < eps); /* x' independent of x */
    fntmtx.noXSkew = (os_fabs(t.d) < eps); /* y' independent of y */
    if (fntmtx.noYSkew || fntmtx.noXSkew) fntmtx.switchAxis = true;
    }

  /* Set up transforms, and do work that needs them */
  SetMtxProcs();
  fntmtx.len1000 = Len1000();
  
  return true;
  }

