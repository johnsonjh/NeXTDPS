
/****************************************************************
  
This is valuable proprietary information and trade secrets of Creative Circuits
Corporation and is not to be disclosed or released without the prior written
permission of an officer of the company.
  
Copyright (c) 1989 by Creative Circuits Corporation.
  
  
 coordB.c
  
****************************************************************/

#include "c3sim.h"

#define sign_bit 0x8000

typedef unsigned char Pixel;

/**
coord(nbits, id, format, npixels, ybfr, ibfr, qbfr, rbfr, gbfr, bbfr)
	id - coordinate transformation id (defined in c3sim.h)
		RGB2YIQ
		YIQ2RGB
		RGB2YUV
		YUV2RGB
	format - RGB format
		RGB16 - 16 bit RGB a la TRUEVISION
		RGB24 - 24 bit RGB " "  "
		RGB32 - 32 bit RGB " "  "
		RGB   - seperate 8 bit planes a la MATROX
	npixels - The number of pixels to convert
	ybfr,ibfr,qbfr are the seperate YIQ buffers
	rbfr is the rgb buffer for formats RGB16,RGB24,RGB32  ... otherwise
	rbfr is the RED buffer   for RGB format
	gbfr is the GREEN buffer for RGB format 
	bbfr is the BLUE  buffer for RGB format 
**/


/** static int outOfBounds[8] = {0,255,255,255,0,0,0,0};**/

static int      outOfBounds444[8] = {0, 255, 255, 255, 0, 0, 0, 0};
static int      outOfBounds[4] = {0, 255, 0, 0,};
static int      outOfBounds422[8] = {0, 0x1FE0, 0x1FE0, 0x1FE0, 0, 0, 0, 0};

unsigned long   lt0, lt1, lt2, lr0, lr1, lr2;

