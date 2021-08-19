/*
    tiffdump.m
    
    Part of tiffutil.
    
    Responsibility: Ali Ozer

    Directly taken from Sam Leffler's tiffdump.c.

*/


/*
 * Copyright (c) 1988, 1990 by Sam Leffler.
 * All rights reserved.
 *
 * This file is provided for unrestricted use provided that this
 * legend is included on all tape media and as a part of the
 * software program in whole or part.  Users may copy, modify or
 * distribute this file at will.
 */

#import <stdio.h>
#import <fcntl.h>
#import <libc.h>
#import "/usr/local/include/appkit/tiffPrivate.h"

void ReadError (char *str);
void TIFFSwabShort(unsigned short *wp);
void TIFFSwabLong(unsigned long *lp);
void TIFFSwabArrayOfShort(unsigned short *wp, int n);
void TIFFSwabArrayOfLong(unsigned long *lp, int n);
void PrintType();
void PrintData();
void dump(int fd);
void InitByteOrder(int magic);
void PrintTag();

char	*curfile;
int	swabflag;
int	bigendian;
int	typeshift[6];	/* data type shift counts */
long	typemask[6];	/* data type masks */
long	ReadDirectory();

int DumpTIFF(char **fileNames, int numFiles)
{
	int one = 1, fd;
	int printFileName = numFiles > 1;

	bigendian = (*(char *)&one == 0);
	for (; numFiles > 0; numFiles--, fileNames++) {
		fd = open(*fileNames, O_RDONLY);
		if (fd < 0) {
			perror(*fileNames);
			return 0;
		}
		curfile = *fileNames;
		swabflag = 0;
		if (printFileName) {
		    /* Use a "more" style file separator. */
		    fprintf (stdout, "*** %s\n", *fileNames);
		}
		dump(fd);
		close(fd);
		if (numFiles > 1) {
		    fprintf (stdout, "\n");
		}
	}
	return 1;
}


void Error(fmt, a1, a2, a3, a4, a5)
	char *fmt; int a1, a2, a3, a4, a5;
{
	fprintf(stderr, "%s: ", curfile);
	fprintf(stderr, fmt, a1, a2, a3, a4, a5);
	fprintf(stderr, ".\n");
}

void Fatal(fmt, a1, a2, a3, a4, a5)
	char *fmt; int a1, a2, a3, a4, a5;
{
	Error(fmt, a1, a2, a3, a4, a5);
	exit(-1);
}

TIFFHeader h;

#define	ord(e)	((int)e)

/*
 * Initialize shift & mask tables and byte
 * swapping state according to the file
 * byte order.
 */
static void
InitByteOrder(int magic)
{
	typemask[0] = 0;
	typemask[ord(TIFF_BYTE)] = 0xff;
	typemask[ord(TIFF_SHORT)] = 0xffff;
	typemask[ord(TIFF_LONG)] = 0xffffffff;
	typemask[ord(TIFF_RATIONAL)] = 0xffffffff;
	typeshift[0] = 0;
	typeshift[ord(TIFF_LONG)] = 0;
	typeshift[ord(TIFF_RATIONAL)] = 0;
	if (magic == TIFF_BIGENDIAN) {
		typeshift[ord(TIFF_BYTE)] = 24;
		typeshift[ord(TIFF_SHORT)] = 16;
		swabflag = !bigendian;
	} else {
		typeshift[ord(TIFF_BYTE)] = 0;
		typeshift[ord(TIFF_SHORT)] = 0;
		swabflag = bigendian;
	}
}

void dump(int fd)
{
	long off;
	int i;

	lseek(fd, 0L, 0);
	if (read(fd, &h, sizeof (h)) != sizeof (h))
		ReadError("TIFF header");
	/*
	 * Setup the byte order handling.
	 */
	if (h.tiff_magic != TIFF_BIGENDIAN && h.tiff_magic != TIFF_LITTLEENDIAN)
		Fatal("Not a TIFF file, bad magic number %u (0x%x)",
		    h.tiff_magic, h.tiff_magic);
	InitByteOrder(h.tiff_magic);
	/*
	 * Swap header if required.
	 */
	if (swabflag) {
		TIFFSwabShort(&h.tiff_version);
		TIFFSwabLong(&h.tiff_diroff);
	}
	/*
	 * Now check version (if needed, it's been byte-swapped).
	 * Note that this isn't actually a version number, it's a
	 * magic number that doesn't change (stupid).
	 */
	if (h.tiff_version != TIFF_VERSION)
		Fatal("Not a TIFF file, bad version number %u (0x%x)",
		    h.tiff_version, h.tiff_version); 
	printf("Magic: 0x%x <%s-endian> Version: 0x%x\n",
	    h.tiff_magic,
	    h.tiff_magic == TIFF_BIGENDIAN ? "big" : "little",
	    h.tiff_version);
	i = 0;
	off = h.tiff_diroff;
	while (off) {
		if (i > 0)
			putchar('\n');
		printf("Directory %d: offset %lu (0x%lx)\n", i++, off, off);
		off = ReadDirectory(fd, off);
	}
}

