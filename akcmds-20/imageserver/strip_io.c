/*******
  
This is valuable proprietary information and trade secrets of Creative Circuits
Corporation and is not to be disclosed or released without the prior written
permission of an officer of the company.
  
Copyright (c) 1989 by Creative Circuits Corporation.
  
  
   STRIP_IO.C
   
*******/


#include "gStruct.h"


typedef unsigned char Pixel;






/****
Obtain raw input data in the proportions proscribed
****/

int 
get_interleave_strip(dg)
    gvl            *dg;
{
    int             i, j;	/* */
    Pixel          *lptr, *uptr, *vptr, *yptr;
    int            *ylptr, *ulptr, *vlptr;
    unsigned int   *ptr;
    long            pos;
    long            pos2;
    int             qq, qqq;

    int             lasty, lastu, lastv;

    /** Initialize Interleaved block ptrs **/

    for (j = 0; j < dg->vy; j++) {
	dg->last_yptr[j] = dg->y_strip + (dg->padx_y * 8) * j;
    }
    for (j = 0; j < dg->vu; j++) {
	dg->last_uptr[j] = dg->u_strip + (dg->padx_u * 8) * j;
    }
    for (j = 0; j < dg->vv; j++) {
	dg->last_vptr[j] = dg->v_strip + (dg->padx_v * 8) * j;
    }
    for (j = 0; j < dg->vk; j++) {
	dg->last_kptr[j] = dg->k_strip + (dg->padx_y * 8) * j;
    }


    dg->num_interleave_blocks_left = dg->num_interleave_blocks_per_strip;

    if (dg->xfm_type == BYP_XFM || dg->xfm_type == BYP_Q)
	return (1);

    switch (dg->in_frame) {

	/** 11/28/1989. Vahid. Put in the new case (5) for Component files **/
    case COST_ONE_CHANNEL:
	dg->y_ptr = dg->y_strip;
	for (j = 0; j < 8; j++) {	/** get 8 lines for a strip **/
	    read(dg->ifh, (char *)dg->y_ptr, dg->lx);
	    dg->y_ptr += dg->padx_y;
	}
	break;

    case CST:			/**  CST  in 4:2:2 sequential **/
	dg->y_ptr = dg->y_strip;
	dg->u_ptr = dg->u_strip;
	dg->v_ptr = dg->v_strip;
	for (j = 0; j < 8; j++) {
	    read(dg->ifh, (char *)dg->y_ptr, dg->lx);
	    read(dg->ifh, (char *)dg->u_ptr, dg->lx / 2);
	    read(dg->ifh, (char *)dg->v_ptr, dg->lx / 2);
	    /** shift u & v to +/- **/
	    for (i = 0; i < dg->lx / 2; i++) {
		*(dg->u_ptr + i) ^= 0X80;
		*(dg->v_ptr + i) ^= 0X80;
	    }
	    dg->y_ptr += dg->lx;
	    dg->u_ptr += dg->lx;
	    dg->v_ptr += dg->lx;
	}
	if (dg->sampling == S411)
	    subsamp_uv422(dg);

	break;

    case MONOCHROME_NHD:	/** monochrome no header **/
	dg->y_ptr = dg->y_strip;
	for (j = 0; j < 8; j++) {
#ifdef NeXT
	    v_s(dg, VS_GETLINE, dg->in_frame, dg->y_ptr);
#else
	    read(dg->ifh, (char *)dg->y_ptr, dg->lx);
#endif
	    dg->y_ptr += dg->lx;
	}
	break;



    case NHD:			/** NHD  (KTAS) 4:2:2 y seperated from u/v   **/
	printf(" format NHD is gone gone gone");
	break;




    case TARGA:		/** TARGA **/
	/** pixels expressed in R,G,B 4:4:4                 **/
	/** pixel_format defines pixel width & packing info **/
	/** coord returns y: 0 to 255, u/v: -128 to +127  **/

	switch (dg->vlsi_out) {
	case NULL:
	case VLSI_RGB2YUV422:
	case VLSI_422:
	case VLSI_411:
	case VLSI_MONO:
	    for (j = 0; j < 8; j++) {
		getJthLinePtrs(dg, j);
		v_s(dg, VS_GETLINE, dg->in_frame, dg->rgbline);
		doRGBfp(dg, dg->rgbin_fp,
			dg->pixel_format_in,
			dg->lx,
			dg->rgbline);

		coord(16, RGB2YUV422,
		      dg->pixel_format_in, dg->lx,
		      dg->y_ptr, dg->u_ptr, dg->v_ptr,
		      dg->rgbline);
		extend422(dg);
		if (dg->vlsi_out == VLSI_411)
		    subsamp_uv422(dg);
	    } /** j **/
	    break;		/** from 422 case **/

	case VLSI_444to422:	/** feed the chip  yuv444 **/
	    for (j = 0; j < 8; j++) {
		getJthLinePtrs(dg, j);
		v_s(dg, VS_GETLINE, dg->in_frame, dg->rgbline);

		coord(16, RGB2YUV444,
		      dg->pixel_format_in, dg->lx,
		      dg->y_ptr, dg->u_ptr, dg->v_ptr,
		      dg->rgbline);
		do444fp(dg, dg->rgbin_fp);

		extend444(dg);
		subsamp_uv444(dg);
	    } /** j **/
	    break;		/** from 444 case **/

	case VLSI_4444:	/** feed the chip  yuvK4444 **/
	    for (j = 0; j < 8; j++) {
		getJthLinePtrs(dg, j);
		v_s(dg, VS_GETLINE, dg->in_frame, dg->rgbline);

		coord(16, RGB2YUV444,
		      dg->pixel_format_in, dg->lx,
		      dg->y_ptr, dg->u_ptr, dg->v_ptr,
		      dg->rgbline);
		genK(dg, j);
		do4444fp(dg, dg->rgbin_fp);
	    } /** j **/
	    break;		/** from 444 case **/
	} /** switch-vlsi_out **/


	break;			/** from TARGA type frame **/
    default:
	printf("\nIllegal Frame Type %X", dg->in_frame);
    } /** end of frame switch **/


    /*** output raw pixels to ascii file ***/
    if (dg->vlsi_out && dg->vlsi_out != VLSI_4444)
	doYUVfp(dg, dg->raw_fp);


    return (1);
}

