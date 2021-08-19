/* PostScript Device Package Type Definitions

Copyright (c) 1984, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Edit History:
Jim Sandman: Mon Aug 14 09:31:28 1989
Bill Paxton: Sat Oct 17 09:29:15 1987
Ivor Durham: Wed Aug 17 14:08:52 1988
Linda Gass: Wed Dec  2 14:27:15 1987
Joe Pasqua: Wed Dec 14 14:29:28 1988
Paul Rovner: Tue Aug 22 10:49:05 1989
Ed Taft: Thu Dec 14 12:07:02 1989
End Edit History.
*/

#ifndef DEVICETYPES_H
#define DEVICETYPES_H

#include PUBLICTYPES
#include ENVIRONMENT
#include FP

typedef struct _t_Device *PDevice;

typedef short int DevShort;
#define minDevShort (0x8000)
#define maxDevShort (0x7fff)

typedef long int DevLong;
#define minDevLong (0x80000000l)
#define maxDevLong (0x7fffffffl)

typedef long int DevFixed;
/* Fixed constants */
#define minDevFixed	(0x80000000l)
#define maxDevFixed	(0x7fffffffl)
#define	HALF		(1<<15)
#define	ONE		(1<<16)

/* Macros to convert from Fixed to short */
#define	IntPart(a)	((a)>>16)
#define FracPart(a)	((a)&0xFFFFL)
/* Macro to round fixeds */
#define	Round(a)	(((a)+(1<<15))>>16)
/* short->fixed */
#define	Fix(a)		(((int)(a))<<16)

typedef short unsigned int DevUnsigned;
#define minDevUnsigned (0x0000)
#define maxDevUnsigned (0xffff)

typedef DevShort DevUnique;
typedef unsigned char DevPrivate;

typedef struct _t_DevInterval
    {
    DevShort l;
    DevShort g;
    } DevInterval;
#define minDevInterval minDevShort
#define maxDevInterval maxDevShort

typedef struct _t_DevBounds
    {
    DevInterval x;
    DevInterval y;
    } DevBounds;


typedef struct _t_DevLInterval
    {
    DevLong l;
    DevLong g;
    } DevLInterval;

typedef struct _t_DevLBounds
    {
    DevLInterval x;
    DevLInterval y;
    } DevLBounds;


typedef struct _t_DevPoint
    {
    DevLong x;
    DevLong y;
    } DevPoint;

typedef struct _t_DevFixedPoint
    {
    DevFixed x;
    DevFixed y;
    } DevFixedPoint;

typedef struct _t_DevTrapEdge
    {
    DevShort xl;
    DevShort xg;
    DevFixed ix;
    DevFixed dx;
    } DevTrapEdge;

typedef struct _t_DevTrap
    {
    DevInterval y;
    DevTrapEdge l;
    DevTrapEdge g;
    } DevTrap;

typedef struct _t_DevRun
    {
    DevBounds bounds;
    DevShort datalen;
    DevShort *data;
    DevShort *indx;
    } DevRun;

typedef struct _t_MaskRec
    {
    DevUnsigned width;
    DevUnsigned height;
    unsigned char *data;
    BitField cached:1;
    BitField unused:15;
    Card16 maskID;
    } MaskRec, *PMask;

typedef struct _t_DevMask
    {
    DevPoint dc;
    PMask mask;
    } DevMask;

typedef struct _t_DevGamutTransferRec *DevGamutTransfer;

typedef struct _t_DevRenderingRec *DevRendering;

typedef struct _t_DevImSampleDecode {
    real min, max;
    } DevImSampleDecode;
  
typedef struct _t_DevImageSource
    {
    unsigned char **samples;
    DevImSampleDecode *decode;
    DevShort colorSpace;
    DevShort nComponents;
    DevShort sourceID;
    DevShort wbytes;
    DevShort height;
    Card8 bitspersample;
    Card8 interleaved;
    procedure (*sampleProc)();
      /* integer sample; DevInputColor *result; DevImageSource *source;*/
    char *procData;
    } DevImageSource;

#define IMG_GRAY_SAMPLES 0

#define IMG_RED_SAMPLES 0
#define IMG_GREEN_SAMPLES 1
#define IMG_BLUE_SAMPLES 2

#define IMG_CYAN_SAMPLES 0
#define IMG_MAGENTA_SAMPLES 1
#define IMG_YELLOW_SAMPLES 2
#define IMG_BLACK_SAMPLES 3

#define IMG_L_SAMPLES 0
#define IMG_A_SAMPLES 1
#define IMG_B_SAMPLES 2
  
#define IMG_INTERLEAVED_SAMPLES 0


