/*
  pathextraops.c

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

Original version: Doug Brotz, May 11, 1983
Edit History:
Doug Brotz: Thu Dec 11 17:23:20 1986
Chuck Geschke: Thu Oct 31 15:22:56 1985
Ed Taft: Tue Jul 14 14:57:40 1987
John Gaffney: Tue Feb 12 10:46:11 1985
Ken Lent: Wed Mar 12 15:53:25 1986
Bill Paxton: Fri Aug 28 15:21:14 1987
Don Andrews: Wed Sep 17 15:28:32 1986
Ivor Durham: Fri May  6 15:30:27 1988
Joe Pasqua: Thu Jan 12 13:10:57 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include FP
#include GRAPHICS
#include LANGUAGE
#include VM

#include "path.h"
#include "graphicspriv.h"

#define PI 3.1415926535
#define TwoPI 6.2831853071
#define PIover2 1.5707963268


private procedure ArcC(cv, ev, angst, angend, ccwise)
  Cd cv, ev;  real angst, angend;  boolean ccwise;
{
real angdif, angmid, radiusc, radiuse, radiusm, ctrlratio;
Cd c0, c1, c2, c3, mv;
angdif = (ccwise) ? angend - angst : angst - angend;
if (RealLt0(angdif)) angdif += TwoPI;
if (angdif > PIover2)
  {
  radiusc = Dist(cv);  radiuse = Dist(ev);
  radiusm = (radiusc + radiuse) / 2.0;
  angmid = (ccwise) ? angend - angdif / 2.0 : angst - angdif / 2.0;
  if (RealLt0(angmid)) angmid += TwoPI;
  mv.x = radiusm * os_cos(angmid);  mv.y = radiusm * os_sin(angmid);
  ArcC(cv, mv, angst, angmid, ccwise);
  mv.x = -mv.x;  mv.y = -mv.y;
  ArcC(mv, ev, angmid, angend, ccwise);
  }
else
  {
  c3.x = cv.x + ev.x;  c3.y = cv.y + ev.y;
  if (angdif < 0.1)
    {RTfmPCd(c3, &gs->matrix, gs->cp, &c3);  LineTo(c3, &gs->path);}
  else
    {
    ctrlratio = (4.0 * (1.0 - os_cos(angdif / 2.0)))
                / (3.0 * os_sin(angdif / 2.0));
    c0 = gs->cp;
    if (ccwise) {c1.x = cv.y * ctrlratio;  c1.y = -cv.x * ctrlratio;}
    else  {c1.x = -cv.y * ctrlratio;  c1.y = cv.x * ctrlratio;}
    if (ccwise)
      {c2.x = c3.x + ev.y * ctrlratio;  c2.y = c3.y - ev.x * ctrlratio;}
    else {c2.x = c3.x - ev.y * ctrlratio;  c2.y = c3.y + ev.x * ctrlratio;}
    RTfmPCd(c1, &gs->matrix, c0, &c1);
    RTfmPCd(c2, &gs->matrix, c0, &c2);
    RTfmPCd(c3, &gs->matrix, c0, &c3);
    CurveTo(c1, c2, c3, &gs->path);
    }
  }
}  /* end of ArcC */


private procedure PSArcC()
{
Cd center, endpt, cv, ev, cur;
real angst, angend, angdif;
integer arctype;
CheckForCurrentPoint(&gs->path);
arctype = PopInteger();
/* arctype values mean:
   0: draw the counterclockwise arc from cur to endpt.
   1: draw the clockwise arc from cur to endpt.
   2: draw the shorter arc from cur to endpt.
   3: draw the longer arc from cur to endt.
*/
PopPCd(&endpt);
PopPCd(&center);
ITfmP(gs->cp, &cur);
cv.x = center.x - cur.x;  cv.y = center.y - cur.y;
ev.x = endpt.x - center.x;  ev.y = endpt.y - center.y;
angst = os_atan2(-cv.y, -cv.x);
angend = os_atan2(ev.y, ev.x);
if (arctype <= 1) ArcC(cv, ev, angst, angend, (boolean)(arctype == 0));
else
  {
  angdif = angend - angst;
  if (RealLt0(angdif)) angdif += TwoPI;
  ArcC(cv, ev, angst, angend, (boolean)((arctype == 2) == (angdif < PI)));
  }
} /* end of PSArcC */


public procedure PathExtraOpsInit(reason)  InitReason reason;
{
switch (reason)
  {
  case romreg:
    if (!MAKEVM) RgstExplicit("arcc", PSArcC);
    break;
  }
} /* end of PathExtraOpsInit */
