/*
				 graypriv.c

   Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
			    All rights reserved.

NOTICE:  All information contained herein is  the property   of Adobe Systems
Incorporated.   Many of  the  intellectual   and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only  to Adobe licensees for their  internal use.  Any reproduction
or dissemination of this software is strictly  forbidden unless prior written
permission is obtained from Adobe.

     PostScript is a registered trademark of Adobe Systems Incorporated.
      Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Doug Brotz: August 8, 1983
Edit History:
Scott Byer: Thu May 18 10:32:00 1989
Doug Brotz: Fri Jan  9 14:29:55 1987
Chuck Geschke: Sun Oct 27 12:25:31 1985
Ed Taft: Wed Jun 24 14:45:11 1987
Bill Paxton: Fri Jan 22 13:20:09 1988
Don Andrews: Mon Jan 13 15:43:32 1986
Jim Sandman: Wed Dec 13 09:11:26 1989
Joe Pasqua: Thu Jan 12 11:33:22 1989
Ivor Durham: Thu Jun 23 14:49:33 1988
Mark Francis: Thu Nov  9 15:54:28 1989
Paul Rovner: Mon Aug 28 11:58:37 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include DEVICE
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include FP
#include GRAPHICS
#include LANGUAGE
#include VM

#include "graphicspriv.h"
#include "graphdata.h"
#include "gray.h"

#define	randx		(graphicsStatics->grayData._randx)
#define	INIT_RAND(x)	randx = 1
#define NEXT_RAND	((randx = randx * 1103515245 + 12345) & MAXinteger)

extern integer GCD();

private real FractPart(a) real a; {return a - os_floor(a);}

private integer ExtendedGCD(u, v, u1, u2)  integer u, v, *u1, *u2;
{
integer q, t1, t2, t3, v1, v2;
*u1 = v2 = 1;
*u2 = v1 = 0;
while (v != 0)
  {
  q = u / v;
  t1 = *u1 - v1 * q;
  t2 = *u2 - v2 * q;
  t3 = u - v * q;
  *u1 = v1; *u2 = v2; u = v;
  v1 = t1; v2 = t2; v = t3;
  }
return u;
} /* end of ExtendedGCD */

private integer LCM(u, v)  integer u, v; {return (u * v) / GCD(u, v);}
public integer LCM4(u, v, w, x)  integer u, v, w, x; {
  return LCM(LCM(u, v), LCM(w, x));
  }

private boolean GrayGreaterThan(a, b)  cardinal a, b;
{return (a != b) ? (boolean)(a > b) : (boolean)(NEXT_RAND & 01);}   /* UNIX */


#define exch(a,b) {x = *a;  *a = *b;  *b = x;}

