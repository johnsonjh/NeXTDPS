/*
  blktrp.s
  derived from optimized -S output of blacktrap.c

Copyright (c) 1989 Adobe Systems Incorporated.
All rights reserved.

NOTICE: All information	 contained herein is  the  property of Adobe  Systems
Incorporated.  Many of	the  intellectual  and technical  concepts  contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to  Adobe licensees  for their internal use.  Any reproduction
or dissemination of this software is strictly  forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Scott Byer: Tue Apr  4 10:43:21 1989
Edit History:
Scott Byer: Thu Jun  1 16:50:25 1989
End Edit History.

*/

#ifndef PERFMONITOR
#define PERFMONITOR 0
#endif	PERFMONITOR

#define SR01		0x3f3c
#define RR01		0x3cfc
#define FIXEDBITS	16
#define SODEVTRAP	28	/* sizeof(DevTrap)			   */
#define ONEASM		65536	/* A Fixed point 1.			   */
#define SOSCANTYPE	4	/* Size of SCANTYPE in bytes.		   */
#define SCANMASK	31
#define	SCANSHIFT	5

#ifdef	__FILE__
#if 	STAGE==DEVELOP
.stabs 	__FILE__,100,0,0,Ltext
Ltext:
.stabs 	"BlackTrapsMark:F15",36,0,0,_BlackTrapsMark
#endif	STAGE==DEVELOP
#endif	__FILE__

|
|	public procedure BlackTrapsMark(t, items, args)
|	  DevTrap *t;
|	  integer items;
|	  MarkArgs *args;
|
/*
	Variable Assignments.

	     a1 = rightSlope
	     a2 = destbase
	     a5 = leftSlope
	     d3 = xl 
	     d5 = xr 
	     d6 = lines 
	 a6@(8) = t 
	   (12) = items 
	   (16) = args 
	   (-4) = ldx 
	   (-8) = rdx 
	  (-12) = xoff 
	  (-16) = wsu 
	  (-18) = xoffset short
	  (-20) = yoffset short
	  (-24) = xrt	  
	  (-28) = _framelog2BD holder 
	  (-32) = lx 
	  (-36) = rx 
	  (-40) = xlt	  
	  (-44) = t
	  (-48) = items

*/

.text
	.even
.globl _BlackTrapsMark
_BlackTrapsMark:
	link 	a6,#-48
	moveml 	#SR01,sp@-

| Performance monitoring section.
#if PERFMONITOR
.data
	.even
LP0:
	.long 0
.text
	lea	LP0, a0
	jsr	mcount
#endif PERFMONITOR

	movel	a6@(8),a6@(-44)
	movel	a6@(12),a6@(-48)
	movel	a6@(16),a0	| DevMarkInfo *info = args->markInfo; 
	movel	a0@,a0		
	clrl	d7		| Do this once, to avoid doing it five times.
	moveb	_framelog2BD,d7
	movel	d7,a6@(-28)
	moveq	#SCANSHIFT,d0	| wsu = ONE << (SCANSHIFT - fl2BD)
	subl	d7,d0
	movel	#ONEASM,d7
	asll	d0,d7
	movel	d7,a6@(-16)
	movel	a0@+,a4		| xoffset = info->offset.x; 
	movew	a4,a6@(-18)
	movew	a0@(2),a6@(-20)	| yoffset = info->offset.y; 
	movew	a4,d7		| xoff = Fix(xoffset); 
	moveq	#FIXEDBITS,d4
	asll	d4,d7
	movel	d7,a6@(-12)
