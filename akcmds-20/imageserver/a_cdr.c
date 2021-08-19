
/*********************************************************************
This is valuable proprietary information and trade secrets of C-Cube Microsystems
and is not to be disclosed or released without the prior written permission of an
officer of the company.
  
Copyright (c) 1990 by C-Cube Microsystems, Inc.
  
			A_CDR.C
  
Modification history
Date		Name	Summary
6/14/89	JD		1. Chg Resync interval from one byte to two bytes
		VAHID 	1. generation of resyncs.
6/15/89	JD		1. Integrate MAC & PC
6/27/89	JD		1. Integrate multiple transform types
            	     KTAS, QSDCT, SDCT, FDCT
       	  		2. Add mono(chrome) parameter
6/29/89	JD		1. HW zigzag sim output for d troops (zz_fp)
            	2. HW coder sim output for d troops (cdr_fp)
7/6/89	JD		1. Revamp alternate visibility file spec
      	  		2. Revamp reconstructed file name spec  
      	  		3. Add help
7/20/89	JD		1. Move strip I/O to seperate file
      	  		2. Provide 4:1:1 & 4:4:4 capability
      	  		
6/10/90	JD		1. Upgrade to JPEG 8R5.2
      	  		
! Simulates a SEQUENTIAL coding scheme , coding one block at         *
! a time.  Currently codes only with a fixed quantizer               *
!                                                                    *
!                                                                    *
!                                                                    *
!********************************************************************/

#define TRUE 1

#define S_IREAD 400
#define S_IWRITE 200

#include "gStruct.h"

#ifdef NeXT
#import <streams/streams.h>
#endif

#ifdef MAC
#include <stdlib.h>
#endif

#ifndef NeXT
int             numberMacArgs = 3;
char           *macArgs[] = {"-input=bal.vst",
			     "-recon=balr.vst",
			     "-fact=50",
			     "-debug=1"};

gvl            *dg;

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

    dg = (gvl *) malloc((long)sizeof(gvl));	/* allocate one big structure
						 * space */

    time(&dg->strt_t);
    mapStdOut(dg, newStdOut, "a_cdr.stdout");	/* Redirect stdio to Files */
    InitArrays(dg);
    initialize_coder(dg, argc, argv);

    generateMarkerCode(dg, SOI_MARKER);	/** generate start of image **/
    code_image_frames(dg);
    generateMarkerCode(dg, EOI_MARKER);	/** generate start of image **/

    time(&dg->stop_t);
    summarize_coder(dg);

    free(dg);

} /* end of main */


/*---------------------------------------------------------------------------------------
	This routine will allocates, and initializes an alternate stdout.
 --------------------------------------------------------------------------------------*/
mapStdOut(dg, newFP, newName)
    gvl            *dg;
    FILE           *newFP;
    char           *newName;
{
    char           *cp1, *cp2;
    int             i;


    newFP = fopen(newName, "w");
    cp1 = (char *)stdout;
    cp2 = (char *)newFP;
    for (i = 0; i < sizeof(FILE); i++)
	*cp1++ = *cp2++;
}

#endif

#ifdef NeXT
extern int      Factor;		/* set in nxjpeg.c */
#endif

/*---------------------------------------------------------------------------------------
	This routine will allocate, and initialize all the variables needed for the coder.
 --------------------------------------------------------------------------------------*/
initialize_coder(dg, argc, argv)
    gvl            *dg;
    int             argc;
    char           *argv[];
{

    allocate_hufutil_variables(dg);
    init_coder_variables(dg);
#ifndef NeXT
    get_coder_options(dg, argc, argv);
    init_coder_files(dg, argc, argv);
#else
    dg->interleavingFormat = BLOCK_INTERLEAVE;
    dg->factor = Factor;
#endif
    init_sputv(dg, dg->dcr_fp, dg->cdr_fp, dg->bs_fh);	/* Init sputv */
    init_image_input(dg, argc, argv);
    init_frame_struct(dg);
    init_display(dg);
    give_me_space_mac(dg);
    initWhatTakenOut2(dg);
#ifndef NeXT
    print_coder_options(dg);
    init_cumulative_histogram(dg);
#endif
    hardwire_baseline_vars(dg);
    make_coder_quantizers(dg);
    init_huffman_code_tables(dg);
    init_more_coder_vars(dg);

    dg->nonZeroACs = 0;

} /* end of function initialize */




/*---------------------------------------------------------------------------------------
	To code an image the JPEGFrame data structure will describe the number of frames
	per image, and number of scans within each frame. Other important information are
	also setup for usage in this structure. This routine initializes this structure
	currently to handle an RGB image in BLOCK, and COMPONENT interleaving format.
 --------------------------------------------------------------------------------------*/
init_frame_struct(dg)
    gvl            *dg;
{
    dg->JPEGFrameCount = 1;	/** Always only one frame **/
    dg->lumaChromaFlag = -1;	/** Each scan increments by 1 **/

    if (dg->interleavingFormat == BLOCK_INTERLEAVE) {
	dg->JPEGFrame[0].frameNumbOfQMatrices = 2;
	dg->JPEGFrame[0].frameQuantMatrixId[0] = 0;
	dg->JPEGFrame[0].frameQuantMatrixId[1] = 1;
	dg->JPEGFrame[0].frameNumbOfComponents = 3;
	dg->JPEGFrame[0].componentHorBlocksPerIntrlv[0][0] = dg->hy;
	dg->JPEGFrame[0].componentVertBlocksPerIntrlv[0][0] = dg->vy;
	dg->JPEGFrame[0].componentHorBlocksPerIntrlv[0][1] = dg->hu;
	dg->JPEGFrame[0].componentVertBlocksPerIntrlv[0][1] = dg->vu;
	dg->JPEGFrame[0].componentHorBlocksPerIntrlv[0][2] = dg->hv;
	dg->JPEGFrame[0].componentVertBlocksPerIntrlv[0][2] = dg->vv;
	dg->JPEGFrame[0].scanCount = 1;
	dg->JPEGFrame[0].scanComponentCount[0] = 3;
	dg->JPEGFrame[0].scanComponent[0][0] = 0;
	dg->JPEGFrame[0].scanComponent[0][1] = 1;
	dg->JPEGFrame[0].scanComponent[0][2] = 2;
	dg->JPEGFrame[0].scanComponentHSize[0][0] = dg->lx;
	dg->JPEGFrame[0].scanComponentHSize[0][1] = dg->lx_u;
	dg->JPEGFrame[0].scanComponentHSize[0][2] = dg->lx_v;
	dg->JPEGFrame[0].scanComponentVSize[0][0] = dg->ly;
	dg->JPEGFrame[0].scanComponentVSize[0][1] = dg->ly;
	dg->JPEGFrame[0].scanComponentVSize[0][2] = dg->ly;
	dg->JPEGFrame[0].scanNumbOfCodeTables[0] = 2;
	dg->JPEGFrame[0].scanCodeTableId[0][0] = 0;
	dg->JPEGFrame[0].scanCodeTableId[0][1] = 1;
	dg->JPEGFrame[0].scanCodeTableId[0][1] = 1;
	dg->JPEGFrame[0].scanInputFileName[0] = dg->ifilnam;
	dg->JPEGFrame[0].scanOutputFileName[0] = dg->recon_filnam;
	dg->JPEGFrame[0].scanFDCTFileName[0] = dg->channelFDCTFileName[0];
	dg->JPEGFrame[0].scanEQDCTFileName[0] = dg->channelEQDCTFileName[0];
	dg->JPEGFrame[0].scanDQDCTFileName[0] = dg->channelDQDCTFileName[0];
	dg->JPEGFrame[0].scanType[0] = YUV_COMP;
    } else {
	dg->JPEGFrame[0].frameNumbOfQMatrices = 1;
	dg->JPEGFrame[0].frameQuantMatrixId[0] = 0;
	dg->JPEGFrame[0].frameNumbOfComponents = 3;
	dg->JPEGFrame[0].scanCount = 3;
	dg->JPEGFrame[0].componentHorBlocksPerIntrlv[0][0] = 1;
	dg->JPEGFrame[0].componentVertBlocksPerIntrlv[0][0] = 1;
	dg->JPEGFrame[0].componentHorBlocksPerIntrlv[0][1] = 0;
	dg->JPEGFrame[0].componentVertBlocksPerIntrlv[0][1] = 0;
	dg->JPEGFrame[0].componentHorBlocksPerIntrlv[0][2] = 0;
	dg->JPEGFrame[0].componentVertBlocksPerIntrlv[0][2] = 0;
	dg->JPEGFrame[0].componentHorBlocksPerIntrlv[1][0] = 1;
	dg->JPEGFrame[0].componentVertBlocksPerIntrlv[1][0] = 1;
	dg->JPEGFrame[0].componentHorBlocksPerIntrlv[1][1] = 0;
	dg->JPEGFrame[0].componentVertBlocksPerIntrlv[1][1] = 0;
	dg->JPEGFrame[0].componentHorBlocksPerIntrlv[1][2] = 0;
	dg->JPEGFrame[0].componentVertBlocksPerIntrlv[1][2] = 0;
	dg->JPEGFrame[0].componentHorBlocksPerIntrlv[2][0] = 1;
	dg->JPEGFrame[0].componentVertBlocksPerIntrlv[2][0] = 1;
	dg->JPEGFrame[0].componentHorBlocksPerIntrlv[2][1] = 0;
	dg->JPEGFrame[0].componentVertBlocksPerIntrlv[2][1] = 0;
	dg->JPEGFrame[0].componentHorBlocksPerIntrlv[2][2] = 0;
	dg->JPEGFrame[0].componentVertBlocksPerIntrlv[2][2] = 0;
	dg->JPEGFrame[0].scanComponentCount[0] = 1;
	dg->JPEGFrame[0].scanComponentCount[1] = 1;
	dg->JPEGFrame[0].scanComponentCount[2] = 1;
	dg->JPEGFrame[0].scanComponent[0][0] = Y_COMP;
	dg->JPEGFrame[0].scanComponent[1][0] = U_COMP;
	dg->JPEGFrame[0].scanComponent[2][0] = V_COMP;
	dg->JPEGFrame[0].scanComponentHSize[0][0] = dg->lx;
	dg->JPEGFrame[0].scanComponentVSize[0][0] = dg->ly;
	dg->JPEGFrame[0].scanComponentHSize[1][0] = dg->lx / 2;
	dg->JPEGFrame[0].scanComponentVSize[1][0] = dg->ly;
	dg->JPEGFrame[0].scanComponentHSize[2][0] = dg->lx / 2;
	dg->JPEGFrame[0].scanComponentVSize[2][0] = dg->ly;
	dg->JPEGFrame[0].scanNumbOfCodeTables[0] = 1;
	dg->JPEGFrame[0].scanNumbOfCodeTables[1] = 1;
	dg->JPEGFrame[0].scanNumbOfCodeTables[2] = 1;
	dg->JPEGFrame[0].scanCodeTableId[0][0] = 0;
	dg->JPEGFrame[0].scanCodeTableId[1][0] = 1;
	dg->JPEGFrame[0].scanCodeTableId[2][0] = 1;
	dg->JPEGFrame[0].scanInputFileName[0] = dg->channelInputFileName[0];
	dg->JPEGFrame[0].scanOutputFileName[0] = dg->channelOutputFileName[0];
	dg->JPEGFrame[0].scanInputFileName[1] = dg->channelInputFileName[1];
	dg->JPEGFrame[0].scanOutputFileName[1] = dg->channelOutputFileName[1];
	dg->JPEGFrame[0].scanInputFileName[2] = dg->channelInputFileName[2];
	dg->JPEGFrame[0].scanOutputFileName[2] = dg->channelOutputFileName[2];
	dg->JPEGFrame[0].scanFDCTFileName[0] = dg->channelFDCTFileName[0];
	dg->JPEGFrame[0].scanEQDCTFileName[0] = dg->channelEQDCTFileName[0];
	dg->JPEGFrame[0].scanDQDCTFileName[0] = dg->channelDQDCTFileName[0];
	dg->JPEGFrame[0].scanFDCTFileName[1] = dg->channelFDCTFileName[1];
	dg->JPEGFrame[0].scanEQDCTFileName[1] = dg->channelEQDCTFileName[1];
	dg->JPEGFrame[0].scanDQDCTFileName[1] = dg->channelDQDCTFileName[1];
	dg->JPEGFrame[0].scanFDCTFileName[2] = dg->channelFDCTFileName[2];
	dg->JPEGFrame[0].scanEQDCTFileName[2] = dg->channelEQDCTFileName[2];
	dg->JPEGFrame[0].scanDQDCTFileName[2] = dg->channelDQDCTFileName[2];
	dg->JPEGFrame[0].scanType[0] = Y_COMP;
	dg->JPEGFrame[0].scanType[1] = U_COMP;
	dg->JPEGFrame[0].scanType[2] = V_COMP;
    }
}



