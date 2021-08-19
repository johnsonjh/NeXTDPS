/*
			       fflattenpath.c

   Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE: All information contained  herein  is the property  of  Adobe Systems
Incorporated.  Many  of  the  intellectual and  technical  concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees  for their internal  use.  Any reproduction
or dissemination of this software is  strictly forbidden unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Edit History:
Scott Byer: Thu May 18 12:51:10 1989
Ed Taft: Thu Jul 28 16:41:22 1988
Bill Paxton: Fri Nov  4 15:08:28 1988
Ivor Durham: Tue Aug 16 10:42:47 1988
Jim Sandman: Mon Nov  7 09:30:37 1988
Joe Pasqua: Tue Feb 28 13:18:55 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES

#include "graphicspriv.h"

private procedure FMiniFltn(f0, f1, f2, f3, pfr, inside)
  DevCd f0, f1, f2, f3; 
  register PFltnRec pfr; 
  boolean inside; 
  {
   /* Like FFltnCurve, but assumes os_abs(deltas) <= 127 pixels */
   /* 8 bits of fraction gives enough precision for splitting curves */
#define MFix(f) ((f)>>8)
#define UnMFix(f) ((f)<<8)
#define MFixInt(f) ((f)<<8)
#define MiniFltnMaxDepth (6)
#define inrect (p[-10])
#define inbbox (p[-9])
#define c0x (p[-8])
#define c0y (p[-7])
#define c1x (p[-6])
#define c1y (p[-5])
#define c2x (p[-4])
#define c2y (p[-3])
#define c3x (p[-2])
#define c3y (p[-1])
#define inrect2 (p[0])
#define bbox2 (p[1])
#define d0x (p[2])
#define d0y (p[3])
#define d1x (p[4])
#define d1y (p[5])
#define d2x (p[6])
#define d2y (p[7])
#define d3x (p[8])
#define d3y (p[9])
#define MiniBlkSz (10)
#define mdpt(a,b) (((long)(a)+(long)(b))>>1)
   Int16 cds[MiniBlkSz*MiniFltnMaxDepth], dpth, eps;
   Int16 bbLLX, bbLLY, bbURX, bbURY;
   register Int16 *p;
   Cd cd;
   p = cds; dpth = 1;
   *(p++) = inside;		/* initial value of inrect */
   *(p++) = false;		/* inbbox starts out false */
     { register Fixed llx, lly;
       llx = pfr->llx; lly = pfr->lly;
       *(p++) = MFix(f0.x-llx); *(p++) = MFix(f0.y-lly);
       *(p++) = MFix(f1.x-llx); *(p++) = MFix(f1.y-lly);
       *(p++) = MFix(f2.x-llx); *(p++) = MFix(f2.y-lly);
       *(p++) = MFix(f3.x-llx); *(p++) = MFix(f3.y-lly);
      }
   if (!inrect) {
      register Fixed c, f128;
      c = pfr->ll.x; bbLLX = (c <= 0)? 0 : MFix(c);
      c = pfr->ll.y; bbLLY = (c <= 0)? 0 : MFix(c);
      f128 = FixInt(128);
      c = pfr->ur.x; bbURX = (c >= f128)? 0x7fff : MFix(c);
      c = pfr->ur.y; bbURY = (c >= f128)? 0x7fff : MFix(c);
     }
   eps = MFix(pfr->feps);
   if (eps < 8) eps = 8;	/* Brotz patch */
   while (true) {
      if (dpth == MiniFltnMaxDepth) goto ReportC3;
      if (!inrect) {
	 register Int16 llx, lly, urx, ury, c;
	 llx = urx = c0x;
	 if ((c=c1x) < llx) llx = c;
	 else if (c > urx) urx = c;
	 if ((c=c2x) < llx) llx = c;
	 else if (c > urx) urx = c;
	 if ((c=c3x) < llx) llx = c;
	 else if (c > urx) urx = c;
	 if (urx < bbLLX || llx > bbURX) goto ReportC3;
	 lly = ury = c0y; 
	 if ((c=c1y) < lly) lly = c;
	 else if (c > ury) ury = c; 
	 if ((c=c2y) < lly) lly = c;
	 else if (c > ury) ury = c; 
	 if ((c=c3y) < lly) lly = c;
	 else if (c > ury) ury = c;
	 if (ury < bbLLY || lly > bbURY) goto ReportC3;
	 if (urx <= bbURX && ury <= bbURY &&
	     llx >= bbLLX && lly >= bbLLY) inrect = true;
	}
      if (!inbbox) {
	 register Int16 mrgn = eps, r0, r3, ll, ur, c;
	 r0 = c0x; r3 = c3x;
	 if (r0 < r3) {ll = r0 - mrgn; ur = r3 + mrgn;}
	 else {ll = r3 - mrgn;  ur = r0 + mrgn;}
	 if (ur < 0) ur = MFixInt(128) - 1;
	 c = c1x;
	 if (c > ll && c < ur) {
	    c = c2x;
	    if (c > ll && c < ur) {
	       r0 = c0y; r3 = c3y;
	       if (r0 < r3) {ll = r0 - mrgn; ur = r3 + mrgn;}
	       else {ll = r3 - mrgn;  ur = r0 + mrgn;}
	       if (ur < 0) ur = MFixInt(128) - 1;
	       c = c1y;
	       if (c > ll && c < ur)
		 {c = c2y;  if (c > ll && c < ur) inbbox = true;}
	      }
	   }
	}
      if (inbbox) {
	 register Int16 eqa, eqb, x, y;
	 register Int32 EPS, d;
	 x = c0x; y = c0y;
	 eqa = c3y - y;
	 eqb = x - c3x;
	 if (eqa == 0 && eqb == 0) goto ReportC3;
	 EPS = ((os_labs(eqa) > os_labs(eqb))? eqa : eqb)*eps;
	 if (EPS < 0) EPS = -EPS;
	 d = eqa*(c1x-x); d += eqb*(c1y-y);
	 if (os_labs(d) < EPS)
	   {
	    d = eqa*(c2x-x); d += eqb*(c2y-y);
	    if (os_labs(d) < EPS) goto ReportC3;
	   }
	}
	{			/* Bezier divide */
	 register Int16 c0, c1, c2, d1, d2, d3;
	 d0x = c0 = c0x; c1 = c1x; c2 = c2x;
	 d1x = d1 = mdpt(c0,c1);
	 d3 = mdpt(c1,c2);
	 d2x = d2 = mdpt(d1,d3);
	 c2x = c2 = mdpt(c2,c3x);
	 c1x = c1 = mdpt(d3,c2);
	 c0x = d3x = mdpt(d2,c1);
	 d0y = c0 = c0y; c1 = c1y; c2 = c2y;
	 d1y = d1 = mdpt(c0,c1);
	 d3 = mdpt(c1,c2);
	 d2y = d2 = mdpt(d1,d3);
	 c2y = c2 = mdpt(c2,c3y);
	 c1y = c1 = mdpt(d3,c2);
	 c0y = d3y = mdpt(d2,c1);
	 bbox2 = inbbox;
	 inrect2 = inrect;
	 p += MiniBlkSz;
	 dpth++;
	 continue;
	}
     ReportC3:
	{
	 DevCd c;
	 if (--dpth == 0) c = f3;
	 else {
	    c.x = UnMFix(c3x) + pfr->llx;
	    c.y = UnMFix(c3y) + pfr->lly;
	   }
	 if (pfr->reportFixed)
	   (*pfr->report)(c);
	 else {
	    UnFixCd(c, &cd);
	    (*pfr->report)(cd);
	   }
	 if (dpth == 0) return;
	 p -= MiniBlkSz;
	}
     }
  }				/* end of FMiniFltn */
