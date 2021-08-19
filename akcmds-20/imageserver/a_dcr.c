

/*********************************************************************
This is valuable proprietary information and trade secrets of C-Cube Microsystems
and is not to be disclosed or released without the prior written permission of an
officer of the company.
  
Copyright (c) 1990 by C-Cube Microsystems, Inc.
  
			A_DCR.C 
  
!                                                                    *
!                                                                    *
!                                                                    *
!********************************************************************/

#include "gStruct.h"


#define S_IREAD 400
#define S_IWRITE 200


#ifdef MAC
#include <stdlib.h>
#endif



#define			DECODE_BLOCK_INTERLEAVE		0


int             numberMacArgs = 3;
char           *macArgs[] = {"dummy",
			     "-output_file=balrr.vst",
			     "-debug=1"};

gvl            *dg;

#ifndef NeXT
#ifdef MAC
main()
{
    main2(numberMacArgs, macArgs);
}

main2(argc, argv)
#else
main(argc, argv)
#endif				/* MAC */

    int             argc;
    char           *argv[];
{

    FILE           *newStdOut;
    char           *cp1, *cp2;
    int             i;
    /* Redirect stdio to Files */

    newStdOut = fopen("a_dcr.stdout", "w");
    cp1 = (char *)stdout;
    cp2 = (char *)newStdOut;
    for (i = 0; i < sizeof(FILE); i++)
	*cp1++ = *cp2++;

    dg = (gvl *) malloc((long)sizeof(gvl));

    /** initialize variables put in the dg, and i_og structures **/
    InitArrays(dg);
    initialize_decoder(dg, argc, argv);
    time(&dg->strt_t);
    dg->mCode = getNextMarkerCode(dg);	/** generate start of image **/
    handleNextMarkerCode(dg, dg->mCode, SOI_MARKER);	/** if no error, we return **/
    decode_image_frames(dg);
    time(&dg->stop_t);
    summarize_decoder(dg);

}

#endif				/* NeXT */

initialize_decoder(dg, argc, argv)
    gvl            *dg;
    int             argc;
    char           *argv[];
{
    allocate_hufutil_variables(dg);
    init_decoder_variables(dg);
#ifndef NeXT
    get_decoder_options(dg, argc, argv);
#endif
    init_data_source(dg);
    dg->check_marker = 0;

    initDecoderTableVars(dg);

#ifndef NeXT
    print_decoder_options(dg);
#endif

}



initDecoderTableVars(dg)
    gvl            *dg;
{
    /* initialize the huffval pointers in the decode_struct */
    dg->huf_decode_struct[0][0].huffval = dg->dts.data[0];
    dg->huf_decode_struct[1][0].huffval = dg->dts.data[1];
    dg->huf_decode_struct[0][1].huffval = dg->ats.data[0];
    dg->huf_decode_struct[1][1].huffval = dg->ats.data[1];

    dg->huf_decode_struct[0][0].huffbits = dg->dts.bits[0];
    dg->huf_decode_struct[1][0].huffbits = dg->dts.bits[1];
    dg->huf_decode_struct[0][1].huffbits = dg->ats.bits[0];
    dg->huf_decode_struct[1][1].huffbits = dg->ats.bits[1];

    dg->huf_decode_struct[0][0].valptr = dg->ldc_valptr;
    dg->huf_decode_struct[0][0].mincode = dg->ldc_mincode;
    dg->huf_decode_struct[0][0].maxcode = dg->ldc_maxcode;

    dg->huf_decode_struct[1][0].valptr = dg->kdc_valptr;
    dg->huf_decode_struct[1][0].mincode = dg->kdc_mincode;
    dg->huf_decode_struct[1][0].maxcode = dg->kdc_maxcode;

    dg->huf_decode_struct[0][1].valptr = dg->lac_valptr;
    dg->huf_decode_struct[0][1].mincode = dg->lac_mincode;
    dg->huf_decode_struct[0][1].maxcode = dg->lac_maxcode;

    dg->huf_decode_struct[1][1].valptr = dg->kac_valptr;
    dg->huf_decode_struct[1][1].mincode = dg->kac_mincode;
    dg->huf_decode_struct[1][1].maxcode = dg->kac_maxcode;
}


