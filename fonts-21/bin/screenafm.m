

/*
	screenafm.m
	creates screen font afm files from a printer
	font afm file and a list of Bitmap Distribution
	files.
	
	Created 04Jun88 Leovitch
	
	CONFIDENTIAL
	Copyright(c) 1988 NeXT, Inc. as an unpublished work
	ALL RIGHTS RESERVED
	
	
	Modified:
	12Jan89 Leo  	Had to make it a .m for objc includes
	15Jun89 Leo  	Name changed to screenafm
	 4May90 pgraff 	add fix to ignore trailing newlines in names.


*/


#include <stdio.h>
#include "font.h"
extern int errno;
extern char *malloc(),*getenv();
static char *myname = "screenafm";

#define AFMPATH ".:../afm:~/Library/Fonts/afm:/LocalLibrary/Fonts/afm:" \
	"/NextLibrary/Fonts/afm"
#define PRINTERFONT (argv[1])
#define AFMSUFFIX ".afm"
#define SCREENPREFIX "Screen-"
#define ENCODINGLENGTH 256 /* Length that kit font stuff assumes fonts are */
#define CHARARRAYLENGTH 512 /* Extra space for unencoded chars */

extern void processBitmapFile(),WriteScreenAfm(),
					WriteAfmHeader(); /* forward */
static int ReadBitmapFile(); /* forward */
extern NXFontMetrics *DPSReadFontInfo(); /* from afm.m */

static int ReadPrinterAfmFile(pathName,metrics)
char *pathName;
NXFontMetrics *metrics;
{
    FILE *printerAfm;
    
    if ((printerAfm = fopen(pathName,"r")) == NULL)
    	return(-1);
    if (DPSReadFontInfo(metrics,NX_FONTHEADER|NX_FONTMETRICS|NX_FONTCHARDATA,
	    printerAfm) == NULL)
	{
	    fclose(printerAfm);
	    return(-2);
	}
    fclose(printerAfm);
    return(-20);
} /* ReadPrinterAfmFile */

main(argc,argv)
int	argc;
char	*argv[];
{
    int		i;
    char	printerFontFile[128];
    FILE	*printerAfm;
    NXFontMetrics printerMetrics;
    
    
    if (argc < 3)
    {
    	fprintf(stderr,"%s: Usage is '%s <fontname> <bitmapfiles>'.\n",
		myname,myname);
	exit(-1);
    }
    bzero(&printerMetrics,sizeof(printerMetrics));
    strcpy(printerFontFile,PRINTERFONT);
    strcat(printerFontFile,AFMSUFFIX);
    switch(NXFilePathSearch(NULL,AFMPATH,1,printerFontFile,ReadPrinterAfmFile,
    	&printerMetrics))
    {
    case 0:
    	fprintf(stderr,"%s: Could not find printer afm file for %s!\n",
			myname,PRINTERFONT);
	exit(-1);
    case -1:
    	fprintf(stderr,"%s: Opening printer afm file",myname);
	perror("");
	exit(-1);
    case -2:
    	fprintf(stderr,"%s: Could not parse printer afm file for %s!\n",
		myname,PRINTERFONT);
	exit(-1);
    default:
    	break;
    } /* switch */

    /* Now process each bitmap widths file in turn */
    for(i=2;i < argc;i++)
    	processBitmapFile(&printerMetrics,PRINTERFONT,argv[i]);

    exit(0);
} /* ScreenAfm */

