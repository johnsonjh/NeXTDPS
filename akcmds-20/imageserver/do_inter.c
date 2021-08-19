/*********************************************************************
  
This is valuable proprietary information and trade secrets of Creative Circuits
Corporation and is not to be disclosed or released without the prior written
permission of an officer of the company.
  
Copyright (c) 1989 by Creative Circuits Corporation.
  
			do_inter.C
*********************************************************************/

#include "gStruct.h"









/*------------------------------------------------------------------------
   f_trans_interleave():
	  This routine transforms an interleave block, and stores
	  the output in the zigzag format.
 -------------------------------------------------------------------------*/
f_trans_interleave(dg)
    gvl            *dg;
{
    int             i, j, k, ix, iy;
    char            blk[64], *blkptr;
    int            *intptr, ii;
    unsigned char  *ptr;

    /** forward transform the y blocks **/
    dg->y_xptr = dg->y_xstrip;	/* point @ slot for first transform block */

    for (j = 0, k = 0; j < dg->vy; j++) {
	for (i = 0; i < dg->hy; i++) {
	    /*
	     * pick up ith hor & jth vert (kth) 8x8 block of Ys from the
	     * interleave 
	     */
	    blkptr = blk;
	    ptr = (unsigned char *)(dg->yblk_ptrs[k]);
	    for (iy = 7; iy >= 0; iy--) {
		for (ix = 7; ix >= 0; ix--)
		    *blkptr++ = (*ptr++ ^ 0x80);	/* y is positive only up
							 * to xform */
		ptr += dg->padx_y - 8;
	    }
	    k++;		/* @ next raw block */
	    ftrans_a_block(dg, blk, dg->y_xptr, dg->lumaChromaFlag);

	    dg->y_xptr += 64;	/* slot for next blk */
	}
    }

    /** forward transform the u blocks **/
    dg->u_xptr = dg->u_xstrip;	/* point @ slot for first transform block */
    for (j = 0, k = 0; j < dg->vu; j++) {
	for (i = 0; i < dg->hu; i++) {
	    blkptr = blk;
	    ptr = (unsigned char *)(dg->ublk_ptrs[k]);
	    for (iy = 7; iy >= 0; iy--) {
		for (ix = 7; ix >= 0; ix--)
		    *blkptr++ = (*ptr++);	/* u is pos/neg to now  leave
						 * it be */
		ptr += dg->padx_u - 8;
	    }
	    k++;		/* @ next raw block */
	    ftrans_a_block(dg, blk, dg->u_xptr, 1);
	    dg->u_xptr += 64;	/* slot for next blk */
	}
    }


    /** forward transform the v blocks **/
    dg->v_xptr = dg->v_xstrip;	/* point @ slot for first transform block */

    for (j = 0, k = 0; j < dg->vv; j++) {
	for (i = 0; i < dg->hv; i++) {
	    blkptr = blk;
	    ptr = (unsigned char *)(dg->vblk_ptrs[k]);
	    for (iy = 7; iy >= 0; iy--) {
		for (ix = 7; ix >= 0; ix--)
		    *blkptr++ = (*ptr++);
		ptr += dg->padx_v - 8;
	    }
	    k++;		/* @ next raw block */
	    ftrans_a_block(dg, blk, dg->v_xptr, 1);


	    dg->v_xptr += 64;	/* slot for next blk */
	}
    }


    /** forward transform the k blocks **/
    dg->k_xptr = dg->k_xstrip;	/* point @ slot for first transform block */

    for (j = 0, k = 0; j < dg->vk; j++) {
	for (i = 0; i < dg->hk; i++) {
	    blkptr = blk;
	    for (iy = 7; iy >= 0; iy--) {
		ptr = (unsigned char *)(dg->kblk_ptrs[k]);
		for (ix = 7; ix >= 0; ix--)
		    *blkptr++ = (*ptr++ ^ 0x80);	/* k is positive only up
							 * to xform */
		ptr += dg->padx_y - 8;
	    }
	    k++;		/* @ next raw block */
	    ftrans_a_block(dg, blk, dg->k_xptr, 1);


	    dg->k_xptr += 64;
	}
    }
}