/*** get std a_cdr bitstream ***/

void 
get_bs_fh(dg)
    gvl            *dg;
{
#ifdef NeXT
    jread(dg->bs_fh, dg->dcr_bs_bfr, 4096);
#else
    read(dg->bs_fh, dg->dcr_bs_bfr, 4096);
#endif
}




/*
 * *
 *
 *	Decode the image *
 *
 *
 */
decode_image_frames(dg)
    gvl            *dg;
{
    short           frameNumber, scanNumber, j;

    frameNumber = -1;

    do {

	dg->mCode = getNextMarkerCode(dg);

	switch (dg->mCode) {

	case SOF_MARKER:
	    frameNumber++;
	    scanNumber = -1;
	    handleNextMarkerCode(dg, dg->mCode, SOF_MARKER);
	    getFrameSignalingParms(dg, frameNumber);
	    break;

	case SOS_MARKER:
	    scanNumber++;
	    getScanSignalingParms(dg, frameNumber, scanNumber);
	    initVarsForNextDecodeScan(dg, frameNumber, scanNumber);
	    dg->check_marker = 1;
	    decodeNextComponent(dg, frameNumber, scanNumber);
	    dg->check_marker = 0;
	    take_your_space_mac(dg);
	    break;

	case DHT_MARKER:
	    getCodeTables(dg);
	    break;

	case DQT_MARKER:
	    getQuantizationTables(dg);
	    break;

	case EOI_MARKER:
	    break;

	default:
	    printf("\nInvalid marker code = %0x", dg->mCode);
	    break;

	}

    }
    while (dg->mCode != EOI_MARKER);
#ifndef NeXT
    close(dg->bs_fh);
    close(dg->ofh);
#endif
}




handleNextMarkerCode(dg, markerRead, markerWanted)
    gvl            *dg;
    short           markerRead, markerWanted;
{
    if (markerRead != markerWanted) {
	printf("\nError.  Expecting a %n markercode, and getting a %n code instead\n",
	       markerWanted, markerRead);
	/* exit(1); */
    }
} /* end of handleNextMarkerCode */




getNextMarkerCode(dg)
    gvl            *dg;
{
    return (sgetmarker(dg));

} /* end of getNextMarkerCode */



