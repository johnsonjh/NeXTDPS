/******************
This is valuable proprietary information and trade secrets of Creative Circuits
Corporation and is not to be disclosed or released without the prior written
permission of an officer of the company.
  
Copyright (c) 1989 by Creative Circuits Corporation.
  
InitArrays.c
*****************/

#include "gStruct.h"
#ifdef NeXT
#import <streams/streams.h>
extern void    *getsectdata();
NXStream       *
_NXOpenStreamOnMacho(const char *segName, const char *sectName)
{
    const char     *data;
    int             size;

    if (segName && sectName &&
	(data = getsectdata((char *)segName, (char *)sectName, &size))) {
	return NXOpenMemory(data, size, NX_READONLY);
    } else {
	fprintf(stderr, "Failed in _NXOpenStreamOnMacho\n");
	exit(1);
	return NULL;
    }
}
#endif







InitArrays(dg)
    gvl            *dg;
{

    int             InitFh;
    int             i;
    int             k, l, m, n, k1;

#ifdef NeXT
    NXStream       *st;
    st = (NXStream *) _NXOpenStreamOnMacho("__PRIV", "an_init");

    n = NXRead(st, (char *)dg->bitmap, sizeof(dg->bitmap));
    n = NXRead(st, (char *)dg->visibility, sizeof(dg->visibility));
    n = NXRead(st, (char *)dg->snake, sizeof(dg->snake));
    n = NXRead(st, (char *)dg->xfzigzag, sizeof(dg->xfzigzag));

    NXClose(st);
#else
    InitFh = open("an_init.dat", O_RDONLY | O_BINARY);

    n = read(InitFh, (char *)dg->bitmap, sizeof(dg->bitmap));
    n = read(InitFh, (char *)dg->visibility, sizeof(dg->visibility));
    n = read(InitFh, (char *)dg->snake, sizeof(dg->snake));
    n = read(InitFh, (char *)dg->xfzigzag, sizeof(dg->xfzigzag));

    close(InitFh);
#endif

    strcpy(dg->ifor[0], "NOHDR");
    strcpy(dg->ifor[1], "TARGA");
    strcpy(dg->ifor[2], "CST");
    strcpy(dg->ifor[3], "???");
    strcpy(dg->ifor[4], "???");
    strcpy(dg->ifor[5], "COST.. ONE CHANNEL");

    strcpy(dg->xtyp[0], "ZIP");
    strcpy(dg->xtyp[1], "KTAS");
    strcpy(dg->xtyp[2], "SDCT");
    strcpy(dg->xtyp[3], "QSDCT");
    strcpy(dg->xtyp[4], "FDCT");
    strcpy(dg->xtyp[5], "BYPASS");
    strcpy(dg->xtyp[6], "THOM");
    strcpy(dg->xtyp[7], "DPDCT");

    strcpy(dg->ctyp[0], "ZIP");
    strcpy(dg->ctyp[1], "VLSI");
    strcpy(dg->ctyp[2], "PROTO");
    strcpy(dg->ctyp[3], "N8R5");


    dg->pixel_format_array[0] = 0;
    dg->pixel_format_array[1] = M8;
    dg->pixel_format_array[2] = RGB16;
    dg->pixel_format_array[3] = RGB24;
    dg->pixel_format_array[4] = RGB32;
    dg->pixel_format_array[5] = RGB;

    dg->bittest[0] = 0;
    for (i = 1; i < 17; i++) {
	dg->bittest[i] = 1 << (i - 1);
    }

    for (i = 0; i < 16; i++) {
	dg->signmask[i] = -1 << i;
    }

    /** output controls **/
    dg->vlsi_out = 0;		/* Enable cdr outputs , zz, cdr, cma */
    dg->proto_out = 0;		/* Enable cdr outputs , zz, cdr, cma */
    dg->raw_out = 0;		/* Enable raw pixel ascii output     */

    dg->hc_init = 0;		/* generate flat dist codes, else read from
				 * file */
    dg->hh_init = 0;		/* clear histogram counters, else read init
				 * values */
    dg->visi_alt = 0;		/* get alternate visibility matrices thru
				 * filename */

    dg->sh8[0] = 0;
    dg->sh8[1] = 2;
    dg->sh8[2] = 1;
    dg->sh8[3] = 2;
    dg->sh8[4] = 0;
    dg->sh8[5] = 2;
    dg->sh8[6] = 1;
    dg->sh8[7] = 2;

    dg->sh16[0] = 0;
    dg->sh16[1] = 2;
    dg->sh16[2] = 1;
    dg->sh16[3] = 2;
    dg->sh16[4] = 1;
    dg->sh16[5] = 2;
    dg->sh16[6] = 1;
    dg->sh16[7] = 1;
    dg->sh16[8] = 0;
    dg->sh16[9] = 1;
    dg->sh16[10] = 1;
    dg->sh16[11] = 2;
    dg->sh16[12] = 1;
    dg->sh16[13] = 2;
    dg->sh16[14] = 2;
    dg->sh16[15] = 3;

    for (i = 0; i < 64; i++)
	dg->q[i] = 2;

    dg->initdct_flag = 0;

    strcpy(dg->version, "V 1.010");

    dg->rec_fp = 0;

}