/****  Forward transform, normalize to 11 bits, snake it out ****/
ftrans_a_block(dg, blk, xptr, LumChrFlag)
    gvl            *dg;
    int            *xptr, LumChrFlag;
    char            blk[];
{
    transform(dg, blk, xptr);
}




/*
 * *
 *
 * Inverse transform a three component interleave block *
 *
 */

fi_trans_interleave(dg)
    gvl            *dg;
{
    /*** Inverse transform an interleave block ***/

    int             i, j, k, ix, iy;
    char            blk[64], *blkptr;
    int            *intptr, ii;
    unsigned char  *ptr;

    /** inverse transform the y blocks **/

    dg->y_dqptr = dg->y_dqstrip;/* point @ slot for first transform block */
    dg->y_qptr = dg->y_qstrip;
    for (j = 0, k = 0; j < dg->vy; j++) {
	for (i = 0; i < dg->hy; i++) {
	    itrans_a_block(dg, blk, dg->y_dqptr, dg->lumaChromaFlag);
	    blkptr = blk;
	    ptr = (unsigned char *)dg->yblk_ptrs[k];
	    for (iy = 7; iy >= 0; iy--) {
		for (ix = 7; ix >= 0; ix--) {
		    *ptr++ = ((*blkptr++)) ^ 0x80;	/* get it */
		}
		ptr += dg->padx_y - 8;
	    }
	    k++;		/* @ next raw block */
	    dg->y_dqptr += 64;	/* slot for next blk */
	    dg->y_qptr += 64;
	}
    }


    /** inverse transform the u blocks **/
    dg->u_dqptr = dg->u_dqstrip;/* point @ slot for first transform block */
    dg->u_qptr = dg->u_qstrip;
    for (j = 0, k = 0; j < dg->vu; j++) {
	for (i = 0; i < dg->hu; i++) {
	    itrans_a_block(dg, blk, dg->u_dqptr, 1);
	    blkptr = blk;
	    ptr = (unsigned char *)dg->ublk_ptrs[k];
	    for (iy = 7; iy >= 0; iy--) {
		for (ix = 7; ix >= 0; ix--) {
		    *ptr++ = *blkptr++;
		}
		ptr += dg->padx_u - 8;
	    }
	    k++;		/* @ next raw block */
	    dg->u_dqptr += 64;	/* slot for next blk */
	    dg->u_qptr += 64;
	}
    }

    /** inverse transform the v blocks **/
    dg->v_dqptr = dg->v_dqstrip;/* point @ slot for first transform block */
    dg->v_qptr = dg->v_qstrip;
    for (j = 0, k = 0; j < dg->vv; j++) {
	for (i = 0; i < dg->hv; i++) {
	    itrans_a_block(dg, blk, dg->v_dqptr, 1);
	    blkptr = blk;
	    ptr = (unsigned char *)dg->vblk_ptrs[k];
	    for (iy = 7; iy >= 0; iy--) {
		for (ix = 7; ix >= 0; ix--) {
		    *ptr++ = *blkptr++;
		}
		ptr += dg->padx_v - 8;
	    }
	    k++;		/* @ next raw block */
	    dg->v_dqptr += 64;	/* slot for next blk */
	    dg->v_qptr += 64;
	}
    }

    /** inverse transform the k blocks **/
    dg->k_dqptr = dg->k_dqstrip;/* point @ slot for first transform block */
    dg->k_qptr = dg->k_qstrip;
    for (j = 0, k = 0; j < dg->vk; j++) {
	for (i = 0; i < dg->hk; i++) {
	    itrans_a_block(dg, blk, dg->k_dqptr, 1);
	    blkptr = blk;
	    for (iy = 7; iy >= 0; iy--) {
		ptr = (unsigned char *)dg->kblk_ptrs[k];
		for (ix = 7; ix >= 0; ix--) {
		    *ptr++ = (*blkptr++) ^ 0x80;
		}
		ptr += dg->padx_y - 8;
	    }
	    k++;		/* @ next raw block */
	    dg->k_dqptr += 64;	/* slot for next blk */
	    dg->k_qptr += 64;
	}
    }
}





