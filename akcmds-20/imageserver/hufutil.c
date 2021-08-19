/*--------------------------------------------------------------------------
  
This is valuable proprietary information and trade secrets of Creative Circuits
Corporation and is not to be disclosed or released without the prior written
permission of an officer of the company.
  
Copyright (c) 1989 by Creative Circuits Corporation.
  
   File Name : hufutil.c
	Date      : 5/30/1989
	Subject   : Huffman code generation (JPEG).
  
Modification history
Date		Name	Summary
6/16/89	Glenn	debugging info fprinted optionally
6/16/89	JD		1. Merge PC into MAC
					2. bs_bfr 256 to 4096
					3. getbit code in decode_huffman_code()
6/29/89	JD		1. HW coder sim output for d troops (cdr_fp is flag & file pointer 
        			2. Moved putbits init to init_putbits (needed another parameter)
  
7/6/89  VAHID 1. added allocation for bs_bfr to init_putbits
--------------------------------------------------------------------------*/

/*
 * #include <stdlib.h> 
 */

#include "gStruct.h"

#define    maximum_frequency_value  0x7fffffffl

static int      sgetbit();

/****************************** JPEG 8-R5 ************************************
  
sgetcode( dg  , h_d_s )
gvl	*dg;
hds	*h_d_s;
  {
    int           s, p, value, next_bit;
    int           temp1,temp2;
  
	 s = 1;
	 dg->hufcode = sgetbit( dg  );
	 
	 while ( ((int)dg->hufcode) > ((int)(*h_d_s).maxcode[s]) )
	   {
		  s++;
		  dg->hufcode = (dg->hufcode  <<1) | sgetbit( dg  );
		}  
	 
	 if ( dg->hufcode < (*h_d_s).biggest )
	    {
		 	p = (*h_d_s).valptr[s];
			p = p + dg->hufcode - (*h_d_s).mincode[s];
			value = (*h_d_s).huffval[p];
		  }
	 else
	   	{
	   		printf("\nError detected in bitstream");
	   		exit(1);
	   	}
   
	 return( value );
  }     /* end of function sgetcode */


/********************* Minimal Memory Decoder (1 bit) *************************
  
sgetcode( dg  , h_d_s )
gvl	*dg;
hds	*h_d_s;
  {
	unsigned char i, k, count[17];
	
	count[0] = 0;
	for ( i=0; i<16; i++ )
		count[i+1] = count[i] + (*h_d_s).huffbits[i];
	
	
	k = 0;
	
	for ( i=0; i<16; i++ )
	{
		if ( k < count[i] )
			break;
		else
			k = 2*k + sgetbit( dg   ) - count[i];
	}
	
	return( (*h_d_s).huffval[k] ); 
 }     /* end of function sgetcode */


/********************* Minimal Memory Decoder (2 bits) *************************
  
sgetcode( dg  , h_d_s )
gvl	*dg;
hds	*h_d_s;
  {
	int i, k, count[17];
	
	count[0] = 0;
	for ( i=0; i<16; i++ )
		count[i+1] = count[i] + (*h_d_s).huffbits[i];
	
	
	k = 0;
	
	for ( i=0; i<16; i+=2 )
	{
  
		k = 4*k + 2*sgetbit(dg)+slookbit(dg) - 4*count[i?(i-1):i]-2*count[i];
  
		if ( k/2 < count[i+1] )
		{
			k = k/2;
			break;
		}
		else
			sgetbit(dg);
  
		if ( k < count[i+1]+count[i+2] )
		{
			k -= count[i+1];
			break;
		}
	} 
	
	return( (*h_d_s).huffval[k] ); 
 }     /* end of function sgetcode */


/********************* Minimal Memory Decoder (2 bits) *************************/

sgetcode(dg, h_d_s)
    gvl            *dg;
    hds            *h_d_s;
{
    unsigned char   i, k, kk, count[17];

    count[0] = 0;
    for (i = 0; i < 16; i++)
	count[i + 1] = count[i] + (*h_d_s).huffbits[i];


    k = 0;
    kk = slookbit(dg);

    for (i = 0; i < 16; i += 2) {

	if (k < count[i])
	    break;
	sgetbit(dg);
	k = 2 * kk + slookbit(dg) - count[i + 1];
	if (kk < count[i + 1]) {
	    k = kk;
	    break;
	}
	sgetbit(dg);
	kk = 2 * k + slookbit(dg) - count[i + 2];
    }

    return ((*h_d_s).huffval[k]);
} /* end of function sgetcode */


/********************* Minimal Memory Encoder *************************/

sputcode(dg, data, bits, nnnnssss)
    gvl            *dg;
    unsigned char   data[], bits[];
