/*****************************************************************************

    bm12comp.c
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.
    
    Created: 04Aug86 Leo
    
    Modified:
    
    24Feb87 Jack  began composite work
    08Mar87 Jack  changed args to StraightCopy cum SingleForwardCopy
    26Mar87 Jack  "rb<<2" in call to PNoSrcAsm, 2 other bugs fixed
    01Apr87 Jack  use s_ofs for PNoSrcAsm as well as CopyAsm
    07Apr87 Jack  gun PNoSrcAsm because it has wrong pat alignment
    03Nov87 Jack  change order of left/right mask tests in FuncRRect
    08Dec87 Jack  code around gcc bug (better code)
    25Jan88 Jack  rework cases in MoveRect
    28Mar88 Jack  convert leftShift to bits before call CopyRect
    02Sep88 Jack  add that forgotten FIRSTF2OP + 13 case in MoveRect!
    08Feb89 Jack  use MPLOG2BD for frameMPLOG2BD
    01Mar89 Ted   diffed from v006/v001
    29Feb90 Terry formatted a bit and added SoverRect special case
    20Jul90 Ted   bm12comp.c

******************************************************************************/

#import PACKAGE_SPECS
#import DEVICE
#import DEVPATTERN
#import BINTREE
#import "bm12.h"

/* MOVERECTSAFETY added to moverectlen so we can safely read words at end.
 * moverectlen stores the size of moverectbuff in words.
 */
#define MOVERECTSAFETY 1
static uint *moverectbuff;
static int moverectlen;

/*****************************************************************************
    MoveLine Dispatch Table
    is a two-dimensional table offering a choice of copyline
    routines.  The first index is the lToR flag:

	    lToR == 0 => right-to-left processing of words
	    lToR == 1 => left-to-right processing of words

    The second index is the source type, from the choices:
    
	    SCON 0 (constant)
	    SBMU 1 (unaligned bitmap)
	    SPAT 2 (pattern)
	    SBMA 3 (aligned bitmap)
******************************************************************************/

static const void (*(moveLineDispatch[2][6][4]))() = {
 {{WCOPYConLine, RWCOPYBmULine, WCOPYPatLine, RWCOPYBmALine },
  {WF0ConLine,   WF0BmRLine,    WF0PatLine,   WF0BmRLine },
  {WF1ConLine,   WF1BmRLine,    WF1PatLine,   WF1BmRLine },
  {WF2ConLine,   WF2BmRLine,    WF2PatLine,   WF2BmRLine },
  {WF3ConLine,   WF3BmRLine,    WF3PatLine,   WF3BmRLine },
  {WF4ConLine,   WF4BmRLine,    WF4PatLine,   WF4BmRLine } },
 {{WCOPYConLine, WCOPYBmULine,  WCOPYPatLine, WCOPYBmALine},
  {WF0ConLine,   WF0BmULine,    WF0PatLine,   WF0BmALine },
  {WF1ConLine,   WF1BmULine,    WF1PatLine,   WF1BmALine },
  {WF2ConLine,   WF2BmULine,    WF2PatLine,   WF2BmALine },
  {WF3ConLine,   WF3BmULine,    WF3PatLine,   WF3BmALine },
  {WF4ConLine,   WF4BmULine,    WF4PatLine,   WF4BmALine }}
};

/*****************************************************************************
	SetUpSource
	Sets up the given LineSource to correspond to the given BitsOrPatInfo.
******************************************************************************/

void BM12SetUpSource(BitsOrPatInfo **psi, LineOperation *lo, int func,
    int lToR) /* psi indirected so we can zero it for real SCON */
{
    BitsOrPatInfo *sI = *psi;
    int lShift;
    
    switch (sI->type)
    {
	case bitmapType:
	    lo->source.data.bm.pointer   = sI->data.bm.pointer;
	    lo->source.data.bm.leftShift = sI->data.bm.leftShift;
	    lo->source.type = sI->data.bm.leftShift ? SBMU : SBMA;
	    if (!lToR) /* now HERE's where we adjust for lToR! */
	    { 
		lo->source.data.bm.pointer += lo->loc.numInts;
		if (lo->source.type == SBMU)
		    lo->source.data.bm.pointer++;
		lo->loc.dstPtr += lo->loc.numInts;
	    }
	    break;
	case patternType:
	    lo->source.data.pat.base = sI->data.pat.yInitial;
	    if (PAABS(sI->data.pat.n) == 1)
	    {
		lo->source.type = SCON;
		lo->source.data.cons.value = *lo->source.data.pat.base;
		break;
	    }
	    lo->source.type = SPAT;
	    lo->source.data.pat.pointer = sI->data.pat.xInitial;
	    lo->source.data.pat.limit =
	    sI->data.pat.xInitial + PAABS(sI->data.pat.n);
	    break;
	case constantType:
	    lo->source.type = SCON;
	    lo->source.data.cons.value = sI->data.cons.value;
	    *psi = NULL;
	    break;
	default:
	    CantHappen();
    }
    lo->proc = moveLineDispatch[lToR][func][lo->source.type];
}