initWhatTakenOut2(dg)
    gvl            *dg;
{
    dg->totbit = 0;		/** init to 0 **/
    dg->help = 0;		/** init to 0 **/
    strcpy(dg->byp_filnam, "trans_by.in");
    dg->xcs_sync = 0;		/* # of sync 1's in excess of 16 */

    dg->wmask[0] = 0x0000;
    dg->wmask[1] = 0x0001;
    dg->wmask[2] = 0x0003;
    dg->wmask[3] = 0x0007;
    dg->wmask[4] = 0x000f;
    dg->wmask[5] = 0x001f;
    dg->wmask[6] = 0x003f;
    dg->wmask[7] = 0x007f;
    dg->wmask[8] = 0x00ff;
    dg->wmask[9] = 0x01ff;
    dg->wmask[10] = 0x03ff;
    dg->wmask[11] = 0x07ff;
    dg->wmask[12] = 0x0fff;
    dg->wmask[13] = 0x1fff;
    dg->wmask[14] = 0x3fff;
    dg->wmask[15] = 0x7fff;
    dg->wmask[16] = 0xffff;

} /* end of initWhatTakenOut2 */




/*---------------------------------------------------------------------------------------
	Code the image, based on the interleaving format chosen. For every frame, generate
	a start of frame, followed by frame signaling parameters. For every scan within a
	frame a start of scan followed by scan parameters are send to the bitstream as well.
 --------------------------------------------------------------------------------------*/
code_image_frames(dg)
    gvl            *dg;
{
    int             sync_coded, i, j, k;

    if (dg->interleavingFormat == BLOCK_INTERLEAVE) {
	dg->check_marker = 0;
	/** There will be 1 Frame, with 1 Scan within the frame **/
	/** generate quantization tables **/
	generateQuantizationTables(dg);
	/** generate start of frame **/
	generateMarkerCode(dg, SOF_MARKER);
	generateFrameSignalParms(dg, 0, YUV_COMP);
	/** generate code tables **/
	generateCodeTables(dg, 0, 3);
	/** generate start of Scan **/
	generateMarkerCode(dg, SOS_MARKER);
	/** write out the header for the scan **/
	initVarsForNextScan(dg, 0, 0);
	generateScanSignalParms(dg, dg->JPEGFrame[0].scanType[0]);
	dg->check_marker = 1;	/** check for maker codes and stuff zeros **/
	Location(DATA,bytelocation(dg));
	codeNextComponent(dg);
	dg->check_marker = 0;	/** turn off marker code checking **/
    } else {
	/** Component interleaving. Initialization will have determined the number of
				frames, and the count of scans within each frame needed. **/
	dg->check_marker = 0;
	/** generate quantization tables **/
	generateQuantizationTables(dg);
	/** generate start of frame **/
	generateMarkerCode(dg, SOF_MARKER);
	/** dg->frameSignal[i].componentType is set in the initialization **/
	dg->lx = dg->JPEGFrame[0].scanComponentHSize[0][0];
	generateFrameSignalParms(dg, 0);

	for (j = 0; j < dg->JPEGFrame[0].scanCount; j++) {
	    /** generate code tables **/
	    if (j < 2)
		generateCodeTables(dg, j * 2, j * 2 + 1);
	    /** generate start of Scan **/
	    generateMarkerCode(dg, SOS_MARKER);
	    initVarsForNextScan(dg, j, 0);
	    generateScanSignalParms(dg, dg->JPEGFrame[0].scanType[j]);
	    dg->check_marker = 1;
	    codeNextComponent(dg);
	    dg->check_marker = 0;
	}
    }

} /* end of function code_image_frame */




/*---------------------------------------------------------------------------------------
	For every scan the coder must be reinitialized.
 --------------------------------------------------------------------------------------*/
initVarsForNextScan(dg, scan, frame)
    gvl            *dg;
    short           scan, frame;
{
    int             t1, t2;
    dg->lumaChromaFlag++;
    if (dg->lumaChromaFlag >= 1) {
	dg->lumaChromaFlag = 1;
	dg->qTableToUse = (int *)dg->kroma_q;
    } else {
	dg->qTableToUse = (int *)dg->luma_q;
    }

    dg->num_lines_processed = 0;
    dg->num_interleave_blocks_left = 0;
    dg->num_interleaves_processed = 0;
    dg->num_lines_out = 0;
    dg->blk_no = 0;
    dg->cdr_nblks = 0;
    dg->stripno = 0;
    /** This is done, due to DoInter.c being hardwired **/
    dg->hy = dg->JPEGFrame[frame].componentHorBlocksPerIntrlv[scan][0];
    dg->vy = dg->JPEGFrame[frame].componentVertBlocksPerIntrlv[scan][0];
    dg->hu = dg->JPEGFrame[frame].componentHorBlocksPerIntrlv[scan][1];
    dg->vu = dg->JPEGFrame[frame].componentVertBlocksPerIntrlv[scan][1];
    dg->hv = dg->JPEGFrame[frame].componentHorBlocksPerIntrlv[scan][2];
    dg->vv = dg->JPEGFrame[frame].componentVertBlocksPerIntrlv[scan][2];

    dg->lx = dg->JPEGFrame[frame].scanComponentHSize[scan][0];
    dg->lx_u = dg->JPEGFrame[frame].scanComponentHSize[scan][1];
    dg->lx_v = dg->JPEGFrame[frame].scanComponentHSize[scan][2];
    dg->ly = dg->JPEGFrame[frame].scanComponentVSize[scan][0];

    dg->cn = dg->JPEGFrame[frame].scanComponentCount[scan];

    dg->num_interleave_blocks_per_strip = (dg->lx + (8 * dg->hy - 1)) / (8 * dg->hy);

    dg->padx_y = dg->num_interleave_blocks_per_strip * dg->hy * 8;	/** use to extend line to exact interleave **/
    dg->padx_u = dg->num_interleave_blocks_per_strip * dg->hu * 8;
    dg->padx_v = dg->num_interleave_blocks_per_strip * dg->hv * 8;
    dg->padx_u = dg->padx_y;
    dg->padx_v = dg->padx_y;

#ifndef NeXT
    close(dg->ifh);
    close(dg->ofh);

    if ((dg->ifh = open(dg->JPEGFrame[frame].scanInputFileName[scan],
			O_RDONLY | O_BINARY)) == -1)
	report_open_failure(dg, dg->JPEGFrame[frame].scanInputFileName[scan]);
#endif				/* NeXT */
    v_s(dg, VS_INIT_I, dg->in_frame, &dg->ifh, &t1, &t2);
#ifndef NeXT
    if (dg->JPEGFrame[frame].scanOutputFileName[scan] != NULL) {
	if ((dg->ofh = open(
			    dg->JPEGFrame[frame].scanOutputFileName[scan],
			    O_RDWR | O_CREAT | O_BINARY
#ifdef UNIX
			    ,S_IREAD | S_IWRITE
#endif
			    )) == -1)
	    report_open_failure(dg, dg->JPEGFrame[frame].scanOutputFileName[scan]);
	dg->pixel_size_out = 3;

	if (dg->out_frame == TARGA) {
	    if (dg->interleavingFormat == BLOCK_INTERLEAVE) {
		dg->pixel_size_out = 3;
		dg->datatype = 2;	/* make it RGB */
	    } else {
		dg->pixel_size_out = 1;
		dg->datatype = 3;	/* make it monochrome */
	    }

	    /***			dg->pixel_size_out = 2;   ***/

	    dg->pixel_format_out = dg->pixel_format_array[dg->pixel_size_out];	/* useful for coord
										 * input */
	    v_s(dg,
		VS_INIT_O,	/* init targa file header */
		dg->out_frame,
		&dg->ofh,
		&dg->lx, &dg->ly,
		&dg->pixel_size_out,
		&dg->datatype);
	}
    }
#endif				/* NeXT */
} /* end of initVarsForNextScan */