int 
coord(nbits, id, format, npixels, ybfr, ibfr, qbfr, rbfr, gbfr, bbfr)
    int             nbits, id, format, npixels;
    Pixel          *rbfr, *gbfr, *bbfr;
    Pixel          *ybfr, *ibfr, *qbfr;
{
    short           i, j;
    int             np2;




#define RY 0x1323
#define GY 0x2591
#define BY 0x074c

#define RU 0x0ad1
#define GU 0x1539
#define BU 0x2000

#define RV 0x2000
#define GV 0x1aca
#define BV 0x0534


#define UG 0x15ffL
#define UB 0x716cL

#define VR 0x59b5L
#define VG 0x2db7L



    switch (id) {

    default:
	return (0);



	/**                  **/
	/** 9 bit data stuff **/
	/**                  **/


    case RGB2YUV444:
	for (i = 0; i < npixels; i++) {
	    register long   red, green, blue;
	    register long   longtemp;

	    blue = *rbfr++;
	    green = *rbfr++;
	    red = *rbfr++;
	    *ybfr++ = ((red * RY + green * GY + blue * BY + 0x2000) >> 14);

	    *ibfr++ = ((-red * RU - green * GU + blue * BU + 0x2000) >> 14);

	    *qbfr++ = ((red * RV - green * GV - blue * BV + 0x2000) >> 14);

	}
	return (1);

    case YUV2RGB444:
	for (i = 0; i < npixels; i++) {
	    char            u, v;
	    unsigned long   y;
	    register long   rl, gl, bl;
	    register unsigned char r, g, b;
	    register unsigned long testbits;

	    y = ((long)(*ybfr++) << 14) + 0x2000;
	    u = *ibfr++;
	    v = *qbfr++;

	    rl = y + VR * v;
	    r = rl >> 14;
	    testbits = (rl & 0xc00000L);
	    if (testbits) {
		if (testbits == 0x400000L)
		    r = 0xff;
		else
		    r = 0;
	    }
	    gl = y - UG * u - VG * v;
	    g = gl >> 14;
	    testbits = (gl & 0xc00000L);
	    if (testbits) {
		if (testbits == 0x400000L)
		    g = 0xff;
		else
		    g = 0;
	    }
	    bl = y + UB * u;
	    b = bl >> 14;
	    testbits = (bl & 0xc00000L);
	    if (testbits) {
		if (testbits == 0x400000L)
		    b = 0xff;
		else
		    b = 0;
	    }
	    *rbfr++ = b;
	    *rbfr++ = g;
	    *rbfr++ = r;
	} /** end of the for **/
	return (1);


    case RGB2YUV422:
	np2 = (npixels + 1) / 2;
	for (i = 0; i < np2; i++) {
	    register long   red, green, blue;
	    register long   red0, green0, blue0, red1, green1, blue1;

	    blue0 = *rbfr++;
	    green0 = *rbfr++;
	    red0 = *rbfr++;
	    blue1 = *rbfr++;
	    green1 = *rbfr++;
	    red1 = *rbfr++;

	    *ybfr++ = ((red0 * RY + green0 * GY + blue0 * BY + 0x2000) >> 14);

	    *ybfr++ = ((red1 * RY + green1 * GY + blue1 * BY + 0x2000) >> 14);


	    red = (red0 + red1 + 1) >> 1;
	    green = (green0 + green1 + 1) >> 1;
	    blue = (blue0 + blue1 + 1) >> 1;

	    *ibfr++ = ((-red * RU - green * GU + blue * BU + 0x2000) >> 14);

	    *qbfr++ = ((red * RV - green * GV - blue * BV + 0x2000) >> 14);
	}
	return (1);




    case YUV2RGB422:
	np2 = (npixels + 1) / 2;
	for (i = 0; i < np2; i++) {

	    char            u, v;
	    unsigned long   y0, y1;
	    register long   rl, gl, bl;
	    register unsigned char r0, g0, b0, r1, g1, b1;
	    register unsigned long testbits;

	    y0 = ((long)(*ybfr++) << 14) + 0x2000;
	    y1 = ((long)(*ybfr++) << 14) + 0x2000;
	    u = *ibfr++;
	    v = *qbfr++;

	    rl = y0 + VR * v;
	    r0 = rl >> 14;
	    testbits = (rl & 0xc00000L);
	    if (testbits) {
		if (testbits == 0x400000L)
		    r0 = 0xff;
		else
		    r0 = 0;
	    }
	    gl = y0 - UG * u - VG * v;
	    g0 = gl >> 14;
	    testbits = (gl & 0xc00000L);
	    if (testbits) {
		if (testbits == 0x400000L)
		    g0 = 0xff;
		else
		    g0 = 0;
	    }
	    bl = y0 + UB * u;
	    b0 = bl >> 14;
	    testbits = (bl & 0xc00000L);
	    if (testbits) {
		if (testbits == 0x400000L)
		    b0 = 0xff;
		else
		    b0 = 0;
	    }
	    *rbfr++ = b0;
	    *rbfr++ = g0;
	    *rbfr++ = r0;


	    rl = y1 + VR * v;
	    r1 = rl >> 14;
	    testbits = (rl & 0xc00000L);
	    if (testbits) {
		if (testbits == 0x400000L)
		    r1 = 0xff;
		else
		    r1 = 0;
	    }
	    gl = y1 - UG * u - VG * v;
	    g1 = gl >> 14;
	    testbits = (gl & 0xc00000L);
	    if (testbits) {
		if (testbits == 0x400000L)
		    g1 = 0xff;
		else
		    g1 = 0;
	    }
	    bl = y1 + UB * u;
	    b1 = bl >> 14;
	    testbits = (bl & 0xc00000L);
	    if (testbits) {
		if (testbits == 0x400000L)
		    b1 = 0xff;
		else
		    b1 = 0;
	    }
	    *rbfr++ = b1;
	    *rbfr++ = g1;
	    *rbfr++ = r1;
	} /** end of the for **/
	return (1);




    case RGB2YUV:
	printf("\n Type 'RGB2YUV' not implemented");
	return (1);

    case YUV2RGB:
	printf("\n Type 'YUV2RGB' not implemented");
	return (1);

    case CCIR2YUV:
	printf("\n Type 'CCIR2YUV' not implemented");
	return (1);

    case YUV2CCIR:
	printf("\n Type 'YUV2CCIR' not implemented");
	return (1);

    } /* end of switch */

}
