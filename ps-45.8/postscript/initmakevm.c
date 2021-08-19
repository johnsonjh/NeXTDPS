/*
  initmakevm.c

Copyright (c) 1989 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Fri Dec 15 10:23:58 1989
Edit History:
Ed Taft: Fri Dec 15 10:23:58 1989
End Edit History.

PSInitMakeVM is an optional procedure that the client can specify as
the PostScriptParameters.writeVMProc argument. If this is done, the
machinery for writing out VMs is included and the makevm operator is
registered in systemdict.
*/

#include PACKAGE_SPECS
#include LANGUAGE
#include PUBLICTYPES
#include VM

public PVoidProc PSInitMakeVM()
{
  RgstPARelocator();
  return (InitMakeVM);
}