/*****************************************************************************
	Macros
******************************************************************************/

#define SOURCEADVANCE(sI, lS) \
    if (sI) { \
	if (sI->type == patternType) \
	    BM12MRPatternAdvance(&(lS),(sI)); \
	else \
	    (lS).data.bm.pointer += (sI)->data.bm.n; \
    }

#define CONSTSOURCE(n, val) \
    lO[(n)].source.type = SCON; \
    lO[(n)].source.data.cons.value = (val); \
    lO[(n)].proc = moveLineDispatch[lToR][WCOPY][SCON]; \
    sI[(n)] = NULL;

#define BUFFERSOURCE(n, func) \
    lO[(n)].source.type = SBMA; \
    lO[(n)].source.data.bm.leftShift = 0; \
    lO[(n)].source.data.bm.pointer = moverectbuff; \
    if (!lToR) { \
	lO[(n)].source.data.bm.pointer += lO[(n)].loc.numInts; \
	lO[(n)].loc.dstPtr += lO[(n)].loc.numInts; \
    } \
    lO[(n)].proc = moveLineDispatch[lToR][func][SBMA];

/*****************************************************************************
	BM12MRInitialize
	Sets up the static moverectbuff used by subsequent MoveRect
	calls.  Has to be static so we can do quintuple mapping.
	Moverectbuff now growable for wide composites. [Ted 21Jul89]
******************************************************************************/

void BM12MRInitialize()
{
    moverectlen = BM12_INITIAL_MRBUFFLEN;
    moverectbuff = (uint *)malloc(moverectlen);
}


/*****************************************************************************
	BM12MRMasks
******************************************************************************/

void BM12MRMasks(uint *leftMask, uint *rightMask, int
    d_ofs, int *num_ints, int num_last /* in pixels */)
{
    int bits, fl2bd = BM12LOG2BD;
    
    d_ofs <<= fl2bd; num_last <<= fl2bd; /* convert these to bits */
    *leftMask = leftBitArray[d_ofs];
    bits = (*num_ints << BM12SCANSHIFT) + num_last;
    /* total to be transferred */
    bits += d_ofs;   /* now counted from beginning of first int to last bit */
    /* Now calculate number of whole words */
    *num_ints = bits >> BM12SCANSHIFT;
    bits &= BM12SCANMASK; /* from start of last int to last pixel */
    *rightMask = rightBitArray[bits];
    if (*num_ints == 0)
		*leftMask &= *rightMask;
    else if (*rightMask == 0) {
	/* don't need to operate on THAT int! */
	(*num_ints)--;
	*rightMask = ONES;
    }
}

/*****************************************************************************
	BM12MRMoveRect
	MegaPixel compositing code.  Switches on steps and op type.
******************************************************************************/

