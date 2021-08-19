/*  global_color.c 

              Copyright 1983, 1986 -- Adobe Systems, Inc.
            PostScript is a trademark of Adobe Systems, Inc.
NOTICE:  All information contained herein or attendant hereto is, and
remains, the property of Adobe Systems, Inc.  Many of the intellectual
and technical concepts contained herein are proprietary to Adobe Systems,
Inc. and may be covered by U.S. and Foreign Patents or Patents Pending or
are protected as trade secrets.  Any dissemination of this information or
reproduction of this material are strictly forbidden unless prior written
permission is obtained from Adobe Systems, Inc.

Steve Schiller: Wed Mar 21 13:11:37 1990
Jim Sandman: Mon Oct  9 10:25:34 1989
End Edit History */


#include "atm.h" 

#if ATM
#include "pubtypes.h"
#include "font.h"
#include "buildch.h"
extern PBuildCharProcs bprocs;
#define os_labs(x) ABS(x)
#else
#include PACKAGE_SPECS
#include BASICTYPES
#include EXCEPT
#include FP
#define FixedHalf 0x8000L
#define FixedOne 0x10000
typedef struct {
  char *ptr;		/* Ptr to bytes in buffer */
  Card32 len;		/* Number of bytes in buffer */
  } GrowableBuffer, *PGrowableBuffer;
#endif

#include "glbclr.h"

private int SortStems();
private void SelectionSort();
private void CullCounters();
private boolean SimpleCounter();
private void SortCounters();
private void AlignIsolatedStems();
private void FixBands();
private PGlbClr ExtendToAnchr();
private Fixed Overlap();
private void FixOnePath();
private void ClumpCntrs();
private void SortGroupsByFrac();
private boolean CounterGt();

#if ATM
#if PROTOTYPES
boolean GlobalColoring(PGlbClr stems, PGlbCntr cntrs, PGrowableBuffer b3, Fixed ef, integer nCntr, integer nStems);
int SortStems(PGlbClr stems, PGlbClr *ss);
private void SelectionSort(PGlbClr *ss, int first, int stop);
private void CullCounters(
  PGlbClr *ss, int ns, PGlbCntr cntrs, PGlbCntr *cs, integer *pNcntr);
boolean SimpleCounter(PGlbClr *ss, int ns, PGlbCntr cntr);
private void SortCounters(PGlbClr stems, PGlbCntr *cs, int nc);
private void AlignIsolatedStems(PGlbClr *ss, int ns);
private void FixBands(PGlbClr *ss, int ns, PGlbCntr *cs, Fixed ef);
private PGlbClr ExtendToAnchr(PGlbClr maxStem);
private Fixed Overlap(PGlbClr s1, PGlbClr s2);
private void FixOnePath(PGlbClr maxStem, PGlbCntr *cs, Fixed ef);
private void ClumpCntrs(PGlbCntr *cs, int nc, Fixed diam);
private void SortGroupsByFrac(PGlbCntr *cs, int nc);
private boolean CounterGt(PGlbCntr c1, PGlbCntr c2);
#endif
#endif

private int SortStems(stems, ss) PGlbClr stems, *ss; {
  register int i, j;
  int ne, nx;
  PGlbClr stm;
  
  ne = 0;
  /* Put all the non-yflg items first. */
  for (stm=stems; stm!=NULL; stm=stm->next) {
    if (!stm->yflg) ss[ne++] = stm;
  }
  nx = ne;
  /* Put all the yflg items last. */
  for (stm=stems; stm!=NULL; stm=stm->next)
    if (stm->yflg) ss[ne++] = stm;
  /* At this point i should be index of first 'yflg' item. */
  SelectionSort(ss, 0, nx);
  SelectionSort(ss, nx, ne);
  for (i=0; i<ne; i++) ss[i]->index = i;
  return ne;
}

private void SelectionSort(ss, first, stop)
PGlbClr *ss;
int first, stop;
{
  register int i, j, minI;
  Fixed min;
  PGlbClr tmp;
  
  for (i=first; i<stop-1; i++) {
    min = ss[i]->f;
    minI = i;
    for (j=i+1; j<stop; j++) {
      if (min > ss[j]->f) {minI = j; min = ss[j]->f;}
    }
    if (minI != i) {
      tmp = ss[i];
      ss[i] = ss[minI];
      ss[minI] = tmp;
    }
  }
}