extend422(dg)
    gvl            *dg;
{
    char           *uptr, *vptr, lastu, lastv;
    unsigned char  *yptr, lasty;
    int             numberToExtend, i;
    if (dg->lx % (dg->hy * 16)) {
	numberToExtend = 16 - dg->lx % (dg->hy * 16);
	yptr = dg->y_ptr + dg->lx - 1;
	uptr = (char *)(dg->u_ptr + dg->lx / 2 - 1);
	vptr = (char *)(dg->v_ptr + dg->lx / 2 - 1);
	lasty = *yptr++;
	lastu = *uptr++;
	lastv = *vptr++;
	for (i = 0; i < numberToExtend; i++) {
	    *yptr++ = lasty;
	    *uptr++ = lastu;
	    *vptr++ = lastv;
	}
    }
}

extend444(dg)
    gvl            *dg;
{
    char           *uptr, *vptr, lastu, lastv;
    unsigned char  *yptr, lasty;
    int             numberToExtend, i;
    if (dg->lx % (dg->hy * 8)) {
	numberToExtend = 8 - dg->lx % (dg->hy * 8);
	yptr = dg->y_ptr + dg->lx - 1;
	uptr = (char *)(dg->u_ptr + dg->lx - 1);
	vptr = (char *)(dg->v_ptr + dg->lx - 1);
	lasty = *yptr++;
	lastu = *uptr++;
	lastv = *vptr++;
	for (i = 0; i < numberToExtend; i++) {
	    *yptr++ = lasty;
	    *uptr++ = lastu;
	    *vptr++ = lastv;
	}
    }
}


genK(dg, j)
    gvl            *dg;
    int             j;
{
    unsigned char  *ptr1;
    int             i;
    ptr1 = dg->k_ptr;
    for (i = 0; i < dg->lx; i++) {
	*ptr1++ = i + j + j;
    }
}