int             nnnnssss;
{
    unsigned char   k, count[16], index[256];
    unsigned int    i, j, c, code[16];

    count[0] = bits[0];
    for (i = 1; i < 16; i++)
	count[i] = count[i - 1] + bits[i];

    for (i = 0; i < count[15]; i++)
	index[data[i]] = i;

    c = 0;
    for (i = 0; i < 16; i++) {
	c += bits[i];
	code[i] = c;
	c <<= 1;
    }


    k = index[nnnnssss];

    for (i = 0; i < 16; i++)
	if (k < count[i])
	    break;

    /*
     * c = 0; for ( j=0; j<i+1; j++ ) if
     * ((code[i]+k-count[i])&dg->bittest[j+1]) c |= dg->bittest[i+1-j]; 
     */

    sputv(dg, c, i + 1);

} /* end of function sputcode */


#if 0
/***********************************************************************
!                                                                      *
! This char function receives a specified number of bits.              *
! A  -2  value of bits will cause the transmission buffer to be        *
! initialized.                                                         *
!                                                                      *
!**********************************************************************/
int 
getbits(dg, bits)
    gvl            *dg;
    int             bits;
{
    int             i, work = 0;

    if (!bits) {
	printf("\n GETBITS ... bits=0?");
	return;
    }
    if (bits == -2) {
    }
    for (i = 0; i < bits; i++) {
	work = (work << 1) | (*dg->bs_ptr < 0);
	*dg->bs_ptr = *dg->bs_ptr << 1;
	if (--dg->unused <= 0) {
	    dg->unused = 8;
	    dg->bs_nbytes++;
	    if (++dg->bs_ptr == dg->bs_bfr_end) {
		(*dg->refresh_fun) (dg);
		dg->bs_ptr = dg->bs_bfr;
	    }
	}
    }
    return (work);
}
#endif

/****************************** JPEG 8-R5 ************************************/

int 
sgetv(dg, bits)
    gvl            *dg;
    int             bits;
{
    int             i, work = 0;

    if (!bits) {
	printf("\n GETBITS ... bits=0?");
	return;
    }
    if (bits == -2) {
	printf("\nOBSOLETE INITIALIZATION OF GETBITS....ERROR\n");
    }
    for (i = 0; i < bits; i++) {
	/* work = work | (sgetbit(dg)<<i);	 */
	work = (work << 1) | sgetbit(dg);
    }
    return (work);
}

#if 0

/***********************************************************************
!                                                                      *
! This function recieves one bit.                                      *
!                                                                      *
!**********************************************************************/
unsigned int 
getbit(dg)
    gvl            *dg;
{
    unsigned int    hold;

    hold = *dg->bs_ptr < 0;
    *dg->bs_ptr = *dg->bs_ptr << 1;
    if (--dg->unused <= 0) {
	dg->unused = 8;
	dg->bs_nbytes++;
	if (++dg->bs_ptr == dg->bs_bfr_end) {
	    (*dg->refresh_fun) (dg);
	    dg->bs_ptr = dg->bs_bfr;
	}
    }
    return (hold);
}
#endif
/****************************** JPEG 8-R5 ************************************/
static          inline
int 
sgetbit(dg)
    gvl            *dg;
{
    unsigned int    hold, marker;

    /* hold = *dg->bs_ptr & dg->bittest[9-dg->unused];   */
    hold = *dg->bs_ptr & dg->bittest[dg->unused];
    if (--dg->unused <= 0) {
	dg->unused = 8;
	dg->bs_nbytes++;
	if (++dg->bs_ptr == dg->bs_bfr_end) {
	    (*dg->refresh_fun) (dg);
	    dg->bs_ptr = dg->bs_bfr;
	}
	if (dg->check_marker) {
	    if (dg->marker_detected) {
		do {
		    marker = *dg->bs_ptr & 255;
		    if (++dg->bs_ptr == dg->bs_bfr_end) {
			(*dg->refresh_fun) (dg);
			dg->bs_ptr = dg->bs_bfr;
		    }
		}
		while (marker == FIL_MARKER);
		dg->marker_detected = 0;
		if (marker)
		    return (-marker);
	    }
	    if (*dg->bs_ptr == -1)
		dg->marker_detected = 1;
	    else
		dg->marker_detected = 0;
	}
    }
    return (hold ? 1 : 0);
}

/****************************** JPEG 8-R5 ************************************/

int 
slookbit(dg)
    gvl            *dg;
{
    unsigned int    hold;

    /* hold = *dg->bs_ptr & dg->bittest[9-dg->unused];   */
    hold = *dg->bs_ptr & dg->bittest[dg->unused];

    return (hold ? 1 : 0);
}


/****************************** JPEG 8-R5 ************************************/


