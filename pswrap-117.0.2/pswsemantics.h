/*
  pswsemantics.h

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Paul Rovner: Thursday, May 12, 1988 1:30:12 PM
Edit History:
Andrew Shore: Fri Jul  1 10:25:44 1988
End Edit History.
*/

#ifndef	PSWSEMANTICS_H
#define	PSWSEMANTICS_H

#include "pswpriv.h"

/* PROCEDURES */

extern PSWName(/* char *s */);
extern FinalizePSWrapDef(/* hdr, body */);
extern Header PSWHeader(/* isStatic, inArgs, outArgs */);
extern Token PSWToken(/* type, val */);
extern Token PSWToken2(/* type, val, ind */);
extern Arg PSWArg(/* type, items */);
extern Item PSWItem(/* name */);
extern Item PSWStarItem(/* name */);
extern Item PSWSubscriptItem(/* name, subscript */);
extern Item PSWScaleItem(/* name, subscript */);
extern Subscript PSWNameSubscript(/* name */);
extern Subscript PSWIntegerSubscript(/* val */);
extern Args ConsPSWArgs(/* arg, args */);
extern Tokens AppendPSWToken(/* token, tokens */);
extern Args AppendPSWArgs(/* arg, args */);
extern Items AppendPSWItems(/* item, items */);

#endif	PSWSEMANTICS_H
