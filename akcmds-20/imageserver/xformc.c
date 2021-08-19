/**********************************************************************
  
This is valuable proprietary information and trade secrets of Creative Circuits
Corporation and is not to be disclosed or released without the prior written
permission of an officer of the company.
  
Copyright (c) 1989 by Creative Circuits Corporation.
  
04/10/90 GRC, V1.0b1 release.
  
   C_XFORM.C
  
**********************************************************************/
#include "gStruct.h"

/*
 * Round a double to an int. The asm version just ends up being one line of
 * assembler. 
 */
static inline int 
R(double a)
{
    int             x;
#if ALLC
    x = (a > 0) ? (a +.5) : (a -.5);
#else
    /* Convert the double to an integer. */
    asm("fmove%.l %1,%0":"=dm"(x) :"f"(a));
#endif
    return x;
}


/*
 * Integer math version of forward DCT. 
 */
transformi(dg, blki, xform)
    gvl            *dg;
    char            blki[64];	/* These are signed 8 bit quantities */
unsigned        xform[64];
{
    int             xt, lx0, lx1, lx2, lx3, lx4, lx5, lx6, lx7;
    int             x, y;
    unsigned char   blk[64];
    short           temp[64];
    int             a0, a1, a2, a3, a4, a5, a6, a7;
    long int        b0, b1, b2, b3, b4, b5, b6, b7;
    long int        c0, c1, c2, c2b, c3, c3b, c4, c5, c6, c7;
    long int        cA, cB, cC, cD;

    /****** sine and cosine tables ******/
    int             spi16 = 100, spi8 = 196, s3pi16 = 284, spi4 = 362;
    int             s5pi16 = 426, s3pi8 = 473, s7pi16 = 502;
    int             cpi16 = 502, cpi8 = 473, c3pi16 = 426, cpi4 = 362;
    int             c5pi16 = 284, c3pi8 = 196, c7pi16 = 100;
    int             scale = 512;
    register int    y8;


    /***
    !      TRANSFORMS EACH ROW OF BLOCK
    ***/
    for (y = 0; y < 8; y++) {
	y8 = y << 3;

	a0 = ((int)blki[y8] + (int)blki[y8 + 7]) << 2;
	a1 = ((int)blki[y8 + 1] + (int)blki[y8 + 6]) << 2;
	a2 = ((int)blki[y8 + 2] + (int)blki[y8 + 5]) << 2;
	a3 = ((int)blki[y8 + 3] + (int)blki[y8 + 4]) << 2;
	a4 = ((int)blki[y8 + 3] - (int)blki[y8 + 4]) << 2;
	a5 = ((int)blki[y8 + 2] - (int)blki[y8 + 5]) << 2;
	a6 = ((int)blki[y8 + 1] - (int)blki[y8 + 6]) << 2;
	a7 = ((int)blki[y8] - (int)blki[y8 + 7]) << 2;

	b0 = a0 + a3;
	b1 = a1 + a2;
	b2 = a1 - a2;
	b3 = a0 - a3;
	b4 = a4;
	b5 = (long)(cpi4 * (long)(a6 - a5)) >> 9;
	if (b5 < 0)
	    b5++;
	b6 = (long)(cpi4 * (long)(a6 + a5)) >> 9;
	if (b6 < 0)
	    b6++;
	b7 = a7;


	c0 = (long)(cpi4 * (b0 + b1)) >> 9;
	if (c0 < 0)
	    c0++;
	c1 = (long)(cpi4 * (b0 - b1)) >> 9;
	if (c1 < 0)
	    c1++;
	c2 = (long)((spi8 * (long)b2) + (cpi8 * (long)b3)) >> 9;
	if (c2 < 0)
	    c2++;
	c3 = (long)((c3pi8 * (long)b3) - (s3pi8 * (long)b2)) >> 9;
	if (c3 < 0)
	    c3++;
	c4 = b4 + b5;
	c5 = b4 - b5;
	c6 = b7 - b6;
	c7 = b7 + b6;

	temp[y8 + 0] = c0;	/** offset 0 **/
	cA = (spi16 * c4) + (cpi16 * c7);
	cA = cA >> 9;
	if (cA < 0)
	    cA++;
	temp[y8 + 1] = cA;	/** offset 1 **/
	temp[y8 + 2] = c2;	/** offset 2 **/
	cA = (c3pi16 * c6) - (s3pi16 * c5);
	cA = cA >> 9;
	if (cA < 0)
	    cA++;
	temp[y8 + 3] = cA;	/** offset 3 **/
	temp[y8 + 4] = c1;	/** offset 4 **/
	cA = (s5pi16 * c5) + (c5pi16 * c6);
	cA = cA >> 9;
	if (cA < 0)
	    cA++;
	temp[y8 + 5] = cA;	/** offset 5 **/
	temp[y8 + 6] = c3;	/** offset 6 **/
	cA = (c7pi16 * c7) - (s7pi16 * c4);
	cA = cA >> 9;
	if (cA < 0)
	    cA++;
	temp[y8 + 7] = cA;	/** offset 7 **/
    }

    /**** now transforms columns */
    for (x = 0; x < 8; x++) {
	a0 = (temp[x] + temp[56 + x]);	/** offset 0+7 in a column **/
	a0 = a0 >> 1;
	if (a0 < 0)
	    a0++;
	a1 = (temp[x + 8] + temp[48 + x]);
	a1 = a1 >> 1;
	if (a1 < 0)
	    a1++;
	a2 = (temp[16 + x] + temp[40 + x]);
	a2 = a2 >> 1;
	if (a2 < 0)
	    a2++;
	a3 = (temp[24 + x] + temp[32 + x]);
	a3 = a3 >> 1;
	if (a3 < 0)
	    a3++;
	a4 = (temp[24 + x] - temp[32 + x]);
	a4 = a4 >> 1;
	if (a4 < 0)
	    a4++;
	a5 = (temp[16 + x] - temp[40 + x]);
	a5 = a5 >> 1;
	if (a5 < 0)
	    a5++;
	a6 = (temp[8 + x] - temp[48 + x]);
	a6 = a6 >> 1;
	if (a6 < 0)
	    a6++;
	a7 = (temp[x] - temp[56 + x]);
	a7 = a7 >> 1;
	if (a7 < 0)
	    a7++;


	b0 = a0 + a3;
	b1 = a1 + a2;
	b2 = a1 - a2;
	b3 = a0 - a3;
	b4 = a4;
	b5 = (long)(cpi4 * (long)(a6 - a5)) >> 9;
	if (b5 < 0)
	    b5++;
	b6 = (long)(cpi4 * (long)(a6 + a5)) >> 9;
	if (b6 < 0)
	    b6++;
	b7 = a7;


	c0 = (long)(cpi4 * (long)(b0 + b1)) >> 9;
	if (c0 < 0)
	    c0++;
	c1 = (long)(cpi4 * (long)(b0 - b1)) >> 9;
	if (c1 < 0)
	    c1++;
	c2 = (long)((spi8 * (long)b2) + (cpi8 * (long)b3)) >> 9;
	if (c2 < 0)
	    c2++;
	c3 = (long)((c3pi8 * (long)b3) - (s3pi8 * (long)b2)) >> 9;
	if (c3 < 0)
	    c3++;
	c4 = b4 + b5;
	c5 = b4 - b5;
	c6 = b7 - b6;
	c7 = b7 + b6;

	xform[(int)dg->xfzigzag[x] - 1] = c0;
	cA = (spi16 * c4) + (cpi16 * c7);
	cA = cA >> 9;
	if (cA < 0)
	    cA++;
	xform[(int)dg->xfzigzag[8 + x] - 1] = cA;
	xform[(int)dg->xfzigzag[16 + x] - 1] = c2;
	cA = (c3pi16 * c6) - (s3pi16 * c5);
	cA = cA >> 9;
	if (cA < 0)
	    cA++;
	xform[(int)dg->xfzigzag[24 + x] - 1] = cA;
	xform[(int)dg->xfzigzag[32 + x] - 1] = c1;
	cA = (s5pi16 * c5) + (c5pi16 * c6);
	cA = cA >> 9;
	if (cA < 0)
	    cA++;
	xform[(int)dg->xfzigzag[40 + x] - 1] = cA;
	xform[(int)dg->xfzigzag[48 + x] - 1] = c3;
	cA = (c7pi16 * c7) - (s7pi16 * c4);
	cA = cA >> 9;
	if (cA < 0)
	    cA++;
	xform[(int)dg->xfzigzag[56 + x] - 1] = cA;

    }

    {
	int            *xxptr = (int *)xform;
	for (x = 0; x < 64; x++, xxptr++)
	    *xxptr = (*xxptr + 4) / 8;
    }

}