static void processBitmapFile(printerMetrics,printerFont,bitmapFileName)
NXFontMetrics *printerMetrics;
char	*printerFont,*bitmapFileName;
{
    FILE	*bitmapFile,*outputFile;
    NXFontMetrics bitmapMetrics;
    char	outFileName[128];
    char	screenFontName[128],screenFullName[128];
    char	fileLine[128],token[64];
    int		fontSize,numChars,j,k;

    /* Make complete writable copy of printer metrics */
    bitmapMetrics = *printerMetrics;
    bitmapMetrics.widths = (float *)malloc(sizeof(float)*CHARARRAYLENGTH);
    for(j=0;j<CHARARRAYLENGTH;j++) bitmapMetrics.widths[j] = 0.0;
    bitmapMetrics.charData = (NXCharData *)
    				malloc(sizeof(NXCharData)*CHARARRAYLENGTH);
    bcopy(printerMetrics->charData,
    		bitmapMetrics.charData,sizeof(NXCharData)*ENCODINGLENGTH);
    /* Open bitmap file, read header */
    if ((bitmapFile = fopen(bitmapFileName,"r")) == NULL)
    {
	fprintf(stderr,"%s:Cannot open bitmap file %s:",myname,bitmapFileName);
	perror("");
	fprintf(stderr,"(continuing)\n");
	return;
    }
    j = ReadBitmapFile(bitmapFile,&bitmapMetrics,&fontSize);
    fclose(bitmapFile);
    if (j == -1)
    {
    	fprintf(stderr,"(in file %s)\n",bitmapFileName);
	return;
    }
    /* Figure some new information for the header */
    sprintf(fileLine,".%d",fontSize);
    strcpy(screenFontName,SCREENPREFIX);
    strcat(screenFontName,printerFont);
    strcpy(screenFullName,screenFontName);
    strcat(screenFullName,fileLine);
    bitmapMetrics.name = screenFontName;
    bitmapMetrics.fullName = screenFullName;
    bitmapMetrics.underlinePosition *= fontSize;
    bitmapMetrics.underlineThickness *= fontSize;
    bitmapMetrics.capHeight *= fontSize;
    bitmapMetrics.xHeight *= fontSize;
    bitmapMetrics.ascender *= fontSize;
    bitmapMetrics.descender *= fontSize;
    /* FIX!! Add isScreenFont and screenFontSize to font.h and set here */
    /* Now begin to write the new afm file */
    strcpy(outFileName,screenFullName);
    strcat(outFileName,AFMSUFFIX);
    if ((outputFile = fopen(outFileName,"w+")) == NULL)
    {
    	fprintf(stderr,"%s: Cannot open file %s for write:",myname,
		outFileName);
	perror("");
	fprintf(stderr,"(continuing)\n");
	fclose(bitmapFile);
	return;
    }
    WriteScreenAfm(outputFile,&bitmapMetrics,fontSize);
    fclose(outputFile);
    /* Alright! We're outa here */
    free(bitmapMetrics.widths);
    free(bitmapMetrics.charData);
} /* processBitmapFile */

static int ReadBitmapFile(inF,metrics,sizeP)
FILE	*inF;
NXFontMetrics *metrics;
int	*sizeP;
{
    int numChars,numUnencoded;
    char fileLine[128],token[64],nameBuffer[64];

    /* Read in all the header information from the bitmap file */
    numChars = numUnencoded = 0;
    while(!numChars)
    {
    	if (fgets(fileLine,128,inF) == NULL)
	{
	    fprintf(stderr,
	    	"%s:End-of-file reading bitmap file.\n(Continuing)\n",
	    	myname);
	    return(-1);
	}
	sscanf(fileLine,"%[^ ,.\r\n]",token);
	if (strcmp(token,"SIZE")==0)
	{
	    sscanf(fileLine,"%*s %d",sizeP);
	}
	else if (strcmp(token,"FONTBOUNDINGBOX")==0)
	{
	    sscanf(fileLine,"%*s %f %f %f %f",
	    	&metrics->fontBBox[2],&metrics->fontBBox[3],
		&metrics->fontBBox[0],&metrics->fontBBox[1]);
	}
	else if (strcmp(token,"CHARS")==0)
	{
	    sscanf(fileLine,"%*s %d",&numChars);
	}
    }
    /* Read the bitmap file, storing info in bitmapMetrics */
    {
    	int	curChar,charCount;
	
	charCount = numChars;
	curChar = -1;
	while(charCount)
	{
	    if (fgets(fileLine,128,inF) == NULL)
	    {
		fprintf(stderr,
		    "%s:End-of-file reading bitmap file.\n(Continuing)\n",
		    myname);
		return(-1);
	    }
	    if (sscanf(fileLine,"%[^ ,.\r\n]",token) == 1)
		if (strcmp(token, "STARTCHAR") == 0)
		{
		    sscanf(fileLine,"%*s %[^ ,.\r\n]",nameBuffer);
		}
	    	else if (strcmp(token,"ENCODING")==0)
		{
		    curChar = -1;
		    sscanf(fileLine,"%*s %d",&curChar);
		    if (curChar == -1)
		    {
		    	curChar =  ENCODINGLENGTH + numUnencoded++;
			/* Copy name into right place */
			metrics->charData[curChar].name = (char *)
			    strcpy(malloc(strlen(nameBuffer)+1),nameBuffer);
		    }
		}
		else if (strcmp(token,"DWIDTH")==0)
		{
		    if (curChar != -1)
		    	sscanf(fileLine,"%*s %f %*d", 
					&metrics->widths[curChar]);
		}
		else if (strcmp(token,"BBX")==0)
		{
		    if (curChar != -1)
		    	sscanf(fileLine,"%*s %f %f %f %f", 
				&metrics->charData[curChar].charBBox[2],
				&metrics->charData[curChar].charBBox[3],
				&metrics->charData[curChar].charBBox[0],
				&metrics->charData[curChar].charBBox[1]);
		}
		else if (strcmp(token,"ENDCHAR")==0)
		{
		    curChar = -1;
		    charCount--;
		}
		else if (strcmp(token,"ENDFONT")==0)
		{
		    curChar = -1;
		    charCount = 0; /* Exit immediately */
		}
	}
    }
    return(0);
} /* ReadBitmapFile */

