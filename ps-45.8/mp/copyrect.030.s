|
|
|	copyrect.s
|	Assembly code to move a rectangle of non-overlapping
|	bits from one place to another.
|
|
| Assembler template for LineOperation structure
destPtr	= 0x0			| First word to be changed in dest
numOtherWords = destPtr+4	| Number of integers after first
leftMask = numOtherWords+4	| Mask for first dest word
rightMask = leftMask + 4	| Mask for last dest word
sourceType = rightMask + 4	| Type of source (always bitmap for us)
sourcePtr = sourceType + 4	| First word of source to be read
leftShift = sourcePtr + 4	| Amount to left shift source by to align
| Rest of a LineOperation is irrelevant
|
| Values for the type field:
alignedType = 3
unalignedType = 1
|
| Calling Sequence:
|
|	CopyRect(lineOpPtr,srcRowBytes,destRowBytes,height)|
|
|
lineOpPtr = 0x08			| Pointer to LineOp structure, above
srcRowBytes = lineOpPtr + 4
destRowBytes = srcRowBytes + 4
height = destRowBytes + 4
|
	.text
||PROC| 04
_CopyRect:
	.globl _CopyRect
	link	a6,#0
	moveml	a0-a6/d0-d7,sp@-
	movl	a6@(destRowBytes),a5
	movl	a6@(srcRowBytes),a4
	movl	a6@(height),d4
	beq		CopyDone		| catch a potential runaway
	subl	#1,d4
	movl	a6@(lineOpPtr),a6
| Now load all elements from LineOperation structure
	movl	a6@(destPtr),a3
	movl	a6@(sourcePtr),a2
	movl	a6@(numOtherWords),d3	| Save the original value
	cmpl	#3,a6@(sourceType)	| Is source aligned with dest?
	beq		AlignedCopy		| It is, go the easy route
	movl	a6@(leftShift),d7
	movl	#32,d6			| Calculate right shift from left
	subl	d7,d6
	movl	a6@(leftMask),d2	| Test use of first source word
	lsrl	d7,d2			| d2 <- (leftMask >> leftShift)
	beq		NoFirst
	cmpl	#1,d3			| How many words per line?
	beqs	ThreeIntoTwoUnaligned	| Two dest words, use 3 source words
	blt		Use1stIntoOneUnaligned	| One dest word, use 1st source word
	subl	#2,d3			| More than two, pre-subtract for dbra
RowLoop:
	movl	a2,a0			| Put current source row pointer in a0
	movl	a3,a1			| And current dest ptr in a1
| Do first partial word
	movl	a0@+,d1			| Get first word of source
	lsll	d7,d1			| Align right half of it with dest
    movl	a0@+,d5			| Get next word of source
   	movl	d5,d0			| Copy it
	lsrl	d6,d0			| Align left half of it with dest
	orl		d0,d1			| Combine halves
	movl	a6@(leftMask),d0	| d0 <- leftMask
	andl	d0,d1			| d1 <- leftMask & src1
	notl	d0				| d0 <- ~leftMask
	andl	a1@,d0			| d0 <- *destPtr & ~leftMask
	orl		d1,d0			| Combine old and new
	movl	d0,a1@+			| *desPtr++ <- d0
| Set up for loop through whole inner words
   	movl	d3,d2			
InnerLoop:
	movl	d5,d1			| d1 <- src0
	lsll	d7,d1			| Move over to left half of dest
	movl	a0@+,d5			| Grab next word of source
	movl	d5,d0			| d0 <- this word of source
	lsrl	d6,d0			| d0 <- right part of dest word
	orl		d1,d0			| d0 <- all of dest word
	movl	d0,a1@+			| Write dest
	dbra	d2,InnerLoop	| Go around

	movl	a0@,d1			| Get last word of source
	lsrl	d6,d1			| d1 <- *sCur>>rShift
	lsll	d7,d5			| d5 <- src0<<shift
	orl		d5,d1			| d1 <- final dest word
	movl	a6@(rightMask),d0	| Grab right mask into d0
	andl	d0,d1			| d1 <- (*sCur)>>rShift & rightMask
	notl	d0				| d0 <- ~rightMask
	andl	a1@,d0			| d0 <- surviving part of dest
	orl		d1,d0			| d0 <- final result *dest
	movl	d0,a1@			| put last word back
	addl	a4,a2			| Bump source row pointer
	addl	a5,a3			| Bump dest row pointer
	dbra	d4,RowLoop		| And go around for next row