rev_transformi(dg, blk, xform)
    gvl            *dg;
    char            blk[64];
int             xform[64];
{
    int             x, y, typ, tempi, temp[64];
    long int        xfrm[64];
    long int        a0, a1, a2, a3, a4, a5, a6, a7;
    long int        b0, b1, b2, b3, b4, b5, b6, b7;
    long int        c0, c1, c2, c3, c4, c5, c6, c7;
    long int        d1, d3, d5, d7;
    long int        cA;

    register short  qqq;

    int             spi16 = 100, spi8 = 196, s3pi16 = 284, spi4 = 362;
    int             s5pi16 = 426, s3pi8 = 473, s7pi16 = 502;
    int             cpi16 = 502, cpi8 = 473, c3pi16 = 426, cpi4 = 362;
    int             c5pi16 = 284, c3pi8 = 196, c7pi16 = 100;
    int             scale = 512;

    for (x = 0; x < 64; x++)
	xform[x] *= 8;

    /* TRANSFORM ROWS  */
    for (x = 0; x < 8; x++) {
	c0 = xform[(int)dg->xfzigzag[x] - 1];
	d1 = xform[(int)dg->xfzigzag[8 + x] - 1];
	c2 = xform[(int)dg->xfzigzag[16 + x] - 1];
	d3 = xform[(int)dg->xfzigzag[24 + x] - 1];
	c1 = xform[(int)dg->xfzigzag[32 + x] - 1];
	d5 = xform[(int)dg->xfzigzag[40 + x] - 1];
	c3 = xform[(int)dg->xfzigzag[48 + x] - 1];
	d7 = xform[(int)dg->xfzigzag[56 + x] - 1];
	/****** multiply by 10 to keep accuracy;   */

	c4 = ((spi16 * d1) - (s7pi16 * d7)) >> 9;
	if (c4 < 0)
	    c4++;
	c5 = ((s5pi16 * d5) - (s3pi16 * d3)) >> 9;
	if (c5 < 0)
	    c5++;
	c6 = ((c3pi16 * d3) + (c5pi16 * d5)) >> 9;
	if (c6 < 0)
	    c6++;
	c7 = ((cpi16 * d1) + (c7pi16 * d7)) >> 9;
	if (c7 < 0)
	    c7++;


	b0 = (long)(cpi4 * (c0 + c1)) >> 9;
	if (b0 < 0)
	    b0++;
	b1 = (long)(cpi4 * (c0 - c1)) >> 9;
	if (b1 < 0)
	    b1++;
	b2 = (long)((spi8 * c2) - (s3pi8 * c3)) >> 9;
	if (b2 < 0)
	    b2++;
	b3 = (long)((cpi8 * c2) + (c3pi8 * c3)) >> 9;
	if (b3 < 0)
	    b3++;
	b4 = c4 + c5;
	b5 = c4 - c5;
	b6 = c7 - c6;
	b7 = c7 + c6;


	a0 = b0 + b3;
	a1 = b1 + b2;
	a2 = b1 - b2;
	a3 = b0 - b3;
	a4 = b4;
	a5 = (long)(cpi4 * (b6 - b5)) >> 9;
	if (a5 < 0)
	    a5++;
	a6 = (long)(cpi4 * (b6 + b5)) >> 9;
	if (a6 < 0)
	    a6++;
	a7 = b7;

	qqq = x * 8;
	xfrm[qqq] = a0 + a7;
	xfrm[qqq + 1] = a1 + a6;
	xfrm[qqq + 2] = a2 + a5;
	xfrm[qqq + 3] = a3 + a4;
	xfrm[qqq + 4] = a3 - a4;
	xfrm[qqq + 5] = a2 - a5;
	xfrm[qqq + 6] = a1 - a6;
	xfrm[qqq + 7] = a0 - a7;
    }

    /******* now transforms columns;  */

    for (y = 0; y < 8; y++) {
	c0 = xfrm[y];
	c1 = xfrm[32 + y];
	c2 = xfrm[16 + y];
	c3 = xfrm[48 + y];
	c4 = (long)((spi16 * xfrm[8 + y]) - (s7pi16 * xfrm[56 + y])) >> 9;
	if (c4 < 0)
	    c4++;
	c5 = (long)((s5pi16 * xfrm[40 + y]) - (s3pi16 * xfrm[24 + y])) >> 9;
	if (c5 < 0)
	    c5++;
	c6 = (long)((c3pi16 * xfrm[24 + y]) + (c5pi16 * xfrm[40 + y])) >> 9;
	if (c6 < 0)
	    c6++;
	c7 = (long)((cpi16 * xfrm[8 + y]) + (c7pi16 * xfrm[56 + y])) >> 9;
	if (c7 < 0)
	    c7++;


	b0 = (long)(cpi4 * (c0 + c1)) >> 9;
	if (b0 < 0)
	    b0++;
	b1 = (long)(cpi4 * (c0 - c1)) >> 9;
	if (b1 < 0)
	    b1++;
	b2 = (long)((spi8 * c2) - (s3pi8 * c3)) >> 9;
	if (b2 < 0)
	    b2++;
	b3 = (long)((cpi8 * c2) + (c3pi8 * c3)) >> 9;
	if (b3 < 0)
	    b3++;
	b4 = c4 + c5;
	b5 = c4 - c5;
	b6 = c7 - c6;
	b7 = c7 + c6;


	a0 = b0 + b3;
	a1 = b1 + b2;
	a2 = b1 - b2;
	a3 = b0 - b3;
	a4 = b4;
	a5 = (long)(cpi4 * (b6 - b5)) >> 9;
	if (a5 < 0)
	    a5++;
	a6 = (long)(cpi4 * (b6 + b5)) >> 9;
	if (a6 < 0)
	    a6++;
	a7 = b7;

	temp[y] = a0 + a7;
	temp[8 + y] = a1 + a6;
	temp[16 + y] = a2 + a5;
	temp[24 + y] = a3 + a4;
	temp[32 + y] = a3 - a4;
	temp[40 + y] = a2 - a5;
	temp[48 + y] = a1 - a6;
	temp[56 + y] = a0 - a7;
    }

    /******  normalize;  */
    for (x = 0; x < 8; x++) {
	for (y = 0; y < 8; y++) {
	    if (temp[x * 8 + y] < 0) {
		tempi = (temp[x * 8 + y] - 16) >> 5;
		if (tempi < 0)
		    tempi++;
	    } else {
		tempi = (temp[x * 8 + y] + 16) >> 5;
		if (tempi < 0)
		    tempi++;
	    }
	    tempi = tempi + 128;
	    if (tempi < 0)
		tempi = 0;
	    if (tempi > 255)
		tempi = 255;
	    blk[y * 8 + x] = (tempi ^ 0x80);
	}
    }
}





