#include "gStruct.h"

/*
 * This is the glue between the jpeg code and NeXT functions. I tried to
 * change as little of CCC code as possible to minimize the effort in getting
 * new updates to newer CCC source. 
 *
 *
 * Planar is handled in a very inefficient manner. It is converted to mesh
 * before compression, & back to planar after compression. 
 */
gvl            *dg;
Globs           NXJPegGlobs;
static int      initvars();
int             Factor;

/*
 * replacement for file read. 
 */
jread(fd, buf, size)
    int             fd;		/* not used */
    unsigned char  *buf;
    int             size;
{
    while (size-- > 0)
	*buf++ = *NXJPegGlobs.jpegdata++;
}

/*
 * replacement for file write. 
 */
jwrite(fd, buf, size)
    int             fd;		/* not used */
    unsigned char  *buf;
    int             size;
{
    while (size-- > 0)
	*NXJPegGlobs.jpegdata++ = *buf++;
}

/*
 * Compress rgb data or 8 bit greyscale and returned compressed jpeg data.
 * jpegdata must be allocated when passed in. 
 */
NXJpegCompress(w, h, spp, factor, planar, tiffdata, jpegdata, sizep)
    int             w, h, spp, planar, factor;
    unsigned char **tiffdata, *jpegdata;
    int            *sizep;
{
    unsigned char  *mesheddata, *alphadata, *inputdata, *inputalphadata;
    unsigned char  *cp1, *cp2;
    unsigned char  *red, *green, *blue, *alpha;
    int             i;
    int             othersize;

    *sizep = 0;
    mesheddata = 0;
    inputdata = 0;
    alphadata = 0;

    if (planar && spp > 1) {
	mesheddata = (unsigned char *)malloc(w * h * 3);
	if (!mesheddata)
	    return;
	cp1 = mesheddata;
	red = tiffdata[0];
	green = tiffdata[1];
	blue = tiffdata[2];
	for (i = w * h; i > 0; i--) {
	    *cp1++ = *red++;
	    *cp1++ = *green++;
	    *cp1++ = *blue++;
	}
	inputdata = mesheddata;
	if (spp == 4) {
	    inputalphadata = tiffdata[3];
	}
    } else {
	if (spp == 4) {
	    alphadata = (unsigned char *)malloc(w * h);
	    if (!alphadata)
		return;
	    cp1 = tiffdata[0];
	    cp2 = alphadata;
	    red = cp1;
	    green = cp1 + 1;
	    blue = cp1 + 2;
	    alpha = cp1 + 3;
	    for (i = w * h; i > 0; i--) {
		*cp1++ = *red;
		red += 4;
		*cp1++ = *green;
		green += 4;
		*cp1++ = *blue;
		blue += 4;
		*cp2++ = *alpha;
		alpha += 4;
	    }
	    inputalphadata = alphadata;
	}
	inputdata = tiffdata[0];
    }

    dg = (gvl *) malloc((long)sizeof(gvl));	/* allocate one big structure 	 */
    bzero(dg, sizeof(gvl));
    NXJPegGlobs.linenum = 0;
    NXJPegGlobs.w = w;
    NXJPegGlobs.h = h;
    NXJPegGlobs.spp = (spp >= 3 ? 3 : 1);
    NXJPegGlobs.tiffdata = (unsigned char *)inputdata;
    NXJPegGlobs.jpegdata = (unsigned char *)jpegdata;


    Factor = factor;

    InitArrays(dg);
    if (spp == 1) {
	dg->sampling = MONO;
	dg->in_frame = MONOCHROME_NHD;
	dg->numberInterleavesPerResync = 0;
	dg->vlsi_out = 0;
	dg->hy = 1;		/* # of horizontal y blocks per interleave */
	dg->vy = 1;		/* # of vertical y blocks per interleave   */
	dg->hu = 0;		/* # of horizontal u blocks per interleave */
	dg->vu = 0;		/* # of vertical u blocks per interleave   */
	dg->hv = 0;		/* # of horizontal v blocks per interleave */
	dg->vv = 0;		/* # of vertical v blocks per interleave   */
	dg->hk = 0;		/* # of horizontal k blocks per interleave */
	dg->vk = 0;		/* # of vertical k blocks per interleave   */
	dg->nb = 1;		/* # of       all  blocks per interleave   */
	dg->cn = 1;		/* # of color components                   */
    } else {
	dg->hy = 2;		/* # of horizontal y blocks per interleave */
	dg->vy = 1;		/* # of vertical y blocks per interleave   */
	dg->hu = 1;		/* # of horizontal u blocks per interleave */
	dg->vu = 1;		/* # of vertical u blocks per interleave   */
	dg->hv = 1;		/* # of horizontal v blocks per interleave */
	dg->vv = 1;		/* # of vertical v blocks per interleave   */
	dg->hk = 0;		/* # of horizontal k blocks per interleave */
	dg->vk = 0;		/* # of vertical k blocks per interleave   */
	dg->nb = 4;		/* # of       all  blocks per interleave   */
	dg->cn = 3;		/* # of color components                   */
    }
    initialize_coder(dg, 0, NULL);
    initvars();

    /*
     * Must do this again. init_coder_variables & initvars will both change
     * to targa by default. 
     */
    if (spp == 1)
	dg->in_frame = MONOCHROME_NHD;

    generateMarkerCode(dg, SOI_MARKER);	/** generate start of image **/
    code_image_frames(dg);
    generateMarkerCode(dg, EOI_MARKER);	/** generate start of image **/

    summarize_coder(dg);
    *sizep = dg->bs_nbytes + 1;
    free(dg);
    if (mesheddata)
	free(mesheddata);

    /*
     * Call ourselves again to do alpha. Just like we were doing greyscale. 
     */
    if (spp == 4) {
	othersize = *sizep;
	NXJpegCompress(w, h, 1, 2, 0, &inputalphadata, jpegdata + othersize, sizep);
	*sizep += othersize;
    }
    if (alphadata)
	free(alphadata);
}

