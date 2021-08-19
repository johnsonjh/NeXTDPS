/******************************************************************************

    mpcursor.c
    Cursor Manipulation for the MP Driver
    
    Created 23Feb90 Ted Cohn
    
    Modification History:
    22Jul90 Ted Added NXCursorInfo structure to eliminate nextdev/evio.h
    
******************************************************************************/

#import PACKAGE_SPECS
#import DEVICETYPES
#import DEVPATTERN
#import BINTREE
#import "mp.h"
#import "mp12.h"

#define CURSORWIDTH	16	/* pixels */
#define CURSORHEIGHT	16	/* pixels */
#define PPXMASK		((unsigned int)0x0000000f)

void MPSetCursor(NXDevice *d, NXCursorInfo *nxci, LocalBitmap *lbm)
{
    unsigned int *cd, *ca, *sp, *ap;
    short i, j;
    
    if (monitorType == MP_MONO_SCREEN) {
	if (nxci->set.curs.state2W) return;
	nxci->set.curs.state2W = 1;
	cd = nxci->cursorData2W;
	ca = nxci->cursorAlpha2;
	sp = lbm->bits;
	ap = lbm->abits;
    
	switch(lbm->base.type) {
	    case NX_TWOBITGRAY: {
		register unsigned int a;
		for (i=16; --i>=0; ) {
		    *ca++ = a = *ap++;
		    *cd++ = a - *sp++;	/* Convert Data:  D' = A-D */
		}
		break;
	    }
	    case NX_TWELVEBITRGB: {
		DevPoint off, size, phase;
		
		size.x = size.y = CURSORWIDTH;
		phase.x = phase.y = off.x = off.y = 0;
		MP12Convert16to2((unsigned short *)sp, cd, ca, size, off,
		    lbm->rowBytes, off, 4, phase);
		break;
	    }
	    case NX_TWENTYFOURBITRGB: {
		DevPoint off, size, phase;
		
		size.x = size.y = CURSORWIDTH;
		phase.x = phase.y = off.x = off.y = 0;
		MP12Convert32to2(sp, cd, ca, size, off, 
		    lbm->rowBytes, off, 4, phase);
		break;
	    }
	}
    } else {	/* MP_COLOR_SCREEN */
	if (nxci->set.curs.state16) return;
	nxci->set.curs.state16 = 1;
	cd = nxci->cursorData16;
	sp = lbm->bits;
	ap = lbm->abits;
    
	switch(lbm->base.type) {
	    case NX_TWOBITGRAY: {
		register unsigned int a, s;
		register unsigned short d, m;
		register unsigned short *dp;
		dp = (unsigned short *)cd;
		for (i=16; --i>=0; ) {
		    a = *ap++;
		    s = *sp++;
		    for (j=30; j>=0; j-=2) {
			d = (s>>j) & 3;
			d |= d<<2; d |= d<<4; d |= d<<8; d &= 0xFFF0;
			m = (a>>j) & 3;
			m |= m<<2;
			d |= m;
			*dp++ = d;
		    }
		}
		break;
	    }
	    case NX_TWELVEBITRGB: {
		for (i=16; --i>=0; ) {
		    *cd++ = *sp++;
		    *cd++ = *sp++;
		    *cd++ = *sp++;
		    *cd++ = *sp++;
		    *cd++ = *sp++;
		    *cd++ = *sp++;
		    *cd++ = *sp++;
		    *cd++ = *sp++;
		}
		break;
	    }
	    case NX_TWENTYFOURBITRGB: {
		register unsigned int s;
		for (i = 16; --i>=0; ) {
		    for (j = 16; --j>=0; ) {
			s = *sp++;
			s = (s >> 4)  & 0xf | 
			    (s >> 8)  & 0xf0 |
			    (s >> 12) & 0xf00 |
			    (s >> 16) & 0xf000;
			s = (s<<16) | s;
			*cd++ = s;
		    }
		}
		break;
	    }
	}
    }
}

