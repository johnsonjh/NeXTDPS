/*
  dict.h

Copyright (c) 1983, '87, '88 Adobe Systems Incorporated.
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
Ivor Durham: Sun Aug 14 10:25:20 1988
Ed Taft: Sat Dec 16 15:00:27 1989
Joe Pasqua: Fri Jan  6 14:46:53 1989
End Edit History.
*/

#ifndef	DICT_H
#define	DICT_H

#include BASICTYPES
#include LANGUAGE
#include VM

/* Procedures */

_dict	procedure DictInit(/* InitReason reason */);
/* Initialize the Dictionary module */

_dict	procedure DictCtxDestroy();
/* Called when a context is destroyed */

_dict	procedure ClearDictStack();
/* Clear items off of the dictionary stack */

_dict	procedure ClearDictStack(/* InitReason reason */);
/* Initialize the dictionary module */

_dict	procedure ClearDictStk();
/* Pops the dictionary stack down to the permanent elements */

_inline /* boolean	ILoadPNE(PNameEntry, PNameEntry, PObject); */

/* Inline Procedures */

#define ILoadPNE(pne, pVal)\
        ((pne->ts.stamp == timestamp.stamp) ?\
         (VMGetValue(pVal, pne->kvloc), true) : LoadName(pne, pVal))

#define INITSIZE 20 	/* initial dictionary size */
#define	MAXPERMDICTS 10	/* max number of permenant dictionaries */

#endif	DICT_H
