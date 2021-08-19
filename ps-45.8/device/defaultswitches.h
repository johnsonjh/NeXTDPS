/* Definitions for the PostScript device implementation.

   This module defines the default configuration options for the PostScript
   device implementation.

Copyright (c) 1990 Adobe Systems Incorporated.
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

#ifndef DEFAULTSWITCHES_H
#define DEFAULTSWITCHES_H

#ifndef SAMPLEDEVICE
#define SAMPLEDEVICE 0
#endif

#ifndef DISKDL
#if SAMPLEDEVICE
#define DISKDL 0
#else
#define DISKDL 0
#endif
#endif

#ifndef DPSXA
#if SAMPLEDEVICE
#define DPSXA 0
#else
#define DPSXA 0
#endif
#endif

#ifndef MASKCOMPRESSION
#if SAMPLEDEVICE
#define MASKCOMPRESSION 0
#else
#define MASKCOMPRESSION 1
#endif
#endif

#ifndef MULTICHROME
#if SAMPLEDEVICE
#define MULTICHROME 1
#else
#define MULTICHROME 0
#endif
#endif

#endif DEFAULTSWITCHES_H