/*
 * *
 *
 * Inverse transform a three component interleave block *
 *
 */

i_trans_interleave(dg, frameNumb, scanNumb)
    gvl            *dg;
    short           frameNumb, scanNumb;
{
    /*** Inverse transform an interleave block ***/

    int             i, j, k, ix, iy;
    char            blk[64], *blkptr;
    int            *intptr, ii;
    int            *qPtr;
    short           qIndex;
    unsigned char  *ptr;

    /** inverse transform the y blocks **/

    qIndex = dg->JPEG2Frame[frameNumb].frameQuantMatrix[scanNumb];
    qPtr = (void *)dg->imageQTables[qIndex];

    dg->y_dqptr = dg->y_dqstrip;/* point @ slot for first transform block */
    dg->y_qptr = dg->y_qstrip;
    for (j = 0, k = 0; j < dg->vy; j++) {
	for (i = 0; i < dg->hy; i++) {
	    itrans_a_block(dg, blk, dg->y_dqptr, qIndex);
	    blkptr = blk;
	    ptr = (unsigned char *)dg->yblk_ptrs[k];
	    for (iy = 7; iy >= 0; iy--) {
		for (ix = 7; ix >= 0; ix--) {
		    *ptr++ = (*blkptr++) ^ 0x80;
		}
		ptr += dg->padx_y - 8;
	    }
	    k++;		/* @ next raw block */
	    dg->y_dqptr += 64;	/* slot for next blk */
	    dg->y_qptr += 64;
	}
    }


    /** inverse transform the u blocks **/

    qIndex = dg->JPEG2Frame[frameNumb].frameQuantMatrix[scanNumb + 1];
    qPtr = (void *)dg->imageQTables[qIndex];

    dg->u_dqptr = dg->u_dqstrip;/* point @ slot for first transform block */
    dg->u_qptr = dg->u_qstrip;
    for (j = 0, k = 0; j < dg->vu; j++) {
	for (i = 0; i < dg->hu; i++) {
	    itrans_a_block(dg, blk, dg->u_dqptr, qIndex);
	    blkptr = blk;
	    ptr = (unsigned char *)dg->ublk_ptrs[k];
	    for (iy = 7; iy >= 0; iy--) {
		for (ix = 7; ix >= 0; ix--) {
		    *ptr++ = *blkptr++;
		}
		ptr += dg->padx_u - 8;
	    }
	    k++;		/* @ next raw block */
	    dg->u_dqptr += 64;	/* slot for next blk */
	    dg->u_qptr += 64;
	}
    }

    /** inverse transform the v blocks **/

    qIndex = dg->JPEG2Frame[frameNumb].frameQuantMatrix[scanNumb + 1];
    qPtr = (void *)dg->imageQTables[qIndex];

    dg->v_dqptr = dg->v_dqstrip;/* point @ slot for first transform block */
    dg->v_qptr = dg->v_qstrip;
    for (j = 0, k = 0; j < dg->vv; j++) {
	for (i = 0; i < dg->hv; i++) {
	    itrans_a_block(dg, blk, dg->v_dqptr, qIndex);
	    blkptr = blk;
	    ptr = (unsigned char *)dg->vblk_ptrs[k];
	    for (iy = 7; iy >= 0; iy--) {
		for (ix = 7; ix >= 0; ix--) {
		    *ptr++ = *blkptr++;
		}
		ptr += dg->padx_v - 8;
	    }
	    k++;		/* @ next raw block */
	    dg->v_dqptr += 64;	/* slot for next blk */
	    dg->v_qptr += 64;
	}
    }

    /** inverse transform the k blocks **/

    qIndex = dg->JPEG2Frame[frameNumb].frameQuantMatrix[scanNumb + 1];
    qPtr = (void *)dg->imageQTables[qIndex];

    dg->k_dqptr = dg->k_dqstrip;/* point @ slot for first transform block */
    dg->k_qptr = dg->k_qstrip;
    for (j = 0, k = 0; j < dg->vk; j++) {
	for (i = 0; i < dg->hk; i++) {
	    itrans_a_block(dg, blk, dg->k_dqptr, qIndex);
	    blkptr = blk;
	    dg->lineptr = (unsigned char *)dg->vblk_ptrs[k];
	    for (iy = 7; iy >= 0; iy--) {
		for (ix = 7; ix >= 0; ix--) {
		    *ptr++ = (*blkptr++) ^ 0x80;
		}
		ptr += dg->padx_y - 8;
	    }
	    k++;		/* @ next raw block */
	    dg->k_dqptr += 64;	/* slot for next blk */
	    dg->k_qptr += 64;
	}
    }
}




