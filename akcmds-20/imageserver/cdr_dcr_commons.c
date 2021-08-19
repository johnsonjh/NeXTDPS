/******************
This is valuable proprietary information and trade secrets of Creative Circuits
Corporation and is not to be disclosed or released without the prior written
permission of an officer of the company.
  
Copyright (c) 1989 by Creative Circuits Corporation.
  
cdr_dcr_commons.c
*****************/

/*
 * #include <stdlib.h> 
 */


#include "gStruct.h"



/* #define		fsRtDirID		2  */




give_me_space_mac(dg)
    gvl            *dg;
{

    dg->y_strip = (unsigned char *)malloc((long)(dg->lx + 16) * 8);
    dg->u_strip = (unsigned char *)malloc((long)(dg->lx + 16) * 8);
    dg->v_strip = (unsigned char *)malloc((long)(dg->lx + 16) * 8);
    dg->k_strip = (unsigned char *)malloc((long)(dg->lx + 16) * 8);

    dg->y_xstrip = (int *)malloc((long)64 * (dg->hy * dg->vy) * sizeof(int));
    dg->u_xstrip = (int *)malloc((long)64 * (dg->hu * dg->vu) * sizeof(int));
    dg->v_xstrip = (int *)malloc((long)64 * (dg->hv * dg->vv) * sizeof(int));
    dg->k_xstrip = (int *)malloc((long)64 * (1) * sizeof(int));

    dg->y_qstrip = (int *)malloc((long)64 * (dg->hy * dg->vy) * sizeof(int));
    dg->u_qstrip = (int *)malloc((long)64 * (dg->hu * dg->vu) * sizeof(int));
    dg->v_qstrip = (int *)malloc((long)64 * (dg->hv * dg->vv) * sizeof(int));
    dg->k_qstrip = (int *)malloc((long)64 * (1) * sizeof(int));

    dg->y_dqstrip = (int *)malloc((long)64 * (dg->hy * dg->vy) * sizeof(int));
    dg->u_dqstrip = (int *)malloc((long)64 * (dg->hu * dg->vu) * sizeof(int));
    dg->v_dqstrip = (int *)malloc((long)64 * (dg->hv * dg->vv) * sizeof(int));
    dg->k_dqstrip = (int *)malloc((long)64 * (1) * sizeof(int));

    dg->yline = (int *)malloc((long)(dg->lx + 16) * sizeof(int) + 2);
    dg->uline = (int *)malloc((long)(dg->lx + 16) * sizeof(int) + 2);
    dg->vline = (int *)malloc((long)(dg->lx + 16) * sizeof(int) + 2);
    dg->kline = (int *)malloc((long)(dg->lx + 16) * sizeof(int) + 2);
    dg->rgbline = (char *)malloc((long)4096 * 4);	/** 4 for B,G,R,alpha **/


    if (dg->v_strip == NULL | dg->v_dqstrip == NULL | dg->rgbline == NULL) {
	printf(" \n not enough ram \n");
    }
}


take_your_space_mac(dg)
    gvl            *dg;
{
    free(dg->y_strip);
    free(dg->u_strip);
    free(dg->v_strip);
    free(dg->k_strip);

    free(dg->y_xstrip);
    free(dg->u_xstrip);
    free(dg->v_xstrip);
    free(dg->k_xstrip);

    free(dg->y_qstrip);
    free(dg->u_qstrip);
    free(dg->v_qstrip);
    free(dg->k_qstrip);

    free(dg->y_dqstrip);
    free(dg->u_dqstrip);
    free(dg->v_dqstrip);
    free(dg->k_dqstrip);

    free(dg->yline);
    free(dg->uline);
    free(dg->vline);
    free(dg->kline);

    free(dg->rgbline);
}



fill_report(dg)
    gvl            *dg;
{
    dg->ImageReport.NumberOfLines = dg->lx;
    dg->ImageReport.PixelsPerLine = dg->ly;
    dg->ImageReport.CompressedSize = dg->bs_nbytes;
    dg->ImageReport.PixelSize = dg->pixel_size_in > 3 ? 3 : dg->pixel_size_in;
    dg->ImageReport.Factor = dg->factor;
    dg->ImageReport.ElapsedTime = dg->stop_t - dg->strt_t;
    dg->ImageReport.CoderType = dg->cdr_type;
    dg->ImageReport.XformType = dg->xfm_type;
    dg->ImageReport.Sampling = dg->sampling;
}




