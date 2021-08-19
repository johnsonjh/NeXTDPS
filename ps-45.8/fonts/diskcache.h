/*
  diskcache.h

Copyright (c) 1986, 1987 Adobe Systems Incorporated.
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
Ivor Durham: Tue Nov 25 15:09:04 1986
Ed Taft: Sun Jun  5 15:46:09 1988
Jim Sandman: Mon Oct 12 17:07:17 1987
End Edit History.
*/

#ifndef	DISKCACHE_H
#define	DISKCACHE_H

#include BASICTYPES

/*
 * Font disk cache support procedures called from elsewhere in the
 * font machinery. These are arranged as a record of call-back procs
 * so that the font disk cache support can be optional.
 *
 * Note: the procedures FCDBDelete, FCDBChangeSize, FCDBNewPages,
 * and SetUserDiskPercent are logically part of this interface;
 * however, they need not be included explicitly since they are
 * never called in no-disk-cache configurations.
 */
  
typedef struct {
  procedure (*ReInitFontCache)();
  integer (*CharFromDisk)(/* PNameObj pcn; MID mid */);
  boolean (*CharToDisk)(/* PNameObj pn; MID mid; BMOffset bmo; */);
  procedure (*FDskProc)(/*when*/);
  procedure (*FreeFC)();
  procedure (*FreeMF)();
  procedure (*InitMF)();
  boolean (*InsertMF)(/*MID*/);
  procedure (*MFFree)(/*MID*/);
  integer sizeMFItem;
} FDC;
  
extern FDC fdc;
  

#endif	DISKCACHE_H