handle_marker_code(dg)
    gvl            *dg;
{
    int             save, marker;

    save = dg->check_marker;
    dg->check_marker = 0;

    if ((marker = *dg->bs_ptr & 0xff) == FIL_MARKER)
	while ((marker = sgetv(dg, dg->unused) & 0xff) == FIL_MARKER);

    switch (marker) {
    case SOS_MARKER:
    case NLO_MARKER:
    case SOI_MARKER:
    case EOI_MARKER:
    case COM_MARKER:
	break;
    default:
	if (marker & 7 == RST_MARKER);
	else if (marker & 15 == APP_MARKER);
	else if (marker & 15 == SOF_MARKER);
	break;
    }

    dg->check_marker = save;
}

sgetmarker(dg)
    gvl            *dg;
{
    int             save, marker;

    save = dg->check_marker;
    dg->check_marker = 0;

    while ((*dg->bs_ptr & 0xff) != 0xff)
	/* while( *dg->bs_ptr&0xff != 0xff )	 */
	sgetv(dg, dg->unused);

    while ((marker = sgetv(dg, dg->unused) & 0xff) == FIL_MARKER);

    dg->check_marker = save;
    return (marker);
}

#if 0
/******
initialize getbits
******/

init_getbit(dg, bfr, bfr_size, refresh)
    gvl            *dg;
    char           *bfr;
    int             bfr_size;
    void           *refresh;
{
    dg->refresh_fun = (void *)refresh;
    dg->bs_bfr = bfr;
    dg->unused = 8;
    dg->bs_ptr = dg->bs_bfr;
    dg->bs_bfr_end = dg->bs_ptr + bfr_size;
}
#endif

/****************************** JPEG 8-R5 ************************************/

init_sgetv(dg, bfr, bfr_size, refresh)
    gvl            *dg;
    char           *bfr;
    int             bfr_size;
    void           *refresh;
{
    dg->refresh_fun = (void *)refresh;
    dg->bs_bfr = bfr;
    dg->unused = 8;
    dg->bs_ptr = dg->bs_bfr;
    dg->bs_bfr_end = dg->bs_ptr + bfr_size;

    dg->marker_detected = 0;
    dg->check_marker = 0;
}


#if 0
/***********************************************************************
!                                                                      *
! This subroutine is used to initialize putbits & the bitstream        *
!                                6/29/89                               *
!**********************************************************************/
init_putbits(dg, dcr_fp_val, cdr_fp_val, bs_fh_val)
    gvl            *dg;
    FILE           *dcr_fp_val, *cdr_fp_val;
    int             bs_fh_val;
{

    dg->bs_bfr = (char *)malloc(4100L);	/** 7/6/89 **/

    dg->hufbs_fh = bs_fh_val;
    dg->cdr_fp = cdr_fp_val;
    dg->bs_ptr = dg->bs_bfr;
    *dg->bs_ptr = 0;
    dg->bs_bfr_end = dg->bs_ptr + 4096;
    dg->bs_nbytes = 0;
    dg->unused = 8;

    dg->dcr_fp = dcr_fp_val;
    dg->cdr_fp = cdr_fp_val;
    dg->cdr_ptr = dg->cdr_bfr;
    *dg->cdr_ptr = 0;
    dg->cdr_bfr_end = dg->cdr_ptr + 512;
    dg->cdr_nbytes = 0;
    dg->cdr_prev_word = 0;
    dg->cdr_cur_word = 0;
    dg->cdr_nwords = -1;
    dg->cdr_unused = 8;
    dg->cdr_nblks = 0;
}

#endif

/****************************** JPEG 8-R5 ************************************/