getJthLinePtrs(dg, j)
    gvl            *dg;
    int             j;
{
    dg->y_ptr = dg->y_strip + j * dg->padx_y;
    dg->u_ptr = dg->u_strip + j * dg->padx_y;
    dg->v_ptr = dg->v_strip + j * dg->padx_y;
    dg->k_ptr = dg->k_strip + j * dg->padx_y;
}


subsamp_uv422(dg)
    gvl            *dg;
{
    int             i;
    unsigned char  *ptr1, *ptr2;
    unsigned int    ui1, ui2, ui3;

    ptr1 = (unsigned char *)dg->u_ptr;
    ptr2 = (unsigned char *)dg->u_ptr;
    for (i = 0; i < dg->lx / 4; i++) {
	ui1 = *(ptr2) ^ 0x80;
	ui2 = *(ptr2 + 1) ^ 0x80;
	ui3 = ui1 + ui2 + 1;
	*ptr1++ = (ui3 >> 1) ^ 0x80;
	ptr2 += 2;
    }

    ptr1 = (unsigned char *)dg->v_ptr;
    ptr2 = (unsigned char *)dg->v_ptr;
    for (i = 0; i < dg->lx / 4; i++) {
	ui1 = *(ptr2) ^ 0x80;
	ui2 = *(ptr2 + 1) ^ 0x80;
	ui3 = ui1 + ui2 + 1;
	*ptr1++ = (ui3 >> 1) ^ 0x80;
	ptr2 += 2;
    }
}


subsamp_uv444(dg)
    gvl            *dg;
{
    int             i;
    unsigned char  *ptr1, *ptr2;
    unsigned int    ui1, ui2, ui3;

    ptr1 = (unsigned char *)dg->u_ptr;
    ptr2 = (unsigned char *)dg->u_ptr;
    for (i = 0; i < dg->lx / 2; i++) {
	ui1 = *(ptr2) ^ 0x80;
	ui2 = *(ptr2 + 1) ^ 0x80;
	ui3 = ui1 + ui2 + 1;
	*ptr1++ = (ui3 >> 1) ^ 0x80;
	ptr2 += 2;
    }

    ptr1 = (unsigned char *)dg->v_ptr;
    ptr2 = (unsigned char *)dg->v_ptr;
    for (i = 0; i < dg->lx / 2; i++) {
	ui1 = *(ptr2) ^ 0x80;
	ui2 = *(ptr2 + 1) ^ 0x80;
	ui3 = ui1 + ui2 + 1;
	*ptr1++ = (ui3 >> 1) ^ 0x80;
	ptr2 += 2;
    }
}








/****
Insert recon  data in the proportions prescribed
****/