CopyDone:
	moveml	sp@+,a0-a6/d0-d7	| restore regs (incl a6)
	unlk	a6
	rts						| Bye!
|
| Two unaligned destination words per line from three source words
|
ThreeIntoTwoUnaligned:
	movl	a6@(leftMask),d2	| Use those regs!
	movl	a6@(rightMask),d3
ThreeIntoTwoWordLoop:
	movl	a2,a0			| Put current source row pointer in a0
	movl	a3,a1			| And current dest ptr in a1
| Do first partial word
	movl	a0@+,d1			| Get first word of source
	lsll	d7,d1			| Align right half of it with dest
    movl	a0@+,d5			| Get next word of source
    movl	d5,d0			| Copy it
	lsrl	d6,d0			| Align left half of it with dest
	orl		d0,d1			| Combine halves
	movl	d2,d0			| d0 <- leftMask
	andl	d0,d1			| d1 <- leftMask & src1
	notl	d0				| d0 <- ~leftMask
	andl	a1@,d0			| d0 <- *destPtr & ~leftMask
	orl		d1,d0			| Combine old and new
	movl	d0,a1@+			| *desPtr++ <- d0
| Do other word of row
	movl	a0@,d1			| Get last word of source
	lsrl	d6,d1			| d1 <- *sCur>>rShift
	lsll	d7,d5			| d5 <- src0<<shift
	orl		d5,d1			| d1 <- final dest word
	movl	d3,d0			| Grab right mask into d0
	andl	d0,d1			| d1 <- (*sCur)>>rShift & rightMask
	notl	d0				| d0 <- ~rightMask
	andl	a1@,d0			| d0 <- surviving part of dest
	orl		d1,d0			| d0 <- final result *dest
	movl	d0,a1@			| put last word back
| Set up for next row
	addl	a4,a2			| Bump source row pointer
	addl	a5,a3			| Bump dest row pointer
	dbra	d4,ThreeIntoTwoWordLoop	| And go around for next row
	bra	CopyDone
|
| One unaligned destination word per line & 1st source word must be used
|
Use1stIntoOneUnaligned:
	movl	a6@(leftMask),d3	| Use those regs!
	movl	d3,d2			| Test use of second source word
	lsll	d6,d2			| d2 <- (leftMask << rightShift)
	beq		OneL2OneUnalignedCopy	| Even better special case!
	movl	d3,d2			| d2 <- leftMask
	notl	d3				| d3 <- ~leftMask
TwoIntoOneWordLoop:
| Do first partial word
	movl	a2@,d1			| Get first word of source
	lsll	d7,d1			| Align right half of it with dest
    movl	a2@(0x04),d0	| Get second word of source
	lsrl	d6,d0			| Align left half of it with dest
	orl		d0,d1			| Combine halves
	andl	d2,d1			| d1 <- leftMask & src1
	movl	d3,d0			| d0 <- ~leftMask
	andl	a3@,d0			| d0 <- *destPtr & ~leftMask
	orl		d1,d0			| Combine old and new
	movl	d0,a3@			| *desPtr++ <- d0
| Set up for next row
	addl	a4,a2			| Bump source row pointer
	addl	a5,a3			| Bump dest row pointer
	dbra	d4,TwoIntoOneWordLoop	| And go around for next row
	bra		CopyDone
OneL2OneUnalignedCopy:
	movl	d3,d2			| d2 <- leftMask
	notl	d3				| d3 <- ~leftMask
OneL2OneWordLoop:
| Do first partial word
	movl	a2@,d1			| Get first word of source
	lsll	d7,d1			| Align right half of it with dest
	andl	d2,d1			| d1 <- leftMask & src1
	movl	d3,d0			| d0 <- ~leftMask
	andl	a3@,d0			| d0 <- *destPtr & ~leftMask
	orl		d1,d0			| Combine old and new
	movl	d0,a3@			| *desPtr++ <- d0
| Set up for next row
	addl	a4,a2			| Bump source row pointer
	addl	a5,a3			| Bump dest row pointer
	dbra	d4,OneL2OneWordLoop	| And go around for next row
	bra	CopyDone