init_sputv(dg, dcr_fp_val, cdr_fp_val, bs_fh_val)
    gvl            *dg;
    FILE           *dcr_fp_val, *cdr_fp_val;
    int             bs_fh_val;
{
    int             i;

    dg->bs_bfr = (char *)malloc(4100L);	/** 7/6/89 **/

    dg->hufbs_fh = bs_fh_val;
    dg->cdr_fp = cdr_fp_val;
    dg->bs_ptr = dg->bs_bfr;
    *dg->bs_ptr = 0;
    dg->bs_bfr_end = dg->bs_ptr + 4096;
    dg->bs_nbytes = 0;
    dg->unused = 8;

    dg->dcr_fp = dcr_fp_val;
    dg->cdr_fp = cdr_fp_val;
    dg->cdr_ptr = dg->cdr_bfr;
    *dg->cdr_ptr = 0;
    dg->cdr_bfr_end = dg->cdr_ptr + 512;
    dg->cdr_nbytes = 0;
    dg->cdr_prev_word = 0;
    dg->cdr_cur_word = 0;
    dg->cdr_nwords = -1;
    dg->cdr_unused = 8;
    dg->cdr_nblks = 0;

    if (dg->cma_fp != 0) {
	fprintf(dg->cma_fp, "%s", "\n                                                ");
	fprintf(dg->cma_fp, "%s", "Index of last complete byte");
	fprintf(dg->cma_fp, "%s", "\nHuffman code (root to left)                     ");
	fprintf(dg->cma_fp, "%s", "|  Value of last complete byte");
	fprintf(dg->cma_fp, "%s", "\n|          Magnitude data                       ");
	fprintf(dg->cma_fp, "%s", "|  |  Current incomplete byte");
	fprintf(dg->cma_fp, "%s", "\n|          |                                    ");
	fprintf(dg->cma_fp, "%s", "|  |  |");
	fprintf(dg->cma_fp, "%s", "\n");
    }
    dg->check_marker = 0;

    dg->lastCompressedFifoEntryCount = 0;
    dg->lastRawFifoEntryCount = 0;
    dg->fifoEntryCount = 0;

    for (i = 0; i < 17; i++)
	dg->codeLengthHisto[i] = 0;
    for (i = 0; i < 32; i++)
	dg->compressedFifoHisto[i] = 0;
    for (i = 0; i < 512; i++)
	dg->rawFifoHisto[i] = 0;

}

#if 0

/***********************************************************************
!                                                                      *
! This subroutine is used to enter a VAL represented in NBITS bits     *
! into the transmission buffer.                                        *
! The first time the routine is called the packed file is opened.(BS)  *
! A  -2  value of CNT will cause the transmission buffer to be         *
! initialized.                                                         *
! A   -1     value of CNT will cause the transmission buffer to be     *
! emptied (written to the packed file).                                *
!                                                                      *
!**********************************************************************/
putbits(dg, val, nbits)
    gvl            *dg;
    unsigned int    val, nbits;
{
    int             bs_delta;
    int             count;

    if (!nbits) {
	printf("\n PUTBITS ... nbits=0?, val%d", val);
	return;
    }
    if (nbits == -2) {
	printf("\nOutdated mode of putbits init, nbits=-2");	/* 6/29/89 */
	exit(1);
    }
    if (nbits == -1) {
	bs_delta = (dg->bs_ptr - dg->bs_bfr + 1);
	dg->bs_nbytes += (bs_delta - 1);
#ifdef NeXT
	jwrite(dg->hufbs_fh, dg->bs_bfr, bs_delta);
#else
	write(dg->hufbs_fh, dg->bs_bfr, bs_delta);
	close(dg->hufbs_fh);
#endif
	putcdr(dg, 0, -1);	/* wrapup coder_out  6/29/89 */
	return;
    }
    count = dg->bs_ptr - dg->bs_bfr;

    val = val & dg->wmask[nbits];
    while (nbits) {
	dg->unused = nbits - dg->unused;
	if (dg->unused >= 0) {
	    *dg->bs_ptr++ |= val >> dg->unused;
	    *dg->bs_ptr = 0;
	    nbits = dg->unused;
	    dg->unused = 8;
	} else {
	    dg->unused = -dg->unused;
	    *dg->bs_ptr |= val << dg->unused;
	    nbits = 0;
	}
    }

    if (dg->bs_ptr >= dg->bs_bfr_end) {
#ifdef NeXT
	jwrite(dg->hufbs_fh, dg->bs_bfr, 4096);
#else
	write(dg->hufbs_fh, dg->bs_bfr, 4096);
#endif
	dg->bs_ptr -= 4096;
	dg->bs_nbytes += 4096;
	dg->bs_bfr[0] = dg->bs_bfr[4096];
	dg->bs_bfr[1] = dg->bs_bfr[4097];
	dg->bs_bfr[2] = dg->bs_bfr[4098];
    }
}
#endif

/****************************** JPEG 8-R5 ************************************/