typedef struct _t_DevImageInfo
    {
    Mtx *mtx;			/* maps from device space to image space */
    DevBounds sourcebounds;	/* source rectangle within source pixel
				 * buffer; given in pixels, not bits */
    DevPoint sourceorigin;	/* image space coords of minx, miny corner
                                 * of source data rectangle */
    DevTrap *trap;		/* pointer to first in array of traps */
    DevShort trapcnt;		/* number of trapezoids */
    } DevImageInfo;

typedef struct _t_DevTfrFcn {
    DevPrivate *priv;
    unsigned char *white;
    unsigned char *red;
    unsigned char *green;
    unsigned char *blue;
    DevShort *ucr;
    unsigned char *bg;
    } DevTfrFcn;
    
typedef struct _t_DevImage
    {
    DevImageInfo info;
    boolean imagemask:8;
    boolean invert:8;
    DevShort unused;
    DevTfrFcn *transfer;
    DevImageSource *source;
    DevGamutTransfer gt;
    DevRendering r;
    } DevImage;

typedef enum
    {
    noneType,
    trapType,
    runType,
    maskType,
    imageType,
    } DevPrimType;

typedef struct _t_DevPrim
    {
    DevPrimType type;
    DevBounds bounds;
    struct _t_DevPrim *next;
    DevPoint xaOffset;
    DevPrivate *priv;
    DevShort items;
    DevShort maxItems;
    union
        {
	DevPrivate *value;
	DevTrap    *trap;
	DevRun     *run;
	DevMask    *string;
	DevMask    *mask;
	DevImage   *image;
        } value;
    } DevPrim;

typedef struct _t_DevInputColor {
    union {
      real gray;
      struct {
        real red, green, blue;
        } rgb;
      struct {
        real cyan, magenta, yellow, black;
        } cmyk;
      real lightness;
      struct {
        real l, a, b;
        } lab;
      real tint;
      } value;
    } DevInputColor;

typedef struct _t_DevColorRec *DevColor;
    
typedef struct _t_DevWhitePoint {
  real Xn, Yn, Zn;
  } DevWhitePoint;
  
typedef struct _t_DevScreen {
    DevPrivate *priv;
    DevShort width;
    DevShort height;
    unsigned char *thresholds;
    } DevScreen;
    
typedef struct _t_DevHalftone {
    DevPrivate *priv;
    DevScreen *red;
    DevScreen *green;
    DevScreen *blue;
    DevScreen *white;
    } DevHalftone;
    
typedef struct _t_DevMarkInfo
    {
    DevPoint offset;
    DevHalftone *halftone;
    DevPoint screenphase;
    DevColor color;
    DevPrivate *priv;
    boolean overprint;
    } DevMarkInfo;

typedef struct _t_MakeMaskDevArgs
    {
    DevShort width, height;
    integer cacheThreshold, compThreshold;
    PMtx mtx;
    PMask *mask;
    DevFixedPoint *offset;
    } MakeMaskDevArgs, *PMakeMaskDevArgs;

typedef struct _t_DeviceInfoArgs
    {
    integer dictSize;
    procedure (*AddIntEntry)(/*
      char *key; integer val; DeviceInfoArgs *args */);
    procedure (*AddRealEntry)(/*
      char *key; PReal val; DeviceInfoArgs *args */);
    procedure (*AddStringEntry)(/*
      char *key, val; DeviceInfoArgs *args */);
    } DeviceInfoArgs;

typedef struct _t_CharMetrics
    {
    DevFixedPoint incr;
    DevFixedPoint offset;
    } CharMetrics;

#define PREBUILT_OUTLINE 0
#define PREBUILT_CLOSEST 1
#define PREBUILT_TRANSFORMED 2

typedef struct _t_PreBuiltArgs
    {
    char *name;
    integer nameLength;
    integer font;
    integer cacheThreshold, compThreshold;
    PMtx matrix;
    boolean bitmapWidths:8;
    Card8 exactSize, inBetweenSize, transformedChar; 
    PMask mask;
    CharMetrics *horizMetrics, *vertMetrics;
    } PreBuiltArgs;

#define DEVGRAY_COLOR_SPACE -1
#define DEVRGB_COLOR_SPACE -2
#define DEVCMYK_COLOR_SPACE -3
#define CIELIGHTNESS_COLOR_SPACE -4
#define CIELAB_COLOR_SPACE -5


typedef struct _t_DevFlushMaskArgs
    {
    char *name;
    integer nameLength;
    integer font;
    PMtx matrix;
    CharMetrics *horizMetrics, *vertMetrics;
    } DevFlushMaskArgs;

typedef char * (*CharNameProc)( 
    /* integer index; integer *nameLength; char *procData; */);
    
typedef enum {
  inside,
  outside,
  overlap
  } BBoxCompareResult;

#endif DEVICETYPES_H

