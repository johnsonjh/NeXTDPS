/*
  whttrp.s
  Based on -S output of whitetrap.c

Copyright (c) 1989 Adobe Systems Incorporated.
All rights reserved.

NOTICE:	 All information  contained herein is  the  property of Adobe Systems
Incorporated.   Many  of the intellectual   and technical concepts  contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available  only to Adobe licensees for  their internal use.  Any reproduction
or dissemination of this software is strictly forbidden  unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Scott Byer: Wed Apr 26 08:39:52 1989
Edit History:
Scott Byer: Tue May 23 14:18:23 1989
End Edit History.
 
*/

#ifndef PERFMONITOR
#define PERFMONITOR 0
#endif	PERFMONITOR

#define SR01	0x3f3c
#define RR01	0x3cfc
#define FIXEDBITS	16	/* Number of bits of fraction in a fixed.  */
#define SODEVTRAP 	28	/* sizeof(DevTrap)			   */
#define ONEASM	65536		/* A Fixed point 1.			   */
#define SOSCANTYPE	4	/* Size of SCANTYPE in bytes.		   */
#define SCANMASK	31
#define	SCANSHIFT	5

#ifdef	__FILE__
#if 	STAGE==DEVELOP
.stabs 	__FILE__,100,0,0,Ltext
Ltext:
.stabs 	"WhiteTrapsMark:F15",36,0,0,_WhiteTrapsMark
#endif	STAGE==DEVELOP
#endif	__FILE__

|
|	public procedure WhiteTrapsMark(t, items, args)
|	  DevTrap *t;
|	  integer items;
|	  MarkArgs *args;
|
/*
	Global variable register assignments.

G	d2	xl		long int	Assigned every nextTrap
G	d4	xr		long int	Assigned every nextTrap
G	d6	lines		DevShort	Assigned every nextTrap
G	a1	leftSlope	boolean		Assigned every nextTrap
G	a2	destbase	PSCANTYPE	Assigned every nextTrap
G	a5	rightSlope	long int	Assigned every nextTrap

	Variable assignments.

A    a6@(-4)	t		DevTrap *
A	(-8)	items		int
G	(-12)	lx		Fixed
G	(-16)	ldx		Fixed
G	(-20)	rdx		Fixed
G	(-24)	xoff		Fixed
G	(-28)	wsu		Fixed
G	(-30)	xoffset		DevShort
G	(-32)	yoffset		DevShort
G	(-36)	xlt		long int
G	(-40)	xrt		long int
opc	(-44)	dx		Fixed
gt	(-48)	rba		PSCANTYPE
G	(-52)	rx		Fixed
opc	(-56)	ix		Fixed
opc	(-60)	maskl		SCANTYPE
gp	(-64)	maskr		SCANTYPE
gt	(-68)	maskr		SCANTYPE
G	(-72)	fl2BD		long int
opc	(-76)	fbw		int
gp	(-80)	rba		PSCANTYPE
		
*/

.text
	.even
.globl _WhiteTrapsMark
_WhiteTrapsMark:
	link	a6,#-80
	moveml	#SR01,sp@-

| Performance monitoring section.
#if	PERFMONITOR
.data
	.even
LP0:
	.long 0
.text
	lea	LP0,a0
	jsr	mcount
#endif	PERFMONITOR

	movel	a6@(8),a6@(-4)	| Make local copies of the arguments.
	movel	a6@(12),a6@(-8)
	movel	a6@(16),a4	|  DevMarkInfo *info = args->markInfo;
	movel	a4@,a0
	clrl	d7		|  long int fl2BD = framelog2BD;
	moveb	_framelog2BD,d7
	movel	d7,a6@(-72)		
	moveq	#SCANSHIFT,d0	|  wsu = ONE << (SCANSHIFT-fl2BD);
	subl	a6@(-72),d0
	movel	#ONEASM,d7
	asll	d0,d7
	movel	d7,a6@(-28)
	clrl	d7		|  xoffset = info->offset.x;
	movew	a0@(2),d7	
	movew	d7,a6@(-30)	
	movew	a0@(6),a6@(-32)	|  yoffset = info->offset.y;
	moveq	#FIXEDBITS,d5	|  xoff = Fix(xoffset);
	asll	d5,d7
	movel	d7,a6@(-24)