/*
 * It doesn't appear to make any difference in speed if these are in
 * variables. 
 */
#define spi16 (100/512.0)
#define spi8 (196/512.0)
#define s3pi16 (284/512.0 )
#define spi4 (362/512.0)
#define s5pi16 (426/512.0)
#define s3pi8  (473/512.0)
#define s7pi16 (502/512.0)
#define cpi16 (502/512.0)
#define cpi8 (473/512.0)
#define c3pi16 (426/512.0 )
#define cpi4 (362/512.0)
#define c5pi16 (284/512.0)
#define c3pi8 (196/512.0)
#define c7pi16 (100/512.0)

transform(dg, blki, xform)
    gvl            *dg;
    char            blki[64];
unsigned        xform[64];
{
    int             x;
    double          tempdata[64];
    double         *temp;
    double          a0, a1, a2, a3, a4, a5, a6, a7;
    double          b0, b1, b2, b3, b4, b5, b6, b7;
    double          c0, c1, c2, c2b, c3, c3b, c4, c5, c6, c7;

    char           *blkip;

    /****** sine and cosine tables ******/

    /***
    !      TRANSFORMS EACH ROW OF BLOCK
    ***/
    temp = tempdata;
    blkip = blki;
    for (x = 0; x < 8; x++) {

	a0 = blkip[0] + blkip[7];
	a1 = blkip[1] + blkip[6];
	a2 = blkip[2] + blkip[5];
	a3 = blkip[3] + blkip[4];
	b4 /* a4 */ = blkip[3] - blkip[4];
	a5 = blkip[2] - blkip[5];
	a6 = blkip[1] - blkip[6];
	b7 /* a7 */ = blkip[0] - blkip[7];
	blkip += 8;

	b0 = a0 + a3;
	b1 = a1 + a2;
	b2 = a1 - a2;
	b3 = a0 - a3;
	/* b4 = a4; */
	b5 = cpi4 * (a6 - a5);
	b6 = cpi4 * (a6 + a5);
	/* b7 = a7; */


	c4 = b4 + b5;
	c5 = b4 - b5;
	c6 = b7 - b6;
	c7 = b7 + b6;

	*temp++ = cpi4 * (b0 + b1);	/** offset 0 **/
	*temp++ = spi16 * c4 + cpi16 * c7;	/** offset 1 **/
	*temp++ = spi8 * b2 + cpi8 * b3;	/** offset 2 **/
	*temp++ = c3pi16 * c6 - s3pi16 * c5;	/** offset 3 **/
	*temp++ = cpi4 * (b0 - b1);	/** offset 4 **/
	*temp++ = s5pi16 * c5 + c5pi16 * c6;	/** offset 5 **/
	*temp++ = c3pi8 * b3 - s3pi8 * b2;	/** offset 6 **/
	*temp++ = c7pi16 * c7 - s7pi16 * c4;	/** offset 7 **/

	/* advance to next row */
    }

    temp = tempdata;

    /**** now transforms columns */
    for (x = 0; x < 8; x++) {
	a0 = temp[x] + temp[56 + x];	/** offset 0+7 in a column **/
	a1 = temp[x + 8] + temp[48 + x];
	a2 = temp[16 + x] + temp[40 + x];
	a3 = temp[24 + x] + temp[32 + x];
	b4 /* a4 */ = temp[24 + x] - temp[32 + x];
	a5 = temp[16 + x] - temp[40 + x];
	a6 = temp[8 + x] - temp[48 + x];
	b7 /* a7 */ = temp[x] - temp[56 + x];

	b0 = a0 + a3;
	b1 = a1 + a2;
	b2 = a1 - a2;
	b3 = a0 - a3;
	/* b4 = a4; */
	b5 = cpi4 * (a6 - a5);
	b6 = cpi4 * (a6 + a5);
	/* b7 = a7 ; */

	c4 = b4 + b5;
	c5 = b4 - b5;
	c6 = b7 - b6;
	c7 = b7 + b6;

	/*
	 * Instead of the /4 there used to be a * 2 after the round. I
	 * changed this because I found that ftrans_a_block would process all
	 * the numbers out of this function with this loop for (i=0;i<64;i++,
	 * xxptr++) *xxptr = (*xxptr+4)/8; 
	 *
	 * It will be more accurate and faster to just divide by 4 here. 
	 */

	xform[(int)dg->xfzigzag[x] - 1] = R((cpi4 * (b0 + b1)) *.25);
	xform[(int)dg->xfzigzag[8 + x] - 1] = R((spi16 * c4 + cpi16 * c7) *.25);
	xform[(int)dg->xfzigzag[16 + x] - 1] = R((spi8 * b2 + cpi8 * b3) *.25);
	xform[(int)dg->xfzigzag[24 + x] - 1] = R((c3pi16 * c6 - s3pi16 * c5) *.25);
	xform[(int)dg->xfzigzag[32 + x] - 1] = R((cpi4 * (b0 - b1)) *.25);
	xform[(int)dg->xfzigzag[40 + x] - 1] = R((s5pi16 * c5 + c5pi16 * c6) *.25);
	xform[(int)dg->xfzigzag[48 + x] - 1] = R((c3pi8 * b3 - s3pi8 * b2) *.25);
	xform[(int)dg->xfzigzag[56 + x] - 1] = R((c7pi16 * c7 - s7pi16 * c4) *.25);
    }
}