initVarsForNextDecodeScan(dg, frameNumb, scanNumb)
    gvl            *dg;
    short           frameNumb, scanNumb;
{
    char            frameScan[32];
    /** must set global code table( AC/DC ), and the global qTablePtr to be used
			for the next components in the scan **/

    dg->num_lines_processed = 0;
    dg->num_interleave_blocks_left = 0;
    dg->num_interleaves_processed = 0;
    dg->num_lines_out = 0;
    dg->blk_no = 0;
    dg->cdr_nblks = 0;
    dg->stripno = 0;

    if (dg->interleavingFormat == DECODE_BLOCK_INTERLEAVE) {
	dg->hy = dg->JPEG2Frame[frameNumb].componentHorBlocksPerIntrlv[0];
	dg->vy = dg->JPEG2Frame[frameNumb].componentVertBlocksPerIntrlv[0];
	dg->hu = dg->JPEG2Frame[frameNumb].componentHorBlocksPerIntrlv[1];
	dg->vu = dg->JPEG2Frame[frameNumb].componentVertBlocksPerIntrlv[1];
	dg->hv = dg->JPEG2Frame[frameNumb].componentHorBlocksPerIntrlv[2];
	dg->vv = dg->JPEG2Frame[frameNumb].componentVertBlocksPerIntrlv[2];
#ifdef NeXT
	if (dg->hu == 0 && dg->vu == 0 && dg->hv == 0 && dg->vv == 0) {
	    dg->pixel_size_out = 1;
	    dg->out_frame = MONOCHROME_NHD;
	}
#endif
    } else {			/** component interleaving **/
	dg->out_frame = 5;
	dg->hy = dg->JPEG2Frame[frameNumb].componentHorBlocksPerIntrlv[scanNumb];
	dg->vy = dg->JPEG2Frame[frameNumb].componentVertBlocksPerIntrlv[scanNumb];
	dg->hu = 0;
	dg->vu = 0;
	dg->hv = 0;
	dg->vv = 0;
    }

    dg->lx = dg->JPEG2Frame[frameNumb].scanComponentHSize[scanNumb];
    dg->lx_u = dg->JPEG2Frame[frameNumb].scanComponentHSize[scanNumb + 1];
    dg->lx_v = dg->JPEG2Frame[frameNumb].scanComponentHSize[scanNumb + 2];
    dg->ly = dg->JPEG2Frame[frameNumb].scanComponentVSize[scanNumb];

    dg->cn = dg->JPEG2Frame[frameNumb].scanNumbOfComponents[scanNumb];

    dg->num_interleave_blocks_per_strip = (dg->lx + (8 * dg->hy - 1)) / (8 * dg->hy);

    dg->padx_y = dg->num_interleave_blocks_per_strip * dg->hy * 8;	/** use to extend line to exact interleave **/
    dg->padx_u = dg->num_interleave_blocks_per_strip * dg->hu * 8;
    dg->padx_v = dg->num_interleave_blocks_per_strip * dg->hv * 8;
    dg->padx_u = dg->padx_y;
    dg->padx_v = dg->padx_y;


    give_me_space_mac(dg);


    strcpy(dg->channelOutputFileName[0], dg->ofilnam);
    sprintf(dg->channelOutputFileName[0], "%s%02d%02d", dg->channelOutputFileName[0], frameNumb, scanNumb);
#ifndef NeXT
    close(dg->ofh);
    if ((dg->ofh = open(
			dg->channelOutputFileName[0],
			O_TRUNC | O_WRONLY | O_BINARY | O_CREAT
#ifdef UNIX
			,S_IREAD | S_IWRITE
#endif
			)
	 ) == -1
	)
	report_open_failure(dg, dg->channelOutputFileName[0]);
#endif				/* NeXT */
    dg->pixel_format_out = dg->pixel_format_array[dg->pixel_size_out];	/* useful for coord
									 * input */
    v_s(dg, VS_INIT_O, dg->out_frame, &dg->ofh, &dg->lx, &dg->ly, &dg->pixel_size_out,
	&dg->datatype);

#ifdef MAC_DISPLAY
    if (dg->display)
	init_mac_display(dg->lx + 1, dg->ly + 1);
#endif				/* MAC_DISPLAY */


} /* end of initVarsForNextDecodeScan */





decodeNextComponent(dg, frameNumb, scanNumb)
    gvl            *dg;
    short           frameNumb, scanNumb;
{
    reset_dc_predictions(dg);
    while (put_interleave(dg)) {
	if (dg->blk_no < dg->str || dg->blk_no > dg->stp)
	    dg->pctl = 0;
	else
	    dg->pctl = dg->debug;

	decode_interleave(dg, frameNumb, scanNumb);
	dequantize_interleave(dg, frameNumb, scanNumb);
	i_trans_interleave(dg, frameNumb, scanNumb);
	if (dg->resync) {
	    if (--dg->resync_count <= 0) {
		decode_sync(dg);
		dg->resync_count = dg->resync;
		reset_dc_predictions(dg);
	    }
	}
	dg->num_interleaves_processed++;
	dg->stripno++;
	dg->blk_no += dg->hy * dg->vy;
    }

} /* end of decodeNextComponent */



/*
 * *
 *
 * Wrapup print output & video output, if any *
 *
 */
summarize_decoder(dg)
    gvl            *dg;
{
#ifndef NeXT
    print_decoder_summary(dg);
#endif
    if (dg->ofh)
	close(dg->ofh);
    deallocate_hufutil_variables(dg);
    free(dg->dcr_bs_bfr);
#ifndef NeXT
    take_your_space_mac(dg);
#endif
}




