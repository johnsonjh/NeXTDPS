/*
  devcreate.h

Copyright (c) 1988, 1989 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Edit History:
Jim Sandman: Mon Aug 14 09:31:28 1989
Bill Paxton: Sat Oct 17 09:29:15 1987
Ivor Durham: Wed Aug 17 14:08:52 1988
Linda Gass: Wed Dec  2 14:27:15 1987
Joe Pasqua: Wed Dec 14 14:29:28 1988
Paul Rovner: Thu Dec 28 17:10:53 1989
Ed Taft: Thu Dec 14 09:58:03 1989
End Edit History.

*/

#ifndef DEVCREATE_H
#define DEVCREATE_H

#include PUBLICTYPES
#include ENVIRONMENT
#include DEVICE

typedef struct _t_DCPixelArgs
    {
     integer depth;	/* bits per pixel. 1, 2, 4, 8, 16 or 32. */

     integer colors;
        /* 1 (grayscale), 3 (RGB), or 4 (CMYK).
	   If colors = 4, depth must = 32.
	   Otherwise, firstColor gives the first pixel value to be used;
	   and the last pixel value used =
	     firstColor + (nReds * nGreens * nBlues) + nGrays */

     /* The following are ignored if depth = 32 */
     integer firstColor;/* pixel value of first color. */
     integer nReds;
     integer nGreens;
     integer nBlues;
     integer nGrays;

     boolean invert;		/* should be true if one means white. */
    } DCPixelArgs, *PDCPixelArgs;

typedef char *PPixelBuffer;
  /* Pointer to frame or band buffer storage. This declaration does not
     commit to a representation or quantization of the buffer; that
     information is private to the device implementation. */

typedef PPixelBuffer (*FrameProc)(/*
  PPixelBuffer frameBuffer; unsigned char *procHook;
  boolean clear; unsigned char *pageHook; integer nCopies; */
  );
  /* Used with frame devices:
     A FrameProc is a call-back procedure to deal with output pixel values
     for a new page. It is called when showpage or copypage is executed.
     The call hands off frameBuffer to the callee. The FrameProc should
     return a non-NIL pointer to a frame buffer for the next page.
     
     procHook is the value passed to FrameDevice.
     
     clear = true for showpage, false for copypage.
     
     pageHook is the value passed to the frame device's ShowPage proc by
     the kernel (see the setdevice spec).
   */

typedef PPixelBuffer (*BandProc)(/*
  integer firstScanLine, nScanLines;
  PPixelBuffer bandBuffer; boolean bandIsClear; unsigned char *procHook;
  boolean clear; unsigned char *pageHook; */);
  /* Used with band devices:

     A BandProc is a call-back procedure to deal with output pixel values
     for each band. The caller hands off bandBuffer to the callee. The
     BandProc should return a non-NIL pointer to a buffer for the next band.
     The new buffer should be cleared to all zeros.
     
     If bandIsClear is true then all pixel values in this band of the page
     image should be set to zero by the BandProc. The BandProc can assume
     that bandBuffer contains all zeros.

     procHook is the value passed to BandDevice.
     
     clear = true for showpage, false for copypage.
     
     pageHook is the value passed to the band device's ShowPage proc by
     the kernel (see the setdevice spec).
   */

typedef procedure (*DLFullProc)(/* unsigned char *procHook; */);
  /* Used with band devices:
     Call-back procedure to report overflow of the current display list.
     procHook is the value passed to BandDevice. */

typedef struct _t_DevTrackingProcsRec
  {
  procedure (*goAway)(/* unsigned char *procHook; */);
  procedure (*sleep)(/* unsigned char *procHook; */);
  procedure (*wakeup)(/* unsigned char *procHook; */);
  } DevTrackingProcsRec, *DevTrackingProcs;
  /*
   An instance of this record type is passed to FrameDevice, BandDevice
   and HybridDevice. See below.
   
   These procs are called by the corresponding device procs for frame,
   band and hybrid devices. A NIL entry means that no tracking is to be
   done for the device. 
   */

/* FrameDevice creates a device that images each page into a monolithic frame
   buffer, then hands off the buffer, then acquires a new buffer for the
   next page. */