/*---------------------------------------------------------------------------------------
	Coding of an image consists of forward transform, quantize, and huffman coding of
	8x8 blocks. If a reconstructed image is needed, the data prior to being coded is
	inversed transformed. The image is divided into interleaves, where an interleave
	contains 1..n blocks of each component.
 --------------------------------------------------------------------------------------*/
codeNextComponent(dg)
    gvl            *dg;
{
    int             delta;
    dg->num_interleaves_processed = 0;
    reset_dc_predictions(dg);
    while (get_interleave(dg)) {
	if (dg->blk_no < dg->str || dg->blk_no > dg->stp)
	    dg->pctl = 0;
	else {
	    dg->pctl = dg->debug;
	    if (dg->pctl & 1 && dg->xfm_type == QSDCT)
		printf("\n\n\n Interleave Block %d", dg->num_interleaves_processed);
	}

	f_trans_interleave(dg);
	quantize_interleave(dg);
	code_interleave(dg);
	if (dg->numberInterleavesPerResync && --dg->InterleaveCount <= 0) {
	    dg->InterleaveCount = dg->numberInterleavesPerResync;
	    dg->check_marker = 0;
	    putCdrMarker(dg, (RST_MARKER & 0xF8) | (dg->resyncCount % 8));
	    generateMarkerCode(dg, (RST_MARKER & 0xF8) | (dg->resyncCount++ % 8));
	    dg->check_marker = 1;
	    reset_dc_predictions(dg);
	}
	delta = dg->fifoEntryCount - dg->lastRawFifoEntryCount;
	dg->lastRawFifoEntryCount = dg->fifoEntryCount;
	if (delta > 511)
	    delta = 511;
	dg->rawFifoHisto[delta]++;

	dg->num_interleaves_processed++;
	dg->blk_no += dg->hy * dg->vy;
    }

} /* end of codeNextComponent */





/*-------------------------------------------------------------------------------------
	This routine will generate the header for a frame, based on the samplingRatio send
	to it. The generated header is then put in the bitstream.
 -------------------------------------------------------------------------------------*/
generateFrameSignalParms(dg, frameNumb)
    gvl            *dg;
    int             frameNumb;
{
    short           i, j, k;
    char           *cptr;
    short           qMatPrec;

    dg->frameComponentSpec.length = 10;
    dg->frameComponentSpec.compCount = 3;	/** 3 comps **/
#ifdef NeXT
    dg->frameComponentSpec.componentID1 = 1;
    dg->frameComponentSpec.h1v1 =
	(dg->JPEGFrame[0].componentHorBlocksPerIntrlv[0][0] << 4) |
	dg->JPEGFrame[0].componentVertBlocksPerIntrlv[0][0];
    dg->frameComponentSpec.qTable1 = 0;
    dg->frameComponentSpec.componentID2 = 2;
    dg->frameComponentSpec.h2v2 =
	(dg->JPEGFrame[0].componentHorBlocksPerIntrlv[0][1] << 4) |
	dg->JPEGFrame[0].componentVertBlocksPerIntrlv[0][1];
    dg->frameComponentSpec.qTable2 = 1;
    dg->frameComponentSpec.componentID3 = 3;
    dg->frameComponentSpec.h3v3 =
	(dg->JPEGFrame[0].componentHorBlocksPerIntrlv[0][2] << 4) |
	dg->JPEGFrame[0].componentVertBlocksPerIntrlv[0][2];
    dg->frameComponentSpec.qTable3 = 1;
#else
    dg->frameComponentSpec.componentID1 = 1;
    dg->frameComponentSpec.h1v1 = 0x21;
    dg->frameComponentSpec.qTable1 = 0;
    dg->frameComponentSpec.componentID2 = 2;
    dg->frameComponentSpec.h2v2 = 0x11;
    dg->frameComponentSpec.qTable2 = 1;
    dg->frameComponentSpec.componentID3 = 3;
    dg->frameComponentSpec.h3v3 = 0x11;
    dg->frameComponentSpec.qTable3 = 1;
#endif
    dg->JPEGFxdFrame.signalingLength = sizeof_JPEGFxdFrame +
	dg->frameComponentSpec.length;

    dg->JPEGFxdFrame.dataPrecision = 8;	/** input data's precision **/
    dg->JPEGFxdFrame.lineLengthHB = dg->lx >> 8;
    dg->JPEGFxdFrame.lineLengthLB = dg->lx & 255;
    dg->JPEGFxdFrame.numbOfLinesHB = dg->ly >> 8;
    dg->JPEGFxdFrame.numbOfLinesLB = dg->ly & 255;

    /** output the frame header to the bitstream **/
    cptr = (char *)&dg->JPEGFxdFrame;
    for (i = 0; i < sizeof_JPEGFxdFrame; i++)	/** fixed portion **/
	sputv(dg, *cptr++, 8);

    /** output the component specification **/
    cptr = (char *)&dg->frameComponentSpec.compCount;
    for (i = 0; i < dg->frameComponentSpec.length; i++)
	sputv(dg, *cptr++, 8);

} /* end of generateFrameSignalParms */





/*-------------------------------------------------------------------------------------
	This routine will generate the header for a Scan, based on the component
	type. The generated header is then put in the bitstream.
 -------------------------------------------------------------------------------------*/
generateScanSignalParms(dg, component)
    gvl            *dg;
    short           component;
{
    char           *cptr;
    short           i, j;

    switch (component) {
    case Y_COMP:
	dg->JPEGScanHdr.signalingLength = 2 + 1 + 1 + 1 + 1 + 1 + 1;
	dg->JPEGScanHdr.compCount = 1;
	dg->JPEGScanHdr.componentID1 = 1;
	dg->JPEGScanHdr.codeTable1 = 0x00;
	dg->JPEGScanHdr.spectralSelectStart = 0;
	dg->JPEGScanHdr.spectralSelectEnd = 63;
	dg->JPEGScanHdr.successiveApproxBits = 0;
	break;

    case U_COMP:
	dg->JPEGScanHdr.signalingLength = 2 + 1 + 1 + 1 + 1 + 1 + 1;
	dg->JPEGScanHdr.compCount = 1;
	dg->JPEGScanHdr.componentID1 = 2;
	dg->JPEGScanHdr.codeTable1 = 0x11;
	dg->JPEGScanHdr.spectralSelectStart = 0;
	dg->JPEGScanHdr.spectralSelectEnd = 63;
	dg->JPEGScanHdr.successiveApproxBits = 0;
	break;

    case V_COMP:
	dg->JPEGScanHdr.signalingLength = 2 + 1 + 1 + 1 + 1 + 1 + 1;
	dg->JPEGScanHdr.compCount = 1;
	dg->JPEGScanHdr.componentID1 = 3;
	dg->JPEGScanHdr.codeTable1 = 0x11;
	dg->JPEGScanHdr.spectralSelectStart = 0;
	dg->JPEGScanHdr.spectralSelectEnd = 63;
	dg->JPEGScanHdr.successiveApproxBits = 0;
	break;

    case YUV_COMP:
	dg->JPEGScanHdr.signalingLength = 2 + 1 + 3 * (1 + 1) + 1 + 1 + 1;
	dg->JPEGScanHdr.compCount = 3;
	dg->JPEGScanHdr.componentID1 = 1;
	dg->JPEGScanHdr.codeTable1 = 0x00;
	dg->JPEGScanHdr.componentID2 = 2;
	dg->JPEGScanHdr.codeTable2 = 0x11;
	dg->JPEGScanHdr.componentID3 = 3;
	dg->JPEGScanHdr.codeTable3 = 0x11;
	dg->JPEGScanHdr.spectralSelectStart = 0;
	dg->JPEGScanHdr.spectralSelectEnd = 63;
	dg->JPEGScanHdr.successiveApproxBits = 0;
	break;

    }

    /** send size of scan to bitstream **/
    cptr = (char *)&dg->JPEGScanHdr.signalingLength;
    sputv(dg, *cptr++, 8);
    sputv(dg, *cptr++, 8);

    /** send scan component specification **/
    cptr = (char *)&dg->JPEGScanHdr.compCount;
    for (i = 0; i < 1 + dg->JPEGScanHdr.compCount * 2; i++)
	sputv(dg, *cptr++, 8);

    /** send progressive mode signaling **/
    cptr = (char *)&dg->JPEGScanHdr.spectralSelectStart;
    sputv(dg, *cptr++, 8);
    sputv(dg, *cptr++, 8);
    sputv(dg, *cptr++, 8);

    dg->InterleaveCount = dg->numberInterleavesPerResync;
    dg->resyncCount = 0;

#ifdef MAC_DISPLAY
    if (dg->display) {
	if (dg->display)
	    init_mac_display(dg->lx + 1, dg->ly + 1);
    }
#endif				/* MAC_DISPLAY */

} /* end of generateScanSignalParms */