getFrameSignalingParms(dg, frameNumber)
    gvl            *dg;
    short           frameNumber;
{
    short           temp, frameSize;
    short           lineWidth, numbOfLines, modeSelected, dataPrecision;
    short           resyncIntvl, i, maxH, qTableId;
    short          *cptr;


    frameSize = sgetv(dg, 8) << 8;
    frameSize |= sgetv(dg, 8);	/** LSByte **/

    dataPrecision = sgetv(dg, 8);

    numbOfLines = sgetv(dg, 8) << 8;
    numbOfLines |= sgetv(dg, 8);/** LSByte **/

    lineWidth = sgetv(dg, 8) << 8;
    lineWidth |= sgetv(dg, 8);	/** LSByte **/

    dg->JPEG2Frame[frameNumber].resyncInterval = 0;	/********** To be changed to DRI *****/

    dg->JPEG2Frame[frameNumber].frameNumbOfComponents = sgetv(dg, 8);

    /** for each component loop and get the ID, horz, vert blocks/interleave, qtable **/
    for (i = 0; i < dg->JPEG2Frame[frameNumber].frameNumbOfComponents; i++) {
	dg->JPEG2Frame[frameNumber].frameComponentId[i] = sgetv(dg, 8);
	dg->JPEG2Frame[frameNumber].componentHorBlocksPerIntrlv[i] = sgetv(dg, 4);
	dg->JPEG2Frame[frameNumber].componentVertBlocksPerIntrlv[i] = sgetv(dg, 4);
	dg->JPEG2Frame[frameNumber].frameQuantMatrix[i] = sgetv(dg, 8);
    } /* end of for statement */

    /** find maximum horizontal sampling ratio. ignore vertical **/
    maxH = 0;
    for (i = 0; i < dg->JPEG2Frame[frameNumber].frameNumbOfComponents; i++) {
	if (dg->JPEG2Frame[frameNumber].componentHorBlocksPerIntrlv[i] > maxH)
	    maxH = dg->JPEG2Frame[frameNumber].componentHorBlocksPerIntrlv[i];
    }

    if (maxH == 0) {
	printf("\nError.  maxH can not be 0.\n");
	exit(1);
    }
    /** maxH is valid when we get to this point. If ever! **/
    for (i = 0; i < dg->JPEG2Frame[frameNumber].frameNumbOfComponents; i++) {
	dg->JPEG2Frame[frameNumber].scanComponentVSize[i] = numbOfLines;
	dg->JPEG2Frame[frameNumber].scanComponentHSize[i] = lineWidth *
	    dg->JPEG2Frame[frameNumber].componentHorBlocksPerIntrlv[i] / maxH;
    }

} /* end of getFrameSignalingParms */





getScanSignalingParms(dg, frameNumb, scanNumb)
    gvl            *dg;
    short           frameNumb, scanNumb;
{
    short           l, codeword[256], codesize[256];
    int             bits[17];
    short           i, j, scanSize;
    short           componentID, codeTableId;


    /** 16 bits representing the size of the scan **/
    scanSize = sgetv(dg, 8) << 8;
    scanSize |= sgetv(dg, 8);	/** LSByte **/

    dg->JPEG2Frame[frameNumb].scanNumbOfComponents[scanNumb] = sgetv(dg, 8);

    /** for every component within the scan, get the codeTable selection **/
    for (i = 0; i < dg->JPEG2Frame[frameNumb].scanNumbOfComponents[scanNumb]; i++) {
	componentID = sgetv(dg, 8);
	codeTableId = sgetv(dg, 8);
	dg->JPEG2Frame[frameNumb].scanACCodeTableId[scanNumb][i] = (codeTableId >> 4) & 0x0F;
	dg->JPEG2Frame[frameNumb].scanDCCodeTableId[scanNumb][i] = codeTableId & 0x0F;
	/** kluge for do_inter **/
	dg->JPEG2Frame[frameNumb].scanACCodeTableId[scanNumb][i + 1] = (codeTableId >> 4) & 0x0F;
	dg->JPEG2Frame[frameNumb].scanDCCodeTableId[scanNumb][i + 1] = codeTableId & 0x0F;
    }


    /** get progressive scan parameters...ignore for baseline **/
    i = sgetv(dg, 8);
    i = sgetv(dg, 8);
    i = sgetv(dg, 8);

    /** set interleave format **/
    if (dg->JPEG2Frame[frameNumb].scanNumbOfComponents[scanNumb] == 1)
	dg->interleavingFormat = COMPONENT_INTERLEAVE;
    else
	dg->interleavingFormat = DECODE_BLOCK_INTERLEAVE;

} /* end of getScanSignalingParms */



