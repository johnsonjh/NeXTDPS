/*
  unixcustom.c

Copyright (c) 1987, '88, '89 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ivor Durham  Sept. 28, 1987
Edit History:
Ivor Durham: Mon Aug 22 09:37:00 1988
Joe Pasqua: Mon Jul 10 16:35:41 1989
Ed Taft: Fri Nov 24 17:37:07 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include FP
#include LANGUAGE
#include ORPHANS
#include PSLIB
#include STREAM

#include "unixcustom.h"

public longcardinal highwatersp, lowwatersp;
private boolean mntring;
private procedure PSPerfMonitor();

private procedure PSbye ()
{
  if (mntring)
    PSPerfMonitor ();	/* Turn off monitoring */
  os_exit (0);
}

private procedure PSPerfMonitor()
{
#if(OS != os_mach)
  if (!mntring){
    os_printf("Performance monitoring -- on.\n"); fflush(os_stdout);
    mntring = true;
#if (OS == os_vaxeln)
    {
    extern init_pc(); integer monParams = 0;
    ker$enter_kernel_context(NULL, init_pc, &monParams);
    }
#else (OS == os_vaxeln)
    {
    extern LOWADDR(), HIGHADDR();
    monstartup(LOWADDR, HIGHADDR);
    }
#endif (OS == os_vaxeln)
    return;}
  else {
#if (OS == os_vaxeln)
    {
    extern write_pc(); integer monParams = 0;
    ker$enter_kernel_context(NULL, write_pc, &monParams);
    }
#else (OS == os_vaxeln)
    monitor((char *)0);
#endif (OS == os_vaxeln)
    os_printf("Performance monitoring -- off.\n"); fflush(os_stdout);
    mntring = false;
    return;}
#endif (OS != os_mach)
}

extern longcardinal highwatersp;

extern longcardinal lowwatersp;

private procedure PSMaxSP()
  {
   register integer temp;
   PushInteger((integer)(temp = lowwatersp - highwatersp, os_labs(temp)));
  }

private readonly RgCmdTable customOperators = {
  "bye",		PSbye,
  "perfmonitor",	PSPerfMonitor,
  "maxstackpointer",	PSMaxSP,
  NIL
};

UnixCustomOpsInit (reason)
  InitReason reason;
{
  switch (reason) {
   case init:
    lowwatersp = (longcardinal) &reason;
      /* well, not quite, but the best we can do */
    highwatersp = MAXunsignedinteger;
    break;
   case romreg:
    Begin (rootShared->vm.Shared.sysDict);
    if (! MAKEVM) RgstMCmds (customOperators);
    End ();
    break;
   endswitch
  }
}