generateQuantizationTables(dg)
    gvl            *dg;
{
    int             i, j;
    unsigned int   *iptr;

    dg->qTable.qTableCount = 2;	/** init qtable parameters **/
    dg->qTable.qTablePtr[0] = dg->luma_q;
    dg->qTable.qTablePtr[1] = dg->kroma_q;

    generateMarkerCode(dg, DQT_MARKER);	/** send DQT marker code **/

    sputv(dg, 2 + 65 * dg->qTable.qTableCount, 16);	/** send byte count **/

    for (i = 0; i < dg->qTable.qTableCount; i++) {
	sputv(dg, i, 8);	/** send precision and table number **/
	if(i == 0)
	    Location(QUANT0,bytelocation(dg));
	else
	    Location(QUANT1,bytelocation(dg));

	iptr = dg->qTable.qTablePtr[i];	/** q tables are in terms of ints **/
	for (j = 0; j < 64; j++) {
	    sputv(dg, *iptr++, 8);
	}
    }

} /* end of generateQuantizationTables */



generateCodeTables(dg, first, last)
    gvl            *dg;
    int             first, last;
{
    int             i, j, count;
    int             codeTableSpec[4];

    codeTableSpec[0] = 0x00;	/** DC table **/
    codeTableSpec[1] = 0x10;	/** AC table **/
    codeTableSpec[2] = 0x01;	/** DC table **/
    codeTableSpec[3] = 0x11;	/** AC table **/


    generateMarkerCode(dg, DHT_MARKER);	/** send DHT marker code **/

    /** find value count in specified tables **/
    for (i = first, count = 0; i <= last; i++) {
	switch (codeTableSpec[i]) {
	case 0x00:
	    count += dg->dts.nent[0];
	    break;
	case 0x10:
	    count += dg->ats.nent[0];
	    break;
	case 0x01:
	    count += dg->dts.nent[1];
	    break;
	case 0x11:
	    count += dg->ats.nent[1];
	    break;
	}
    }

    sputv(dg, 2 + (last - first + 1) * (1 + 16) + count, 16);	/** send byte count **/

    /** download specified code tables **/
    for (i = first; i <= last; i++) {

	sputv(dg, codeTableSpec[i], 8);

	switch (codeTableSpec[i]) {
	case 0x00:
	    Location(HUFFDC0,bytelocation(dg));
	    make_huffman_code_table(dg, 0);
	    for (j = 0; j < 16; j++)
		sputv(dg, dg->dts.bits[0][j], 8 * sizeof(char));
	    for (j = 0; j < dg->dts.nent[0]; j++)
		sputv(dg, dg->dts.data[0][j], 8 * sizeof(char));
	    break;
	case 0x10:
	    Location(HUFFAC0,bytelocation(dg));
	    for (j = 0; j < 16; j++)
		sputv(dg, dg->ats.bits[0][j], 8 * sizeof(char));
	    for (j = 0; j < dg->ats.nent[0]; j++)
		sputv(dg, dg->ats.data[0][j], 8 * sizeof(char));
	    break;
	case 0x01:
	    Location(HUFFDC1,bytelocation(dg));
	    make_huffman_code_table(dg, 1);
	    for (j = 0; j < 16; j++)
		sputv(dg, dg->dts.bits[1][j], 8 * sizeof(char));
	    for (j = 0; j < dg->dts.nent[1]; j++)
		sputv(dg, dg->dts.data[1][j], 8 * sizeof(char));
	    break;
	case 0x11:
	    Location(HUFFAC1,bytelocation(dg));
	    for (j = 0; j < 16; j++)
		sputv(dg, dg->ats.bits[1][j], 8 * sizeof(char));
	    for (j = 0; j < dg->ats.nent[1]; j++)
		sputv(dg, dg->ats.data[1][j], 8 * sizeof(char));
	    break;
	}

    }


} /* end of generateHuffmanTables */


generateMarkerCode(dg, code)
    gvl            *dg;
    short           code;
{

    sputmarker(dg, code);

} /* end of generateMarkerCode */






summarize_coder(dg)
    gvl            *dg;
{
    /* update_histograms ( dg   ); */
    update_bitstream_header(dg);
    deallocate_hufutil_variables(dg);
    deallocate_bitstream_buffer(dg);
#ifndef NeXT
    print_coder_summary(dg);
#endif
    fill_report(dg);
    take_your_space_mac(dg);
    close(dg->ofh);
    close(dg->ifh);
    close(dg->visi_fh);
} /* end of function summarize_coder */




init_coder_variables(dg)
    gvl            *dg;
{

    dg->num_lines_processed = 0;
    dg->num_interleave_blocks_left = 0;
    dg->num_interleaves_processed = 0;

    dg->lx = 128;		/* # of pixels per line */
    dg->ly = 128;		/* # of lines */
    dg->num_lines_out = 0;

    /** Image format, TARGA or NHD (KTAS style) **/
    dg->in_frame = TARGA;	/* input image format */
    dg->out_frame = TARGA;	/* output image format */
    dg->file_out = 0;		/* Enable recon image output */
    dg->display = 0;

#ifdef MSDOS_DISPLAY
    dg->display = 1;
#else
#ifdef MAC_DISPLAY
    dg->display = 1;
#endif				/* end of MAC_DISPLAY */
#endif				/* end of MSDOS_DISPLAY */


    dg->pixel_size_in = 2;	/* Number of bytes per pixel */
    dg->pixel_size_out = 3;	/* Number of bytes per pixel */
    dg->datatype = 2;		/* TARGA header variable, defines RGB or mono */

    /** processing controls **/
    dg->mono = 0;
    dg->four44 = 0;
    dg->four22 = 0;
    dg->four11 = 0;
#ifndef NeXT
    dg->hy = 2;			/* # of horizontal y blocks per interleave */
    dg->vy = 1;			/* # of vertical y blocks per interleave   */
    dg->hu = 1;			/* # of horizontal u blocks per interleave */
    dg->vu = 1;			/* # of vertical u blocks per interleave   */
    dg->hv = 1;			/* # of horizontal v blocks per interleave */
    dg->vv = 1;			/* # of vertical v blocks per interleave   */
    dg->hk = 0;			/* # of horizontal k blocks per interleave */
    dg->vk = 0;			/* # of vertical k blocks per interleave   */
    dg->nb = 4;			/* # of       all  blocks per interleave   */
    dg->cn = 3;			/* # of color components                   */
#endif
    dg->q_spec = 0xc0;		/* quantization tbl specification          */
    dg->d_spec = 0xc0;		/* DC code      tbl specification          */
    dg->a_spec = 0xc0;		/* AC code      tbl specification          */

    dg->resync = 0;		/* Resync flag/# interleave blocks per sync */
    dg->factor = 25;		/* Quantization Scale Factor */
    dg->hc_init = 0;		/* generate flat dist codes, else read from
				 * file */
    dg->hh_init = 0;		/* clear histogram counters, else read init
				 * values */
    dg->visi_alt = 0;		/* get alternate visibility matrices thru
				 * filename */
    /* following output image filename */
    dg->debug = 0;

    dg->lx2 = dg->lx / 2;	/**	6/29/89	**/
    dg->nb_visi = 0;
    dg->nb_dc_lum_tbl = 0;
    dg->nb_dc_lum_dat = 0;
    dg->nb_ac_lum_tbl = 0;
    dg->nb_ac_lum_dat = 0;
    dg->nb_dc_chr_tbl = 0;
    dg->nb_dc_chr_dat = 0;
    dg->nb_ac_chr_tbl = 0;
    dg->nb_ac_chr_dat = 0;
    dg->coder_out = 0;
    dg->zigzag_out = 0;
    dg->vlsi_out = 0;
    dg->proto_out = 0;
    dg->raw_out = 0;






    /* controls for debug output on block basis */
    dg->pctl = 0;
    dg->str = 0;
    dg->stp = 8;		/* block bounds for debug, switch selectable */
    dg->blk_no = 0;
    dg->cdr_nblks = 0;

    dg->resync_count = 0;
    dg->stripno = 0;

    dg->nover = 0;

}





