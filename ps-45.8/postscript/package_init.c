/*
  package_init.c

Copyright (c) 1986, '87 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ivor Durham: November 11, 1986
Edit History:
Ivor Durham: Sat Aug 27 14:22:00 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include BASICTYPES

#if (OS == os_vms)
globalref void (*packageInitProcedure[])();
#else (OS == os_vms)
extern void (*packageInitProcedure[])();
#endif (OS == os_vms)

void InitializePackages(Reason)
  InitReason Reason;
{
  int     i;

  for (i = 0; packageInitProcedure[i] != NIL; i++)
    (*packageInitProcedure[i]) (Reason);
}