void MPDisplayCursor2(NXDevice *device, NXCursorInfo *nxci)
{
    Bounds bounds;		/* screen bounds */
    Bounds saveRect;		/* cursor save rectangle (local) */
    unsigned int vramRow;	/* screen row longs (4-byte chunks) */
    volatile unsigned int *cursPtr;	/* cursor data pointer */
    volatile unsigned int *vramPtr;	/* screen data pointer */
    volatile unsigned int *savePtr;	/* saved screen data pointer */
    volatile unsigned int *maskPtr;	/* cursor mask pointer */
    int i, doLeft, doRight, skew, rSkew;

    vramPtr = ((LocalBitmap *)device->bm)->bits;
    vramRow = ((LocalBitmap *)device->bm)->rowBytes >> 2;
    bounds = device->bounds;
    saveRect = *nxci->cursorRect;
 
    /* Clip saveRect vertical within screen bounds */
    if (saveRect.miny < bounds.miny) saveRect.miny = bounds.miny;
    if (saveRect.maxy > bounds.maxy) saveRect.maxy = bounds.maxy;
    i = nxci->cursorRect->minx - bounds.minx;
    saveRect.minx = i - (i & PPXMASK) + bounds.minx;
    saveRect.maxx = saveRect.minx + CURSORWIDTH*2;
    *nxci->saveRect = saveRect;

    vramPtr += (vramRow * (saveRect.miny - bounds.miny)) +
	       ((saveRect.minx - bounds.minx) >> 4);

    skew = (nxci->cursorRect->minx & PPXMASK)<<1; /* skew is in bits */
    rSkew = 32-skew;

    savePtr = nxci->saveData;
    cursPtr = nxci->cursorData2W + saveRect.miny - nxci->cursorRect->miny;
    maskPtr = nxci->cursorAlpha2 + saveRect.miny - nxci->cursorRect->miny;

    doLeft =  (saveRect.minx >= bounds.minx);
    doRight = (saveRect.maxx <= bounds.maxx);

    for (i = saveRect.maxy - saveRect.miny; --i>=0; ) {
	register unsigned int workreg;
	if (doLeft) {
	    *savePtr++ = workreg = *vramPtr;
	    *vramPtr = (workreg&(~((*maskPtr)>>skew))) | ((*cursPtr)>>skew);
	}
	if (doRight) {
	    *savePtr++ = workreg = *(vramPtr+1);
	    *(vramPtr+1)=(workreg&(~((*maskPtr)<<rSkew)))|((*cursPtr)<<rSkew);
	}
	vramPtr += vramRow;
	cursPtr++;
	maskPtr++;
    }
}

void MPRemoveCursor2(NXDevice *device, NXCursorInfo *nxci)
{
    int i, doLeft, doRight;
    unsigned int vramRow, lmask, rmask;
    volatile unsigned int *vramPtr;
    volatile unsigned int *savePtr;
    Bounds bounds = device->bounds;
    Bounds saveRect = *nxci->saveRect;
    static const unsigned int mask_array[16] = {
    	0xFFFFFFFF,0x3FFFFFFF,0x0FFFFFFF,0x03FFFFFF,
	0x00FFFFFF,0x003FFFFF,0x000FFFFF,0x0003FFFF,
	0x0000FFFF,0x00003FFF,0x00000FFF,0x000003FF,
	0x000000FF,0x0000003f,0x0000000F,0x00000003
    };

    vramPtr = ((LocalBitmap *)device->bm)->bits;
    vramRow = ((LocalBitmap *)device->bm)->rowBytes >> 2;
    vramPtr += (vramRow * (saveRect.miny - bounds.miny)) +
	       ((saveRect.minx - bounds.minx) >> 4);
    savePtr = nxci->saveData;
    if (doLeft = (saveRect.minx >= bounds.minx))
    	lmask = mask_array[nxci->cursorRect->minx - saveRect.minx];
    if (doRight = (saveRect.maxx <= bounds.maxx))
    	rmask = ~mask_array[16-(saveRect.maxx - nxci->cursorRect->maxx)];
    for (i = saveRect.maxy - saveRect.miny; --i>=0; ) {
	if (doLeft) *vramPtr = (*vramPtr&(~lmask))|(*savePtr++&lmask);
	if (doRight) *(vramPtr+1)=(*(vramPtr+1)&(~rmask))|(*savePtr++&rmask);
	vramPtr += vramRow;
    }
}

