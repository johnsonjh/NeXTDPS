/* Definitions for the PostScript device implementation.

   This module defines the "generic" device object, which
   is the ancestor of all other device objects except the null device.

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

#ifndef GENDEVICE_H
#define GENDEVICE_H

#include DEVICE

#ifndef SAMPLEDEVICE
#define SAMPLEDEVICE 0
#endif

#ifndef MASKCOMPRESSION
#if (SAMPLEDEVICE)
#define MASKCOMPRESSION 0
#else
#define MASKCOMPRESSION 1
#endif
#endif

#ifndef MULTICHROME
#if (SAMPLEDEVICE)
#define MULTICHROME 1
#else
#define MULTICHROME 0
#endif
#endif

typedef struct _t_GenStuff{	/* concrete "Device" type for a generic device */
  Device  d;		       /* must be first */
  DevLBounds bbox;
  Mtx     matrix;  
#if (MULTICHROME == 1)
  integer colors;    
#endif
} GenStuff, *PGenStuff;

extern DevProcs *genProcs;

#endif GENDEVICE_H