getQuantizationTables(dg)
    gvl            *dg;
{
    int             i, count, qTableID;
    int            *cptr;
    register int    temp;

    count = sgetv(dg, 16) - 2;

    while (count > 0) {
	qTableID = sgetv(dg, 8);/** top 4 bits are precision **/
	dg->imageQTables[qTableID] = (int *)malloc(64L * sizeof(int));
	cptr = dg->imageQTables[qTableID];
	for (i = 0; i < 64; i++) {
	    temp = (int)sgetv(dg, 8);	/** get q table **/
	    *cptr++ = temp;
	}
	count -= 65;
    }

} /* end of getQuantizationTables */



getCodeTables(dg)
    gvl            *dg;
{
    int             i, j, count, cTableId, tableNumb, valueCounter;
    int             bits[17];
    int             hc_c[256], hc_n[256];
    unsigned char  *cptr;
    char           *cptr2;


    count = sgetv(dg, 16) - 2;

    /** read in the codeTables, and store pointers to them in global structures **/
    while (count > 0) {
	cTableId = sgetv(dg, 8);
	tableNumb = cTableId & 0x0F;	/** table numbers 0,1 are only valid **/
	switch (cTableId & 0xF0) {
	case 0x00:		/** custom DC table specification **/
	    /** read the bits table **/
	    valueCounter = 0;
	    cptr = (unsigned char *)dg->dts.bits[tableNumb];
	    for (i = 0; i < DC_BITS_TABLE_SIZE; i++) {
		*cptr = sgetv(dg, 8);
		valueCounter += *cptr++;
	    }

	    /** read the values represented **/
	    cptr2 = (char *)dg->dts.data[tableNumb];
	    for (i = 0; i < valueCounter; i++)
		*cptr2++ = sgetv(dg, 8);

	    for (j = 0; j < 16; j++)
		bits[j + 1] = dg->dts.bits[tableNumb][j];

	    bits[0] = valueCounter;

	    size_table(dg, bits, dg->decoder_hufsi);
	    code_table(dg, dg->decoder_hufsi, hc_c);
	    make_decoder_tables(dg, bits, hc_c, &dg->huf_decode_struct[tableNumb][0]);

	    break;

	case 0x10:		/** custom AC table specification **/
	    /** read the bits table **/
	    valueCounter = 0;
	    cptr = (unsigned char *)dg->ats.bits[tableNumb];
	    for (i = 0; i < AC_BITS_TABLE_SIZE; i++) {
		*cptr = sgetv(dg, 8);
		valueCounter += *cptr++;
	    }

	    /** read the values represented **/
	    cptr2 = (char *)dg->ats.data[tableNumb];
	    for (i = 0; i < valueCounter; i++)
		*cptr2++ = sgetv(dg, 8);

	    for (j = 0; j < 16; j++)
		bits[j + 1] = dg->ats.bits[tableNumb][j];

	    bits[0] = valueCounter;

	    size_table(dg, bits, dg->decoder_hufsi);
	    code_table(dg, dg->decoder_hufsi, hc_c);
	    make_decoder_tables(dg, bits, hc_c, &dg->huf_decode_struct[tableNumb][1]);

	    break;
	} /* end of switch */

	count -= 17 + valueCounter;

    } /* end of while count > 0 */

} /* end of getCodeTables */


