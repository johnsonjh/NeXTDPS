/*
  DEFSsun.h

Copyright (c) 1986, 1988 Adobe Systems Incorporated.
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
Ed Taft: Thu May 12 10:02:35 1988
Leo Hourvitz: Mon June 27 1988 ANSI
End Edit History.

*/
  
#ifndef C_LANGUAGE
#define C_LANGUAGE 0
#endif
#include ENVIRONMENT


#ifdef PROF
	.globl  mcount
#define MCOUNT		 lea 277$,a0;\
		 .data; 277$: .long 0; .text;\
			 jsr mcount
#ifdef __GNU__ 
/* ANSI C preprocessors treat the # character specially. */
#define LINK		link  a6,\#0
#define RTMCOUNT	moveml	\#0xC0C0,sp@-; MCOUNT; moveml sp@+,\#0x0303
#define RET	unlk a6; rts
#define RETN(n)	unlk a6; movl sp@,sp@(n); addql \#n,sp; rts;
#define PARAMX( n )	a6@(8+n)
#else __GNU__ 
#define LINK		link  a6,#0
#define RTMCOUNT	moveml	#0xC0C0,sp@-; MCOUNT; moveml sp@+,#0x0303
#define RET	unlk a6; rts
#define RETN(n)	unlk a6; movl sp@,sp@(n); addql #n,sp; rts;
#define PARAMX( n )	a6@(8+n)
#endif __GNU__ 

#else not PROF

#define MCOUNT
#define RTMCOUNT
#define LINK
#define RET	rts
#ifdef __GNU__ 
#define RETN(n) movl sp@,sp@(n); addql \#n,sp; rts;
#else __GNU__ 
#define RETN(n) movl sp@,sp@(n); addql #n,sp; rts;
#endif __GNU__ 
#define PARAMX( n )	sp@(4+n)

#endif not PROF

#define	ENTRY(x)	.globl CAT(_,x); CAT(_,x): LINK; MCOUNT
#define	RTENTRY(x)	.globl x; x: LINK; RTMCOUNT
#define PARAM0	PARAMX(-4)
#define PARAM	PARAMX(0)
#define PARAM2	PARAMX(4)
#define PARAM3	PARAMX(8)