inline
sputv(dg, val, nbits)
    gvl            *dg;
    unsigned        val;
    int             nbits;
{
    int             bs_delta;
    int             count;

    if (nbits <= 0) {
	if (!nbits) {
	    printf("\n PUTBITS ... nbits=0?, val%d", val);
	    return;
	}
	if (nbits == -2) {
	    printf("\nOutdated mode of putbits init, nbits=-2");	/* 6/29/89 */
	    exit(1);
	}
	if (nbits == -1) {
	    bs_delta = (dg->bs_ptr - dg->bs_bfr);
	    if (dg->unused != 8)
		bs_delta++;
	    dg->bs_nbytes += (bs_delta - 1);
#ifdef NeXT
	    jwrite(dg->hufbs_fh, dg->bs_bfr, bs_delta);
#else
	    write(dg->hufbs_fh, dg->bs_bfr, bs_delta);
	    close(dg->hufbs_fh);
#endif
	    putcdr(dg, 0, -1);	/* wrapup coder_out  6/29/89 */
	    return;
	}
    }
    count = dg->bs_ptr - dg->bs_bfr;

    while (nbits) {
	val = val & dg->wmask[nbits];
	if (dg->unused - nbits <= 0) {
	    *dg->bs_ptr++ |= val >> (nbits - dg->unused);
	    *dg->bs_ptr = 0;
	    if (dg->check_marker && *(dg->bs_ptr - 1) == -1) {
		dg->bs_ptr++;
		*dg->bs_ptr = 0;
	    }
	    nbits -= dg->unused;
	    dg->unused = 8;
	} else {
	    *dg->bs_ptr |= val << (dg->unused - nbits);
	    dg->unused -= nbits;
	    nbits = 0;
	}
    }

    if (dg->bs_ptr >= dg->bs_bfr_end) {
#ifdef NeXT
	jwrite(dg->hufbs_fh, dg->bs_bfr, 4096);
#else
	write(dg->hufbs_fh, dg->bs_bfr, 4096);
#endif
	dg->bs_ptr -= 4096;
	dg->bs_nbytes += 4096;
	dg->bs_bfr[0] = dg->bs_bfr[4096];
	dg->bs_bfr[1] = dg->bs_bfr[4097];
	dg->bs_bfr[2] = dg->bs_bfr[4098];
    }
}

/*
 * Return current byte offset in output stream.
 */
int bytelocation(dg)
gvl            *dg;
{
    return(dg->bs_nbytes + (dg->bs_ptr - dg->bs_bfr));
}


sputmarker(dg, marker_code)
    gvl            *dg;
    int             marker_code;
{
    int             save;

    if (dg->unused < 8)
	sputv(dg, 0xff, dg->unused);

    save = dg->check_marker;
    dg->check_marker = 0;

    sputv(dg, 0xff, 8);
    sputv(dg, marker_code, 8);

    dg->check_marker = save;

}




allocate_hufutil_variables(dg)
    gvl            *dg;
{
    dg->bits = (int *)malloc((long)sizeof(int) * (CODE_TBL_SIZE + 1));
    dg->codesize = (int *)malloc((long)sizeof(int) * (CODE_TBL_SIZE + 1));
    dg->others = (int *)malloc((long)sizeof(int) * (CODE_TBL_SIZE + 1));
    dg->freq = (long int *)malloc((long)sizeof(long) * (CODE_TBL_SIZE + 1));
    dg->freq_copy = (long int *)malloc((long)sizeof(long) * (CODE_TBL_SIZE + 1));
    dg->huffsize = (int *)malloc((long)sizeof(int) * (CODE_TBL_SIZE + 1));
    dg->huffcode = (int *)malloc((long)sizeof(int) * (CODE_TBL_SIZE + 1));
    dg->hufco = (int *)malloc((long)sizeof(int) * (CODE_TBL_SIZE + 1));
    dg->hufsi = (int *)malloc((long)sizeof(int) * (CODE_TBL_SIZE + 1));
    dg->huffval = (unsigned char *)malloc((long)CODE_TBL_SIZE + 1);

}


deallocate_hufutil_variables(dg)
    gvl            *dg;
{
    free(dg->bits);
    free(dg->codesize);
    free(dg->others);
    free(dg->freq);
    free(dg->freq_copy);
    free(dg->huffsize);
    free(dg->huffcode);
    free(dg->hufco);
    free(dg->hufsi);
    free(dg->huffval);
}



deallocate_bitstream_buffer(dg)
    gvl            *dg;
{
    free(dg->bs_bfr);
}


