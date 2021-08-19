
/*********************************************************************
This is valuable proprietary information and trade secrets of Creative Circuits
Corporation and is not to be disclosed or released without the prior written
permission of an officer of the company.
  
Copyright (c) 1989 by Creative Circuits Corporation.
  
			blk_cdrs.C
*********************************************************************/


#include "gStruct.h"



/***                                                           ***/
/*** Sequentially  code a block at a time       		       ***/
/*** The coder     - Does not introduce a loss of information  ***/
/***                                                           ***/

n8r5_coder(dg, ptr, prev_dc, quantizer, code_tbl)
    gvl            *dg;
    int            *ptr, *prev_dc;
    unsigned int   *quantizer;
    code_table_struct *code_tbl;
{
    int             visi, tempi, mask;
    int             zero_rl, word, delta, index;
    unsigned        i, nb, delta_sign;
    int             ii;		/* 6/29/89 */

    /** do dc firstus **/

    tempi = *ptr++;		/* get transform DC  coef */


    /** compute the delta =  current - predicted	**/
    delta = tempi - *prev_dc;

    *prev_dc = tempi;		/* save for prediction in next block */

    /** Code magnitude category & if nonzero output value the bitstream  **/

    nb = magnitude_category_of(delta);
    sputv(dg, (*code_tbl).dc_c[nb], (*code_tbl).dc_n[nb]);

    if (delta < 0)
	delta--;
    if (nb)
	sputv(dg, delta, nb);

    zero_rl = 0;		/* start with no 0's */

    /***  loop until we have examined all 63 ac coeff's  ***/

    for (i = 1; i < 64; i++) {
	word = *ptr++;


	/** on a run of zero's ?  **/
	if (!word) {
	    zero_rl++;
	    continue;
	}
	/** coeff is nonzero **/
	while (zero_rl > 15) {	/* if run larger than 15 *//* special code */
	    sputv(dg, (*code_tbl).ac_c[15 * 16], (*code_tbl).ac_n[15 * 16]);
	    zero_rl -= 16;	/* and subtract 16 from run; */
	}

	/** compute the number of bits required to describe our coeff **/
	nb = magnitude_category_of(word);

	/** use number of bits & run length  to reach into 1 dimensional  **/
	/** table to get the huffman code.  Forget not his scorecard      **/

	index = 16 * zero_rl + nb;
	sputv(dg, (*code_tbl).ac_c[index], (*code_tbl).ac_n[index]);


	if (word < 0)
	    word--;		/* and subtract 1 if negative; */

	sputv(dg, word, nb);

	/** reset the the run length counter **/
	zero_rl = 0;

    } /* end i  loop on ac coeffs */

    /** code an end of block (EOB),  if necessary **/

    if (zero_rl) {
	sputv(dg, (*code_tbl).ac_c[0], (*code_tbl).ac_n[0]);

    }
}



/*********************************************************
set block to zeros
decode non zero transform coefficients
*********************************************************/

n8r5_decoder(dg, blk_bfr, prev_dc, quantizer, huf_struct)
    gvl            *dg;
    int            *blk_bfr, *prev_dc;
    unsigned int   *quantizer;
    hds             huf_struct[];
{
    unsigned int    tempi;
    int             nbits, index;
    int             ac_dist, word, delta;
    unsigned        i, nb, delta_sign;

    /** do dc firstus **/

    delta = 0;
    nbits = sgetcode(dg, &huf_struct[0]);
    if (nbits > 0) {
	delta = sgetv(dg, nbits);
	if (!(delta & dg->bittest[nbits])) {
	    delta |= dg->signmask[nbits];
	    delta++;
	}
    }
    *prev_dc = *prev_dc + delta;

    bzero(blk_bfr, sizeof(int) * 64);	/* zero out the ac coeff */
    blk_bfr[0] = *prev_dc;	/* dc is put in place */

    index = 1;
    while (1) {
	tempi = sgetcode(dg, &huf_struct[1]);
	ac_dist = tempi / 16;
	nbits = tempi % 16;

	if (!nbits && ac_dist) {
	    index += 16;
	    continue;
	}
	if (!nbits && !ac_dist)
	    break;

	if (nbits > 0) {
	    word = sgetv(dg, nbits);
	    if (!(word & dg->bittest[nbits])) {
		word |= dg->signmask[nbits];
		word++;
	    }
	    index += ac_dist;
	    blk_bfr[index++] = word;
#ifdef NeXT
	    if (index == 64)
		break;
#endif
	}
    } /* end of the while (1) */

}

magnitude_category_of(coef)
    int             coef;
{
    if (!coef)
	return (0);

    if (coef < 0) {
	if (coef == -32768)
	    return (16);
	coef = -coef;
    }
    if (coef <= 1)
	return (1);
    if (coef <= 3)
	return (2);
    if (coef <= 7)
	return (3);
    if (coef <= 15)
	return (4);
    if (coef <= 31)
	return (5);
    if (coef <= 63)
	return (6);
    if (coef <= 127)
	return (7);
    if (coef <= 255)
	return (8);
    if (coef <= 511)
	return (9);
    if (coef <= 1023)
	return (10);
    if (coef <= 2047)
	return (11);
    if (coef <= 4095)
	return (12);
    if (coef <= 8191)
	return (13);
    if (coef <= 16383)
	return (14);
    if (coef <= 32767)
	return (15);
    return (16);
}