/****  Normalize to 11 bits, Desnake when needed, Inverse transform ****/

itrans_a_block(dg, blk, xptr, LumChrFlag)
    gvl            *dg;
    int            *xptr, LumChrFlag;
    char            blk[];
{
    rev_transform(dg, blk, xptr);
}



quantize_interleave(dg)
    gvl            *dg;
{
    unsigned int    i, j, k, ix, iy;
    int             blk[64], *blkptr, tblk[64];
    short          *snkptr;
    int            *intptr, ii, xform_val;
    int             visi;

    /** quantize the y_blocks **/
    dg->y_xptr = dg->y_xstrip;	/* point @ slot for first transform block */
    dg->y_qptr = dg->y_qstrip;
    dg->y_dqptr = dg->y_dqstrip;

    for (j = 0; j < dg->vy; j++) {
	for (i = 0; i < dg->hy; i++) {
	    switch (dg->lumaChromaFlag) {
	    case 0:
		quantize_a_blk(dg, dg->y_xptr, dg->y_qptr, dg->y_dqptr, dg->luma_q);
		break;
	    case 1:
		quantize_a_blk(dg, dg->y_xptr, dg->y_qptr, dg->y_dqptr, dg->kroma_q);
		break;
	    }
	    dg->y_xptr += 64;
	    dg->y_qptr += 64;
	    dg->y_dqptr += 64;
	}
    }

    /** quantize the u_blocks **/
    dg->u_xptr = dg->u_xstrip;	/* point @ slot for first transform block */
    dg->u_qptr = dg->u_qstrip;
    dg->u_dqptr = dg->u_dqstrip;

    for (j = 0, k = 0; j < dg->vu; j++) {
	for (i = 0; i < dg->hu; i++) {
	    quantize_a_blk(dg, dg->u_xptr, dg->u_qptr, dg->u_dqptr, dg->kroma_q);
	    dg->u_xptr += 64;
	    dg->u_qptr += 64;
	    dg->u_dqptr += 64;
	}
    }


    /** quantize the v_blocks **/
    dg->v_xptr = dg->v_xstrip;	/* point @ slot for first transform block */
    dg->v_qptr = dg->v_qstrip;
    dg->v_dqptr = dg->v_dqstrip;

    for (j = 0, k = 0; j < dg->vv; j++) {
	for (i = 0; i < dg->hv; i++) {
	    quantize_a_blk(dg, dg->v_xptr, dg->v_qptr, dg->v_dqptr, dg->kroma_q);
	    dg->v_xptr += 64;
	    dg->v_qptr += 64;
	    dg->v_dqptr += 64;
	}
    }


    /** quantize the k_blocks **/
    dg->k_xptr = dg->k_xstrip;	/* point @ slot for first transform block */
    dg->k_qptr = dg->k_qstrip;
    dg->k_dqptr = dg->k_dqstrip;

    for (j = 0, k = 0; j < dg->vk; j++) {
	for (i = 0; i < dg->hk; i++) {
	    quantize_a_blk(dg, dg->k_xptr, dg->k_qptr, dg->k_dqptr, dg->kroma_q);
	    dg->k_xptr += 64;
	    dg->k_qptr += 64;
	    dg->k_dqptr += 64;
	}
    }
} /* end of quantize interleave */