get_coder_options(dg, argc, argv)
    gvl            *dg;
    int             argc;
    char           *argv[];
{
    short           i, j;
    dg->help = 0;
    getopt(dg, argc, argv, "help", &dg->help);	/* 7/6/89 */
    if (argc < 2 || dg->help) {
	printf("\n       B A S E L I N E    S I M U L A T I O N");
	printf("\n\n");
	printf("\n Invocation:");
	printf("\n\n");
	printf("\n A_CDR [options]");
	printf("\n  ");
	printf("\n  where the option names, preceded by either - or / are:");

	printf("\n  input_file=input_image_filename[bal.vst]");
	printf("\n  recon=reconstructed_image_filename");
	printf("\n  packed_file=bitstream_filename[packed.fil]");
	printf("\n  ");
	printf("\n  help");
	printf("\n  ");
	printf("\n  factor=nn");
	printf("\n  resync=nn");
	printf("\n  lx=nn");
	printf("\n  ly=nn");
	printf("\n  in_frame=n");
	printf("\n  out_frame=n");
	printf("\n  ");
	printf("\n  mono");
	printf("\n  four44");
	printf("\n  four22");
	printf("\n  four11");
	printf("\n  cn=nn");
	printf("\n  hy=nn");
	printf("\n  vy=nn");
	printf("\n  hu=nn");
	printf("\n  vu=nn");
	printf("\n  hv=nn");
	printf("\n  vv=nn");
	printf("\n  ");
	printf("\n  ");
	exit(0);
    }
    strcpy(dg->packedfile, "packed.fil");
    strcpy(dg->ifilnam, "bal.vst");
    getopt(dg, argc, argv, "input_file", dg->ifilnam);	/* 8/22/89 j?e */
    if (strlen(dg->ifilnam) > 128)
	report_open_failure(dg, "input filename > 128 chars... make it shorter");

    getopt(dg, argc, argv, "packed_file", dg->packedfile);

    dg->recon_filnam[0] = 0;;
    getopt(dg, argc, argv, "recon_file", dg->recon_filnam);
    dg->file_out = dg->recon_filnam[0];

    getopt(dg, argc, argv, "interleavingFormat", &dg->interleavingFormat);	/* 6/27/89 */

    getopt(dg, argc, argv, "mono", &dg->mono);	/* 6/27/89 */
    getopt(dg, argc, argv, "four44", &dg->four44);	/* 7/20/89 */
    getopt(dg, argc, argv, "four22", &dg->four22);	/* 7/20/89 */
    getopt(dg, argc, argv, "four11", &dg->four11);	/* 7/20/89 */
    if (dg->mono) {
	dg->cn = 1;
	dg->hy = 1;
	dg->vy = 1;
	dg->hu = 0;
	dg->vu = 0;
	dg->hv = 0;
	dg->vv = 0;
	dg->hk = 0;
	dg->vk = 0;
    }
    if (dg->four44) {		/* 7/20/89 */
	dg->cn = 3;
	dg->hy = 1;
	dg->vy = 1;
	dg->hu = 1;
	dg->vu = 1;
	dg->hv = 1;
	dg->vv = 1;
	dg->hk = 0;
	dg->vk = 0;
    }
    if (dg->four22) {
	dg->cn = 3;
	dg->hy = 2;
	dg->vy = 1;
	dg->hu = 1;
	dg->vu = 1;
	dg->hv = 1;
	dg->vv = 1;;
	dg->hk = 0;
	dg->vk = 0;
    }
    if (dg->four11) {
	dg->cn = 3;
	dg->hy = 4;
	dg->vy = 1;
	dg->hu = 1;
	dg->vu = 1;
	dg->hv = 1;
	dg->vv = 1;;
	dg->hk = 0;
	dg->vk = 0;
    }
    getopt(dg, argc, argv, "hy", &dg->hy);
    getopt(dg, argc, argv, "vy", &dg->vy);
    getopt(dg, argc, argv, "hu", &dg->hu);
    getopt(dg, argc, argv, "vu", &dg->vu);
    getopt(dg, argc, argv, "hv", &dg->hv);
    getopt(dg, argc, argv, "vv", &dg->vv);
    getopt(dg, argc, argv, "cn", &dg->cn);

    dg->sampling = SOTHER;
    if (dg->cn == 3 && dg->hy == 1) {
	dg->four44 = 1;
	dg->sampling = S444;
    }
    if (dg->cn == 3 && dg->hy == 2) {
	dg->four22 = 1;
	dg->sampling = S422;
    }
    if (dg->cn == 3 && dg->hy == 4) {
	dg->four11 = 1;
	dg->sampling = S411;
    }
    if (dg->mono)
	dg->sampling = MONO;

    dg->nb = dg->hy * dg->vy + dg->hu * dg->vu + dg->vu * dg->vv;
    dg->bip.cn = dg->cn;
    dg->bip.hivi[0][0] = dg->hy;
    dg->bip.hivi[0][1] = dg->vy;
    dg->bip.hivi[1][0] = dg->hu;
    dg->bip.hivi[1][1] = dg->vu;
    dg->bip.hivi[2][0] = dg->hv;
    dg->bip.hivi[2][1] = dg->vv;
    getopt(dg, argc, argv, "q_spec", &dg->q_spec);
    getopt(dg, argc, argv, "d_spec", &dg->d_spec);
    getopt(dg, argc, argv, "a_spec", &dg->a_spec);
    getopt(dg, argc, argv, "visi_alt", &dg->visi_alt);
    getopt(dg, argc, argv, "hc_init", &dg->hc_init);
    getopt(dg, argc, argv, "hh_init", &dg->hh_init);
    getopt(dg, argc, argv, "debug", &dg->debug);
    getopt(dg, argc, argv, "lx", &dg->lx);
    getopt(dg, argc, argv, "ly", &dg->ly);
    getopt(dg, argc, argv, "resync", &dg->resync);
    if (dg->resync)
	dg->resync_count = dg->resync;

    getopt(dg, argc, argv, "xfm_type", &dg->xfm_type);	/* 6/27/89 */
    getopt(dg, argc, argv, "cdr_type", &dg->cdr_type);	/* 8/08/89 */
    getopt(dg, argc, argv, "visi", &dg->visi_alt);	/* 7/6/89 */
    strcpy(dg->visifn, "QUANT.DAT");
    getopt(dg, argc, argv, "file_out", &dg->file_out);	/* 7/6/89 */

    getopt(dg, argc, argv, "factor", &dg->factor);
    getopt(dg, argc, argv, "in_frame", &dg->in_frame);


    dg->vlsi_out = 0;


    /*** COMPILE TIME OPTIONS ***/
    dg->interleavingFormat = BLOCK_INTERLEAVE;

    if (dg->interleavingFormat != BLOCK_INTERLEAVE)
	dg->in_frame = 5;
    else
	dg->in_frame = TARGA;



    dg->xfm_type = KTAS;	/* transform type */

    dg->cdr_type = N8R5_CDR;	/* coder type */

    dg->numberInterleavesPerResync = 0;

    dg->enbFifoHistoOuto = FALSE;

    /*** for monochrome ****	
    	dg->sampling = MONO;
      
    	dg->in_frame=MONOCHROME_NHD;
    	dg->cn=1;dg->hy=1;dg->vy=1;dg->hu=0;dg->vu=0;dg->hv=0;dg->vv=0;dg->hk=0;dg->vk=0;
    	dg->vlsi_out = 0;
    	dg->xfm_type=KTAS;
    	dg->numberInterleavesPerResync = 0;
    /*** end of monochrome ***/


    dg->codingDirection = MSB_FIRST;

    dg->fdct_record = 0;	/* record Transform data */
    dg->eqdct_record = 0;	/* record Quantizer data */
    dg->dqdct_record = 0;	/* record Dequantizer data */

    if (dg->xfm_type == BYP_Q || dg->xfm_type == BYP_XFM)
	dg->in_frame = CST;

    /*** END OF COMPILE TIME OPTIONS ***/
    if (dg->sampling == MONO) {
	dg->cn = 1;
	dg->hy = 1;
	dg->vy = 1;
	dg->hu = 0;
	dg->vu = 0;
	dg->hv = 0;
	dg->vv = 0;
    }
    for (i = 0; i < dg->cn; i++) {
	strcpy(dg->channelInputFileName[i], dg->ifilnam);
	if (dg->file_out)
	    strcpy(dg->channelOutputFileName[i], dg->recon_filnam);
	else
	    strcpy(dg->channelOutputFileName[i], "");

	if (dg->fdct_record)
	    strcpy(dg->channelFDCTFileName[i], dg->ifilnam);
	else
	    strcpy(dg->channelFDCTFileName[i], "");

	if (dg->eqdct_record)
	    strcpy(dg->channelEQDCTFileName[i], dg->ifilnam);
	else
	    strcpy(dg->channelEQDCTFileName[i], "");

	if (dg->dqdct_record)
	    strcpy(dg->channelDQDCTFileName[i], dg->ifilnam);
	else
	    strcpy(dg->channelDQDCTFileName[i], "");

    }




    if (dg->cn == 3 && dg->interleavingFormat != BLOCK_INTERLEAVE) {
	i = 0;
	while (dg->channelInputFileName[0][i] && dg->channelInputFileName[0][i] != '.')
	    i++;
	for (j = 0; j < 3; j++) {
	    dg->channelInputFileName[j][i] = 0;
	    dg->channelFDCTFileName[j][i] = 0;
	    dg->channelEQDCTFileName[j][i] = 0;
	    dg->channelDQDCTFileName[j][i] = 0;
	}
	strcat(dg->channelInputFileName[0], ".Y");
	strcat(dg->channelInputFileName[1], ".Cb");
	strcat(dg->channelInputFileName[2], ".Cr");

	strcat(dg->channelOutputFileName[0], ".Y");
	strcat(dg->channelOutputFileName[1], ".Cb");
	strcat(dg->channelOutputFileName[2], ".Cr");

	strcat(dg->channelFDCTFileName[0], ".FDCT.Y");
	strcat(dg->channelFDCTFileName[1], ".FDCT.Cb");
	strcat(dg->channelFDCTFileName[2], ".FDCT.Cr");

	strcat(dg->channelEQDCTFileName[0], ".EQDCT.Y");
	strcat(dg->channelEQDCTFileName[1], ".EQDCT.Cb");
	strcat(dg->channelEQDCTFileName[2], ".EQDCT.Cr");

	strcat(dg->channelDQDCTFileName[0], ".DQDCT.Y");
	strcat(dg->channelDQDCTFileName[1], ".DQDCT.Cb");
	strcat(dg->channelDQDCTFileName[2], ".DQDCT.Cr");
    }
    if (dg->interleavingFormat == BLOCK_INTERLEAVE) {
	strcat(dg->channelFDCTFileName[0], ".FDCT");
	strcat(dg->channelEQDCTFileName[0], ".EDCT");
	strcat(dg->channelDQDCTFileName[0], ".DDCT");
    }
    dg->out_frame = dg->in_frame;
    getopt(dg, argc, argv, "display", &dg->display);
    if (dg->display && !dg->file_out)
	dg->out_frame = TARGA;
    getopt(dg, argc, argv, "out_frame", &dg->out_frame);
    getopt(dg, argc, argv, "str", &dg->str);
    dg->stp = dg->str + 8;
    getopt(dg, argc, argv, "stp", &dg->stp);
    getopt(dg, argc, argv, "pctl", &dg->pctl);

    if (dg->interleavingFormat == COMPONENT_INTERLEAVE)
	dg->in_frame = dg->out_frame = 5;

    if (dg->in_frame == MONOCHROME_NHD)
	dg->out_frame = MONOCHROME_NHD;


} /* end of function get_coder_options */