/* Select the simple counters and transfer them to an array */
private void CullCounters(ss, ns, cntrs, cs, pNcntr)
PGlbClr *ss;
PGlbCntr cntrs, *cs;
int ns; integer *pNcntr;
{
  int i, nctr;
  
  nctr = 0;
  /* Enter coutners. */
  while (cntrs != NULL) {
    if (SimpleCounter(ss, ns, cntrs)) cs[nctr++] = cntrs;
    cntrs = cntrs->next;
  }
  *pNcntr = nctr;
}

private boolean SimpleCounter(ss, ns, cntr)
PGlbClr *ss;
PGlbCntr cntr;
int ns;
{
#define NINTERVAL 12
#define Even(m) (~(m) & 1)
#define Odd(m) ((m) & 1)
  /* This could done more efficiently out of the calling loop,
     so we could process all counters belonging to a single
     upper (leftmost) band in the same loop and maintain the
     'mask' state developed below.  But... */
  int iUp, iLw, is, sum, len;
  register int imn, imx, ni, inc, j, k;
  short int mn, mx, mask[2*NINTERVAL];
  PGlbClr stem;
  
  ni = 2;
  mask[0] = FTrunc(MAX(cntr->upper->mn, cntr->lower->mn));
  mask[1] = FTrunc(MIN(cntr->upper->mx, cntr->lower->mx));
  len = mask[1] - mask[0];
  iLw = cntr->lower->index;
  iUp = cntr->upper->index;
  for (is=iLw+1; is<iUp; is++) {
    stem = ss[is];
    mn = FTrunc(stem->mn);
      /* I assume all points on character space fall in ints. */
    mx = FTrunc(stem->mx);
    if (mn >= mask[ni-1]) continue;
    if (mx <= mask[0]) continue;
    if (mn < mask[0]) mn = mask[0];
    if (mx > mask[ni-1]) mx = mask[ni-1];
    for (imn=0; mn > mask[imn]; imn++);
    for (imx=imn; mx > mask[imx]; imx++);
    if (Odd(imn) && mn == mask[imn]) ++imn;
    if (Odd(imx) && mx == mask[imx]) ++imx;
    inc = Odd(imx) + Odd(imn) - (imx - imn);
    Assert(Even(inc));
    if (inc == -ni) return false;	/* entire overlap is masked. */
    if (ni + inc > 2*NINTERVAL) /* can't process this interval */ continue;
    for (j=ni-1, k=j+inc; j>= 0; j--) {
      if (j < imn) break;
      if (j > imx ) mask[k--] = mask[j];
      else {
        if (j == imx) {mask[k--] = mask[j]; if (Odd(imx)) mask[k--] = mx;}
        if (j == imn) {if (Odd(imn)) mask[k--] = mn;}
      } 
    }
    ni += inc;
  }
  for (sum=0,j=0; j<ni; j++) sum += Even(j) ? -mask[j] : mask[j];
  return sum*2 >= len;
#undef NINTERVAL
#undef Even
#undef Odd
}

/* Not really sorting, just putting all counters with the same lower
   (leftmost) stem in a bin hanging off that lower (leftmost) stem. */
private void SortCounters(stems, cs, nc)
PGlbClr stems;
int nc;
PGlbCntr *cs;
{
  register int i;
  register PGlbCntr list;
  
  while (stems!=NULL) {stems->clist = NULL; stems=stems->next;}
  for (i=0; i<nc; i++) {
    list = cs[i]->lower->clist;
    cs[i]->next = list;
    cs[i]->lower->clist = cs[i];
  }
}

private void AlignIsolatedStems(ss, ns)
PGlbClr *ss;
int ns;
{
  register int i, jj;
  
  /* Look for nested stems: */
  for (i=0; i<ns; i++) {
    for (jj=i+1;
         jj<ns && (ss[jj]->n <= ss[i]->n || ss[jj]->f == ss[i]->f); jj++) {
      register PGlbClr ind, dep;
      Fixed totD, wf, wn, d;
      if (ss[i]->yflg != ss[jj]->yflg) break;
      /* ss[jj] is nested inside ss[i] */
      if (ss[i]->alignable) {
        /* If both are alignable, make the first stem in the array
           the independent one: */
        if (ss[jj]->alignable) {ind = ss[i]; dep = ss[jj];}
        else {ind = ss[jj]; dep = ss[i];}
      } else {
        if (ss[jj]->alignable) {ind = ss[i]; dep = ss[jj];}
        else continue;
      }
      totD = dep->nn-dep->ff-(ind->nn-ind->ff);
      wf = ind->f - dep->f;
      wn = dep->n - ind->n;
      d = FRoundF(fxfrmul(totD, fixratio(wf, wn+wf)));
      dep->ff = ind->ff - d;
      dep->nn = ind->nn + totD - d;
    }
  }
}