static int datawidth[] = {
    1,	/* nothing */
    1,	/* TIFF_BYTE */
    1,	/* TIFF_ASCII */
    2,	/* TIFF_SHORT */
    4,	/* TIFF_LONG */
    8,	/* TIFF_RATIONAL */
};
static	int TIFFFetchData();

/*
 * Read the next TIFF directory from a file
 * and convert it to the internal format.
 * We read directories sequentially.
 */
long
ReadDirectory(fd, off)
	int fd;
	long off;
{
	register TIFFDirEntry *dp;
	register int n;
	TIFFDirEntry *dir = 0;
	unsigned short dircount;
	int space;
	long nextdiroff = 0;

	if (off == 0)			/* no more directories */
		goto done;
	if (lseek(fd, off, 0) != off) {
		Fatal("Seek error accessing TIFF directory");
		goto done;
	}
	if (read(fd, &dircount, sizeof (short)) != sizeof (short)) {
		ReadError("directory count");
		goto done;
	}
	if (swabflag)
		TIFFSwabShort(&dircount);
	dir = (TIFFDirEntry *)malloc(dircount * sizeof (TIFFDirEntry));
	if (dir == NULL) {
		Fatal("No space for TIFF directory");
		goto done;
	}
	n = read(fd, dir, dircount*sizeof (*dp));
	if (n != dircount*sizeof (*dp)) {
		n /= sizeof (*dp);
		Error(
	    "Could only read %u of %u entries in directory at offset 0x%x",
		    n, dircount, off);
		dircount = n;
	}
	if (read(fd, &nextdiroff, sizeof (long)) != sizeof (long))
		nextdiroff = 0;
	if (swabflag)
		TIFFSwabLong((unsigned long *)&nextdiroff);
	for (dp = dir, n = dircount; n > 0; n--, dp++) {
		if (swabflag) {
			TIFFSwabArrayOfShort(&dp->tdir_tag, 2);
			TIFFSwabArrayOfLong(&dp->tdir_count, 2);
		}
		PrintTag(stdout, dp->tdir_tag);
		putchar(' ');
		PrintType(stdout, dp->tdir_type);
		putchar(' ');
		printf("%u<", dp->tdir_count);
		space = dp->tdir_count * datawidth[dp->tdir_type];
		if (space <= 4) {
			switch (dp->tdir_type) {
			case TIFF_ASCII: {
				char data[4];
				bcopy(&dp->tdir_offset, data, 4);
				if (swabflag)
					TIFFSwabLong((unsigned long *)data);
				PrintData(stdout,
				    dp->tdir_type, dp->tdir_count, data);
				break;
			}
			case TIFF_BYTE:
				if (h.tiff_magic == TIFF_LITTLEENDIAN) {
					switch ((int)dp->tdir_count) {
					case 4:
						printf("0x%02x",
						    dp->tdir_offset&0xff);
					case 3:
						printf("0x%02x",
						    (dp->tdir_offset>>8)&0xff);
					case 2:
						printf("0x%02x",
						    (dp->tdir_offset>>16)&0xff);
					case 1:
						printf("0x%02x",
						    dp->tdir_offset>>24);
						break;
					}
				} else {
					switch ((int)dp->tdir_count) {
					case 4:
						printf("0x%02x",
						    dp->tdir_offset>>24);
					case 3:
						printf("0x%02x",
						    (dp->tdir_offset>>16)&0xff);
					case 2:
						printf("0x%02x",
						    (dp->tdir_offset>>8)&0xff);
					case 1:
						printf("0x%02x",
						    dp->tdir_offset&0xff);
						break;
					}
				}
				break;
			case TIFF_SHORT:
				if (h.tiff_magic == TIFF_LITTLEENDIAN) {
					switch (dp->tdir_count) {
					case 2:
						printf("%u ",
						    dp->tdir_offset>>16);
					case 1:
						printf("%u",
						    dp->tdir_offset&0xffff);
						break;
					}
				} else {
					switch (dp->tdir_count) {
					case 2:
						printf("%u ",
						    dp->tdir_offset&0xffff);
					case 1:
						printf("%u",
						    dp->tdir_offset>>16);
						break;
					}
				}
				break;
			case TIFF_LONG:
				printf("%lu", dp->tdir_offset);
				break;
			}
		} else {
			char *data = (char *)malloc(space);
			if (data == 0) {
				Error("No space for data for tag %u",
				    dp->tdir_tag);
				continue;
			}
			TIFFFetchData(fd, dp, data);
			PrintData(stdout, dp->tdir_type, dp->tdir_count, data);
			free(data);
		}
		printf(">\n");
	}
done:
	if (dir)
		free((char *)dir);
	return (nextdiroff);
}