init_coder_files(dg, argc, argv)
    gvl            *dg;
    int             argc;
    char           *argv[];
{

    dg->zz_fp = NULL;
    dg->raw_fp = NULL;
    dg->rec_fp = NULL;
    dg->cdr_fp = NULL;
    dg->dcr_fp = NULL;
    dg->cma_fp = NULL;
    dg->rgbin_fp = NULL;
    dg->rgbout_fp = NULL;

    if ((dg->bs_fh = open(dg->packedfile, O_TRUNC | O_WRONLY | O_BINARY | O_CREAT
#ifdef UNIX
			  ,S_IREAD | S_IWRITE
#endif
			  )) == -1)
	report_open_failure(dg, dg->packedfile);



} /* end of function init_coder_files */



init_image_input(dg, argc, argv)
    gvl            *dg;
    int             argc;
    char           *argv[];
{
#ifdef NeXT
    v_s(dg, VS_INIT_I, dg->in_frame, &dg->ifh, &dg->lx);
    return;
#endif

    if ((dg->in_frame == 1) && (dg->interleavingFormat == BLOCK_INTERLEAVE)) {	/** TARGA  **/
	dg->ifh = open(dg->ifilnam, O_RDONLY | O_BINARY);
	if (read(dg->ifh, (char *)&dg->hdr, sizeof(dg->hdr)) <= 0) {	/* read vista header */
	    printf("\nHeader read error");
	    exit(1);
	}
	lseek(dg->ifh, 0L, SEEK_SET);
	dg->lx = dg->hdr.x_msb << 8 | dg->hdr.x_lsb;
	dg->ly = dg->hdr.y_msb << 8 | dg->hdr.y_lsb;
	dg->datatype = dg->hdr.datatype;
	dg->pixel_size_in = dg->hdr.databits / 8;
    }
    dg->pixel_format_in = dg->pixel_format_array[dg->pixel_size_in];	/* useful for coord
									 * input */

    /* handy for debug ... let the operator work with a smaller image */
    getopt(dg, argc, argv, "ly", &dg->ly);
    getopt(dg, argc, argv, "lx", &dg->lx);
    dg->lx_u = (dg->lx * dg->hu) / dg->hy;	/* 7/20/89 */
    dg->lx_v = (dg->lx * dg->hv) / dg->hy;	/* 7/20/89 */

    dg->num_interleave_blocks_per_strip = (dg->lx + (8 * dg->hy - 1)) / (8 * dg->hy);

    dg->padx_y = dg->num_interleave_blocks_per_strip * dg->hy * 8;	/** use to extend line to exact interleave **/
    dg->padx_u = dg->num_interleave_blocks_per_strip * dg->hu * 8;
    dg->padx_v = dg->num_interleave_blocks_per_strip * dg->hv * 8;
    dg->padx_u = dg->padx_y;
    dg->padx_v = dg->padx_y;



    /* init input & ouput image  */
    v_s(dg, VS_INIT_I, dg->in_frame, &dg->ifh, &dg->lx);
    dg->pixel_size_out = 3;
    if (dg->file_out)
	v_s(VS_INIT_O, dg->out_frame, &dg->ofh, &dg->lx, &dg->ly, &dg->pixel_size_out,
	    &dg->datatype);	/* params could be prettier */
    if (dg->ifh)
	close(dg->ifh);

} /* end of function init_image_input */


init_display(dg)
    gvl            *dg;
{
} /* end of function init_display */




print_coder_options(dg)
    gvl            *dg;
{
    unsigned        i, j, k;
    printf("\n       ANDROMEDA %s\n", dg->version);
    printf("\n");
    printf("\n       COMPRESSION");
    printf("\n\n");
    printf("\nThis is valuable proprietary information and trade secrets of C-Cube Microsystems");
    printf("\nand is not to be disclosed or released without the prior written premission");
    printf("\nof an officer of the company.");
    printf("\n");
    printf("\nCopyright (c) 1990 by C-Cube Microsystems, Inc.");
    printf("\n\n");
    printf("\n Input File Name        : % s", dg->ifilnam);
    printf("\n Packed File Name       : % s", dg->packedfile);
    if (dg->file_out)
	printf("\n Recon File Name        : % s", dg->recon_filnam);
    else
	printf("\n No Output Image File   :");

    printf("\n Number of lines        : %6d", dg->ly);
    printf("\n Number of pixels/line  : %6d", dg->lx);
    printf("\n Input File Format      : %6s", dg->ifor[dg->in_frame]);
    if (dg->in_frame == TARGA) {
	printf("\n Pixel Size             : %6d", dg->pixel_size_in);
	printf("\n Image Data Type        : %6d", dg->datatype);
    }
    if (dg->file_out) {
	printf("\n Output File Format     : %6s", dg->ifor[dg->out_frame]);
	if (dg->out_frame == TARGA) {
	    printf("\n Pixel Size             : %6d", dg->pixel_size_out);
	    printf("\n Image Data Type        : %6d", dg->datatype);
	}
    }
    if (dg->vlsi_out)
	printf("\n VLSI Processing Mode   : %6s", dg->vlsi_processing_states[dg->vlsi_out]);
    printf("\n Quantization Factor    : %6d", dg->factor);
    printf("\n Coding Direction from  : %6s", dg->codingDirection == LSB_FIRST ? "LSB" : "MSB");
    printf("\n Resync  Interval       : %6d", dg->numberInterleavesPerResync);
    if (dg->sampling != SOTHER)
	printf("\n Sampling               : %6s", dg->sampling == S444 ? "4:4:4" :
	       dg->sampling == S422 ? "4:2:2" :
	       dg->sampling == S411 ? "4:1:1" :
	       dg->sampling == S4444 ? "4 CHNL" :
	       dg->sampling == MONO ? " MONO" : "     ");
    printf("\n Coder Type             : %6s", dg->ctyp[dg->cdr_type]);
    printf("\n Xform Type             : %6s", dg->xtyp[dg->xfm_type]);
    printf("\n Display                : %6s", dg->display ? " ON" : "OFF");
    printf("\n Debug                  : %6s", dg->debug ? " ON" : "OFF");
    if (dg->debug) {
	printf("\n  Block start           : %6d", dg->str);
	printf("\n  Block stop            : %6d", dg->stp);
	printf("\n  Debug value           : %6d", dg->debug);
    }
} /* end of function print_coder_options */



init_cumulative_histogram(dg)
    gvl            *dg;
{
    unsigned        i, j, k, nbytes;
    /** either read cumulative histogram from "hist.dat" or start clean **/
    if (dg->hh_init) {
	for (j = 0; j < CODE_TBL_SIZE; j++) {
	    dg->luma_codes.dc_h[j] = 0;
	    dg->luma_codes.ac_h[j] = 0;
	    dg->kroma_codes.dc_h[j] = 0;
	    dg->kroma_codes.ac_h[j] = 0;
	}
    } else {
	if ((dg->hh_fh = open("hist.dat", O_RDONLY | O_BINARY)) == -1) {
	    for (j = 0; j < CODE_TBL_SIZE; j++) {
		dg->luma_codes.dc_h[j] = 0;
		dg->luma_codes.ac_h[j] = 0;
		dg->kroma_codes.dc_h[j] = 0;
		dg->kroma_codes.ac_h[j] = 0;
	    }
	} else {
	    nbytes = read(dg->hh_fh, (char *)dg->luma_codes.dc_h, 12 * 4);
	    nbytes = read(dg->hh_fh, (char *)dg->kroma_codes.dc_h, 12 * 4);
	    nbytes = read(dg->hh_fh, (char *)dg->luma_codes.ac_h, 256 * 4);
	    nbytes = read(dg->hh_fh, (char *)dg->kroma_codes.ac_h, 256 * 4);

	    if (nbytes <= 0) {
		printf("\nHistogram read error");
	    }
	    close(dg->hh_fh);
	}
    }
} /* end of init_cumulative_histogram */



/**********
hardwire_baseline_vars sets variables that should either be
  operator controlled or
  computed analytically
**********/

hardwire_baseline_vars(dg)
    gvl            *dg;
{
    dg->ipata.nd = 2;		/** hardwire to two per baseline **/
    dg->ipata.na = 2;
    dg->ipata.nq = 2;

    dg->dts.spec = dg->d_spec;
    dg->ats.spec = dg->a_spec;
    dg->qts.spec = dg->q_spec;

    dg->ipata.pr_size = 2;	/* should compute analytically from nb       */
    dg->ipata.tq_size = 1;	/* should compute analytically from ipata.nq */
    dg->ipata.td_size = 1;	/* should compute analytically from ipata.nd */
    dg->ipata.ta_size = 1;	/* should compute analytically from ipata.na */
}