init_decoder_variables(dg)
    gvl            *dg;
{
    dg->num_lines_processed = 0;
    dg->lx = 720;		/* # of pixels per line */
    dg->ly = 576;		/* # of lines */
    dg->num_lines_out = 0;
    /** Image format, TARGA or NHD (KTAS style) **/
    dg->in_frame = TARGA;	/* input image format */
    dg->out_frame = TARGA;	/* output image format */
    dg->file_out = 0;		/* Enable recon image output */
    dg->display = 0;		/* Enable recon to a display board */
    dg->bs_nbytes = 0;


    dg->pixel_size_out = 3;	/* Number of bytes per pixel */
    dg->datatype = 2;		/* TARGA header variable, defines RGB or mono */

    /** processing controls **/
    dg->hy = 0;			/* # of horizontal y blocks per interleave */
    dg->vy = 0;			/* # of vertical y blocks per interleave   */
    dg->hu = 0;			/* # of horizontal u blocks per interleave */
    dg->vu = 0;			/* # of vertical u blocks per interleave   */
    dg->hv = 0;			/* # of horizontal v blocks per interleave */
    dg->vv = 0;			/* # of vertical v blocks per interleave   */
    dg->nb = 0;			/* # of       all  blocks per interleave   */
    dg->cn = 0;			/* # of color components                   */
    dg->q_spec = 0x00;		/* quantization tbl specification          */
    dg->d_spec = 0x00;		/* DC code      tbl specification          */
    dg->a_spec = 0x00;		/* AC code      tbl specification          */

    dg->resync = 0;		/* Resync flag/# interleave blocks per sync */
    dg->factor = 0;		/* Quantization Scale Factor */
    dg->notch = 0;		/* notch @ bottom of each displayed strip */

    dg->debug = 0;

    dg->pctl = 0;
    dg->str = 0;
    dg->stp = 8;

    dg->resync = 0;
    dg->resync_count = 0;
    dg->stripno = 0;
    dg->blk_no = 0;
    dg->num_interleaves_processed = 0;
    dg->num_interleave_blocks_left = 0;
    /***  defaults overridden by operator inputs  ***/

    dg->xfm_type = KTAS;	/* transform type */
    dg->cdr_type = N8R5_CDR;
    dg->getbit_src = GETBIT_FILE;
    dg->getbit_src_format = GNUBUS_ASCII;

    dg->cdr_nblks = 0;
}




get_decoder_options(dg, argc, argv)
    gvl            *dg;
    int             argc;
    char           *argv[];
{

    if (!getopt(dg, argc, argv, "packed_file", dg->bs_fnam))	/* 8/22/89 j?e */
	strcpy(dg->bs_fnam, "packed.fil");

    if ((dg->bs_fh = open(dg->bs_fnam, O_RDONLY | O_BINARY)) == -1) {
	printf("\nDECODE Open failure for input file %s", dg->bs_fnam);
	exit(1);
    }
    dg->file_out = getopt(dg, argc, argv, "output_file", dg->ofilnam);

    dg->cma_fp = NULL;
    dg->rgbout_fp = NULL;

    dg->vlsi_out = 0;

    if (dg->vlsi_out) {		/** 6/29/89 **/
	if ((dg->cma_fp = fopen("cma.dcr", "w")) == NULL)
	    report_open_failure(dg, "cma.dcr");
    }
    /*
     * *
     *
     *	Get command line switches *
     *
     */

    getopt(dg, argc, argv, "ly", &dg->ly);
    getopt(dg, argc, argv, "notch", &dg->notch);
    getopt(dg, argc, argv, "debug", &dg->debug);
    getopt(dg, argc, argv, "str", &dg->str);
    dg->stp = dg->str + 8;
    getopt(dg, argc, argv, "stp", &dg->stp);
    getopt(dg, argc, argv, "display", &dg->display);
    getopt(dg, argc, argv, "file_out", &dg->file_out);
    if (dg->display && !dg->file_out)
	dg->out_frame = TARGA;
    getopt(dg, argc, argv, "out_frame", &dg->out_frame);
    getopt(dg, argc, argv, "xfm_type", &dg->xfm_type);

    /*
     * getbit_src = GETBIT_PTR; 
     *
     */

    dg->hk = 0;
    dg->vk = 0;

    /***  COMPILE TIME OPTIONS  ***/

    dg->xfm_type = KTAS;	/* transform type */
    dg->cdr_type = N8R5_CDR;
    dg->getbit_src = GETBIT_FILE;
    dg->getbit_src_format = GNUBUS_ASCII;
    dg->out_frame = TARGA;
    dg->fdct_record = 0;	/* record Transform data */
    dg->eqdct_record = 0;	/* record Quantizer data */
    dg->dqdct_record = 0;	/* record Dequantizer data */

}