|
| Special cases for unaligned source with useless 1st word in each line
|
NoFirst:
	addl	#4,a2			| Bump source to first useful word
	cmpl	#1,d3			| How many words per line?
	beqs	TwoIntoTwoUnalignedCopy	| Exactly two, use special case
	blt		OneR2OneUnalignedCopy	| Exactly one, use special case
	subl	#2,d3			| More than two, pre-subtract for dbra
No1stRowLoop:
	movl	a2,a0			| Put current source row pointer in a0
	movl	a3,a1			| And current dest ptr in a1
| Do first partial word
	movl	a0@+,d5			| Get first word of source
	movl	d5,d1			| Copy it
	lsrl	d6,d1			| Align left half of it with dest
	movl	a6@(leftMask),d0	| d0 <- leftMask
	andl	d0,d1			| d1 <- leftMask & src1
	notl	d0				| d0 <- ~leftMask
	andl	a1@,d0			| d0 <- *destPtr & ~leftMask
	orl		d1,d0			| Combine old and new
	movl	d0,a1@+			| *desPtr++ <- d0
| Set up for loop through whole inner words
    movl	d3,d2			
No1stInner:
	movl	d5,d1			| d1 <- src0
	lsll	d7,d1			| Move over to left half of dest
	movl	a0@+,d5			| Grab next word of source
	movl	d5,d0			| d0 <- this word of source
	lsrl	d6,d0			| d0 <- right part of dest word
	orl		d1,d0			| d0 <- all of dest word
	movl	d0,a1@+			| Write dest
	dbra	d2,No1stInner	| Go around

	movl	a0@,d1			| Get last word of source
	lsrl	d6,d1			| d1 <- *sCur>>rShift
	lsll	d7,d5			| d5 <- src0<<shift
	orl		d5,d1			| d1 <- final dest word
	movl	a6@(rightMask),d0	| Grab right mask into d0
	andl	d0,d1			| d1 <- (*sCur)>>rShift & rightMask
	notl	d0				| d0 <- ~rightMask
	andl	a1@,d0			| d0 <- surviving part of dest
	orl		d1,d0			| d0 <- final result *dest
	movl	d0,a1@			| put last word back
No1stRowDone:
	addl	a4,a2			| Bump source row pointer
	addl	a5,a3			| Bump dest row pointer
	dbra	d4,No1stRowLoop		| And go around for next row
	bra	CopyDone
|
| Two unaligned words per line, 1st src word only used in 1st dest word
|
TwoIntoTwoUnalignedCopy:
	movl	a6@(leftMask),d2	| Use those regs!
	movl	a6@(rightMask),d3
TwoIntoTwoLoop:
	movl	a2,a0			| Put current source row pointer in a0
	movl	a3,a1			| And current dest ptr in a1
| Do first partial word
    movl	a0@+,d5			| Get first word of source
    movl	d5,d1			| Copy it
	lsrl	d6,d1			| Align left half of it with dest
	movl	d2,d0			| d0 <- leftMask
	andl	d0,d1			| d1 <- leftMask & src1
	notl	d0				| d0 <- ~leftMask
	andl	a1@,d0			| d0 <- *destPtr & ~leftMask
	orl		d1,d0			| Combine old and new
	movl	d0,a1@+			| *desPtr++ <- d0
| Do other word of row
	movl	a0@,d1			| Get last word of source
	lsrl	d6,d1			| d1 <- *sCur>>rShift
	lsll	d7,d5			| d5 <- src0<<shift
	orl		d5,d1			| d1 <- final dest word
	movl	d3,d0			| Grab right mask into d0
	andl	d0,d1			| d1 <- (*sCur)>>rShift & rightMask
	notl	d0			| d0 <- ~rightMask
	andl	a1@,d0			| d0 <- surviving part of dest
	orl		d1,d0			| d0 <- final result *dest
	movl	d0,a1@			| put last word back
| Set up for next row
	addl	a4,a2			| Bump source row pointer
	addl	a5,a3			| Bump dest row pointer
	dbra	d4,TwoIntoTwoLoop	| And go around for next row
	bra	CopyDone
|
| Special case for one unaligned word per line into one destination word
|
OneR2OneUnalignedCopy:
	movl	a6@(leftMask),d2	| Use those regs!
	movl	d2,d3
	notl	d3