/*********
make_coder_quantizers:
	reads custom visibility matrices, if required
	applies factor to generate specific quantizing tables
*********/

make_coder_quantizers(dg)
    gvl            *dg;
{
    short           i, j, k, *snkptr;
    unsigned char   kroma_unsnake_q[64], luma_unsnake_q[64];



    /* */
    if (dg->factor < 1)
	dg->factor = 1;

    /***  generate luma & chroma quantizers  ***/


    if (dg->visi_alt) {
	read(dg->visi_fh, (char *)dg->visibility, 2 * 64 * 2);
	printf("\n Alternate Quantization :           LUMA");
	if (!dg->mono)
	    printf("                        CHROMA");
	dg->vptr1 = &dg->visibility[0];
	dg->vptr2 = &dg->visibility[64];
	for (j = 0; j < 8; j++) {
	    printf("\n                        :");
	    for (k = 0; k < 8; k++)
		printf(" %2X", *dg->vptr1++);
	    if (dg->mono)
		continue;
	    printf("  ");
	    for (k = 0; k < 8; k++)
		printf(" %2X", *dg->vptr2++);
	}
    }
    for (i = 0; i < dg->ipata.nq; i++) {
	for (j = 0; j < 64; j++)
	    dg->qts.data[i][j] = dg->visibility[i * 64 + j];
    }

    snkptr = dg->snake;

    for (i = 0; i < 64; i++) {
	dg->luma_q[i] = (dg->visibility[i] * dg->factor) / 50;
	dg->kroma_q[i] = (dg->visibility[i + 64] * dg->factor) / 50;

	if (dg->luma_q[i] < 2)
	    dg->luma_q[i] = 2;
	if (dg->kroma_q[i] < 2)
	    dg->kroma_q[i] = 2;
	dg->luma_q[i] = dg->luma_q[i] > 0xff ? 0xff : dg->luma_q[i];
	dg->kroma_q[i] = dg->kroma_q[i] > 0xff ? 0xff : dg->kroma_q[i];

	luma_unsnake_q[*snkptr] = dg->luma_q[i];
	kroma_unsnake_q[*snkptr++] = dg->kroma_q[i];
    }



    /***  initialize the selected transform  ***/
    switch (dg->xfm_type) {
    case KTAS:
    case FDCT:
	break;
    case BYP_XFM:
    case BYP_Q:
    case SDCT:
    case QSDCT:
	/**
				initdct( dg ,  8, 8, 16, luma_unsnake_q, kroma_unsnake_q, dg->debug&1 );
	**/
	break;
    }

} /* end of function make_coder_quantizers */



/*****
init_huffman_code_tables
	reads bits array & values represented
	generates code tables
*****/

init_huffman_code_tables(dg)
    gvl            *dg;
{
    int             i, j, k;
    int             bits[17];
    int             l, codeword[256], codesize[256];
#ifdef NeXT
    NXStream       *st;
    st = (NXStream *) _NXOpenStreamOnMacho("__PRIV", "hufftab");
    for (i = 0; i < dg->ipata.nd; i++) {
	NXRead(st, (char *)dg->dts.bits[i], MAX_LDC_CODE_LEN);
	dg->dts.nent[i] = count_bits_tbl(dg, dg->dts.bits[i], MAX_LDC_CODE_LEN);
	NXRead(st, (char *)dg->dts.data[i], dg->dts.nent[i]);
    }


    for (i = 0; i < dg->ipata.na; i++) {
	NXRead(st, (char *)dg->ats.bits[i], MAX_LAC_CODE_LEN);
	dg->ats.nent[i] = count_bits_tbl(dg, dg->ats.bits[i], MAX_LAC_CODE_LEN);
	NXRead(st, (char *)dg->ats.data[i], dg->ats.nent[i]);
    }
    NXClose(st);
#else
    if ((dg->hb_fh = open("hufftab.dat", O_RDONLY | O_BINARY)) == -1)
	report_open_failure(dg, "hufftab.dat");

    for (i = 0; i < dg->ipata.nd; i++) {
	read(dg->hb_fh, (char *)dg->dts.bits[i], MAX_LDC_CODE_LEN);
	dg->dts.nent[i] = count_bits_tbl(dg, dg->dts.bits[i], MAX_LDC_CODE_LEN);
	read(dg->hb_fh, (char *)dg->dts.data[i], dg->dts.nent[i]);
    }


    for (i = 0; i < dg->ipata.na; i++) {
	read(dg->hb_fh, (char *)dg->ats.bits[i], MAX_LAC_CODE_LEN);
	dg->ats.nent[i] = count_bits_tbl(dg, dg->ats.bits[i], MAX_LAC_CODE_LEN);
	read(dg->hb_fh, (char *)dg->ats.data[i], dg->ats.nent[i]);
    }
#endif
} /* end of function init_huffman_code_tables */




/*****
make_huffman_code_tables
	reads bits array & values represented
	generates code tables
*****/

make_huffman_code_table(dg, tableIndex)
    gvl            *dg;
    short           tableIndex;
{
    int             i, j, k;
    int             bits[17];
    int             l, codeword[256], codesize[256];


    /**
    to provide compatibility with KTAS code generator, we'll copy the 
    char bits ,which start at 1 to an int array which starts with 0
    **/
    i = tableIndex;

    for (j = 0; j < 16; j++) {
	bits[j + 1] = dg->dts.bits[i][j];
    }
    bits[0] = dg->dts.nent[i];
    if (i == 0) {
	size_table(dg, bits, dg->hufsi);
	code_table(dg, dg->hufsi, dg->hufco);
	if (dg->cdr_type == N8R5_CDR && dg->codingDirection == LSB_FIRST)
	    reverse_code_table(dg, dg->hufco, dg->hufsi, dg->dts.nent[i]);
	order_codes(dg, dg->dts.data[i], dg->hufco, dg->hufsi,
		    dg->luma_codes.dc_c, dg->luma_codes.dc_n);
    }
    if (i == 1) {
	size_table(dg, bits, dg->hufsi);
	code_table(dg, dg->hufsi, dg->hufco);
	if (dg->cdr_type == N8R5_CDR && dg->codingDirection == LSB_FIRST)
	    reverse_code_table(dg, dg->hufco, dg->hufsi, dg->dts.nent[i]);
	order_codes(dg, dg->dts.data[i], dg->hufco, dg->hufsi,
		    dg->kroma_codes.dc_c, dg->kroma_codes.dc_n);
    }
    for (j = 0; j < 16; j++) {
	bits[j + 1] = dg->ats.bits[i][j];
    }
    bits[0] = dg->ats.nent[i];
    if (i == 0) {
	size_table(dg, bits, dg->hufsi);
	code_table(dg, dg->hufsi, dg->hufco);
	if (dg->cdr_type == N8R5_CDR && dg->codingDirection == LSB_FIRST)
	    reverse_code_table(dg, dg->hufco, dg->hufsi, dg->ats.nent[i]);
	order_codes(dg, dg->ats.data[i], dg->hufco, dg->hufsi,
		    dg->luma_codes.ac_c, dg->luma_codes.ac_n);
    }
    if (i == 1) {
	size_table(dg, bits, dg->hufsi);
	code_table(dg, dg->hufsi, dg->hufco);
	if (dg->cdr_type == N8R5_CDR && dg->codingDirection == LSB_FIRST)
	    reverse_code_table(dg, dg->hufco, dg->hufsi, dg->ats.nent[i]);
	order_codes(dg, dg->ats.data[i], dg->hufco, dg->hufsi,
		    dg->kroma_codes.ac_c, dg->kroma_codes.ac_n);
    }
} /* end of function make_huffman_code_tables */



reverse_code_table(dg, huffcode, huffsize, nent)
    gvl            *dg;
    unsigned        huffcode[], huffsize[], nent;
{
    int             i, j, code;

    for (i = 0; i < nent; i++) {
	code = 0;
	for (j = 0; j < huffsize[i]; j++)
	    if (huffcode[i] & dg->bittest[j + 1])
		code |= dg->bittest[huffsize[i] - j];
	huffcode[i] = code;
    }
}




init_bitstream_header(dg)
    gvl            *dg;
{
    /*** stuff useful & historic info in bitstream header & write header ***/

    dg->hdr_id.len = sizeof(dg->hdr_id) - 1;	/* # of remaining bytes */
    dg->hdr_id.cdr_type = dg->cdr_type;
    dg->hdr_id.xfm_type = dg->xfm_type;
    dg->hdr_id.id1 = HDR_ID1;
    dg->hdr_id.id2 = HDR_ID2;

    dg->bs_hdr.lx_msb = dg->lx >> 8;
    dg->bs_hdr.lx_lsb = dg->lx;
    dg->bs_hdr.ly_msb = dg->ly >> 8;
    dg->bs_hdr.ly_lsb = dg->ly;
    dg->bs_hdr.resync_msb = dg->resync >> 8;
    dg->bs_hdr.resync_lsb = dg->resync;
    dg->bs_hdr.factor_msb = dg->factor >> 8;
    dg->bs_hdr.factor_lsb = dg->factor;
    dg->bs_hdr.factor_msb = dg->factor >> 8;
    dg->bs_hdr.factor_lsb = dg->factor;
    strcpy(dg->bs_hdr.ifn, dg->ifilnam);
    dg->bs_hdr.coder_type = CODER_BASE;
} /* init_bitstream_header */



