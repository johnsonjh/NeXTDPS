/******************************************************************************

    kbdroutines.c

    User-level end of keyboard interface routines.
    
    CONFIDENTIAL
    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All Rights Reserved.

    Created 02Jan88 Leo
    
    Modified:

    09Jun88 Leo Floating point time values
    13Sep88 Leo No more Mr. Nice Guy -- all config routines gone
    24Aug89 Ted ANSI C Prototyping.

******************************************************************************/

#import PACKAGE_SPECS
#import <sys/types.h>
#import <sys/ioctl.h>
#import <nextdev/evio.h>
#import "vars.h"

int StillDown(int eNum)
{
    ioctl(MouseFd(), EVIOSD, &eNum); /* Returns in situ */
    return(eNum);
}

int RightStillDown(int eNum)
{
    ioctl(MouseFd(), EVIORSD, &eNum); /* Returns in situ */
    return(eNum);
}

void SetMouse(int x, int y)
{
    Point p;
    
    p.x = x;
    p.y = y;
    ioctl(MouseFd(), EVIOSM, &p);
}

void CurrentMouse(int *xAt, int *yAt)
{
    Point p;
    
    MouseFd();
    *xAt = eventGlobals->cursorLoc.x;
    *yAt = eventGlobals->cursorLoc.y;
}