/***********************************************************************
!         6/29/89                                                             *
! This subroutine is used to enter a VAL represented in NBITS bits     *
! into the CODER ONLY buffer.   Operates similar to PUTBITS except     *
! it is only called by the block coding routine , hence it is an       *
! incomplete bitstream and output is in ascii rather than binary       *
!                                                                      *
!**********************************************************************/
putcdr(dg, val, nbits)
    gvl            *dg;
    unsigned int    val;
    int             nbits;
{
    int             cdr_delta, i, j;
    char            ctemp;

    if (dg->cdr_fp == NULL)	/**coder_out operational ? */
	return;
    if (nbits == -1) {		/** close ?  **/
	cdr_delta = (dg->cdr_ptr - dg->cdr_bfr + 1);
	write_cdr_bfr(dg, cdr_delta);
	if (dg->cdr_fp)
	    fclose(dg->cdr_fp);
	if (dg->dcr_fp)
	    fclose(dg->dcr_fp);
	return;
    }
    val = val & dg->wmask[nbits];
    if (dg->codingDirection == LSB_FIRST) {
	while (nbits) {
	    if (dg->cdr_unused - nbits <= 0) {
		*dg->cdr_ptr++ |= val << (8 - dg->cdr_unused);
		dg->cdr_nwords++;
		dg->cdr_prev_word = *(dg->cdr_ptr - 1);
		*dg->cdr_ptr = 0;
		if (dg->check_marker && *(dg->cdr_ptr - 1) == -1) {
		    dg->cdr_prev_word = -1;
		    dg->cdr_nwords++;
		    dg->cdr_ptr++;
		    *dg->cdr_ptr = 0;
		}
		val >>= dg->cdr_unused;
		nbits -= dg->cdr_unused;
		dg->cdr_unused = 8;
	    } else {
		*dg->cdr_ptr |= val << (8 - dg->cdr_unused);
		dg->cdr_unused -= nbits;
		nbits = 0;
	    }
	}
    } else {			/** must be MSB_FIRST **/
	while (nbits) {
	    dg->cdr_unused = nbits - dg->cdr_unused;
	    if (dg->cdr_unused >= 0) {
		*dg->cdr_ptr++ |= val >> dg->cdr_unused;
		dg->cdr_nwords++;
		dg->cdr_prev_word = *(dg->cdr_ptr - 1);
		*dg->cdr_ptr = 0;
		if (dg->cdr_nwords && !(dg->cdr_nwords % 4)) {	/** check for longword boundary **/
		    cdr_delta = dg->fifoEntryCount - dg->lastCompressedFifoEntryCount;
		    dg->lastCompressedFifoEntryCount = dg->fifoEntryCount;
		    if (cdr_delta > 31)
			cdr_delta = 31;
		    dg->compressedFifoHisto[cdr_delta]++;
		}
		if (dg->check_marker && *(dg->cdr_ptr - 1) == -1) {
		    dg->cdr_prev_word = -1;
		    dg->cdr_nwords++;
		    dg->cdr_ptr++;
		    *dg->cdr_ptr = 0;
		}
		nbits = dg->cdr_unused;
		dg->cdr_unused = 8;
	    } else {
		dg->cdr_unused = -dg->cdr_unused;
		*dg->cdr_ptr |= val << dg->cdr_unused;
		nbits = 0;
	    }
	}
    }


    if (dg->cdr_ptr >= dg->cdr_bfr_end) {
	write_cdr_bfr(dg, 512);

	dg->cdr_ptr -= 512;
	dg->cdr_bfr[0] = dg->cdr_bfr[512];
	dg->cdr_bfr[1] = dg->cdr_bfr[513];
	dg->cdr_bfr[2] = dg->cdr_bfr[514];
    }
}



write_cdr_bfr(dg, nbytes)
    gvl            *dg;
    int             nbytes;
{
    int             j, k;
    unsigned char   byte1, byte2, byte3, byte4;
#ifdef NeXT
    fprintf(stderr, "In write_cdr_bfr\n");
    exit(1);
#endif
    for (j = 0, k = 0; j < (nbytes + 3) / 4; j++) {
	byte1 = dg->cdr_bfr[k++];
	byte2 = dg->cdr_bfr[k++];
	byte3 = dg->cdr_bfr[k++];
	byte4 = dg->cdr_bfr[k++];

	if (dg->cdr_fp) {
	    fprintf(dg->cdr_fp, "%02X%02X\n", byte1, byte2);
	    fprintf(dg->cdr_fp, "%02X%02X\n", byte3, byte4);
	}
	if (dg->dcr_fp)
	    fprintf(dg->dcr_fp, "%02X%02X%02X%02X\n", byte4, byte3, byte2, byte1);
    }
}

putCdrMarker(dg, marker_code)
    gvl            *dg;
    int             marker_code;
{
    if (dg->cdr_unused < 8)
	putcdr(dg, 0xff, dg->cdr_unused);
    putcdr(dg, 0xff, 8);
    putcdr(dg, marker_code, 8);
}


/*------------------------------------------------------------------------
    init_huffman_code_generator():
	    This routine is called to initialize all the variables needed for the
		 huffman code generator. Some of the initialization are done for testing
		 only, and will later be removed.
 -------------------------------------------------------------------------*/
init_huffman_code_generator(dg, codesize, others, bits)
    gvl            *dg;
    int             codesize[], others[], bits[];
{
    int             x = 0;

    for (x = 0; x <= CODE_TBL_SIZE; x++) {	/* init the size and others
						 * arrays */
	codesize[x] = 0;
	others[x] = -1;
    }
    for (x = 0; x < 32; x++) {	/* init the bits  array */
	bits[x] = 0;
    }
} /* end of function */