/*******
prints coder ouput in comma form
 pos -5  SYNC
 pos -4  BOB
 pos -3  EOB
 pos -2  ESC
 pos -1  DC
 pos >0  position of ac coefficient
*******/
cma_prnt(dg, pos, hc_bits, hc_size, mag, mag_size, value)
    gvl            *dg;
    int             pos, hc_bits, hc_size, mag, mag_size, value;
{
    int             i, nc, pad;
    register unsigned int bits, test;

    bits = hc_bits;
    nc = hc_size;
    if (pos >= 0) {		/**  AC  **/

	nc = hc_size + 3 + mag_size;
	test = 1 << (hc_size - 1);
	for (i = 0; i < hc_size; i++) {
	    fprintf(dg->cma_fp, "%1s", (bits & test) ? "1" : "0");
	    test /= 2;
	}
	fprintf(dg->cma_fp, " , ");

	bits = mag;
	test = 1 << (mag_size - 1);
	for (i = 0; i < mag_size; i++) {
	    fprintf(dg->cma_fp, "%1s", (bits & test) ? "1" : "0");
	    test /= 2;
	}
	pad = 30 - nc;
	for (i = 0; i < pad; i++)
	    fprintf(dg->cma_fp, " ");
	fprintf(dg->cma_fp, "; AC%02d    %03X", pos, value & 0xfff);
	cma_cdr_word(dg, 1);
	return;
    }
    switch (pos) {
    case -1:			/**  DC  **/
	nc = hc_size + 3 + mag_size;
	test = 1 << (hc_size - 1);
	for (i = 0; i < hc_size; i++) {
	    fprintf(dg->cma_fp, "%1s", (bits & test) ? "1" : "0");
	    test /= 2;
	}
	fprintf(dg->cma_fp, " , ");
	bits = mag;
	test = 1 << (mag_size - 1);
	for (i = 0; i < mag_size; i++) {
	    fprintf(dg->cma_fp, "%1s", (bits & test) ? "1" : "0");
	    test /= 2;
	}

	pad = 30 - nc;
	for (i = 0; i < pad; i++)
	    fprintf(dg->cma_fp, " ");
	if (mag < 0)
	    mag++;
	fprintf(dg->cma_fp, "; DC      %03X", value & 0xfff);
	cma_cdr_word(dg, 1);
	break;
    case -2:			/** ESC  **/
	test = 1 << (hc_size - 1);
	for (i = 0; i < hc_size; i++) {
	    fprintf(dg->cma_fp, "%1s", (bits & test) ? "1" : "0");
	    test /= 2;
	}
	pad = 30 - nc;
	for (i = 0; i < pad; i++)
	    fprintf(dg->cma_fp, " ");
	fprintf(dg->cma_fp, "; RL ESC");
	cma_cdr_word(dg, 6);
	break;
    case -3:			/**  EOB  **/
	test = 1 << (hc_size - 1);
	for (i = 0; i < hc_size; i++) {
	    fprintf(dg->cma_fp, "%1s", (bits & test) ? "1" : "0");
	    test /= 2;
	}
	pad = 30 - nc;
	for (i = 0; i < pad; i++)
	    fprintf(dg->cma_fp, " ");
	fprintf(dg->cma_fp, "; EOB");
	cma_cdr_word(dg, 9);	/* 7 = 11 - strnlen(" EOB") */
	break;
    case -4:			/**  BOB  **/
	pad = 30 - nc;
	for (i = 0; i < pad; i++)
	    fprintf(dg->cma_fp, " ");
	fprintf(dg->cma_fp, ";BLK%04d\n", dg->cdr_nblks++);
	break;
    case -5:			/**  SYNC  **/
	test = 1 << (hc_size - 1);
	for (i = 0; i < hc_size; i++) {
	    fprintf(dg->cma_fp, "%1s", (bits & test) ? "1" : "0");
	    test /= 2;
	}
	pad = 30 - nc;
	for (i = 0; i < pad; i++)
	    fprintf(dg->cma_fp, " ");
	fprintf(dg->cma_fp, "; SYNC");
	cma_cdr_word(dg, 6);	/* 6 = 11 - strnlen(" SYNC") */
	break;
    } /** end of switch **/
}

cma_cdr_word(dg, NumberOfSpaces)
    gvl            *dg;
    int             NumberOfSpaces;
{
    int             i;
    for (i = 0; i < NumberOfSpaces; i++)
	fprintf(dg->cma_fp, " ");
    fprintf(dg->cma_fp, "%5d  %02X %02X\n", dg->cdr_nwords, dg->cdr_prev_word & 0XFF, *dg->cdr_ptr & 0XFF);
}