/*
 * Given compressed jpeg data return rgb or greyscale data and dimensions. 
 *
 * This routine returns the data in the tiffdata[] array. If the data is planar,
 * then tiffdata[0] points to a giant chunk which stores all planes
 * consecutively. tiffdata[1], etc still point to the other planes, for
 * convenience. 
 */
NXJpegDecompress(wp, hp, sppp, planar, tiffdata, jpegdata, size)
    int            *wp, *hp, *sppp;
    int             planar;
    unsigned char **tiffdata, *jpegdata;
    int             size;
{
    unsigned char  *red, *green, *blue, *cp1, *cp2, *alphadata, *new;
    unsigned char  *old, *alphastart, *alphatiffdata;
    int             w, h, spp, firstsize, i;
    unsigned char  *safedata;

    dg = (gvl *) malloc((long)sizeof(gvl));	/* allocate one big structure
						 * space */
    bzero(dg, sizeof(gvl));

    /*
     * The CCubed code bogusly assumes that it can read in 4096 blocks. This
     * can cause read protection errors. To avoid this we copy the data into
     * a safe piece of memory. 
     */
    safedata = (unsigned char *)malloc(size + 4096);
    bcopy(jpegdata, safedata, size);
    NXJPegGlobs.jpegdata = safedata;

    dg->ofh = 0;
    dg->ifh = 0;

    /** initialize variables put in the dg structure **/
    InitArrays(dg);
    initialize_decoder(dg, 0, NULL);
    initvars();
    dg->mCode = getNextMarkerCode(dg);	/** generate start of image **/
    handleNextMarkerCode(dg, dg->mCode, SOI_MARKER);	/** if no error, we return **/
    decode_image_frames(dg);
    summarize_decoder(dg);

    *tiffdata = NXJPegGlobs.tiffdataalloced;
    *wp = NXJPegGlobs.w;
    *hp = NXJPegGlobs.h;
    *sppp = NXJPegGlobs.spp;

    free(dg);

    /*
     * Check for alpha image 
     */
    alphadata = 0;
    alphatiffdata = 0;
    for (i = 0; i < size - 3; i++) {
	if (jpegdata[i] == 0xFF && jpegdata[i + 1] == EOI_MARKER) {
	    firstsize = i + 2;
	    alphadata = &jpegdata[i + 2];
	    break;
	}
    }

    if (alphadata) {
	(*sppp)++;
	NXJpegDecompress(&w, &h, &spp, 0, &alphatiffdata, alphadata, size - firstsize);
    }
    if (planar && *sppp > 1) {
	/*
	 * convert to planar format 
	 */
	cp1 = old = tiffdata[0];
	tiffdata[0] = red = (unsigned char *)valloc(*wp * *hp * (alphatiffdata ? 4 : 3));
	tiffdata[1] = green = red + *wp * *hp;
	tiffdata[2] = blue = green + *wp * *hp;
	for (i = *wp * *hp; i > 0; i--) {
	    *red++ = *cp1++;
	    *green++ = *cp1++;
	    *blue++ = *cp1++;
	}
	free(old);
	if (alphatiffdata) {
	    tiffdata[3] = old = tiffdata[2] + *wp * *hp;
	    for (i = *wp * *hp; i > 0; i--) {
		*old++ = *alphatiffdata++;
	    }
	    free(alphatiffdata);
	    alphatiffdata = NULL;
	}
    } else {
	/*
	 * Possibly add in the alpha data. 
	 */
	if (alphatiffdata) {
	    cp1 = new = (unsigned char *)valloc(*wp * *hp * 4);
	    alphastart = alphatiffdata;
	    cp2 = tiffdata[0];
	    for (i = *wp * *hp; i > 0; i--) {
		*cp1++ = *cp2++;
		*cp1++ = *cp2++;
		*cp1++ = *cp2++;
		*cp1++ = *alphatiffdata++;
	    }
	    free(tiffdata[0]);
	    free(alphastart);
	    tiffdata[0] = (unsigned char *)new;
	}
    }
    free(safedata);
}

static int 
initvars()
{
    dg->interleavingFormat = BLOCK_INTERLEAVE;

    dg->in_frame = dg->out_frame = VS_TARGA_FILE;
    dg->pixel_format_in = RGB24;
    dg->pixel_format_out = RGB24;
    dg->xfm_type = KTAS;	/* transform type */
    dg->cdr_type = N8R5_CDR;	/* coder type */
    dg->getbit_src = GETBIT_FILE;
    dg->getbit_src_format = GNUBUS_ASCII;

    dg->fdct_record = 0;	/* record Transform data */
    dg->eqdct_record = 0;	/* record Quantizer data */
    dg->codingDirection = MSB_FIRST;
}


#ifdef DEBUG
int 
dump(name, cp, size)
    char           *name;
    char           *cp;
    int             size;
{
    FILE           *fp;

    fp = fopen(name, "w");
    if (!fp) {
	printf("counld not open %s\n", name);
	exit(1);
    }
    while (size-- > 0) {
	putc(*cp++, fp);
    }
    fclose(fp);
}
#endif
