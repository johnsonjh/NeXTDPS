/*      

Copyright (c) 1983, '84, '85, '86, '87, '88 Adobe Systems Incorporated.
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
Scott Byer: Wed May 17 14:56:54 1989
Bill Paxton: Sat Oct 17 14:20:22 1987
Paul Rovner: Sunday, January 10, 1988 11:28:05 PM
Jim Sandman: Fri Aug 12 12:04:38 1988
Ivor Durham: Mon Feb  8 10:15:33 1988
Joe Pasqua: Mon Jul 10 16:03:33 1989
End Edit History.
*/

#ifndef WINCLIP_H
#define WINCLIP_H

#include PUBLICTYPES
#include DEVICE

/*	===== BEGIN	TYPE DECLS =====	*/
typedef struct _t_wininfo {
  DevPrivate *window;
    /* Window system specific info for this window	*/
  DevPoint origin;
    /* Xlation from window to device coordinates	*/
  DevPrim *winclip;
    /* Intersection of DPS clip and window clip.	*/
  } WinInfo;


/*	===== BEGIN	PUBLIC Routines =====	*/
extern procedure InitWinClip();
  /* Call this procedure to initialize the winclip pkg	*/
  /* This must be called before any other procedure.	*/

extern DevPrim *FindDevWinClip( /* DevPrim *devclip, *winclip; */ );
  /* Find a clip in the cache that is the intersection	*/
  /* of the two input clips. If the cache doesn't have	*/
  /* that intersection, install it in the cache.	*/

extern procedure SetWindow(
  /* PDevice device; DevPoint origin; DevPrim *winclip; DevPrivate *window*/);
  /* Set the window information for a device object.	*/
  /* If device doesn't already have a WinInfo assoc'd.	*/
  /* with it, create one. If it does have a WinInfo,	*/
  /* free the winclip and reuse that structure. Set the	*/
  /* WinInfo fields with the parameters origin, winclip	*/
  /* and window.					*/

extern procedure TermDevWinClip( /* DevPrim *clip; */ );
  /* Flush clip cache of entries using this clip. Call	*/
  /* when deleting the window or flushing a devclip	*/

extern procedure TermWindow( /* PDevice device */ );
  /* Free the clip info currently assoc'd. w/ device,	*/
  /* then free the WinInfo struct assoc'd. w/ device.	*/


#endif WINCLIP_H
