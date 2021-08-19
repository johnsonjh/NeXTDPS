/*
    tiffutil.m
    
    A utility to deal with TIFF files.
    
    Ali Ozer
    Copyright 1990, NeXT, Inc.
    
    Created: April 12, 1990
    Added some of Sam Leffler's PD code from tiffcp.c May 4, 1990

    Copyright (c) 1988 by Sam Leffler. All rights reserved.
    This file is provided for unrestricted use provided that this
    legend is included on all tape media and as a part of the
    software program in whole or part.  Users may copy, modify or
    distribute this file at will.
	
*/

#import <stdio.h>
#import <streams/streams.h>
#import <objc/objc.h>
#import <appkit/graphics.h>
#import <libc.h>
#import <strings.h>
#import "/usr/local/include/appkit/tiffPrivate.h"

#define COMPRESSION_LZW_HORDIFF (-2)

void DoUsageAndExit(char *msg)
{
    if (msg) {
	fprintf (stderr, "Error: %s", msg);
    }
    fprintf(stderr,
	"Usage: tiffutil -none      infile                  [-out outfile]\n"
	"                -lzw       infile                  [-out outfile]\n"
	"                -packbits  infile                  [-out outfile]\n"
	"                -cat       infile1 [infile2 ...]   [-out outfile]\n"
	"                -extract   num infile              [-out outfile]\n"
	"                -fix       infile                  [-out outfile]\n"
	"                -info      infile1 [infile2 ...]\n"
	"                -dump      infile1 [infile2 ...]\n\n"
    );
    exit(1);
}

#define	streq(a,b)	(strcmp(a,b) == 0)

#define	CopyField(tag, v) \
    if (_NXTIFFGetField(in, tag, &v)) \
	_NXTIFFSetField(out, tag, v)

#define	CopyField3(tag, v1, v2, v3) \
    if (_NXTIFFGetField(in, tag, &v1, &v2, &v3)) \
	_NXTIFFSetField(out, tag, v1, v2, v3)

typedef enum _Op {OP_COMPRESS, OP_CONCATANATE, OP_EXTRACT, OP_INFO, OP_FIX, OP_DUMP} Op;

static int numWritten = 0;

#define	FIELD(tif,f)	TIFFFieldSet(tif, CAT(FIELD_,f))

static char *ResponseUnitNames[] = {
	"#0",
	"10ths",
	"100ths",
	"1,000ths",
	"10,000ths",
	"100,000ths",
};
static	float ResponseUnit[] = { 1., .1, .01, .001, .0001, .00001 };
#define	MAXRESPONSEUNIT \
    (sizeof (ResponseUnitNames) / sizeof (ResponseUnitNames[0]))

/*
 * Print the contents of the current directory.
 * (This function taken from tif_print.c in Sam Leffler's library.)
 */