private procedure SortGQ (gqbase, curgq) PGrayQ gqbase; cardinal curgq;
{ /* quick sort */
cardinal v;
GrayQ x;
int l, r, i, j, p, mn, mx;
PGrayQ gqi, gqj, gql, gqr, gqip1, gqjm1;
int stack[20];  /* for subfiles */
boolean done, leftIsMax;
p = l = 0; r = curgq - 1; done = (boolean)(r <= 9);
while (!done)
  {
  /* use median of three as partition element */
  gql = gqbase + l;  gqr = gqbase + r;
  j = (l + r) / 2;   gqj = gqbase + j;
  i = l + 1;  gqi = gqbase + i;   exch(gqj, gqi);
  j = r;  gqj = gqr;
  if (GrayGreaterThan(gqi->val, gqr->val)) exch(gqi, gqr);
  if (GrayGreaterThan(gql->val, gqr->val)) exch(gql, gqr);
  if (GrayGreaterThan(gqi->val, gql->val)) exch(gqi, gql);
  v = gql->val;
  while (true)
    {
    while (true) {i++; gqi++; if (!GrayGreaterThan(v, gqi->val)) break;}
    while (true) {j--; gqj--; if (!GrayGreaterThan(gqj->val, v)) break;}
    if (j < i) break;
    exch(gqi, gqj);
    }
  exch(gql, gqj);
  mn = j - l;   mx = r - i + 1; leftIsMax = (mn > mx);
  if (leftIsMax) {int temp;  temp = mn;  mn = mx;  mx = temp;}
  if (mx <= 9)
    {
    if (p == 0) done = true;
    else {r = stack[--p];  l = stack[--p];}
    }
  else if (mn <= 9)
    { /* skip the smaller subfile */
    if (leftIsMax)  r = j - 1;  else  l = i;
    }
  else
    { /* push the large subfile and do the small one */
    if (leftIsMax) {stack[p++] = l;  stack[p++] = j - 1;  l = i;}
    else {stack[p++] = i;  stack[p++] = r;  r = j - 1;}
    }
  }
i = curgq - 1;  gqip1 = gqbase + i;  gqi = gqip1 - 1;
while (--i >= 0)
  {
  if (GrayGreaterThan(gqi->val, gqip1->val))
    {
    x = *gqi;
    v = gqi->val;
    j = i + 1;  gqj = gqip1;  gqjm1 = gqi;
    do {*gqjm1 = *gqj;  j++;  gqjm1 = gqj;  gqj++;}
      while (j < curgq && GrayGreaterThan(v, gqj->val));
    *gqjm1 = x;
    }
  gqip1 = gqi;  gqi--;
  }
}  /* end of SortGQ */

typedef struct {
  integer iu1, iu2, iv1, iv2, disp, rept, w, h, repH;
  } Numbers, *PNumbers;

private FreqAngleToNumbers(xSpotSize, ySpotSize, angle, nums)
  real xSpotSize, ySpotSize, angle;  PNumbers nums;
  {
  /* Calculates width and height given frequency and angle.
   */
  real u1, u2, v1, v2, angsin, angcos, rval;
  integer t, absiu1, absiu2, absiv1, absiv2, x, y;
  angsin = os_sin(RAD(angle));  angcos = os_cos(RAD(angle));
  u1 = xSpotSize * angsin;  u2 = ySpotSize * angsin;
  v1 = ySpotSize * angcos;  v2 = xSpotSize * angcos;
  
  RRoundP(&u1, &rval);  nums->iu1 = (integer)rval;
  RRoundP(&u2, &rval);  nums->iu2 = (integer)rval;
  RRoundP(&v1, &rval);  nums->iv1 = (integer)rval;
  RRoundP(&v2, &rval);  nums->iv2 = (integer)rval;
  
  if (nums->iu1 == 0 && nums->iv2 == 0) nums->iu1 = 1;
  if (nums->iu2 == 0 && nums->iv1 == 0) nums->iu2 = 1;
  
  absiu1 = os_labs(nums->iu1);  absiv1 = os_labs(nums->iv1);
  absiu2 = os_labs(nums->iu2);  absiv2 = os_labs(nums->iv2);
  
  if (nums->iu1 * nums->iv2 < 0) {t = absiv2; absiv2 = absiu1; absiu1 = t;}
  if (nums->iu2 * nums->iv1 < 0) {t = absiv1; absiv1 = absiu2; absiu2 = t;}
  
  nums->h = ExtendedGCD(absiu2, absiv1, &x, &y);
  
  nums->w = (nums->iu1 * nums->iu2 + nums->iv1 * nums->iv2) / nums->h;
  
  nums->disp = (x * absiv2 - y * absiu1) % nums->w;
  while (nums->disp < 0) nums->disp += nums->w;
  nums->rept = (nums->disp == 0) ? 1 : LCM(nums->disp, nums->w) / nums->disp;
  nums->repH = nums->rept * nums->h;
  } /* end of FreqAngleToNumbers */


