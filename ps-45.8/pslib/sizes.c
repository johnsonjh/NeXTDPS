/*
  sizes.c

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
Ivor Durham: Sun Oct 30 16:52:20 1988
Jim Sandman: Mon Oct 17 14:11:14 1988
Joe Pasqua: Fri Jan  6 15:53:04 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include EXCEPT
#include PUBLICTYPES
#include SIZES

private int sizeTable[SIZE_LAST + 1] = {
  SIZE_UNDEFINED,	/* SIZE_CUSTOMDICT */
  SIZE_UNDEFINED,	/* SIZE_VM_EXPANSION */
  SIZE_UNDEFINED,	/* SIZE_GC_MIN_THRESHOLD */
  SIZE_UNDEFINED,	/* SIZE_GC_MAX_THRESHOLD */
  SIZE_UNDEFINED,	/* SIZE_GC_DEF_THRESHOLD */
  SIZE_UNDEFINED,	/* SIZE_GC_SHARED_THRESHOLD */
  SIZE_UNDEFINED,	/* SIZE_GC_OVERSIZE_THRESHOLD */
  SIZE_UNDEFINED,	/* SIZE_GS_DEVICE_DATA */
  SIZE_UNDEFINED,	/* SIZE_IMAGE_BUFFER */
  SIZE_UNDEFINED,	/* SIZE_UPATH_CACHE */
  SIZE_UNDEFINED,	/* SIZE_FONT_CACHE */
  SIZE_UNDEFINED,	/* SIZE_MASKS */
  SIZE_UNDEFINED,	/* SIZE_MIDS */
  SIZE_UNDEFINED,	/* SIZE_TFR_FCN_POOL */
  SIZE_UNDEFINED,	/* SIZE_MAX_HALFTONE_PATTERN */
  SIZE_UNDEFINED,	/* SIZE_MAX_PATTERN */
  SIZE_UNDEFINED,	/* SIZE_MAX_TOTAL_PATTERN */
  256,			/* SIZE_ID_SPACE */
  SIZE_UNDEFINED,	/* SIZE_SPARE_16 */
  SIZE_UNDEFINED,	/* SIZE_SPARE_17 */
  SIZE_UNDEFINED	/* SIZE_SPARE_18 */
};

public int ps_getsize (sizeIndex, defaultSize)
  int sizeIndex, defaultSize;
{
  if ((sizeIndex < 0) || (sizeIndex > SIZE_LAST))
    RAISE (ecRangeCheck, (char *)NIL);
  else {
    int size;
    switch (sizeIndex) {
        case SIZE_FONT_CACHE:
	       size = GetCSwitch("CACHE", SIZE_UNDEFINED);
	       break;
        case SIZE_MASKS:
	       size = GetCSwitch("MASKS", SIZE_UNDEFINED);
	       break;
        case SIZE_MIDS:
	       size = GetCSwitch("MIDS", SIZE_UNDEFINED);
	       break;
	    default:
	       size = SIZE_UNDEFINED;
	       break;

    }
    if(size != SIZE_UNDEFINED)
	    return(size);
    if (sizeTable[sizeIndex] == SIZE_UNDEFINED)
        return (defaultSize);
    else
        return (sizeTable[sizeIndex]);
  }
}
