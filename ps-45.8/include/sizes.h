/*
  sizes.h

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ivor Durham: Mon Aug 15 20:12:37 1988
Edit History:
Ivor Durham: Sat Oct 29 15:29:13 1988
Jim Sandman: Mon Oct 17 11:15:45 1988
End Edit History.
*/

#ifndef	SIZES_H
#define	SIZES_H

/* Size parameter identifiers */

#define	SIZE_CUSTOMDICT			0
  /* Size of the product specific custom operator dictionary */
#define	SIZE_VM_EXPANSION		1
  /* Size in bytes of VM default expansion segments */
#define	SIZE_GC_MIN_THRESHOLD		2
  /* The minimum allocation threshold value for any private VM. */
#define	SIZE_GC_MAX_THRESHOLD		3
  /* The maximum allocation threshold value for any private VM. */
#define	SIZE_GC_DEF_THRESHOLD		4
  /* The default allocation threshold for all private VM's. */
#define	SIZE_GC_SHARED_THRESHOLD	5
  /* The default allocation threshold for shared VM. */
#define	SIZE_GC_OVERSIZE_THRESHOLD	6
  /* Per VM threshold for total memory usage beyond which more aggressive
     collection strategy is used. */
#define	SIZE_GS_DEVICE_DATA		7
  /* The size in bytes of the extra data to be carried in a GState for
     the device. */
#define	SIZE_IMAGE_BUFFER		8
  /* The size in bytes of the buffer the image operators will use to hold
     samples. */
#define	SIZE_UPATH_CACHE		9
  /* The size in bytes of the user path cache. */
#define	SIZE_FONT_CACHE			10
  /* The size in bytes of the font cache. */
#define	SIZE_MASKS			11
  /* The number of masks to cache */
#define	SIZE_MIDS			12
  /* The number of MIDs to cache */
#define	SIZE_TFR_FCN_POOL		13
  /* Number of transfer function arrays that are cached. */
#define	SIZE_MAX_HALFTONE_PATTERN	14
  /* The maximum area of a threshold array in a DevScreen. */
#define	SIZE_MAX_PATTERN		15
  /* Maximum number of bytes for a single halftone. Used as parameter
     to InitPatternImpl. */
#define	SIZE_MAX_TOTAL_PATTERN		16
  /* Maximum number of bytes cached by pattern implementation. Used as
     parameter to InitPatternImpl. */
#define	SIZE_ID_SPACE			17
  /* The combined total number of context and space id's.
     Must not exceed 1024 in current implementation */
#define	SIZE_SPARE_18			18
#define	SIZE_SPARE_19			19
#define	SIZE_SPARE_20			20
#define	SIZE_LAST			21	/* Index of last parameter */

#define	SIZE_UNDEFINED	0x8000000

/* Exported Procedures */

extern int ps_getsize (/* int sizeIndex, defaultSize */);
 /*
   Returns the integer size parameter identified by "sizeIndex" unless the
   parameter value is the special SIZE_UNDEFINED value, in which case the
   "defaultSize" is returned instead.
  */

/* Inline Procedures */

/* Exported Data */

#endif	SIZES_H