report_open_failure(dg, fname)
    gvl            *dg;
    char           *fname;
{
    printf("\nOpen failure for file %s", fname);
    exit(0);
}



report_invalid_header(dg)
    gvl            *dg;
{
    printf("\nInvalid bitstream header\n");
    exit(0);
}



xBlockMove(src, dest, size)
    Ptr             src, dest;
    long            size;
{
    register long   i;
    for (i = 0; i < size; i++)
	*dest++ = *src++;
}


#define TOLOWER(c) ((c>='A'&&c<='Z')?(c-'A'+'a'):c)
#define TOUPPER(c) ((c>='a'&&c<='z')?(c-'a'+'A'):c)


/* 
 * getopt( argc, argv, optnam, optval ) 
 *
 * Routine to search for and evaluate an option in the command line argument
 * list. 
 *
 * An option in the command line must be framed by spaces and must be preceded
 * by a slash ('/') or a minus('-'). Options may consist of only the option
 * name or may be followed by and equal sign ('=') and an expression. If only
 * the option name is present, the option takes the value of -1.  If an
 * expression is present, the option takes the value of the evaluated
 * expression. 
 *
 * Examples: 
 *
 * /opt1			Option name is 'opt1', option value is -1. 
 *
 * -opt2=123		Option name is 'opt2', option value is 123. 
 *
 * /opt3=value		Option name is 'opt3', option value is 'value'. 
 *
 * -opt4=0.5		Option name is 'opt4', option value is 0.5. 
 *
 * The routine, getopt(), searches for a designated option name in the argument
 * list and, if found, evaluates the option value.  getopt() returns the
 * relative argument position if the option is present, 0 otherwise. 
 * Arguments of getopt() are: 
 *
 * argc			Total number of command line arguments. 
 *
 * argv			Array of pointers to command line argument strings. 
 *
 * optnam			Option name string to search for. 
 *
 * optval			Pointer to option value used to return evaluated
 * result. If option name is not present, optval is unaffected. 
 *
 */

getopt(dg, argc, argv, optnam, optval)
    gvl            *dg;
    int             argc, *optval;
    char           *argv[], optnam[];
{
    int             i, j, arg;
    char           *p1, *p2, *p3;
    long            intval;

    for (arg = 0; arg < argc; arg++) {

	p1 = argv[arg];

	if (*p1 == '/' || *p1 == '-') {
	    p1++;
	    i = 0;
	    while (optnam[i] == TOLOWER(*p1) && optnam[i]) {
		i++;
		p1++;
	    }
	    if (optnam[i]) {
		while (optnam[i] && optnam[i] != '=')
		    i++;
	    }
	    if (!optnam[i]) {

		if (!(*p1)) {
		    *optval = -1;
		    return (arg);
		}
		if (*p1++ == '=') {
		    if (*p1 == '0' && TOUPPER(*(p1 + 1)) == 'X') {
			p2 = p1 + 2;
			p3 = p2;
			while ((*p2 >= '0' && *p2 <= '9')
			    || (TOUPPER(*p2) >= 'A' && TOUPPER(*p2) <= 'F'))
			    p2++;
			if (!*p2) {
			    sscanf(p3, "%x", optval);
			    return (arg);
			}
		    }
		    p2 = p1;
		    if (*p2 == '-' || *p2 == '+')
			p2++;
		    p3 = p2;
		    while (*p2 >= '0' && *p2 <= '9')
			p2++;
		    if (!*p2) {
			sscanf(p1, "%ld", &intval);
			*optval++ = intval & 0xffff;
			intval = intval >> 16;
			if ((intval != 0 && intval != -1) || (p2 - p3 > 5))
			    *optval = intval;
			return (arg);
		    } else if (*p2++ == '.') {
			while (*p2 >= '0' && *p2 <= '9')
			    p2++;
			if (!*p2) {
			    sscanf(p1, "%f", optval);
			    return (arg);
			} else {
			    strcpy((char *)optval, p1);
			    return (arg);
			}
		    } else {
			strcpy((char *)optval, p1);
			return (arg);
		    }

		}
	    }
	}
    }
    return (0);
}
