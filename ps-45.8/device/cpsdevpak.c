/* PostScript expansion module for grpahics package

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

*/


#include PACKAGE_SPECS

#include "nulldev.h"
#include "devcommon.h"
#include "genericdev.h"
#include "framedev.h"
#include "framemaskdev.h"

public procedure RevealDPSCursor() {} /* called from ServicePostScript */
public procedure CheckScreenDevices() {}
public procedure SetPrebuiltPath() {}
public integer DevRgstPrebuiltFontInfo( /* stub prebuilt stuff from device.h */
  font, fontName, length, stdEncoding, proc, procData)
  integer font; char *fontName; integer length;
  boolean stdEncoding; CharNameProc proc; char *procData;
  {
  return 0;
  }

public DevHalftone *defaultHalftone;
private DevHalftone halftone;
private DevScreen defaultScreen;
private unsigned char defaultThresholds[24] = { /* XXX */
   63, 233, 127, 191,
  255, 170,  85,  42,
  106,  21, 212, 148,
  127, 191,  63, 233,
   85,  42, 255, 170,
  212, 148, 106,  21};

public procedure PSDeviceInit()
{
  defaultScreen.width = 4;
  defaultScreen.height = 6;
  defaultScreen.thresholds = defaultThresholds;
  halftone.priv = (DevPrivate *)1;
  halftone.white = halftone.red = halftone.blue = 
    halftone.green = &defaultScreen;
  defaultHalftone = &halftone;
  InitPatternImpl((integer)8000, (integer)40000, (integer)8000, (integer)40000);

  IniDevCommon();
  IniGenDevImpl();
  IniFmDevImpl();
  IniMaskDevImpl();
  IniNullDevImpl();
  InitMaskCache(1500, 120000, 60000, 250000, 12500);
}  /* end of PSDeviceInit */

