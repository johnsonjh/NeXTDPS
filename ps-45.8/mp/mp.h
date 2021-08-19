/*****************************************************************************

    mp.h
    Header file for MegaPixel Device Driver

    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 27Feb89 Ted Cohn
    Composite.h created 14Jan88 Leo

    (modifications from composite.h)
    15Jan88  Leo  Included BitsOrPatInfo
    25Jan88  Jack move "CopyBits" (now MoveRect) ops from bintree.h
    31Jan88  Jack added op 1:7 (d = 1-s;)
    08Apr88  Leo  Added op 0:4 (d = highlight(d) )
    23Mar89  New mpheader.h file with composite.h included.
    12Jul90  Ted  Moved compositing defines and structures to mp12.h
 
******************************************************************************/

#import PACKAGE_SPECS
#import BITMAP

#define uchar unsigned char
#define uint unsigned int

#define MP_ROMID 0
#define MP_MONO_SCREEN 0	/* Monochrome monitor type */
#define MP_COLOR_SCREEN 1	/* Color monitor type */
#define MPSCREEN_ROMID 512	/* Unique value for this product */
#define MPSCREEN_WIDTH 1120	/* Visible pixels per scanline */
#define MPSCREEN_HEIGHT 832	/* Visible number of lines */
#define MPSCREEN_ROWBYTES 288	/* Actual rowBytes to next line */
#define MPSCREEN_ROWWORDS 72	/* Max words per scanline */

/*****************************************************************************
	Forward Declarations (ANSI C Prototypes)
******************************************************************************/

/**** mp ****/

extern int  MPStart(NXDriver *);
extern void MPComposite(CompositeOperation *, Bounds *);
extern void MPFreeWindow(NXBag *, int);
extern void MPHook(NXHookData *);
extern void MPInitScreen(NXDevice *);
extern void MPMark(NXBag *, int, MarkRec *, Bounds *, Bounds *);
extern void MPMoveWindow(NXBag *bag, short dx, short dy, Bounds *, Bounds *);
extern void MPNewAlpha(NXBag *);
extern void MPNewWindow(NXBag *, Bounds *, int, int, int);
extern void MPPromoteWindow(NXBag *, Bounds *, int, int, DevPoint);
extern void MPRegisterScreen(NXDevice *);
extern void MPSetBitmapExtent(uint *, uint *);
extern int  MPWindowSize(NXBag *bag);

/**** cursor ****/

extern void MPSetCursor(NXDevice *, NXCursorInfo *, LocalBitmap *);
extern void MPDisplayCursor2(NXDevice *, NXCursorInfo *);
extern void MPRemoveCursor2(NXDevice *, NXCursorInfo *);
extern void MPDisplayCursor16(NXDevice *, NXCursorInfo *);
extern void MPRemoveCursor16(NXDevice *, NXCursorInfo *);


/*****************************************************************************
	Globals
******************************************************************************/
extern uint *mpAddr;
extern uint screenOffsets[4];
extern uint memoryOffsets[4];
extern NXDevice *deviceList;
extern Pattern *blackpattern;
extern Pattern *whitepattern;
extern int use_wf_hardware;
extern int monitorType;