quantize_a_blk(dg, xptr, qptr, dqptr, q)
    gvl            *dg;
    int            *xptr, *qptr, *dqptr, *q;
{
    unsigned int    i, j, k, ix, iy;
    int             blk[64], *blkptr, tblk[64], *qptrcpy, *dqptrcpy;
    short          *snkptr;
    int            *intptr, ii, xform_val;
    int             visi;
    char           *cptr, temp;

    qptrcpy = qptr;
    dqptrcpy = dqptr;
    switch (dg->xfm_type) {
    case KTAS:
    case SDCT:
    case FDCT:
    case DPDCT:
    case BYP_XFM:
	for (ii = 0; ii < 64; ii++) {
	    visi = q[ii];
	    if (*xptr < 0)
		*qptr = (*xptr - (visi) / 2) / (visi);
	    else
		*qptr = (*xptr + (visi) / 2) / (visi);

	    if (dg->cdr_type != N8R5_CDR) {
		if (*qptr < -127)
		    *qptr = -127;
		if (*qptr > 127)
		    *qptr = 127;
	    }
	    qptr++;
	    xptr++;
	}
	break;
    case BYP_Q:
	for (ii = 0; ii < 64; ii++) {
	    *qptr++ = *xptr++;
	}
	break;
    case QSDCT:
	for (ii = 0; ii < 64; ii++) {
	    *qptr++ = *xptr;
	    *dqptr++ = *xptr++;
	}
	break;
    }
}



dequantize_interleave(dg, frameNumb, scanNumb)
    gvl            *dg;
    short           frameNumb, scanNumb;
{
    unsigned int    i, j, k, ix, iy;
    int             blk[64], *blkptr, tblk[64];
    short          *snkptr;
    int            *intptr, ii, xform_val;
    int             visi;
    int             qIndex, *qPtr;


    /** dequantize the y_blocks **/
    qIndex = dg->JPEG2Frame[frameNumb].frameQuantMatrix[scanNumb];
    qPtr = dg->imageQTables[qIndex];


    dg->y_qptr = dg->y_qstrip;
    dg->y_dqptr = dg->y_dqstrip;

    for (j = 0; j < dg->vy; j++) {
	for (i = 0; i < dg->hy; i++) {
	    dequantize_a_blk(dg, dg->y_qptr, dg->y_dqptr, qPtr);
	    dg->y_qptr += 64;
	    dg->y_dqptr += 64;
	}
    }

    /** dequantize the u_blocks **/

    qIndex = dg->JPEG2Frame[frameNumb].frameQuantMatrix[scanNumb + 1];
    qPtr = dg->imageQTables[qIndex];

    dg->u_qptr = dg->u_qstrip;
    dg->u_dqptr = dg->u_dqstrip;

    for (j = 0, k = 0; j < dg->vu; j++) {
	for (i = 0; i < dg->hu; i++) {
	    dequantize_a_blk(dg, dg->u_qptr, dg->u_dqptr, qPtr);
	    dg->u_qptr += 64;
	    dg->u_dqptr += 64;
	}
    }


    /** dequantize the v_blocks **/

    qIndex = dg->JPEG2Frame[frameNumb].frameQuantMatrix[scanNumb + 1];
    qPtr = dg->imageQTables[qIndex];

    dg->v_qptr = dg->v_qstrip;
    dg->v_dqptr = dg->v_dqstrip;

    for (j = 0, k = 0; j < dg->vv; j++) {
	for (i = 0; i < dg->hv; i++) {
	    dequantize_a_blk(dg, dg->v_qptr, dg->v_dqptr, qPtr);
	    dg->v_qptr += 64;
	    dg->v_dqptr += 64;
	}
    }


    /** dequantize the k_blocks **/

    qIndex = dg->JPEG2Frame[frameNumb].frameQuantMatrix[scanNumb + 1];
    qPtr = dg->imageQTables[qIndex];

    dg->k_qptr = dg->k_qstrip;
    dg->k_dqptr = dg->k_dqstrip;

    for (j = 0, k = 0; j < dg->vk; j++) {
	for (i = 0; i < dg->hk; i++) {
	    dequantize_a_blk(dg, dg->k_qptr, dg->k_dqptr, qPtr);
	    dg->k_qptr += 64;
	    dg->k_dqptr += 64;
	}
    }

} /* end of dequantize interleave */


