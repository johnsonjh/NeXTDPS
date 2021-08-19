/*
  maxmin.c

Copyright (c) 1985, 1988 Adobe Systems Incorporated.
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
Ed Taft: Sun May  1 13:46:41 1988
End Edit History.

C-language prototype for long integer max and min functions.
Actual implementations will likely be in assembly language.
*/

long int os_max(a, b) long int a, b; {return (a > b)? a : b;}
long int os_min(a, b) long int a, b; {return (a < b)? a : b;}
