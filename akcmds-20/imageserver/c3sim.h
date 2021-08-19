/**

This is valuable proprietary information and trade secrets of Creative Circuits
Corporation and is not to be disclosed or released without the prior written
permission of an officer of the company.

Copyright (c) 1989 by Creative Circuits Corporation.

c3sim.h


Modification history
Date		Name	Summary
6/14/89	JD		1. Chg Resync interval from one byte to two bytes
					2. Del Mac_overhead (one byte to make even) from header
					3. Del old (Prior to 1 May) header struct
6/27/89	JD		1. Add Transform types
**/

unsigned int gethuf();
unsigned int getbit();


/** vlsi processing states **/
#define VLSI_RGB2YUV422	1
#define VLSI_422		2
#define VLSI_MONO		3
#define VLSI_444to422	4
#define VLSI_4444		5
#define VLSI_411		6

/**  video frame types  **/
#define NHD 0
#define TARGA 1
#define CST 2
#define ABEKUS_YUV_FRAME 3
#define COST_ONE_CHANNEL 5
#define ABEKUS_BUFFERED 6
#define MONOCHROME_NHD 7


/**   conversion identification  **/
#define RGB2YIQ 0
#define YIQ2RGB 1
#define RGB2YUV 2
#define YUV2RGB 3
#define CCIR2YUV 4
#define YUV2CCIR 5
#define YUV2MONO 6
#define MONO2YUV 7
#define RGB2YUV422		8
#define YUV2RGB422		9
#define RGB2YUV444		10
#define YUV2RGB444		11
#define CYMK2YUV4444	12
#define YUV2CYMK4444	13
#define RGB2YUV444d8	14
#define YUV2RGB444d8	15

/**   conversion formats  **/
#define RGB16		0
#define RGB24		1
#define RGB32		2
#define RGB			3
#define M8			4
#define REALBGR		5
#define GRAYSCALE	6

/**    outdated coder types **/
#define CODER_UNDF 0
#define CODER_BASE 1
#define CODER_KTAS 2

/**   transform types **/			/**  6/27/89  **/
#define KTAS 1
#define SDCT 2
#define QSDCT 3
#define FDCT 4
#define BYP_XFM 5
#define THOM 6
#define DPDCT 7
#define BYP_Q 8
#define XFM_TYPE_MIN 1
#define XFM_TYPE_MAX 8

/**   coder types   **/				/** 8/8/89 **/
#define VLSI_CDR 1
#define PROTO_CDR 2
#define N8R5_CDR 3
#define CDR_TYPE_MIN 1
#define CDR_TYPE_MAX 3

/**   Sampling Rates   **/
#define S411 1
#define S422 2
#define S444 3
#define MONO 4
#define SOTHER 5
#define S4444 6

/**   GetBit Options  **/
#define GETBIT_FILE 1
#define GETBIT_PTR  2
#define GNUBUS_ASCII 1
#define GNUBUS_BIN 2

/** Coding direction **/
#define LSB_FIRST 1
#define MSB_FIRST 2


/****  R6 Marker codes  ****/

#define 	SOF_MARKER 				0xC0
#define		DHT_MARKER				0xC4
#define 	RST_MARKER 				0xD0
#define 	SOS_MARKER 				0xDA
#define		DQT_MARKER				0xDB
#define		NLO_MARKER				0xDC
#define 	SOI_MARKER 				0xD8
#define 	EOI_MARKER 				0xD9
#define 	APP_MARKER 				0xE0
#define 	COM_MARKER 				0xFE
#define 	FIL_MARKER 				0xFF


/**   code table information  **/
		/* four tables each of max 256 entries each */
		/* DC LUM, AC LUM, DC KROM, AC KROM         */
#define TBL_FLAT 0					/* tables are flat,           but present */
#define TBL_STD1 1					/* tables are standard set 1, but present */
#define TBL_STD2 2					/* tables are standard set 2, but present */
#define TBL_STD3 3					/* tables are standard set 3, but present */
#define TBL_STD4 4					/* tables are standard set 4, but present */
#define TBL_NSTD 5					/* tables are NON standard  and present */


/****   BASELINE CODER constants   ****/
#define RL_ESC 248					/* coder ikndex for run_length escapes */
#define MAX_LDC_CODE_LEN 16
#define MAX_KDC_CODE_LEN 16
#define MAX_LAC_CODE_LEN 16
#define MAX_KAC_CODE_LEN 16

