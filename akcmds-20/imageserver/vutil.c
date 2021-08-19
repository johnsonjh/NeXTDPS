
/*********************************************************************
  
This is valuable proprietary information and trade secrets of Creative Circuits
Corporation and is not to be disclosed or released without the prior written
permission of an officer of the company.
  
Copyright (c) 1989 by Creative Circuits Corporation.
  
  
		VUTIL.C
  
	Provides a generalized video utility interface
  
  
*********************************************************************/


#include "gStruct.h"
#import <appkit/tiff.h>
#import "nxjpeg.h"


typedef union {
    int             ipar;
    float           fpar;
    char            ppar;
}               par;
int             InLine;

v_s(dg, service, frametype, p1, p2, p3, p4, p5)
    gvl            *dg;
    int             service, frametype;
    par            *p1;
    par            *p2;
    par            *p3;
    par            *p4;
    par            *p5;
{
    int             bps, spp, h, w;
    switch (frametype) {
    case VS_TARGA_FILE:
    case MONOCHROME_NHD:	/* needed for monochrome out */
	switch (service) {
	case VS_INIT_I:	/* (,,vuifh,lx) */
	    dg->vunpels_i = NXJPegGlobs.w;
	    dg->pixel_size_out = dg->pixel_size_in = NXJPegGlobs.spp;

	    dg->vuraster_size_i = dg->vunpels_i * NXJPegGlobs.spp;
	    dg->lx = NXJPegGlobs.w;
	    dg->ly = NXJPegGlobs.h;
	    return (0);
	case VS_INIT_O:	/* (,,dg->vuofh,vunpels_o,vunlines_o,pixel_siz
				 * e,datatype) */
	    NXJPegGlobs.w = dg->vunpels_o = p2->ipar;
	    NXJPegGlobs.h = dg->vunlines_o = p3->ipar;
	    /* printf("pixel out %d  cn %d\n", dg->pixel_size_out, dg->cn); */
	    NXJPegGlobs.spp = dg->pixel_size_out;
	    dg->vuraster_size_o = dg->vunpels_o * dg->pixel_size_out;
	    w = (NXJPegGlobs.w + 7) & ~7;	/* multiple of 8 safe plus 8 */
	    h = (NXJPegGlobs.h + 7) & ~7;	/* multiple of 8 safe plus 8 */
	    NXJPegGlobs.tiffdata = NXJPegGlobs.tiffdataalloced = (unsigned char *)
		malloc(w * h * NXJPegGlobs.spp);
	    return (0);
	case VS_GETLINE:	/* (,,line) */
	    NXJPegGlobs.linenum++;
	    if (NXJPegGlobs.linenum == NXJPegGlobs.h)
		return (0);	/* no more data */
	    bcopy(NXJPegGlobs.tiffdata, &p1->ppar, dg->vuraster_size_i);
	    NXJPegGlobs.tiffdata += dg->vuraster_size_i;
	    return (0);
	case VS_PUTLINE:	/* (,,line) */
	    if (!NXJPegGlobs.tiffdata)
		return (1);
	    bcopy(&p1->ppar, NXJPegGlobs.tiffdata, dg->vuraster_size_o);
	    NXJPegGlobs.tiffdata += dg->vuraster_size_o;
	    return (0);
	default:		/* service */
	    return (2);
	}
	/* return(4);	 *//* would expect each frame type to return */
	break;

    case 999:
	switch (service) {
	case VS_INIT_I:	/* (,,vuifh,lx) */
	    dg->vuifh = p1->ipar;
	    if (read(dg->vuifh, (char *)&dg->vuhdr, 18) <= 0)
		return (1);
	    dg->vunpels_i = dg->vuhdr.x_msb << 8 | dg->vuhdr.x_lsb;
	    dg->vuraster_size_i = dg->vunpels_i * (dg->vuhdr.databits / 8);
	    return (0);
	case VS_INIT_O:	/* (,,dg->vuofh,vunpels_o,vunlines_o,pixel_siz
				 * e,datatype) */
	    if (!(dg->vuofh = p1->ipar))
		return (0);
	    dg->vunpels_o = p2->ipar;
	    dg->vunlines_o = p3->ipar;
	    dg->vuhdr.x_lsb = dg->vunpels_o;
	    dg->vuhdr.x_msb = dg->vunpels_o >> 8;
	    dg->vuhdr.y_lsb = dg->vunlines_o;
	    dg->vuhdr.y_msb = dg->vunlines_o >> 8;
	    dg->vuhdr.databits = p4->ipar * 8;
	    dg->vuhdr.textsize = 0;
	    dg->vuhdr.datatype = p5->ipar;
	    dg->vuhdr.maptype = 0;
	    dg->vuhdr.maporig_lsb = 0;
	    dg->vuhdr.maporig_msb = 0;
	    dg->vuhdr.maplength_lsb = 0;
	    dg->vuhdr.maplength_msb = 0;
	    dg->vuhdr.cmapbits = 0;
	    dg->vuhdr.xoffset_msb = 0;
	    dg->vuhdr.yoffset_msb = 0;
	    dg->vuhdr.xoffset_lsb = 0;
	    dg->vuhdr.yoffset_lsb = 0;
	    dg->vuhdr.imdesc = 0;
	    if (write(dg->vuofh, (char *)&dg->vuhdr, 18) <= 0)
		return (1);
	    dg->vuraster_size_o = dg->vunpels_o * (dg->vuhdr.databits / 8);
	    return (0);
	case VS_GETLINE:	/* (,,line) */
	    return (read(dg->vuifh, &p1->ppar, dg->vuraster_size_i));
	case VS_PUTLINE:	/* (,,line) */
	    return (write(dg->vuofh, &p1->ppar, dg->vuraster_size_o));
	default:		/* service */
	    return (2);
	}
	/* return(4);	 *//* would expect each frame type to return */
	break;

    case VS_NOHDR_FILE:	/* switch frametype */
	switch (service) {
	case VS_INIT_I:	/* (,,vuifh) */
	    dg->vuifh = p1->ipar;
	    dg->vunpels_i = p2->ipar;
	    dg->vuraster_size_i = dg->vunpels_i;
	    return (0);
	case VS_INIT_I2:	/* (,,vuifh) */
	    dg->vuifh2 = p1->ipar;
	    dg->vunpels_i2 = p2->ipar;
	    dg->vuraster_size_i2 = dg->vunpels_i2;
	    return (0);
	case VS_INIT_O:	/* (,,vuofh) */
	    dg->vuofh = p1->ipar;
	    dg->vunpels_o = p2->ipar;
	    dg->vuraster_size_o = dg->vunpels_o;
	    return (0);
	case VS_GETLINE:	/* (,,line) */
	    if (read(dg->vuifh, &p1->ppar, dg->vuraster_size_i) <= 0)
		return (1);
	    return (0);
	case VS_GETLINE2:	/* (,,line) */
	    if (read(dg->vuifh2, &p1->ppar, dg->vuraster_size_i2) <= 0)
		return (1);
	    return (0);
	case VS_PUTLINE:	/* (,,line) */
	    if (write(dg->vuofh, &p1->ppar, dg->vuraster_size_o) <= 0)
		return (1);
	    return (0);
	default:		/* service */
	    return (2);		/* invalid service */
	}
    default:			/* frametype */
	return (3);		/* invalid frametype */
    }
}
