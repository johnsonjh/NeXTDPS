/*****************************************************************************

    pattern.c
    Pattern routines for the bintree window system.

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 11Jul86 Leo

    Modified:

    03Nov86 Leo   createFromBits:
    07Apr87 Jack  Added yOrigin
    17Nov87 Jack  Convert to use Halftones, need to add ScrRefs
    01Jun88 Jack  Added calls to *ScrRef
    16May89 Ted   ANSI C funtion prototypes
    12Apr90 Jack  Removed colortowhite
    30May90 Terry Reduced code size from 500 to 416 bytes
    29Jul90 Ted   Removed obsolete PPrintOn.

******************************************************************************/

#import PACKAGE_SPECS
#import "bintreetypes.h"
#import WINDOWDEVICE

static DevScreen myScreen;
static DevHalftone myHalftone;
static unsigned char checkerboard[4] = {64, 191, 255, 128};
static char *patternPool; /* Blind pointer to storage pool for Patterns */

/*****************************************************************************
    PInitialize
    Initialize pattern code.
******************************************************************************/
void PInitialize()
{
    patternPool = (char *) os_newpool(sizeof(Pattern), 0, 0);
    myScreen.width = 2;
    myScreen.height = 2;
    myScreen.thresholds = checkerboard;
    myHalftone.white = &myScreen;
    myHalftone.priv = NULL; /* Continues to be null until gray is used */
    whitepattern = PNewColorAlpha(WHITE_COLOR, OPAQUE, &myHalftone);
    blackpattern = PNewColorAlpha(BLACK_COLOR, OPAQUE, &myHalftone);
}


/*****************************************************************************
    PNew
    Return a new pattern.
******************************************************************************/
Pattern *PNew()
{
    Pattern *pat;
    
    pat = (Pattern *) os_newelement(patternPool);
    pat->type = PATTERN;
    return pat;
}


/*****************************************************************************
    PSetHalftone
    Set attributes of given pattern to info's attributes.
******************************************************************************/
void PSetHalftone(Pattern *pat, DevMarkInfo *info)
{
    pat->type	   = PATTERN;
    pat->halftone  = info->halftone;
    pat->alpha     = (*((PNextGSExt *)info->priv))->alpha;
    pat->color     = info->color;
    pat->phase     = info->screenphase;
}


/*****************************************************************************
	PNewColorAlpha
	Create a pattern from a given color (toward black) and alpha.
******************************************************************************/
Pattern *PNewColorAlpha(unsigned int color, unsigned char alpha,
    DevHalftone *halftone)
{
    Pattern *pat;
    
    pat = PNew();
    pat->alpha = alpha;
    *((unsigned int *)&pat->color) = color;
    pat->phase.x = pat->phase.y = 0;
    pat->permanent = 1;
    pat->halftone = halftone;
    return pat;
}


/*****************************************************************************
    PFree
    Free a pattern unless it's permanent.
******************************************************************************/
void PFree(Pattern *pat)
{
    if (pat->permanent)
	return;
    os_freeelement(patternPool, pat);
}