L2:				| nextTrap:
	subql	#1,a6@(-8)	|    if (--items < 0)
	jlt	L1		|      return;
	movel	a6@(-4),a3	|    register DevTrap *tt = t++;
	addl	#SODEVTRAP,a6@(-4)
	movel	a6@(-24),d1	|    register Fixed w, xof = xoff;
	movew	a6@(-30),a1	|    register int y, xo = xoffset;
	movew	a3@,a0		|    y = tt->y.l;
	movew	a3@(2),d6	|    lines = tt->y.g - y;
	subw	a0,d6
	jle	L2		|    if (lines <= 0) goto nextTrap;
	addw	a6@(-32),a0	|    y += yoffset;
	movel	a0,d0		|    destbase = (PSCANTYPE) ((integer)
	mulsl	_framebytewidth,d0 |	 framebase + y * framebytewidth);
	movel	d0,a2
	addl	_framebase,a2
	movew	a3@(4),d2	|    xl = tt->l.xl + xo;
	extl	d2
	addl	a1,d2
	movel	a6@(-72),d7	|    xl <<= fl2BD;
	asll	d7,d2
	movew	a3@(16),d4	|    xr = tt->g.xl + xo;
	extl	d4
	addl	a1,d4
	asll	d7,d4		|    xr <<= fl2BD;
	cmpw	#1,d6		|    if (lines == 1)
	jeq	L6		|      goto rect;
	movew	a3@(6),d0	|    xlt = tt->l.xg + xo;
	extl	d0
	addl	a1,d0
	asll	d7,d0		|    xlt <<= fl2BD;
	movel	d0,a6@(-36)
	movew	a3@(18),d5	|    xrt = tt->g.xg + xo;
	extl	d5
	addl	a1,d5
	asll	d7,d5		|    xrt <<= fl2BD;
	movel	d5,a6@(-40)
	cmpl	d4,d5		|    rightSlope = (xr != xrt);
	sne	d7
	andl	#1,d7
	movel	d7,a5
	cmpl	d2,d0		|    leftSlope = (xl != xlt);
	sne	d7
	andl	#1,d7
	movel	d7,a1
	jne	L7		|    if (!leftSlope) {		Left vertical
	tstl	a5		|	if (!rightSlope)	+ Right vert
	jeq	L6		|		goto rect;	+ || shaped.
	movel	d1,d7		|	else {  		+ Right sloped
	addl	a3@(20),d7	|		rx = tt->g.ix + xof;
	movel	d7,a6@(-52)	|				+ |\ shaped.
	movel	a3@(24),a6@(-20) |		rdx = tt->g.dx;
	jra	LgenLeftV	|		goto genLeftV; }
L7:				|    } else {			Left sloped
	movel	d1,d7		|	lx = tt->l.ix + xof;	
	addl	a3@(8),d7
	movel	d7,a6@(-12)
	movel	a3@(12),a6@(-16) |      ldx = tt->l.dx;
	tstl	a5		|	if (!rightSlope)	+ Right vert
	jeq	LgenRightV	|		goto genRightV	+ /| shaped.
	addl	a3@(20),d1	|	else {			+ Right sloped
	movel	d1,a6@(-52)	|		rx = tt->g.ix + xof;
	movel	a3@(24),a6@(-20) |		rdx = tt->g.dx; } }
				| Check for things that require general case.
	subl	a6@(-12),d1	|    if (((w = rx - lx) > wsu)
	cmpl	a6@(-28),d1	|		Must use whole scanunit loop
	jgt	LgenBoth
	cmpw	#3,d6		|	||(lines <= 3)
	jle	LgenBoth
	cmpl	#ONEASM,d1	|	||(w < ONE))
	jlt	LgenBoth	|      goto genTrap;
	jne	L13		|    if (w == ONE 
	movew	a3@(4),a0	|	&& tt->l.xl + 1 == tt->g.xl 
	addqw	#1,a0
	movew	a3@(16),a1
	cmpl	a0,a1
	jne	L13
	movew	a3@(6),a0	|	&& tt->l.xg + 1 == tt->g.xg)
	addqw	#1,a0
	movew	a3@(18),a1
	cmpl	a0,a1
	jeq	L14		|      goto onePerRow;
L13:
	movel	a6@(-16),d0	|    if (ldx < 0 && w == -ldx &&
	jge	L15		
	negl	d0
	cmpl	d1,d0
	jne	L15
	movew	a3@(4),a0	|	tt->l.xl == FTrunc(tt->g.ix) &&
	movel	a3@(20),d0
	moveq	#FIXEDBITS,d7
	asrl	d7,d0
	cmpl	a0,d0
	jne	L15
	movew	a3@(18),a0	|	tt->g.xg == FTrunc(tt->l.ix +
	movew	d6,a1		| 		(lines-3)*(tt->l.dx)))
	movel	a1,d0
	subql	#3,d0
	mulsl	a3@(12),d0
	addl	a3@(8),d0
	asrl	d7,d0
	cmpl	a0,d0
	jeq	L16		|	 goto onePerCol;
L15:
	tstl	a6@(-16)	|    if (ldx > 0 && w == ldx &&
	jle	LgenBoth
	cmpl	a6@(-16),d1
	jne	LgenBoth
	movew	a3@(16),a0	|	tt->g.xl == FTrunc(tt->l.ix) &&
	movel	a3@(8),d0
	moveq	#FIXEDBITS,d7
	asrl	d7,d0
	cmpl	a0,d0
	jne	LgenBoth
	movew	a3@(6),a0	|	tt->l.xg == FTrunc(tt->g.ix +
	movew	d6,a1		|	 (lines-3)*(tt->g.dx))) 
	movel	a1,d0
	subql	#3,d0
	mulsl	a3@(24),d0
	addl	a3@(20),d0
	asrl	d7,d0
	cmpl	a0,d0
	jeq	L16		|	goto onePerCol;
	jra	LgenBoth		|    goto genPara;
L6:				|rect:
	moveq	#SCANMASK,d0	|    maskl = leftBitArray[xl & SCANMASK];
	andl	d2,d0
	lea	_leftBitArray,a0
	movel	a0@(d0:l:4),d1
	moveq	#SCANMASK,d0	|    maskr = rightBitArray[xr & SCANMASK];
	andl	d4,d0
	lea	_rightBitArray,a0
	movel	a0@(d0:l:4),d0
	asrl	#SCANSHIFT,d2	|    xl = xl >> SCANSHIFT;
	lea	a2@(d2:l:4),a2	|    destbase += xl;
	asrl	#SCANSHIFT,d4	|    xr = (xr >> SCANSHIFT) - xl;
	subl	d2,d4
	jne	L19		|    if (xr == 0) {
	andl	d0,d1		|      maskl &= maskr;
L25:
	movel	_framebytewidth,d2 |      xl = framebytewidth;
	movel	d1,d0
	notl	d0
	jra	L20		|      while (--lines >= 0) {
L22:
	andl	d0,a2@		|	*destbase &= ~maskl;
	addl	d2,a2		|	(integer)destbase += xl;
L20:
	dbra	d6,L22		|      }
	jra	L2
L19:				|    } else if (xr == 1) {
	moveq	#1,d7		
	cmpl	d4,d7
	jne	L24
	tstl	d0		|      if (maskr) {
	jeq	L25
	movel	_framebytewidth,d2 |	xl = framebytewidth - sizeof(SCANTYPE);
	subql	#SOSCANTYPE,d2
	notl	d1
	notl	d0
	jra	L26		|	while (--lines >= 0) {
L28:
	andl	d1,a2@+		|	  *(destbase++) &= ~maskl;
	andl	d0,a2@		|	  *destbase &= ~maskr;
	addl	d2,a2		|	  (integer)destbase += xl;
L26:
	dbra	d6,L28		|	}
	jra	L2		|      }
L24:				|    } else { 
	clrl	d3		|      register allzeros = 0;
	tstl	d0		|      if (maskr) {
	jeq	L34
	notl	d1
	notl	d0
	jra	L35		|	while (--lines >= 0) {
L40:
	andl	d1,a2@		|	  destunit = destbase;
	lea	a2@(4),a0	|	  *(destunit++) &= ~maskl;
	movel	d4,d2		|	  xl = xr;
	subql	#1,d2
	jra	L37		|	  while (--xl) 
L39:
	movel	d3,a0@+		|		*(destunit++) = allzeros;
L37:
	dbra	d2,L39
	andl	d0,a0@		|	  *destunit &= ~maskr;
	addl	_framebytewidth,a2 |	  (integer)destbase += framebytewidth;
L35:
	dbra	d6,L40		|	}
	jra	L2
L34:				|      } else {
	movel	d1,d0		
	notl	d0
	jra	L42		|	 while (--lines >= 0) {
L47:
	andl	d0,a2@		|	  destunit = destbase;
	lea	a2@(4),a0	|	  *(destunit++) &= ~maskl;
	movel	d4,d2		|	  xl = xr;
	subql	#1,d2		
	jra	L44		|	  while (--xl) 
L46:
	movel	d3,a0@+		|		*(destunit++) = allzeros;
L44:
	dbra	d2,L46
	addl	_framebytewidth,a2 |	  (integer)destbase += framebytewidth;
L42:
	dbra	d6,L47		|      }}
	jra	L2		|    }    goto nextTrap;
L14:				|onePerRow:
	movel	a6@(-72),d5	|    fl2BD -> d5
	movel	_deepOnes,a0	|    register PSCANTYPE ob = deepOnes[fl2BD];
	movel	a0@(d5:l:4),a0
	movel	_framebytewidth,a5 |    register integer fbw = framebytewidth;
	movel	a6@(-12),d4	|    register Fixed lxx = lx, ldxx = ldx;
	movel	a6@(-16),a1
	moveq	#FIXEDBITS,d7
	moveq	#SCANMASK,d1
	moveq	#SCANSHIFT,d3
	subqw	#2,d6		|    lines -= 2;
	jra	L109		|    while (lines-- > 0) {
L50:
	movel	d4,d2		|      xl = lxx;
	asrl	d7,d2		|      xl >>= FIXEDBITS;
	asll	d5,d2		|      xl <<= fl2BD;
	addl	a1,d4		|      lxx += ldxx;
	addl	a5,a2		|      (integer)destbase += fbw;
L109:
	movel	d2,d0		|      *(destbase + (xl >> SCANSHIFT)) 
	asrl	d3,d0		|	&= ~ob[xl & SCANMASK];
	andl	d1,d2
	movel	a0@(d2:l:4),d2
	notl	d2
	andl	d2,a2@(d0:l:4)
	dbra	d6,L50		|    }
	addl	a5,a2		|    (integer)destbase += fbw;
	movel	a6@(-36),d4	|    xr = xlt;
	movel	d4,d0		|    *(destbase + (xr >> SCANSHIFT)) 
	asrl	d3,d0		|	&= ~ob[xr & SCANMASK];
	andl	d4,d1
	movel	a0@(d1:l:4),d1
	notl	d1
	andl	d1,a2@(d0:l:4)
	jra	L2		|    goto nextTrap;
L16:				|onePerCol:
	movel	_framebytewidth,a4 | register integer fbw=framebytewidth
	moveq	#SCANSHIFT,d7
	movel	a6@(-16),a5	|    register Fixed ix, dx = ldx;
	movel	a5,d0
	jge	L51		|    if (dx < 0) {
	movel	a6@(-12),a0	|      ix = lx;
	moveq	#SCANMASK,d3	|      maskr = rightBitArray[xr & SCANMASK];
	andl	d4,d3
	lea	_rightBitArray,a3
	movel	a3@(d3:l:4),d3
	lea	_leftBitArray,a3 |      ba = leftBitArray;
	asrl	d7,d4		|      xr >>= SCANSHIFT;
	subqw	#2,d6		|      if ( (lines-=2) < 0 ) goto opcOne;
	jmi	L53
	movel	a6@(-72),d0	| Prepare for loop.
	moveq	#FIXEDBITS,d1	| Prepare for loop.
	jra	L54		|      goto opcTwo;
L60:				|      while ( (--lines) >= 0 ) {
	movel	a0,d2		|	xl = IntPart(ix) << fl2BD;
	asrl	d1,d2
	asll	d0,d2
	addl	a5,a0		|	ix += dx;
L54:				|      opcTwo:
	moveq	#SCANMASK,d5	|	maskl = ba[xl & SCANMASK];
	andl	d2,d5
	movel	a3@(d5:l:4),d5
	asrl	d7,d2		|	unit = xl >> SCANSHIFT;
	lea	a2@(d2:l:4),a1	|	destunit = destbase + unit;
	cmpl	d4,d2		|	if (xr == unit) {
	jne	L57
	andl	d5,d3		|	  *destunit &= ~(maskl & maskr);
	notl	d5
	jra	L110
L57:				|	} else {
	notl	d5		|	  *destunit++ &= ~maskl;
	andl	d5,a1@+
	tstl	d3		|	  if (maskr) *destunit &= ~maskr;
	jeq	L58
L110:
	notl	d3
	andl	d3,a1@
L58:				|	}
	movel	d2,d4		|	xr = unit;
	movel	d5,d3		|	maskr = ~maskl;
	addl	a4,a2		|	(integer)destbase += fbw;
	dbra	d6,L60		|      }
	movel	a6@(-36),d2	|      xl = xlt;
L53:				|    opcOne:
	moveq	#SCANMASK,d5	|      maskl = ba[xl & SCANMASK];
	andl	d2,d5
	movel	a3@(d5:l:4),d5
	asrl	d7,d2		|      unit = xl >> SCANSHIFT;
	jra	L111
L51:				|    } else {
	movel	a6@(-52),a0	|      ix = rx;
	moveq	#SCANMASK,d5	|      maskl = leftBitArray[xl & SCANMASK];
	andl	d2,d5
	lea	_leftBitArray,a3
	movel	a3@(d5:l:4),d5
	lea	_rightBitArray,a3 |      ba = rightBitArray;
	asrl	d7,d2		|      xl >>= SCANSHIFT;
	subqw	#2,d6		|      if ( (lines-=2) < 0 ) goto opcThree;
	jmi	L66
	movel	a6@(-72),d0	| Prepare for loop.
	moveq	#FIXEDBITS,d1	| Prepare for loop.
	jra	L67		|      goto opcFour;
L73:				|      while ( (--lines) >= 0 ) {
	movel	a0,d4		|	xr = IntPart(ix) << fl2BD;
	asrl	d1,d4
	asll	d0,d4
	addl	a5,a0		|	ix += dx;
L67:				|      opcFour:
	moveq	#SCANMASK,d3	|	maskr = ba[xr & SCANMASK];
	andl	d4,d3
	movel	a3@(d3:l:4),d3
	asrl	d7,d4		|	unit = xr >> SCANSHIFT;
	lea	a2@(d2:l:4),a1	|	destunit = destbase + xl;
	cmpl	d2,d4		|	if (xl == unit) {
	jne	L70
	andl	d3,d5		|	  *destunit &= ~(maskl & maskr);
	jra	L113
L70:				|	} else {
	notl	d5		|	  *destunit++ &= ~maskl;
	andl	d5,a1@+
	tstl	d3		|	  if (maskr) *destunit &= ~maskr;
	jeq	L71
	movel	d3,d5
L113:
	notl	d5
	andl	d5,a1@
L71:				|	}
	movel	d4,d2		|	xl = unit;
	movel	d3,d5		|	maskl = ~maskr;
	notl	d5	
	addl	a4,a2		|	(integer)destbase += fbw;
	dbra	d6,L73		|      }
	movel	a6@(-40),d4	|      xr = xrt;
L66:				|    opcThree:
	moveq	#SCANMASK,d3	|      maskr = ba[xr & SCANMASK];
	andl	d4,d3
	movel	a3@(d3:l:4),d3
	asrl	d7,d4		|      unit = xr >> SCANSHIFT;
L111:
	lea	a2@(d2:l:4),a1	|      destunit = destbase + xl;
	cmpl	d2,d4		|      if (xl == unit) {
	jne	L74
	andl	d5,d3		|	*destunit &= ~(maskl & maskr);
	notl	d3
	andl	d3,a1@
	jra	L2
L74:				|      } else {
	notl	d5		|	*destunit++ &= ~maskl;
	andl	d5,a1@+
	tstl	d3		|	if (maskr) *destunit &= ~maskr;
	jeq	L2
	notl	d3
	andl	d3,a1@		|      }
	jra	L2		|      goto nextTrap;

	/* Note: units,  when figured out, must   be  representable in a word
	   quantity as  follows: xl and  xr  both  come from word  quantities
	   (tt->l.xl & tt->g.xl) which are offset  and then shifted by fl2BD.
	   Then,  they  are shifted back  and units  becomes   the difference
	   between  them, negating the offset.  The  amount  they are shifted
	   back  by (SCANSHIFT) is  necessarily greater than  or equal to the
	   log base 2 of the number of bits per pixel,  since a SCANUNIT must
	   at a minimum contain one pixel.  Thus dbra  can be  used with what
	   is now a word quantity.					   */

LgenRightV:	/* The right side of the trap is  vertical.   This means that
		   register   d4, containing xr,   is sacred and  must not be
		   changed during  the loop.  This  also means  the the right
		   side  mask, maskr, does not change  and can be set once at
		   the beginning of the loop.				   */
	lea	_leftBitArray,a3 |    register PSCANTYPE lba = leftBitArray,
	lea	_rightBitArray,a4 | rba = rightBitArray;

	subqw	#2,d6		|    if ( (lines-=2) < 0 ) goto gtOne;
	jmi	LastLine
	moveq	#SCANMASK,d5	|    maskr = rba[xr & SCANMASK];
	andl	d4,d5
	movel	a4@(d5:l:4),d5
	asrl	#SCANSHIFT,d4	|    xr >>= SCANSHIFT;
	tstl	d6		| Only need to load regs if more than 3 lines.
	jeq	L80		
	movel	a6@(-72),d7	| fl2BD -> d7
	movel	a6@(-12),d0	| lx -> d0
	movel	a6@(-16),a1	| ldx -> a1
	jra	L80		|    goto gtTwo;
L81:				|    while ( (--lines) >= 0 ) {
	moveq	#FIXEDBITS,d3
	movel	d0,d2		|	xl = IntPart(lx) << fl2BD;
	asrl	d3,d2
	asll	d7,d2
	addl	a1,d0		|	lx += ldx;
L80:				|    gtTwo:
	moveq	#SCANMASK,d3	|      maskl = lba[xl & SCANMASK];
	andl	d2,d3
	movel	a3@(d3:l:4),d3
	asrl	#SCANSHIFT,d2	|      xl >>= SCANSHIFT;
	lea	a2@(d2:l:4),a0	|      destunit = destbase + xl;
	movel	d4,d1		|      units = xr - xl;
	subl	d2,d1
	jgt	L86		|      if (units <= 0) {
	andl	d5,d3		|	*destunit &= ~(maskl & maskr);
	jra	L83
L86:				|      } else {
	notl	d3		|	*(destunit++) &= ~maskl;
	andl	d3,a0@+		
	clrl	d3		|	maskl = 0;
	subql	#1,d1
	jra	L84		|	while (--units > 0) {
L85:
	movel	d3,a0@+		|	  *(destunit++) = maskl;
L84:
	dbra	d1,L85		|	}
	tstl	d5		|	if (maskr) *destunit &= ~maskr;
	jeq	L82
	movel	d5,d3
L83:
	notl	d3
	andl	d3,a0@
L82:				|      }
	addl	_framebytewidth,a2 |      (integer)destbase +=  framebytewidth;
	dbra	d6,L81		|    }
	jra	LoopDone

LgenLeftV:	/* The   left side of  the  trap  is vertical.   Register d2,
		   containing xl, cannot  be touched.  Nor can d3,  contining
		   maskl.						   */
	lea	_leftBitArray,a3 |    register PSCANTYPE lba = leftBitArray,
	lea	_rightBitArray,a4 | rba = rightBitArray;

	subqw	#2,d6		|    if ( (lines-=2) < 0 ) goto gtOne;
	jmi	LastLine
	moveq	#SCANMASK,d3	|    maskl = lba[xl & SCANMASK];
	andl	d2,d3
	movel	a3@(d3:l:4),d3
	asrl	#SCANSHIFT,d2	|    xl >>= SCANSHIFT;
	tstl	d6		| Only need to load regs if more than 3 lines.
	jeq	L87		
	movel	a6@(-72),d7	| fl2BD -> d7
	movel	a6@(-52),d1	| rx -> d1
	movel	a6@(-20),a5	| rdx -> a5
	jra	L87		|    goto gtTwo;
L88:				|    while ( (--lines) >= 0 ) {
	moveq	#FIXEDBITS,d5
	movel	d1,d4		|	xr = IntPart(rx) << fl2BD;
	asrl	d5,d4
	asll	d7,d4
	addl	a5,d1		|	rx += rdx;
L87:				|    gtTwo:
	moveq	#SCANMASK,d5	|      maskr = rba[xr & SCANMASK];
	andl	d4,d5
	movel	a4@(d5:l:4),d5
	lea	a2@(d2:l:4),a0	|      destunit = destbase + xl;
	asrl	#SCANSHIFT,d4	|      xr >>= SCANSHIFT;
	subl	d2,d4		|      xr -= xl;
	jgt	L94		|      if (xr <= 0) {
	andl	d3,d5		|	*destunit &= ~(maskl & maskr);
	jra	L90
L94:				|      } else {
	movel	d3,d0
	notl	d0		|	*(destunit++) &= ~maskl;
	andl	d0,a0@+		
	clrl	d0		|	maskl = 0;
	subql	#1,d4
	jra	L92		|	while (--xr > 0) {
L93:
	movel	d0,a0@+		|	  *(destunit++) = maskl;
L92:
	dbra	d4,L93		|	}
	tstl	d5		|	if (maskr) *destunit &= ~maskr;
	jeq	L89
L90:
	notl	d5
	andl	d5,a0@
L89:				|      }
	addl	_framebytewidth,a2 |      (integer)destbase +=  framebytewidth;
	dbra	d6,L88		|    }
	jra	LoopDone

LgenBoth:	/* Both edges of  the  trap are sloped.   So go ahead and use
		   the registers to their best.				   */
	lea	_leftBitArray,a3 |    register PSCANTYPE lba = leftBitArray,
	lea	_rightBitArray,a4 | rba = rightBitArray;

	subqw	#2,d6		|    if ( (lines-=2) < 0 ) goto gtOne;
	jmi	LastLine
	jeq	L91		| Only need to load regs if more than 3 lines.
	movel	a6@(-72),d7	| fl2BD -> d7
	movel	a6@(-12),d0	| lx -> d0
	movel	a6@(-52),d1	| rx -> d1
	movel	a6@(-16),a1	| ldx -> a1
	movel	a6@(-20),a5	| rdx -> a5
	jra	L91		|    goto gtTwo;
L102:				|    while ( (--lines) >= 0 ) {
	moveq	#FIXEDBITS,d5
	movel	d0,d2		|	xl = IntPart(lx) << fl2BD;
	asrl	d5,d2
	asll	d7,d2
	addl	a1,d0		|	lx += ldx;
	movel	d1,d4		|	xr = IntPart(rx) << fl2BD;
	asrl	d5,d4
	asll	d7,d4
	addl	a5,d1		|	rx += rdx;
L91:				|    gtTwo:
	moveq	#SCANMASK,d3	|      maskl = lba[xl & SCANMASK];
	andl	d2,d3
	movel	a3@(d3:l:4),d3
	moveq	#SCANMASK,d5	|      maskr = rba[xr & SCANMASK];
	andl	d4,d5
	movel	a4@(d5:l:4),d5
	asrl	#SCANSHIFT,d2	|      unitl = xl >> SCANSHIFT;
	lea	a2@(d2:l:4),a0	|      destunit = destbase + unitl;
	asrl	#SCANSHIFT,d4	|      units = (xr >> SCANSHIFT) - unitl;
	subl	d2,d4
	jgt	L96		|      if (units <= 0) {
	andl	d3,d5		|	*destunit &= ~(maskl & maskr);
	jra	L116
L96:				|      } else {
	notl	d3		|	*(destunit++) &= ~maskl;
	andl	d3,a0@+		
	clrl	d3		|	maskl = 0;
	subql	#1,d4
	jra	L98		|	while (--units > 0) {
L100:
	movel	d3,a0@+		|	  *(destunit++) = maskl;
L98:
	dbra	d4,L100		|	}
	tstl	d5		|	if (maskr) *destunit &= ~maskr;
	jeq	L97
L116:
	notl	d5
	andl	d5,a0@
L97:				|      }
	addl	_framebytewidth,a2 |      (integer)destbase +=  framebytewidth;
	dbra	d6,L102		|    }

LoopDone:
	movel	a6@(-36),d2	|    xl = xlt;
	movel	a6@(-40),d4	|    xr = xrt;
LastLine:
	moveq	#SCANMASK,d3	|    maskl = lba[xl & SCANMASK];
	andl	d2,d3
	movel	a3@(d3:l:4),d3
	moveq	#SCANMASK,d5	|    maskr = rba[xr & SCANMASK];
	andl	d4,d5
	movel	a4@(d5:l:4),d5
	asrl	#SCANSHIFT,d2	|    unitl = xl >> SCANSHIFT;
	lea	a2@(d2:l:4),a0	|    destunit = destbase + unitl;
	asrl	#SCANSHIFT,d4	|    units = (xr >> SCANSHIFT) - unitl;
	subl	d2,d4
	jgt	L103		|    if (units <= 0) {
	andl	d3,d5		|      *destunit &= ~(maskl & maskr);
	notl	d5
	andl	d5,a0@		|    }
	jra	L2		|    goto nextTrap;
	jra	L115
L103:				|    } else {
	notl	d3		|      *(destunit++) &= ~maskl;
	andl	d3,a0@+	
	clrl	d3		|      maskl = 0;
	subql	#1,d4
	jra	L105		|      while (--units > 0) {
L107:
	movel	d3,a0@+		|	*(destunit++) = maskl;
L105:
	dbra	d4,L107
	tstl	d5		|      if (maskr) *destunit &= ~maskr;
	jeq	L2
L115:
	notl	d5
	andl	d5,a0@		|    }
	jra	L2		|    goto nextTrap;
L1:
	moveml	a6@(-120),#RR01
	unlk	a6
	rts
