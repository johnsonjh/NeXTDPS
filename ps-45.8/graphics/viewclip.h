/*
  viewclip.h

Copyright (c) 1987, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: 
Edit History:
Joe Pasqua: Tue Nov  3 11:07:30 1987
Bill Paxton: Sat Mar 12 10:37:04 1988
Ivor Durham: Wed Jun 29 09:59:17 1988
End Edit History.
*/

#ifndef VCLIP_H
#define VCLIP_H

/* ViewClips are clippaths that apply to all subsequent	*/
/* graphics states. Obey save-restore. */
typedef struct _viewclip {
  struct _viewclip *next; 	/* Linked list of these	*/
  BitField	evenOdd:8;
  BitField	baseSave:8;	/* Savelevel @ creation	*/
  Path		path;		/* What I actually am	*/
} ViewClip, *PViewClip;

#endif VCLIP_H
