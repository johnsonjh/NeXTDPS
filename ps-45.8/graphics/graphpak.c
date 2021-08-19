/*
  graphpak.c

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

Original version: Chuck Geschke: November 15, 1983
Edit History:
Chuck Geschke: Wed Nov 16 12:04:37 1983
Doug Brotz: Tue May  6 22:15:55 1986
Ivor Durham: Tue Jul 12 14:54:18 1988
Ed Taft: Fri Jun 26 16:09:33 1987
Jim Sandman: Thu Aug  3 09:55:52 1989
Bill Paxton: Thu Sep 15 10:28:13 1988
Joe Pasqua: Thu Jan 12 11:20:10 1989
End Edit History.
*/


#define GSMASK 0

#include PACKAGE_SPECS
#include COPYRIGHT
#include BASICTYPES
#include GRAPHICS
#include ORPHANS
#include "graphdata.h"

public procedure GraphicsInit(reason)
  InitReason reason;
{
  extern procedure IniNullDevice ();
  extern procedure IniStroke ();

  extern procedure PathBuildInit ();
  extern procedure MarkInit ();

  extern procedure IniPathPriv ();
  extern procedure IniUCache ();
  extern procedure IniGraphics ();
  extern procedure IniTransfer ();
  extern procedure IniGStates ();
  extern procedure IniViewClip ();
  extern procedure GrayInit ();
  extern procedure IniReducer ();
  extern procedure IniClrSpace ();

  extern procedure GStateDataHandler ();

  switch (reason) {
   case init:
    RegisterData (
      (PCard8 *)&graphicsStatics, (integer)sizeof(GraphicsData),
      GStateDataHandler,
      (integer)(
        STATICEVENTFLAG (staticsCreate) | STATICEVENTFLAG (staticsDestroy) |
	STATICEVENTFLAG (staticsLoad) | STATICEVENTFLAG (staticsUnload)));
    break;
  }

  IniNullDevice (reason);	/* must precede IniGraphics ! */
  IniReducer (reason);
  IniStroke (reason);
  PathBuildInit (reason);
  MarkInit (reason);
  IniPathPriv (reason);
  IniUCache (reason);
  GrayInit (reason);		/* must precede IniGraphics ! */
  IniGraphics (reason);
  IniTransfer (reason);
  IniGStates (reason);
  IniViewClip (reason);
  IniClrSpace (reason);
}				/* end of GraphPakInit */