/*
 * Make a table to truncate the range to a signed char. 
 */
static int      range_init;
static char     range_data[520];
static char    *range_check;
dorange_init()
{
    int             i, j;
    range_init = 1;
    range_check = &range_data[260];
    for (i = -255; i < 256; i++) {
	j = i + 128;
	if (j < 0)
	    j = 0;
	if (j > 255)
	    j = 255;
	range_check[i] = (j ^ 0x80);
    }
}

rev_transform(dg, blk, xform)
    gvl            *dg;
    char            blk[64];
int             xform[64];
{
    int             x, tempi;
    double          temp[64];
    double          xfrm[64], *xfrmp;
    double          a0, a1, a2, a3, a4, a5, a6, a7;
    double          b0, b1, b2, b3, b4, b5, b6, b7;
    double          c0, c1, c2, c3, c4, c5, c6, c7;
    double          d1, d3, d5, d7;
    char           *blkp;

    if (!range_init)
	dorange_init();

    /* TRANSFORM ROWS  */
    xfrmp = xfrm;
    for (x = 0; x < 8; x++) {
	c0 = xform[(int)dg->xfzigzag[x] - 1];
	d1 = xform[(int)dg->xfzigzag[8 + x] - 1];
	c2 = xform[(int)dg->xfzigzag[16 + x] - 1];
	d3 = xform[(int)dg->xfzigzag[24 + x] - 1];
	c1 = xform[(int)dg->xfzigzag[32 + x] - 1];
	d5 = xform[(int)dg->xfzigzag[40 + x] - 1];
	c3 = xform[(int)dg->xfzigzag[48 + x] - 1];
	d7 = xform[(int)dg->xfzigzag[56 + x] - 1];

	c4 = spi16 * d1 - s7pi16 * d7;
	c5 = s5pi16 * d5 - s3pi16 * d3;
	c6 = c3pi16 * d3 + c5pi16 * d5;
	c7 = cpi16 * d1 + c7pi16 * d7;


	b0 = cpi4 * (c0 + c1);
	b1 = cpi4 * (c0 - c1);
	b2 = spi8 * c2 - s3pi8 * c3;
	b3 = cpi8 * c2 + c3pi8 * c3;
	b4 = c4 + c5;
	b5 = c4 - c5;
	b6 = c7 - c6;
	b7 = c7 + c6;


	a0 = b0 + b3;
	a1 = b1 + b2;
	a2 = b1 - b2;
	a3 = b0 - b3;
	a4 = b4;
	a5 = cpi4 * (b6 - b5);
	a6 = cpi4 * (b6 + b5);
	a7 = b7;

	*xfrmp++ = a0 + a7;
	*xfrmp++ = a1 + a6;
	*xfrmp++ = a2 + a5;
	*xfrmp++ = a3 + a4;
	*xfrmp++ = a3 - a4;
	*xfrmp++ = a2 - a5;
	*xfrmp++ = a1 - a6;
	*xfrmp++ = a0 - a7;
    }

    /******* now transforms columns;  */
    xfrmp = xfrm;
    blkp = blk;
    for (x = 0; x < 8; x++) {
	c0 = xfrmp[x];
	c1 = xfrmp[32 + x];
	c2 = xfrmp[16 + x];
	c3 = xfrmp[48 + x];
	c4 = spi16 * xfrmp[8 + x] - s7pi16 * xfrmp[56 + x];
	c5 = s5pi16 * xfrmp[40 + x] - s3pi16 * xfrmp[24 + x];
	c6 = c3pi16 * xfrmp[24 + x] + c5pi16 * xfrmp[40 + x];
	c7 = cpi16 * xfrmp[8 + x] + c7pi16 * xfrmp[56 + x];


	b0 = cpi4 * (c0 + c1);
	b1 = cpi4 * (c0 - c1);
	b2 = spi8 * c2 - s3pi8 * c3;
	b3 = cpi8 * c2 + c3pi8 * c3;
	a4 /* b4 */ = c4 + c5;
	b5 = c4 - c5;
	b6 = c7 - c6;
	a7 /* b7 */ = c7 + c6;


	a0 = b0 + b3;
	a1 = b1 + b2;
	a2 = b1 - b2;
	a3 = b0 - b3;
	/* a4 = b4; */
	a5 = cpi4 * (b6 - b5);
	a6 = cpi4 * (b6 + b5);
	/* a7 = b7; */

	*blkp++ = range_check[R((a0 + a7) *.25)];
	*blkp++ = range_check[R((a1 + a6) *.25)];
	*blkp++ = range_check[R((a2 + a5) *.25)];
	*blkp++ = range_check[R((a3 + a4) *.25)];
	*blkp++ = range_check[R((a3 - a4) *.25)];
	*blkp++ = range_check[R((a2 - a5) *.25)];
	*blkp++ = range_check[R((a1 - a6) *.25)];
	*blkp++ = range_check[R((a0 - a7) *.25)];
    }


#ifdef notdef
    for (x = 0; x < 64; x++) {
	printf("float x %d %d\n", x, blk[x]);
    }
    rev_transformi(dg, blk, xform);
    for (x = 0; x < 64; x++) {
	printf("int x %d %d\n", x, blk[x]);
    }
    exit(0);
#endif
}