private void FixBands(ss, ns, cs, ef)
register PGlbClr *ss;
int ns;
register PGlbCntr *cs;
Fixed ef;
{
  int is, newPl, maxPl;
  register PGlbCntr clist;
  boolean useNew;
  PGlbClr maxStem;
  
  /* Initialization: */
  for (is=0; is<ns; is++) {
    ss[is]->pathLen = 0;
    ss[is]->parent = NULL;
  }
  
  maxPl = 0;
  maxStem = NULL;
  /* First set up parent pointers and a first pass at longest path: */
  for (is=0; is<ns; is++) {
    newPl = ss[is]->pathLen + 1;
    for (clist = ss[is]->clist; clist != NULL; clist = clist->next) {
      if (clist->upper->anchored) continue;
      useNew = clist->upper->pathLen < newPl;
      if (!useNew && clist->upper->pathLen == newPl) {
        /* Break tie by choosing parent with most overlap */
        Fixed oldOvpl, newOvlp;
        newOvlp = Overlap(ss[is], clist->upper);
        oldOvpl = Overlap(clist->upper->parent->lower, clist->upper);
        useNew = newOvlp > oldOvpl;
        if (!useNew && newOvlp == oldOvpl) {
          /* Break tie by choosing closest one: */
          useNew = clist->w < clist->upper->parent->w;
        }
      }
      if (useNew) {
        clist->upper->pathLen = newPl;
        clist->upper->parent = clist;
        if (newPl > maxPl) {maxPl = newPl; maxStem = clist->upper;}
      } 
    }	/* End of loop for all counters hanging off of ss[is] */
  }
  if (maxStem == NULL) return;
  maxStem = ExtendToAnchr(maxStem);
  FixOnePath(maxStem, cs, ef);
  /* Now find longest of remaining paths each time through loop until none */
  while (true) {
    maxPl = 0;
    maxStem = NULL;
    for (is=0; is<ns; is++) {
      if (ss[is]->anchored || ss[is]->parent == NULL) continue;
      newPl = ss[is]->pathLen = ss[is]->parent->lower->pathLen + 1;
      if (newPl > maxPl) {maxPl = newPl; maxStem = ss[is];}
    }
    if (maxStem == NULL) return;
    maxStem = ExtendToAnchr(maxStem);
    FixOnePath(maxStem, cs, ef);
  }
}

private PGlbClr ExtendToAnchr(maxStem)
/* Extend maxStem to an anchoered stem below it if possible: */
PGlbClr maxStem;
{
  Fixed minW;
  PGlbCntr minCntr;
  register PGlbCntr clist;
  
  minW = FixInt(10000);
  minCntr = NULL;
  for (clist = maxStem->clist; clist != NULL; clist = clist->next) {
    if (clist->w < minW) {minW = clist->w; minCntr = clist;}
  }
  if (minCntr != NULL)
    {minCntr->upper->parent = minCntr; maxStem = minCntr->upper;}
  return maxStem;
}

private Fixed Overlap(s1, s2)
PGlbClr s1, s2;
{
  Fixed min, max;
  min = MAX(s1->mn, s2->mn);
  max = MIN(s1->mx, s2->mx);
  return max - min;
}

