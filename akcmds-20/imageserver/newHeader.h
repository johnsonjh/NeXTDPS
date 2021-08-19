

/****  R5 codes... find new R6 code definitions in c3sim.h

#define 	PRV_MARKER 				0xc0
#define 	APP_MARKER 				0xd0
#define 	SOF_MARKER 				0xe0
#define 	RSC_MARKER 				0xf0
#define 	SOS_MARKER 				0xf8
#define 	LCT_MARKER 				0xf9
#define 	SOI_MARKER 				0xfc
#define 	EOI_MARKER 				0xfe
#define 	FIL_MARKER 				0xff
****/




#define		BLOCK_INTERLEAVE		1
#define		COMPONENT_INTERLEAVE	2


#define		MAX_COMP		4
#define		MAX_SCANS		4


#define		Y_COMP			0
#define		U_COMP			1
#define		V_COMP			2
#define		UV_COMP			3
#define		YUV_COMP		4


typedef struct
	{
	short	frameNumbOfQMatrices;	
	short	frameNumbOfComponents;
	short	frameQuantMatrixId[MAX_COMP];	/** Q matrices to use with frame **/
	short	frameComponentId[MAX_COMP];
	short	scanCount;				/** # of scans within the frame **/
	short	scanComponentCount[MAX_SCANS];
	short	componentHorBlocksPerIntrlv[MAX_SCANS][MAX_COMP];	
	short	componentVertBlocksPerIntrlv[MAX_SCANS][MAX_COMP];
	short	scanComponent[MAX_SCANS][MAX_COMP];
	short	scanComponentHSize[MAX_SCANS][MAX_COMP];
	short	scanComponentVSize[MAX_SCANS][MAX_COMP];
	short	scanNumbOfCodeTables[MAX_SCANS];			
	short	scanCodeTableId[MAX_SCANS][MAX_COMP];
	char	*scanInputFileName[MAX_SCANS];			
	char	*scanOutputFileName[MAX_SCANS];			
	char	*scanFDCTFileName[MAX_SCANS];
	char	*scanEQDCTFileName[MAX_SCANS];
	char	*scanDQDCTFileName[MAX_SCANS];
	char	*scanBypassFileName[MAX_SCANS];
	short	scanType[MAX_SCANS];
	} jPEGFrameStruct;



typedef struct
	{
	short	resyncInterval;
	short	frameNumbOfQMatrices;	
	short	frameNumbOfComponents;
	short	frameQuantMatrix[MAX_COMP];	/** Q matrices to use with frame **/
	short	frameComponentId[MAX_COMP];
/*	short	scanCount;				/** # of scans within the frame **/
	short	scanNumbOfComponents[MAX_SCANS];
	short	componentHorBlocksPerIntrlv[MAX_COMP];	
	short	componentVertBlocksPerIntrlv[MAX_COMP];
	short	scanComponentId[MAX_SCANS];
	short	scanComponentHSize[MAX_COMP];
	short	scanComponentVSize[MAX_COMP];
	short	scanNumbOfCodeTables[MAX_SCANS];			
	short	scanACCodeTableId[MAX_SCANS][MAX_COMP];
	short	scanDCCodeTableId[MAX_SCANS][MAX_COMP];
	char	*scanInputFileName[MAX_SCANS];			
	char	*scanOutputFileName[MAX_SCANS];			
	char	*scanFDCTFileName[MAX_SCANS];
	char	*scanEQDCTFileName[MAX_SCANS];
	char	*scanDQDCTFileName[MAX_SCANS];
	char	*scanBypassFileName[MAX_SCANS];
	short	scanType[MAX_SCANS];
	short	componentType;
	} jPEG2FrameStruct;


#define sizeof_JPEGFxdFrame 7

typedef struct
	{
	short		signalingLength;
	char		dataPrecision;
	char		numbOfLinesHB;
	char		numbOfLinesLB;
	char		lineLengthHB;
	char		lineLengthLB;
	} jPEGFxdFrameHdr;
	
	
	
typedef struct
	{
	short		length;
	char		compCount;
	char		componentID1;
	char		h1v1;
	char		qTable1;
	char		componentID2;
	char		h2v2;
	char		qTable2;
	char		componentID3;
	char		h3v3;
	char		qTable3;
	char		componentID4;
	char		h4v4;
	char		qTable4;
	} jPEGFrameComponentSpec;	/** maximum components are 4 **/
	


typedef struct
	{
	short			qTableCount;
	unsigned int	*qTablePtr[5];
	} jPEGQuantMatAssign;
	

typedef struct
	{
	short		signalingLength;
	char		compCount;
	char		componentID1;
	char		codeTable1;
	char		componentID2;
	char		codeTable2;
	char		componentID3;
	char		codeTable3;
	char		componentID4;
	char		codeTable4;
	char		spectralSelectStart;
	char		spectralSelectEnd;
	char		successiveApproxBits;
	} jPEGScanHdr;
	