static	struct tagname {
	int	tag;
	char	*name;
} tagnames[] = {
    { TIFFTAG_SUBFILETYPE,	"SubFileType" },
    { TIFFTAG_OSUBFILETYPE,	"OldSubFileType" },
    { TIFFTAG_IMAGEWIDTH,	"ImageWidth" },
    { TIFFTAG_IMAGELENGTH,	"ImageLength" },
    { TIFFTAG_BITSPERSAMPLE,	"BitsPerSample" },
    { TIFFTAG_COMPRESSION,	"Compression" },
    { TIFFTAG_PHOTOMETRIC,	"Photometric" },
    { TIFFTAG_THRESHHOLDING,	"Threshholding" },
    { TIFFTAG_CELLWIDTH,	"CellWidth" },
    { TIFFTAG_CELLLENGTH,	"CellLength" },
    { TIFFTAG_FILLORDER,	"FillOrder" },
    { TIFFTAG_DOCUMENTNAME,	"DocumentName" },
    { TIFFTAG_IMAGEDESCRIPTION,	"ImageDescription" },
    { TIFFTAG_MAKE,		"Make" },
    { TIFFTAG_MODEL,		"Model" },
    { TIFFTAG_STRIPOFFSETS,	"StripOffsets" },
    { TIFFTAG_ORIENTATION,	"Orientation" },
    { TIFFTAG_SAMPLESPERPIXEL,	"SamplesPerPixel" },
    { TIFFTAG_ROWSPERSTRIP,	"RowsPerStrip" },
    { TIFFTAG_STRIPBYTECOUNTS,	"StripByteCounts" },
    { TIFFTAG_MINSAMPLEVALUE,	"MinSampleValue" },
    { TIFFTAG_MAXSAMPLEVALUE,	"MaxSampleValue" },
    { TIFFTAG_XRESOLUTION,	"XResolution" },
    { TIFFTAG_YRESOLUTION,	"YResolution" },
    { TIFFTAG_PLANARCONFIG,	"PlanarConfig" },
    { TIFFTAG_PAGENAME,		"PageName" },
    { TIFFTAG_XPOSITION,	"XPosition" },
    { TIFFTAG_YPOSITION,	"YPosition" },
    { TIFFTAG_FREEOFFSETS,	"FreeOffsets" },
    { TIFFTAG_FREEBYTECOUNTS,	"FreeByteCounts" },
    { TIFFTAG_GRAYRESPONSEUNIT,	"GrayResponseUnit" },
    { TIFFTAG_GRAYRESPONSECURVE,"GrayResponseCurve" },
    { TIFFTAG_GROUP3OPTIONS,	"Group3Options" },
    { TIFFTAG_GROUP4OPTIONS,	"Group4Options" },
    { TIFFTAG_RESOLUTIONUNIT,	"ResolutionUnit" },
    { TIFFTAG_PAGENUMBER,	"PageNumber" },
    { TIFFTAG_COLORRESPONSEUNIT,"ColorResponseUnit" },
    { TIFFTAG_COLORRESPONSECURVE,"ColorResponseCurve" },
    { TIFFTAG_SOFTWARE,		"Software" },
    { TIFFTAG_DATETIME,		"DateTime" },
    { TIFFTAG_ARTIST,		"Artist" },
    { TIFFTAG_HOSTCOMPUTER,	"HostComputer" },
    { TIFFTAG_PREDICTOR,	"Predictor" },
    { TIFFTAG_WHITEPOINT,	"Whitepoint" },
    { TIFFTAG_PRIMARYCHROMATICITIES,"PrimaryChromaticities" },
    { TIFFTAG_COLORMAP,		"Colormap" },
    { TIFFTAG_BADFAXLINES,	"BadFaxLines" },
    { TIFFTAG_CLEANFAXDATA,	"CleanFaxData" },
    { TIFFTAG_CONSECUTIVEBADFAXLINES, "ConsecutiveBadFaxLines" },
    { 32996,			"OLD BOGUS SGIColormap tag" },
    { 32768,			"OLD BOGUS Matteing tag" },
    { TIFFTAG_MATTEING,		"Matteing" },
};
#define	NTAGS	(sizeof (tagnames) / sizeof (tagnames[0]))