private void FixOnePath(maxStem, cs, ef)
PGlbClr maxStem;
PGlbCntr *cs;
Fixed ef; /* expansion factor */
{
#define ISO 2
#define Fixed0p6 39322	/* 0.6 in fixed point */
  PGlbClr stem;
  int nc, ic, top, fudge, split;
  int iStmW, iCntW;
  Fixed diam, fudgeFac, rTop, rBot, totW, err, stemW, newNN;
  
  nc = 0;
  iStmW = 0;
  for (stem=maxStem; stem->parent != NULL; stem = stem->parent->lower) {
    if (stem != maxStem && stem->anchored) break;
    iStmW += FTrunc(stem->w);
    cs[nc++] = stem->parent;
  }
  if (nc > ISO) {
    /* Then remove alignments of stems in chain: */
    for (ic=0; ic<nc; ic++) {
      cs[ic]->upper->alignable = false;
      cs[ic]->lower->alignable = false;
    }
  }
  iStmW += FTrunc(stem->w);
  diam = fixdiv(12 * (maxStem->n-maxStem->f), maxStem->cn-maxStem->cf);
  diam = MIN(diam, Fixed0p6);
  ClumpCntrs(cs, nc, diam);
  SortGroupsByFrac(cs, nc);
  /* First, assume that all counters will be rounded down: */
  for (ic=0, iCntW = 0; ic<nc; ic++) iCntW += FTrunc(cs[ic]->grpMean);
  rBot = stem->f;
  rTop = maxStem->n;
  fudgeFac = FixedOne;
  if (stem->anchored) {fudgeFac -= FixedHalf; rBot = stem->ff;}
  if (maxStem->anchored) {fudgeFac -= FixedHalf; rTop = maxStem->nn;}
  /* Now calculate 'split' as the actual number we should round down: */
  split = nc - (FTrunc((totW = rTop-rBot) + FixedHalf) - (iCntW+iStmW));
  while (split < 0) {
    /* This loop should be symetric to the one below, but it isn't
       because floor(x+1) does not always equal 1+floor(x), (for
       example: .9999999). Thus, instead of just adding nc to split
       I have to calculate how much the sum of the floors changes
       and add that: */
    int newCntW = 0;
    for (ic=0; ic<nc; ic++) {
      cs[ic]->grpMean += FixedOne;
      newCntW += FTrunc(cs[ic]->grpMean);
    }
    split += newCntW - iCntW;
    iCntW = newCntW;
  }
  while (split > nc) {
    for (ic=0; ic<nc; ic++) cs[ic]->grpMean -= FixedOne;
    split -= nc;
  }
  fudge = FTrunc(fixmul(fixmul(totW,fudgeFac),ef) + FixedHalf);
  /* Check to see if we are going to split a group: */
  if (fudge > 0 && split > 0 && (top=cs[split-1]->group) != split-1) {
    for (ic=0; cs[ic]->group<top; ic++);
    if (split-ic <= fudge) split = ic;	/* move split to start of group. */
    else {
      if (top-split+1 <= fudge) split = top+1;	/* move split to end of group. */
      /* else just split the group. */
    }
  }
  /* Round counters: */
  iCntW = 0;
  for (ic=0; ic<nc; ic++) {
    /* use grpMean field to record rounded value: */
    if (ic < split) cs[ic]->grpMean = FTruncF(cs[ic]->grpMean);
    else cs[ic]->grpMean = FTruncF(cs[ic]->grpMean) + FixedOne;
    iCntW += FTrunc(cs[ic]->grpMean);
  }
  err = FixInt(iStmW + iCntW) - totW;
  /* Now we can set the absolute position of stems: */
  if (!maxStem->anchored) {
    stemW = maxStem->nn - maxStem->ff;
    if (stem->anchored) maxStem->nn = stem->ff + FixInt(iStmW + iCntW);
    else maxStem->nn = FTruncF(maxStem->n + err/2 + FixedHalf);
    maxStem->ff = maxStem->nn - stemW;
    /* Reset stem so it wont be looked at again: */
    maxStem->anchored = true;
    maxStem->pathLen = 0;
  } else {
    if (stem->anchored)
      Assert(err == 0);
  }
  stem=maxStem;
  do {
    newNN = stem->ff - stem->parent->grpMean;
    stem = stem->parent->lower;
    if (stem->anchored) break;
    /* reposition next stem: */
    stemW = stem->nn - stem->ff;
    stem->nn = newNN;
    stem->ff = stem->nn - stemW;
    /* Reset stem so it wont be looked at again: */
    stem->anchored = true;
    stem->pathLen = 0;
  } while (stem->parent != NULL);
}

