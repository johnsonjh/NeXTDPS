/* bm38pat.c -- implementation of 2-bit gray dither patterns
   for 32-bit rgb bitmaps */


typedef struct {
    PatternProcsRec procs;
    PatternData *pd[3];
} NXGrayPatRec;


/* setupPattern vector for nextgray patterns.  We just copy the precomputed
   pattern data into the passed pointer
 */
void
NXGrayPatSetup(PatternHandle h, DevMarkInfo *markInfo, PatternData *data)
{
    PNextGSExt ep;

    if(markInfo->priv &&
       (ep = *(PNextGSExt *)markInfo->priv) &&
       (ep->graypatstate != NOGRAYPAT)) {
	int grayindex = ep->graypatstate - 1;
	*data = *(((NXGrayPatRec *)h)->pd[grayindex])
	}
    else
	CantHappen();
}

void
NXGrayPatDestroy(PatternHandle h)
{
    free(h);
}

/* this vector should never be called (this pattern is not used during
   imaging.)
 */
int
NXGrayPatInfo(PatternHandle h, DevColorData *red, DevColorData *green,
		   DevColorData *blue, DevColorData *gray, int firstColor)
{
    CantHappen();
    return 0;
}


PatternHandle NXGrayPat(PatternData *logray, PatterData *midgray,
			PatternData *higray)
{
    int i;
    NXGrayPatRec *gp = calloc(sizeof(NXGrayPatRec));
    

    gp->pd[0] = logray;
    gp->pd[1] = midgray;
    gp->pd[2] = higray;
    gp->procs.setupPattern = NXGrayPatSetup;
    gp->procs.destroyPattern = NXGrayPatDestroy;
    gp->procs.patternInfo = NXGrayPatInfo;
    return((PatternHandle)gp);
}    