#undef MFix
#undef UnMFix
#undef MFixInt
#undef MiniFltnMaxDepth
#undef inrect
#undef inbbox
#undef c0x
#undef c0y
#undef c1x
#undef c1y
#undef c2x
#undef c2y
#undef c3x
#undef c3y
#undef inrect2
#undef bbox2
#undef d0x
#undef d0y
#undef d1x
#undef d1y
#undef d2x
#undef d2y
#undef d3x
#undef d3y
#undef MiniBlkSz
#undef mdpt

#define FixedMidPoint(m,a,b) \
  (m).x=((long)((a).x)+(long)((b).x))>>1;\
  (m).y=((long)((a).y)+(long)((b).y))>>1

#define FixedBezDiv(a0, a1, a2, a3, b0, b1, b2, b3) \
  b3 = a3; \
  FixedMidPoint(b2, a2, a3); \
  FixedMidPoint(a3, a1, a2); \
  FixedMidPoint(a1, a0, a1); \
  FixedMidPoint(a2, a1, a3); \
  FixedMidPoint(b1, a3, b2); \
  FixedMidPoint(b0, a2, b1); \
  a3 = b0

public procedure FFltnCurve(c0, c1, c2, c3, pfr, inrect)
                                               /* inrect = !testRect */
  DevCd c0, c1, c2, c3;  register PFltnRec pfr;
  register boolean inrect;
  /* Like FltnCurve, but works in the Fixed domain. */
  /* absolute values of coords must be < 2^14 so will not overflow when
     find midpoint by add and shift */
  {
   DevCd d0, d1, d2, d3;
   register Fixed llx, lly, urx, ury;
   register integer ll, ur, c, th;
   Cd cd;
   if (c0.x==c1.x && c0.y==c1.y && c2.x==c3.x && c2.y==c3.y) goto ReportC3;
   if (pfr->limit <= 0) goto ReportC3;
     { register Fixed c;
       llx = urx = c0.x;
       if ((c=c1.x) < llx) llx = c;
       else if (c > urx) urx = c;
       if ((c=c2.x) < llx) llx = c;
       else if (c > urx) urx = c;
       if ((c=c3.x) < llx) llx = c;
       else if (c > urx) urx = c;
       lly = ury = c0.y; 
       if ((c=c1.y) < lly) lly = c;
       else if (c > ury) ury = c; 
       if ((c=c2.y) < lly) lly = c;
       else if (c > ury) ury = c; 
       if ((c=c3.y) < lly) lly = c;
       else if (c > ury) ury = c; 
      }
   if (!inrect) {
      if (urx < pfr->ll.x || llx > pfr->ur.x ||
	  ury < pfr->ll.y || lly > pfr->ur.y) goto ReportC3;
      if (urx <= pfr->ur.x && ury <= pfr->ur.y &&
	  llx >= pfr->ll.x && lly >= pfr->ll.y) inrect = true;
     }
     { register Fixed th;
       th = FixInt(127);	/* delta threshhold of 127 pixels */
       if (urx-llx >= th || ury-lly >= th) {
	  goto Split;
	 }
      }
   pfr->llx = llx; pfr->lly = lly;
   if (!inrect) {
      pfr->ll.x -= llx; pfr->ur.x -= llx;
      pfr->ll.y -= lly; pfr->ur.y -= lly;
     }
   FMiniFltn(c0, c1, c2, c3, pfr, inrect);
   if (!inrect) {
      pfr->ll.x += llx; pfr->ur.x += llx; 
      pfr->ll.y += lly; pfr->ur.y += lly;
     }
   return;

  Split:
   FixedBezDiv(c0, c1, c2, c3, d0, d1, d2, d3);
   pfr->limit--;
   FFltnCurve(c0, c1, c2, c3, pfr, inrect);
   FFltnCurve(d0, d1, d2, d3, pfr, inrect);
   pfr->limit++;
   return;

  ReportC3:
   if (pfr->reportFixed) (*pfr->report)(c3);
   else { UnFixCd(c3, &cd);  (*pfr->report)(cd); }
  }