extern PDevice FrameDevice(/*
    FrameProc frameProc;
    unsigned char *procHook;
    PPixelBuffer base;
    PMtx pmtx;
    integer byteWidth, pixelWidth, height;
    PDCPixelArgs pixelArgs;
    DevTrackingProcs trackingProcs;
    */);

  /* Creates a frame device, i.e. one that images into a frame buffer for a
     grayscale or RGB device. Returns NIL if it can't construct such a
     device.

     If frameProc is non-NIL, it is called when showpage or copypage is
     executed. base and procHook are passed to it then as arguments.
     frameProc returns a frame buffer pointer to be used for the next page. 
     
     base is a pointer to the frame buffer for the first page.

     pmtx is used to derive the default transformation matrix (DTM) of the
     new device. The a, b, c and d components of mtx determine the scale
     and rotation transformations. These components are used verbatim in
     the DTM. The tx and ty components of mtx specify the x and y margins,
     if any, of the output page. tx should be the negative of the x margin
     and ty should be the negative of the y margin. 

     byteWidth is the width of frame buffer.
     pixelWidth is the width of imaged area in pixels.
     height is the height of frame buffer in scanlines.

     pixelArgs specifies the pixel size and encoding.
     
     trackingProcs specifies procs to be called by the corresponding
     device procs of the new device. If trackingProcs is NIL, no such
     tracking is done.
   */

extern PDevice CreateNullDevice();
  /* Creates a null device, i.e. one that ignores marking operations
     but builds masks like a buffer device.
   */

extern procedure SetBandStorage(/*
    unsigned char *memDL; integer memDLBytes;
    unsigned char *memSL; integer memSLBytes;
    PPixelBuffer memFrame; integer memFrameBytes; */
    );
  /*
   This establishes buffer storage to be used by the implementations
   of band and hybrid devices.

   Call this before calling BandDevice or HybridDevice.
   
   memDL must point to memDLBytes bytes of memory storage to be used for
   display lists. For band devices memDLBytes must be at least
     ((1028)*(1+(height+scanLinesPerBand-1)/scanLinesPerBand)).
   A size of twice this value is recommended.
   
   memSL must point to memSLBytes bytes of memory storage to be used for
   image source data and large character masks. A size of 400000 bytes is
   recommended for memSL.
   
   (HYBRID DEVICES ONLY)
   The memFrame and memFrameBytes parameters are ignored by band devices.

   memFrame must point to memFrameBytes bytes of frame and/or band buffer
   storage for use by hybrid devices. SetBandStorage clears this storage
   before using it. memFrameBytes must be at least large enough to hold a
   full frame buffer:

     memFrameBytes >= frameByteWidth * height
   
   SetBandStorage calls FlushPendingPages (see below) and clears all
   current page images of existing band or hybrid devices.
   */

extern procedure FlushPendingPages();
  /*
   (HYBRID DEVICES ONLY)
   This discards all pages that are queued for delivery but have not yet
   been fed. This may be called either from the background thread or from
   the foreground thread.
   */

extern procedure AwaitPendingPages();
  /*
   (HYBRID DEVICES ONLY)
   This waits until all pages that are queued for delivery have been
   delivered. Call AwaitPendingPages only in the background (i.e. the
   interpreter's) thread.
   */

extern PDevice BandDevice(/*
    PMtx pmtx;
    PDCPixelArgs pixelArgs;
    integer scanLinesPerBand, frameByteWidth, pixelWidth, height;

    BandProc bandProc;
    DLFullProc dlFullProc;
    unsigned char *procHook;
    
    PPixelBuffer bandBuffer;
    DevTrackingProcs trackingProcs;
    */);
  
  /* Creates a band device. If there is an existing band device, 
     this causes the display list and source list to be cleared.
     
     DO NOT create a band device if there is an existing hybrid device.

     pmtx is used to derive the default transformation matrix (DTM) of the
     new device. The a, b, c and d components of mtx determine the scale
     and rotation transformations. These components are used verbatim in
     the DTM. The tx and ty components of mtx specify the x and y margins,
     if any, of the output page. tx should be the negative of the x margin
     and ty should be the negative of the y margin. 

     pixelArgs->colors must be either 1 (monochrome), 3 (RGB) or 4 (CMYK).
     If 4, then nReds, etc. are taken to mean nCyans, nMagentas, nYellows,
     nBlacks. 
	 
     scanLinesPerBand must be a power of 2

     frameByteWidth must be a multiple of 4
     (pixelWidth * pixelArgs->depth) <= (frameByteWidth * 8)
     pixelWidth < 32000
     height < 32000
     
     bandBuffer must point to storage to be used for the first band.
     The size of bandBuffer must = frameByteWidth * scanLinesPerBand.
     The new buffer should be cleared to all zeros.
     
     Sample parameter settings (assuming pixels per inch == 2048 and
     bits per pixel = 8):
 
      pixelWidth = (pixelsPerInch * 8.5) = 17408
      frameByteWidth = (((pixelWidth*(pixelArgs->depth))+31)/32)*4 = 17408
      height = (pixelsPerInch * 11) = 22528
      mtx = {pixelsPerInch/72, 0, 0, -pixelsPerInch/72, 0, 0}
      scanLinesPerBand = 256
      size of bandBuffer = 4456448 bytes
      number of bands = 88
       
      (So the size arguments to SetBandStorage should be
        memDLBytes = 182984
        memSLBytes = 400000
      )
 
     bandProc is called once for each band when either the copypage
     or the showpage operator is executed. The parameters to each call
     address a pixel array (bandBuffer) and indicate the portion of the
     output page that it represents (firstScanLine, nScanLines). The call
     on bandProc hands off bandBuffer to the callee. bandProc should
     return a non-NIL pointer to a buffer to be used for the next band.
     
     procHook may be any value. It is passed to bandProc and dlFullProc
     on each call.

     dlFullProc is called to report overflow of the current display list.
     The page will be printed incomplete.

     trackingProcs specifies procs to be called by the corresponding
     device procs of the new device. If trackingProcs is NIL, no such
     tracking is done.

     BandDevice returns NULL if it is unable to create a new device.
   */

