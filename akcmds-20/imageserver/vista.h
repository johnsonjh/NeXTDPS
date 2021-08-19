/********

This is valuable proprietary information and trade secrets of Creative Circuits
Corporation and is not to be disclosed or released without the prior written
permission of an officer of the company.

Copyright (c) 1989 by Creative Circuits Corporation.

vista.h
********/
typedef struct {
	unsigned char textsize;
	unsigned char maptype;
	unsigned char datatype;
	unsigned char  maporig_lsb;
	unsigned char  maporig_msb;
	unsigned char  maplength_lsb;
	unsigned char  maplength_msb;
	unsigned char  cmapbits;
	unsigned char  xoffset_lsb;
	unsigned char  xoffset_msb;
	unsigned char  yoffset_lsb;
	unsigned char  yoffset_msb;
	unsigned char  x_lsb;
	unsigned char  x_msb;
	unsigned char  y_lsb;
	unsigned char  y_msb;
	unsigned char  databits, imdesc;
}hdstruct ;