/***
**
	Setup the bitstream location to get the bitstream
**
***/
init_data_source(dg)
    gvl            *dg;
{

    dg->dcr_bs_bfr = (char *)malloc(4096L);	/** 7/6/89 **/
#ifdef NeXT
    jread(dg->bs_fh, dg->dcr_bs_bfr, 4096);
#else
    read(dg->bs_fh, dg->dcr_bs_bfr, 4096);
#endif
    init_sgetv(dg, dg->dcr_bs_bfr, 4096, get_bs_fh);
    dg->nb_fp = NULL;

}




/*
 * *
 *
 *	Summarize inputs *
 *
 */
print_decoder_options(dg)
    gvl            *dg;
{
    printf("\n");
    printf("\n       ANDROMEDA %s\n", dg->version);
    printf("\n       DECOMPRESSION");
    printf("\n\n");
    printf("\nThis is valuable proprietary information and trade secrets of C-Cube Microsystems");
    printf("\nand is not to be disclosed or released without the prior written premission");
    printf("\nof an officer of the company.");
    printf("\n");
    printf("\nCopyright (c) 1990 by C-Cube Microsystems, Inc.");
    printf("\n\n");
    printf("\n Input File Name       : % s", dg->bs_fnam);
    if (dg->file_out) {
	printf("\n Output File Name       : % s", dg->ofilnam);
    } else
	printf("\n No Output Image File   :");

    printf("\n Resync  Interval       : %6d", dg->resync);
    printf("\n Display                : %6s", dg->display ? " ON" : "OFF");
    printf("\n Debug                  : %6s", dg->debug ? " ON" : "OFF");
    if (dg->debug) {
	printf("\n  Block start           : %6d", dg->str);
	printf("\n  Block stop            : %6d", dg->stp);
	printf("\n  Debug value           : %6d", dg->debug);
    }
}

/*
 * *
 *
 * Summarize the image decoding statistics *
 *
 */
print_decoder_summary(dg)
    gvl            *dg;
{
    dg->imsize = (float)(dg->origlx) * dg->origly;
    printf("\n Number of lines        : %6d", dg->ly);
    printf("\n Number of pixels/line  : %6d", dg->lx);
    printf("\n Number of bytes/pixel  : %6d", dg->pixel_size_out);
    printf("\n ***********************:");
    printf("\n Elapsed time (secs)    : %6ld", dg->stop_t - dg->strt_t);
    /*
     * printf("\n Quantization Factor    : %6d",dg->factor); printf("\n Num
     * of syncs detected  : %6d",dg->sync_decoded_counter); printf("\n Size
     * of Bit Stream     : %6ld",dg->bs_nbytes); 
     */
    printf("\n ***********************:");
}






/*****
Desert a sync code
*****/
decode_sync(dg)
    gvl            *dg;
{
    dg->time_for_resync = 1;

}






int 
put_interleave(dg)
    gvl            *dg;
{
    int             i, j, k;

    /** test for done **/
    if (--dg->num_interleave_blocks_left <= 0) {
	put_interleave_strip(dg);
	if (dg->num_lines_processed >= dg->ly)
	    return (0);
	dg->num_lines_processed += (8 * dg->vy);
    }
    /** fetch pointers to y data **/
    for (j = 0, k = 0; j < dg->vy; j++) {
	for (i = 0; i < dg->hy; i++) {
	    dg->yblk_ptrs[k++] = dg->last_yptr[j];
	    dg->last_yptr[j] += 8;
	}
    }

    /** fetch pointers to u data **/
    for (j = 0, k = 0; j < dg->vu; j++) {
	for (i = 0; i < dg->hu; i++) {
	    dg->ublk_ptrs[k++] = dg->last_uptr[j];
	    dg->last_uptr[j] += 8;
	}
    }

    /** fetch pointers to v data **/
    for (j = 0, k = 0; j < dg->vv; j++) {
	for (i = 0; i < dg->hv; i++) {
	    dg->vblk_ptrs[k++] = dg->last_vptr[j];
	    dg->last_vptr[j] += 8;
	}
    }
    return (1);
}