L2:				| nextTrap: 
	subql	#1,a6@(-48)	| if (--items < 0) 
	jlt	L1		| return; 
	movel	a6@(-44),a3	| register DevTrap *tt = t++; 
	addl	#SODEVTRAP,a6@(-44)
	movew	a6@(-18),a1	| register int xo = xoffset; 
	clrl	d7		| y = tt->y.l;
	movew	a3@,d7		
	clrl	d6		| lines = tt->y.g - y; 
	movew	a3@(2),d6		
	subw	d7,d6
	jle	L2		| if (lines <= 0) goto nextTrap;
	addw	a6@(-20),d7	| y += yoffset; 
	mulsl	_framebytewidth,d7 | destbase = (PSCANTYPE)((integer)framebase 
	movel	_framebase,a2	|	 + y * framebytewidth); 
	addl	d7,a2
	movew	a3@(4),d3	| xl = tt->l.xl + xo; 
	extl	d3
	addl	a1,d3	
	movel	a6@(-28),d2	| xl <<= framelog2BD; 
	asll	d2,d3
	movew	a3@(16),d5	| xr = tt->g.xl + xo; 
	extl	d5
	addl	a1,d5
	asll	d2,d5		| xr <<= framelog2BD; 
	cmpw	#1,d6		| if (lines == 1) 
	jeq	L6		|   goto rect; 
	movew	a3@(6),d1	| xlt = tt->l.xg + xo; 
	extl	d1
	addl	a1,d1
	asll	d2,d1		| xlt <<= framelog2BD; 
	movel	d1,a6@(-40)	
	movew	a3@(18),d0	| xrt = tt->g.xg + xo; 
	extl	d0
	addl	a1,d0
	asll	d2,d0		| xrt <<= framelog2BD; 
	movel	d0,a6@(-24)
	cmpl	d5,d0		| rightSlope = (xr != xrt); 
	sne	d7
	andl	#1,d7
	movel	d7,a1
	cmpl	d3,d1		| leftSlope = (xl != xlt); 
	sne	d7
	andl	#1,d7
	movel	d7,a5
	jne	L7		| if (!leftSlope) {	    Left side vertical.
	tstl	a1		|	if (!rightslope)    
	jeq	L6		|		goto rect;  || shaped.
	movel	a3@(20),d7	|	|\ shaped, cant be line.
	addl	a6@(-12),d7	|	else { rx = tt->g.ix + xoff; 
	movel	d7,a6@(-36)
	movel	a3@(24),a6@(-8)	|		rdx = tt->g.dx; 
	jra	LgenLeftV	|		goto genLeftV; }
L7:				| } else {		    Left side sloped.
	movel	a3@(8),a4	|	lx = tt->l.ix + xoff; 
	addl	a6@(-12),a4
	movel	a4,a6@(-32)	
	movel	a3@(12),a6@(-4)	|	ldx = tt->l.dx; 
	tstl	a1		|	if (!rightSlope)    /| shaped, no line.
	jeq	LgenRightV	|		goto genRightV;
	movel	a3@(20),d7	|	/\ shaped, could be line.
	addl	a6@(-12),d7	|	else { rx = tt->g.ix + xoff; 
	movel	d7,a6@(-36)
	movel	a3@(24),a6@(-8)	|		rdx = tt->g.dx;	 } }
				| Check for things that require general case.
	cmpw	#3,d6		| if ((lines <= 3)
	jle	LgenBoth	|	Slope may not be valid.
	subl	a4,d7		|    || ((w = rx - lx) > wsu)
	cmpl	a6@(-16),d7
	jgt	LgenBoth	
	cmpl	#ONEASM,d7	|    || (w < ONE))	
	jlt	LgenBoth	|    goto genBoth; 
	jne	L13		| if (w == ONE
	movew	a3@(6),a0	|	&& tt->l.xg + 1 == tt->g.xg) 
	addqw	#1,a0
	movew	a3@(18),a1
	cmpl	a0,a1
	jne	L13
	movew	a3@(4),a0	|	&& tt->l.xl + 1 == tt->g.xl 
	addqw	#1,a0
	movew	a3@(16),a1
	cmpl	a0,a1
	jeq	L14		|   goto onePerRow;
L13:	
	moveq	#FIXEDBITS,d1
	movel	a6@(-4),d0	| if (ldx < 0 
	jge	L15
	negl	d0		|	&& w == -ldx 
	cmpl	d7,d0
	jne	L15				
	movew	a3@(4),a0	|	&& tt->l.xl == FTrunc(tt->g.ix) 
	movel	a3@(20),d0
	asrl	d1,d0
	cmpl	a0,d0
	jne	L15
	movew	a3@(18),a0	|	&& tt->g.xg ==	
	movew	d6,d0		|	FTrunc(tt->l.ix+(lines-3)*(tt->l.dx))
	subqw	#3,d0
	mulsl	a3@(12),d0
	addl	a3@(8),d0
	asrl	d1,d0
	cmpl	a0,d0
	jeq	L16		|	) goto onePerCol; 