void PrintTag(fd, tag)
	FILE *fd;
	unsigned short tag;
{
	register struct tagname *tp;

	for (tp = tagnames; tp < &tagnames[NTAGS]; tp++)
		if (tp->tag == tag) {
			fprintf(fd, "%s (%u)", tp->name, tag);
			return;
		}
	fprintf(fd, "%u (0x%x)", tag, tag);
}

void PrintType(fd, type)
	FILE *fd;
	unsigned short type;
{
	static char *typenames[] = {
	    "0",
	    "BYTE",
	    "ASCII",
	    "SHORT",
	    "LONG",
	    "RATIONAL",
	};
#define	NTYPES	(sizeof (typenames) / sizeof (typenames[0]))

	if (type < NTYPES)
		fprintf(fd, "%s (%u)", typenames[type], type);
	else
		fprintf(fd, "%u (0x%x)", type, type);
}

void PrintData(fd, type, count, data)
	FILE *fd;
	unsigned short type;
	long count;
	unsigned char *data;
{

	switch (type) {
	case TIFF_BYTE:
		while (count-- > 0)
			fprintf(fd, "0x%02x", *data++);
		break;
	case TIFF_ASCII:
		fprintf(fd, "%.*s", count, data);
		break;
	case TIFF_SHORT: {
		register unsigned short *wp = (unsigned short *)data;
		while (count-- > 0)
			fprintf(fd, "%u%s", *wp++, count > 0 ? " " : "");
		break;
	}
	case TIFF_LONG: {
		register unsigned long *lp = (unsigned long *)data;
		while (count-- > 0)
			fprintf(fd, "%lu%s", *lp++, count > 0 ? " " : "");
		break;
	}
	case TIFF_RATIONAL: {
		register unsigned long *lp = (unsigned long *)data;
		while (count-- > 0) {
			if (lp[1] == 0)
				fprintf(fd, "Nan");
			else
				fprintf(fd, "%g",
				    (double)lp[0] / (double)lp[1]);
			if (count > 0)
				fprintf(fd, " ");
		}
		break;
	}
	}
}

void TIFFSwabShort(wp)
	unsigned short *wp;
{
	register unsigned char *cp = (unsigned char *)wp;
	int t;

	t = cp[1]; cp[1] = cp[0]; cp[0] = t;
}

void TIFFSwabLong(lp)
	unsigned long *lp;
{
	register unsigned char *cp = (unsigned char *)lp;
	int t;

	t = cp[3]; cp[3] = cp[0]; cp[0] = t;
	t = cp[2]; cp[2] = cp[1]; cp[1] = t;
}

void TIFFSwabArrayOfShort(wp, n)
	unsigned short *wp;
	int n;
{
	register unsigned char *cp;
	register int t;

	/* XXX unroll loop some */
	while (n-- > 0) {
		cp = (unsigned char *)wp;
		t = cp[1]; cp[1] = cp[0]; cp[0] = t;
		wp++;
	}
}

void TIFFSwabArrayOfLong(lp, n)
	register unsigned long *lp;
	int n;
{
	register unsigned char *cp;
	register int t;

	/* XXX unroll loop some */
	while (n-- > 0) {
		cp = (unsigned char *)lp;
		t = cp[3]; cp[3] = cp[0]; cp[0] = t;
		t = cp[2]; cp[2] = cp[1]; cp[1] = t;
		lp++;
	}
}

/*
 * Fetch a contiguous directory item.
 */
static int
TIFFFetchData(fd, dir, cp)
	int fd;
	TIFFDirEntry *dir;
	char *cp;
{
	int cc, w;

	w = datawidth[dir->tdir_type];
	cc = dir->tdir_count * w;
	if (lseek(fd, dir->tdir_offset, 0) == dir->tdir_offset &&
	    read(fd, cp, cc) == cc) {
		if (swabflag) {
			switch (dir->tdir_type) {
			case TIFF_SHORT:
				TIFFSwabArrayOfShort((unsigned short *)cp, dir->tdir_count);
				break;
			case TIFF_LONG:
				TIFFSwabArrayOfLong((unsigned long *)cp, dir->tdir_count);
				break;
			case TIFF_RATIONAL:
				TIFFSwabArrayOfLong((unsigned long *)cp, 2*dir->tdir_count);
				break;
			}
		}
		return (cc);
	}
	{ char msg[32];
	  sprintf(msg, "data for tag %u", dir->tdir_tag);
	  ReadError(msg);
	}
	/*NOTREACHED*/
	return 0;
}

void ReadError(what)
	char *what;
{
	Fatal("Error while reading %s", what);
}

/*

Created: 10/15/90 aozer from Sam Leffler's tiffdump.c

Modifications:

*/