/*-------------------------------------------------------------------------
    code_size():
  	    This routine will determine the code lengths for each symbol.
  		 Algorithm is presented on page 18, of the JPEG standard.
 -------------------------------------------------------------------------*/
code_size(dg, freq, others, codesize)
    gvl            *dg;
    int             others[], codesize[];
long int        freq[];
{
    int             c1, c2;	/* used as offsets into freq table */
    int             terminate_flag;	/* used as a flag to stop while loops */
    long            temp_freq1;	/* used for storing freq[c1] prior to
				 * resetting it */

    while (at_least_2_freqs_not_0(freq)) {
	c1 = get_lowest_symbol_freq(freq);	/* c1 = offset of lowest */
	temp_freq1 = freq[c1];
	freq[c1] = 0;

	c2 = get_lowest_symbol_freq(freq);	/* c2 = offset of 2nd lowest */
	freq[c1] = temp_freq1 + freq[c2];	/* updating the frequencies */
	freq[c2] = 0;

	terminate_flag = 0;
	while (!terminate_flag) {
	    codesize[c1]++;
	    if (others[c1] != -1)
		c1 = others[c1];
	    else
		terminate_flag = 1;
	} /* end of while !terminate_flag */

	others[c1] = c2;
	terminate_flag = 0;
	while (!terminate_flag) {
	    codesize[c2]++;
	    if (others[c2] != -1)
		c2 = others[c2];
	    else
		terminate_flag = 1;
	} /* end of while around terminate_flag */
    } /* end of while 2 freq != 0 */
} /* end of function code_size */




/*------------------------------------------------------------------------
    count_number_of_bits():
	    This routine will count the number of code words of the same size.
		 The array bits[] is used to keep the total of each size.
 -----------------------------------------------------------------------*/
count_number_of_bits(dg, codesize, bits)
    gvl            *dg;
    int             codesize[];
unsigned        bits[];
{
    int             i;

    i = 0;
    while (i <= CODE_TBL_SIZE) {
	if (codesize[i] != 0)
	    bits[codesize[i]]++;
	i++;
    }
} /* end of function count_number_of_bits */




/*-------------------------------------------------------------------------
    adjust_bits():
	   This routine is called to reduce all the code sizes to less than 16
		bits. Algorithm is in JPEG page #20.
 -------------------------------------------------------------------------*/
adjust_bits(dg, bits)
    gvl            *dg;
    int             bits[];
{
    int             i, j;
    int             done;

    done = 0;
    i = 31;
    while (!done) {
	if (bits[i] > 0) {
	    j = i - 1;

	    do
		j--;
	    while (bits[j] == 0);

	    bits[i] = bits[i] - 2;
	    bits[i - 1]++;
	    bits[j + 1] = bits[j + 1] + 2;
	    bits[j]--;
	}
	 /* end of if part of if bits[i]>0 */ 
	else {
	    i--;
	    if (i == 16) {
		while (bits[i] == 0)
		    i--;
		bits[i]--;
		done = 1;
	    } /* end of if i==16 */
	} /* end of else part of if bits[i]>0 */
    } /* end of while !done */
} /* end of function adjust_bits */




/*-----------------------------------------------------------------------
   sort_input():
	  This routine will sort the input huffman values based on the codesize
	  of each entry. Algorithm is given in JPEG page #21.
 -----------------------------------------------------------------------*/
sort_input(dg, codesize, huffval)
    gvl            *dg;
    int             codesize[];
unsigned char   huffval[];
{
    int             i, j, p;

    i = 1;
    p = 0;
    while (i <= 32) {
	j = 0;
	while (j <= CODE_TBL_SIZE) {
	    if (codesize[j] == i) {
		huffval[p] = j;
		p++;
	    }
	    j++;
	} /* end of while j<=CODE_TBL_SIZE */
	i++;
    } /* end of while i>32 */
} /* end of function sort_input */



/*-----------------------------------------------------------------------
   size_table():
	  This routine will setup the huffsize[] array with the size of all
	  huffman codes in ascending order. Algorithm is in JPEG page #22.
 ----------------------------------------------------------------------*/
size_table(dg, bits, huffsize)
    gvl            *dg;
    int             bits[], huffsize[];
{
    int             p, i, j;

    p = 0;
    i = 1;
    j = 1;

    while (i <= 16) {
	while (j <= bits[i]) {
	    huffsize[p] = i;
	    p++;
	    j++;
	} /* end of while j<=bits[i] */
	i++;
	j = 1;
    } /* end of while i<=16 */

    huffsize[p] = 0;
    dg->lastp = p;
} /* end of function size_table */




/*-------------------------------------------------------------------------
    code_table():
	   This routine generates the huffman codes for each codesize in the 
		symbol table. Algorithm is in JPEG, page #23.
 ------------------------------------------------------------------------*/