L15:
	movel	a6@(-4),d0	| if (ldx > 0 
	jle	LgenBoth
	cmpl	d0,d7		|	&& w == ldx 
	jne	LgenBoth
	movew	a3@(16),a0	|	&& tt->g.xl == FTrunc(tt->l.ix) 
	movel	a3@(8),d0
	asrl	d1,d0
	cmpl	a0,d0
	jne	LgenBoth	|	&& tt->l.xg == 
	movew	a3@(6),a0	|	FTrunc(tt->g.ix+(lines-3)*(tt->g.dx))
	movew	d6,d0
	subqw	#3,d0
	mulsl	a3@(24),d0
	addl	a3@(20),d0
	asrl	d1,d0
	cmpl	a0,d0
	jeq	L16		|	) goto onePerCol; 
	jra	LgenBoth	| goto genBoth;	 ( /\ shaped gen trap )
L6:				| rect: 
	movel	_framebytewidth,d4 | frambytewidth -> d4
	moveq	#SCANMASK,d0	| maskl = leftBitArray[xl & SCANMASK]; 
	andl	d3,d0
	lea	_leftBitArray,a0
	movel	a0@(d0:l:4),d1
	moveq	#SCANMASK,d0	| maskr = rightBitArray[xr & SCANMASK]; 
	andl	d5,d0
	lea	_rightBitArray,a0
	movel	a0@(d0:l:4),d0
	asrl	#SCANSHIFT,d3	| xl = xl >> SCANSHIFT; 
	lea	a2@(d3:l:4),a2	| destbase += xl; 
	asrl	#SCANSHIFT,d5	| xr = (xr >> SCANSHIFT) - xl; 
	subl	d3,d5
	jne	L19			| if (xr == 0) { 
	andl	d0,d1		|	maskl	&= maskr; 
L21:
	movel	d4,d3		|	xl	= framebytewidth; 
	jra	L20			
L22:
	orl	d1,a2@		|	*destbase |= maskl; 
	addl	d3,a2		|	(integer)destbase += xl *} 
L20:
	dbra	d6,L22		|	while (--lines >= 0) {* 
	jra	L2
L19:
	moveq	#1,d7		| } else if (xr == 1) { 
	cmpl	d5,d7
	jne	L24
	tstl	d0		|	if (maskr)
	jeq	L21		|	goto loop without or.
	movel	d4,d3		|	xl	= framebytewidth-sizeof(SCANTYPE);
	subql	#SOSCANTYPE,d3
	jra	L125		|	goto	loop with or.
L128:
	orl	d1,a2@+		|	*(destbase++) |= maskl; 
	orl	d0,a2@		|	*destbase |= maskr; 
	addl	d3,a2		|	(integer)destbase += xl; *} 
L125:
	dbra	d6,L128		|	while (--lines >= 0) {* 
	jra	L2
L24:				| } else { 
	moveq	#-1,d2		|	allones = -1; 
	tstl	d0		|	if (maskr)
	jeq	L30		| goto loop without or.
	jra	L130		| goto loop with or.
L136:
	orl	d1,a2@		|	*(destunit++) |= maskl; 
	lea	a2@(4),a0		|	destunit = destbase; 
	movel	d5,d3		|	xl = xr; 
	subql	#1,d3		|	while (--xl) {
	jra	L132
L134:
	movel	d2,a0@+		|		*(destunit++) = allones;
L132:
	dbra	d3,L134		|	}
	orl	d0,a0@		|	*destunit |= maskr; 
	addl	d4,a2		|	(integer)destbase+=framebytewidth;*} 
L130:
	dbra	d6,L136		|	while (--lines >= 0) {* 
	jra	L2			| } goto nextTrap; 
L36:
	orl	d1,a2@		|	*(destunit++) |= maskl; 
	lea	a2@(4),a0		|	destunit = destbase; 
	movel	d5,d3		|	xl = xr; 
	subql	#1,d3		|	while (--xl) {
	jra	L32
L34:
	movel	d2,a0@+		|		*(destunit++) = allones;  
L32:
	dbra	d3,L34		|	}      
	addl	d4,a2		|	(integer)destbase+=framebytewidth;*} 
L30:
	dbra	d6,L36		|	while (--lines >= 0) {* 
	jra	L2		| } goto nextTrap; 