typedef enum
    {
    canFeedEngineStatus,   /* ready to start a new page, but not to image */
    readyEngineStatus,     /* ready to image a page or start a new one */
    canImageEngineStatus,  /* ready to image a page, but not to start one */
    busyEngineStatus,      /* do not image a page or start a new one */
    } DevEngineStatus;

typedef enum
    {
    unknownPageStatus,     /* engine has no knowledge of the page */
    feedingPageStatus,     /* engine is now feeding a sheet for the page */
    fedPageStatus,         /* engine has fed a sheet; can now image it */
    imagingPageStatus,     /* engine is in the midst of imaging the page */
    holdingPageStatus,     /* engine has the page queued for delivery */
    deliveredPageStatus,   /* engine has delivered the page */
    damagedPageStatus,     /* page should be re-sent (e.g.,jammed) */
    } DevPageStatus;
    /* When a page image is created, its status is set to feedingPageStatus.
       This changes to fedPageStatus when the engine becomes ready to accept
       a call on the imagePage proc to image the page. The imagePage proc
       immediately changes the page status of its parameter to
       imagingPageStatus if it succeeds, i.e. if it actually accepts the
       page for printing. The imagePage proc returns true if it succeeds,
       false otherwise. If the imagePage proc succeeds, the page status
       normally then progresses thru holdingPageStatus to
       deliveredPageStatus.
       
       The page status will be set to damagedPageStatus by the engine if
       a fault occurs that requires the page to be reprinted.
       
       After the pageStatus proc returns deliveredPageStatus or
       damagedPageStatus the page image object becomes invalid.
       
       WARNING: do not pass an invalid DevPageImage value to a pageStatus
       or imagePage proc.
     */

typedef struct _t_DevPageImageRec *DevPageImage;
  /* Opaque designator for a page image on its way thru the paper path */

typedef struct _t_DevPageImageProcsRec
    {
    DevEngineStatus (*engineStatus) (/* unsigned char *procHook; */);

    DevPageImage (*newDevPageImage) (/*
      unsigned char *procHook; unsigned char *pageHook; */);
      /* 
         procHook is the parameter passed to HybridDevice.
         
         pageHook is the value passed to the hybrid device's ShowPage
         proc by the kernel (see the setdevice spec).

         The initial status of the new page image is 'feedingPageStatus'.
       */

    DevPageStatus (*pageStatus) (/*
      DevPageImage devPageImage;
      integer *nScanlinesPrinted;
      unsigned char *procHook; */
      );

    boolean (*imagePage) (/*
      DevPageImage devPageImage;
      PPixelBuffer *bands;
      unsigned char *clearBandFlags;
      unsigned char *procHook; */
      );

    } DevPageImageProcsRec, *DevPageImageProcs;