private procedure GetXDistYDist(xdist, ydist) real *xdist, *ydist; {
  Mtx m;
  Cd c;
  (*gs->device->procs->DefaultMtx)(gs->device, &m);
  c.x = fpOne; c.y = fpZero;  IDTfmPCd(c, &m, &c);
  *xdist = os_sqrt(c.x * c.x + c.y * c.y);
  c.x = fpZero; c.y = fpOne;  IDTfmPCd(c, &m, &c);
  *ydist = os_sqrt(c.x * c.x + c.y * c.y);
  }


public procedure GetValidFreqAnglePair(pSpot, dims)
  PSpotFunction pSpot; integer *dims; {
  Numbers nums;
  real xSpotSize, ySpotSize, xtdist, ytdist, newangle, newfreq, freqDelta;
  integer freqError, angError;
  newfreq = pSpot->freq;
  newangle = pSpot->angle;
  if (RealLe0(newfreq))
    UndefResult();
  GetXDistYDist(&xtdist, &ytdist);
  freqDelta = newfreq / 20;
  freqError = 0;
  angError = 0;
  while (true)
    {
    xSpotSize = fp72 / (newfreq * xtdist);
    ySpotSize = fp72 / (newfreq * ytdist);
    FreqAngleToNumbers(xSpotSize, ySpotSize, newangle, &nums);
    if ((nums.w > maxDevShort) || (nums.h > maxDevShort))
      LimitCheck();
    if (nums.w * nums.h < grayPatternLimit &&
      DevCheckScreenDims((DevShort)nums.w, (DevShort)nums.repH)) {
      pSpot->freq = newfreq;
      pSpot->angle = newangle;
      dims[0] = nums.w;
      dims[1] = nums.repH;
      return;
      }
    if (angError - 2 >= freqError)
      {freqError += 2;  newfreq = newfreq + freqDelta;  angError = 0;}
    else angError = (angError >= 0) ? -angError - 2 : -angError;
    newangle = pSpot->angle + angError;
    }
  }


public procedure GetValidFreqAngleOctet(fcns, dims)
  PSpotFunction fcns; integer *dims; {
  Numbers nums[4];
  real xSpotSize, ySpotSize, xtdist, ytdist;
  real freq[4], freqDelta[4];
  real angle[4];
  integer i, max;
  integer freqError[4]; /* id: not on VAX = {0, 0, 0, 0}; */
  integer angleError[4]; /* id: not on VAX = {0, 0, 0, 0}; */
  for (i = 0; i < 4; i++) {
    freqError[i] = 0;	/* id: see above */
    angleError[i] = 0;	/* ditto */
    freq[i] = fcns[i].freq;
    angle[i] = fcns[i].angle;
    freqDelta[i] = freq[i]/20;
    if (RealLe0(freq[i] ))
      UndefResult();
    }
  GetXDistYDist(&xtdist, &ytdist);
  while (true)
    {
    for (i = 0; i < 4; i++) {
      FreqAngleToNumbers(
        fp72 / (freq[i] * xtdist), fp72 / (freq[i] * ytdist),
        angle[i], &nums[i]);
      }
    for (i = 0; i < 4; i++) {
      if ((nums[i].w > maxDevShort) || (nums[i].h > maxDevShort))
        LimitCheck();
      if (nums[i].w * nums[i].h >= grayPatternLimit) goto tooBig;
      }
    if (DevCheckScreenDims(
        (DevShort)LCM4(nums[0].w, nums[1].w, nums[2].w, nums[3].w),
        (DevShort)LCM4(
          nums[0].repH, nums[1].repH, nums[2].repH, nums[3].repH))) {
      for (i = 0; i < 4; i++) {
	fcns[i].freq = freq[i];
	fcns[i].angle = angle[i];
	dims[i*2] = nums[i].w;
	dims[i*2+1] = nums[i].repH;
	}
      return;
      }
  tooBig:
    max = 0;
    for (i = 1; i < 4; i++) {
      if (nums[i].w * nums[i].repH > nums[max].w * nums[max].repH)
        max = i;
      }
    if (angleError[max] - 2 >= freqError[max]) {
      freqError[max] += 2;
      freq[max] = freq[max] + freqDelta[max];
      angleError[max] = 0;
      }
    else
      angleError[max] = 
        (angleError[max] >= 0) ? -angleError[max] - 2 : -angleError[max];
    angle[max] = fcns[max].angle + angleError[max];
    }
  }