L14:				| onePerRow: 
	movel	a6@(-28),d2	| register integer fl2BD = framelog2BD; 
	movel	_deepOnes,a0	| register PSCANTYPE ob = deepOnes[fl2BD]; 
	movel	a0@(d2:l:4),a3
	movel	_framebytewidth,a5 | register integer fbw = framebytewidth, 
	subqw	#2,d6		| midlines = lines - 2; 
	movel	a6@(-4),d5	| ldxx = ldx; 
	moveq	#FIXEDBITS,d7	| Prepare for xl >>= 16;
	moveq	#SCANSHIFT,d1	
	moveq	#SCANMASK,d4 
	jra	L87		| a4 contains lx from above.
L39:
	movel	a4,d3		| xl = lxx; 
	asrl	d7,d3		| xl >>= 16; 
	asll	d2,d3		| xl <<= fl2BD; 
	addl	d5,a4		| lxx += ldxx; 
	addl	a5,a2		| (integer)destbase += fbw; 
L87:
	movel	d3,d0		| *(destbase + (xl >> SCANSHIFT))  
	asrl	d1,d0		|		|= ob[xl & SCANMASK]; 
	lea	a2@(d0:l:4),a0	| *} 
	andl	d4,d3
	movel	a3@(d3:l:4),d0
	orl	d0,a0@
	dbra	d6,L39		| while (midlines-- > 0) {* 
	addl	a5,a2		| (integer)destbase += fbw; 
	movel	a6@(-40),d5	| xr = xlt; 
	movel	d5,d0		| *(destbase + (xr >> SCANSHIFT))  
	asrl	d1,d0		|		|= ob[xr & SCANMASK]; 
	lea	a2@(d0:l:4),a0
	andl	d4,d5
	movel	a3@(d5:l:4),d7
	orl	d7,a0@
	jra	L2		| goto nextTrap; 
L16:				| onePerCol: 
	moveq	#FIXEDBITS,d7
	lea	_leftBitArray,a3 | register PSCANTYPE ba = leftBitArray; 
	movel	_framebytewidth,a5 | register integer fbw = framebytewidth; 
	movel	a6@(-28),d4	| framelog2BD -> d4
	movel	a6@(-4),d2	| register Fixed dx = ldx; 
	jge	L40		| if (dx < 0) {	 (a4 contains lx)
	moveq	#SCANMASK,d1	|	maskr = rightBitArray[xr & SCANMASK];
	andl	d5,d1
	lea	_rightBitArray,a0
	movel	a0@(d1:l:4),d1
	asrl	#SCANSHIFT,d5	|	xr >>= SCANSHIFT; 
	subqw	#2,d6		|	if ( (--lines) == 0 ) goto nextTrap;
	jlt	L41
	jra	L42		|	while ( --lines > 0 ) {
L48:
	movel	a4,d3		|		xl = IntPart(ix)<<framelog2BD; 
	asrl	d7,d3
	asll	d4,d3
	addl	d2,a4		|		ix += dx;  
L42:
	moveq	#SCANMASK,d0	|		maskl = ba[xl & SCANMASK]; 
	andl	d3,d0
	movel	a3@(d0:l:4),d0
	asrl	#SCANSHIFT,d3	|		unit = xl >> SCANSHIFT; 
	lea	a2@(d3:l:4),a1	|		destunit = destbase + unit; 
	cmpl	d5,d3		|		if (xr == unit) 
	jne	L43
	andl	d0,d1		|		*destunit |= maskl & maskr; 
	orl	d1,a1@
	jra	L49
L43:				|		else	{ 
	orl	d0,a1@+		|		*destunit++ |= maskl; 
	orl	d1,a1@		|		*destunit |= maskr;
L49:				|		}
	movel	d3,d5		|	xr = unit; 
	movel	d0,d1		|	maskr = ~maskl; 
	notl	d1
	addl	a5,a2		|	(integer)destbase += fbw;
	dbra	d6,L48		|	}
	movel	a6@(-40),d3	|	xl = xlt; 
L41:
	moveq	#SCANMASK,d0	|	maskl = ba[xl & SCANMASK]; 
	andl	d3,d0
	movel	a3@(d0:l:4),d0
	asrl	#SCANSHIFT,d3	|	unit = xl >> SCANSHIFT; 
	lea	a2@(d3:l:4),a1	|	destunit = destbase + unit; 
	cmpl	d5,d3		|	if (xr == unit) 
	jne	L44
	andl	d0,d1		|		*destunit |= maskl & maskr; 
	orl	d1,a1@
	jra	L2
L44:				|	else	{ 
	orl	d0,a1@+		|		*destunit++ |= maskl; 
	orl	d1,a1@		|		*destunit |= maskr;
	jra	L2		|	} goto nextTrap;
L40:				| } else { 
	movel	a6@(-36),a4	|	ix = rx; 
	moveq	#SCANMASK,d0	|	maskl = leftBitArray[xl & SCANMASK]; 
	andl	d3,d0
	movel	a3@(d0:l:4),d0
	lea	_rightBitArray,a3 |	ba = rightBitArray; 
	asrl	#SCANSHIFT,d3	|	xl >>= SCANSHIFT; 
	subqw	#2,d6		|	if ( (--lines) == 0 ) goto nextTrap;
	jlt	L51
	jra	L52		|	while ( --lines > 0 ) {
L59:
	movel	a4,d5		|		xr = IntPart(ix)<<framelog2BD; 
	asrl	d7,d5
	asll	d4,d5
	addl	d2,a4		|		ix += dx;  
L52:
	moveq	#SCANMASK,d1	|		maskr = ba[xr & SCANMASK]; 
	andl	d5,d1
	movel	a3@(d1:l:4),d1
	asrl	#SCANSHIFT,d5	|		unit = xr >> SCANSHIFT; 
	lea	a2@(d3:l:4),a1	|		destunit = destbase + xl; 
	cmpl	d3,d5		|		if (xl == unit) 
	jne	L54
	andl	d1,d0		|		*destunit |= maskl & maskr; 
	orl	d0,a1@
	jra	L60
L54:				|		else	{ 
	orl	d0,a1@+		|		*destunit++ |= maskl; 
	orl	d1,a1@		|		*destunit |= maskr;
L60:				|		}
	movel	d5,d3		|	xl = unit; 
	movel	d1,d0		|	maskl = ~maskr; 
	notl	d0		
	addl	a5,a2		|	(integer)destbase += fbw;
	dbra	d6,L59		|	}
	movel	a6@(-24),d5	|	xr = xrt; 
L51:
	moveq	#SCANMASK,d1	|	maskr = ba[xr & SCANMASK];	 
	andl	d5,d1
	movel	a3@(d1:l:4),d1
	asrl	#SCANSHIFT,d5	|	unit = xr >> SCANSHIFT; 
	lea	a2@(d3:l:4),a1	|	destunit = destbase + xl; 
	cmpl	d3,d5		|	if (xl == unit) 
	jne	L55
	andl	d1,d0		|		*destunit |= maskl & maskr; 
	orl	d0,a1@
	jra	L2
L55:				|	else	{ 
	orl	d0,a1@+		|		*destunit++ |= maskl; 
	orl	d1,a1@		|		*destunit |= maskr;
	jra	L2		|	} goto nextTrap;

	/* Note: units,	  when figured out, must be   representable in a word
	   quantity as	follows: xl  and  xr both come	from  word quantities
	   (tt->l.xl & tt->g.xl) which are  offset and then shifted by fl2BD.
	   Then, they	are shifted  back and  units  becomes  the difference
	   between them,  negating  the offset.	 The  amount they are shifted
	   back by (SCANSHIFT) is necessarily  greater	than or equal  to the
	   log base 2 of the number of bits per	 pixel, since a SCANUNIT must
	   at a minimum contain one pixel.  Thus dbra  can be used  with what
	   is now a word quantity.					   */

LgenRightV:	/* The	right side of the trap	is vertical.  This means that
		   register d5, containing xr,	is sacred and  should not get
		   touched.  Also, the right mask, maskr, is always the same,
		   and should also not be touched after first set.	   */

	lea	_rightBitArray,a3 | register PSCANTYPE	rba = rightBitArray; 
	lea	_leftBitArray,a4 |			lba = leftBitArray;

	subqw	#2,d6		| if ( (lines-=2) < 0 ) goto oneLine;
	jmi	LastLine
	moveq	#SCANMASK,d4	| maskr = rba[xr & SCANMASK]; 
	andl	d5,d4		| Set it only once for the loop.
	movel	a3@(d4:l:4),d4
	asrl	#SCANSHIFT,d5	| xr >>= SCANSHIFT;
	tstl	d6		| Only need to load regs if more than 3 lines.
	jeq	L61		
	movel	a6@(-28),d7	| framelog2BD -> d7
	movel	a6@(-32),d0	| lx -> d0
	movel	a6@(-4),a1	| ldx -> a1
	jra	L61	
L62:				| while ( (--lines) > 0 ) {
	movel	d0,d3		|	xl = IntPart(lx)<<framelog2BD; 
	moveq	#FIXEDBITS,d2
	asrl	d2,d3
	asll	d7,d3
	addl	a1,d0		|	lx += ldx;
L61:
	moveq	#SCANMASK,d2	|	maskl = lba[xl & SCANMASK]; 
	andl	d3,d2
	movel	a4@(d2:l:4),d2
	asrl	#SCANSHIFT,d3	|	xl >>= SCANSHIFT; 
	lea	a2@(d3:l:4),a0	|	destunit = destbase + xl; 
	movel	d5,d1		|	units = xr - xl; 
	subl	d3,d1
	jgt	L63		|	if (units == 0) 
	andl	d4,d2		|		*destunit |= maskl & maskr; 
	orl	d2,a0@
	jra	L66
L63:				|	else	{ 
	orl	d2,a0@+		|		*(destunit++) |= maskl; 
	moveq	#-1,d2		|		maskl = -1; 
	subql	#1,d1	
	jra	L65		|		while (--units > 0) {	
L64:
	movel	d2,a0@+		|			*(destunit++) = maskl; 
L65:
	dbra	d1,L64		|		}
	orl	d4,a0@		|		*destunit |= maskr; 
L66:				|	} 
	addl	_framebytewidth,a2 | (integer)destbase += framebytewidth;
	dbra	d6,L62		| }
	jra	LoopDone	|	Do the last line.

LgenLeftV:	/* The	left side of  the   trap  is vertical,	meaning	 that
		   register d3, containing xl, is  sacred and should  not  be
		   touched.  Also, the left mask, maskl, remains the same and
		   should not be touched.				   */

	lea	_rightBitArray,a3 | register PSCANTYPE	rba = rightBitArray; 
	lea	_leftBitArray,a4 |			lba = leftBitArray;

	subqw	#2,d6		| if ( (lines-=2) < 0 ) goto oneLine;
	jmi	LastLine
	moveq	#SCANMASK,d2	| maskl = lba[xl & SCANMASK]; 
	andl	d3,d2
	movel	a4@(d2:l:4),d2
	asrl	#SCANSHIFT,d3	| xl >>= SCANSHIFT;
	tstl	d6		| Only need to load regs if more than 3 lines.
	jeq	L67		
	movel	a6@(-28),d7	| framelog2BD -> d7
	movel	a6@(-36),d1	| rx -> d1
	movel	a6@(-8),a5	| rdx -> a5
	jra	L67	
L68:				| while ( (--lines) > 0 ) {
	moveq	#FIXEDBITS,d4
	movel	d1,d5		|	xr = IntPart(rx)<<framelog2BD; 
	asrl	d4,d5
	asll	d7,d5
	addl	a5,d1		|	rx += rdx; 
L67:
	moveq	#SCANMASK,d4	|	maskr = rba[xr & SCANMASK]; 
	andl	d5,d4
	movel	a3@(d4:l:4),d4
	lea	a2@(d3:l:4),a0	|	destunit = destbase + xl; 
	asrl	#SCANSHIFT,d5	|	xr >>= SCANSHFT;
	subl	d3,d5		|	xr -= xl;
	jgt	L69		|	if (units == 0) 
	andl	d2,d4		|		*destunit |= maskl & maskr; 
	orl	d4,a0@
	jra	L75
L69:				|	else	{ 
	orl	d2,a0@+		|		*(destunit++) |= maskl; 
	moveq	#-1,d0		|		unitl = -1; 
	subql	#1,d5		
	jra	L73		|		while (--units > 0) {	
L70:
	movel	d0,a0@+		|			*(destunit++) = unitl; 
L73:
	dbra	d5,L70		|		}
	orl	d4,a0@		|		*destunit |= maskr; 
L75:				|	} 
	addl	_framebytewidth,a2 | (integer)destbase += framebytewidth;
	dbra	d6,L68		| }
	jra	LoopDone	|	Do the last line.

LgenBoth:	/* Both	 sides of  the	   trap	 are   sloped.	 Could	be  a
		   parallelogram.  This is the most generalized trap code. */

	lea	_rightBitArray,a3 | register PSCANTYPE	rba = rightBitArray; 
	lea	_leftBitArray,a4 |			lba = leftBitArray;

	subqw	#2,d6		| if ( (lines-=2) < 0 ) goto oneLine;
	jmi	LastLine
	jeq	L72		| Only need to load regs if more than 3 lines.
	movel	a6@(-28),d7	| framelog2BD -> d7
	movel	a6@(-32),d0	| lx -> d0
	movel	a6@(-36),d1	| rx -> d1
	movel	a6@(-4),a1	| ldx -> a1
	movel	a6@(-8),a5	| rdx -> a5
	jra	L72	
L82:				| while ( (--lines) > 0 ) {
	moveq	#FIXEDBITS,d2
	movel	d0,d3		|	xl = IntPart(lx)<<framelog2BD; 
	asrl	d2,d3
	asll	d7,d3
	addl	a1,d0		|	lx += ldx;
	movel	d1,d5		|	xr = IntPart(rx)<<framelog2BD; 
	asrl	d2,d5
	asll	d7,d5
	addl	a5,d1		|	rx += rdx; 
L72:
	moveq	#SCANMASK,d2	|	maskl = lba[xl & SCANMASK]; 
	andl	d3,d2
	movel	a4@(d2:l:4),d2
	moveq	#SCANMASK,d4	|	maskr = rba[xr & SCANMASK]; 
	andl	d5,d4
	movel	a3@(d4:l:4),d4
	asrl	#SCANSHIFT,d3	|	unitl = xl >> SCANSHIFT; 
	lea	a2@(d3:l:4),a0	|	destunit = destbase + unitl; 
	asrl	#SCANSHIFT,d5	|	units = (xr >> SCANSHIFT) - unitl; 
	subl	d3,d5
	jgt	L74		|	if (units == 0) 
	andl	d2,d4		|		*destunit |= maskl & maskr; 
	orl	d4,a0@
	jra	L83
L74:				|	else	{ 
	orl	d2,a0@+		|		*(destunit++) |= maskl; 
	moveq	#-1,d2		|		maskl = -1; 
	subql	#1,d5		
	jra	L76		|		while (--units > 0) {	
L78:
	movel	d2,a0@+		|			*(destunit++) = maskl; 
L76:
	dbra	d5,L78		|		}
	orl	d4,a0@		|		*destunit |= maskr; 
L83:				|	} 
	addl	_framebytewidth,a2 | (integer)destbase += framebytewidth;
	dbra	d6,L82		| }

LoopDone:
	movel	a6@(-40),d3	| xl = xlt; 
	movel	a6@(-24),d5	| xr = xrt; 
LastLine:
	moveq	#SCANMASK,d2	| maskl = lba[xl & SCANMASK]; 
	andl	d3,d2
	movel	a4@(d2:l:4),d2
	moveq	#SCANMASK,d4	| maskr = rba[xr & SCANMASK]; 
	andl	d5,d4
	movel	a3@(d4:l:4),d4
	asrl	#SCANSHIFT,d3	| unitl = xl >> SCANSHIFT; 
	lea	a2@(d3:l:4),a0	| destunit = destbase + unitl; 
	asrl	#SCANSHIFT,d5	| units = (xr >> SCANSHIFT) - unitl; 
	subl	d3,d5
	jgt	L94		| if (units == 0) 
	andl	d2,d4		|	*destunit |= maskl & maskr; 
	orl	d4,a0@
	jra	L2
L94:				| else { 
	orl	d2,a0@+		|	*(destunit++) |= maskl; 
	moveq	#-1,d2		|	maskl = -1; 
	subql	#1,d5		|	while (--units > 0) {
	jra	L96			
L98:
	movel	d2,a0@+		|		*(destunit++) = maskl;	
L96:
	dbra	d5,L98		|	}
	orl	d4,a0@		|	*destunit |= maskr; 
	jra	L2		| } goto nextTrap;
L1:
	moveml	a6@(-88),#RR01
	unlk	a6
	rts