#define MAX_INTRLV_BLKS 4

/****   JPEG baseline bit stream header structure   ****/

#define MAX_NUM_CODE_TBLS  2
#define MAX_HV 4		/* */
#define MAX_CN 3		/* # image components */
#define MAX_NQ 2		/* # quant matrices */
#define MAX_ND MAX_NUM_CODE_TBLS		/* # DC code tbls */
#define MAX_NA MAX_NUM_CODE_TBLS		/* # AC code tbls */
#define MAX_NB 7		/* total # blocks in an interleave */
#define SIZEOF_CN 4
#define SIZEOF_HIVI 2
#define SIZEOF_NQ 3
#define SIZEOF_ND 3
#define SIZEOF_NA 3
#define SIZEOF_QTS_SPEC 8
#define SIZEOF_DTS_SPEC 8
#define SIZEOF_ATS_SPEC 8

#define DC_BITS_TABLE_SIZE 16
#define AC_BITS_TABLE_SIZE 16

typedef struct 								/** id header prior to JPEG hdr **/
{
	char len;								/* # remaining bytes in hdr_id */
	char cdr_type;
	char xfm_type;
	char id1;
	char id2;
}hdr_id_struct;

#define HDR_ID1 0x5A
#define HDR_ID2 0x7E
typedef struct 						/** Initial Fixed Portion  of JPEG Hdr **/
{
	char ifn[32];									/* JPEG length undefined */
	unsigned char coord_sys_id;
	unsigned char ly_msb;
	unsigned char ly_lsb;
	unsigned char lx_msb;
	unsigned char lx_lsb;
	unsigned char coder_type;
	unsigned char resync_msb;
	unsigned char resync_lsb;
	unsigned char factor_msb;
	unsigned char factor_lsb;
}jpeg_fixed_hdr_struct;

/* block interleave pattern ...allocating fixed fields... open question */
typedef struct 
{
	unsigned char cn;
	unsigned char hivi[MAX_CN][2];	/* [j][0] - # h blks for jth component */
												/* [j][1] - # v blks for jth component */
}jpeg_bip_hdr_struct;

/* interleave prediction and table assignment */
typedef struct 
{
	int pr_size;
	unsigned char dc_predic_id[MAX_NB];
	int tq_size;
	int nq;
	unsigned char tq[MAX_CN];
	int td_size;
	int nd;
	unsigned char td[MAX_CN];
	int ta_size;
	int na;
	unsigned char ta[MAX_CN];
}jpeg_ipata_hdr_struct;
	
/* quantization matrix specification */
typedef struct 
{
	unsigned char spec;
	int nent[2];
	unsigned char data[2][64];
}jpeg_qts_hdr_struct;


	
/* code table specification */
typedef struct 
{
	unsigned char spec;
	unsigned char bits[2][16];		/* one for Table0, one for Table1 */
	int nent[2];
	unsigned char data[2][256];
}jpeg_code_hdr_struct;
	
	

/******  CODE TABLE STRUCTURE  ******/

#define CODE_TBL_SIZE 256

typedef struct 
{
	unsigned int dc_c[CODE_TBL_SIZE];							/** dc code bits **/
	unsigned int dc_n[CODE_TBL_SIZE];							/** number of code bits **/
	unsigned int ac_c[CODE_TBL_SIZE];							/** ac code bits **/
	unsigned int ac_n[CODE_TBL_SIZE];
	unsigned  long dc_h[CODE_TBL_SIZE];						/** histogram **/
	unsigned  long ac_h[CODE_TBL_SIZE];
}code_table_struct;


/*----------put in by vahid 6/5/1989------------*/
	typedef struct					/*  Huffman Decode Structure  */
	  {
	  		unsigned char *huffval;
	  		unsigned char *huffbits; 
			int           *mincode;
		   int           *maxcode;
		 	int           *valptr;
			int            biggest;
	  } hds	;
	  


/*****  INTERMODULE COMMUNICATION STRUCTURE   *****/
typedef struct 
{
	int NumberOfLines;
	int PixelsPerLine;
	int CompressedSize;
	int PixelSize;
	int Factor;
	long ElapsedTime;
	int CoderType;		/* {VLSI, PROTO} */
	int XformType;		/* {KTAS, FDCT, SDCT, QSDCT, BYPASS}*/
	int Sampling;		/*  {S422, S411, S444, MONO, SOTHER}*/
}image_report_struct;