dequantize_a_blk(dg, qptr, dqptr, q)
    gvl            *dg;
    int            *qptr, *dqptr, *q;
{
    unsigned int    i, j, k, ix, iy;
    int             blk[64], *blkptr, tblk[64];
    short          *snkptr;
    int            *intptr, ii, xform_val;
    int             visi;
    char           *cptr, temp;

    switch (dg->xfm_type) {
    case SDCT:
    case FDCT:
    case DPDCT:
    case BYP_Q:
    case BYP_XFM:
    case KTAS:			/* leave in Snake, and deQuantize */
	for (ii = 0; ii < 64; ii++)
	    *dqptr++ = *qptr++ * (*q++);	/* dequantize value */
	break;
    case QSDCT:		/* leave in Snake and leave Quantized */
	for (ii = 0; ii < 64; ii++)
	    *dqptr++ = *qptr++;	/* qidct will take care of dequantizing
				 * values */
	break;
    }

    if (dg->dqdct_record)
	write(dg->dqdct_record_fh, (char *)dqptr, 64 * 2);

}







code_interleave(dg)
    gvl            *dg;
{
    int             i, j, k;

    dg->y_xptr = dg->y_xstrip;
    dg->u_xptr = dg->u_xstrip;
    dg->v_xptr = dg->v_xstrip;
    dg->k_xptr = dg->k_xstrip;

    dg->y_qptr = dg->y_qstrip;
    dg->u_qptr = dg->u_qstrip;
    dg->v_qptr = dg->v_qstrip;
    dg->k_qptr = dg->k_qstrip;


    /** code the y blocks **/
    for (j = 0; j < dg->vy; j++) {
	for (i = 0; i < dg->hy; i++) {

	    switch (dg->lumaChromaFlag) {
	    case 0:
		switch (dg->cdr_type) {
		case N8R5_CDR:	/* */
		    n8r5_coder(dg, dg->y_qptr,
			       &dg->prev_ydc[0],
			       dg->luma_q,
			       &dg->luma_codes);
		    break;
		} /* end of switch on dg->cdr_type */
		break;

	    case 1:
		switch (dg->cdr_type) {
		case VLSI_CDR:	/* */
		case N8R5_CDR:	/* */
		    n8r5_coder(dg, dg->y_qptr,
			       &dg->prev_ydc[0],
			       dg->kroma_q,
			       &dg->kroma_codes);
		    break;
		} /* end of switch on dg->cdr_type */
		break;
	    } /* end of switch on dg->lumaChromaFlag */
	    dg->y_qptr += 64;
	}
    }

    /** code the u blocks **/
    for (j = 0; j < dg->vu; j++) {
	for (i = 0; i < dg->hu; i++) {
	    switch (dg->cdr_type) {
	    case N8R5_CDR:	/* */
		n8r5_coder(dg, dg->u_qptr,
			   &dg->prev_udc[0],
			   dg->kroma_q,
			   &dg->kroma_codes);
		break;
	    }
	    dg->u_qptr += 64;
	}
    }

    /** code the v blocks **/
    for (j = 0; j < dg->vv; j++) {
	for (i = 0; i < dg->hv; i++) {

	    switch (dg->cdr_type) {
	    case N8R5_CDR:	/* */
		n8r5_coder(dg, dg->v_qptr,
			   &dg->prev_vdc[0],
			   dg->kroma_q,
			   &dg->kroma_codes);
		break;
	    }
	    dg->v_qptr += 64;
	}
    }

    /** code the k blocks **/
    for (j = 0; j < dg->vk; j++) {
	for (i = 0; i < dg->hk; i++) {

	    switch (dg->cdr_type) {
	    case N8R5_CDR:	/* */
		n8r5_coder(dg, dg->k_qptr,
			   &dg->prev_kdc[0],
			   dg->kroma_q,
			   &dg->kroma_codes);
		break;
	    }
	    dg->k_qptr += 64;
	}
    }
}