code_table(dg, huffsize, huffcode)
    gvl            *dg;
    int             huffsize[], huffcode[];
{
    int             i, j, p, code, si;
    int             done;

    p = 0;
    code = 0;
    si = huffsize[0];

    done = 0;
    while (!done) {
	do {
	    huffcode[p] = code;
	    code++;
	    p++;
	}
	while (huffsize[p] == si);

	if (huffsize[p] == 0)
	    done = 1;		/* setup to exit the routine */
	else {
	    do {
		code = code << 1;
		si++;
	    }
	    while (huffsize[p] != si);
	} /* end of else part of huffsize[p]=0 */
    } /* end of while !done */



} /* end of function code_table */




/*-----------------------------------------------------------------------
    order_codes():
	    This routine reorders the codewords based on the symbols they 
		 represent. Algorithm in JPEG, page #24.
 ----------------------------------------------------------------------*/
order_codes(dg, huffval, huffcode, huffsize, hufco, hufsi)
    gvl            *dg;
    unsigned char   huffval[];
int             huffcode[], huffsize[], hufsi[], hufco[];
{
    int             p, i;

    p = 0;
    while (p < dg->lastp) {
	i = huffval[p];
	hufco[i] = huffcode[p];
	hufsi[i] = huffsize[p];
	p++;
    } /* end of while p<lastp */

} /* end of function order_codes */




/*--------------------------------------------------------------------------
     make_decoder_tables():
	     This routine will create the mincode[], and maxcode[] arrays, which
		  point to the minimum, and maximum codes within each code size, 
		  respectively.  Algorithm is in JPEG, page #25.
 --------------------------------------------------------------------------*/
make_decoder_tables(dg, bits, huffcode, h_d_s)
    gvl            *dg;
    int             bits[], huffcode[];
hds            *h_d_s;
{
    int             s, p, ms;
    int             done;

    s = 0;
    p = 0;
    done = 0;

    while (!done) {
	s++;
	if (s > 16) {
	    h_d_s->maxcode[ms]++;
	    done = 1;		/* end outer loop */
	} else if (bits[s] != 0) {
	    h_d_s->valptr[s] = p;
	    h_d_s->mincode[s] = huffcode[p];
	    p = p + bits[s] - 1;
	    h_d_s->maxcode[s] = huffcode[p];
	    ms = s;
	    p++;
	} else
	    h_d_s->maxcode[s] = -1;
    } /* end of while !done */
    h_d_s->biggest = h_d_s->maxcode[ms];
} /* end of function decoder_tables */


/*-----------------------------------------------------------------------
    at_least_2_freqs_not_0():
	    This routine will return true, if there are at least 2 frequencies
		 with values greater than 0 in the freq[] table.
 -----------------------------------------------------------------------*/
at_least_2_freqs_not_0(dg, freq)
    gvl            *dg;
    long            freq[];
{
    int             freq_count, index;

    freq_count = 0;
    index = 0;
    for (index = 0; index <= CODE_TBL_SIZE; index++) {
	if (freq[index] != 0)
	    freq_count++;
    }

    if (freq_count > 1)		/* determine the number of non-zero */
	return (1);
    else
	return (0);
} /* end of at_least_2_freqs_not_0 function */



/*------------------------------------------------------------------------
    get_lowest_symbol_freq():
	   This routine gets the smallest frequency value in the freq[] table.
 ------------------------------------------------------------------------*/
int 
get_lowest_symbol_freq(dg, freq)
    gvl            *dg;
    long            freq[];
{
    int             index, pos;
    long            lowest;

    lowest = maximum_frequency_value;
    for (index = 0; index <= CODE_TBL_SIZE; index++) {
	if ((freq[index] != 0) && (freq[index] <= lowest)) {
	    lowest = freq[index];
	    pos = index;	/* offset of lowest frequency in table */
	}
    }
    return (pos);

} /* end of function get_lowest_symbol_freq */




handle_error_in_input(dg)
    gvl            *dg;
{
    printf("\nERROR FOUND WHEN EXPECTING A SYNC\n");
}


handle_eoi(dg)
    gvl            *dg;
{
}



/***********************************************************************
  
This subroutine counts the total number of values represented by the
Huffman code table generated from the bits table
The bits table being one's relative, i.e. the first entry describes
the number of codes of length 1
  
***********************************************************************/

int 
count_bits_tbl(dg, bits, max_num_bits)
    gvl            *dg;
    int             max_num_bits;
    unsigned char  *bits;
{
    int             i, count;

    for (i = 0, count = 0; i < max_num_bits; i++)
	count += bits[i];
    return (count);
}
