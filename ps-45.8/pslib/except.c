/*
  except.c

Copyright (c) 1984, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Jeffrey Mogul, Stanford, 18 February 1983
Edit History:
Ed Taft: Thu May 12 14:43:17 1988
Ivor Durham: Sun Aug 14 20:12:48 1988
Joe Pasqua: Fri Jan  6 15:05:41 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include EXCEPT

extern int os_eprintf(); /* from STREAM, which we cannot import */

public _Exc_Buf *_Exc_Header;

public procedure os_raise(code, msg)
  int code; char *msg;
{
  register _Exc_Buf *EBp = _Exc_Header;

  if (EBp == 0)		/* uncaught exception */
    {
    os_eprintf("Uncaught exception: %d  %s\n", code, (msg == NIL)? "" : msg);
    CantHappen();
    }

  EBp->Code = code;
  EBp->Message = msg;
  _Exc_Header = EBp->Prev;
  longjmp(EBp->Environ, 1);
}

/* The following definition determines the offset, in bytes, of the
   caller's program counter on the stack relative to the current
   procedure's first local variable. Naturally, this is machine-dependent.
*/

#define PCOFFSET \
  ((MC68K)? 8 : \
   (ISP==isp_vax)? ((OS==os_vms || OS==os_vaxeln)? 24 : 20) : \
   0)

public procedure CantHappen()
{
  integer localvar[1];

#if (PCOFFSET == 0)
  os_eprintf("Fatal system error (address unknown)\n");
#else (PCOFFSET == 0)
  os_eprintf("Fatal system error at 0x%x\n",
    * (integer *) ((char *) &localvar[0] + PCOFFSET));
#endif (PCOFFSET == 0)
  os_abort();
}