void BM12MRMoveRect(BitsOrPatInfo *sI[3], BitsOrPatInfo *dI, int op, int d_ofs,
		int height, int argNumInts, int numLastPix, int lToR)
{
    /* dI must be Bitmap variant of BitsOrPatInfo */
    LineOperation lO[4];
    int numInts = argNumInts;
    static const uchar steps[FIRSTF4OP] =
    {1,1,3,0,1,  3,1,1,1,4,1,1,2,1,  2,9,2,2,4,9,3,9,4,4,4,4,9,4,  4,9,4,4};
    
    /* If numInts is larger than current moverectlen, expand moverectbuff */
    if (numInts*4 > moverectlen - MOVERECTSAFETY) {
	free(moverectbuff); /* Free old guy */
	moverectlen = numInts*4 + MOVERECTSAFETY; /* Grow moverectbuff */
	moverectbuff = (uint *)malloc(moverectlen);
    }
    
    BM12MRMasks(&lO[0].loc.leftMask, &lO[0].loc.rightMask, d_ofs, &numInts,
	numLastPix);
    lO[0].loc.numInts = numInts; /* common in lO[0], copy to others later */
    lO[0].loc.dstPtr = dI->data.bm.pointer;
    switch (steps[op])
    {
	case 0:	   /* nop */
	    return;
	case 1:
	    switch (op)
	    {
		case 0:			/* Clear to 0 */
		case 1:			/* Set to 1 */
		    CONSTSOURCE(0, (op == 0) ? 0 : ONES);
		    break;
		case 4:			/* Highlight */
		    HighlightRect(
			&lO[0], 0, dI->data.bm.n*sizeof(SCANTYPE), height);
		    return;
		case FIRSTF1OP + 1:	/* D = D*S */
		    SetUpSource(&sI[0], &lO[0], WF0, lToR);
		    break;
		case FIRSTF1OP + 2:	/* D = S */
		    SetUpSource(&sI[0], &lO[0], WCOPY, lToR);
		    /* Insert optimized rect mover */
		    if (lToR && sI[0] && (sI[0]->type == bitmapType))
		    {
			lO[0].source.data.bm.leftShift <<= BM12LOG2BD;
			CopyRect(&lO[0], sI[0]->data.bm.n*sizeof(SCANTYPE),
			    dI->data.bm.n*sizeof(SCANTYPE), height);
			return;
		    }
		    break;
		case FIRSTF1OP + 3:	/* D = (1-S)*D */
		case FIRSTF1OP + 5:	/* D = S + D */
		case FIRSTF1OP + 6:	/* D = S + D - S*D */
		case FIRSTF1OP + 8:	/* D = 1 - ((1-S) + (1-D)) */
		    {
			static const uchar cases3568[6]={WF2,0,WF1,WF3,0,WF4};
			SetUpSource(&sI[0], &lO[0],
				    (int)cases3568[op-FIRSTF1OP-3], lToR);
		    }
		    break;
		default:
		    CantHappen();
	    } /* switch (op) */
	    while (height-- > 0)
	    {
		(*(lO[0].proc))(&lO[0]);
		SOURCEADVANCE(sI[0], lO[0].source);
		lO[0].loc.dstPtr += dI->data.bm.n;
	    }
	    return;
	case 2:
	    lO[1].loc = lO[0].loc;
	    switch (op)
	    {
		case FIRSTF1OP + 7:	/* D = 1-S */
		    sI[1] = sI[0];	/* promote for SOURCEADVANCE */
		    CONSTSOURCE(0, ONES);
		    SetUpSource(&sI[1], &lO[1], WF2, lToR);
		    break;
		case FIRSTF2OP + 0:	/* D = S0 + (D*(1-S1)) */
		    sI[2] = sI[1];
		    sI[1] = sI[0];
		    sI[0] = sI[2];	/* swap for SOURCEADVANCE */
		    SetUpSource(&sI[0], &lO[0], WF2, lToR);
		    SetUpSource(&sI[1], &lO[1], WF1, lToR);
#if 1
		    /* Left-to-right bitmap-to-bitmap data-alpha-align Sover */
		    if (lToR && sI[0] && (sI[0]->type == bitmapType) &&
				sI[1] && (sI[1]->type == bitmapType) &&
			(lO[0].source.data.bm.leftShift ==
			 lO[1].source.data.bm.leftShift) &&
			(sI[0]->data.bm.n == sI[1]->data.bm.n))
		    {
			lO[1].source.data.bm.leftShift <<= BM12LOG2BD;
			SoverRect(&lO[0], &lO[1], sI[0]->data.bm.n*4,
				  dI->data.bm.n*4, height);
			return;
		    }
#endif
		    break;
		case FIRSTF2OP + 2:	/* D = S0*(1-S1)  */
		case FIRSTF2OP + 3:	/* D = S0*S1 */
		    {
			static const char cases23op1[2] = {WF2, WF0};
			SetUpSource(&sI[0], &lO[0], WCOPY, lToR);
			SetUpSource(&sI[1], &lO[1],
			    	    (int)cases23op1[op-FIRSTF2OP-2], lToR);
		    }
		    break;
		default:
		    CantHappen();
	    } /* switch (op) */
	    while (height-- > 0)
	    {
		(*(lO[0].proc))(&lO[0]);
		(*(lO[1].proc))(&lO[1]);
		SOURCEADVANCE(sI[0], lO[0].source);
		SOURCEADVANCE(sI[1], lO[1].source);
		lO[0].loc.dstPtr += dI->data.bm.n;
		lO[1].loc.dstPtr += dI->data.bm.n;
	    }
	    return;
	case 3:
	    lO[2].loc = lO[1].loc = lO[0].loc;
	    lO[0].loc.dstPtr = moverectbuff;
	    lO[1].loc.dstPtr = moverectbuff;
	    switch (op)
	    {
		case 2:			/* Invert destination */
		    CONSTSOURCE(0, ONES);
		case FIRSTF1OP + 0:	/* D = (1-D)*S */
		    sI[1] = dI;
		    BUFFERSOURCE(2, WCOPY);
		    break;
		case FIRSTF2OP + 6:	/* D = D + S0*(1-S1) */
		    BUFFERSOURCE(2, WF1);
		    break;
		default:
				CantHappen();
	    } /* switch (op) */
	    SetUpSource(&sI[1], &lO[1], WF2, lToR);
	    if (sI[0])
		SetUpSource(&sI[0], &lO[0], WCOPY, lToR);
	    while (height-- > 0)
	    {
		(*(lO[0].proc))(&lO[0]);
		(*(lO[1].proc))(&lO[1]);
		(*(lO[2].proc))(&lO[2]);
		SOURCEADVANCE(sI[0], lO[0].source);
		SOURCEADVANCE(sI[1], lO[1].source);
		lO[2].loc.dstPtr += dI->data.bm.n;
	    }
	    return;
	case 4:
	    {
		static const uchar func2[10] =
		    {WF2, WF0, WF2, WF2, WF2, WF2, WF0, WF0, WF2, WF2};
		static const uchar func3[10] =
		    {WF2, WF2, WF2, WF0, WF2, WF0, WF2, WF2, WF2, WF0};
		int newop;
		
		lO[3].loc = lO[2].loc = lO[1].loc = lO[0].loc;
		lO[0].loc.dstPtr = lO[1].loc.dstPtr = moverectbuff;
		switch (op)
		{
		    case FIRSTF1OP + 4:	/* D = D ^ S */
			sI[1] = dI;
			sI[2] = sI[0];
			newop = 0;
			break;
		    case FIRSTF2OP + 4:	/* D = d & (s0 | ~s1) */
			sI[2] = sI[1];
			sI[1] = dI;
			newop = 1;
			break;
		    case FIRSTF2OP + 8:	/* D = (s0 & ~d) | (d & ~s1) */
		    case FIRSTF2OP + 9:	/* D = (s0 & ~d) | (d & s1) */
			sI[2] = sI[1];
			sI[1] = dI;
			newop = op - FIRSTF2OP - 6;	/* 2 or 3 */
			break;
		    case FIRSTF2OP + 10:  /* D = (s0 & ~s1) | (d & ~s0) */
		    case FIRSTF2OP + 11:  /* D = (s0 & ~s1) | (d & s0) */
			sI[2] = sI[0];
			newop = op - FIRSTF2OP - 6;	/* 4 or 5 */
			break;
		    case FIRSTF2OP + 13: /* D = (s0 & s1) | (d & s0) */
			sI[2] = sI[0];
			newop = 7;
			break;
		    case FIRSTF3OP + 0:  /* D = (s0 & s1) | (d & ~s2) */
		    case FIRSTF3OP + 2:  /* D = (s0 & ~s1) | (d & ~s2) */
		    case FIRSTF3OP + 3:  /* D = (s0 & ~s1) | (d & s2) */
			newop = op - FIRSTF3OP + 6;	/* 6, 8 or 9 */
			break;
		    default:
			CantHappen();
		} /* switch (op) */
		SetUpSource(&sI[0], &lO[0], WCOPY, lToR);
		SetUpSource(&sI[1], &lO[1], (int)func2[newop], lToR);
		SetUpSource(&sI[2], &lO[2], (int)func3[newop], lToR);
		BUFFERSOURCE(3, WF1);
		while (height-- > 0)
		{
		    (*(lO[0].proc))(&lO[0]);
		    (*(lO[1].proc))(&lO[1]);
		    (*(lO[2].proc))(&lO[2]);
		    (*(lO[3].proc))(&lO[3]);
		    SOURCEADVANCE(sI[0], lO[0].source);
		    SOURCEADVANCE(sI[1], lO[1].source);
		    SOURCEADVANCE(sI[2], lO[2].source);
		    lO[2].loc.dstPtr += dI->data.bm.n;
		    lO[3].loc.dstPtr += dI->data.bm.n;
		}
	    } /* case 4 */
	    return;
	default:
	    CantHappen();
    } /* switch (steps[op]) */
}


/*****************************************************************************
	BM12MRPatternAdvance
	Both parameters must be patterns.
******************************************************************************/

void BM12MRPatternAdvance(LineSource *lS, BitsOrPatInfo *sI)
{
    if (lS->type == SPAT)
    {
	lS->data.pat.base += sI->data.pat.n;
	if (lS->data.pat.base == sI->data.pat.yLimit)
	{
	    lS->data.pat.base = sI->data.pat.yBase;
	    lS->data.pat.pointer = sI->data.pat.yBase +
	    (sI->data.pat.xInitial - sI->data.pat.yInitial);
	    lS->data.pat.limit = sI->data.pat.yBase + PAABS(sI->data.pat.n);
	}
	else
	{
	    lS->data.pat.pointer += sI->data.pat.n;
	    lS->data.pat.limit += sI->data.pat.n;
	}
    }
    else
    {
	/* This is one of those one-word-wide patterns, which
	 * masquerades as a constant.  We use base as the pointer
	 * to the row of the pattern
	 */
	lS->data.pat.base += sI->data.pat.n;
	if (lS->data.pat.base == sI->data.pat.yLimit)
		lS->data.pat.base = sI->data.pat.yBase;
	lS->data.cons.value = *lS->data.pat.base;
    }
}












