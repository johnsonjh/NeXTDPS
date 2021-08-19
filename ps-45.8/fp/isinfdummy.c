/*
  isinfdummy.c

Copyright (c) 1987, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version:
Edit History:
Ed Taft: Thu Oct  5 15:57:58 1989
End Edit History.

generic os_isinf and os_isnan procedures for architectures that do not
have "infinity" and "NaN" representations
*/

#include PACKAGE_SPECS
#include FPFRIENDS

public boolean os_isinf(d)
  double d;
  {return false;}

public boolean os_isnan(d)
  double d;
  {return false;}

public boolean IsValidReal(pReal)
  float *pReal;
  {return true;}