static void WriteScreenAfm(outF,metrics,fontSize)
FILE *outF;
NXFontMetrics *metrics;
int fontSize;
{
    int j,numChars;
    
    WriteAfmHeader(outF,metrics,fontSize);
    /* Now write the character info part of the new afm file */
    /* Count number of characters we really read info about */
    numChars = 0;
    for(j=0;j<CHARARRAYLENGTH;j++)
    	if (metrics->widths[j])
		numChars++;
    fprintf(outF,"StartCharMetrics %d\n",numChars);
    for(j=0;j<CHARARRAYLENGTH;j++)
    {
    	if (metrics->widths[j])
	{
	    fprintf(outF,"C %d ; WX %d ; N %s ; B %d %d %d %d ;\n",
	    	((j >= ENCODINGLENGTH) ? -1 : j),
		(int)metrics->widths[j],metrics->charData[j].name,
		(int)metrics->charData[j].charBBox[0],
		(int)metrics->charData[j].charBBox[1],
		(int)metrics->charData[j].charBBox[2],
		(int)metrics->charData[j].charBBox[3]);
	}
    }
    /* Finish the sucker off */
    fprintf(outF,"EndCharMetrics\nEndFontMetrics\n");
} /* WriteScreenAfm */

static void WriteAfmHeader(f,metrics,fontSize)
FILE *f;
NXFontMetrics *metrics;
int	fontSize;
{
	fprintf(f,"StartFontMetrics %s\n",metrics->formatVersion);
	fprintf(f,"Comment Copyright 1988 NeXT, Inc.\n");
	fprintf(f,"Comment Created by ScreenAfm from printer afm file\n");
	fprintf(f,"FontName %s\n",metrics->name);
	fprintf(f,"EncodingScheme %s\n",metrics->encodingScheme);
	fprintf(f,"FullName %s\n",metrics->fullName);
	fprintf(f,"FamilyName %s\n",metrics->familyName);
	fprintf(f,"Weight %s\n",metrics->weight);
	fprintf(f,"ItalicAngle %.1f\n",metrics->italicAngle);
	fprintf(f,"IsFixedPitch %s\n",metrics->isFixedPitch?"true":"false");
	fprintf(f,"IsScreenFont true\n");
	fprintf(f,"ScreenFontSize %d\n",fontSize);
	fprintf(f,"UnderlinePosition %d\n",(int)metrics->underlinePosition);
	fprintf(f,"UnderlineThickness %d\n",(int)metrics->underlineThickness);
	fprintf(f,"Version %s\n",metrics->version);
	fprintf(f,"FontBBox %d %d %d %d\n",(int)metrics->fontBBox[0],
	    (int)metrics->fontBBox[1],(int)metrics->fontBBox[2],
	    (int)metrics->fontBBox[3]);
	fprintf(f,"CapHeight %d\n",(int)metrics->capHeight);
	fprintf(f,"XHeight %d\n",(int)metrics->xHeight);
	fprintf(f,"Descender %d\n",(int)metrics->descender);
	fprintf(f,"Ascender %d\n",(int)metrics->ascender);
} /* WriteAfmHeader */






























