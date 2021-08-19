/*
  gray.h

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Doug Brotz, Sept 19, 1983
Edit History:
Doug Brotz: Thu Sep 11 08:34:57 1986
Chuck Geschke: Wed Jul 25 11:16:27 1984
Don Andrews: Thu Dec 26 11:30:15 1985
Ivor Durham: Wed Jun 15 16:30:57 1988
Ed Taft: Sat Jul 11 13:11:02 1987
Bill Paxton: Thu Mar 10 09:09:56 1988
Jim Sandman: Fri Dec  2 15:46:24 1988
Joe Pasqua: Thu Jan 12 11:24:09 1989
End Edit History.
*/

#ifndef GRAY_H
#define GRAY_H

#include BASICTYPES
#include ENVIRONMENT
#include GRAPHICS
#include DEVICE

/* Data Structures */

typedef struct {
  cardinal val;
  cardinal index;
  } GrayQ, *PGrayQ;

/* Exported Procedures */

#define _gray extern

_gray procedure GetValidFreqAnglePair(/* PSpotFunction pSpot */);
_gray procedure GetValidFreqAngleOctet(/* PSpotFunction pSpot */);
/* Adjusts the frequency and angle for the spot function to fit with storage
   limitations of the device and the kernel. */


_gray procedure GenerateThresholds(/*PSpotFunction pSpot; DevScreen *devS */);
/* Allocates devS->thresholds and fills in devS->width, devS->height and
   actual threshold values. The frequency and angle values of pSpot should
   have been adjusted by either GetValidFreqAnglePair or
   GetValidFreqAngleOctet. */

_gray procedure SetDefaultHalftone();
/* Makes a screen using the DevHalftone from the device as the thresholds. */

_gray procedure AddScrRef( /* Screen screen; */ );
/* Must be called whenever a copy is made of a Screen.  This procedure
   adds to the reference count in the associated ScreenRec.  SetUpScreen
   calls this procedure itself for the Screen it returns.  This procedure
   may be called with NIL (for a no-op). */

_gray procedure RemScrRef( /* Screen screen; */ );
/* Must be called whenever a Screen is thrown away or overwritten.  This
   procedure will reclaim ScreenRec storage when the reference count goes
   to zero.  */

_gray procedure AddTfrRef( /* PTfrFcn *tfrFcn; */ );
/* Must be called whenever a copy is made of a TfrFcn.  This procedure
   adds to the reference count in the associated TfrFcnRec. This procedure
   may be called with NIL (for a no-op). */

_gray procedure RemTfrRef( /* PTfrFcn *tfrFcn; */ );
/* Must be called whenever a TfrFcn is thrown away or overwritten.  This
   procedure will reclaim TfrFcn storage when the reference count goes
   to zero.  */

_gray procedure ActivateTfr( /* PTfrFcn *tfrFcn; */ );
/* Makes TfrFcn active. Makes sure the function tables are allocated
   and filled in. */

extern procedure SetDevTfr();
extern procedure ClearDevTfr();

extern procedure SetColorForDevice();
   /* must be called before giving the device a color if
   device->colors != gs->nColors. */
   
_gray integer LCM4();
_gray PGrayQ GetPatternBase();
_gray procedure FreePatternBase();
_gray integer grayPatternLimit, tfrTableLimit;

/* Exported Data */

_gray integer curTfrFcnID;

#endif GRAY_H