decode_interleave(dg, frameNumb, scanNumb)
    gvl            *dg;
    short           frameNumb, scanNumb;
{
    int             i, j, k;
    int             cIndex, qIndex, *qPtr;

    dg->y_qptr = dg->y_qstrip;
    dg->u_qptr = dg->u_qstrip;
    dg->v_qptr = dg->v_qstrip;
    dg->k_qptr = dg->k_qstrip;


    /** decode the y blocks **/

    cIndex = dg->JPEG2Frame[frameNumb].scanACCodeTableId[scanNumb][0];

    qIndex = dg->JPEG2Frame[frameNumb].frameQuantMatrix[0];
    qPtr = dg->imageQTables[qIndex];

    for (j = 0; j < dg->vy; j++) {
	for (i = 0; i < dg->hy; i++) {
	    switch (dg->cdr_type) {
	    case N8R5_CDR:	/* */
		n8r5_decoder(dg, dg->y_qptr,
			     &dg->prev_ydc[0],
			     qPtr,
			     dg->huf_decode_struct[cIndex]);
		break;
	    } /* end of switch on dg->cdr_type */
	    dg->y_qptr += 64;
	}
    }

    /** decode the u blocks **/

    cIndex = dg->JPEG2Frame[frameNumb].scanACCodeTableId[scanNumb][1];

    qIndex = dg->JPEG2Frame[frameNumb].frameQuantMatrix[1];
    qPtr = dg->imageQTables[qIndex];

    for (j = 0; j < dg->vu; j++) {
	for (i = 0; i < dg->hu; i++) {
	    switch (dg->cdr_type) {
	    case N8R5_CDR:	/* */
		n8r5_decoder(dg, dg->u_qptr,
			     &dg->prev_udc[0],
			     qPtr,
			     dg->huf_decode_struct[cIndex]);
		break;
	    }
	    dg->u_qptr += 64;
	}
    }

    /** decode the v blocks **/

    cIndex = dg->JPEG2Frame[frameNumb].scanACCodeTableId[scanNumb][1];

    qIndex = dg->JPEG2Frame[frameNumb].frameQuantMatrix[1];
    qPtr = dg->imageQTables[qIndex];

    for (j = 0; j < dg->vv; j++) {
	for (i = 0; i < dg->hv; i++) {
	    switch (dg->cdr_type) {
	    case VLSI_CDR:	/* */
	    case N8R5_CDR:	/* */
		n8r5_decoder(dg, dg->v_qptr,
			     &dg->prev_vdc[0],
			     qPtr,
			     dg->huf_decode_struct[cIndex]);
		break;
	    }
	    dg->v_qptr += 64;
	}
    }

    /** decode the k blocks **/

    cIndex = dg->JPEG2Frame[frameNumb].scanACCodeTableId[scanNumb][1];

    qIndex = dg->JPEG2Frame[frameNumb].frameQuantMatrix[1];
    qPtr = dg->imageQTables[qIndex];

    for (j = 0; j < dg->vk; j++) {
	for (i = 0; i < dg->hk; i++) {
	    switch (dg->cdr_type) {
	    case N8R5_CDR:	/* */
		n8r5_decoder(dg, dg->k_qptr,
			     &dg->prev_kdc[0],
			     qPtr,
			     dg->huf_decode_struct[cIndex]);
		break;
	    }
	    dg->k_qptr += 64;
	}
    }
}





reset_dc_predictions(dg)
    gvl            *dg;
{
    int             j;

    for (j = 0; j < dg->vy; j++)
	dg->prev_ydc[j] = 0;
    for (j = 0; j < dg->vu; j++)
	dg->prev_udc[j] = 0;
    for (j = 0; j < dg->vv; j++)
	dg->prev_vdc[j] = 0;
    for (j = 0; j < dg->vk; j++)
	dg->prev_kdc[j] = 0;
}
