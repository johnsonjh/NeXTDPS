/*
  fonts.h

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

Original version: Chuck Geschke: July 14, 1983
Edit History:
Chuck Geschke: Mon Jan 20 22:39:44 1986
Doug Brotz: Mon Jul 21 09:51:19 1986
Andrew Shore: Fri Mar 30 13:41:38 1984
Ed Taft: Tue Jun  7 11:06:51 1988
Bill Paxton: Wed Jun 20 09:46:39 1984
Don Andrews: Tue Dec 17 10:11:28 1985
Peter Hibbard: Mon Dec 30 17:28:25 1985
Ivor Durham: Mon Feb  8 21:26:01 1988
Jim Sandman: Tue Dec 13 16:41:11 1988
Joe Pasqua: Wed Dec 14 15:16:33 1988
End Edit History.

This interface is presently quite incomplete.
*/

#ifndef	FONTS_H
#define	FONTS_H

#include BASICTYPES

/*
 * fontbuild interface
 */

extern	Cd		LockCd(/*cp:Cd*/);
extern	procedure	LockPCd(/*cp:Cd, pcd:PCd*/);
extern	procedure	SetXLock(/*c1,c2:real*/);
extern	procedure	SetYLock(/*c1,c2:real*/);
extern	procedure	StartLock();
extern  procedure       RgstFontsSemaphoreProc (/* PVoidProc */);
/*
 * fontcache interface
 */

/* procedures PurgeCip, SetFont, and FindInCache defined in graphics.h
   due to package circularities.
 */

/*
 * fontdskcache interface
 */

extern procedure SetUserDiskPercent(/* integer pct; */);
/* Sets minimum percentage of disk capacity to be guaranteed available
   for user files. This procedure deletes system files (cached characters)
   if necessary to meet the guarantee. It raises a disk exception if
   that is not possible or a disk error occurs. */

#endif	FONTS_H
/* v004 durham Sun Feb 7 14:10:46 PST 1988 */
/* v005 sandman Thu Mar 10 09:35:05 PST 1988 */
/* v006 durham Fri Apr 8 11:35:18 PDT 1988 */
/* v007 taft Tue Jun 7 11:30:46 PDT 1988 */
/* v008 durham Thu Aug 11 13:10:17 PDT 1988 */
/* v009 caro Wed Nov 9 14:41:00 PST 1988 */
/* v010 pasqua Wed Dec 14 17:51:48 PST 1988 */
/* v011 byer Tue May 16 15:17:35 PDT 1989 */
/* v013 taft Thu Nov 23 15:03:23 PST 1989 */
/* v014 thompson Mon Feb 12 16:33:39 PST 1990 */