extern PDevice HybridDevice(/*
    PMtx pmtx;
    PDCPixelArgs pixelArgs;
    integer scanLinesPerBand, frameByteWidth, pixelWidth, height;
    DevPageImageProcs devPageImageProcs;
    unsigned char *procHook;
    boolean errorRecoveryOn;
    DevTrackingProcs trackingProcs;
    */);
  /* Creates a hybrid device. If there is an existing hybrid device, 
     this causes its current display list, source list and frame storage
     (if any) to be cleared.
     
     DO NOT create a hybrid device if there is an existing band device.

     The differences between HybridDevice and BandDevice are summarized
     below:
     
     devPageImageProcs->engineStatus is called by the hybrid device
     implementation to determine the current status of the engine.
     The 'procHook' argument is the procHook parameter to HybridDevice.

     devPageImageProcs->newDevPageImage is called by the hybrid device
     implementation after showpage is executed and (usually) before the
     display list is rendered. devPageImageProcs->newDevPageImage should
     prepare to image a new page (e.g., by feeding a new sheet). It
     should return an opaque pointer to its descriptor for the new page
     image, or NIL if for some reason it is unable to accept new pages now.
     Provision of a way for devPageImageProcs->newDevPageImage to return
     NIL is intended to handle the race condition in which engineStatus
     says it's ready to feed a new page but the engine becomes not-ready
     before the request is sent.

     devPageImageProcs->imagePage is called by the hybrid device
     implementation to send a completed page to the marking engine to be
     printed. The imagePage proc immediately changes the page status of
     its parameter to imagingPageStatus if it succeeds, i.e. if it
     actually accepts the page for printing. The imagePage proc returns
     true if it succeeds, false otherwise. If the imagePage proc succeeds,
     the page status normally then progresses thru holdingPageStatus to
     deliveredPageStatus. Provision of a way for
     devPageImageProcs->imagePage to returnfalse is intended to handle the
     race condition in which engineStatus says it's ready to image but the
     engine becomes not-ready before the request is sent.
     
     The 'devPageImage' argument must be a page image previously
     created by devPageImageProcs->newDevPageImage. The 'bands' argument
     is a vector of pointers to band-sized chunks of the page. The
     'clearBandFlags' argument is a vector of chars, one for each band,
     indicating whether the engine should clear the corresponding band
     buffer referenced from 'bands' (if non-NIL) to zeros (element != 0)
     or leave it untouched (element == 0).

     devPageImageProcs->pageStatus is called by the hybrid device
     implementation to determine the status of a page image previously
     created by devPageImageProcs->newDevPageImage. The 'nScanlinesPrinted'
     result parameter is the address of an integer variable. This is
     set by devPageImageProcs->pageStatus to the number of scanlines
     that have been printed for this page by the marking engine.
     
     If errorRecoveryOn is true, pages will be re-imaged if they become
     damaged (damagedPageStatus) before being delivered. If errorRecoveryOn
     is false, each page's display list will be cleared when it is
     rendered (thereby freeing up buffer storage for reuse earlier
     than otherwise).
     
     trackingProcs specifies procs to be called by the corresponding
     device procs of the new device. If trackingProcs is NIL, no such
     tracking is done.
   */

/* ***** */
/* Definitions for creating and managing window devices */

extern PDevice ScreenDevice(/*
    base, matrix, byteWidth, pixelArgs, window */);
  /* PPixelBuffer base;	-- pointer to frame buffer
     PMtx matrix;		-- initial user-space to device-space
	 			  coordinate transformation
     integer byteWidth;        -- (bytes): width of frame buffer
     PDCPixelArgs pixelArgs;
     DevPrivate *window; -- a window descriptor (opaque).
     
   * Does NOT reclaim the frame buffer automatically when device goes away.
   * Suports visible region clipping (via SetWindow).
   * Tracks calls on ChangeScreenDevices. This can be used to
     accommodate a change to the number of bits per pixel or to the screen
     color map.
  */

extern boolean ps_setwin_flg;
  /* A flag that outside window system can set to force a screen device
     to update the its origin and visible clip region.
  */
  
extern procedure ChangeScreenDevices(/* base, byteWidth, pixelArgs */);
  /* Same arguments as ScreenDevice, except that matrix stays the same
     and the windowProc is called to re-establish region clipping (see
     SetWindowProc, below). */

extern procedure SetWindowProc(
  /* procedure (*windowProc)( PDevice device ); */
  /* procedure (*deviceProc)(); */
  );
  /* Sets two procedures to be called by the device package:
     1. windowProc gets called sometime after ps_setwin_flg is set, and
        before the next call into the device package by the Display
        PostScript kernel causes the framebuffer to be written. windowProc
        should calculate the new origin and visible clipping region of the
        window associated with its device argument and then call SetWindow
	(defined below).

     2. deviceProc gets called sometime after ChangeScreenDevices is called,
        and before the next call into the device package by the Display
	PostScript kernel causes the framebuffer to be written. deviceProc
        should ensure that the color map settings in the video monitors are
	correct for DPS.
  */
  
extern procedure SetWindow(/*
  PDevice device; DevPoint origin; DevPrim *winclip; DevPrivate *window;*/);
  /* Sets the priv field of the device with a structure that records the 
     origin, visible clip region and window hook for the device.
  */ 

extern DevPrivate *GetWindowHook(/* PDevice device; */);
  /* Gets the window hook previously established by SetWindow for device.
  */ 

extern procedure CheckScreenDevices(); /* Imported by the scheduler */
    /* Arm the device code to take note of any changes to the properties
	   of DPS windows that affect its correctness.
     */

#endif DEVCREATE_H