put_interleave_strip(dg)
    gvl            *dg;
{
    Pixel           tempULine[1024], tempVLine[1024];
    int             i, j;
    unsigned char  *lptr, *uptr, *vptr;
    int            *ylptr, *ulptr, *vlptr;
    unsigned int   *ptr;
    long            pos;
    register short  sint;
    char           *rbufPtr, *gbufPtr, *bbufPtr;
    char           *rgblinePtr;

    int             old_stripno;

    dg->num_interleave_blocks_left = dg->num_interleave_blocks_per_strip;




    for (j = 0; j < dg->vy; j++) {
	dg->last_yptr[j] = dg->y_strip + (dg->padx_y * 8) * j;
    }
    for (j = 0; j < dg->vu; j++) {
	dg->last_uptr[j] = dg->u_strip + (dg->padx_u * 8) * j;
    }
    for (j = 0; j < dg->vv; j++) {
	dg->last_vptr[j] = dg->v_strip + (dg->padx_v * 8) * j;
    }
    for (j = 0; j < dg->vk; j++) {
	dg->last_kptr[j] = dg->k_strip + (dg->padx_y * 8) * j;
    }

    if ((old_stripno = dg->stripno - 1) < 0) {
	return;
    }
    /***   check for vlsi output   ***/
    if (dg->vlsi_out && dg->vlsi_out != VLSI_4444)
	doYUVfp(dg, dg->rec_fp);


    switch (dg->out_frame) {
	/** 11/28/1989. Vahid. Put in the new case (5) for Component files **/
    case 5:
	dg->y_ptr = dg->y_strip;
	for (j = 0; j < 8; j++) {	/** get 8 lines for a strip **/
	    write(dg->ofh, (char *)dg->y_ptr, dg->lx);
	    if (dg->display)
		display_mono(dg);
	    dg->y_ptr += dg->padx_y;
	} /* j */
	break;


    case CST:			/** CST **/
	dg->y_ptr = dg->y_strip;
	dg->u_ptr = dg->u_strip;
	dg->v_ptr = dg->v_strip;
	for (j = 0; j < 8; j++) {
	    if (dg->num_lines_out > dg->ly)
		continue;
	    /** shift u & v from +/- **/
	    for (i = 0; i < dg->lx / 2; i++) {
		tempULine[i] = *(dg->u_ptr + i) ^ 0X80;
		tempVLine[i] = *(dg->v_ptr + i) ^ 0X80;
	    }
	    write(dg->ofh, (char *)dg->y_ptr, dg->lx);
	    write(dg->ofh, (char *)tempULine, dg->lx / 2);
	    write(dg->ofh, (char *)tempVLine, dg->lx / 2);
	    dg->rgb_ready = (dg->pixel_format_out == RGB32);	/*** display_if_possible interogates this flag ***/
	    display_if_possible(dg);
	    dg->num_lines_out++;
	    dg->y_ptr += dg->lx;
	    dg->u_ptr += dg->lx;
	    dg->v_ptr += dg->lx;
	}
	break;

    case MONOCHROME_NHD:	/** monochrome no header **/
	dg->y_ptr = dg->y_strip;
	for (j = 0; j < 8; j++) {
#ifdef NeXT
	    v_s(dg, VS_PUTLINE, dg->out_frame, dg->y_ptr);
#else
	    write(dg->ofh, (char *)dg->y_ptr, dg->lx);
	    if (dg->display)
		display_mono(dg);
#endif
	    dg->y_ptr += dg->lx;
	}
	break;

    case NHD:			/** NHD  (KTAS)   **/
	printf(" format NHD is gone gone gone");
	break;

    case TARGA:		/** TARGA                                                    **/
	/** pixels expressed in R,G,B 4:4:4                          **/
	/** pixel_format defines pixel width & packing info          **/
	/** procedure coord transforms YUV to RGB for us             **/

	switch (dg->vlsi_out) {
	case NULL:
	case VLSI_RGB2YUV422:
	case VLSI_422:
	case VLSI_411:
	case VLSI_MONO:
	    for (j = 0; j < 8; j++) {
		getJthLinePtrs(dg, j);
		if (dg->vlsi_out == VLSI_411)
		    interp_uv411(dg);

		coord(16, YUV2RGB422,
		      dg->pixel_format_out, dg->lx,
		      dg->y_ptr, dg->u_ptr, dg->v_ptr,
		      dg->rgbline);
		doRGBfp(dg, dg->rgbout_fp,
			dg->pixel_format_out,
			dg->lx,
			dg->rgbline);
		v_s(dg, VS_PUTLINE, dg->out_frame, dg->rgbline);
		dg->rgb_ready = (dg->pixel_format_out == RGB32);	/*** display_if_possible interogates this flag ***/
		display_if_possible(dg);
	    } /** j **/
	    break;		/** from 422 case **/

	case VLSI_444to422:	/** feed the chip  yuv444 **/
	    for (j = 0; j < 8; j++) {
		getJthLinePtrs(dg, j);

		interp_uv422(dg);

		coord(16, YUV2RGB444,
		      dg->pixel_format_out, dg->lx,
		      dg->y_ptr, dg->u_ptr, dg->v_ptr,
		      dg->rgbline);
		do444fp(dg, dg->rgbout_fp);
		v_s(dg, VS_PUTLINE, dg->out_frame, dg->rgbline);
		dg->rgb_ready = (dg->pixel_format_out == RGB32);	/*** display_if_possible interogates this flag ***/
		display_if_possible(dg);
	    } /** j **/
	    break;		/** from 444 case **/
	case VLSI_4444:	/** feed the chip  yuvK4444 **/
	    for (j = 0; j < 8; j++) {
		getJthLinePtrs(dg, j);
		coord(16, YUV2RGB444,
		      dg->pixel_format_out, dg->lx,
		      dg->y_ptr, dg->u_ptr, dg->v_ptr,
		      dg->rgbline);
		do4444fp(dg, dg->rgbout_fp);
		v_s(dg, VS_PUTLINE, dg->out_frame, dg->rgbline);
		dg->rgb_ready = (dg->pixel_format_out == RGB32);	/*** display_if_possible interogates this flag ***/
		display_if_possible(dg);
	    } /** j **/
	    break;		/** from 444 case **/
	} /** switch-vlsi_out **/
	break;			/** from TARGA frame type  **/
    default:
	printf("\nIllegal Frame Type %X", dg->in_frame);
    } /** end of frame switch **/
}




