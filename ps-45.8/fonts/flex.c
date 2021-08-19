/* Flex procedures

      Copyright 1983, 1986, 1989 -- Adobe Systems, Inc.
      PostScript is a trademark of Adobe Systems, Inc.
NOTICE: All information contained herein or attendant hereto is, and
remains, the property of Adobe Systems, Inc. Many of the intellectual
and technical concepts contained herein are proprietary to Adobe Systems,
Inc. and may be covered by U.S. and Foreign Patents or Patents Pending or
are protected as trade secrets. Any dissemination of this information or
reproduction of this material are strictly forbidden unless prior written
permission is obtained from Adobe Systems, Inc.

*/

#include "atm.h" 

#if ATM
#include "pubtypes.h"
#include "font.h"
#include "matrix.h"
#define os_labs(x) ABS(x)
extern procedure (*lineto)();
extern procedure (*curveto)();
global Fixed erosion;
global boolean isoutline;
#if PROTOTYPES
private procedure FlexLineTo(FCd l, PFCd p);
private procedure FlexCurveTo(FCd p0, FCd p1, FCd p2, PFCd p);
#endif  /* PROTOTYPES */

#else
#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include FP
#include "fontdata.h"
#define FixedOne 0x10000
#define FixedHalf 0x8000L
#define FixedTwo 0x20000
#define fntmtx (fontCtx->fontBuild._fntmtx)
#define erosion (fontCtx->fontBuild._erosion)
#define isoutline (fontCtx->fontBuild._outline)
#define tfmLockPt (fontCtx->fontBuild._tfmLockPt)
#define lineto (fontCtx->fontBuild._lineto)
#define curveto (fontCtx->fontBuild._curveto)
#define gsmatrix (&fntmtx)
#define FntTfmP(c1,c2) (*gsmatrix->tfm)(c1,c2)
#define FntDTfmP(c1,c2) (*gsmatrix->dtfm)(c1,c2)
#define FntITfmP(c1,c2) (*gsmatrix->itfm)(c1,c2)
#define FntIDTfmP(c1,c2) (*gsmatrix->idtfm)(c1,c2)
#define FRoundF(x) ((((integer)(x))+(1<<15)) & 0xFFFF0000)
#endif

/* For testing flex, look at "H" in AdobeGaramond-Regular.  The bases of
 * the columns should start showing "flex" at 40 pt on 300-dpi printer
 * (about 170 pixels).  Many of the capitals in this font have flex.
 */

#define floor(f) ((f) & 0xFFFF0000L)
#define ceil(f) ((((f) & 0xFFFFL) == 0)? (floor(f)) : (floor(f) + FixedOne))

private procedure FlexCurveTo(p0, p1, p2, p)
FCd p0, p1, p2; PFCd p;
{
FntTfmP(p0, &p0);
FntTfmP(p1, &p1);
FntTfmP(p2, &p2);
(*curveto)(p, &p0, &p1, &p2);
*p = p2;
}

#define yshrink(a) ( fixmul(a-c42.y, shrink) + c42.y )
#define xshrink(a) ( fixmul(a-c42.x, shrink) + c42.x )

