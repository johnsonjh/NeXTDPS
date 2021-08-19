/*   devmark.h
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



Edit History:
Jim Sandman: Tue Aug 23 16:28:47 1988
Bill Paxton: Tue Oct 20 08:25:07 1987
Paul Rovner: Tue Nov 21 10:27:58 1989
Ivor Durham: Fri Jul  8 16:44:05 1988
End Edit History.
*/

#ifndef DEVMARK_H
#define DEVMARK_H

#include PUBLICTYPES
#include DEVICE
#include DEVPATTERN
#include DEVIMAGE


typedef struct {
  procedure (*MasksMark)(/* DevMask *m; integer nMasks; MarkArgs *args; */);
  procedure (*TrapsMark)(/* DevTrap *t; integer nTraps; MarkArgs *args; */);
  procedure (*RunMark)(/* DevRun *run; MarkArgs *args; */);
  } SimpleMarkProcsRec, *PSimpleMarkProcs;

typedef struct {
  SimpleMarkProcsRec black;
  SimpleMarkProcsRec white;
  SimpleMarkProcsRec constant;
  SimpleMarkProcsRec gray;
  procedure (*ClippedMasksMark)(
    /* DevTrap *t; DevRun *r; DevMask *m; integer nMasks; MarkArgs *args; */);
  procedure (*SetupImageArgs)(/* PDevice device; ImageArgs *args; */);
  procedure (*ImageTraps)(/* DevTrap *t; integer nTraps; ImageArgs *args; */);
  procedure (*ImageRun)(/* DevRun *run; ImageArgs *args; */);
  PImageProcs imageProcs;
  } MarkProcsRec, *PMarkProcs;

typedef struct {
  DevMarkInfo *markInfo;
  PatternData patData;
  PatternHandle pattern;
  PMarkProcs procs;
  PDevice device;
  } MarkArgs;
  
#endif DEVMARK_H
