/* 
  matrix.h -- Interface to matrix routines.

	      Copyright 1986 -- Adobe Systems, Inc.
	    PostScript is a trademark of Adobe Systems, Inc.
NOTICE:  All information contained herein or attendant hereto is, and
remains, the property of Adobe Systems, Inc.  Many of the intellectual
and technical concepts contained herein are proprietary to Adobe Systems,
Inc. and may be covered by U.S. and Foreign Patents or Patents Pending or
are protected as trade secrets.  Any dissemination of this information or
reproduction of this material are strictly forbidden unless prior written
permission is obtained from Adobe Systems, Inc.

Original version: 
Mike Byron: ...
*/

#ifndef	MATRIX_H
#define	MATRIX_H

#ifndef	USE68KMATRIX
#define USE68KMATRIX		0
#endif	/* USE68KMATRIX */

  /* SetupMtx return codes:						   */
#define SM_NOERR   0		/* No problems				   */
#define SM_TOOBIG  1		/* Coefficients too big			   */
#define SM_BADINV  2		/* Matrix inversion failed		   */

#if PROTOTYPES
public Fixed ApproxDLen(PFCd cp);

public boolean FixMtxInvert(	/* returns true/false success code	   */
  FixMtx *m,			/* a matrix				   */
  FixMtx *im			/* return the inverse			   */
  );

public integer SetupMtx(
  PFracMtx fontMatrix,		/* The matrix from the font file	   */
  PFixMtx tfmMatrix,		/* The transform matrix (scale, etc)	   */
  PMtx resultMatrix		/* The matrix to pass to BuildChar	   */
  );

  /* This routine must be   called  to  set  up the  matrix  that is used  by
     BuildChar.  It takes  two floating-point matrices,  and multiplies  them
     together, and records  the  matrix results and  some other info in a Mtx
     structure that must be  passed into BuildChar.   This routine only needs
     to be called when fontMatrix and/or tfmMatrix change.		   */

public procedure TransformFCd(
  FCd src,		/* The coordinate to transform			   */
  PFCd result,		/* Result of transform				   */
  PMtx mtx,		/* A matrix from SetupMtx.  NULL = Current.	   */
  boolean delta		/* No translation -- tfm like a vector		   */
  );
  /* This   routine     will  take  a  coordinate      ("fixed"  numbers, not
     floating-point) in  the character space and  transform it into the pixel
     space that the character pixels are painted in.  SetupMtx must be called
     to set  up the transformation matrix.  BuildChar   sets  up  a "current"
     matrix, which can be used by this routine  -- set mtx to NULL.   "delta"
     should be true  when the coordinate   is position-independent, like  the
     width vector of a character.					   */

public procedure SetMtx(PMtx);
/* This is the  procedure  used  to set gsmatrix  to a new  matrix.  It first
   checks to see if the new matrix really is different from the old, and then
   if it is, it looks at the matrix and decides which procedures will be used
   to transform points with this matrix.				   */

#else	/* PROTOTYPES */

public Fixed	ApproxDLen();
public boolean	FixMtxInvert();
public integer	SetupMtx();
public procedure TransformFCd();
public procedure SetMtx();
public boolean  SetupFntMtx();

#endif	/* PROTOTYPES */

#if	USE68KMATRIX

#if	PROTOTYPES
public procedure FntTfmP(FCd, PFCd);
public procedure FntITfmP(FCd, PFCd);
public procedure FntDTfmP(FCd, PFCd);
public procedure FntIDTfmP(FCd, PFCd);

#else	/* PROTOTYPES */
public procedure FntTfmP();
public procedure FntITfmP();
public procedure FntDTfmP();
public procedure FntIDTfmP();
#endif	/* PROTOTYPES */

#else	/* USE68KMATRIX */

global procedure (*Transform)(/* FCd, PFCd */);
global procedure (*ITransform)(/* FCd, PFCd */);
global procedure (*DTransform)(/* FCd, PFCd */);
global procedure (*IDTransform)(/* FCd, PFCd */);

#if	ATM
#define FntTfmP(c, ct)  (*(Transform))(c, ct)
#define FntITfmP(c, ct) (*(ITransform))(c, ct)
#define FntDTfmP(c, ct)  (*(DTransform))(c, ct)
#define FntIDTfmP(c, ct)  (*(IDTransform))(c, ct)
#endif	/* ATM */

#endif	/* USE68KMATRIX */

/* The next group of procedures are the transformation procs used by
   SetMtx and the Tfm #defines. */
#if	PROTOTYPE

public procedure bcz_fixed_tfm(FCd, PFCd);
public procedure bcz_frac_tfm(FCd, PFCd);
public procedure adz_fixed_tfm(FCd, PFCd);
public procedure adz_frac_tfm(FCd, PFCd);
public procedure gen_fixed_tfm(FCd, PFCd);
public procedure gen_frac_tfm(FCd, PFCd);
public procedure d_bcz_fixed_tfm(FCd, PFCd);
public procedure d_bcz_frac_tfm(FCd, PFCd);
public procedure d_adz_fixed_tfm(FCd, PFCd);
public procedure d_adz_frac_tfm(FCd, PFCd);
public procedure d_gen_fixed_tfm(FCd, PFCd);
public procedure d_gen_frac_tfm(FCd, PFCd);
public procedure inv_bcz_fixed_tfm(FCd, PFCd);
public procedure inv_adz_fixed_tfm(FCd, PFCd);
public procedure inv_gen_fixed_tfm(FCd, PFCd);
public procedure inv_d_bcz_fixed_tfm(FCd, PFCd);
public procedure inv_d_adz_fixed_tfm(FCd, PFCd);
public procedure inv_d_gen_fixed_tfm(FCd, PFCd);

#else	/* PROTOTYPES */

public procedure bcz_fixed_tfm();
public procedure bcz_frac_tfm();
public procedure adz_fixed_tfm();
public procedure adz_frac_tfm();
public procedure gen_fixed_tfm();
public procedure gen_frac_tfm();
public procedure d_bcz_fixed_tfm();
public procedure d_bcz_frac_tfm();
public procedure d_adz_fixed_tfm();
public procedure d_adz_frac_tfm();
public procedure d_gen_fixed_tfm();
public procedure d_gen_frac_tfm();
public procedure inv_bcz_fixed_tfm();
public procedure inv_adz_fixed_tfm();
public procedure inv_gen_fixed_tfm();
public procedure inv_d_bcz_fixed_tfm();
public procedure inv_d_adz_fixed_tfm();
public procedure inv_d_gen_fixed_tfm();

#endif	/* PROTOTYPES */

#endif	/* MATRIX_H */