public procedure FlexProc(flexCds, dmin, p)
  FCd *flexCds; Fixed dmin; PFCd p; {
 /* Input parameters */
 Fixed epX, epY;
 boolean yFlag, erode;
 integer flipXY;
 Fixed shrink;
 Fixed dY, dX;
 Fixed ex, ey, eShift;
 FCd tmpCd, cxy;
 FCd z0, z1, z2, z3, z4, z5;
 FCd c30, c31, c32, c40, c41, c42;
 FCd c10, c11, c12, c20, c21, c22;

 /* Get all the input parameters */
 dmin = os_labs(dmin) / 100;
 c12 = flexCds[0];
 c30 = flexCds[1];
 c31 = flexCds[2];
 c32 = flexCds[3];
 c40 = flexCds[4];
 c41 = flexCds[5];
 c42 = flexCds[6];
 epX = flexCds[7].x;
 epY = flexCds[7].y;

 c22 = c42;
 yFlag = ( os_labs(c12.y-c32.y) > os_labs(c12.x-c32.x) );

 switch(gsmatrix->mtxtype) {
   case bc_zero:	flipXY = 1;	break;
   case ad_zero:	flipXY = -1;	break;
   default:		flipXY = 0;	break;
   }

 if (yFlag) {
    if (flipXY == 0 || c32.y == c42.y) {
      z0 = c30; z1 = c31; z2 = c32; z3 = c40; z4 = c41; z5 = c42; }
    else {
     if (c32.y == c42.y) shrink = 0;
     else {
       shrink = fixdiv(c12.y-c42.y, c32.y-c42.y); shrink = os_labs(shrink); }
     c10.y = yshrink(c30.y); c11.y = yshrink(c31.y);
     c20.y = yshrink(c40.y); c21.y = yshrink(c41.y);
     c10.x = c30.x; c11.x = c31.x; c20.x = c40.x; c21.x = c41.x;
     /* Adjust curve control points */
     tmpCd.x = 0; tmpCd.y = FRoundF(c32.y-c12.y);
     FntDTfmP(tmpCd, &tmpCd);
     dY = (flipXY==1)? tmpCd.y : tmpCd.x;
     dY = os_labs(dY);
     /* When we do a dtransform of the difference of two charspace values,
         we must round the difference first to eliminate small errors
         introduced by floating point mapping. */
     if (dY < dmin) {
       z0 = c10; z1 = c11; z2 = c12; z3 = c20; z4 = c21; z5 = c22; }
     else {
       z0 = c30; z1 = c31; z2 = c32; z3 = c40; z4 = c41; z5 = c42; }
     if (os_labs(z2.y-c12.y) > 66L) { /* 66 == fixed for .001 */
        /* Force at least one pixel difference between high and low points
          and adjust subpixel location of high point (z2.y) assuming that
          the low point (c12.y) has been set by coloring. */
        FntTfmP(c12, &tmpCd);
        if (flipXY == 1)            /* Low point */
         cxy = tmpCd;
        else
         { cxy.x = tmpCd.y; cxy.y = tmpCd.x; }
      tmpCd.x = 0; tmpCd.y = FRoundF(z2.y-c12.y);
      FntDTfmP(tmpCd, &tmpCd);
      dY = (flipXY==1)? tmpCd.y : tmpCd.x;
        /* Device distance from cy to ey */
        if (FRoundF(dY) != 0)
         dY = FRoundF(dY);
        else            /* Force os_labs(dY) to be at least 1 */
         dY = (dY<0)? -FixedOne : FixedOne;
        erode = (!isoutline && erosion >= FixedHalf);
        if (erode) cxy.y -= FixedHalf;
        /* As long as assume eroding by 0.5, will get same subpixel
          position whether positive or negative erosion */
        /* cxy.y is now in its post-erosion subpixel location */
        ey = cxy.y + dY;    /* ey is integral number of pixels from cxy.y */
        ey = ceil(ey) - ey + floor(ey);        /* Reverse loc in pixel */
        if (erode) ey += FixedHalf;                /* Remove the erosion */
        if (flipXY == 1)
         { tmpCd.x = cxy.x; tmpCd.y = ey; }
        else
         { tmpCd.y = cxy.x; tmpCd.x = ey; }
        FntITfmP(tmpCd, &tmpCd);
        eShift = tmpCd.y - z2.y;
        z1.y += eShift; z2.y += eShift; z3.y += eShift;
        }
     }
    }
 else {
    if (flipXY == 0 || c32.x == c42.x) {
     z0 = c30; z1 = c31; z2 = c32; z3 = c40; z4 = c41; z5 = c42;
     }
    else {
     if (c32.x == c42.x) shrink = 0;
     else {
       shrink = fixdiv(c12.x-c42.x, c32.x-c42.x); shrink = os_labs(shrink); }
     c10.x = xshrink(c30.x); c11.x = xshrink(c31.x);
     c20.x = xshrink(c40.x); c21.x = xshrink(c41.x);
     c10.y = c30.y; c11.y = c31.y; c20.y = c40.y; c21.y = c41.y;
     /* Adjust curve control points */
     tmpCd.y = 0; tmpCd.x = FRoundF(c32.x-c12.x);
     FntDTfmP(tmpCd, &tmpCd);
     dX = (flipXY==(-1))? tmpCd.y : tmpCd.x;
     dX = os_labs(dX);
     if (dX < dmin) {
       z0 = c10; z1 = c11; z2 = c12; z3 = c20; z4 = c21; z5 = c22; }
     else {
       z0 = c30; z1 = c31; z2 = c32; z3 = c40; z4 = c41; z5 = c42; }
     if (os_labs(z2.x-c12.x) > 66L) { /* 66 == fixed for .001 */
        FntTfmP(c12, &tmpCd);
        if (flipXY == (-1))            /* Low point */
         { cxy.x = tmpCd.y; cxy.y = tmpCd.x; }
        else
         cxy = tmpCd;
      tmpCd.y = 0; tmpCd.x = FRoundF(z2.x-c12.x);
      FntDTfmP(tmpCd, &tmpCd);
      dX = (flipXY==(-1))? tmpCd.y : tmpCd.x;
        if (FRoundF(dX) != 0)
         dX = FRoundF(dX);
        else
         dX = (dX<0)? -FixedOne : FixedOne;
        erode = (!isoutline && erosion >= FixedHalf);
        if (erode) cxy.x -= FixedHalf;
        ex = cxy.x + dX;
        ex = ceil(ex) - ex + floor(ex);
        if (erode) ex += FixedHalf;
        if (flipXY == (-1))
         { tmpCd.x = cxy.y; tmpCd.y = ex; }
        else
         { tmpCd.y = cxy.y; tmpCd.x = ex; }
        FntITfmP(tmpCd, &tmpCd);
        eShift = tmpCd.x - z2.x;
        z1.x += eShift; z2.x += eShift; z3.x += eShift;
        }
     }
  }
 if (z2.x == z5.x || z2.y == z5.y) {
    FntTfmP(z5, p); (*lineto)(p);
    }
 else {
    FlexCurveTo(z0, z1, z2, p);
    FlexCurveTo(z3, z4, z5, p);
    }

 /* Final position in character space */
 flexCds[0].x = epX;
 flexCds[0].y = epY;
}

public procedure FlexProc2(flexCds, dmin, p)
  FCd *flexCds; Fixed dmin; PFCd p; {
  /* for pathback.  always return unmodified curves */
  FlexCurveTo(flexCds[1], flexCds[2], flexCds[3], p);
  FlexCurveTo(flexCds[4], flexCds[5], flexCds[6], p);
  flexCds[0] = flexCds[7];
  }

#if !ATM
private procedure RFlexCurveTo(p0, p1, p2, p)
  RCd p0, p1, p2, *p; {
  TfmP(p0, &p0); TfmP(p1, &p1); TfmP(p2, &p2);
  (*curveto)(p, &p0, &p1, &p2);
  *p = p2;
  }

public procedure RFlexProc(flexCds, dmin, p)
  RCd *flexCds; Fixed dmin; PRCd p; {
  /* p points to current point; already in device coord */
  /* flexCds are in character space */
  RFlexCurveTo(flexCds[1], flexCds[2], flexCds[3], p);
  RFlexCurveTo(flexCds[4], flexCds[5], flexCds[6], p);
  flexCds[0] = flexCds[7];
  }
#endif
/* BC Version:  v002  Thu Mar 29 16:46:55 PST 1990 */
