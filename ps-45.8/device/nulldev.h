/* Interface to the PostScript nulldev implementation

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

#ifndef NULLDEV_H
#define NULLDEV_H

#include DEVICE

extern DevProcs *nullProcs;

extern PDevice MakeNullDevice (/* PDevice parent; */);
extern procedure IniNullDevImpl();

#endif NULLDEV_H
