/* Initialize the PostScript device implementation

Copyright (c) 1983, '84, '85, '86, '87, '88, '89, '90 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

14May90 Jack args to InitMaskCache maximize bmFree->header.length w/o overflow 

*/


#include PACKAGE_SPECS

#include "nulldev.h"
#include "devcommon.h"
#include "genericdev.h"
#include "framedev.h"
#include "framemaskdev.h"

#define	SIZE_FONT_CACHE         10
/* The size in bytes of the font cache. - keep in sync with sizes.h */
#define	SIZE_MASKS              11
/* The number of masks to cache - keep in sync with sizes.h */

extern int ps_getsize(); /* imported from the VM package */
 
#if (DISABLEDPS)
public procedure SetPrebuiltPath() {}
public integer DevRgstPrebuiltFontInfo( /* stub prebuilt stuff from device.h */
  font, fontName, length, stdEncoding, proc, procData)
  integer font; char *fontName; integer length;
  boolean stdEncoding; CharNameProc proc; char *procData;
  {
  return 0;
  }
#endif (DISABLEDPS)

public DevHalftone *defaultHalftone;

public procedure PSDeviceInit()
{
  InitPatternImpl((integer)8000, (integer)40000, (integer)8000, (integer)40000);

  IniDevCommon();
  IniGenDevImpl();
  IniFmDevImpl();
#if (OS == os_mach)
  IniMpdDevImpl();
#endif
  IniWdDevImpl();
  IniMaskDevImpl();
  IniNullDevImpl();
  InitMaskCache(ps_getsize(SIZE_MASKS,1800) + 200, 
		ps_getsize(SIZE_FONT_CACHE, 120000), 60000, 6000000, 12500);
  IniPreBuiltChars();
}  /* end of PSDeviceInit */



