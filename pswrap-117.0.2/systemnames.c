/*
  systemnames.c

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version:
Edit History:
Richard Cohn: Fri Oct 21 15:39:16 1988
End Edit History.
*/

#ifdef os_mpw
#include <stdio.h>
#include <Files.h>
#include <Memory.h>
#include <OSUtils.h>
#include <Resources.h>
#endif

#include "pswdict.h"

PSWDict wellKnownPSNames;

void InitWellKnownPSNames()
{
#ifdef os_mpw
#include "macsysnames.c"
#else
#include "sysnames_gen.c"
#endif
}