display_mono(dg)
    gvl            *dg;
{
    int             i;
    char           *ptr1, *ptr2;
    ptr1 = (char *)dg->rgbline;
    ptr2 = (char *)dg->y_ptr;
    for (i = 0; i < dg->lx; i++) {
	*ptr1++ = *ptr2;
	*ptr1++ = *ptr2;
	*ptr1++ = *ptr2++;
	*ptr1++ = 0;
    }
    dg->rgb_ready = TRUE;
    display_if_possible(dg);
}


display_if_possible(dg)
    gvl            *dg;
{
    char           *lb_ptr, *rb_ptr;
#ifdef MAC_DISPLAY
    /* call joe's routine to display one line */
    if (dg->display) {
	if (!dg->rgb_ready)
	    coord(16, dg->sampling == S444 ? YUV2RGB444 : YUV2RGB422,
		  RGB32,
		  dg->lx, dg->y_ptr, dg->u_ptr, dg->v_ptr,
		  dg->rgbline);
	put_mac_line(dg->rgbline, dg->lx);
    }
#endif				/* MAC_DISPLAY */

}

interp_uv411(dg)
    gvl            *dg;
{
    int             i;
    char           *ptr1, *ptr2;
    ptr1 = (char *)dg->u_ptr + dg->lx / 2 - 1;
    ptr2 = (char *)dg->u_ptr + dg->lx / 4 - 1;
    for (i = 0; i < dg->lx / 4; i++) {
	*ptr1-- = *ptr2;
	*ptr1-- = *ptr2--;
    }
    ptr1 = (char *)dg->v_ptr + dg->lx / 2 - 1;
    ptr2 = (char *)dg->v_ptr + dg->lx / 4 - 1;
    for (i = 0; i < dg->lx / 4; i++) {
	*ptr1-- = *ptr2;
	*ptr1-- = *ptr2--;
    }
}


interp_uv422(dg)
    gvl            *dg;
{
    int             i;
    char           *ptr1, *ptr2;
    ptr1 = (char *)dg->u_ptr + dg->lx - 1;
    ptr2 = (char *)dg->u_ptr + dg->lx / 2 - 1;
    for (i = 0; i < dg->lx / 2; i++) {
	*ptr1-- = *ptr2;
	*ptr1-- = *ptr2--;
    }
    ptr1 = (char *)dg->v_ptr + dg->lx - 1;
    ptr2 = (char *)dg->v_ptr + dg->lx / 2 - 1;
    for (i = 0; i < dg->lx / 2; i++) {
	*ptr1-- = *ptr2;
	*ptr1-- = *ptr2--;
    }
}