OneR2OneLoop:
| Do first partial word
	movl	a2@,d1			| Get first word of source
	lsrl	d6,d1			| Align left half of it with dest
	andl	d2,d1			| d1 <- leftMask & src1
	movl	d3,d0			| d0 <- ~leftMask
	andl	a3@,d0			| d0 <- *destPtr & ~leftMask
	orl		d1,d0			| Combine old and new
	movl	d0,a3@			| *desPtr++ <- d0
| Set up for next row
	addl	a4,a2			| Bump source row pointer
	addl	a5,a3			| Bump dest row pointer
	dbra	d4,OneR2OneLoop		| And go around for next row
	bra	CopyDone
|
| Special cases for aligned copies
|
AlignedCopy:
	movl	a6@(leftMask),d7	| Pick up masks into unused regs
	movl	d7,d5
	notl	d5				| d5 <- ~leftMask
	movl	a6@(rightMask),d6
	cmpl	#1,d3			| How many words per line?
	beqs	TwoAlignedWordCopy	| Exactly two
	blts	OneAlignedWordCopy	| Exactly one
	subl	#2,d3			| Pre-subtract for dbra
AlignedRowLoop:
	movl	a2,a0			| Put current source row pointer in a0
	movl	a3,a1			| And current dest ptr in a1
    movl	a0@+,d1			| Pick up first word of source
	andl	d7,d1			| d1 <- leftMask & src1
	movl	d5,d0			| d0 <- ~leftMask
	andl	a1@,d0			| d0 <- *destPtr & ~leftMask
	orl		d1,d0
	movl	d0,a1@+			| *desPtr++ <- d0
| Set up for Loop through inner words
    movl	d3,d2
AlignedInnerLoop:
	movl	a0@+,a1@+		| Grab next word of source
	dbra	d2,AlignedInnerLoop	| Go around

	movl	a0@,d1			|
	andl	d6,d1			| d1 <- (*sCur) & rightMask
	movl	d6,d0			| d1 <- rightMask
	notl	d0				| d0 <- ~rightMask
	andl	a1@,d0			| d0 <- surviving part of dest
	orl		d1,d0			| d0 <- final result *dest
	movl	d0,a1@			| put last word back
| Set up for next row
	addl	a4,a2			| Bump source row pointer
	addl	a5,a3			| Bump dest row pointer
	dbra	d4,AlignedRowLoop	| And go around for next row
	bra		CopyDone		| and go back upstairs to clean up
|
| Special case for two words per line
|
TwoAlignedWordCopy:
	movl	d7,d3
	notl	d3				| d3 <- ~leftMask
	movl	d6,d2
	notl	d2				| d2 <- ~rightMask
TwoWordRowLoop:
	movl	a2,a0			| Put current source row pointer in a0
	movl	a3,a1			| And current dest ptr in a1
| Do first word with leftMask
   	movl	a0@+,d1			| Pick up first word of source
	andl	d7,d1			| d1 <- leftMask & src1
	movl	d3,d0			| d0 <- ~leftMask
	andl	a1@,d0			| d0 <- *destPtr & ~leftMask
	orl		d1,d0			| d0 <- final result *dest
	movl	d0,a1@+			| put first word back
| Do second word with rightMask
	movl	a0@,d1			| Pick up the second word of source
	andl	d6,d1			| d1 <- (*sCur) & rightMask
	movl	d2,d0			| d0 <- ~rightMask
	andl	a1@,d0			| d0 <- surviving part of dest
	orl		d1,d0			| d0 <- final result *dest
	movl	d0,a1@			| put last word back
| Bump pointers 
	addl	a4,a2			| Bump source row pointer
	addl	a5,a3			| Bump dest row pointer
	dbra	d4,TwoWordRowLoop	| And go around for next row
	bra	CopyDone
| 
| Special case for one word
|
OneAlignedWordCopy:
	movl	d7,d6			| Use that reg!
	notl	d6				| d6 <- ~leftMask
OneWordRowLoop:
| Do first word with leftMask
    movl	a2@,d1			| Pick up first word of source
	andl	d7,d1			| d1 <- leftMask & src1
	movl	d6,d0			| d0 <- ~leftMask
	andl	a3@,d0			| d0 <- *destPtr & ~leftMask
	orl		d1,d0
	movl	d0,a3@			| *destPtr <- d0
| Bump pointers 
	addl	a4,a2			| Bump source row pointer
	addl	a5,a3			| Bump dest row pointer
	dbra	d4,OneWordRowLoop	| And go around for next row
	bra	CopyDone
|



		
