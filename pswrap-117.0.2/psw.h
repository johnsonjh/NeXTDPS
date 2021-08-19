/*
  psw.h

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: 
Paul Rovner: Thursday, May 12, 1988 1:30:12 PM
Edit History:
Andrew Shore: Fri Jul  1 10:25:25 1988
Paul Rovner: Friday, May 20, 1988 3:46:44 PM
End Edit History.
*/

#ifndef	PSW_H
#define	PSW_H

extern char *currentPSWName; /* valid between DEFINEPS and ENDPS */

/* C types */

#define T_BOOLEAN	   101
#define T_FLOAT		   102
#define T_DOUBLE	   103
#define T_CHAR		   104
#define T_UCHAR		   105
#define T_INT		   106
#define T_UINT		   107
#define T_LONGINT	   108
#define T_SHORTINT	   109
#define T_ULONGINT	   110
#define T_USHORTINT	   111
#define T_USEROBJECT   112
#define T_NUMSTR       113
#define T_FLOATNUMSTR  114
#define T_LONGNUMSTR   115
#define T_SHORTNUMSTR  116


/* PostScript types */

#define T_STRING       91
#define T_HEXSTRING    92
#define T_NAME         93
#define T_LITNAME      94
#define T_ARRAY        95
#define T_PROC         96
#define T_CONTEXT      97
#define T_SUBSCRIPTED  98

/* Other PostScript types:

   T_FLOAT is used for real
   T_INT is used for integer
   T_BOOLEAN is used for boolean
   T_USEROBJECT is used for userobjects

*/

#endif	PSW_H