void ShowInfo (TIFF *tif, FILE *fd,
	       int showstrips, int showresponsecurve, int showcolormap)
{
	register TIFFDirectory *td;
	char *sep;
	int i;
	long n;
	float unit;

	fprintf(fd, "Directory at 0x%x\n", tif->tif_diroff);
	td = &tif->tif_dir;
	if (FIELD(tif,SUBFILETYPE)) {
		fprintf(fd, "  Subfile Type:");
		sep = " ";
		if (td->td_subfiletype & FILETYPE_REDUCEDIMAGE) {
			fprintf(fd, "%sreduced-resolution image", sep);
			sep = "/";
		}
		if (td->td_subfiletype & FILETYPE_PAGE) {
			fprintf(fd, "%smulti-page document", sep);
			sep = "/";
		}
		if (td->td_subfiletype & FILETYPE_MASK) {
			fprintf(fd, "%stransparency mask", sep);
			sep = "/";
		}
		fprintf(fd, " (%u = 0x%x)\n",
		    td->td_subfiletype, td->td_subfiletype);
	}
	if (FIELD(tif,IMAGEDIMENSIONS))
		fprintf(fd, "  Image Width: %u Image Length: %u\n",
		    td->td_imagewidth, td->td_imagelength);
	if (FIELD(tif,RESOLUTION))
		fprintf(fd, "  Resolution: %g, %g\n",
		    td->td_xresolution, td->td_yresolution);
	if (FIELD(tif,RESOLUTIONUNIT)) {
		fprintf(fd, "  Resolution Unit: ");
		switch (td->td_resolutionunit) {
		case RESUNIT_NONE:
			fprintf(fd, "none\n");
			break;
		case RESUNIT_INCH:
			fprintf(fd, "pixels/inch\n");
			break;
		case RESUNIT_CENTIMETER:
			fprintf(fd, "pixels/cm\n");
			break;
		default:
			fprintf(fd, "%u (0x%x)\n",
			    td->td_resolutionunit, td->td_resolutionunit);
			break;
		}
	}
	if (FIELD(tif,POSITION))
		fprintf(fd, "  Position: %g, %g\n",
		    td->td_xposition, td->td_yposition);
	if (FIELD(tif,BITSPERSAMPLE))
		fprintf(fd, "  Bits/Sample: %u\n", td->td_bitspersample);
	if (FIELD(tif,COMPRESSION)) {
		fprintf(fd, "  Compression Scheme: ");
		switch (td->td_compression) {
		case COMPRESSION_NONE:
			fprintf(fd, "none\n");
			break;
		case COMPRESSION_CCITTRLE:
			fprintf(fd, "CCITT modified Huffman encoding\n");
			break;
		case COMPRESSION_CCITTFAX3:
			fprintf(fd, "CCITT Group 3 facsimile encoding\n");
			break;
		case COMPRESSION_CCITTFAX4:
			fprintf(fd, "CCITT Group 4 facsimile encoding\n");
			break;
		case COMPRESSION_CCITTRLEW:
			fprintf(fd, "CCITT modified Huffman encoding %s\n",
			    "w/ word alignment");
			break;
		case COMPRESSION_PACKBITS:
			fprintf(fd, "PackBits encoding\n");
			break;
		case COMPRESSION_THUNDERSCAN:
			fprintf(fd, "ThunderScan 4-bit encoding\n");
			break;
		case COMPRESSION_LZW:
			fprintf(fd, "Lempel-Ziv & Welch encoding\n");
			break;
		case COMPRESSION_JPEG:
			fprintf(fd, "NeXT JPEG encoding\n");
			break;
		case COMPRESSION_PICIO:
			fprintf(fd, "Pixar picio encoding\n");
			break;
		case COMPRESSION_NEXT:
			fprintf(fd, "NeXT 2-bit encoding\n");
			break;
		default:
			fprintf(fd, "%u (0x%x)\n",
			    td->td_compression, td->td_compression);
			break;
		}
	}
	if (FIELD(tif,PHOTOMETRIC)) {
		fprintf(fd, "  Photometric Interpretation: ");
		switch (td->td_photometric) {
		case PHOTOMETRIC_MINISWHITE:
			fprintf(fd, "\"min-is-white\"\n");
			break;
		case PHOTOMETRIC_MINISBLACK:
			fprintf(fd, "\"min-is-black\"\n");
			break;
		case PHOTOMETRIC_RGB:
			fprintf(fd, "RGB color\n");
			break;
		case PHOTOMETRIC_PALETTE:
			fprintf(fd, "palette color (RGB from colormap)\n");
			break;
		case PHOTOMETRIC_MASK:
			fprintf(fd, "transparency mask\n");
			break;
		case 5:
			if (td->td_samplesperpixel==2) {
			    fprintf(fd, "5 (NeXT alpha, used in 1.0)\n");
			} else {
			    fprintf(fd, "5 (CMYK color)\n");
			}
			break;
		default:
			fprintf(fd, "%u (0x%x)\n",
			    td->td_photometric, td->td_photometric);
			break;
		}
	}
	if (FIELD(tif,MATTEING))
		fprintf(fd, "  Alpha: %s\n",
		    td->td_matteing ? "Present" : "Not present");
	if (FIELD(tif,THRESHHOLDING)) {
		fprintf(fd, "  Thresholding: ");
		switch (td->td_threshholding) {
		case THRESHHOLD_BILEVEL:
			fprintf(fd, "bilevel art scan\n");
			break;
		case THRESHHOLD_HALFTONE:
			fprintf(fd, "halftone or dithered scan\n");
			break;
		case THRESHHOLD_ERRORDIFFUSE:
			fprintf(fd, "error diffused\n");
			break;
		default:
			fprintf(fd, "%u (0x%x)\n",
			    td->td_threshholding, td->td_threshholding);
			break;
		}
	}
	if (FIELD(tif,FILLORDER)) {
		fprintf(fd, "  FillOrder: ");
		switch (td->td_fillorder) {
		case FILLORDER_MSB2LSB:
			fprintf(fd, "msb-to-lsb\n");
			break;
		case FILLORDER_LSB2MSB:
			fprintf(fd, "lsb-to-msb\n");
			break;
		default:
			fprintf(fd, "%u (0x%x)\n",
			    td->td_fillorder, td->td_fillorder);
			break;
		}
	}
	if (FIELD(tif,PREDICTOR)) {
		fprintf(fd, "  Predictor: ");
		switch (td->td_predictor) {
		case 1:
			fprintf(fd, "none\n");
			break;
		case 2:
			fprintf(fd, "horizontal differencing\n");
			break;
		default:
			fprintf(fd, "%u (0x%x)\n",
			    td->td_predictor, td->td_predictor);
			break;
		}
	}
	if (FIELD(tif,ARTIST))
		fprintf(fd, "  Artist: \"%s\"\n", td->td_artist);
	if (FIELD(tif,DATETIME))
		fprintf(fd, "  Date & Time: \"%s\"\n", td->td_datetime);
	if (FIELD(tif,HOSTCOMPUTER))
		fprintf(fd, "  Host Computer: \"%s\"\n", td->td_hostcomputer);
	if (FIELD(tif,SOFTWARE))
		fprintf(fd, "  Software: \"%s\"\n", td->td_software);
	if (FIELD(tif,DOCUMENTNAME))
		fprintf(fd, "  Document Name: \"%s\"\n", td->td_documentname);
	if (FIELD(tif,IMAGEDESCRIPTION))
		fprintf(fd, "  Image Description: \"%s\"\n",
		    td->td_imagedescription);
	if (FIELD(tif,MAKE))
		fprintf(fd, "  Make: \"%s\"\n", td->td_make);
	if (FIELD(tif,MODEL))
		fprintf(fd, "  Model: \"%s\"\n", td->td_model);
	if (FIELD(tif,ORIENTATION)) {
		fprintf(fd, "  Orientation: ");
		switch (td->td_orientation) {
		case ORIENTATION_TOPLEFT:
			fprintf(fd, "row 0 top, col 0 lhs\n");
			break;
		case ORIENTATION_TOPRIGHT:
			fprintf(fd, "row 0 top, col 0 rhs\n");
			break;
		case ORIENTATION_BOTRIGHT:
			fprintf(fd, "row 0 bottom, col 0 rhs\n");
			break;
		case ORIENTATION_BOTLEFT:
			fprintf(fd, "row 0 bottom, col 0 lhs\n");
			break;
		case ORIENTATION_LEFTTOP:
			fprintf(fd, "row 0 lhs, col 0 top\n");
			break;
		case ORIENTATION_RIGHTTOP:
			fprintf(fd, "row 0 rhs, col 0 top\n");
			break;
		case ORIENTATION_RIGHTBOT:
			fprintf(fd, "row 0 rhs, col 0 bottom\n");
			break;
		case ORIENTATION_LEFTBOT:
			fprintf(fd, "row 0 lhs, col 0 bottom\n");
			break;
		default:
			fprintf(fd, "%u (0x%x)\n",
			    td->td_orientation, td->td_orientation);
			break;
		}
	}
	if (FIELD(tif,SAMPLESPERPIXEL))
		fprintf(fd, "  Samples/Pixel: %u\n", td->td_samplesperpixel);
	if (FIELD(tif,ROWSPERSTRIP)) {
		fprintf(fd, "  Rows/Strip: ");
		if (td->td_rowsperstrip == 0xffffffffL)
			fprintf(fd, "(infinite)\n");
		else
			fprintf(fd, "%u\n", td->td_rowsperstrip);
	}
	if (FIELD(tif,STRIPOFFSETS)) {
		fprintf(fd, "  Number of Strips: %u\n", td->td_nstrips);
	}
	if (FIELD(tif,MINSAMPLEVALUE))
		fprintf(fd, "  Min Sample Value: %u\n", td->td_minsamplevalue);
	if (FIELD(tif,MAXSAMPLEVALUE))
		fprintf(fd, "  Max Sample Value: %u\n", td->td_maxsamplevalue);
	if (FIELD(tif,PLANARCONFIG)) {
		fprintf(fd, "  Planar Configuration: ");
		switch (td->td_planarconfig) {
		case PLANARCONFIG_CONTIG:
			fprintf(fd, "Not planar\n");
			break;
		case PLANARCONFIG_SEPARATE:
			fprintf(fd, "Planar\n");
			break;
		default:
			fprintf(fd, "%u (0x%x)\n",
			    td->td_planarconfig, td->td_planarconfig);
			break;
		}
	}
	if (FIELD(tif,PAGENAME))
		fprintf(fd, "  Page Name: \"%s\"\n", td->td_pagename);
	if (FIELD(tif,GRAYRESPONSEUNIT)) {
		fprintf(fd, "  Gray Response Unit: ");
		if (td->td_grayresponseunit < MAXRESPONSEUNIT)
			fprintf(fd, "%s\n",
			    ResponseUnitNames[td->td_grayresponseunit]);
		else
			fprintf(fd, "%u (0x%x)\n",
			    td->td_grayresponseunit, td->td_grayresponseunit);
	}
	if (FIELD(tif,GRAYRESPONSECURVE)) {
		fprintf(fd, "  Gray Response Curve: ");
		if (showresponsecurve) {
			fprintf(fd, "\n");
			unit = ResponseUnit[td->td_grayresponseunit];
			n = 1L<<td->td_bitspersample;
			for (i = 0; i < n; i++)
				fprintf(fd, "    %2d: %g (%u)\n",
				    i,
				    td->td_grayresponsecurve[i] * unit,
				    td->td_grayresponsecurve[i]);
		} else
			fprintf(fd, "(present)\n");
	}
	if (FIELD(tif,GROUP3OPTIONS)) {
		fprintf(fd, "  Group 3 Options:");
		sep = " ";
		if (td->td_group3options & GROUP3OPT_2DENCODING)
			fprintf(fd, "%s2-d encoding", sep), sep = "+";
		if (td->td_group3options & GROUP3OPT_FILLBITS)
			fprintf(fd, "%sEOL padding", sep), sep = "+";
		if (td->td_group3options & GROUP3OPT_UNCOMPRESSED)
			fprintf(fd, "%sno compression", sep), sep = "+";
		fprintf(fd, " (%u = 0x%x)\n",
		    td->td_group3options, td->td_group3options);
	}
	if (FIELD(tif,CLEANFAXDATA)) {
		fprintf(fd, "  Fax Data: ");
		switch (td->td_cleanfaxdata) {
		case CLEANFAXDATA_CLEAN:
			fprintf(fd, "clean\n");
			break;
		case CLEANFAXDATA_REGENERATED:
			fprintf(fd, "receiver regenerated\n");
			break;
		case CLEANFAXDATA_UNCLEAN:
			fprintf(fd, "uncorrected errors\n");
			break;
		default:
			fprintf(fd, "(%u = 0x%x)\n",
			    td->td_cleanfaxdata, td->td_cleanfaxdata);
			break;
		}
	}
	if (FIELD(tif,BADFAXLINES))
		fprintf(fd, "  Bad Fax Lines: %u\n", td->td_badfaxlines);
	if (FIELD(tif,BADFAXRUN))
		fprintf(fd, "  Consecutive Bad Fax Lines: %u\n",
		    td->td_badfaxrun);
	if (FIELD(tif,GROUP4OPTIONS))
		fprintf(fd, "  Group 4 Options: 0x%x\n", td->td_group4options);
	if (FIELD(tif,PAGENUMBER))
		fprintf(fd, "  Page Number: %u-%u\n",
		    td->td_pagenumber[0], td->td_pagenumber[1]);
	if (FIELD(tif,COLORRESPONSEUNIT)) {
		fprintf(fd, "  Color Response Unit: ");
		if (td->td_colorresponseunit < MAXRESPONSEUNIT)
			fprintf(fd, "%s\n",
			    ResponseUnitNames[td->td_colorresponseunit]);
		else
			fprintf(fd, "%u (0x%x)\n",
			    td->td_colorresponseunit, td->td_colorresponseunit);
	}
	if (FIELD(tif,COLORMAP)) {
		fprintf(fd, "  Color Map: ");
		if (showcolormap) {
			fprintf(fd, "\n");
			n = 1L<<td->td_bitspersample;
			for (i = 0; i < n; i++)
				fprintf(fd, "   %5d: %5u %5u %5u\n",
				    i,
				    td->td_redcolormap[i],
				    td->td_greencolormap[i],
				    td->td_bluecolormap[i]);
		} else
			fprintf(fd, "(present)\n");
	}
	if (FIELD(tif,COLORRESPONSECURVE)) {
		fprintf(fd, "  Color Response Curve: ");
		if (showresponsecurve) {
			fprintf(fd, "\n");
			unit = ResponseUnit[td->td_colorresponseunit];
			n = 1L<<td->td_bitspersample;
			for (i = 0; i < n; i++)
				fprintf(fd, "    %2d: %6.4f %6.4f %6.4f\n",
				    i,
				    td->td_redresponsecurve[i] * unit,
				    td->td_greenresponsecurve[i] * unit,
				    td->td_blueresponsecurve[i] * unit);
		} else
			fprintf(fd, "(present)\n");
	}
	if (showstrips && FIELD(tif,STRIPOFFSETS)) {
		fprintf(fd, "  %u Strips:\n", td->td_nstrips);
		for (i = 0; i < td->td_nstrips; i++)
			fprintf(fd, "    %3d: [%8u, %8u]\n",
			    i, td->td_stripoffset[i], td->td_stripbytecount[i]);
	}
}


