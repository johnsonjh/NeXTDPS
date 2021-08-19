#include "config.h"
#include "vista.h"
#include "vservice.h"
#include "c3sim.h"

#include "newHeader.h"
#include "nxjpeg.h"

typedef	struct 			/** global variable list (gvl) **/
	{
	 long bs_nbytes;				/** byte count maintained by putbits */
     hdr_id_struct hdr_id;			/** Pre JPEG HEADER Identification  **/
     jpeg_fixed_hdr_struct bs_hdr;  /** fixed portion bitstream file header  **/
     jpeg_bip_hdr_struct bip;  /** bip portion bitstream file header  **/
     jpeg_ipata_hdr_struct ipata;  /** ipata portion bitstream file header  **/
     jpeg_qts_hdr_struct qts;  /** qts portion bitstream file header  **/
     jpeg_code_hdr_struct dts;  /** dts portion bitstream file header  **/
     jpeg_code_hdr_struct ats;  /** ats portion bitstream file header  **/
     code_table_struct luma_codes;
     code_table_struct kroma_codes;
     hds	huf_decode_struct[MAX_NUM_CODE_TBLS][2];
	 int   ldc_valptr[33];
	 int   ldc_mincode[33];
	 int   ldc_maxcode[33];
	 int   kdc_valptr[33];
	 int   kdc_mincode[33];
	 int   kdc_maxcode[33];
	 int   lac_valptr[33];
	 int   lac_mincode[33];
	 int   lac_maxcode[33];
	 int   kac_valptr[33];
	 int   kac_mincode[33];
	 int   kac_maxcode[33];
 	 unsigned char *y_strip, *u_strip, *v_strip, *k_strip;
	 int  *y_xstrip,  *u_xstrip,  *v_xstrip, *k_xstrip;
	 int  *y_qstrip,  *u_qstrip,  *v_qstrip, *k_qstrip;
	 int  *y_dqstrip, *u_dqstrip, *v_dqstrip,*k_dqstrip;
	 unsigned char *y_ptr, *u_ptr,  *v_ptr, *k_ptr, *lineptr;
	 int *yline, *uline, *vline, *kline;
	 char *rgbline;
	 long  strt_t;
	 long  stop_t;
	 unsigned char *yblk_ptrs[16];
 	 unsigned char *ublk_ptrs[16];
 	 unsigned char *vblk_ptrs[16];
	 unsigned char *kblk_ptrs[16];
 	 unsigned char *last_yptr[4];
 	 unsigned char *last_uptr[4];
 	 unsigned char *last_vptr[4];
 	 unsigned char *last_kptr[4];
	 unsigned char *blkptr, blk[64];
	 int  *y_xptr, *u_xptr,   *v_xptr, *k_xptr;
 	 int  *y_qptr, *u_qptr,   *v_qptr, *k_qptr;
	 int  *y_dqptr, *u_dqptr, *v_dqptr,*k_dqptr;
	 int prev_ydc[4], prev_udc[2], prev_vdc[2], prev_kdc[2];  /** 1D DC predictors  **/
	 int num_lines_processed;
 	 int num_interleave_blocks_left;
	 int num_interleave_blocks_per_strip;
	 int num_interleaves_processed;
	 short bitmap[256]; 
	 unsigned short visibility[128];
	 short snake[64];
	 unsigned int luma_q[64], kroma_q[64];  /** two fixed quantizers  **/
													/** These are filled by
														 visibility * factor / 50  **/
											/** Image size **/
	 unsigned int lx;				/* # of pixels per line */
	 unsigned int ly;				/* # of lines */
	 unsigned int lx_u;
	 unsigned int lx_v;
	 unsigned int padx_y;
	 unsigned int padx_u;
	 unsigned int padx_v;
	 unsigned int num_lines_out;
											/** Image format, TARGA or NHD (KTAS style) **/
	 int in_frame;			/* input image format */
	 int out_frame;			/* output image format */
	 int pixel_size_in;				/* Number of bytes per pixel input */
	 int pixel_size_out;			/* Number of bytes per pixel output */
	 int datatype;				/* TARGA header variable, defines RGB or mono */
 										/** input controls **/
	 unsigned int getbit_src;		/* Switch between file & pointer    */
	 unsigned int getbit_src_format;	/* Switch between NuBus Ascii & Bin */
 											/** output controls **/
	 unsigned int file_out;		/* Enable recon image output         */
	 unsigned int display;			/* Enable recon to a display board   */
	 unsigned int vlsi_out;		/* Enable cdr outputs , zz, cdr, cma */
	 unsigned int proto_out;		/* Enable cdr outputs , zz, cdr, cma */
	 unsigned int raw_out;			/* Enable raw pixel ascii output     */
											/** processing controls **/
	 int mono;						/* force to cn=1,hy=nb=1,hu=vu=hv=vv=0     6/27/89  */
	 int four44;					/* cn=3,hy=vy=hu=hv=vu=vv=1                7/20/89  */
	 int four22;					/* cn=3,hy=2, vy=hu=hv=vu=vv=1             7/20/89  */
	 int four11;					/* cn=3,hy=2, vy=2, hu=hv=vu=vv=1          7/20/89  */

	 int hk;						/* # of horizontal y blocks per interleave */
	 int vk;						/* # of vertical y blocks per interleave   */
	 int hy;						/* # of horizontal y blocks per interleave */
	 int vy;						/* # of vertical y blocks per interleave   */
	 int hu;						/* # of horizontal u blocks per interleave */
	 int vu;						/* # of vertical u blocks per interleave   */
	 int hv;						/* # of horizontal v blocks per interleave */
	 int vv;						/* # of vertical v blocks per interleave   */
	 int nb;						/* # of       all  blocks per interleave   */
	 int cn;						/* # of color components                   */
	 int q_spec;				/* quantization tbl specification          */
	 int d_spec;				/* DC code      tbl specification          */
	 int a_spec;				/* AC code      tbl specification          */

	 int resync;					/* Resync flag/# interleave blocks per sync*/
	 int master_sync_counter;
	 int factor;				/* Quantization Scale Factor */
	 int hc_init;				/* generate flat dist codes, else read from file */
	 int hh_init;				/* clear histogram counters, else read init values */
	 int visi_alt;				/* get alternate visibility matrices thru filename */
											
	 int fdct_record;			/* Transform output flag*/
	 int eqdct_record;			/* Quantizer output flag*/
	 int dqdct_record;			/* Dequantizer output flag*/
											/* following output image filename */
	 int debug;

	 char ifor[8][32];
	 int pixel_format_in;			/* describes RGB layout for a pixel, values in c3sim.h */
	 int pixel_format_out;
	 int pixel_format_array[6];/* this array NOT ROBUST */
	 int pctl;
	 int bno, bnok;
	 int str, stp; 	/* block bounds for debug, switch selectable */
	 
	char vlsi_processing_states[8][32];
	 
	 /*** Resync Variables ***/
	int		resyncCount;
	int		InterleaveCount;
	int		numberInterleavesPerResync;
	
		/*** number of bit counters for various categories ***/
	 unsigned long int nb_dc_lum_tbl,nb_dc_lum_dat,nb_dc_chr_tbl,nb_dc_chr_dat;
	 unsigned long int nb_ac_lum_tbl,nb_ac_lum_dat,nb_ac_chr_tbl,nb_ac_chr_dat;
	 unsigned long int nb_visi;

	 float imsize, fbits;		/**  whatnots used in print summary **/

	 unsigned int lx2;
	 int resync_count;
	 int stripno;
	 long int nover;  /** counts overflows **/
	 char ifilnam[128], ofilnam[128],packedfile[128];
	 char bs_fnam[128];
	 char visifn[128];
	 hdstruct hdr; /* Header for Truevision type files */
	 int ifh, ofh;			/* image input & output */
	 int hh_fh, hc_fh;		/* histogram file & Huffman code  file */
	 int len_hist_fh;		/* code length histogram file */
	 int fifo_hist_fh;		/* fifo histogram file */
	 int hb_fh;				/* Huffman code  bits array file */
	 int bs_fh;				/* bit stream */
	 int hufbs_fh;
	 int visi_fh;			/* alternate visibility matrices source */
	 int byp_fh;				/* transform bypass file */
	 int fdct_record_fh;		/* Transform output */
	 int eqdct_record_fh;		/* Quanizer output */
	 int dqdct_record_fh;		/* Dequantizer output */
	 FILE *zz_fp;			/* sim hw zigzag ascii output   6/29/98 */
	 FILE *cdr_fp;			/* sim hw coder ascii output   6/29/98 */
	 FILE *rgbin_fp;		/* sim hw rgb input   6/29/98 */
	 FILE *rgbout_fp;		/* sim hw rgb output   6/29/98 */
	 FILE *dcr_fp;			/* sim hw coder ascii output   6/29/98 */
	 FILE *cma_fp;			/* sim hw coder ascii output w/ comma   7/14/98 */
	 FILE *raw_fp;			/* raw pixel output in ascii per vlsi   8/14/98 */
	 FILE *rec_fp;			/* recon pixel output in ascii per vlsi         */
	 FILE *nb_fp;			/* ascii Nubus data input */
	 int bittest[17];
	 int signmask[16];
	 int origlx, origly;
	 int notch;					/* notch @ bottom of each displayed strip*/
	 int line_pos, line_lim;
	 int x0,y0_i, dx,dy, border;
	 unsigned int zigzag_out;/* Zigzag ascii output */
	 int xfm_type;			/* transform type */
	 int cdr_type;			/* coder type */
	 unsigned int coder_out;	/* Coder ascii output */

	 char xtyp[10][32];
	 char ctyp[4][32];
	 
	 
	 /*** histogram arrays ***/
	 unsigned long codeLengthHisto[17];
	 unsigned long compressedFifoHisto[32];
	 unsigned long rawFifoHisto[512];
	 
	 Boolean enbFifoHistoOuto;
	 
	 int fifoEntryCount;
	 int lastCompressedFifoEntryCount;
	 int lastRawFifoEntryCount;


	 unsigned int	cdr_nbytes;
	 char			cdr_prev_word, cdr_cur_word;
	 int			cdr_nwords;
	 int			cdr_nblks;		/* block sequenctial id for cma_out */
	 char			cdr_bfr[520], *cdr_ptr, *cdr_bfr_end;
	 int			cdr_unused;
	 int			codingDirection;	/* msb fisrt or lsb first ... cdr_out only */
	 
     int check_marker, marker_detected;
	 char *dcr_bs_bfr;
	 image_report_struct ImageReport;
	 int sampling;
     int  blk_no;
	 char version[64];
	 char txt[80];
	 int txt_x, txt_y, txt_len;
	 int   decoder_hufsi[ CODE_TBL_SIZE+1 ];
	 char  *decoder_pa[100];
	 char  *coder_pa[100];
	 FILE *hufutil_log_file;		/* debug info logging file, if any */
	 int hufutil_dbug_flag;		/* if set non-zero, logging is also printed */
	 unsigned int      code;
	 unsigned int	   hufcode;		/** static used in hufutil.c **/
	 int             *bits;
	 int             *codesize;
	 int             *others;
	 long int        *freq;
	 long int        *freq_copy;
	 int             *huffsize;
	 int             *huffcode;
	 int             *hufco;
	 int             *hufsi;
	 unsigned char   *huffval;
	 int             lastp;
	 int             valptr[16];
	 int             mincode[16];
	 int             maxcode[16];
	 int             ms;
	 int             time_for_resync;
	 int             time_for_2nd_resync;
	 int sync_decoded_counter;
	 int wmask[17];
	 int unused ;					/** # unused bits @ head of bitstream **/
     char *bs_bfr, *bs_ptr, *bs_bfr_end;
	 code_table_struct	*cts;			/** allocate space in initialization **/     
	 int rgb_ready;	/** flag maintained by put_interleave_strip, used by display_if_possible **/
	 unsigned char *u_ptr2,  *v_ptr2;
	 unsigned int r1n, g1n, b1n, u1n, v1n;
	 unsigned int yn, un, vn;
	 unsigned int u2n, u3n, v2n, v3n;
	 unsigned int rgbn;
	 unsigned int nmsk[17];
	 unsigned int nrnd[17];
     unsigned int r1,g1,b1,dr,dg,db;
	 int signbit;
	 int signbit_char;
	 int pmode;
	 hdstruct vuhdr;
	 int vunpels_i, vuraster_size_i;
	 int vunpels_o, vuraster_size_o, vunlines_o;
	 int vuifh, vuofh;
	 int vuifh2, vunpels_i2, vuraster_size_i2;		/** vutil statics **/
	short xfzigzag[64];
	 int xfdebug;
	 int coorddebug;				/** static debug in coord **/
	 int qscoef1[16];
	 int qsfcoef2[2][16][16];
	 int qsicoef2[2][16][16];
	 int qsqshift[2][16][16];
	 unsigned int qsmask;
	 unsigned int sh8[8];
	 unsigned int sh16[16];
	 unsigned char q[64];
	 int	initdct_flag;
	 void (*refresh_fun)();		/* ptr to function to refresh the bitstream */
	 char *rbuffer;
	 char *gbuffer;
	 char *bbuffer;
	 long totbit;		/** init to 0 **/
	 int  help;			/** init to 0 **/
	 char recon_filnam[128];		/** 8/22/89 j?e **/
	 char zz_filnam[32];
	 char cdr_filnam[32];
	 char raw_filnam[32];
	 char rec_filnam[32];
	 char cma_filnam[32];
	 char dcr_filnam[32];
	 char byp_filnam[32];
	 
	 char channelInputFileName[MAX_SCANS][132];
	 char channelOutputFileName[MAX_SCANS][132];
	 
	 char channelFDCTFileName[MAX_SCANS][132];
	 char channelEQDCTFileName[MAX_SCANS][132];
	 char channelDQDCTFileName[MAX_SCANS][132];
	 char channelBypassFileName[MAX_SCANS][132];
	 
	 short xcs_sync;			/* # of sync 1's in excess of 16 */
 	 unsigned short *vptr1, *vptr2;	
 	  
 			/** 11/29/1989. Vahid. **/
 	 jPEGFxdFrameHdr	JPEGFxdFrame;
 	 jPEGFrameComponentSpec	frameComponentSpec;
 	 jPEGQuantMatAssign	qTable;
 	 jPEGScanHdr		JPEGScanHdr;
 	 jPEGFrameStruct	JPEGFrame[10];
 	 jPEG2FrameStruct	JPEG2Frame[10];
 	 short				interleavingFormat;
 	 short				JPEGFrameCount;
 	 short				lumaChromaFlag;
 	 int				*qTableToUse;
 	 short				mCode;
 	 int				*imageQTables[5];
 	 long				nonZeroACs;
 	 }	gvl;



typedef		struct
	{
	short		currentPlane;
	short		currentRow;
	short		currentCol;	
	short		planeCount;
	short		fRefNum;
	} middleManData;


#ifdef UNIX
typedef unsigned short Point[2];
typedef unsigned char *Ptr;
#endif


typedef struct
	{
	char	*decoderFileName;		/** name of the file to decompress **/
	int		outputFormat;			/** 1 plane at a time, or 1 pixel(RGB) at a time **/	
	short	statusFlag;				/** 1 means valid, 0 means invalid **/
	short	moreToGoFlag;			/** set to zero at the end of the image **/
	short	numbOfLinesDecoded;		
	Point	totalImageSize;			/** # of pixels per line, and the total # of lines **/
	Ptr		rgbData;
	} ioSharedRec;
	
	

typedef struct
	{
	middleManData	*middleMan;
	gvl				*dg;
	ioSharedRec		*sharedRec;
	} middleManDecoderInterface;
	
	
	
