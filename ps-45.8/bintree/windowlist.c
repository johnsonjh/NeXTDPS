/*****************************************************************************

    windowlist.c
    Operators on windowlists.

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 08Jul86 Leo

    Modified:

    16Dec87 Leo   Removed old mod log, made into subroutines
    10Feb88 Leo   Fixed ordering of params to bcopy
    13Mar89 Ted   Modified by mfbs
    26Apr89 Ted   Alphabetized methods
    16May89 Ted   ANSI C funtion prototypes
    26Mar90 Terry Moved Get*Window and Get*Bounds in from old nextmain.c
    07Jun90 Terry Code size reduction from 1312 to 1068 bytes
    28Jun90 Ted   Removed WLSize (dead code)

******************************************************************************/

#import PACKAGE_SPECS
#import BINTREE
#import WINDOWDEVICE
#import "bintreetypes.h"

#define WLGROWINC 32		/* Amount to grow by */
#define MAXWLSIZE 30000		/* Maximum legal size */

static WindowList pwl;		/* Private window list */


/*****************************************************************************
    WLInitialize
    Initialize the window list.
******************************************************************************/
void WLInitialize()
{
    pwl.contents = (Layer **) malloc(WLGROWINC * sizeof(Layer *));
    pwl.capacity = WLGROWINC;
}


/*****************************************************************************
    GetFrontWindow
    Return the frontmost window layer.
******************************************************************************/
Layer *GetFrontWindow()
{
    return WLAt(0);
}


/*****************************************************************************
    GetNextWindow
    Return the next window in the window list.
******************************************************************************/
Layer *GetNextWindow(Layer *layer)
{
    return WLAt(WLOffsetOf(layer)+1);
}


/*****************************************************************************
	GetTLWinBounds
	Return the window's bounds in screen space.
******************************************************************************/
void GetTLWinBounds(Layer *layer, Bounds *r)
{
    *r = layer ? layer->bounds : wsBounds;
}


/*****************************************************************************
	GetWinBounds
	Return the window's bounds in user space.
******************************************************************************/
void GetWinBounds(Layer *layer, Bounds *r)
{
    short temp;

    *r = layer ? layer->bounds : wsBounds;
    temp = r->miny;
    r->miny = remapY - r->maxy;
    r->maxy = remapY - temp;
}


/*****************************************************************************
    WLAboveButNotAbove
    This procedure returns a SubList of this WindowList giving the layers
    that are above the first position but not above the second. Positions
    are considered to be between entries in the list; the op can be either
    ABOVE, OUT (not in list), or BELOW; if the window id for ABOVE or BELOW
    is nil, it's that end of the list; set subtraction applies. ******************************************************************************/
SubList WLAboveButNotAbove(int op1, Layer *window1, int op2, Layer *window2)
{	
    SubList sl;
    int endPos, begPos;

    sl.ptr = NULL;
    sl.len = 0;
    if (op1 == OUT)
	begPos = 0;
    else {
	if (window1 == NULL)
	    if (op1 == ABOVE)
		begPos = 0;
	    else
		begPos = pwl.length;
	else
	    begPos = WLOffsetOf(window1) + (op1 == -1);
    }
    if (op2 == OUT)
	endPos = 0;
    else {
	if (window2 == NULL)
	    if (op2 == ABOVE)
		endPos = 0;
	    else
		endPos = pwl.length;
	else
	    endPos = WLOffsetOf(window2) + (op2 == -1);
    }
    if (endPos >= begPos)
	return sl;
    sl.ptr = pwl.contents + endPos;
    sl.len = begPos-endPos;
    return sl;
}


/*****************************************************************************
    WLAt
    Return the layer which is at the given offset in the window list.
******************************************************************************/
static Layer *WLAt(int offset)
{
    return (offset < pwl.length) ? *(pwl.contents+offset) : NULL;
}


/*****************************************************************************
    WLBelowButNotBelow
    This procedure is similar to WLAboveButNotAbove but returns a list of
    layers which are below a certain position but not below another. ******************************************************************************/
SubList WLBelowButNotBelow(int op1, Layer *window1, int op2, Layer *window2)
{
    SubList sl;
    int begPos, endPos;

    sl.ptr = NULL;
    sl.len = 0;
    if (op1 == OUT)
	begPos = pwl.length;
    else {
	if (window1 == NULL)
	    if (op1 == ABOVE)
		begPos = 0;
	    else
		begPos = pwl.length;
	else
	    begPos = WLOffsetOf(window1) + (op1 == -1);
    }
    if (op2 == OUT)
	endPos = pwl.length;
    else {
	if (window2 == NULL)
	    if (op2 == ABOVE)
		endPos = 0;
	    else
		endPos = pwl.length;
	else
	    endPos = WLOffsetOf(window2) + (op2 == -1);
    }
    if (endPos <= begPos)
	return sl;
    sl.ptr = pwl.contents + begPos;
    sl.len = endPos - begPos;
    return sl;
}


/*****************************************************************************
    WLExpand
    Expand the capacity of the windowlist by WLGROWINC.
******************************************************************************/
static void WLExpand()
{
    if ((pwl.capacity += WLGROWINC) > MAXWLSIZE)
	PSLimitCheck();
    pwl.contents = (Layer **) realloc(pwl.contents, pwl.capacity *
	sizeof(Layer *));
}


/*****************************************************************************
    WLOffsetOf
    Return the index of the given window in the windowlist.
******************************************************************************/
static int WLOffsetOf(register Layer *window)
{
    int i;
    Layer **ptr;

    for (i=0, ptr = pwl.contents; i<pwl.length; i++)
	if ((*ptr++) == window)
	    return i;
    return -1;
}


/*****************************************************************************
    WLPutAfter
    Places window1 just after (below) window2 in the windowlist.
******************************************************************************/
Layer *WLPutAfter(Layer *window1, Layer *window2)
{
    if (pwl.length >= pwl.capacity)
	WLExpand();
    if (window2 == NULL)
	pwl.contents[pwl.length] = window1;
    else
    {
	int i, j;
	Layer **ptr;
	
	for (i=0; i<pwl.length; i++)
	    if (pwl.contents[i] == window2)
		break;
	if (i >= pwl.length)
	    return NULL;
	for (j=pwl.length; j>(i+1); j--)
	    pwl.contents[j] = pwl.contents[j-1];
	pwl.contents[i+1] = window1;
    }
    pwl.length++;
    return window1;
}


/*****************************************************************************
    WLPutBefore
    Places window1 in the windowlist just before (above) window2.
******************************************************************************/
Layer *WLPutBefore(Layer *window1, Layer *window2)
{
    int i, j;
    Layer **ptr;

    if (pwl.length >= pwl.capacity)
	WLExpand();
    if (window2 == NULL)
	i = 0;
    else {
	for (i=0; i<pwl.length; i++)
	    if (pwl.contents[i] == window2)
		break;
	if (i >= pwl.length)
	    return NULL;
    }
    for (j=pwl.length; j>i; j--)
	pwl.contents[j] = pwl.contents[j-1];
    pwl.contents[i] = window1;
    pwl.length++;
    return window1;
}


/*****************************************************************************
    WLRemove
    Remove the window from the windowlist.
******************************************************************************/
void WLRemove(Layer *window)
{
    register int i;
    
    for (i=0; i<pwl.length; i++)
	if (pwl.contents[i] == window)
	    break;
    if (i >= pwl.length)
	return;
    for ( ; i < pwl.length-1; i++)
	pwl.contents[i] = pwl.contents[i+1];
    pwl.length--;
}


