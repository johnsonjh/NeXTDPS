/* winblt.c

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

Edit History: created by Paxton
Paul Rovner: Wednesday, December 30, 1987 4:34:20 PM
Jim Sandman: Tue May 10 15:46:17 1988
Perry Caro: Tue Nov  8 15:00:39 1988
End Edit History.
*/
  

#include PACKAGE_SPECS
#include DEVICE

#include "devsupport.h"
#include "framedevice.h"
#include "winclip.h"

public procedure ScrollLines(device, bounds, dy)
  PDevice device; DevBounds *bounds; int dy; {
  register PSCANTYPE destunit, sourceunit;
  register int h, u, fbw, cnt;
  DevShort yb, yt, xl, xr, totalunits, unitl, bytes;
  WinInfo *w;
  DevPrim *winclip;
  integer scanshift = SCANSHIFT - framelog2BD;
  integer scanunit = SCANUNIT >> framelog2BD;

  yb = bounds->y.l;
  yt = bounds->y.g;
  xl = bounds->x.l;
  xr = bounds->x.g;
  if (device->priv != NULL) {
    w  = (WinInfo *)(device->priv);
    winclip = w->winclip;
    if (dy < 0) 
      if (yb + dy < winclip->bounds.y.l) yb = winclip->bounds.y.l - dy;
    else
      if (yb < winclip->bounds.y.l) yb = winclip->bounds.y.l;
    if (dy < 0)
      if (yt > winclip->bounds.y.g) yt = winclip->bounds.y.g;
    else
      if (yt + dy > winclip->bounds.y.g) yt = winclip->bounds.y.g - dy;
    if (xl < winclip->bounds.x.l) xl = winclip->bounds.x.l;
    if (xr > winclip->bounds.x.g) xr = winclip->bounds.x.g;
    yb += w->origin.y; yt += w->origin.y;
    xl += w->origin.x; xr += w->origin.x;
    }
  h = yt - yb;
  if (h <= 0) return;
  h--;
  unitl = xl >> scanshift;
  totalunits = ((xr+scanunit) >> scanshift) - unitl;
  if (totalunits <= 0) return;
  cnt = (totalunits >> 1) - 1;
  bytes = totalunits * sizeof(SCANTYPE);
  fbw = framebytewidth;
  if (dy < 0) { /* move block to lower addresses */
    destunit = (PSCANTYPE)((integer)framebase + (yb+dy) * fbw) + unitl;
    sourceunit = (PSCANTYPE)((integer)framebase + yb * fbw) + unitl; 
    fbw -= bytes;
    if (totalunits == 1) {
      do {
        *destunit++ = *sourceunit++;
        sourceunit = (PSCANTYPE)((integer)sourceunit + fbw);
        destunit = (PSCANTYPE)((integer)destunit + fbw);
        } while (--h >= 0);
      }
    else if (totalunits & 1) {
      do {
        *destunit++ = *sourceunit++;
        u = cnt;
        do {
          *destunit++ = *sourceunit++;
	  *destunit++ = *sourceunit++;
	  } while (--u >= 0);
        sourceunit = (PSCANTYPE)((integer)sourceunit + fbw);
        destunit = (PSCANTYPE)((integer)destunit + fbw);
        } while (--h >= 0);
      }
    else {
      do {
        u = cnt;
        do {
          *destunit++ = *sourceunit++;
	  *destunit++ = *sourceunit++;
	  } while (--u >= 0);
        sourceunit = (PSCANTYPE)((integer)sourceunit + fbw);
        destunit = (PSCANTYPE)((integer)destunit + fbw);
        } while (--h >= 0);
      }
    }
  else { /* move to higher addresses */
    sourceunit = (PSCANTYPE)((integer)framebase + (yt-1) * fbw) + unitl; 
    destunit = (PSCANTYPE)((integer)framebase + (yt-1+dy) * fbw) + unitl;
    fbw += bytes;
    if (totalunits == 1) {
      do {
        *destunit++ = *sourceunit++;
        sourceunit = (PSCANTYPE)((integer)sourceunit - fbw);
        destunit = (PSCANTYPE)((integer)destunit - fbw);
        } while (--h >= 0);
      }
    else if (totalunits & 1) {
      do {
        *destunit++ = *sourceunit++;
        u = cnt; 
        do {
          *destunit++ = *sourceunit++;
	  *destunit++ = *sourceunit++;
	  } while (--u >= 0); 
        sourceunit = (PSCANTYPE)((integer)sourceunit - fbw);
        destunit = (PSCANTYPE)((integer)destunit - fbw);
        } while (--h >= 0);
      }
    else {
      do {
        u = cnt; 
        do {
          *destunit++ = *sourceunit++;
	  *destunit++ = *sourceunit++;
	  } while (--u >= 0); 
        sourceunit = (PSCANTYPE)((integer)sourceunit - fbw);
        destunit = (PSCANTYPE)((integer)destunit - fbw);
        } while (--h >= 0);
      }
    }    
  }
  
              
