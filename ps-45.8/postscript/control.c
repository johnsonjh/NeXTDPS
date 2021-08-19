/*
  control.c

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Chuck Geschke: February 11, 1983
Edit History:
Scott Byer: Thu Jun  1 15:35:25 1989
Chuck Geschke: Tue Jan 28 16:50:08 1986
Doug Brotz: Thu Jun 26 15:28:12 1986
Ed Taft: Thu Nov 23 16:26:35 1989
John Gaffney: Tue Sep 24 21:31:19 1985
Bill Paxton: Mon Jan 14 12:45:37 1985
Jeff Peters: Thu Oct 31 14:19:38 1985
Don Andrews: Wed Sep 10 10:28:50 1986
Ivor Durham: Sun Aug 14 14:01:47 1988
Jim Sandman: Wed Mar 16 16:32:54 1988
Joe Pasqua: Fri Jan 13 14:58:12 1989
Paul Rovner: Wednesday, June 8, 1988 4:58:13 PM
Ramin Behtash: Tue Apr 19 13:11:56 1988
End Edit History.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include FP

#if (OS == os_mpw)
#include <OSUtils.h>
#define Fixed MPWFixed
#endif (OS == os_mpw)

#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include LANGUAGE
#include PSLIB
#include STREAM
#include VM

#include "postscriptnames.h"

private procedure PSUndef()
{
#if VMINIT
  Object ob; StrObj so;
  character s[MAXnameLength];
  PopP(&ob);
  NameToPString(ob, &so);
  so.length = MIN(MAXnameLength-1,so.length);
  VMGetText(so,s);
  os_printf("Undefined name: %s\n",s);
  RAISE(PS_STOP, (char *)NIL);
#else VMINIT
  CantHappen();
#endif VMINIT
}

public procedure PSUserTime()
{
  PushInteger((integer)os_userclock());
}

public procedure PSRealTime()
{
  PushInteger((integer)os_clock());
}

public procedure ControlInit(reason)  InitReason reason;
{
  switch (reason) {
   case init:{
      break;
    }
   case romreg:{
      Begin (rootShared->trickyDicts.val.arrayval[tdErrorDict]);
      RgstExplicit ("undefined", PSUndef);
      End ();
      break;
    }
   case ramreg:{
      break;
    }
   endswitch
  }
}