doYUVfp(dg, fp)
    gvl            *dg;
    FILE           *fp;
{
    int             i, j;

    if (!fp)
	return;

    for (j = 0; j < 8; j++) {
	dg->y_ptr = dg->y_strip + j * dg->padx_y;
	dg->u_ptr = dg->u_strip + j * dg->padx_u;
	dg->v_ptr = dg->v_strip + j * dg->padx_v;
	if (dg->sampling == S444) {
	    for (i = 0; i < dg->lx; i++) {
		fprintf(fp, "%02X%02X%02X\n", *dg->v_ptr++ ^ 0x80,
			*dg->u_ptr++ ^ 0x80,
			*dg->y_ptr++);
	    }
	} else if (dg->sampling == S422) {
	    for (i = 0; i < dg->lx / 2; i++) {
		fprintf(fp, "%02X%02X\n", *dg->u_ptr++ ^ 0x80, *dg->y_ptr++);
		fprintf(fp, "%02X%02X\n", *dg->v_ptr++ ^ 0x80, *dg->y_ptr++);
	    }
	} else if (dg->sampling == S411) {
	    if (!(j & 1)) {	/* lead with u */
		for (i = 0; i < dg->lx / 4; i++) {
		    fprintf(fp, "%01X%02X\n", (*dg->u_ptr & 0xf), *dg->y_ptr++);
		    fprintf(fp, "%01X%02X\n", ((*dg->u_ptr++ ^ 0x80) & 0xf0) >> 4, *dg->y_ptr++);
		    fprintf(fp, "%01X%02X\n", (*dg->v_ptr & 0xf), *dg->y_ptr++);
		    fprintf(fp, "%01X%02X\n", ((*dg->v_ptr++ ^ 0x80) & 0xf0) >> 4, *dg->y_ptr++);
		}
	    } else {		/* lead with v */
		for (i = 0; i < dg->lx / 4; i++) {
		    fprintf(fp, "%01X%02X\n", (*dg->v_ptr & 0xf), *dg->y_ptr++);
		    fprintf(fp, "%01X%02X\n", ((*dg->v_ptr++ ^ 0x80) & 0xf0) >> 4, *dg->y_ptr++);
		    fprintf(fp, "%01X%02X\n", (*dg->u_ptr & 0xf), *dg->y_ptr++);
		    fprintf(fp, "%01X%02X\n", ((*dg->u_ptr++ ^ 0x80) & 0xf0) >> 4, *dg->y_ptr++);
		}
	    }
	} else if (dg->sampling == MONO) {
	    for (i = 0; i < dg->lx / 2; i++) {
		fprintf(fp, "%02X%02X\n", *(dg->y_ptr + 1), *dg->y_ptr);
		dg->y_ptr += 2;
	    }
	}
    } /** j **/
}


doRGBfp(dg, fp, format, npels, rgbbfr)
    gvl            *dg;
    FILE           *fp;
    int             format;
    unsigned int    npels;
    unsigned char   rgbbfr[];
{
    int             i, j;
    if (!fp)
	return;
    switch (format) {
    case RGB32:
	for (i = 0, j = 0; i < npels; i++) {
	    fprintf(fp, "%02X%02X%02X\n", rgbbfr[j],
		    rgbbfr[j + 1],
		    rgbbfr[j + 2]);
	    j += 4;
	} /** i **/
	break;
    case RGB24:
	for (i = 0, j = 0; i < npels; i++) {
	    fprintf(fp, "%02X%02X%02X\n", rgbbfr[j],
		    rgbbfr[j + 1],
		    rgbbfr[j + 2]);
	    j += 3;
	} /** i **/
	break;
    default:;
    } /* switch(format) */
}


do444fp(dg, fp)
    gvl            *dg;
    FILE           *fp;
{
    int             i;
    unsigned char  *ptr1, *ptr2, *ptr3;

    if (!fp)
	return;
    ptr1 = (unsigned char *)dg->y_ptr;
    ptr2 = (unsigned char *)dg->u_ptr;
    ptr3 = (unsigned char *)dg->v_ptr;
    for (i = 0; i < dg->lx; i++)
	fprintf(fp, "%02X%02X%02X\n", *ptr3++ ^ 0x80,
		*ptr2++ ^ 0x80,
		*ptr1++);
}

do4444fp(dg, fp)
    gvl            *dg;
    FILE           *fp;
{
    int             i;
    unsigned char  *ptr1, *ptr2, *ptr3, *ptr4;

    if (!fp)
	return;
    ptr1 = (unsigned char *)dg->y_ptr;
    ptr2 = (unsigned char *)dg->u_ptr;
    ptr3 = (unsigned char *)dg->v_ptr;
    ptr4 = (unsigned char *)dg->k_ptr;
    for (i = 0; i < dg->lx; i++)
	fprintf(fp, "%02X%02X\n%02X%02X\n", *ptr2++ ^ 0x80,
		*ptr1++,
		*ptr4++,
		*ptr3++ ^ 0x80);
}
