/*   
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

#ifndef DEVCOMMON_H
#define DEVCOMMON_H

#include DEVICE

#define MAXCOLOR MAXCard8

extern PDevice CurrentDevice;
extern DevHalftone *defaultHalftone;

extern DevColor ConvertColorRGB (); /* see device.h for args */

extern DevColor ConvertColorCMYK (); /* see device.h for args */

extern procedure DevNoOp (/* PDevice device; */);
  
extern boolean DevAlwaysFalse (/* PDevice device; */);

extern procedure IniDevCommon();
extern procedure IniDevImpl();
extern boolean AwaitPipelineProgress();

extern procedure Mark();

extern procedure ImageTraps(/*
  DevTrap *t; integer nTraps; ImageArgs *args; */);
  
extern procedure ImageRun(/*
  DevRun *run; ImageArgs *args; */);
  
extern procedure TrapTrapDispatch();
extern procedure ClipTrapsRunDispatch();

#endif DEVCOMMON_H