private procedure CalcThresholds(nums, pSpot, devS)
  PNumbers nums;  PSpotFunction pSpot;  DevScreen *devS; {
  PCard8 graydest, graysrc, grayStop;
  Cd cd;
  PGrayQ gqbase, curgqp;
  cardinal curgq;
  real a, b, c, d, ri, rj, curGray, grayDec;
  integer i, j, k, chunkdisp;
  integer area = nums->w * nums->h;
  gqbase = GetPatternBase((cardinal)area);
  curgqp = gqbase;  curgq = 0;
  INIT_RAND (1);
  
  a =  (real)nums->iv2 / (real)area;
  b = -(real)nums->iu1 / (real)area;
  c =  (real)nums->iu2 / (real)area;
  d =  (real)nums->iv1 / (real)area;
  
  DURING
  for (j = 0; j < nums->h; j++)
    {
    rj = (real)j + fpHalf;
    for (i = 0; i < nums->w; i++)
      {
      real rval;
      integer ival;
      ri = (real)i + fpHalf;
      cd.x = FractPart(ri * a + rj * c) * fpTwo - fpOne;
      cd.y = FractPart(ri * b + rj * d) * fpTwo - fpOne;
      PushPCd(&cd);
      if (psExecute(pSpot->ob)) RAISE((int)GetAbort(), (char *)NIL);
      PopPReal(&rval);
      if (os_fabs(rval) > 1.1)  RangeCheck();
      rval = (rval + fpOne) * 32768.0;
      RRoundP(&rval, &rval);
      ival = (integer)rval;
      curgqp->index = curgq;
      curgqp->val =
        (ival > MAXcardinal) ? MAXcardinal : ((ival<0) ? 0 : ival);
      curgq++;  curgqp++;
      }
    }
  HANDLER
    {FreePatternBase(gqbase); RERAISE;}
  END_HANDLER;
  SortGQ(gqbase, curgq);
  curgqp = gqbase + (area - 1);
  grayDec = fpOne / (real)area;
  curGray = fpOne;
  while (true)
    {
    real gray = MAXCOLOR * curGray;
    RRoundP(&gray, &gray);
    devS->thresholds[curgqp->index] = (integer)gray;
    if (curgqp-- == gqbase) break;
    curGray -= grayDec;
    }
  FreePatternBase(gqbase);
  graydest = devS->thresholds + area;
  chunkdisp = 0;
  for (k = 1; k < nums->rept; k++)
    {
    chunkdisp += nums->disp;
    if (chunkdisp >= nums->w) chunkdisp -= nums->w;
    grayStop = devS->thresholds;
    for (j = 0; j < nums->h; j++)
      {
      grayStop += nums->w;
      graysrc = grayStop - chunkdisp;
      for (i = 0; i < nums->w; i++)
	{
	*(graydest++) = *(graysrc++);
	if (graysrc >= grayStop) graysrc -= nums->w;
	}
      }
    }
  } /* end of CalcThresholds */


public procedure GenerateThresholds(pSpot, devS)
  PSpotFunction pSpot; DevScreen *devS; {
  Numbers nums;
  real xSpotSize, ySpotSize, xtdist, ytdist;
  GetXDistYDist(&xtdist, &ytdist);
  xSpotSize = fp72 / (pSpot->freq * xtdist);
  ySpotSize = fp72 / (pSpot->freq * ytdist);
  FreqAngleToNumbers(xSpotSize, ySpotSize, pSpot->angle, &nums);
  CalcThresholds(&nums, pSpot, devS);
  }