/*
 * if compression = -1, use incoming
 * if dirNum = -1, copy all
 */
BOOL CopyTIFF (TIFF *out, char *fileName, short compression, int dirNum, Op op)
{
    NXStream *inStream;
    TIFF *in = NULL;
    int dirCount, toDir = 0;

    short bitspersample, samplesperpixel, shortv;
    short imagewidth, imagelength, config = -1;
    long rowsperstrip = -1;
    float floatv;
    char *stringv;
    u_long longv;
    u_char *buf;
    int row, s;
    int err = -1;

    /* Open the input stream and the input TIFF file. */

    if (inStream = NXMapFile(fileName, NX_READONLY)) {
	if (in = _NXTIFFOpenStream(inStream, YES, &err)) {
	    toDir = _NXTIFFNumDirectories(in);
	    if (!((dirNum == -1) || ((dirNum < toDir) && (dirNum >= 0)))) {
		fprintf (stderr, "Error: File %s doesn't have an image %d\n",
				    fileName, dirNum);
		err = -1;
	    } else {
		err = 0;
	    }
	} else {
	    fprintf (stderr, "Error: %s doesn't seem to be a TIFF file\n", 
			fileName);
	    err = -1;
	}
    } else {
	fprintf (stderr, "Error: Could not open file %s\n", fileName);
    }	

    if (err) {
	if (in) {
	    _NXTIFFClose (in);
	    if (inStream) {
		NXCloseMemory (inStream, NX_FREEBUFFER);
	    }
	}
	return NO;
    }

    if (dirNum != -1) {
	toDir = dirNum+1;
	dirCount = dirNum;
    } else {
	dirCount = 0;
    }

    while (dirCount < toDir) {

	if (dirCount) {
	    _NXTIFFSetDirectory	(in, dirCount);
	}

	if (op == OP_INFO) {
	    ShowInfo (in, stdout, 0, 0, 0);
	    goto done;
	}

	if (numWritten) {
	    _NXTIFFWriteDirectory (out);
	}

	CopyField(TIFFTAG_SUBFILETYPE, longv);
	CopyField(TIFFTAG_IMAGEWIDTH, imagewidth);
	CopyField(TIFFTAG_IMAGELENGTH, imagelength);
	CopyField(TIFFTAG_BITSPERSAMPLE, bitspersample);
	if (compression != -1) {
	    _NXTIFFSetField(out, TIFFTAG_COMPRESSION,
			(compression == COMPRESSION_LZW_HORDIFF) ?
				COMPRESSION_LZW : compression);
	    if (compression == COMPRESSION_LZW_HORDIFF && bitspersample == 8) {
		_NXTIFFSetField(out, TIFFTAG_PREDICTOR, 2);
	    }
	} else {
	    CopyField(TIFFTAG_COMPRESSION, compression);
	    CopyField(TIFFTAG_PREDICTOR, shortv);
	}
	CopyField(TIFFTAG_THRESHHOLDING, shortv);
	CopyField(TIFFTAG_FILLORDER, shortv);
	CopyField(TIFFTAG_ORIENTATION, shortv);
	if ((op == OP_FIX) &&
	    (in->tif_dir.td_photometric == 5) &&
	    (in->tif_dir.td_samplesperpixel == 2) &&
	    (in->tif_dir.td_bitspersample == 2) &&
	    (!(FIELD(in,MATTEING)))) {
	    _NXTIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
	    _NXTIFFSetField(out, TIFFTAG_MATTEING, 1);
	} else {			
	    CopyField(TIFFTAG_PHOTOMETRIC, shortv);
	    CopyField(TIFFTAG_MATTEING, shortv);
	}
        CopyField(TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
	CopyField(TIFFTAG_MINSAMPLEVALUE, shortv);
	CopyField(TIFFTAG_MAXSAMPLEVALUE, shortv);
	CopyField(TIFFTAG_XRESOLUTION, floatv);
	CopyField(TIFFTAG_YRESOLUTION, floatv);
	CopyField(TIFFTAG_GROUP3OPTIONS, longv);
	CopyField(TIFFTAG_GROUP4OPTIONS, longv);
	CopyField(TIFFTAG_RESOLUTIONUNIT, shortv);
	CopyField(TIFFTAG_PLANARCONFIG, config);
	/*
	 * Prevent huge strips with LZW; otherwise readers on wimpy machines
	 * will choke. Also there seems to be a bug in creating huge strips
	 * in LZW mode, anyway. 
	 */
	if (compression == COMPRESSION_LZW ||
	    compression == COMPRESSION_LZW_HORDIFF) {
	    int rowsperstrip = (32*1024)/_NXTIFFScanlineSize(out);
	    if (rowsperstrip == 0) rowsperstrip = 1;
	    _NXTIFFSetField(out, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
	} else {
	    CopyField(TIFFTAG_ROWSPERSTRIP, rowsperstrip);
	}
	CopyField(TIFFTAG_XPOSITION, floatv);
	CopyField(TIFFTAG_YPOSITION, floatv);
	CopyField(TIFFTAG_GRAYRESPONSEUNIT, shortv);
	{u_short *graycurve; CopyField(TIFFTAG_GRAYRESPONSECURVE, graycurve);}
	CopyField(TIFFTAG_COLORRESPONSEUNIT, shortv);
	{u_short *red, *green, *blue;
	 CopyField3(TIFFTAG_COLORRESPONSECURVE, red, green, blue);
	}
	{u_short *red, *green, *blue;
	 CopyField3(TIFFTAG_COLORMAP, red, green, blue);
	}
	CopyField(TIFFTAG_ARTIST, stringv);
	CopyField(TIFFTAG_IMAGEDESCRIPTION, stringv);
	if (((unsigned short)compression) == COMPRESSION_JPEG) {
	    /* Deal with this... */
	} else {
	    (void) _NXTIFFGetField(in, TIFFTAG_PLANARCONFIG, &shortv);
	    buf = (u_char *)malloc(_NXTIFFScanlineSize(in));
	    if (shortv == PLANARCONFIG_CONTIG) {
		for (row = 0; row < imagelength; row++) {
		    if (_NXTIFFReadScanline(in, buf, row, 0) < 0 ||
			_NXTIFFWriteScanline(out, buf, row, 0) < 0)
			    goto done;
		}
	    } else {
		for (s = 0; s < samplesperpixel; s++) {
		    for (row = 0; row < imagelength; row++) {
			if (_NXTIFFReadScanline(in, buf, row, s) < 0 ||
			    _NXTIFFWriteScanline(out, buf, row, s) < 0)
				goto done;
		    }
		}
	    }
	    free (buf);
	}

done:
	dirCount++;
	numWritten++;
    }

    (void) _NXTIFFClose(in);
    NXCloseMemory (inStream, NX_FREEBUFFER);

    return YES;
}

BOOL Concatanate (TIFF *out, char **fileNames, int numFiles)
{
    while (numFiles) {
	if (!CopyTIFF(out, *fileNames, -1, -1, OP_CONCATANATE)) {
	    return NO;
	}
	fileNames++;
	numFiles--;
    }
    return YES;
}

BOOL GiveInfo (char **fileNames, int numFiles)
{
    BOOL printFileName = numFiles > 1;

    while (numFiles) {
	if (printFileName) {
	    /* Use a "more" style file separator. */
	    fprintf (stdout, "*** %s\n", *fileNames);
	}
	if (!CopyTIFF(NULL, *fileNames, -1, -1, OP_INFO)) {
	    return NO;
	}
	fileNames++;
	numFiles--;
	if (numFiles) {
	    fprintf (stdout, "\n");
	}
    }
    return YES;
}

extern int DumpTIFF (char **fileNames, int numFiles);

void main(int argc, char *argv[])
{
    TIFF *out = NULL;
    NXStream *outStream = NULL;
    char *opArg, *outFile, **inFile;
    char *defaultOutFile = "out.tiff";
    int err = -1;
    int arg;
    int numFiles;
    Op op = 0;
    
    if (argc < 3) {
	DoUsageAndExit(NULL);
    }

    opArg = argv[1];

    if ((argc < 5) || (strcmp(argv[argc-2],"-out") && strcmp(argv[argc-2],"-o"))) {
	outFile = defaultOutFile;
	numFiles = argc - 2;
    } else {
	outFile = argv[argc-1];
	numFiles = argc - 4;
    }

    inFile = argv+2;

    if (streq(opArg, "-none")) {
	op = OP_COMPRESS;
	arg = COMPRESSION_NONE;
    } else if (streq(opArg, "-packbits")) {
	op = OP_COMPRESS;
	arg = COMPRESSION_PACKBITS;
    } else if (streq(opArg, "-lzw")) {
	op = OP_COMPRESS;
	arg = COMPRESSION_LZW;
    } else if (streq(opArg, "-lzwhd")) {	/* Not documented */
	op = OP_COMPRESS;
	arg = COMPRESSION_LZW_HORDIFF;
#if 0	// Doesn't work for 2.0. -Ali
    } else if (streq(opArg, "-jpeg")) {		/* Doesn't work */
	op = OP_COMPRESS;
	arg = COMPRESSION_JPEG;
#endif
    } else if (streq(opArg, "-cat")) {
	op = OP_CONCATANATE;
    } else if (streq(opArg, "-fix")) {
	op = OP_FIX;
    } else if (streq(opArg, "-info")) {
	op = OP_INFO;
	if (outFile != defaultOutFile) {
	    DoUsageAndExit("Can't specify output file name for -info.\n");
	} else {
	    outFile = NULL;
	}
    } else if (streq(opArg, "-dump")) {
	op = OP_DUMP;
	if (outFile != defaultOutFile) {
	    DoUsageAndExit("Can't specify output file name for -dump.\n");
	} else {
	    outFile = NULL;
	}
    } else if (streq(opArg, "-extract")) {
	op = OP_EXTRACT;
	if (sscanf(*inFile, "%d",  &arg) != 1) {
	    DoUsageAndExit("Image number to be extracted expected.\n");
	} else {
	    inFile++;
	    numFiles--;
	}
    } else {
	DoUsageAndExit("No valid command provided.\n");
    }

    if (numFiles < 1) {
	DoUsageAndExit("Input file name expected.\n");
    } else if ((numFiles > 1) && (op != OP_CONCATANATE) && (op != OP_INFO) && (op != OP_DUMP)) {
	DoUsageAndExit("One input file name expected.\n");
    }

    if (outFile) {
	if (outStream = NXOpenMemory(NULL, 0, NX_READWRITE)) {
	    int err;
	    if (!(out = _NXTIFFOpenStream(outStream, NO, &err))) {
		NXCloseMemory (outStream, NX_FREEBUFFER);
		exit (5);
	    }
	} else {
	    exit (5);
	}
    }

    switch (op) {
	case OP_COMPRESS:
	    err = !CopyTIFF (out, *inFile, arg, -1, op);
	    break;
	case OP_CONCATANATE:
	    err = !Concatanate (out, inFile, numFiles);
	    break;
	case OP_EXTRACT:
	    err = !CopyTIFF (out, *inFile, -1, arg, op);
	    break;
	case OP_INFO:
	    err = !GiveInfo (inFile, numFiles);
	    break;
	case OP_DUMP:
	    err = !DumpTIFF (inFile, numFiles);
	    break;
	case OP_FIX:
	    err = !CopyTIFF (out, *inFile, -1, -1, op);
	    break;
    }

    if ((op != OP_INFO) && (op != OP_DUMP)) {
	if (!err) {
	    _NXTIFFClose (out);
	    NXFlush (outStream);
	    if (err = NXSaveToFile (outStream, outFile)) {
		perror (outFile);
	    } else {
		fprintf (stderr, "%d image%s written to %s.\n",
			numWritten, (numWritten == 1 ? "" : "s"), outFile);
	    }
	} else {
	    fprintf (stderr, "Error; no output.\n");
	}
	NXCloseMemory (outStream, NX_FREEBUFFER);
    }

    exit (err ? 5 : 0);

}

/*
  
Modifications:

10
--
 4/12/90 aozer	Created with minimal functionality (being able to concat)
  5/4/90 aozer	Added more functionality (extract, compress). Changed to
		use public (private) _NXTIFF functions from the Kit.

11
--
 6/10/90 aozer	Changed to use Leffler's TIFF Library v2.2.
 6/10/90 aozer	Added -lzwdiff option to support horizontal differencing

13
--
 8/12/90 aozer	Multiple file name support for the "-info" option.

17
--
 10/13/90 aozer	Check return value of NXSaveToFile() to catch write errors.

*/

 


