/*
  prebuiltformat.h

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version:
Edit History:
Ivor Durham: Wed Sep 28 16:56:14 1988
Linda Gass: Thu Sep 29 10:08:32 1988
Jim Sandman: Fri Apr 21 14:46:23 1989
End Edit History.
*/

#ifndef	PREBUILTFORMAT_H
#define	PREBUILTFORMAT_H

#define PREBUILTFILEVERSION 3

#if SWAPBITS
#define PREBUILT_SUFFIX ".lepf"
#else
#define PREBUILT_SUFFIX ".bepf"
#endif
#define PBSUFFIXLENGTH 5


typedef struct _t_PrebuiltFile {
  short   byteOrder;		/* equal to 0 iff little-endian, otherwise
				 * big-endian */
  short   version;		/* file version number (equal to
				 * PREBUILTFILEVERSION) */
  long    identifier;		/* font's fid */
  long    fontName;		/* file position of name of font (null
				 * terminated) */
  long    characterSetName;	/* file position of name of character set
				 * (null terminated) (equal to either
				 * "ISOLatin1CharacterSet" or the name of the
				 * font) */
  short   numberChar;		/* number of character in character set */
  short   numberMatrix;		/* number of distinct matrices imaged */
  long    widths;		/* file position of the first element of an
				 * array of numberChar PrebuiltWidth's */
  long    names;		/* file position of first of numberChar
				 * character name's (sorted
				 * lexicographically, each null terminated) */
  long    lengthNames;		/* total number of bytes of character name's
				 * (including nulls) */
  long    matrices;		/* file position of the first element of an
				 * array of 1: Unterminated literal
				 * numberMatrix PrebuiltMatrix 's */
  long    masks;		/* file position of the first element of an
				 * array of numberChar*numberMatrix
				 * PrebuiltMask's */
  long    vertWidths;		/* equal to 0 iff vertical data NOT included,
				 * otherwise file position of first element
				 * of an array of numberChar
				 * PrebuiltVertWidths's */
  long    vertMetrics;		/* equal to 0 iff vertical data NOT included,
				 * otherwise file position of first element
				 * of an array of numberChar*numberMatrix
				 * PrebuiltVertMetrics's */
  long    lengthMaskData;	/* total number of bytes of mask data.  Mask
				 * data are positioned in the file in the
				 * same order as their corresponding
				 * PrebuiltMask. */
} PrebuiltFile;

typedef struct _t_PrebuiltMatrix {
  long int a;			/* x' = a*x + c*y + tx (fixed)*/
  long int b;			/* y' = b*x + d*y + ty (fixed)*/
  long int c;			/* (fixed)*/
  long int d;			/* (fixed)*/
  long int tx;			/* (fixed)*/
  long int ty;			/* (fixed)*/
  long int depth;		/* depth of mask pixel */
} PrebuiltMatrix;

typedef struct _t_PrebuiltWidth {
  long int hx;			/* x component of horizontal vector (fixed) */
  long int hy;			/* y component of horizontal vector (fixed) */
}       PrebuiltWidth;

typedef struct _t_PrebuiltMask {
  unsigned char width;		/* width of mask in pixels */
  unsigned char height;		/* height of mask in pixels */
  struct {char hx, hy;}
     maskOffset;		/* horizontal offsets */
  struct {char hx, hy;}
     maskWidth;			/* horizontal widths */
  short pad;			/* pad to make multiple of 4 bytes; */
  long    maskData;		/* file position of mask data ((width * depth
				 * + 7) / 8 * height bytes) */
} PrebuiltMask;

typedef struct _t_PrebuiltVertWidths {
  long int vx;			/* x component of vertical vector; (fixed) */
  long int vy;			/* y component of vertical vector; (fixed) */
} PrebuiltVertWidths;

typedef struct _t_PrebuiltVertMetrics {
  struct {char vx, vy;}
     offset;			/* vertical offsets */
  struct {char vx, vy;}
     width;			/* vertical widths */
} PrebuiltVertMetrics;

#endif	PREBUILTFORMAT_H
