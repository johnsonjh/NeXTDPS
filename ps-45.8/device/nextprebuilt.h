/*****************************************************************************

    nextprebuilt.h
    Integrated + nextified prebuilt code

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 24May90 by Peter + Terry from *prebuil*.h

    Modified:

******************************************************************************/

#ifndef NEXTPREBUILT_H
#define NEXTPREBUILT_H

#import "hostdict.h"
#import "prebuiltformat.h"

#define uchar unsigned char
#define ushort unsigned short

typedef struct {
    int base;
    int length;
    ushort *startpos;
    uchar *code;
} PrebuiltEncoding;

typedef struct {
    PrebuiltFile	*file;
    PrebuiltWidth	*widths;
    PrebuiltMatrix	*matrices;
    PrebuiltVertWidths	*vertWidths;
    PrebuiltEncoding	*encoding;
    Stm   stm;
} NextPrebuiltFont;

/* Use prebuilt if the desired size is strictly greater than the smallest 
 * available prebuilt size, but no smaller than MINIMUMSIZE, strictly less than
 * 1.25 times the largest available prebuilt size, but no larger
 * than MAXIMUMSIZE
 */
#define MINIMUMSIZE 5.0
#define MAXIMUMSIZE 24.0

#endif	NEXTPREBUILT_H
