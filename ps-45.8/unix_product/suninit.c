/*
  suninit.c

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
Doug Brotz: Wed Jan  7 19:43:36 1987
Chuck Geschke: Wed Oct  9 22:16:59 1985
Ed Taft: Fri Jun 19 13:38:38 1987
Jeff Peters: Tue Nov  5 16:41:23 1985
Bill Paxton: Thu Mar 12 13:16:34 1987
Jim Sandman: Mon Nov  6 10:35:49 1989
Don Andrews: Wed Oct 22 11:36:35 1986
Ivor Durham: Sun Oct 30 11:47:23 1988
Joe Pasqua: Mon Jan 16 14:18:06 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include GRAPHICS
#include DEVICE
#include VM

extern procedure IniSunDevice(), IniSunInput();

public procedure ProductInit(reason)  InitReason reason;
{
switch (reason)
  {
  case init:
    break;
  case romreg:
    break;
  endswitch
  }
  /*
   * IniSunDevice and IniSunInput both have initialisation "switch"
   * statements and so must be called each time DeviceInit is called
   * rather than just in the "romreg" case as it was before.  They
   * also take care of testing vSUNTOOL when appropriate.
   */

   IniSunInput(reason);

}  /* end of ProductInit */


