/*
	font.h
	Copyright 1988, NeXT, Inc.
*/

#ifndef FONT_H
#define FONT_H


#define NX_IDENTITYMATRIX ((float *) 0)
#define NX_FLIPPEDMATRIX  ((float *) -1)

#define NX_FONTHEADER	1
#define NX_FONTMETRICS	2
#define NX_FONTWIDTHS	4
#define NX_FONTCHARDATA	8
#define NX_FONTKERNING	16
#define NX_FONTCOMPOSITES	32

/* questions:	unencoded characters???
*/

/* 
 *  first pass at four additional space characters, whose ascii codes
 *  we have to talk to Adobe about. Figure is aka non-breaking space
 */

#define NX_FIGSPACE	((unsigned short)0x80)
#define NX_EMSPACE	((unsigned short)0x81)
#define NX_ENSPACE	((unsigned short)0x82)
#define NX_THINSPACE	((unsigned short)0x83)

typedef struct _NXLigature {	/* a ligature */
    char *successor;		/* char that follows current character */
    char *result;		/* the ligature char that results */
} NXLigature;

typedef struct _NXCharData {	/* character data besides widths */
    char *name;			/* name of character */
    float charBBox[4];		/* bounding box (llx lly urx ury) */
    short numLigatures;		/* number of ligature pairs */
    NXLigature *ligatures;	/* the ligatures */
} NXCharData;

typedef struct _NXKernTrack {	/* a kerning track */
    int degree;			/* tightness of kerning */
    float minSize;		/* kerning amounts for a min and max */
    float minAmount;		/* point size. */
    float maxSize;
    float maxAmount;
} NXKernTrack;

typedef struct _NXKernPair {	/* a kerning pair */
    char *name1;		/* names of chars to kern */
    char *name2;
    float x, y;			/* amount to move second character */
} NXKernPair;

typedef struct _NXCompositePart { /* part of a composite character */
    char *name;			/* name of char */
    float dx, dy;		/* displacement of part from char origin */
} NXCompositePart;

typedef struct _NXCompositeChar { /* a composite character */
    char *name;			/* name of char */
    short numParts;		/* number of parts making up the char */
    NXCompositePart *parts;	/* var length array of parts */
} NXCompositeChar;

typedef struct _NXFontMetrics { /* data from a afm file */
    char *formatVersion;	/* version of afm file format */
    char *name;			/* name of font for findfont */
    char *fullName;		/* full name of font */
    char *familyName;		/* "font family" name */
    char *weight;		/* weight of font */
    float italicAngle;		/* degrees ccw from vertical */
    char isFixedPitch;		/* is the font mono-spaced? */
    char isScreenFont;		/* is the font a screen font? */
    short screenFontSize;	/* If it is, how big is it? */
    float fontBBox[4];		/* bounding box (llx lly urx ury) */
    float underlinePosition;	/* dist from basline for underlines */
    float underlineThickness;	/* thickness of underline stroke */
    char *version;		/* version identifier */
    char *notice;		/* trademark or copyright */
    char *encodingScheme;	/* default encoding vector */
    float capHeight;		/* top of 'H' */
    float xHeight;		/* top of 'x' */
    float ascender;		/* top of 'd' */
    float descender;		/* bottom of 'p' */
    short hasYWidths;		/* do any chars have non-0 y width? */
    float *widths;		/* character widths in x */
    float *yWidths;		/* y components of width vector */
				/* a NULL if widths are only in x */
    NXCharData *charData;	/* character data besides widths */
    short numKTracks;		/* number of kerning tracks */
    NXKernTrack *kernTracks;	/* the tracks */
    short numKPairs;		/* number of kerning pairs */
    NXKernPair *kernPairs;	/* the kerning pairs */
    short numCompositeChars;	/* number of composite characters */
    NXCompositeChar *compositeChars;	/* the composite chars */
    short numBuffers;		/* # of char buffers in following array */
    char **buffers;		/* buffers of characters, pointed into by */
				/* all these (char *) name fields */
} NXFontMetrics;

typedef struct _NXFaceInfo {
    NXFontMetrics   fontMetrics;/* Information from afm file */
    int             flags;	/* Which font info is present */
    struct {	/* Keeps track of font usage for Conforming PS */
	unsigned int	usedInDoc:1;	/* has font been used in doc? */
	unsigned int	usedInPage:1;	/* has font been used in page? */
	unsigned int	_PADDING:14;
    } fontFlags;
    struct _NXFaceInfo   *nextFInfo;	/* Next faceInfo in the list */
}               NXFaceInfo;




#endif FONT_H


