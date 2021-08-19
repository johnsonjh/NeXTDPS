/* PostScript frame mask device definitions

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

#ifndef MASKDEVICE_H
#define MASKDEVICE_H

#include DEVICE
#include DEVPATTERN
#include "framedev.h"

typedef struct {	/* concrete "Device" type for a charmask subdevice */
  FmStuff fm;
  PMask mask;
  DevFixedPoint *offset;
} MaskStuff, *PMaskStuff;


#define MaskUnitWidth(mask) (((mask)->width + SCANUNIT - 1) >> SCANSHIFT)

#if (MULTICHROME == 1)
extern SCANTYPE *source8bits, *source4bits;
extern PCard16 source2bits;
#endif

#define BANDMASKID 256

/* the following procedures and variables are exported by maskdev.c */

extern DevProcs *maskProcs;
extern PMarkProcs maskMarkProcs;
extern PatternHandle gMaskPattern; /* cached result from MaskPattern(true); */

extern boolean CmptMaskBBMin(
  /* SCANTYPE *cmb; integer wunits, height; DevPoint *ll, *ur; */
  );

extern PDevice FmMakeMaskDevice (
  /* PDevice device; MakeMaskDevArgs *args; */);

extern procedure IniMaskDevImpl();

#endif MASKDEVICE_H