write_bitstream_header(dg)
    gvl            *dg;
{
    int             i, j, k, temp;
    char           *cptr;
    unsigned char  *uchptr;
    int             table_id;

    /** output our very own id header **/
    cptr = (char *)&dg->hdr_id;
    for (i = 0; i < sizeof(dg->hdr_id); i++)
	sputv(dg, *cptr++, 8);

    /** output fixed portion of JPEG header **/
    cptr = (char *)&dg->bs_hdr;
    for (i = 0; i < sizeof(dg->bs_hdr); i++)
	sputv(dg, *cptr++, 8);

    /** block interleave pattern **/
    sputv(dg, dg->cn - 1, SIZEOF_CN);
    for (i = 0; i < dg->cn; i++) {
	sputv(dg, dg->bip.hivi[i][0] - 1, SIZEOF_HIVI);
	sputv(dg, dg->bip.hivi[i][1] - 1, SIZEOF_HIVI);
    }

    /** interleave prediction & tbl asgn **/

    /* interleave prediction  */
    /* build dc prediction */
    uchptr = dg->ipata.dc_predic_id;
    table_id = 0;
    for (i = 0; i < dg->cn; i++) {
	for (k = 0; k < dg->bip.hivi[i][1]; k++) {
	    for (j = 0; j < dg->bip.hivi[i][0]; j++)
		*uchptr++ = table_id;
	    table_id++;
	}
    }

    /*** compute variable sizes ***/

    temp = 2;
    dg->ipata.pr_size = 1;
    while (temp < dg->nb) {
	dg->ipata.pr_size++;
	temp *= 2;
    }
    temp = 2;
    dg->ipata.tq_size = 1;
    while (temp < dg->ipata.nq) {
	dg->ipata.tq_size++;
	temp *= 2;
    }
    temp = 2;
    dg->ipata.td_size = 1;
    while (temp < dg->ipata.nd) {
	dg->ipata.td_size++;
	temp *= 2;
    }
    temp = 2;
    dg->ipata.ta_size = 1;
    while (temp < dg->ipata.na) {
	dg->ipata.ta_size++;
	temp *= 2;
    }



    /* write dc prediction */
    for (i = 0; i < dg->nb; i++) {
	sputv(dg, dg->ipata.dc_predic_id[i], dg->ipata.pr_size);
    }

    /* quantizer tbl assignment */
    /* build */
    dg->ipata.tq[0] = 0;	/* hardwire */
    dg->ipata.tq[1] = 1;	/* hardwire */
    dg->ipata.tq[2] = 1;	/* hardwire */
    /* write */
    sputv(dg, dg->ipata.nq, SIZEOF_NQ);
    for (i = 0; i < dg->cn; i++) {
	sputv(dg, dg->ipata.tq[i], dg->ipata.tq_size);
    }

    /* DC code tbl assignment */
    /* build */
    dg->ipata.td[0] = 0;	/* hardwire */
    dg->ipata.td[1] = 1;	/* hardwire */
    dg->ipata.td[2] = 1;	/* hardwire */
    /* write */
    sputv(dg, dg->ipata.nd, SIZEOF_ND);
    for (i = 0; i < dg->cn; i++) {
	sputv(dg, dg->ipata.td[i], dg->ipata.td_size);
    }

    /* AC code tbl assignment */
    /* build */
    dg->ipata.ta[0] = 0;	/* hardwire */
    dg->ipata.ta[1] = 1;	/* hardwire */
    dg->ipata.ta[2] = 1;	/* hardwire */
    /* write */
    sputv(dg, dg->ipata.na, SIZEOF_NA);
    for (i = 0; i < dg->cn; i++) {
	sputv(dg, dg->ipata.ta[i], dg->ipata.ta_size);
    }

    /** quantization table specification **/
    sputv(dg, dg->qts.spec, SIZEOF_QTS_SPEC);
    for (i = 0; i < dg->ipata.nq; i++) {
	dg->qts.nent[i] = 64;	/* hardwired */
	if (dg->qts.spec & (1 << (8 * sizeof(dg->qts.spec) - 1 - i))) {
	    for (j = 0; j < dg->qts.nent[i]; j++)
		sputv(dg, dg->qts.data[i][j], 8 * sizeof(char));	/* quant tbl data */
	}
    }
    /** DC code table specification **/
    sputv(dg, dg->dts.spec, SIZEOF_DTS_SPEC);
    for (i = 0; i < dg->ipata.nd; i++) {
	if (dg->dts.spec & (1 << (8 * sizeof(dg->dts.spec) - 1 - i))) {
	    for (j = 0; j < 16; j++)
		sputv(dg, dg->dts.bits[i][j], 8 * sizeof(char));	/* bits  data */
	    for (j = 0; j < dg->dts.nent[i]; j++)
		sputv(dg, dg->dts.data[i][j], 8 * sizeof(char));	/* dc tbl data */
	}
    }
    /** AC code table specification **/
    sputv(dg, dg->dts.spec, SIZEOF_ATS_SPEC);
    for (i = 0; i < dg->ipata.na; i++) {
	if (dg->ats.spec & (1 << (8 * sizeof(dg->ats.spec) - 1 - i))) {
	    for (j = 0; j < 16; j++)
		sputv(dg, dg->ats.bits[i][j], 8 * sizeof(char));	/* bits  data */
	    for (j = 0; j < dg->ats.nent[i]; j++)
		sputv(dg, dg->ats.data[i][j], 8 * sizeof(char));	/* dc tbl data */
	}
    }
} /* end of function write_bitstream_header */





init_more_coder_vars(dg)
    gvl            *dg;
{
    dg->lx2 = dg->lx / 2;
    dg->nb_visi = 0;
    dg->nb_dc_lum_tbl = 0;
    dg->nb_dc_lum_dat = 0;
    dg->nb_ac_lum_tbl = 0;
    dg->nb_ac_lum_dat = 0;
    dg->nb_dc_chr_tbl = 0;
    dg->nb_dc_chr_dat = 0;
    dg->nb_ac_chr_tbl = 0;
    dg->nb_ac_chr_dat = 0;
} /* end of init_vars  */


/*********
Establish pointers to each component block in an interleave block.
Boolean return indicates presence( or absence ) of data.
*********/

int
get_interleave(dg)
    gvl            *dg;
{
    int             i, j, k;	/* */
    /** test for done **/
    if (--dg->num_interleave_blocks_left <= 0) {
	if (dg->display || dg->file_out)
	    put_interleave_strip(dg);

	if (dg->num_lines_processed >= dg->ly)
	    return (0);

	if (!get_interleave_strip(dg))
	    return (0);

	dg->stripno++;
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

    /** fetch pointers to k data **/
    for (j = 0, k = 0; j < dg->vk; j++) {
	for (i = 0; i < dg->hk; i++) {
	    dg->kblk_ptrs[k++] = dg->last_kptr[j];
	    dg->last_kptr[j] += 8;
	}
    }
    return (1);
}







update_histograms(dg)
    gvl            *dg;
{
    if ((dg->hh_fh = open("hist.dat", O_RDWR | O_BINARY | O_CREAT
#ifdef UNIX
			  ,S_IREAD | S_IWRITE
#endif
			  )) == -1) {
	printf("\nOpen failure for histogram file");
	exit(1);
    }
    write(dg->hh_fh, (char *)dg->luma_codes.dc_h, 12 * 4);
    write(dg->hh_fh, (char *)dg->kroma_codes.dc_h, 12 * 4);
    write(dg->hh_fh, (char *)dg->luma_codes.ac_h, 256 * 4);
    write(dg->hh_fh, (char *)dg->kroma_codes.ac_h, 256 * 4);

    close(dg->hh_fh);
} /* end of function update_histograms */


update_bitstream_header(dg)
    gvl            *dg;
{
    sputv(dg, dg->bs_fh, -1);
} /* end of function update_bitstream_header */


print_coder_summary(dg)
    gvl            *dg;
{

    /*
     * dg->imsize = (float)(dg->lx)*dg->ly; dg->totbit = dg->bs_nbytes*8; if(
     * (dg->fbits =(float)dg->totbit/100.) == 0.)  dg->fbits=1.; printf("\n
     * ***********************:"); printf("\n Elapsed time (secs)    :
     * %6ld",dg->stop_t - dg->strt_t); printf("\n Bit Allocation         : 
     * Bits  Bytes      %%"); printf("\n DC Lum Data            : %6ld  %5ld
     * %5.2f",dg->nb_dc_lum_dat,dg->nb_dc_lum_dat/8,(float)dg->nb_dc_lum_dat/d
     * g->fbits); printf("\n AC Lum Data            : %6ld  %5ld
     * %5.2f",dg->nb_ac_lum_dat,dg->nb_ac_lum_dat/8,(float)dg->nb_ac_lum_dat/d
     * g->fbits); printf("\n DC Chr Data            : %6ld  %5ld
     * %5.2f",dg->nb_dc_chr_dat,dg->nb_dc_chr_dat/8,(float)dg->nb_dc_chr_dat/d
     * g->fbits); printf("\n AC Chr Data            : %6ld  %5ld
     * %5.2f",dg->nb_ac_chr_dat,dg->nb_ac_chr_dat/8,(float)dg->nb_ac_chr_dat/d
     * g->fbits); printf("\n Total (Bits Per Pixel) : %6ld  %5ld
     * %5.3f",dg->totbit,(dg->totbit+7)/8,(float)dg->totbit/dg->imsize);
     * printf("\n Quantization Factor    : %6d",dg->factor); printf("\n
     * Number of Overflows    : %6ld",dg->nover); 
     */
    printf("\n Resync interval        : %6d", dg->numberInterleavesPerResync);
    printf("\n Number of syncs        : %6d", dg->resyncCount);
    printf("\n Size of Byte Stream    : %6ld", dg->bs_nbytes);
    printf("\n Elapsed time (secs)    : %6ld", dg->stop_t - dg->strt_t);
    printf("\n Number of non zero AC's: %6ld", dg->nonZeroACs);

} /* end of function print_coder_summary */