/*	16 Bit Color frame buffer */
#define RBMASK	0xF0F0		/* Short, or 16 bit format */
#define GAMASK	0x0F0F		/* Short, or 16 bit format */
#define AMASK	0x000F		/* Short, or 16 bit format */

void MPDisplayCursor16(NXDevice *device, NXCursorInfo *nxci)
{
    Bounds bounds;			/* screen bounds */
    Bounds saveRect;			/* cursor save rectangle (local) */
    volatile unsigned short *cursPtr;	/* cursor data pointer */
    volatile unsigned short *vramPtr;	/* screen data pointer */
    volatile unsigned short *savePtr;	/* saved screen data pointer */
    unsigned short s, d, f;
    int width, cursRow, vramRow;
    int i, j;

    vramPtr = (unsigned short *)((LocalBitmap *)device->bm)->bits;
    vramRow = ((LocalBitmap *)device->bm)->rowBytes >> 1;
    bounds = device->bounds;
    saveRect = *nxci->cursorRect;
 
    /* Clip saveRect vertical within screen bounds */
    if (saveRect.miny < bounds.miny) saveRect.miny = bounds.miny;
    if (saveRect.maxy > bounds.maxy) saveRect.maxy = bounds.maxy;
    if (saveRect.minx < bounds.minx) saveRect.minx = bounds.minx;
    if (saveRect.maxx > bounds.maxx) saveRect.maxx = bounds.maxx;
    *nxci->saveRect = saveRect;

    vramPtr += (vramRow * (saveRect.miny - bounds.miny)) +
	       (saveRect.minx - bounds.minx);
    width = saveRect.maxx - saveRect.minx;
    vramRow -= width;

    savePtr = (volatile unsigned short *) nxci->saveData;
    cursPtr = (volatile unsigned short *) nxci->cursorData16 + 
		saveRect.miny - nxci->cursorRect->miny;
    cursPtr += (saveRect.miny - nxci->cursorRect->miny) * CURSORWIDTH +
    	       (saveRect.minx - nxci->cursorRect->minx);
    cursRow = CURSORWIDTH - width;
    for (i = saveRect.maxy - saveRect.miny; --i>=0; ) {
	for (j = width; --j>=0; ) {
	    d = *savePtr++ = *vramPtr;
	    s = *cursPtr++;
	    f = (~s) & (unsigned int)AMASK;
	    d = s + (((((d & RBMASK)>>4)*f + GAMASK) & RBMASK)
		| ((((d & GAMASK)*f+GAMASK)>>4) & GAMASK));
	    *vramPtr++ = d;
	}
	cursPtr += cursRow; /* starting point of next cursor line */
	vramPtr += vramRow; /* starting point of next screen line */
    }
}

void MPRemoveCursor16(NXDevice *device, NXCursorInfo *nxci)
{
    int i, j, vramRow;
    volatile unsigned short *vramPtr;
    volatile unsigned short *savePtr;
    Bounds bounds = device->bounds;
    Bounds saveRect = *nxci->saveRect;

    vramPtr = (unsigned short *)((LocalBitmap *)device->bm)->bits;
    vramRow = ((LocalBitmap *)device->bm)->rowBytes >> 1;
    vramPtr += (vramRow * (saveRect.miny - bounds.miny)) +
	       (saveRect.minx - bounds.minx);
    vramRow -= (saveRect.maxx - saveRect.minx);
    savePtr = (volatile unsigned short *)nxci->saveData;
    for (i = saveRect.maxy - saveRect.miny; --i>=0; ) {
	for (j = saveRect.maxx - saveRect.minx; --j>=0; )
		*vramPtr++ = *savePtr++;
	vramPtr += vramRow;
    }
}


