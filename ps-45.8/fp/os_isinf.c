/*
  os_isinf.c

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

Original version: Ed Taft, April 1988
Edit History:
Ed Taft: Thu Dec  7 08:38:31 1989
End Edit History.

This module implements one of the os_xxx operations as a simple veneer
over the corresponding procedure in the C runtime library.
*/

#include PACKAGE_SPECS
#include "os_math.h"


public boolean os_isinf(x)
  double x;
{
  return isinf(x);
}

public boolean os_isnan(x)
  double x;
{
  return isnan(x);
}

public boolean IsValidReal(pReal)
  float *pReal;
{
  double x = *pReal;
  return ! isnan(x) && ! isinf(x);
}