private void ClumpCntrs(cs, nc, diam)
PGlbCntr *cs;
int nc;
Fixed diam;
{
  int j, minI;
  Fixed minDiff, tmp;
  
  for (j=0; j<nc; j++) {
    cs[j]->group = j;
    cs[j]->grpMn = cs[j]->grpMx = cs[j]->grpMean = cs[j]->w;
    cs[j]->tested = false;
  }
  do {
    minI = -1;
    /* Find the smallest untested gap between counters: */
    for (j=1; j<nc; j++) {
      if (!cs[j]->tested) {
        tmp = cs[j-1]->w-cs[j]->w;
        if (tmp < 0) tmp = -tmp;
        if (minI < 0 || minDiff > tmp) {
          minI = j;
          minDiff = tmp;
        }
      }
    }
    /* test to see if the two groups on either side of the gap can be merged: */
    if (minI >= 0) {
      Fixed newMn, newMx;
      int k;
      cs[minI]->tested = true;
      newMn = MIN(cs[minI-1]->grpMn, cs[minI]->grpMn);
      newMx = MAX(cs[minI-1]->grpMx, cs[minI]->grpMx);
      if (newMx - newMn <= diam) {
        for (k=minI-1; k>0 && cs[k-1]->group==minI-1; k--);
        for (j=k; j<minI; j++) cs[j]->group = cs[minI]->group;
        minI = k;
        k = cs[minI]->group;
        for (j=minI; j<=k; j++) {cs[j]->grpMn = newMn; cs[j]->grpMx = newMx;}
      }
    }
  } while (minI >= 0);
  /* Now set grpMean for each group: */
  for (j=0; j<nc; j++) {
    if (cs[j]->group > j) {
      int k;
      Fixed mean;
#ifdef TRUEMEAN
      mean = 0;
      for (k=j; k<=cs[j]->group; k++) mean += cs[k]->w;
      mean /= cs[j]->group - j + 1;
#else
      /* By using the midpoint instead of the true mean we guarentee that
         adjacent counters will be equal or rounded in an order-preserving
         fashion: */
      mean = (cs[j]->grpMn >> 1) + (cs[j]->grpMx >> 1);
#endif
      for (k=j; k<=cs[j]->group; k++) cs[k]->grpMean = mean;
      j = cs[j]->group;
    }
  }
}

private void SortGroupsByFrac(cs, nc)
PGlbCntr *cs;
int nc;
{
  int i, j, minI;
  PGlbCntr tmp;
  
  for (i=0; i<nc-1; i++) {
    minI = i;
    for (j=i+1; j<nc; j++) {
      if (CounterGt(cs[minI], cs[j])) minI = j;
    }
    if (minI != i) {
      tmp = cs[i];
      cs[i] = cs[minI];
      cs[minI] = tmp;
    }
  }
  /* Have to reset ->group to be index of last element in group: */
  for (i=0; i<nc;) {
    for (j=i; j<nc-1; j++) if (cs[j+1]->group != cs[j]->group) break;
    while (i <= j) cs[i++]->group = j;
  }
}

private boolean CounterGt(c1, c2)
PGlbCntr c1, c2;
{
  Fixed f1, f2, i1, i2;
  boolean rUp1, rUp2;
  
  if (c1->group != c2->group) {
    i1 = FTruncF(c1->grpMean);
    i2 = FTruncF(c2->grpMean);
    f1 = c1->grpMean - i1;
    f2 = c2->grpMean - i2;
    rUp1 = i1 == 0 && f1 >= FixedHalf;
    rUp2 = i2 == 0 && f2 >= FixedHalf;
    /* Be sure that counters you don't want rounded to 0 are gt all others: */
    if (rUp1 != rUp2) return rUp1;
    if (f1 > f2) return true;
    if (f1 < f2) return false;
  }
  if (c1->lower->yflg)
    return c1->lower->cn < c2->lower->cn;
  else
    return c1->lower->cn > c2->lower->cn;
}

public boolean GlobalColoring(stems, cntrs, b3, ef, nCntr, nStems)
PGlbClr stems; PGlbCntr cntrs; PGrowableBuffer b3; Fixed ef;
integer nCntr, nStems;
{
  PGlbClr *stemSort;
  PGlbCntr *cntrSort;
  long i;
  if (nStems == 0) return true;
  i = (long) sizeof(PGlbCntr) * nCntr + (long) sizeof(PGlbClr) * nStems;
  if (b3->len < i) {
#if !ATM
    return false;
#else
    if (!(*bprocs->GrowBuff)(b3, i - b3->len, false))
      return false;
#endif
    }
  stemSort = (PGlbClr *)(b3->ptr);
  cntrSort = (PGlbCntr *)(b3->ptr + (long) sizeof(PGlbClr) * nStems);
  i = SortStems(stems, stemSort);
  Assert(nStems == i);
  for (i=0; i<nStems; i++) stemSort[i]->alignable = true;
  CullCounters(stemSort, nStems, cntrs, cntrSort, &nCntr);
  SortCounters(stems, cntrSort, nCntr);
  FixBands(stemSort, nStems, cntrSort, ef);
  AlignIsolatedStems(stemSort, nStems);
  return true;
}

