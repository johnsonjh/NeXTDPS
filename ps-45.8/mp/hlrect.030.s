|
|
|	highlightrectnext.s
|	Assembly code to highlight a particular rectangle.
|	On the two bit/pixel NeXT screen this is equivalent to
|	swapping white pixels with light gray ones.  This is done
|	by taking the not of the high-order bits of each pixel
|	and exclusive-oring it with the low-order bit of that pixel.
|
|
| Assembler template for LineOperation structure
destPtr	= 0x0			| First word to be changed in dest
numOtherWords = destPtr+4	| Number of integers after first
leftMask = numOtherWords+4	| Mask for first dest word
rightMask = leftMask + 4	| Mask for last dest word
| Rest of a LineOperation is irrelevant for highlightrect
|
|
| Calling Sequence:
|
|	HighlightRect(lineOpPtr,srcRowBytes,destRowBytes,height)|
|
|
lineOpPtr = 0x08		| Pointer to LineOp structure, above
srcRowBytes = lineOpPtr+4	| ignored
destRowBytes = srcRowBytes + 4	| 
height = destRowBytes + 4
|
|
highorderbitmask = 0xaaaaaaaa	| mask to extract high order bits of each pixel
|
	.text
||PROC| 04
_HighlightRect:
	.globl	_HighlightRect
	link	a6,#0
	moveml	a0-a6/d0-d7,sp@-
	movl	a6@(destRowBytes),a5
	movl	a6@(height),d4
	subl	#1,d4
	movl	a6@(lineOpPtr),a6
| Now load all elements from LineOperation structure
	movl	a6@(destPtr),a3
	movl	a6@(numOtherWords),d3	| Save the original value
	movl	a6@(leftMask),d7	| Pick up masks into unused regs
	movl	#highorderbitmask,d5	| d5 <- mask for high-order bits
	movl	a6@(rightMask),d6
	cmpl	#1,d3			| How many words per line?
	beqs	TwoWordHighL		| Exactly two
	blt	OneWordHighL		| Exactly one
	subl	#2,d3			| Pre-subtract for dbra
HighLRowLoop:
	movl	a3,a1			| Put current dest ptr in a1
| Do first word with leftMask
    	movl	a1@,d1			| d1 <- destWord
	movl	d1,d0			| d0 <- destWord
	movl	d1,d2			| d2 <- destWord
	notl	d1			| d1 <- ~destWord
	andl	d5,d1			| d1 <- ~h-o-b-of-destWord
	lsrl	#1,d1			| d1 <- ~h-o-b-of-destWord >> 1
	eorl	d1,d0			| d0 <- Swapped colors in destWord
	andl	d7,d0			| d0 <- swapped colors inside leftMask
	movl	d7,d1			| d1 <- leftMask
	notl	d1			| d1 <- ~leftMask
	andl	d2,d1			| d1 <- destWord & ~leftMask
	orl	d1,d0			| d0 <- combined halves
	movl	d0,a1@+			| *destPtr++ <- combined halves
| Set up for Loop through inner words
    	movl	d3,d2
HighLInnerLoop:
		movl	a1@,d1		| d0 <- destWord
		movl	d1,d0		| d1 <- destWord
		notl	d1		| d1 <- ~destWord
		andl	d5,d1		| d1 <- ~high-order-bits-of-destWord
		lsrl	#1,d1
		eorl	d1,d0		| Swap light gray and white
		movl	d0,a1@+		| Done with that word
		dbra	d2,HighLInnerLoop	| Go around
| Do last word with rightMask
    	movl	a1@,d1			| d1 <- destWord
	movl	d1,d0			| d0 <- destWord
	movl	d1,d2			| d2 <- destWord
	notl	d1			| d1 <- ~destWord
	andl	d5,d1			| d1 <- ~h-o-b-of-destWord
	lsrl	#1,d1			| d1 <- ~h-o-b-of-destWord >> 1
	eorl	d1,d0			| d0 <- Swapped colors in destWord
	andl	d6,d0			| d0 <- swapped colors inside rightMask
	movl	d6,d1			| d1 <- rightMask
	notl	d1			| d1 <- ~rightMask
	andl	d2,d1			| d1 <- destWord & ~rightMask
	orl	d1,d0			| d0 <- combined halves
	movl	d0,a1@			| *destPtr <- combined halves
| Set up for next row
	addl	a5,a3			| Bump dest row pointer
	dbra	d4,HighLRowLoop		| And go around for next row
HighLDone:
	moveml	sp@+,a0-a6/d0-d7	| restore regs (incl a6)
	unlk	a6
	rts				| Bye!
|
| Special case for two words per line
|
TwoWordHighL:
	movl	d7,d3			| d3 <- leftMask
	notl	d3			| d3 <- ~leftMask
TwoWordHighLoop:
	movl	a3,a1			| And current dest ptr in a1
| Do first word with leftMask
    	movl	a1@,d1			| d1 <- destWord
	movl	d1,d0			| d0 <- destWord
	movl	d1,d2			| d2 <- destWord
	notl	d1			| d1 <- ~destWord
	andl	d5,d1			| d1 <- ~h-o-b-of-destWord
	lsrl	#1,d1			| d1 <- ~h-o-b-of-destWord >> 1
	eorl	d1,d0			| d0 <- Swapped colors in destWord
	andl	d7,d0			| d0 <- swapped colors inside leftMask
	andl	d3,d2			| d2 <- destWord & ~leftMask
	orl	d2,d0			| d0 <- combined halves
	movl	d0,a1@+			| *destPtr++ <- combined halves
| Do second word with rightMask
    	movl	a1@,d1			| d1 <- destWord
	movl	d1,d0			| d0 <- destWord
	movl	d1,d2			| d2 <- destWord
	notl	d1			| d1 <- ~destWord
	andl	d5,d1			| d1 <- ~h-o-b-of-destWord
	lsrl	#1,d1			| d1 <- ~h-o-b-of-destWord >> 1
	eorl	d1,d0			| d0 <- Swapped colors in destWord
	andl	d6,d0			| d0 <- swapped colors inside rightMask
	movl	d6,d1			| d1 <- rightMask
	notl	d1			| d1 <- ~rightMask
	andl	d2,d1			| d1 <- destWord & ~rightMask
	orl	d1,d0			| d0 <- combined halves
	movl	d0,a1@			| *destPtr <- combined halves
| Bump pointers 
	addl	a5,a3			| Bump dest row pointer
	dbra	d4,TwoWordHighLoop	| And go around for next row
	bra	HighLDone
| 
| Special case for one word
|
OneWordHighL:
	movl	d7,d3
	notl	d3			| d5 <- ~leftMask
OneWordHighLoop:
| Do word with leftMask
    	movl	a3@,d1			| d1 <- destWord
	movl	d1,d0			| d0 <- destWord
	movl	d1,d2			| d2 <- destWord
	notl	d1			| d1 <- ~destWord
	andl	d5,d1			| d1 <- ~h-o-b-of-destWord
	lsrl	#1,d1			| d1 <- ~h-o-b-of-destWord >> 1
	eorl	d1,d0			| d0 <- Swapped colors in destWord
	andl	d7,d0			| d0 <- swapped colors inside leftMask
	andl	d3,d2			| d2 <- destWord & ~leftMask
	orl	d2,d0			| d0 <- combined halves
	movl	d0,a3@			| *destPtr++ <- combined halves
| Bump pointers 
	addl	a5,a3			| Bump dest row pointer
	dbra	d4,OneWordHighLoop	| And go around for next row
	bra	HighLDone
|


